// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI7 client, for capturing TTI_DESCRIBE (0x2B) traffic against a
// real Oracle backend: olog, oopen, oparse, then - depending on --describe=
// - odefin/oexec/ofen, then odescr, then oclose, ologof. Nothing else.
//
//   ./oci7describe SID \
//     [--describe=parse|exec|fetch|outofrange|concurrent|midfetch] \
//                    [--defines=LIST] [USER PASSWORD [QUERY]]
//
// oci7.cpp in this directory is the full OCI7 protocol test, including a
// "Concurrent Cursors" section that forks a second connection for its
// Authentication coverage; a wire capture of it is noisy. This program never
// forks and never touches Authentication, so a capture of it holds only the
// login, one oparse, and whichever describe variant was asked for - small
// enough to read by hand or feed straight to oradecode.
//
// The six --describe= variants exist because each one has to be captured on
// its own:
//   parse        odescr right after oparse, before any oexec
//   exec         odescr after oexec, before the first ofen
//   fetch        odescr after at least one ofen (#9599 - a describe must not
//                rewind a cursor that is mid-fetch)
//   outofrange   odescr past the end of the select list, to see what a real
//                server answers with (assertColumnCount in oci7.cpp expects
//                ORA-01007, unverified against a live server - see #9654)
//   concurrent   a second cursor is odescr'd while the first cursor's fetch
//                is still in progress, so the capture shows whether the
//                server keeps each cursor's describe state separate
//   midfetch     oci7.cpp's "odescr - mid-fetch" section, call for call
//                (#9810) - four real columns with only column 1 defined,
//                one ofen, then odescr over columns 1-4, one past the end,
//                and 1-4 again, then the two ofen calls that follow. fetch
//                above stops at the describe; this one is the only variant
//                that captures what a real server sends for a fetch issued
//                after a describe, which is where the real oci7 client
//                segfaults against sqlrelay
//
// QUERY, when given, only applies to parse/exec/fetch/outofrange - it always
// has to select three columns, since outofrange describes column 4. The
// concurrent and midfetch variants ignore QUERY; they open their own fixed
// queries, the same way oci7.cpp's Concurrent Cursors and Fetch sections do.
//
// --defines= applies to midfetch alone, and takes the column positions to
// odefin as single digits 1 through 4 - "1" (the default, and what oci7.cpp
// does), "3", "12", "1234". It is there because a real server sends back only
// the columns the client defined (#9810), and the captures on file all define
// every column, so nothing on file says whether the server keys that off the
// positions the client named or just off how many it named. "--defines=3" and
// "--defines=1" against the same query answer that in one run each.

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>
#include <config.h>

// see oci7.cpp for why this wrap, this include order, and leaving ocikpr.h
// out are all required
extern "C" {
	#include <oratypes.h>
	#include <ocidfn.h>
	#include <ociapr.h>
}

const char	*user="testuser";
const char	*password="testpassword";
const char	*sid=NULL;

Lda_Def		lda;
ub4		hda[256];

// print the ORA number a call left in the cursor, if any
static void printError(const char *what, Cda_Def *cursor) {
	stdoutput.printf("%s failed: ORA-%05d\n",what,(int)cursor->rc);
}

// run one OCI7 call, print what happened, and report whether it succeeded
static bool run(const char *what, Cda_Def *cursor, sword result) {
	if (result) {
		printError(what,cursor);
		return false;
	}
	stdoutput.printf("%s ok\n",what);
	return true;
}

// open a cursor against the session in lda
static bool openCursor(const char *what, Cda_Def *cursor) {
	bytestring::zero(cursor,sizeof(Cda_Def));
	return run(what,cursor,oopen(cursor,&lda,(text *)0,-1,-1,(text *)0,-1));
}

// describe one column and print what came back, or the ORA number if the
// position doesn't exist
static void describeColumn(Cda_Def *cursor, sword pos) {

	sb4	dbsize=0;
	sb2	dbtype=0;
	sb1	cbuf[128];
	sb4	cbufl=(sb4)sizeof(cbuf);
	sb4	dsize=0;
	sb2	precision=0;
	sb2	scale=0;
	sb2	nullok=0;
	bytestring::zero(cbuf,sizeof(cbuf));

	stdoutput.printf("odescr - column %d\n",(int)pos);
	if (odescr(cursor,pos,&dbsize,&dbtype,cbuf,&cbufl,
				&dsize,&precision,&scale,&nullok)) {
		printError("odescr",cursor);
		return;
	}

	// odescr does not null terminate the name - cbufl comes back as its
	// length
	if (cbufl>=0 && cbufl<(sb4)sizeof(cbuf)) {
		cbuf[cbufl]='\0';
	} else {
		cbuf[sizeof(cbuf)-1]='\0';
	}
	stdoutput.printf("  name=%s dbtype=%d dbsize=%d dsize=%d "
				"precision=%d scale=%d nullok=%d\n",
				(char *)cbuf,(int)dbtype,(int)dbsize,
				(int)dsize,(int)precision,(int)scale,
				(int)nullok);
}

// oparse a select that always returns three columns, so --describe=outofrange
// has a column 4 to reach past
static bool parseQuery(Cda_Def *cursor, const char *query) {
	return run("oparse",cursor,
			oparse(cursor,(text *)query,(sb4)-1,0,(ub4)2));
}

// odefin all three columns as strings, the way every define in oci7.cpp does
static bool defineColumns(Cda_Def *cursor, char buf[3][64],
				sb2 ind[3], ub2 retlen[3], ub2 retcode[3]) {
	for (sword pos=1; pos<=3; pos++) {
		bytestring::zero(buf[pos-1],64);
		char	what[32];
		charstring::printf(what,sizeof(what),"odefin - column %d",
					(int)pos);
		if (!run(what,cursor,
				odefin(cursor,pos,(ub1 *)buf[pos-1],64,
					SQLT_STR,-1,&ind[pos-1],(text *)0,
					-1,-1,&retlen[pos-1],&retcode[pos-1]))) {
			return false;
		}
	}
	return true;
}

int main(int argc, char **argv) {

	const char	*variant="parse";
	const char	*defines="1";
	const char	*query="select 1 as num, 'two' as txt, "
					"sysdate as dt from dual";
	int		positional=0;

	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"--describe=",11)) {
			variant=argv[i]+11;
		} else if (!charstring::compare(argv[i],"--defines=",10)) {
			defines=argv[i]+10;
		} else if (positional==0) {
			sid=argv[i];
			positional++;
		} else if (positional==1) {
			user=argv[i];
			positional++;
		} else if (positional==2) {
			password=argv[i];
			positional++;
		} else if (positional==3) {
			query=argv[i];
			positional++;
		}
	}

	if (!sid) {
		stdoutput.printf("usage: %s SID "
				"[--describe=parse|exec|fetch|outofrange|"
				"concurrent|midfetch] [--defines=LIST] "
				"[USER PASSWORD [QUERY]]\n",
				argv[0]);
		return 1;
	}

	stdoutput.printf("logging in to %s as %s...\n",sid,user);
	bytestring::zero(&lda,sizeof(lda));
	bytestring::zero(hda,sizeof(hda));
	if (!run("olog",&lda,
			olog(&lda,(ub1 *)hda,
				(text *)user,(sword)-1,
				(text *)password,(sword)-1,
				(text *)sid,(sword)-1,
				(ub4)OCI_LM_DEF))) {
		return 1;
	}

	if (!charstring::compare(variant,"concurrent")) {

		// cursor A: a multi-row query, fetched one row at a time so
		// its fetch is still in progress when B is described - the
		// same shape as oci7.cpp's "odescr - on B while A is
		// mid-ping-pong" case
		Cda_Def	curA;
		Cda_Def	curB;
		if (!openCursor("oopen - cursor A",&curA) ||
			!openCursor("oopen - cursor B",&curB)) {
			ologof(&lda);
			return 1;
		}

		const char	*queryA="select level as num from dual "
					"connect by level<=5 order by 1";
		const char	*queryB="select 1 as num, 'two' as txt, "
					"sysdate as dt from dual";
		if (!parseQuery(&curA,queryA) || !parseQuery(&curB,queryB)) {
			oclose(&curA);
			oclose(&curB);
			ologof(&lda);
			return 1;
		}

		char	numA[64];
		sb2	indA=0;
		ub2	lenA=0;
		ub2	codeA=0;
		bytestring::zero(numA,sizeof(numA));
		if (!run("odefin - cursor A",&curA,
				odefin(&curA,1,(ub1 *)numA,(sword)sizeof(numA),
					SQLT_STR,-1,&indA,(text *)0,-1,-1,
					&lenA,&codeA)) ||
			!run("oexec - cursor A",&curA,oexec(&curA))) {
			oclose(&curA);
			oclose(&curB);
			ologof(&lda);
			return 1;
		}

		stdoutput.printf("ofen - cursor A, first row\n");
		if (!run("ofen",&curA,ofen(&curA,1))) {
			oclose(&curA);
			oclose(&curB);
			ologof(&lda);
			return 1;
		}
		stdoutput.printf("  numA=%s\n",numA);

		// A's fetch is now mid-flight (rows 2-5 unread) while B is
		// only parsed - describe B here
		describeColumn(&curB,1);

		run("oclose - cursor A",&curA,oclose(&curA));
		run("oclose - cursor B",&curB,oclose(&curB));

	} else if (!charstring::compare(variant,"parse")) {

		Cda_Def	cda;
		if (!openCursor("oopen",&cda) || !parseQuery(&cda,query)) {
			ologof(&lda);
			return 1;
		}
		describeColumn(&cda,1);
		describeColumn(&cda,2);
		describeColumn(&cda,3);
		run("oclose",&cda,oclose(&cda));

	} else if (!charstring::compare(variant,"outofrange")) {

		Cda_Def	cda;
		if (!openCursor("oopen",&cda) || !parseQuery(&cda,query)) {
			ologof(&lda);
			return 1;
		}
		// query has three columns - position 4 doesn't exist
		describeColumn(&cda,4);
		run("oclose",&cda,oclose(&cda));

	} else if (!charstring::compare(variant,"exec")) {

		Cda_Def	cda;
		char	buf[3][64];
		sb2	ind[3];
		ub2	retlen[3];
		ub2	retcode[3];
		bytestring::zero(ind,sizeof(ind));
		bytestring::zero(retlen,sizeof(retlen));
		bytestring::zero(retcode,sizeof(retcode));
		if (!openCursor("oopen",&cda) || !parseQuery(&cda,query) ||
			!defineColumns(&cda,buf,ind,retlen,retcode) ||
			!run("oexec",&cda,oexec(&cda))) {
			oclose(&cda);
			ologof(&lda);
			return 1;
		}
		describeColumn(&cda,1);
		describeColumn(&cda,2);
		describeColumn(&cda,3);
		run("oclose",&cda,oclose(&cda));

	} else if (!charstring::compare(variant,"fetch")) {

		Cda_Def	cda;
		char	buf[3][64];
		sb2	ind[3];
		ub2	retlen[3];
		ub2	retcode[3];
		bytestring::zero(ind,sizeof(ind));
		bytestring::zero(retlen,sizeof(retlen));
		bytestring::zero(retcode,sizeof(retcode));
		if (!openCursor("oopen",&cda) || !parseQuery(&cda,query) ||
			!defineColumns(&cda,buf,ind,retlen,retcode) ||
			!run("oexec",&cda,oexec(&cda)) ||
			!run("ofen",&cda,ofen(&cda,1))) {
			oclose(&cda);
			ologof(&lda);
			return 1;
		}
		stdoutput.printf("  row: %s / %s / %s\n",
					buf[0],buf[1],buf[2]);
		describeColumn(&cda,1);
		describeColumn(&cda,2);
		describeColumn(&cda,3);
		run("oclose",&cda,oclose(&cda));

	} else if (!charstring::compare(variant,"midfetch")) {

		// oci7.cpp's "odescr - mid-fetch" section, call for call.
		// protocoltesttable is the table that section's own run
		// creates - four columns, three rows, the third one all
		// nulls but its number.  run oci7 once first if it isn't
		// there
		const char	*midfetchquery=
					"select * from protocoltesttable "
					"order by testnumber";

		Cda_Def	cda;
		char	buf[4][64];
		sb2	ind[4];
		ub2	retlen[4];
		ub2	retcode[4];
		bytestring::zero(buf,sizeof(buf));
		bytestring::zero(ind,sizeof(ind));
		bytestring::zero(retlen,sizeof(retlen));
		bytestring::zero(retcode,sizeof(retcode));

		if (!openCursor("oopen",&cda) ||
			!parseQuery(&cda,midfetchquery)) {
			ologof(&lda);
			return 1;
		}

		// only the columns --defines= names get odefin'd - column 1
		// alone by default, the way that section defines it.  the
		// undefined ones get described but never fetched into
		for (const char *d=defines; *d; d++) {

			if (*d<'1' || *d>'4') {
				continue;
			}
			sword	pos=(sword)(*d-'0');

			char	what[32];
			charstring::printf(what,sizeof(what),
						"odefin - column %d",(int)pos);
			if (!run(what,&cda,
					odefin(&cda,pos,(ub1 *)buf[pos-1],64,
						SQLT_STR,-1,&ind[pos-1],
						(text *)0,-1,-1,
						&retlen[pos-1],
						&retcode[pos-1]))) {
				oclose(&cda);
				ologof(&lda);
				return 1;
			}
		}

		if (!run("oexec",&cda,oexec(&cda)) ||
			!run("ofen - row 1",&cda,ofen(&cda,1))) {
			oclose(&cda);
			ologof(&lda);
			return 1;
		}
		stdoutput.printf("  row 1: %s / %s / %s / %s\n",
					buf[0],buf[1],buf[2],buf[3]);

		// every column, then one past the end, then every column
		// again - the ORA-01007 in the middle is what drops the
		// client's cached select list, so the second pass has to go
		// back to the wire
		describeColumn(&cda,1);
		describeColumn(&cda,2);
		describeColumn(&cda,3);
		describeColumn(&cda,4);
		describeColumn(&cda,5);
		describeColumn(&cda,1);
		describeColumn(&cda,2);
		describeColumn(&cda,3);
		describeColumn(&cda,4);

		// and the fetches that follow the describe - the state no
		// other variant reaches, and where the real client segfaults
		// against sqlrelay (#9810)
		if (run("ofen - row 2",&cda,ofen(&cda,1))) {
			stdoutput.printf("  row 2: %s / %s / %s / %s\n",
					buf[0],buf[1],buf[2],buf[3]);
		}
		if (run("ofen - row 3",&cda,ofen(&cda,1))) {
			stdoutput.printf("  row 3: %s / %s / %s / %s\n",
					buf[0],buf[1],buf[2],buf[3]);
		}
		// past the last row - an ORA-01403 is the right answer here
		run("ofen - past the end",&cda,ofen(&cda,1));
		run("oclose",&cda,oclose(&cda));

	} else {
		stdoutput.printf("unknown --describe= variant: %s\n",variant);
		ologof(&lda);
		return 1;
	}

	run("ologof",&lda,ologof(&lda));

	stdoutput.printf("done\n");
	return 0;
}
