// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI7 client that fetches ONE column and prints exactly what came
// back: olog, oopen, oparse, odefin, oexec, ofen, oclose, ologof.  Nothing
// else.
//
//   ./oci7datestr SID [--query=QUERY] [--dat] [--bin] [--bindstr]
//				[--bufsize=N] [--alter=NLSDATEFORMAT]
//				[USER PASSWORD]
//
//   --query=    what to select.  one column.  default "select sysdate from
//               dual"
//   --dat       odefin the column SQLT_DAT into a 7 byte buffer instead of
//               SQLT_STR into a character buffer, so the two forms of the
//               same fetch can be diffed against each other
//   --bin       odefin the column SQLT_BIN instead of SQLT_STR, so the raw
//               wire bytes come back with no client-side charset
//               reinterpretation - the control #10273's capture is diffed
//               against.  --dat wins if both are given
//   --bindstr   skip --query entirely and instead obndrv the literal byte
//               sequence 61 c3 a9 e2 82 ac as a SQLT_STR IN bind on
//               "select :v from dual", then odefin/oexec/ofen it straight
//               back - the client-to-server half of #10273's capture, in a
//               cursor and code path of its own
//   --bufsize=  the SQLT_STR/SQLT_BIN buffer's size, default 64.  the size
//               the client puts in the define descriptor is one less than
//               this
//   --alter=    run "alter session set NLS_DATE_FORMAT='...'" before the
//               select, to show whether the answer moves with the session
//
// It exists for #9974: no capture on file had an OCI7 client asking a real
// server to convert a DATE column to text.  Every DATE capture in samples/
// defines the column SQLT_DAT (binary), so what a real server puts on the
// wire when the client asks for SQLT_STR instead was unknown - and
// putField()'s ORACLE_TYPE_DATE arm in src/protocols/oracle.cpp answers
// every define with the 7 byte binary form regardless of what was asked
// for.  See samples/9974-dev-oci23api7-native-datestrfetch-realserver
// .oraproxy and its README entry for what this program captured.
//
// --bin and --bindstr were added for #10273: a real OCI7 client corrupted
// multibyte text defined SQLT_STR, independent of NLS_LANG.  Traced to the
// module always declaring charset 31 (WE8ISO8859P1) for a verifiertype="9i"
// listener regardless of the backend's real charset - not a client
// limitation.  This program is the minimal fetch and bind side of that
// capture.  To reproduce the fetch side, pass a --query that builds the test
// value from ASCII with chr(), so the client never has to send a multibyte
// character in the statement text itself:
//
//   --query="select 'a'||chr(50089)||chr(14844588) from dual"
//
// On an AL32UTF8 database chr()'s argument is the byte value chr() encodes
// to, not the Unicode code point - chr(233)||chr(8364) does NOT give U+00E9/
// U+20AC there, it gives the single byte 0x20 (space) and 0xac.  50089 and
// 14844588 are the decimal values of the UTF-8 byte pairs/triples for those
// two code points (c3 a9 and e2 82 ac) read as big-endian integers.  Check
// server-side with "select dump('a'||chr(50089)||chr(14844588)) from dual",
// expecting the byte dump 97,195,169,226,130,172 (hex 61 c3 a9 e2 82 ac).
//
// --bin is a raw-bytes control only against a RAW column (e.g. one built
// with utl_raw.cast_to_raw(...)) - a real server answers ORA-01465 if it's
// used against a VARCHAR2 column, since it then parses the text as hex.
//
// Modeled on oci7bind.cpp - same includes, same login sequence, same style.
//
// This program is deliberately NOT in this directory's Makefile.  configure's
// FW_CHECK_OCI7 probe wants a full ORACLE_HOME/rdbms/demo layout, so it does
// not detect an Instant Client, and HAVE_OCI7 is unset on the machines that
// have one - but the Instant Client's libclntsh still exports the legacy OCI7
// entry points, so this compiles by hand against it:
//
//   g++ -Wall -std=gnu++98 -I<sqlrelay top> -I/usr/local/firstworks/include
//	-I/usr/include/oracle/23/client64 -o oci7datestr oci7datestr.cpp
//	-Wl,/usr/lib/oracle/23/client64/lib/libclntsh.so.23.1
//	-L/usr/lib/oracle/23/client64/lib -lnnz
//	-Wl,/usr/lib/oracle/23/client64/lib/libclntshcore.so.23.1
//	-Wl,-rpath,/usr/lib/oracle/23/client64/lib
//	-L/usr/local/firstworks/lib -lrudiments -lpthread -ldl
//	-Wl,-rpath,/usr/local/firstworks/lib
//
// (all one command - the line breaks above are the comment's, not the
// shell's)

#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
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

// the largest --bufsize this program will odefin, and the size of the buffer
// it odefins into
const int	maxbufsize=64;

// print the ORA number a call left in the cursor, and the rest of the
// cursor's error fields - same fields, and for the same reasons, as
// oci7bind.cpp's own printError()
static void printError(const char *what, Cda_Def *cursor) {
	stdoutput.printf("%s failed: ORA-%05d "
			"(v2_rc=%d fc=%d peo=%d ose=%d)\n",
			what,(int)cursor->rc,(int)cursor->v2_rc,
			(int)cursor->fc,(int)cursor->peo,(int)cursor->ose);
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

static bool openCursor(const char *what, Cda_Def *cursor) {
	bytestring::zero(cursor,sizeof(Cda_Def));
	return run(what,cursor,oopen(cursor,&lda,(text *)0,-1,-1,(text *)0,-1));
}

static bool parse(const char *what, Cda_Def *cursor, const char *query) {
	stdoutput.printf("%s: %s\n",what,query);
	return run(what,cursor,
			oparse(cursor,(text *)query,(sb4)-1,0,(ub4)2));
}

// oparse plus oexec on a cursor of its own, for the alter session
static bool execImmediate(const char *what, const char *query) {

	Cda_Def	cda;
	if (!openCursor("oopen",&cda)) {
		return false;
	}
	bool	ok=parse(what,&cda,query) && run("oexec",&cda,oexec(&cda));
	oclose(&cda);
	return ok;
}

// bind one program variable by name - same shape as oci7bind.cpp's own
// bind(), sqlvl -1 for "null terminated, measure it"
static bool bind(Cda_Def *cursor, const char *name,
			ub1 *value, sword valuesize, sword type, sb2 *ind) {
	char	what[64];
	charstring::printf(what,sizeof(what),"obndrv - %s",name);
	return run(what,cursor,
			obndrv(cursor,(text *)name,-1,value,valuesize,
					type,-1,ind,(text *)0,-1,-1));
}

// #10273's client-to-server capture: obndrv the literal byte sequence 61 c3
// a9 e2 82 ac (ascii 'a' + the utf-8 bytes of U+00E9 and U+20AC) as a
// SQLT_STR IN bind, then odefin/oexec/ofen it straight back on a cursor of
// its own - kept apart from the --query fetch path above so the two
// directions never share a cursor or a define call
static bool bindStrVariant(bool dat, bool bin, int bufsize) {

	Cda_Def	cda;
	if (!openCursor("oopen - bindstr",&cda) ||
			!parse("oparse - bindstr",&cda,"select :v from dual")) {
		return false;
	}

	// built byte by byte rather than as a string literal, so the source
	// file itself never has to carry a multibyte character
	ub1	bindvalue[7];
	bindvalue[0]=0x61;				// 'a'
	bindvalue[1]=0xC3; bindvalue[2]=0xA9;		// U+00E9
	bindvalue[3]=0xE2; bindvalue[4]=0x82; bindvalue[5]=0xAC; // U+20AC
	bindvalue[6]=0x00;

	sb2	bindind=0;
	if (!bind(&cda,":v",bindvalue,(sword)sizeof(bindvalue),
					SQLT_STR,&bindind)) {
		oclose(&cda);
		return false;
	}

	char	buf[maxbufsize];
	sb2	ind=0;
	ub2	retlen=0;
	ub2	retcode=0;
	bytestring::zero(buf,sizeof(buf));
	if (!run("odefin - bindstr",&cda,
			odefin(&cda,1,(ub1 *)buf,
				(sword)((dat)?7:bufsize),
				(dat)?SQLT_DAT:((bin)?SQLT_BIN:SQLT_STR),-1,
				&ind,(text *)0,-1,-1,
				&retlen,&retcode))) {
		oclose(&cda);
		return false;
	}

	bool	ok=true;
	if (run("oexec - bindstr",&cda,oexec(&cda)) &&
			run("ofen - bindstr",&cda,ofen(&cda,1))) {
		stdoutput.printf("  ind=%d retlen=%d retcode=%d\n",
					(int)ind,(int)retlen,(int)retcode);
		if (!bin) {
			stdoutput.printf("  string: \"%s\"\n",buf);
		}
		stdoutput.printf("  bytes :");
		for (int i=0; i<(int)retlen; i++) {
			stdoutput.printf(" %02x",(unsigned char)buf[i]);
		}
		stdoutput.printf("\n");
	} else {
		ok=false;
	}

	run("oclose - bindstr",&cda,oclose(&cda));
	return ok;
}

int main(int argc, char **argv) {

	const char	*query="select sysdate from dual";
	const char	*alter=NULL;
	bool		dat=false;
	bool		bin=false;
	bool		bindstr=false;
	int		bufsize=maxbufsize;
	int		positional=0;

	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"--query=",8)) {
			query=argv[i]+8;
		} else if (!charstring::compare(argv[i],"--dat")) {
			dat=true;
		} else if (!charstring::compare(argv[i],"--bin")) {
			bin=true;
		} else if (!charstring::compare(argv[i],"--bindstr")) {
			bindstr=true;
		} else if (!charstring::compare(argv[i],"--bufsize=",10)) {
			bufsize=(int)charstring::convertToInteger(argv[i]+10);
			if (bufsize<1 || bufsize>maxbufsize) {
				bufsize=maxbufsize;
			}
		} else if (!charstring::compare(argv[i],"--alter=",8)) {
			alter=argv[i]+8;
		} else if (positional==0) {
			sid=argv[i];
			positional++;
		} else if (positional==1) {
			user=argv[i];
			positional++;
		} else if (positional==2) {
			password=argv[i];
			positional++;
		}
	}

	if (!sid) {
		stdoutput.printf("usage: %s SID [--query=QUERY] [--dat] [--bin] "
				"[--bindstr] [--bufsize=N] "
				"[--alter=NLSDATEFORMAT] [USER PASSWORD]\n",
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

	if (alter) {
		char	stmt[256];
		charstring::printf(stmt,sizeof(stmt),
			"alter session set NLS_DATE_FORMAT='%s'",alter);
		if (!execImmediate("oparse - alter",stmt)) {
			ologof(&lda);
			return 1;
		}
	}

	// --bindstr runs its own cursor and skips the --query fetch path below
	// entirely - see bindStrVariant()'s comment for why the two never share
	// a cursor or a define call
	if (bindstr) {
		int	result=(bindStrVariant(dat,bin,bufsize))?0:1;
		run("ologof",&lda,ologof(&lda));
		stdoutput.printf("done\n");
		return result;
	}

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse("oparse",&cda,query)) {
		ologof(&lda);
		return 1;
	}

	// SQLT_DAT gets the 7 bytes an oracle internal date is; SQLT_BIN gets
	// the raw wire bytes with no client-side charset reinterpretation;
	// SQLT_STR (the default) gets whatever --bufsize asked for, and the
	// client itself takes one off that for the null terminator when it
	// describes the define.  --dat wins over --bin if both are given
	char	buf[maxbufsize];
	sb2	ind=0;
	ub2	retlen=0;
	ub2	retcode=0;
	bytestring::zero(buf,sizeof(buf));
	if (!run("odefin",&cda,
			odefin(&cda,1,(ub1 *)buf,
				(sword)((dat)?7:bufsize),
				(dat)?SQLT_DAT:((bin)?SQLT_BIN:SQLT_STR),-1,
				&ind,(text *)0,-1,-1,
				&retlen,&retcode))) {
		oclose(&cda);
		ologof(&lda);
		return 1;
	}

	int	result=0;
	if (run("oexec",&cda,oexec(&cda)) && run("ofen",&cda,ofen(&cda,1))) {

		// both forms, so the answer is in this program's own output
		// and not only in the capture
		stdoutput.printf("  ind=%d retlen=%d retcode=%d\n",
					(int)ind,(int)retlen,(int)retcode);
		// SQLT_BIN isn't null terminated the way SQLT_STR/SQLT_DAT
		// are, so printing it as a string would run past retlen
		if (!bin) {
			stdoutput.printf("  string: \"%s\"\n",buf);
		}
		stdoutput.printf("  bytes :");
		for (int i=0; i<(int)retlen; i++) {
			stdoutput.printf(" %02x",(unsigned char)buf[i]);
		}
		stdoutput.printf("\n");
	} else {
		result=1;
	}

	run("oclose",&cda,oclose(&cda));
	run("ologof",&lda,ologof(&lda));

	stdoutput.printf("done\n");
	return result;
}
