// Copyright (c) David Muse
// See the file COPYING for more information.

// A minimal OCI7 client, for capturing bind-variable traffic against a real
// Oracle backend: olog, oopen, oparse, obndrv, then - depending on --bind=
// - odefin/oexec/ofen or just oexec, then oclose, ologof. Nothing else.
//
//   ./oci7bind SID [--bind=VARIANT] [USER PASSWORD]
//
// oci7.cpp's Binds section (around line 1200) exercises the same calls, but
// it is the tail of a suite that logs in twice, creates tables and runs a
// hundred other statements first, so a capture of it is far too noisy to
// read a byte layout out of. This program never forks, runs one variant per
// invocation, and puts nothing on the wire but the login and that variant -
// the same reason oci7describe.cpp exists next to it for describes.
//
// It exists for #9700. src/protocols/oracle.cpp's OPTION_BIND path in
// query2() is written against a pre-8i wiki page, not against bytes: it
// answers a bind request with a hardcoded 11-byte response and then reads
// one follow-up packet per bind. No capture on file has ever had OPTION_BIND
// set, and the one real OALL7 capture we do have (samples/oracle102-oci7-
// native-multicol-1col-fetch.cap, packet [0027]) marshals its defines
// INLINE in the request packet rather than as a separate round trip - so the
// implemented model and the observed one cannot both be right. These
// captures are what settles it.
//
// The variants each have to be captured on their own, and each one isolates
// a different unknown:
//   selectint    one IN bind (SQLT_INT) plus one odefin, on a select. the
//                smallest packet that has both a bind block and a define
//                block in it, so it shows where one ends and the other
//                begins - the offset the whole already-verified plain
//                select + odefin path depends on
//   selectstr    the same, with the bind SQLT_STR instead of SQLT_INT, so
//                the datatype and length fields can be told apart from
//                everything around them by diffing the two captures
//   selecttwo    two binds of different types and two defines, which gives
//                the stride between one bind descriptor and the next - a
//                one-bind capture alone cannot
//   insert       three binds and NO defines at all (dml), isolating the
//                bind block with nothing behind it
//   nullbind     the same insert with two of the three indicators set to
//                -1, so a null bind's wire form can be diffed against
//                nullbind's non-null counterpart
//   many         bind once, oexec three times. obndrv binds by reference,
//                so this says whether the client re-marshals the binds on
//                every execute or sends them only with the first
//   reverse      the same insert with the obndrv calls made backwards, to
//                settle whether the wire order of the bind values is the
//                order of the placeholders in the statement or the order
//                the client happened to call obndrv in. every other
//                variant binds in text order, so none of them can tell
//                those two apart, and the module assumes the former
//   out          a pl/sql block with one OUT bind (:cnt), the shape the
//                module has zero bytes for on the response side
//   inout        a pl/sql block whose bind is read and written (:v := :v*2)
//   nullout      a pl/sql block that assigns NULL to its out bind, so the
//                null indicator's return form is captured too
//
// insert, nullbind and many need the protocoltestbind table. They create it
// themselves (dropping any leftover first), so no other program has to have
// run against the backend, at the cost of a few extra statements at the head
// of the capture - they are the first thing after the login and easy to skip
// past in the decode.

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

static bool parse(Cda_Def *cursor, const char *query) {
	stdoutput.printf("oparse: %s\n",query);
	return run("oparse",cursor,
			oparse(cursor,(text *)query,(sb4)-1,0,(ub4)2));
}

// oparse plus oexec on a cursor of its own, for the table setup the dml
// variants need.  failure is reported but not fatal - the drop fails the
// first time through, when there is no leftover table to drop
static void execImmediate(const char *query, bool checked) {
	Cda_Def	cda;
	if (!openCursor("oopen - setup",&cda)) {
		return;
	}
	stdoutput.printf("setup: %s\n",query);
	if (oparse(&cda,(text *)query,(sb4)-1,0,(ub4)2) ||
					oexec(&cda)) {
		if (checked) {
			printError("setup",&cda);
		} else {
			stdoutput.printf("setup ignored: ORA-%05d\n",
						(int)cda.rc);
		}
	}
	oclose(&cda);
}

static void createBindTable() {
	execImmediate("drop table protocoltestbind",false);
	execImmediate("create table protocoltestbind ("
			"testnumber number(10),"
			"testchar char(20),"
			"testvarchar varchar2(40))",true);
}

// bind one program variable by name, exactly the way oci7.cpp's Binds
// section does - sqlvl -1 for "null terminated, measure it"
static bool bind(Cda_Def *cursor, const char *name,
			ub1 *value, sword valuesize, sword type, sb2 *ind) {
	char	what[64];
	charstring::printf(what,sizeof(what),"obndrv - %s",name);
	return run(what,cursor,
			obndrv(cursor,(text *)name,-1,value,valuesize,
					type,-1,ind,(text *)0,-1,-1));
}

// odefin one column as a string, the way every define in oci7.cpp does
static bool define(Cda_Def *cursor, sword pos, char *buf, sword bufsize,
				sb2 *ind, ub2 *retlen, ub2 *retcode) {
	char	what[64];
	charstring::printf(what,sizeof(what),"odefin - column %d",(int)pos);
	bytestring::zero(buf,(size_t)bufsize);
	return run(what,cursor,
			odefin(cursor,pos,(ub1 *)buf,bufsize,SQLT_STR,-1,
				ind,(text *)0,-1,-1,retlen,retcode));
}

// the three-bind insert the dml variants share
static const char	*bindinsert=
				"insert into protocoltestbind "
				"(testnumber,testchar,testvarchar) "
				"values (:num,:chr,:vchr)";

// a select with binds in it, run through odefin/oexec/ofen.  binds and
// defines end up in the same request packet here, which is the case the
// module's byte offsets actually have to survive
static int selectVariant(const char *query, bool twobinds, bool stringbind) {

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse(&cda,query)) {
		return 1;
	}

	sb4	numvalue=10;
	char	strvalue[32];
	sb2	bindind[2];
	charstring::copy(strvalue,"bindstring");
	bytestring::zero(bindind,sizeof(bindind));

	if (stringbind) {
		if (!bind(&cda,":s",(ub1 *)strvalue,
				(sword)sizeof(strvalue),SQLT_STR,&bindind[0])) {
			oclose(&cda);
			return 1;
		}
	} else {
		if (!bind(&cda,":num",(ub1 *)&numvalue,
				(sword)sizeof(numvalue),SQLT_INT,&bindind[0])) {
			oclose(&cda);
			return 1;
		}
	}
	if (twobinds && !bind(&cda,":b",(ub1 *)strvalue,
				(sword)sizeof(strvalue),SQLT_STR,&bindind[1])) {
		oclose(&cda);
		return 1;
	}

	char	buf[2][64];
	sb2	ind[2];
	ub2	retlen[2];
	ub2	retcode[2];
	bytestring::zero(ind,sizeof(ind));
	bytestring::zero(retlen,sizeof(retlen));
	bytestring::zero(retcode,sizeof(retcode));

	sword	cols=(twobinds)?2:1;
	for (sword pos=1; pos<=cols; pos++) {
		if (!define(&cda,pos,buf[pos-1],(sword)sizeof(buf[pos-1]),
				&ind[pos-1],&retlen[pos-1],&retcode[pos-1])) {
			oclose(&cda);
			return 1;
		}
	}

	if (!run("oexec",&cda,oexec(&cda)) ||
			!run("ofen",&cda,ofen(&cda,1))) {
		oclose(&cda);
		return 1;
	}
	if (twobinds) {
		stdoutput.printf("  row: %s / %s\n",buf[0],buf[1]);
	} else {
		stdoutput.printf("  row: %s\n",buf[0]);
	}

	run("oclose",&cda,oclose(&cda));
	return 0;
}

// the same three-bind insert, but with the obndrv calls made in the REVERSE
// of the order the placeholders appear in the statement.
//
// obndrv binds by name, and the names never reach the wire - the server sees
// an array of values with nothing but their position to identify them. So
// the module has to assume the nth value on the wire is the nth placeholder
// in the query text. That is true of the modern OALL8 path, where the client
// binds positionally to begin with, but for OCI7 it is an assumption about
// what order the client marshals its own bind array in, and every other
// variant here calls obndrv in text order, so none of them can tell the two
// orders apart.
//
// This one can. It binds :vchr, then :chr, then :num, against
// "values (:num,:chr,:vchr)". If the client marshals in text order the row
// comes out right; if it marshals in call order and the module reads it
// positionally, testnumber ends up holding what belonged to :vchr and the
// insert fails or lands garbage. Either way the answer is unambiguous.
static int reverseVariant() {

	createBindTable();

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse(&cda,bindinsert)) {
		return 1;
	}

	sb4	bindnumber=50;
	char	bindchar[32];
	char	bindvarchar[64];
	sb2	bindind[3];
	charstring::copy(bindchar,"revchar");
	charstring::copy(bindvarchar,"revvarchar");
	bytestring::zero(bindind,sizeof(bindind));

	// deliberately backwards
	if (!bind(&cda,":vchr",(ub1 *)bindvarchar,
				(sword)sizeof(bindvarchar),
				SQLT_STR,&bindind[2]) ||
		!bind(&cda,":chr",(ub1 *)bindchar,
				(sword)sizeof(bindchar),
				SQLT_STR,&bindind[1]) ||
		!bind(&cda,":num",(ub1 *)&bindnumber,
				(sword)sizeof(bindnumber),
				SQLT_INT,&bindind[0])) {
		oclose(&cda);
		return 1;
	}

	if (!run("oexec",&cda,oexec(&cda))) {
		oclose(&cda);
		return 1;
	}
	stdoutput.printf("  rows processed: %d\n",(int)cda.rpc);
	stdoutput.printf("  expect testnumber=50, testchar=revchar, "
				"testvarchar=revvarchar\n");

	run("oclose",&cda,oclose(&cda));
	return 0;
}

// the three-bind insert, with or without null indicators, executed once or
// three times
static int insertVariant(bool nulls, int iterations) {

	createBindTable();

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse(&cda,bindinsert)) {
		return 1;
	}

	// each variant uses a testnumber of its own, so a row can be told
	// apart from the others afterward.  they used to share 10, which made
	// the null case's row indistinguishable from the plain insert's when
	// the table had not been dropped between runs
	sb4	bindnumber=(nulls)?20:10;
	char	bindchar[32];
	char	bindvarchar[64];
	sb2	bindind[3];
	charstring::copy(bindchar,"bindchar");
	charstring::copy(bindvarchar,"bindvarchar");
	bytestring::zero(bindind,sizeof(bindind));

	if (!bind(&cda,":num",(ub1 *)&bindnumber,
				(sword)sizeof(bindnumber),
				SQLT_INT,&bindind[0]) ||
		!bind(&cda,":chr",(ub1 *)bindchar,
				(sword)sizeof(bindchar),
				SQLT_STR,&bindind[1]) ||
		!bind(&cda,":vchr",(ub1 *)bindvarchar,
				(sword)sizeof(bindvarchar),
				SQLT_STR,&bindind[2])) {
		oclose(&cda);
		return 1;
	}

	// the buffers keep their values here, so a null has to come from the
	// indicator rather than from an empty buffer - the same way oci7.cpp's
	// "obndrv - null binds" case sets it up
	if (nulls) {
		bindind[1]=-1;
		bindind[2]=-1;
	}

	// obndrv binds by reference, so only the buffer changes between
	// executes - nothing is re-bound
	sb4	firstnumber=bindnumber;
	for (int i=0; i<iterations; i++) {
		bindnumber=firstnumber+i;
		if (!run("oexec",&cda,oexec(&cda))) {
			oclose(&cda);
			return 1;
		}
		stdoutput.printf("  rows processed: %d\n",(int)cda.rpc);
	}

	run("oclose",&cda,oclose(&cda));
	return 0;
}

// a pl/sql block with one bind the block writes to
static int plsqlVariant(const char *block, sb4 invalue) {

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse(&cda,block)) {
		return 1;
	}
	stdoutput.printf("  statement type: %d\n",(int)cda.ft);

	sb4	value=invalue;
	sb2	ind=0;
	if (!bind(&cda,":v",(ub1 *)&value,(sword)sizeof(value),
						SQLT_INT,&ind)) {
		oclose(&cda);
		return 1;
	}

	if (!run("oexec",&cda,oexec(&cda))) {
		oclose(&cda);
		return 1;
	}
	stdoutput.printf("  value=%d ind=%d\n",(int)value,(int)ind);

	run("oclose",&cda,oclose(&cda));
	return 0;
}

int main(int argc, char **argv) {

	const char	*variant="selectint";
	int		positional=0;

	for (int i=1; i<argc; i++) {
		if (!charstring::compare(argv[i],"--bind=",7)) {
			variant=argv[i]+7;
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
		stdoutput.printf("usage: %s SID "
				"[--bind=selectint|selectstr|selecttwo|"
				"insert|nullbind|many|reverse|out|inout|nullout] "
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

	int	result=0;
	if (!charstring::compare(variant,"selectint")) {
		result=selectVariant("select :num from dual",false,false);
	} else if (!charstring::compare(variant,"selectstr")) {
		result=selectVariant("select :s from dual",false,true);
	} else if (!charstring::compare(variant,"selecttwo")) {
		result=selectVariant("select :num, :b from dual",true,false);
	} else if (!charstring::compare(variant,"insert")) {
		result=insertVariant(false,1);
	} else if (!charstring::compare(variant,"nullbind")) {
		result=insertVariant(true,1);
	} else if (!charstring::compare(variant,"many")) {
		result=insertVariant(false,3);
	} else if (!charstring::compare(variant,"reverse")) {
		result=reverseVariant();
	} else if (!charstring::compare(variant,"out")) {
		// an out-only bind: the block never reads :v
		result=plsqlVariant("begin :v := 7; end;",0);
	} else if (!charstring::compare(variant,"inout")) {
		result=plsqlVariant("begin :v := :v * 2; end;",21);
	} else if (!charstring::compare(variant,"nullout")) {
		result=plsqlVariant("begin :v := NULL; end;",99);
	} else {
		stdoutput.printf("unknown --bind= variant: %s\n",variant);
		ologof(&lda);
		return 1;
	}

	run("ologof",&lda,ologof(&lda));

	stdoutput.printf("done\n");
	return result;
}
