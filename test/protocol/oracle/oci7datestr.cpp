// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI7 client that fetches ONE column and prints exactly what came
// back: olog, oopen, oparse, odefin, oexec, ofen, oclose, ologof.  Nothing
// else.
//
//   ./oci7datestr SID [--query=QUERY] [--dat] [--bufsize=N]
//				[--alter=NLSDATEFORMAT] [USER PASSWORD]
//
//   --query=    what to select.  one column.  default "select sysdate from
//               dual"
//   --dat       odefin the column SQLT_DAT into a 7 byte buffer instead of
//               SQLT_STR into a character buffer, so the two forms of the
//               same fetch can be diffed against each other
//   --bufsize=  the SQLT_STR buffer's size, default 64.  the size the client
//               puts in the define descriptor is one less than this
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

int main(int argc, char **argv) {

	const char	*query="select sysdate from dual";
	const char	*alter=NULL;
	bool		dat=false;
	int		bufsize=maxbufsize;
	int		positional=0;

	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"--query=",8)) {
			query=argv[i]+8;
		} else if (!charstring::compare(argv[i],"--dat")) {
			dat=true;
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
		stdoutput.printf("usage: %s SID [--query=QUERY] [--dat] "
				"[--bufsize=N] [--alter=NLSDATEFORMAT] "
				"[USER PASSWORD]\n",argv[0]);
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

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse("oparse",&cda,query)) {
		ologof(&lda);
		return 1;
	}

	// SQLT_DAT gets the 7 bytes an oracle internal date is; SQLT_STR gets
	// whatever --bufsize asked for, and the client itself takes one off
	// that for the null terminator when it describes the define
	char	buf[maxbufsize];
	sb2	ind=0;
	ub2	retlen=0;
	ub2	retcode=0;
	bytestring::zero(buf,sizeof(buf));
	if (!run("odefin",&cda,
			odefin(&cda,1,(ub1 *)buf,
				(sword)((dat)?7:bufsize),
				(dat)?SQLT_DAT:SQLT_STR,-1,
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
		stdoutput.printf("  string: \"%s\"\n",buf);
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
