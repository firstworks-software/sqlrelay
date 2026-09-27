// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI7 client, for capturing commit, rollback and autocommit
// traffic against a real Oracle backend: olog, then one --sequence= variant
// built from oopen/oparse/odefin/oexec/ofen/oclose and ocom/orol/ocon/ocof,
// then ologof.  Nothing else.
//
//   ./oci7transaction SID [--sequence=NAME] [--table=TABLE]
//                     [--ddltable=TABLE] [--lngflg=N] [USER PASSWORD]
//
//   --sequence=  which variant to run, default select-commit:
//     select-commit          oparse/odefin/oexec/ofen a one-row select, then
//                             ocom - the same shape as the commit-fetch
//                             capture in samples/9656-redhat9x86-oci7-native-
//                             commitfetch3-realserver.oraproxy
//     dml-commit              insert a row into TABLE, then ocom
//     dml-rollback            insert a row into TABLE, then orol
//     empty-rollback          orol with nothing pending
//     autocommit-toggle       ocon, then ocof, with nothing in between
//     autocommit-on-dml       ocon, then insert a row into TABLE, no ocom
//     autocommit-off-dml      ocof, then insert a row into TABLE, then ocom
//     dml-dml-commit          insert two rows into TABLE, then ocom
//     dml-select-commit       insert a row, select 1 from dual, then ocom
//     dml-openclose-commit    insert a row, open and close a second cursor
//                             while the insert is still pending, then ocom
//     rollback-dml-commit     orol with nothing pending (leaves call status
//                             5), then insert a row into TABLE, then ocom
//     commit-select           insert a row, ocom (leaves call status 5),
//                             then select 1 from dual
//     rollback-autocommit-toggle  orol with nothing pending, then ocon,
//                             then ocof
//     dml-autocommit-on       insert a row into TABLE, then ocon, no ocom
//     autocommit-on-dml-dml   ocon, then insert two rows into TABLE
//     ddl-alone               drop then create the scratch table DDLTABLE
//     dml-ddl                 insert a row into TABLE, then drop and create
//                             DDLTABLE
//     plsql-null              oparse/oexec "begin null; end;"
//     plsql-dml-commit        oparse/oexec a PL/SQL block that inserts a
//                             row into TABLE, then ocom
//     dml-sqlcommit           insert a row into TABLE, then oparse/oexec the
//                             literal text "commit" instead of calling ocom
//     dml-error-rollback      insert a good row into TABLE, then oparse/oexec
//                             an insert that fails on a type mismatch, then
//                             orol
//     select-for-update-commit  insert a row, select it back "for update",
//                             then ocom
//     merge-commit            from idle, oparse/oexec a MERGE that inserts
//                             one row, then ocom
//     rollback-merge-commit   orol with nothing pending, then the merge,
//                             then ocom
//     autocommit-on-merge     ocon, then the merge, no ocom
//     dml-ddl-misc            one insert before each of a batch of otherwise
//                             unclassified DDL verbs - grant, revoke,
//                             comment, rename (both ways), analyze, audit,
//                             noaudit, drop, flashback, purge recyclebin,
//                             and a deliberately failing grant - then ocom
//     select-for-update-idle  from idle, select the scratch table's first
//                             row back "for update", then ocom
//     commit-describe         insert a row, ocom, then oparse a one-column
//                             select and odescr column 1
//     commit-exfet            insert a row, ocom, then oparse a one-column
//                             select, odefin and oexfet instead of
//                             oexec/ofen
//     plsql-commit-inside     from idle, oparse/oexec a PL/SQL block that
//                             inserts a row into TABLE and commits inside
//                             the block
//     dml-plsql-null          insert a row, "begin null; end;", then ocom
//     commit-select-for-update  insert a row, ocom, then select the scratch
//                             table's first row back "for update", then ocom
//     autocommit-on-select-for-update  ocon, then select the scratch
//                             table's first row back "for update", no ocom
//   --table=     the scratch table the DML variants insert into, default
//                protocoltest10293txn.  it must already exist, as:
//                  create table protocoltest10293txn
//                        (id number, txt varchar2(40))
//                each insert adds one row whose txt names the variant
//   --ddltable=  the scratch table the DDL variants drop and (re)create,
//                default protocoltest10295ddl.  it need not exist first -
//                the drop's failure is ignored
//   --lngflg=    oparse()'s language flag, default 2 (V7 behavior)
//
// Written for #10293, extended for #10295 and #10315.  Every variant runs its
// transaction calls inside an oopen/oclose pair of its own, then opens and
// closes one more cursor afterward, and prints the return value, cda.rc,
// v2_rc and fc after every single call, so this program's output lines up
// with the capture call for call.
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
const char	*ddltable="protocoltest10295ddl";
ub4		lngflg=2;

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
	"dml-dml-commit",
	"dml-select-commit",
	"dml-openclose-commit",
	"rollback-dml-commit",
	"commit-select",
	"rollback-autocommit-toggle",
	"dml-autocommit-on",
	"autocommit-on-dml-dml",
	"ddl-alone",
	"dml-ddl",
	"plsql-null",
	"plsql-dml-commit",
	"dml-sqlcommit",
	"dml-error-rollback",
	"select-for-update-commit",
	"merge-commit",
	"rollback-merge-commit",
	"autocommit-on-merge",
	"dml-ddl-misc",
	"select-for-update-idle",
	"commit-describe",
	"commit-exfet",
	"plsql-commit-inside",
	"dml-plsql-null",
	"commit-select-for-update",
	"autocommit-on-select-for-update",
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
			oparse(cursor,(text *)query,(sb4)-1,0,lngflg));
}

// oparse and oexec an arbitrary statement
static bool execSql(Cda_Def *cursor, const char *query) {
	return parse(cursor,query) && run("oexec",cursor,oexec(cursor));
}

// oparse and oexec one insert into the scratch table, with LABEL as the
// row's txt value
static bool insertRow(Cda_Def *cursor, const char *label) {
	char	query[256];
	charstring::printf(query,sizeof(query),
			"insert into %s values (1,'%s')",table,label);
	return execSql(cursor,query);
}

// oparse and oexec a MERGE that always inserts one row, with LABEL as the
// row's txt value.  a 10.2 server drops the connection (ORA-03113) on a
// MERGE whose ON clause is a constant like (1=0), so this one matches on a
// source id one past the table's highest instead
static bool mergeRow(Cda_Def *cursor, const char *label) {
	char	query[320];
	charstring::printf(query,sizeof(query),
			"merge into %s t using "
			"(select nvl(max(id),0)+1 id from %s) s "
			"on (t.id=s.id) when not matched then insert (id,txt) "
			"values (s.id,'%s')",table,table,label);
	return execSql(cursor,query);
}

// oparse, odefin, oexec and ofen a one-row, one-column QUERY
static bool selectQuery(Cda_Def *cursor, const char *query) {

	if (!parse(cursor,query)) {
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

static bool selectRow(Cda_Def *cursor) {
	return selectQuery(cursor,"select 1 from dual");
}

// oparse, odefin and oexfet a one-row, one-column QUERY - oexfet is the
// other legacy fetch shape, with no separate oexec/ofen
static bool exfetQuery(Cda_Def *cursor, const char *query) {

	if (!parse(cursor,query)) {
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

	if (!run("oexfet",cursor,oexfet(cursor,(ub4)1,0,0))) {
		return false;
	}
	stdoutput.printf("row: \"%s\"\n",buf);
	return true;
}

// odescr column POS of the cursor's current parse and print what came back
static bool describeColumn(Cda_Def *cursor, sword pos) {

	sb4	dbsize=0;
	sb2	dbtype=0;
	sb1	cbuf[128];
	sb4	cbufl=(sb4)sizeof(cbuf);
	sb4	dsize=0;
	sb2	precision=0;
	sb2	scale=0;
	sb2	nullok=0;
	bytestring::zero(cbuf,sizeof(cbuf));

	if (!run("odescr",cursor,
			odescr(cursor,pos,&dbsize,&dbtype,cbuf,&cbufl,
				&dsize,&precision,&scale,&nullok))) {
		return false;
	}

	// odescr does not null terminate the name - cbufl comes back as its
	// length
	if (cbufl<0 || cbufl>=(sb4)sizeof(cbuf)) {
		cbufl=(sb4)sizeof(cbuf)-1;
	}
	cbuf[cbufl]='\0';
	stdoutput.printf("  name=%s dbtype=%d dbsize=%d dsize=%d "
				"precision=%d scale=%d nullok=%d\n",
				(char *)cbuf,(int)dbtype,(int)dbsize,
				(int)dsize,(int)precision,(int)scale,
				(int)nullok);
	return true;
}

// select the scratch table's first row back, with a row lock held
static bool selectForUpdate(Cda_Def *cursor) {
	char	query[256];
	charstring::printf(query,sizeof(query),
			"select txt from %s where rownum=1 for update",table);
	return selectQuery(cursor,query);
}

// drop then create the DDL scratch table - the drop's failure is expected
// and ignored on every run but the first
static bool ddlStatement(Cda_Def *cursor) {
	char	dropquery[128];
	charstring::printf(dropquery,sizeof(dropquery),
			"drop table %s",ddltable);
	if (parse(cursor,dropquery)) {
		run("oexec (drop, ignored)",cursor,oexec(cursor));
	}

	char	createquery[160];
	charstring::printf(createquery,sizeof(createquery),
			"create table %s (id number, txt varchar2(40))",
			ddltable);
	return execSql(cursor,createquery);
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
	} else if (!charstring::compare(variant,"dml-dml-commit")) {
		ok=insertRow(&cda,"dml-dml-commit-1") &&
			insertRow(&cda,"dml-dml-commit-2") &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"dml-select-commit")) {
		ok=insertRow(&cda,variant) &&
			selectRow(&cda) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"dml-openclose-commit")) {
		Cda_Def	extra;
		ok=insertRow(&cda,variant) &&
			openCursor("oopen - nested",&extra) &&
			closeCursor("oclose - nested",&extra) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"rollback-dml-commit")) {
		ok=run("orol",&lda,orol(&lda)) &&
			insertRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"commit-select")) {
		ok=insertRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda)) &&
			selectRow(&cda);
	} else if (!charstring::compare(variant,
			"rollback-autocommit-toggle")) {
		ok=run("orol",&lda,orol(&lda)) &&
			run("ocon",&lda,ocon(&lda)) &&
			run("ocof",&lda,ocof(&lda));
	} else if (!charstring::compare(variant,"dml-autocommit-on")) {
		ok=insertRow(&cda,variant) &&
			run("ocon",&lda,ocon(&lda));
	} else if (!charstring::compare(variant,"autocommit-on-dml-dml")) {
		ok=run("ocon",&lda,ocon(&lda)) &&
			insertRow(&cda,"autocommit-on-dml-dml-1") &&
			insertRow(&cda,"autocommit-on-dml-dml-2");
	} else if (!charstring::compare(variant,"ddl-alone")) {
		ok=ddlStatement(&cda);
	} else if (!charstring::compare(variant,"dml-ddl")) {
		ok=insertRow(&cda,variant) &&
			ddlStatement(&cda);
	} else if (!charstring::compare(variant,"plsql-null")) {
		ok=execSql(&cda,"begin null; end;");
	} else if (!charstring::compare(variant,"plsql-dml-commit")) {
		char	query[256];
		charstring::printf(query,sizeof(query),
				"begin insert into %s values "
				"(1,'plsql-dml-commit'); end;",table);
		ok=execSql(&cda,query) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"dml-sqlcommit")) {
		ok=insertRow(&cda,variant) &&
			execSql(&cda,"commit");
	} else if (!charstring::compare(variant,"dml-error-rollback")) {
		ok=insertRow(&cda,variant);
		if (ok) {
			char	badquery[256];
			charstring::printf(badquery,sizeof(badquery),
					"insert into %s values "
					"('not-a-number','%s')",
					table,variant);
			// deliberately fails - id is numeric; the point is
			// the status the failed execute leaves behind, not
			// whether it succeeds
			parse(&cda,badquery);
			run("oexec (expected failure)",&cda,oexec(&cda));
			ok=run("orol",&lda,orol(&lda));
		}
	} else if (!charstring::compare(variant,
			"select-for-update-commit")) {
		ok=insertRow(&cda,variant) &&
			selectForUpdate(&cda) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"merge-commit")) {
		ok=mergeRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"rollback-merge-commit")) {
		ok=run("orol",&lda,orol(&lda)) &&
			mergeRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"autocommit-on-merge")) {
		ok=run("ocon",&lda,ocon(&lda)) &&
			mergeRow(&cda,variant);
	} else if (!charstring::compare(variant,"dml-ddl-misc")) {

		// walk the unclassified DDL verbs, each preceded by an
		// insert so every DDL is seen ending a pending txn - a DDL
		// failure here doesn't stop the run, since the point is the
		// call status each one leaves behind, not whether it succeeds
		ddlStatement(&cda);

		char	q[160];
		insertRow(&cda,"grant");
		charstring::printf(q,sizeof(q),
				"grant select on %s to public",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"revoke");
		charstring::printf(q,sizeof(q),
				"revoke select on %s from public",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"comment");
		charstring::printf(q,sizeof(q),
				"comment on table %s is 'x'",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"rename-out");
		charstring::printf(q,sizeof(q),
				"rename %s to protocoltest10315ren",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"rename-back");
		charstring::printf(q,sizeof(q),
				"rename protocoltest10315ren to %s",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"analyze");
		charstring::printf(q,sizeof(q),
				"analyze table %s compute statistics",
				ddltable);
		execSql(&cda,q);

		insertRow(&cda,"audit");
		charstring::printf(q,sizeof(q),
				"audit select on %s",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"noaudit");
		charstring::printf(q,sizeof(q),
				"noaudit select on %s",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"drop");
		charstring::printf(q,sizeof(q),"drop table %s",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"flashback");
		charstring::printf(q,sizeof(q),
				"flashback table %s to before drop",ddltable);
		execSql(&cda,q);

		insertRow(&cda,"purge");
		execSql(&cda,"purge recyclebin");

		insertRow(&cda,"grant-fail");
		charstring::printf(q,sizeof(q),
				"grant select on %s to nosuchuser10315",
				ddltable);
		execSql(&cda,q);

		ok=run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"select-for-update-idle")) {
		ok=selectForUpdate(&cda) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,"commit-describe")) {
		ok=insertRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda)) &&
			parse(&cda,"select 1 from dual") &&
			describeColumn(&cda,1);
	} else if (!charstring::compare(variant,"commit-exfet")) {
		ok=insertRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda)) &&
			exfetQuery(&cda,"select 1 from dual");
	} else if (!charstring::compare(variant,"plsql-commit-inside")) {
		char	query[256];
		charstring::printf(query,sizeof(query),
				"begin insert into %s values "
				"(1,'plsql-commit-inside'); commit; end;",
				table);
		ok=execSql(&cda,query);
	} else if (!charstring::compare(variant,"dml-plsql-null")) {
		ok=insertRow(&cda,variant) &&
			execSql(&cda,"begin null; end;") &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,
			"commit-select-for-update")) {
		ok=insertRow(&cda,variant) &&
			run("ocom",&lda,ocom(&lda)) &&
			selectForUpdate(&cda) &&
			run("ocom",&lda,ocom(&lda));
	} else if (!charstring::compare(variant,
			"autocommit-on-select-for-update")) {
		ok=run("ocon",&lda,ocon(&lda)) &&
			selectForUpdate(&cda);
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
		} else if (!charstring::compare(argv[i],"--ddltable=",11)) {
			ddltable=argv[i]+11;
		} else if (!charstring::compare(argv[i],"--lngflg=",9)) {
			lngflg=(ub4)charstring::convertToInteger(argv[i]+9);
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
				"autocommit-off-dml|dml-dml-commit|"
				"dml-select-commit|dml-openclose-commit|"
				"rollback-dml-commit|commit-select|"
				"rollback-autocommit-toggle|"
				"dml-autocommit-on|autocommit-on-dml-dml|"
				"ddl-alone|dml-ddl|plsql-null|"
				"plsql-dml-commit|dml-sqlcommit|"
				"dml-error-rollback|"
				"select-for-update-commit|merge-commit|"
				"rollback-merge-commit|"
				"autocommit-on-merge|dml-ddl-misc|"
				"select-for-update-idle|commit-describe|"
				"commit-exfet|plsql-commit-inside|"
				"dml-plsql-null|commit-select-for-update|"
				"autocommit-on-select-for-update] "
				"[--table=TABLE] [--ddltable=TABLE] "
				"[--lngflg=N] "
				"[USER PASSWORD]\n",
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
