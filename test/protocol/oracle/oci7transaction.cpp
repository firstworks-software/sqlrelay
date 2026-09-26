// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI7 client, for capturing commit, rollback and autocommit
// traffic against a real Oracle backend: olog, then one --sequence= variant
// built from oopen/oparse/odefin/oexec/ofen/oclose and ocom/orol/ocon/ocof,
// then ologof.  Nothing else.
//
//   ./oci7transaction SID [--sequence=NAME] [--table=TABLE] [USER PASSWORD]
//
//   --sequence=  which variant to run, default select-commit:
//     select-commit       oparse/odefin/oexec/ofen a one-row select, then
//                         ocom - the same shape as the commit-fetch capture
//                         in samples/9656-redhat9x86-oci7-native-
//                         commitfetch3-realserver.oraproxy
//     dml-commit          insert a row into TABLE, then ocom
//     dml-rollback        insert a row into TABLE, then orol
//     empty-rollback      orol with nothing pending
//     autocommit-toggle   ocon, then ocof, with nothing in between
//     autocommit-on-dml   ocon, then insert a row into TABLE, no ocom
//     autocommit-off-dml  ocof, then insert a row into TABLE, then ocom
//   --table=     the scratch table the DML variants insert into, default
//                protocoltest10293txn.  it must already exist, as:
//                  create table protocoltest10293txn
//                        (id number, txt varchar2(40))
//                each insert adds one row whose txt is the variant's name
//
// Written for #10293.  The only commit reply on file came from a session that
// committed selects, and no capture on file shows a real server's reply to a
// rollback or an autocommit change, or whether the call status a commit
// leaves behind carries over into the replies that follow it.  So every
// variant runs its transaction call inside an oopen/oclose pair of its own,
// then opens and closes one more cursor afterward, and prints the return
// value, cda.rc, v2_rc and fc after every single call, so this program's
// output lines up with the capture call for call.
//
// Modeled on oci7describe.cpp and oci7datestr.cpp - same includes, same login
// sequence, same style.  Like them, it compiles by hand against an Instant
// Client, where configure's FW_CHECK_OCI7 probe does not detect one - see
// oci7datestr.cpp's header comment for the command line.

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
const char	*table="protocoltest10293txn";

Lda_Def		lda;
ub4		hda[256];

const char	*variants[]={
	"select-commit",
	"dml-commit",
	"dml-rollback",
	"empty-rollback",
	"autocommit-toggle",
	"autocommit-on-dml",
	"autocommit-off-dml",
	NULL
};

// run one OCI7 call and print its return value and the status fields it left
// in the cursor or lda - ocom/orol/ocon/ocof/olog/ologof report through the
// lda, everything else through the cursor
static bool run(const char *what, Cda_Def *cursor, sword result) {
	stdoutput.printf("%s: return=%d rc=%d v2_rc=%d fc=%d %s\n",
			what,(int)result,(int)cursor->rc,(int)cursor->v2_rc,
			(int)cursor->fc,(result)?"failed":"ok");
	return !result;
}

static bool openCursor(const char *what, Cda_Def *cursor) {
	bytestring::zero(cursor,sizeof(Cda_Def));
	return run(what,cursor,oopen(cursor,&lda,(text *)0,-1,-1,(text *)0,-1));
}

static bool closeCursor(const char *what, Cda_Def *cursor) {
	return run(what,cursor,oclose(cursor));
}

static bool parse(Cda_Def *cursor, const char *query) {
	stdoutput.printf("query: %s\n",query);
	return run("oparse",cursor,
			oparse(cursor,(text *)query,(sb4)-1,0,(ub4)2));
}

// oparse and oexec one insert into the scratch table
static bool insertRow(Cda_Def *cursor, const char *variant) {
	char	query[256];
	charstring::printf(query,sizeof(query),
			"insert into %s values (1,'%s')",table,variant);
	return parse(cursor,query) && run("oexec",cursor,oexec(cursor));
}

// oparse, odefin, oexec and ofen a one-row, one-column select
static bool selectRow(Cda_Def *cursor) {

	if (!parse(cursor,"select 1 from dual")) {
		return false;
	}

	char	buf[64];
	sb2	ind=0;
	ub2	retlen=0;
	ub2	retcode=0;
	bytestring::zero(buf,sizeof(buf));
	if (!run("odefin",cursor,
			odefin(cursor,1,(ub1 *)buf,(sword)sizeof(buf),
				SQLT_STR,-1,&ind,(text *)0,-1,-1,
				&retlen,&retcode))) {
		return false;
	}

	if (!run("oexec",cursor,oexec(cursor)) ||
			!run("ofen",cursor,ofen(cursor,1))) {
		return false;
	}
	stdoutput.printf("row: \"%s\"\n",buf);
	return true;
}

static bool runSequence(const char *variant) {

	Cda_Def	cda;

	// open the cursor the transaction call runs inside of
	if (!openCursor("oopen - bracket",&cda)) {
		return false;
	}

	// the variant itself
	bool	ok=true;
	if (!charstring::compare(variant,"select-commit")) {
		ok=selectRow(&cda) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"dml-commit")) {
		ok=insertRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"dml-rollback")) {
		ok=insertRow(&cda,variant) &&
			run("orol",&lda,orol(&lda));
	} else if (!charstring::compare(variant,"empty-rollback")) {
		ok=run("orol",&lda,orol(&lda));
	} else if (!charstring::compare(variant,"autocommit-toggle")) {
		ok=run("ocon",&lda,ocon(&lda)) &&
			run("ocof",&lda,ocof(&lda));
	} else if (!charstring::compare(variant,"autocommit-on-dml")) {
		ok=run("ocon",&lda,ocon(&lda)) &&
			insertRow(&cda,variant);
	} else if (!charstring::compare(variant,"autocommit-off-dml")) {
		ok=run("ocof",&lda,ocof(&lda)) &&
			insertRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda));
	}

	// close the bracketing cursor
	if (!closeCursor("oclose - bracket",&cda)) {
		ok=false;
	}

	// open and close one more cursor, so the capture shows whether the
	// call status the transaction call set carries over past it
	if (!openCursor("oopen - after",&cda) ||
			!closeCursor("oclose - after",&cda)) {
		ok=false;
	}

	return ok;
}

int main(int argc, char **argv) {

	const char	*variant="select-commit";
	int		positional=0;

	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"--sequence=",11)) {
			variant=argv[i]+11;
		} else if (!charstring::compare(argv[i],"--table=",8)) {
			table=argv[i]+8;
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

	if (!sid || !charstring::isInSet(variant,variants)) {
		stdoutput.printf("usage: %s SID "
				"[--sequence=select-commit|dml-commit|"
				"dml-rollback|empty-rollback|"
				"autocommit-toggle|autocommit-on-dml|"
				"autocommit-off-dml] "
				"[--table=TABLE] [USER PASSWORD]\n",
				argv[0]);
		return 1;
	}

	// log in
	stdoutput.printf("logging in to %s as %s...\n",sid,user);
	stdoutput.printf("sequence: %s\n",variant);
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

	int	result=(runSequence(variant))?0:1;

	// log out
	if (!run("ologof",&lda,ologof(&lda))) {
		result=1;
	}

	stdoutput.printf("done\n");
	return result;
}
