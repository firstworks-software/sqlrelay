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
//   selectint     one IN bind (SQLT_INT) plus one odefin, on a select. the
//                 smallest packet that has both a bind block and a define
//                 block in it, so it shows where one ends and the other
//                 begins - the offset the whole already-verified plain
//                 select + odefin path depends on
//   selectstr     the same, with the bind SQLT_STR instead of SQLT_INT, so
//                 the datatype and length fields can be told apart from
//                 everything around them by diffing the two captures
//   selectstrlong the same, with a bind value 300 characters long - past the
//                 point (252 bytes) where the wire format's one-byte length
//                 can no longer hold it and has to switch to
//                 CLR_LONG_FORM_MARKER; see #9985
//   selecttwo     two binds of different types and two defines, which gives
//                 the stride between one bind descriptor and the next - a
//                 one-bind capture alone cannot
//   datebind      one IN bind of a DATE (SQLT_DAT), against a select, so the
//                 bind descriptor's wire type/buffer size/byte layout for a
//                 date can be read directly, and cross-checked against the
//                 already-proven fetch-side date bytes in oci7.cpp - #9986
//   insert        three binds and NO defines at all (dml), isolating the
//                 bind block with nothing behind it
//   nullbind      the same insert with two of the three indicators set to
//                 -1, so a null bind's wire form can be diffed against
//                 nullbind's non-null counterpart
//   many          bind once, oexec three times, inserting testnumber 10, 11
//                 and 12. obndrv binds by reference, so this says whether the
//                 client re-marshals the binds on every execute or sends them
//                 only with the first. it re-reads the table afterward and
//                 prints every testnumber in it, so which of the three
//                 executes actually landed a row is in this program's own
//                 output rather than something another program has to go
//                 look up
//   reverse       the same insert with the obndrv calls made backwards, to
//                 settle whether the wire order of the bind values is the
//                 order of the placeholders in the statement or the order
//                 the client happened to call obndrv in. every other
//                 variant binds in text order, so none of them can tell
//                 those two apart, and the module assumes the former
//   out           a pl/sql block with one OUT bind (:cnt), the shape the
//                 module has zero bytes for on the response side
//   inout         a pl/sql block whose bind is read and written (:v := :v*2)
//   nullout       a pl/sql block that assigns NULL to its out bind, so the
//                 null indicator's return form is captured too
//   biginout      an in-out bind of a short value (SQLT_STR, "hi") declared
//                 with an obndrv buffer size of 512 bytes, against a block
//                 that writes back 300 bytes - isolates whether the module
//                 sizes the out-bind buffer from the client's declared
//                 buffer size rather than a fixed floor. #10012
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

// ORA-01403, no data found - what a fetch past the last row leaves in cda.rc
const int	nodatafound=1403;

// the most rows verifyBindTable will fetch before it gives up
const int	maxverifyrows=1000;

// the buffer size selectLongStrVariant sizes its arrays to, regardless of
// which length it is actually asked to bind
const sword	maxlongbindlen=512;

// print the ORA number a call left in the cursor, and the rest of the
// cursor's error fields
static void printError(const char *what, Cda_Def *cursor) {

	// fc is the OCI function code, so it names the call the error came out
	// of even when the caller's label is vague, and peo is the offset a
	// parse error is at.  v2_rc and ose catch the case where a call fails
	// with nothing in rc at all - v2_rc is an sb2, so a number past 32767
	// wraps there.
	//
	// oerhms would turn the number into message text, but it is outside
	// the 12 symbols acsite.m4's FW_CHECK_OCI7 link-tests, so it is left
	// out here the same way oci7.cpp leaves it out, and the number is
	// printed on its own.
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

// open a cursor against the session in lda
static bool openCursor(const char *what, Cda_Def *cursor) {
	bytestring::zero(cursor,sizeof(Cda_Def));
	return run(what,cursor,oopen(cursor,&lda,(text *)0,-1,-1,(text *)0,-1));
}

// oparse one query, labeled what so the statement it belongs to is never in
// doubt
static bool parse(const char *what, Cda_Def *cursor, const char *query) {
	stdoutput.printf("%s: %s\n",what,query);
	return run(what,cursor,
			oparse(cursor,(text *)query,(sb4)-1,0,(ub4)2));
}

// oparse plus oexec on a cursor of its own, for the table setup the dml
// variants need.  what labels every line the statement puts out, so the
// drop's are never mistaken for the create's.  failure is reported but not
// fatal - the drop fails the first time through, when there is no leftover
// table to drop
static void execImmediate(const char *what, const char *query, bool checked) {

	char	label[64];
	charstring::printf(label,sizeof(label),"oopen - %s",what);

	Cda_Def	cda;
	if (!openCursor(label,&cda)) {
		return;
	}

	// parse and execute are run and reported apart, so which of the two an
	// error came out of is never in doubt either
	charstring::printf(label,sizeof(label),"oparse - %s",what);
	bool	failed=!parse(label,&cda,query);
	if (!failed) {
		charstring::printf(label,sizeof(label),"oexec - %s",what);
		failed=!run(label,&cda,oexec(&cda));
	}

	if (failed && !checked) {
		stdoutput.printf("  (%s is best effort - "
					"an error here is not fatal)\n",what);
	}

	oclose(&cda);
}

static void createBindTable() {
	execImmediate("drop","drop table protocoltestbind",false);
	execImmediate("create","create table protocoltestbind ("
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
static bool define(const char *what, Cda_Def *cursor, sword pos,
				char *buf, sword bufsize,
				sb2 *ind, ub2 *retlen, ub2 *retcode) {
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
	if (!openCursor("oopen",&cda) || !parse("oparse",&cda,query)) {
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
		char	what[64];
		charstring::printf(what,sizeof(what),
					"odefin - column %d",(int)pos);
		if (!define(what,&cda,pos,buf[pos-1],(sword)sizeof(buf[pos-1]),
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

// like selectVariant's stringbind case, but for a value long enough to
// cross the wire format's one-byte length limit - above 252 bytes it has
// to switch to CLR_LONG_FORM_MARKER instead - see #9985. len is how long a
// value to bind; the caller picks it, so a later step can add more sizes
// without a new function. the row is fetched back and compared against
// what was bound, not just printed, so a truncation shows up as a
// mismatch rather than passing silently
static int selectLongStrVariant(sword len) {

	if (len>maxlongbindlen) {
		stdoutput.printf("  requested length %d exceeds "
					"maxlongbindlen %d\n",
					(int)len,(int)maxlongbindlen);
		return 1;
	}

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) ||
			!parse("oparse",&cda,"select :s from dual")) {
		return 1;
	}

	char	strvalue[maxlongbindlen+1];
	bytestring::set(strvalue,'x',(size_t)len);
	strvalue[len]='\0';

	sb2	bindind=0;
	if (!bind(&cda,":s",(ub1 *)strvalue,(sword)(len+1),
					SQLT_STR,&bindind)) {
		oclose(&cda);
		return 1;
	}

	char	buf[maxlongbindlen+1];
	sb2	ind=0;
	ub2	retlen=0;
	ub2	retcode=0;
	if (!define("odefin",&cda,1,buf,(sword)(len+1),
					&ind,&retlen,&retcode)) {
		oclose(&cda);
		return 1;
	}

	if (!run("oexec",&cda,oexec(&cda)) ||
			!run("ofen",&cda,ofen(&cda,1))) {
		oclose(&cda);
		return 1;
	}

	bool	match=!charstring::compare(buf,strvalue);
	stdoutput.printf("  row length=%d (bound %d) %s\n",
				(int)retlen,(int)len,
				match?"(matches)":"(MISMATCH)");

	run("oclose",&cda,oclose(&cda));
	return (match)?0:1;
}

// an IN bind of a DATE (SQLT_DAT), fetched back through a define of its own
// so the round trip can be checked rather than just printed.  binds and
// defines end up in the same request packet here, the same as
// selectVariant - this is the first OCI7 capture with a SQLT_DAT bind in it.
//
// the bound value is 01-JAN-2001 01:01:01, encoded the way OCI7 puts a date
// on the wire - excess-100 century and year, then month, day, and excess-1
// hour, minute and second.  oci7.cpp's "ofen - date" fetch of
// protocoltesttable's testdate for testnumber=1 already proved this exact
// byte layout on the define side, so this capture is directly comparable to
// that one
static int selectDateVariant() {

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) ||
			!parse("oparse",&cda,"select :d from dual")) {
		return 1;
	}

	ub1	datevalue[7]={120,101,1,1,2,2,2};
	sb2	bindind=0;
	if (!bind(&cda,":d",datevalue,(sword)sizeof(datevalue),
					SQLT_DAT,&bindind)) {
		oclose(&cda);
		return 1;
	}

	ub1	buf[7];
	sb2	ind=0;
	ub2	retlen=0;
	ub2	retcode=0;
	bytestring::zero(buf,sizeof(buf));
	if (!run("odefin",&cda,
			odefin(&cda,1,buf,(sword)sizeof(buf),SQLT_DAT,-1,
				&ind,(text *)0,-1,-1,&retlen,&retcode))) {
		oclose(&cda);
		return 1;
	}

	if (!run("oexec",&cda,oexec(&cda)) ||
			!run("ofen",&cda,ofen(&cda,1))) {
		oclose(&cda);
		return 1;
	}

	bool	match=!bytestring::compare(buf,datevalue,sizeof(buf));
	stdoutput.printf("  row length=%d %s\n",
				(int)retlen,
				match?"(matches)":"(MISMATCH)");

	run("oclose",&cda,oclose(&cda));
	return (match)?0:1;
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
	if (!openCursor("oopen",&cda) || !parse("oparse",&cda,bindinsert)) {
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

// read the table back through a cursor of its own and print every testnumber
// in it.
//
// A bind-once/execute-many run is only interesting for which of its executes
// landed a row, and neither thing the executes themselves report answers
// that: a rows-processed count is what the module said, not what the table
// holds, and the setup drop is not reliable enough to guarantee the table
// started out empty, so a row found afterward with sqlplus cannot be pinned
// to the run that was being watched. This select settles both, in the same
// process and the same capture.
//
// It runs in the session that did the inserts, which sees them whether or not
// they are committed, so no ocom is needed for it to see what landed.
//
// The count line is printed on every path out, failures included, so a run
// that never got as far as the select and a run that found nothing can be
// told apart in the output.
static void verifyBindTable() {

	int		rows=0;
	const char	*aborted=NULL;

	Cda_Def	cda;
	if (!openCursor("oopen - verify",&cda)) {
		stdoutput.printf("verify: 0 row(s) in protocoltestbind "
					"(aborted: oopen)\n");
		return;
	}

	const char	*query="select testnumber from protocoltestbind "
				"order by testnumber";

	char	buf[64];
	sb2	ind=0;
	ub2	retlen=0;
	ub2	retcode=0;

	if (!parse("oparse - verify",&cda,query)) {
		aborted="oparse";
	} else if (!define("odefin - verify",&cda,1,buf,(sword)sizeof(buf),
						&ind,&retlen,&retcode)) {
		aborted="odefin";
	} else if (!run("oexec - verify",&cda,oexec(&cda))) {
		aborted="oexec";
	} else {

		int	lastrpc=(int)cda.rpc;
		for (;;) {

			// the row cap keeps a module that never signals the
			// end of the fetch from spinning here forever
			if (rows>=maxverifyrows) {
				stdoutput.printf("verify: row cap (%d) "
						"reached, stopping\n",
						maxverifyrows);
				break;
			}

			bytestring::zero(buf,sizeof(buf));
			sword	fetched=ofen(&cda,1);

			// a client that hands back the last row and the
			// end-of-fetch signal in the same call leaves the row
			// in the buffer, so rpc, not the return code, is what
			// says whether there is a row to print
			if ((int)cda.rpc>lastrpc) {
				lastrpc=(int)cda.rpc;
				rows++;
				stdoutput.printf("verify: row testnumber=%s\n",
							(ind==-1)?"NULL":buf);
			}

			if (fetched) {
				if ((int)cda.rc!=nodatafound) {
					printError("ofen - verify",&cda);
					aborted="ofen";
				}
				break;
			}
		}
	}

	run("oclose - verify",&cda,oclose(&cda));

	if (aborted) {
		stdoutput.printf("verify: %d row(s) in protocoltestbind "
					"(aborted: %s)\n",rows,aborted);
	} else {
		stdoutput.printf("verify: %d row(s) in protocoltestbind\n",rows);
	}
}

// the three-bind insert, with or without null indicators, executed once or
// three times.  verify reads the table back afterward, whether the executes
// all ran or one of them gave up part way through
static int insertVariant(bool nulls, int iterations, bool verify) {

	createBindTable();

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse("oparse",&cda,bindinsert)) {
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
			if (verify) {
				verifyBindTable();
			}
			return 1;
		}
		stdoutput.printf("  rows processed: %d\n",(int)cda.rpc);
	}

	run("oclose",&cda,oclose(&cda));

	if (verify) {
		verifyBindTable();
	}

	return 0;
}

// a pl/sql block with one bind the block writes to
static int plsqlVariant(const char *block, sb4 invalue) {

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) || !parse("oparse",&cda,block)) {
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

// an in-out bind whose obndrv buffer size (512, maxlongbindlen) is far
// larger than its input value ("hi"), against a block that writes back 300
// bytes - the module has to read that buffer size off the wire and size the
// out-bind buffer to it, rather than to the input value's length or a fixed
// floor. the row is compared against what the block should have written
// back, not just printed, so a truncation shows up as a mismatch
static int bigInOutVariant() {

	Cda_Def	cda;
	if (!openCursor("oopen",&cda) ||
			!parse("oparse",&cda,
				"begin :v := rpad('X',300,'X'); end;")) {
		return 1;
	}

	char	buf[maxlongbindlen+1];
	bytestring::zero(buf,sizeof(buf));
	charstring::copy(buf,"hi");

	sb2	ind=0;
	if (!bind(&cda,":v",(ub1 *)buf,(sword)maxlongbindlen,SQLT_STR,&ind)) {
		oclose(&cda);
		return 1;
	}

	if (!run("oexec",&cda,oexec(&cda))) {
		oclose(&cda);
		return 1;
	}

	char	expected[301];
	bytestring::set(expected,'X',300);
	expected[300]='\0';

	bool	match=!charstring::compare(buf,expected);
	stdoutput.printf("  value length=%d ind=%d %s\n",
				(int)charstring::getLength(buf),(int)ind,
				match?"(matches)":"(MISMATCH)");

	run("oclose",&cda,oclose(&cda));
	return (match)?0:1;
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
				"[--bind=selectint|selectstr|selectstrlong|"
				"selecttwo|datebind|"
				"insert|nullbind|many|reverse|out|inout|nullout|"
				"biginout] "
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
	} else if (!charstring::compare(variant,"selectstrlong")) {
		result=selectLongStrVariant(300);
	} else if (!charstring::compare(variant,"selecttwo")) {
		result=selectVariant("select :num, :b from dual",true,false);
	} else if (!charstring::compare(variant,"datebind")) {
		result=selectDateVariant();
	} else if (!charstring::compare(variant,"insert")) {
		result=insertVariant(false,1,false);
	} else if (!charstring::compare(variant,"nullbind")) {
		result=insertVariant(true,1,false);
	} else if (!charstring::compare(variant,"many")) {
		result=insertVariant(false,3,true);
	} else if (!charstring::compare(variant,"reverse")) {
		result=reverseVariant();
	} else if (!charstring::compare(variant,"out")) {
		// an out-only bind: the block never reads :v
		result=plsqlVariant("begin :v := 7; end;",0);
	} else if (!charstring::compare(variant,"inout")) {
		result=plsqlVariant("begin :v := :v * 2; end;",21);
	} else if (!charstring::compare(variant,"nullout")) {
		result=plsqlVariant("begin :v := NULL; end;",99);
	} else if (!charstring::compare(variant,"biginout")) {
		result=bigInOutVariant();
	} else {
		stdoutput.printf("unknown --bind= variant: %s\n",variant);
		ologof(&lda);
		return 1;
	}

	run("ologof",&lda,ologof(&lda));

	stdoutput.printf("done\n");
	return result;
}
