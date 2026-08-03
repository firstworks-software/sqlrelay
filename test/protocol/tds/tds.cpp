extern "C" {
	#include <ctpublic.h>
	#include <bkpublic.h>
}
#include <rudiments/sys.h>
#include <rudiments/process.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stringbuffer.h>
#include <rudiments/stdio.h>
#include <config.h>

#include "asserts.cpp"

CS_RETCODE csMessageCallback(CS_CONTEXT *ctxt, CS_CLIENTMSG *msgp) {

	stringbuffer	errorstring;
	errorstring.append("Client Library error: ")->append(msgp->msgstring);
	errorstring.append(" severity(")->
		append((int32_t)CS_SEVERITY(msgp->msgnumber))->append(")");
	errorstring.append(" layer(")->
		append((int32_t)CS_LAYER(msgp->msgnumber))->append(")");
	errorstring.append(" origin(")->
		append((int32_t)CS_ORIGIN(msgp->msgnumber))->append(")");
	errorstring.append(" number(")->
		append((int32_t)CS_NUMBER(msgp->msgnumber))->append(")");

	if (msgp->osstringlen>0) {
		errorstring.append("  Operating System Error: ");
		errorstring.append(msgp->osstring);
	}

	stdoutput.printf("%s\n",errorstring.getString());

	return CS_SUCCEED;
}

CS_RETCODE clientMessageCallback(CS_CONTEXT *ctxt, 
					CS_CONNECTION *cnn,
					CS_CLIENTMSG *msgp) {
	stringbuffer	errorstring;
	errorstring.append("Client Library error: ")->append(msgp->msgstring);
	errorstring.append(" severity(")->
		append((int32_t)CS_SEVERITY(msgp->msgnumber))->append(")");
	errorstring.append(" layer(")->
		append((int32_t)CS_LAYER(msgp->msgnumber))->append(")");
	errorstring.append(" origin(")->
		append((int32_t)CS_ORIGIN(msgp->msgnumber))->append(")");
	errorstring.append(" number(")->
		append((int32_t)CS_NUMBER(msgp->msgnumber))->append(")");

	if (msgp->osstringlen>0) {
		errorstring.append("  Operating System Error: ");
		errorstring.append(msgp->osstring);
	}

	stdoutput.printf("%s\n",errorstring.getString());

	return CS_SUCCEED;
}

CS_RETCODE serverMessageCallback(CS_CONTEXT *ctxt, 
					CS_CONNECTION *cnn,
					CS_SERVERMSG *msgp) {
	stringbuffer	errorstring;
	errorstring.append("Server message: ")->append(msgp->text);
	errorstring.append(" severity(")->
		append((int32_t)CS_SEVERITY(msgp->msgnumber))->append(")");
	errorstring.append(" number(")->
		append((int32_t)CS_NUMBER(msgp->msgnumber))->append(")");
	errorstring.append(" state(")->
		append((int32_t)msgp->state)->append(")");
	errorstring.append(" line(")->
		append((int32_t)msgp->line)->append(")");
	errorstring.append("  Server Name:")->append(msgp->svrname);
	errorstring.append("  Procedure Name:")->append(msgp->proc);

	stdoutput.printf("%s\n",errorstring.getString());

	return CS_SUCCEED;
}

int	main(int argc, char **argv) {

	const char	*server;
	const char	*user;
	const char	*password;
	const char	*db;
	const char	*language="us_english";
	const char	*charset="utf-8";

	// pass "native" to test a real sql server or sybase instance
	// instead of sqlrelay's tds protocol.  a second argument picks
	// which one - "mssql" (the default) or "sybase".
	bool	issqlrelay=!(argc>=2 && !charstring::compare(argv[1],"native"));
	bool	issybase=false;
	if (issqlrelay) {
		server="sqlrelay";
		db="";
	} else {
		issybase=(argc>=3 && !charstring::compare(argv[2],"sybase"));
		// short hostname, matching the db the native odbc tests use
		char	*hostname=sys::getHostName();
		char	*dot=(char *)charstring::findFirstOrEnd(hostname,'.');
		*dot='\0';
		server=(issybase)?"sybase":"mssql";
		db=hostname;
	}
	user="testuser";
	password="testpassword";


	CS_CONTEXT	*context=NULL;
	CS_CONNECTION	*dbconn=NULL;

	// point ct-lib at the in-tree freetds.conf rather than the system
	// one, so the test needs no hand-edited config.  the path is
	// relative to test/protocol/tds, where run_protocol_test runs it
	// from.  SYBASE is ignored by ct-lib here, even set absolute.
	environment::setValue("FREETDSCONF",
			"../../sqlrelay.conf.d/freetds/etc/freetds.conf");

	environment::setValue("DSQUERY",server);

	stdoutput.printf("\n================ Login ================\n\n");

	stdoutput.printf("cs_ctx_alloc\n");
	assertEquals(cs_ctx_alloc(CS_VERSION_100,&context),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_init\n");
	assertEquals(ct_init(context,CS_VERSION_100),CS_SUCCEED);
	stdoutput.printf("\n");


	// Freetds waits forever by default.  Without these timeouts, a
	// protocol module that never finishes the login handshake hangs
	// this test, and make tests along with it.
	stdoutput.printf("ct_config: timeouts\n");
	CS_INT	logintimeout=30;
	assertEquals(ct_config(context,CS_SET,CS_LOGIN_TIMEOUT,
				(CS_VOID *)&logintimeout,CS_UNUSED,
				(CS_INT *)NULL),CS_SUCCEED);
	CS_INT	cmdtimeout=60;
	assertEquals(ct_config(context,CS_SET,CS_TIMEOUT,
				(CS_VOID *)&cmdtimeout,CS_UNUSED,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_config: callbacks\n");
	assertEquals(cs_config(context,CS_SET,CS_MESSAGE_CB,
				(CS_VOID *)csMessageCallback,
				CS_UNUSED,(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ct_callback(context,NULL,CS_SET,CS_CLIENTMSG_CB,
				(CS_VOID *)clientMessageCallback),
				CS_SUCCEED);
	assertEquals(ct_callback(context,NULL,CS_SET,CS_SERVERMSG_CB,
				(CS_VOID *)serverMessageCallback),
				CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_con_alloc\n");
	assertEquals(ct_con_alloc(context,&dbconn),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_con_props: user\n");
	assertEquals(ct_con_props(dbconn,CS_SET,
				CS_USERNAME,(CS_VOID *)user,CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_con_props: password\n");
	assertEquals(ct_con_props(dbconn,CS_SET,
				CS_PASSWORD,(CS_VOID *)password,CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_con_props: appname\n");
	assertEquals(ct_con_props(dbconn,CS_SET,
			CS_APPNAME,(CS_VOID *)"SQL Relay Test",CS_NULLTERM,
			(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_con_props: packet size\n");
	// ASE rejects anything over 2048 at login time
	uint16_t	ps=(issybase)?2048:4096;
	assertEquals(ct_con_props(dbconn,CS_SET,
				CS_PACKETSIZE,(CS_VOID *)&ps,sizeof(ps),
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	#ifdef CS_SEC_ENCRYPTION
	stdoutput.printf("cs_con_props: sec encryption\n");
	CS_INT	enc=CS_TRUE;
	assertEquals(ct_con_props(dbconn,CS_SET,
				CS_SEC_ENCRYPTION,(CS_VOID *)&enc,CS_UNUSED,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	#endif
	stdoutput.printf("cs_loc_alloc\n");
	CS_LOCALE	*locale=NULL;
	assertEquals(cs_loc_alloc(context,&locale),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_locale: lc_all\n");
	assertEquals(cs_locale(context,CS_SET,
				locale,CS_LC_ALL,(CS_CHAR *)NULL,
				CS_UNUSED,(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_locale: language\n");
	assertEquals(cs_locale(context,CS_SET,locale,
				CS_SYB_LANG,(CS_CHAR *)language,CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_locale: charset\n");
	assertEquals(cs_locale(context,CS_SET,locale,
				CS_SYB_CHARSET,(CS_CHAR *)charset,CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");
	stdoutput.printf("cs_con_props: locale\n");
	assertEquals(ct_con_props(dbconn,CS_SET,
				CS_LOC_PROP,(CS_VOID *)locale,CS_UNUSED,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_connect\n");
	CS_RETCODE	connected=ct_connect(dbconn,(CS_CHAR *)NULL,(CS_INT)0);
	assertEquals(connected,CS_SUCCEED);
	stdoutput.printf("\n\n");

	// Nothing below here can work without a connection.  Running on
	// anyway leaves the column count unset and spins the describe
	// loop for millions of iterations.
	if (connected!=CS_SUCCEED) {
		reportTestStatus();
		return status;
	}



	stdoutput.printf("\n=============== Queries ===============\n\n");

	CS_INT		resultstype;


	stdoutput.printf("ct_cmd_alloc: cmd\n");
	CS_COMMAND	*cmd=NULL;
	assertEquals(ct_cmd_alloc(dbconn,&cmd),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: use db\n");
	stringbuffer	q;
	q.append("use ")->append(db);
	const char	*query=q.getString();
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	CS_INT	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");

	query="drop table testtable";
	ct_command(cmd,CS_LANG_CMD,query,charstring::getLength(query),CS_UNUSED);
	ct_send(cmd);
	while (ct_results(cmd,&resultstype)==CS_SUCCEED) {}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);


	stdoutput.printf("ct_command: create\n");
	if (issybase) {
		// sybase has no uniqueidentifier, datetime2,
		// datetimeoffset, xml, or ntext
		query="create table testtable ("
				"testtinyint tinyint, "
				"testbit bit, "
				"testsmallint smallint, "
				"testint int, "
				"testsmalldatetime smalldatetime, "
				"testreal real, "
				"testmoney money, "
				"testdatetime datetime, "
				"testfloat float, "
				"testsmallmoney smallmoney, "
				"testbigint bigint, "
				"testdecimal decimal(3,2), "
				"testnumeric numeric(3,2), "
				"testdate date, "
				"testtime time, "
				"testchar char(40), "
				"testvarchar varchar(40), "
				"testbinary binary(40), "
				"testvarbinary varbinary(40), "
				"testnvarchar nvarchar(40), "
				"testnchar nchar(40), "
				"testtext text, "
				"testimage image"
				") lock datarows";
	} else {
		query="create table testtable ("
				"testtinyint tinyint, "
				"testbit bit, "
				"testsmallint smallint, "
				"testint int, "
				"testsmalldatetime smalldatetime, "
				"testreal real, "
				"testmoney money, "
				"testdatetime datetime, "
				"testfloat float, "
				"testsmallmoney smallmoney, "
				"testbigint bigint, "
				"testguid uniqueidentifier, "
				"testdecimal decimal(3,2), "
				"testnumeric numeric(3,2), "
				"testdate date, "
				"testtime time, "
				"testdatetime2 datetime2, "
				"testdatetimeoffset datetimeoffset, "
				"testchar char(40), "
				"testvarchar varchar(40), "
				"testbinary binary(40), "
				"testvarbinary varbinary(40), "
				"testnvarchar nvarchar(40), "
				"testnchar nchar(40), "
				"testxml xml, "
				"testtext text, "
				"testimage image, "
				"testntext ntext"
				")";
	}
	assertEquals(ct_command(cmd,CS_LANG_CMD,
				query,charstring::getLength(query),
				CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: insert\n");
	if (issybase) {
		query="insert into testtable values ("
				"1,"
				"1,"
				"1,"
				"1,"
				"'2001-01-01 13:01:01.000', "
				"1.5, "
				"1.5, "
				"'2001-01-01 13:01:01:000', "
				"1.5, "
				"1.5, "
				"1, "
				"1.5, "
				"1.5, "
				"'2001-01-01', "
				"'13:01:01.000', "
				"'char1', "
				"'varchar1', "
				"CONVERT(binary, 'binary1'), "
				"CONVERT(varbinary, 'varbinary1'), "
				"'nvarchar1', "
				"'nchar1', "
				"'text1', "
				"CONVERT(image, 'image1')"
				")";
	} else {
		query="insert into testtable values ("
				"1,"
				"1,"
				"1,"
				"1,"
				"'2001-01-01 13:01:01.000', "
				"1.5, "
				"1.5, "
				"'2001-01-01 13:01:01:000', "
				"1.5, "
				"1.5, "
				"1, "
				"'01020304-0102-0304-0102-030401020304', "
				"1.5, "
				"1.5, "
				"'2001-01-01', "
				"'13:01:01.000', "
				"'2001-01-01T13:01:01.000', "
				"'2001-01-01 13:01:01.000', "
				"'char1', "
				"'varchar1', "
				"CONVERT(binary, 'binary1'), "
				"CONVERT(varbinary, 'varbinary1'), "
				"'nvarchar1', "
				"'nchar1', "
				"'xml1', "
				"'text1', "
				"CONVERT(image, 'image1'), "
				"'ntext1'"
				")";
	}
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	CS_INT	affectedrows;
	assertEquals(ct_res_info(cmd,CS_ROW_COUNT,
					(CS_VOID *)&affectedrows,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(affectedrows,1);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: insert\n");
	if (issybase) {
		query="insert into testtable values ("
				"2,"
				"1,"
				"2,"
				"2,"
				"'2002-02-02 14:02:02.000', "
				"2.5, "
				"2.5, "
				"'2002-02-02 14:02:02.000', "
				"2.5, "
				"2.5, "
				"2, "
				"2.5, "
				"2.5, "
				"'2002-02-02', "
				"'14:02:02.000', "
				"'char2', "
				"'varchar2', "
				"CONVERT(binary, 'binary2'), "
				"CONVERT(varbinary, 'varbinary2'), "
				"'nvarchar2', "
				"'nchar2', "
				"'text2', "
				"CONVERT(image, 'image2')"
				")";
	} else {
		query="insert into testtable values ("
				"2,"
				"1,"
				"2,"
				"2,"
				"'2002-02-02 14:02:02.000', "
				"2.5, "
				"2.5, "
				"'2002-02-02 14:02:02.000', "
				"2.5, "
				"2.5, "
				"2, "
				"'01020304-0102-0304-0102-030401020304', "
				"2.5, "
				"2.5, "
				"'2002-02-02', "
				"'14:02:02.000', "
				"'2002-02-02T14:02:02.000', "
				"'2002-02-02 14:02:02.000', "
				"'char2', "
				"'varchar2', "
				"CONVERT(binary, 'binary2'), "
				"CONVERT(varbinary, 'varbinary2'), "
				"'nvarchar2', "
				"'nchar2', "
				"'xml2', "
				"'text2', "
				"CONVERT(image, 'image2'), "
				"'ntext2'"
				")";
	}
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	assertEquals(ct_res_info(cmd,CS_ROW_COUNT,
					(CS_VOID *)&affectedrows,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(affectedrows,1);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: select\n");
	query="select * from testtable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	stdoutput.printf("\n");

	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	stdoutput.printf("\n");


	// mssql's 28 columns are the canonical order.  present[] flags the
	// ones this server actually has, so the arrays below stay indexed
	// by that order no matter which server we're talking to.
	bool	present[28];
	for (CS_INT i=0; i<28; i++) {
		present[i]=true;
	}
	if (issybase) {
		present[11]=false;	// uniqueidentifier
		present[16]=false;	// datetime2
		present[17]=false;	// datetimeoffset
		present[24]=false;	// xml
		present[27]=false;	// ntext
	}


	stdoutput.printf("ct_res_info: col count\n");
	CS_INT	ncols=0;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,(issybase)?23:28);
	stdoutput.printf("\n");


	stdoutput.printf("ct_bind:\n");
	CS_DATAFMT	column[28];
	char		*data[28];
	CS_INT		*datalength[28];
	CS_SMALLINT	*nullindicator[28];
	CS_INT		col=0;
	for (CS_INT i=0; i<28; i++) {
		if (!present[i]) {
			data[i]=NULL;
			datalength[i]=NULL;
			nullindicator[i]=NULL;
			continue;
		}
		col++;

		data[i]=new char[1024];
		bytestring::zero(data[i],1024);
		datalength[i]=new CS_INT[1];
		nullindicator[i]=new CS_SMALLINT[1];

		column[i].datatype=CS_CHAR_TYPE;
		column[i].format=CS_FMT_NULLTERM;
		column[i].maxlength=1024;
		column[i].scale=CS_UNUSED;
		column[i].precision=CS_UNUSED;
		column[i].status=CS_UNUSED;
		column[i].count=1;
		column[i].usertype=CS_UNUSED;
		column[i].locale=NULL;
		assertEquals(ct_bind(cmd,col,&(column[i]),
						(CS_VOID *)data[i],
						datalength[i],
						nullindicator[i]),
						CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_describe:\n");
	col=0;
	for (CS_INT i=0; i<28; i++) {
		if (!present[i]) {
			continue;
		}
		col++;
		assertEquals(ct_describe(cmd,col,&(column[i])),CS_SUCCEED);
	}
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[0].name);
	assertEquals(column[0].name,"testtinyint");
	assertEquals(column[0].datatype,CS_TINYINT_TYPE);
	assertEquals(column[0].format,CS_FMT_NULLTERM);
	assertEquals(column[0].maxlength,1);
	assertEquals(column[0].precision,0);
	assertEquals(column[0].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[0].status,CS_UNUSED);
	assertEquals(column[0].count,1);
	// ase sends each column's systypes usertype, mssql sends 0
	assertEquals(column[0].usertype,(issybase)?5:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[1].name);
	assertEquals(column[1].name,"testbit");
	assertEquals(column[1].datatype,CS_BIT_TYPE);
	assertEquals(column[1].format,CS_FMT_NULLTERM);
	assertEquals(column[1].maxlength,1);
	assertEquals(column[1].precision,0);
	assertEquals(column[1].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[1].status,CS_UNUSED);
	assertEquals(column[1].count,1);
	assertEquals(column[1].usertype,(issybase)?16:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[2].name);
	assertEquals(column[2].name,"testsmallint");
	assertEquals(column[2].datatype,CS_SMALLINT_TYPE);
	assertEquals(column[2].format,CS_FMT_NULLTERM);
	assertEquals(column[2].maxlength,2);
	assertEquals(column[2].precision,0);
	assertEquals(column[2].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[2].status,CS_UNUSED);
	assertEquals(column[2].count,1);
	assertEquals(column[2].usertype,(issybase)?6:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[3].name);
	assertEquals(column[3].name,"testint");
	assertEquals(column[3].datatype,CS_INT_TYPE);
	assertEquals(column[3].format,CS_FMT_NULLTERM);
	assertEquals(column[3].maxlength,4);
	assertEquals(column[3].precision,0);
	assertEquals(column[3].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[3].status,CS_UNUSED);
	assertEquals(column[3].count,1);
	assertEquals(column[3].usertype,(issybase)?7:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[4].name);
	assertEquals(column[4].name,"testsmalldatetime");
	assertEquals(column[4].datatype,CS_DATETIME4_TYPE);
	assertEquals(column[4].format,CS_FMT_NULLTERM);
	assertEquals(column[4].maxlength,4);
	assertEquals(column[4].precision,0);
	assertEquals(column[4].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[4].status,CS_UNUSED);
	assertEquals(column[4].count,1);
	assertEquals(column[4].usertype,(issybase)?22:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[5].name);
	assertEquals(column[5].name,"testreal");
	assertEquals(column[5].datatype,CS_REAL_TYPE);
	assertEquals(column[5].format,CS_FMT_NULLTERM);
	assertEquals(column[5].maxlength,4);
	assertEquals(column[5].precision,0);
	assertEquals(column[5].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[5].status,CS_UNUSED);
	assertEquals(column[5].count,1);
	assertEquals(column[5].usertype,(issybase)?23:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[6].name);
	assertEquals(column[6].name,"testmoney");
	assertEquals(column[6].datatype,CS_MONEY_TYPE);
	assertEquals(column[6].format,CS_FMT_NULLTERM);
	assertEquals(column[6].maxlength,8);
	assertEquals(column[6].precision,0);
	assertEquals(column[6].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[6].status,CS_UNUSED);
	assertEquals(column[6].count,1);
	assertEquals(column[6].usertype,(issybase)?11:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[7].name);
	assertEquals(column[7].name,"testdatetime");
	assertEquals(column[7].datatype,CS_DATETIME_TYPE);
	assertEquals(column[7].format,CS_FMT_NULLTERM);
	assertEquals(column[7].maxlength,8);
	assertEquals(column[7].precision,0);
	assertEquals(column[7].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[7].status,CS_UNUSED);
	assertEquals(column[7].count,1);
	assertEquals(column[7].usertype,(issybase)?12:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[8].name);
	assertEquals(column[8].name,"testfloat");
	assertEquals(column[8].datatype,CS_FLOAT_TYPE);
	assertEquals(column[8].format,CS_FMT_NULLTERM);
	assertEquals(column[8].maxlength,8);
	assertEquals(column[8].precision,0);
	assertEquals(column[8].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[8].status,CS_UNUSED);
	assertEquals(column[8].count,1);
	assertEquals(column[8].usertype,(issybase)?8:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[9].name);
	assertEquals(column[9].name,"testsmallmoney");
	assertEquals(column[9].datatype,CS_MONEY4_TYPE);
	assertEquals(column[9].format,CS_FMT_NULLTERM);
	assertEquals(column[9].maxlength,4);
	assertEquals(column[9].precision,0);
	assertEquals(column[9].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[9].status,CS_UNUSED);
	assertEquals(column[9].count,1);
	assertEquals(column[9].usertype,(issybase)?21:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[10].name);
	assertEquals(column[10].name,"testbigint");
	assertEquals(column[10].datatype,CS_BIGINT_TYPE);
	assertEquals(column[10].format,CS_FMT_NULLTERM);
	assertEquals(column[10].maxlength,8);
	assertEquals(column[10].precision,0);
	assertEquals(column[10].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[10].status,CS_UNUSED);
	assertEquals(column[10].count,1);
	assertEquals(column[10].usertype,(issybase)?43:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	if (present[11]) {
		stdoutput.printf("%s\n",column[11].name);
		assertEquals(column[11].name,"testguid");
		assertEquals(column[11].datatype,CS_UNIQUE_TYPE);
		assertEquals(column[11].format,CS_FMT_NULLTERM);
		assertEquals(column[11].maxlength,16);
		assertEquals(column[11].precision,0);
		assertEquals(column[11].scale,0);
		// FIXME: 48 direct, 0 via relay
		//assertEquals(column[11].status,CS_UNUSED);
		assertEquals(column[11].count,1);
		assertEquals(column[11].usertype,CS_CHAR_TYPE);
		stdoutput.printf("\n");
	}

	stdoutput.printf("%s\n",column[12].name);
	assertEquals(column[12].name,"testdecimal");
	assertEquals(column[12].datatype,CS_DECIMAL_TYPE);
	assertEquals(column[12].format,CS_FMT_NULLTERM);
	assertEquals(column[12].maxlength,35);
	assertEquals(column[12].precision,3);
	assertEquals(column[12].scale,2);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[12].status,CS_UNUSED);
	assertEquals(column[12].count,1);
	assertEquals(column[12].usertype,(issybase)?26:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[13].name);
	assertEquals(column[13].name,"testnumeric");
	assertEquals(column[13].datatype,CS_NUMERIC_TYPE);
	assertEquals(column[13].format,CS_FMT_NULLTERM);
	assertEquals(column[13].maxlength,35);
	assertEquals(column[13].precision,3);
	assertEquals(column[13].scale,2);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[13].status,CS_UNUSED);
	assertEquals(column[13].count,1);
	assertEquals(column[13].usertype,(issybase)?10:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[14].name);
	assertEquals(column[14].name,"testdate");
	assertEquals(column[14].datatype,CS_DATE_TYPE);
	assertEquals(column[14].format,CS_FMT_NULLTERM);
	// FIXME: 64 direct, 16 via relay
	//assertEquals(column[14].maxlength,64);
	assertEquals(column[14].precision,0);
	assertEquals(column[14].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[14].status,CS_UNUSED);
	assertEquals(column[14].count,1);
	assertEquals(column[14].usertype,(issybase)?37:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[15].name);
	assertEquals(column[15].name,"testtime");
	assertEquals(column[15].datatype,
			(issybase)?CS_TIME_TYPE:CS_BIGTIME_TYPE);
	assertEquals(column[15].format,CS_FMT_NULLTERM);
	// FIXME: 16/7/7 direct, 64/0/0 via relay
	//assertEquals(column[15].maxlength,16);
	//assertEquals(column[15].precision,7);
	//assertEquals(column[15].scale,7);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[15].status,CS_UNUSED);
	assertEquals(column[15].count,1);
	assertEquals(column[15].usertype,(issybase)?38:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	if (present[16]) {
		stdoutput.printf("%s\n",column[16].name);
		assertEquals(column[16].name,"testdatetime2");
		assertEquals(column[16].datatype,CS_BIGDATETIME_TYPE);
		assertEquals(column[16].format,CS_FMT_NULLTERM);
		// FIXME: 16/7/7 direct, 64/0/0 via relay
		//assertEquals(column[16].maxlength,16);
		//assertEquals(column[16].precision,7);
		//assertEquals(column[16].scale,7);
		// FIXME: 48 direct, 0 via relay
		//assertEquals(column[16].status,CS_UNUSED);
		assertEquals(column[16].count,1);
		assertEquals(column[16].usertype,CS_CHAR_TYPE);
		stdoutput.printf("\n");
	}

	if (present[17]) {
		stdoutput.printf("%s\n",column[17].name);
		assertEquals(column[17].name,"testdatetimeoffset");
		assertEquals(column[17].datatype,CS_BIGDATETIME_TYPE);
		assertEquals(column[17].format,CS_FMT_NULLTERM);
		// FIXME: 16/7/7 direct, 64/0/0 via relay
		//assertEquals(column[17].maxlength,16);
		//assertEquals(column[17].precision,7);
		//assertEquals(column[17].scale,7);
		// FIXME: 48 direct, 0 via relay
		//assertEquals(column[17].status,CS_UNUSED);
		assertEquals(column[17].count,1);
		assertEquals(column[17].usertype,CS_CHAR_TYPE);
		stdoutput.printf("\n");
	}

	stdoutput.printf("%s\n",column[18].name);
	assertEquals(column[18].name,"testchar");
	assertEquals(column[18].datatype,CS_CHAR_TYPE);
	assertEquals(column[18].format,CS_FMT_NULLTERM);
	// FIXME: 40 direct, 160 via relay	#4783
	//assertEquals(column[18].maxlength,40);
	assertEquals(column[18].precision,0);
	assertEquals(column[18].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[18].status,CS_UNUSED);
	assertEquals(column[18].count,1);
	assertEquals(column[18].usertype,(issybase)?1:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[19].name);
	assertEquals(column[19].name,"testvarchar");
	//assertEquals(column[19].datatype,CS_VARCHAR_TYPE);
	assertEquals(column[19].datatype,CS_CHAR_TYPE);		// #4652
	assertEquals(column[19].format,CS_FMT_NULLTERM);
	// FIXME: 40 direct, 160 via relay	#4783
	//assertEquals(column[19].maxlength,40);
	assertEquals(column[19].precision,0);
	assertEquals(column[19].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[19].status,CS_UNUSED);
	assertEquals(column[19].count,1);
	assertEquals(column[19].usertype,(issybase)?2:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[20].name);
	assertEquals(column[20].name,"testbinary");
	assertEquals(column[20].datatype,CS_BINARY_TYPE);
	assertEquals(column[20].format,CS_FMT_NULLTERM);
	assertEquals(column[20].maxlength,40);
	assertEquals(column[20].precision,0);
	assertEquals(column[20].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[20].status,CS_UNUSED);
	assertEquals(column[20].count,1);
	assertEquals(column[20].usertype,(issybase)?3:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[21].name);
	assertEquals(column[21].name,"testvarbinary");
	//assertEquals(column[21].datatype,CS_VARBINARY_TYPE);
	assertEquals(column[21].datatype,CS_BINARY_TYPE);	// #4781
	assertEquals(column[21].format,CS_FMT_NULLTERM);
	assertEquals(column[21].maxlength,40);
	assertEquals(column[21].precision,0);
	assertEquals(column[21].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[21].status,CS_UNUSED);
	assertEquals(column[21].count,1);
	assertEquals(column[21].usertype,(issybase)?4:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[22].name);
	assertEquals(column[22].name,"testnvarchar");
	//assertEquals(column[22].datatype,CS_NVARCHAR_TYPE);
	assertEquals(column[22].datatype,CS_CHAR_TYPE);		// #4652
	assertEquals(column[22].format,CS_FMT_NULLTERM);
	// FIXME: 40 direct, 160 via relay	#4783
	//assertEquals(column[22].maxlength,40);
	assertEquals(column[22].precision,0);
	assertEquals(column[22].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[22].status,CS_UNUSED);
	assertEquals(column[22].count,1);
	assertEquals(column[22].usertype,(issybase)?25:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[23].name);
	assertEquals(column[23].name,"testnchar");
	//assertEquals(column[23].datatype,CS_NCHAR_TYPE);
	assertEquals(column[23].datatype,CS_CHAR_TYPE);		// #4652
	assertEquals(column[23].format,CS_FMT_NULLTERM);
	// FIXME: 40 direct, 160 via relay	#4783
	//assertEquals(column[23].maxlength,40);
	assertEquals(column[23].precision,0);
	assertEquals(column[23].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[23].status,CS_UNUSED);
	assertEquals(column[23].count,1);
	assertEquals(column[23].usertype,(issybase)?24:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	if (present[24]) {
		stdoutput.printf("%s\n",column[24].name);
		assertEquals(column[24].name,"testxml");
		//assertEquals(column[24].datatype,CS_XML_TYPE);
		assertEquals(column[24].datatype,CS_LONGCHAR_TYPE);
		assertEquals(column[24].format,CS_FMT_NULLTERM);
		// maxlength limited by maxfieldlength via relay,
		// but not directly
		//assertEquals(column[24].maxlength,131068);
		assertEquals(column[24].precision,0);
		assertEquals(column[24].scale,0);
		// FIXME: 48 direct, 0 via relay
		//assertEquals(column[24].status,CS_UNUSED);
		assertEquals(column[24].count,1);
		assertEquals(column[24].usertype,CS_CHAR_TYPE);
		stdoutput.printf("\n");
	}

	stdoutput.printf("%s\n",column[25].name);
	assertEquals(column[25].name,"testtext");
	assertEquals(column[25].datatype,CS_TEXT_TYPE);
	assertEquals(column[25].format,CS_FMT_NULLTERM);
	// maxlength limited by maxfieldlength via relay, but not directly
	//assertEquals(column[25].maxlength,131068);
	assertEquals(column[25].precision,0);
	assertEquals(column[25].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[25].status,CS_UNUSED);
	assertEquals(column[25].count,1);
	assertEquals(column[25].usertype,(issybase)?19:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[26].name);
	assertEquals(column[26].name,"testimage");
	assertEquals(column[26].datatype,CS_IMAGE_TYPE);
	assertEquals(column[26].format,CS_FMT_NULLTERM);
	// maxlength limited by maxfieldlength via relay, but not directly
	//assertEquals(column[26].maxlength,131068);
	assertEquals(column[26].precision,0);
	assertEquals(column[26].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[26].status,CS_UNUSED);
	assertEquals(column[26].count,1);
	assertEquals(column[26].usertype,(issybase)?20:CS_CHAR_TYPE);
	stdoutput.printf("\n");

	if (present[27]) {
		stdoutput.printf("%s\n",column[27].name);
		assertEquals(column[27].name,"testntext");
		assertEquals(column[27].datatype,CS_TEXT_TYPE);
		assertEquals(column[27].format,CS_FMT_NULLTERM);
		// maxlength limited by maxfieldlength via relay,
		// but not directly
		//assertEquals(column[27].maxlength,131068);
		assertEquals(column[27].precision,0);
		assertEquals(column[27].scale,0);
		// FIXME: 48 direct, 0 via relay
		//assertEquals(column[27].status,CS_UNUSED);
		assertEquals(column[27].count,1);
		assertEquals(column[27].usertype,CS_CHAR_TYPE);
		stdoutput.printf("\n");
	}


	stdoutput.printf("ct_fetch:\n");
	CS_INT	rowsread;
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	stdoutput.printf("row data:\n");
	assertEquals(data[0],"1");
	assertEquals(*(datalength[0]),2);
	assertEquals(*(nullindicator[0]),0);
	assertEquals(data[1],"1");
	assertEquals(*(datalength[1]),2);
	assertEquals(*(nullindicator[1]),0);
	assertEquals(data[2],"1");
	assertEquals(*(datalength[2]),2);
	assertEquals(*(nullindicator[2]),0);
	assertEquals(data[3],"1");
	assertEquals(*(datalength[3]),2);
	assertEquals(*(nullindicator[3]),0);
	assertEquals(data[4],"Jan  1 2001 01:01:00:000PM");
	assertEquals(*(datalength[4]),27);
	assertEquals(*(nullindicator[4]),0);
	// reals aren't converted to strings reliably enough to compare
	//assertEquals(data[5],"1.5");
	assertEquals(*(nullindicator[5]),0);
	// moneys aren't converted to strings reliably enough to compare
	//assertEquals(data[6],"1.50");
	assertEquals(*(nullindicator[6]),0);
	assertEquals(data[7],"Jan  1 2001 01:01:01:000PM");
	assertEquals(*(datalength[7]),27);
	assertEquals(*(nullindicator[7]),0);
	// floats aren't converted to strings reliably enough to compare
	//assertEquals(data[8],"1.5");
	assertEquals(*(nullindicator[8]),0);
	// smallmoneys aren't converted to strings reliably enough to compare
	//assertEquals(data[9],"1.50");
	assertEquals(*(nullindicator[9]),0);
	assertEquals(data[10],"1");
	assertEquals(*(datalength[10]),2);
	assertEquals(*(nullindicator[10]),0);
	if (present[11]) {
		assertEquals(data[11],
				"01020304-0102-0304-0102-030401020304");
		assertEquals(*(datalength[11]),37);
		assertEquals(*(nullindicator[11]),0);
	}
	assertEquals(data[12],"1.50");
	assertEquals(*(datalength[12]),5);
	assertEquals(*(nullindicator[12]),0);
	assertEquals(data[13],"1.50");
	assertEquals(*(datalength[13]),5);
	assertEquals(*(nullindicator[13]),0);
	// FIXME: #4780 date, time, datetime2, datetimeoffset types
	//assertEquals(data[14],"2001-01-01");
	//assertEquals(data[15],"13:01:01.0000000");
	//assertEquals(data[16],"2001-01-01 13:01:01.0000000");
	//assertEquals(data[17],"2001-01-01 13:01:01.0000000 +00:00");
	assertEquals(data[18],"char1                                   ");
	assertEquals(*(datalength[18]),41);
	assertEquals(*(nullindicator[18]),0);
	assertEquals(data[19],"varchar1");
	assertEquals(*(datalength[19]),9);
	assertEquals(*(nullindicator[19]),0);
	assertEquals(data[20],"62696e61727931000000000000000000000000000000000000000000000000000000000000000000");
	assertEquals(*(datalength[20]),81);
	assertEquals(*(nullindicator[20]),0);
	assertEquals(data[21],"76617262696e61727931");
	assertEquals(*(datalength[21]),21);
	assertEquals(*(nullindicator[21]),0);
	assertEquals(data[22],"nvarchar1");
	assertEquals(*(datalength[22]),10);
	assertEquals(*(nullindicator[22]),0);
	assertEquals(data[23],"nchar1                                  ");
	assertEquals(*(datalength[23]),41);
	assertEquals(*(nullindicator[23]),0);
	if (present[24]) {
		assertEquals(data[24],"xml1");
		assertEquals(*(datalength[24]),5);
		assertEquals(*(nullindicator[24]),0);
	}
	assertEquals(data[25],"text1");
	assertEquals(*(datalength[25]),6);
	assertEquals(*(nullindicator[25]),0);
	assertEquals(data[26],"696d61676531");
	assertEquals(*(datalength[26]),13);
	assertEquals(*(nullindicator[26]),0);
	if (present[27]) {
		assertEquals(data[27],"ntext1");
		assertEquals(*(datalength[27]),7);
		assertEquals(*(nullindicator[27]),0);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");

	stdoutput.printf("row data:\n");
	assertEquals(data[0],"2");
	assertEquals(data[1],"1");
	assertEquals(data[2],"2");
	assertEquals(data[3],"2");
	assertEquals(data[4],"Feb  2 2002 02:02:00:000PM");
	//assertEquals(data[5],"2.5");
	//assertEquals(data[6],"2.50");
	assertEquals(data[7],"Feb  2 2002 02:02:02:000PM");
	//assertEquals(data[8],"2.5");
	//assertEquals(data[9],"2.50");
	assertEquals(data[10],"2");
	if (present[11]) {
		assertEquals(data[11],
				"01020304-0102-0304-0102-030401020304");
	}
	assertEquals(data[12],"2.50");
	assertEquals(data[13],"2.50");
	//assertEquals(data[14],"2002-02-02");		#4780
	//assertEquals(data[15],"14:02:02.0000000");	#4780
	//assertEquals(data[16],"2002-02-02 14:02:02.0000000");		#4780
	//assertEquals(data[17],"2002-02-02 14:02:02.0000000 +00:00");	#4780
	assertEquals(data[18],"char2                                   ");
	assertEquals(data[19],"varchar2");
	assertEquals(data[20],"62696e61727932000000000000000000000000000000000000000000000000000000000000000000");
	assertEquals(data[21],"76617262696e61727932");
	assertEquals(data[22],"nvarchar2");
	assertEquals(data[23],"nchar2                                  ");
	if (present[24]) {
		assertEquals(data[24],"xml2");
	}
	assertEquals(data[25],"text2");
	assertEquals(data[26],"696d61676532");
	if (present[27]) {
		assertEquals(data[27],"ntext2");
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: drop\n");
	query="drop table testtable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype),
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("\n========= Charset and Collation ========\n\n");


	CS_CHAR	buf[1024];
	CS_INT	outlen;
	CS_INT	intval;


	stdoutput.printf("cs_locale: get charset\n");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,locale,CS_SYB_CHARSET,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(buf,"utf-8");
	stdoutput.printf("\n");


	stdoutput.printf("cs_locale: get language\n");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,locale,CS_SYB_LANG,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(buf,"us_english");
	stdoutput.printf("\n");


	stdoutput.printf("cs_locale: get language and charset\n");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,locale,CS_SYB_LANG_CHARSET,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(buf,"us_english.utf-8");
	stdoutput.printf("\n");


	stdoutput.printf("cs_locale: get sort order\n");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,locale,CS_SYB_SORTORDER,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(buf,"");
	stdoutput.printf("\n");


	stdoutput.printf("cs_locale: get lc_all\n");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,locale,CS_LC_ALL,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_FAIL);
	stdoutput.printf("\n");


	// The charset set through cs_locale never reaches libtds' iconv
	// layer, so the readback is empty.  Only a ct_con_props CS_SET of
	// CS_CLIENTCHARSET takes.  This connection converts as ISO-8859-1,
	// which passes bytes through unchanged.
	stdoutput.printf("ct_con_props: get client charset\n");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(ct_con_props(dbconn,CS_GET,CS_CLIENTCHARSET,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(outlen,0);
	assertEquals(buf,"");
	stdoutput.printf("\n");


	stdoutput.printf("ct_con_props: get locale\n");
	CS_LOCALE	*conlocale=NULL;
	assertEquals(cs_loc_alloc(context,&conlocale),CS_SUCCEED);
	assertEquals(ct_con_props(dbconn,CS_GET,CS_LOC_PROP,
				(CS_VOID *)conlocale,CS_UNUSED,
				(CS_INT *)NULL),CS_SUCCEED);
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,conlocale,CS_SYB_CHARSET,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(buf,"utf-8");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,conlocale,CS_SYB_LANG,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(buf,"us_english");
	assertEquals(cs_loc_drop(context,conlocale),CS_SUCCEED);
	stdoutput.printf("\n");


	// the tds version decides whether the n-types travel as ucs-2 or
	// as server-charset bytes
	stdoutput.printf("ct_con_props: get tds version\n");
	intval=-1;
	assertEquals(ct_con_props(dbconn,CS_GET,CS_TDS_VERSION,
				(CS_VOID *)&intval,CS_UNUSED,
				(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(intval,(issybase)?CS_TDS_50:CS_TDS_74);
	stdoutput.printf("\n");


	stdoutput.printf("cs_locale: set bogus charset\n");
	CS_LOCALE	*throwawaylocale=NULL;
	assertEquals(cs_loc_alloc(context,&throwawaylocale),CS_SUCCEED);
	assertEquals(cs_locale(context,CS_SET,throwawaylocale,CS_SYB_CHARSET,
				(CS_VOID *)"nosuchcharset",CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,throwawaylocale,CS_SYB_CHARSET,
				(CS_VOID *)buf,(CS_INT)sizeof(buf),
				&outlen),CS_SUCCEED);
	assertEquals(buf,"nosuchcharset");
	stdoutput.printf("\n");


	stdoutput.printf("cs_locale: set sort order\n");
	assertEquals(cs_locale(context,CS_SET,throwawaylocale,CS_SYB_SORTORDER,
				(CS_VOID *)"nocase",CS_NULLTERM,
				(CS_INT *)NULL),CS_FAIL);
	stdoutput.printf("\n");


	// outlen is the size the value needs, not the size copied
	stdoutput.printf("cs_locale: get charset, short buffer\n");
	bytestring::zero(buf,sizeof(buf));
	outlen=-1;
	assertEquals(cs_locale(context,CS_GET,throwawaylocale,CS_SYB_CHARSET,
				(CS_VOID *)buf,(CS_INT)3,
				&outlen),CS_FAIL);
	assertEquals(outlen,14);
	assertEquals(cs_loc_drop(context,throwawaylocale),CS_SUCCEED);
	stdoutput.printf("\n");


	// buffers for the charset and collation queries
	CS_DATAFMT	csfmt[4];
	char		*csdata[4];
	CS_INT		csdatalength[4];
	CS_SMALLINT	csnullindicator[4];
	CS_INT		cscols;
	for (CS_INT i=0; i<4; i++) {
		csdata[i]=new char[1024];
	}


	stdoutput.printf("ct_command: select server charset\n");
	if (issybase) {
		// ase stores its charset as an id in sysconfigures, so the
		// name has to come from a join to syscharsets
		query="select c.name "
			"from master..syscharsets c, "
				"master..sysconfigures f "
			"where f.name='default character set id' "
			"and c.id=f.value";
	} else {
		query="select "
			"convert(varchar(30),"
				"serverproperty('SqlCharSetName')), "
			"convert(varchar(30),"
				"serverproperty('SqlSortOrderName')), "
			"convert(varchar(60),"
				"serverproperty('Collation')), "
			"convert(varchar(60),"
				"databasepropertyex(db_name(),'Collation'))";
	}
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	stdoutput.printf("\n");


	stdoutput.printf("ct_res_info: col count\n");
	cscols=(issybase)?1:4;
	ncols=0;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,cscols);
	stdoutput.printf("\n");


	stdoutput.printf("ct_bind:\n");
	for (CS_INT i=0; i<cscols; i++) {
		bytestring::zero(csdata[i],1024);
		csfmt[i].datatype=CS_CHAR_TYPE;
		csfmt[i].format=CS_FMT_NULLTERM;
		csfmt[i].maxlength=1024;
		csfmt[i].scale=CS_UNUSED;
		csfmt[i].precision=CS_UNUSED;
		csfmt[i].status=CS_UNUSED;
		csfmt[i].count=1;
		csfmt[i].usertype=CS_UNUSED;
		csfmt[i].locale=NULL;
		assertEquals(ct_bind(cmd,i+1,&(csfmt[i]),
						(CS_VOID *)csdata[i],
						&(csdatalength[i]),
						&(csnullindicator[i])),
						CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_describe:\n");
	for (CS_INT i=0; i<cscols; i++) {
		assertEquals(ct_describe(cmd,i+1,&(csfmt[i])),CS_SUCCEED);
		assertEquals(csfmt[i].datatype,CS_CHAR_TYPE);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	// both servers name their charset iso_1
	stdoutput.printf("row data:\n");
	assertEquals(csdata[0],"iso_1");
	if (!issybase) {
		assertEquals(csdata[1],"nocase_iso");
		assertEquals(csdata[2],"SQL_Latin1_General_CP1_CI_AS");
		assertEquals(csdata[3],"SQL_Latin1_General_CP1_CI_AS");
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	if (issybase) {

		stdoutput.printf("ct_command: select charset globals\n");
		query="select @@char_convert, @@ncharsize, @@maxcharlen";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_ROW_RESULT);
		stdoutput.printf("\n");


		stdoutput.printf("ct_res_info: col count\n");
		cscols=3;
		ncols=0;
		assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
		assertEquals(ncols,cscols);
		stdoutput.printf("\n");


		stdoutput.printf("ct_bind:\n");
		for (CS_INT i=0; i<cscols; i++) {
			bytestring::zero(csdata[i],1024);
			csfmt[i].datatype=CS_CHAR_TYPE;
			csfmt[i].format=CS_FMT_NULLTERM;
			csfmt[i].maxlength=1024;
			csfmt[i].scale=CS_UNUSED;
			csfmt[i].precision=CS_UNUSED;
			csfmt[i].status=CS_UNUSED;
			csfmt[i].count=1;
			csfmt[i].usertype=CS_UNUSED;
			csfmt[i].locale=NULL;
			assertEquals(ct_bind(cmd,i+1,&(csfmt[i]),
						(CS_VOID *)csdata[i],
						&(csdatalength[i]),
						&(csnullindicator[i])),
						CS_SUCCEED);
		}
		stdoutput.printf("\n");


		stdoutput.printf("ct_describe:\n");
		for (CS_INT i=0; i<cscols; i++) {
			assertEquals(ct_describe(cmd,i+1,&(csfmt[i])),
								CS_SUCCEED);
		}
		stdoutput.printf("\n");


		stdoutput.printf("ct_fetch:\n");
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);
		stdoutput.printf("\n");


		stdoutput.printf("row data:\n");
		assertEquals(csdata[0],"0");
		assertEquals(csdata[1],"1");
		assertEquals(csdata[2],"1");
		stdoutput.printf("\n");


		stdoutput.printf("ct_results:\n");
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");
	}


	query="drop table charsettable";
	ct_command(cmd,CS_LANG_CMD,query,charstring::getLength(query),CS_UNUSED);
	ct_send(cmd);
	while (ct_results(cmd,&resultstype)==CS_SUCCEED) {}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);


	stdoutput.printf("ct_command: create\n");
	if (issybase) {
		query="create table charsettable ("
				"testid int, "
				"testchar char(20), "
				"testvarchar varchar(20), "
				"testnchar nchar(20), "
				"testnvarchar nvarchar(20), "
				"testtext text"
				") lock datarows";
	} else {
		query="create table charsettable ("
				"testid int, "
				"testchar char(20), "
				"testvarchar varchar(20), "
				"testnchar nchar(20), "
				"testnvarchar nvarchar(20), "
				"testtext text"
				")";
	}
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// U+00E9, U+20AC and U+65E5, one per row, in all five text columns,
	// hex escaped to keep this file ascii
	const char	*charsetinserts[3]={
		"insert into charsettable values (1, "
			"'a\xc3\xa9z', 'a\xc3\xa9z', "
			"N'a\xc3\xa9z', N'a\xc3\xa9z', 'a\xc3\xa9z')",
		"insert into charsettable values (2, "
			"'a\xe2\x82\xacz', 'a\xe2\x82\xacz', "
			"N'a\xe2\x82\xacz', N'a\xe2\x82\xacz', "
			"'a\xe2\x82\xacz')",
		"insert into charsettable values (3, "
			"'a\xe6\x97\xa5z', 'a\xe6\x97\xa5z', "
			"N'a\xe6\x97\xa5z', N'a\xe6\x97\xa5z', "
			"'a\xe6\x97\xa5z')"
	};


	stdoutput.printf("ct_command: insert\n");
	for (CS_INT i=0; i<3; i++) {
		query=charsetinserts[i];
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		assertEquals(ct_res_info(cmd,CS_ROW_COUNT,
					(CS_VOID *)&affectedrows,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
		assertEquals(affectedrows,1);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: select\n");
	query="select testid, testchar, testvarchar, "
		"testnchar, testnvarchar, testtext "
		"from charsettable order by testid";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	stdoutput.printf("\n");


	stdoutput.printf("ct_res_info: col count\n");
	ncols=0;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,6);
	stdoutput.printf("\n");


	// buffers for the non-ascii round trip
	CS_DATAFMT	nafmt[6];
	char		*nadata[6];
	CS_INT		nadatalength[6];
	CS_SMALLINT	nanullindicator[6];


	stdoutput.printf("ct_bind:\n");
	for (CS_INT i=0; i<6; i++) {
		nadata[i]=new char[1024];
		bytestring::zero(nadata[i],1024);
		nafmt[i].datatype=CS_CHAR_TYPE;
		nafmt[i].format=CS_FMT_NULLTERM;
		nafmt[i].maxlength=1024;
		nafmt[i].scale=CS_UNUSED;
		nafmt[i].precision=CS_UNUSED;
		nafmt[i].status=CS_UNUSED;
		nafmt[i].count=1;
		nafmt[i].usertype=CS_UNUSED;
		nafmt[i].locale=NULL;
		assertEquals(ct_bind(cmd,i+1,&(nafmt[i]),
						(CS_VOID *)nadata[i],
						&(nadatalength[i]),
						&(nanullindicator[i])),
						CS_SUCCEED);
	}
	stdoutput.printf("\n");


	// Freetds' CS_DATAFMT has no collation field, so locale is the only
	// place per-column collation could surface.  Poisoning it first
	// keeps the NULL assert below from passing vacuously.
	stdoutput.printf("ct_describe:\n");
	CS_LOCALE	*poisonlocale=NULL;
	assertEquals(cs_loc_alloc(context,&poisonlocale),CS_SUCCEED);
	for (CS_INT i=0; i<6; i++) {
		nafmt[i].locale=poisonlocale;
		assertEquals(ct_describe(cmd,i+1,&(nafmt[i])),CS_SUCCEED);
		assertTrue(nafmt[i].locale==NULL);
	}
	assertEquals(cs_loc_drop(context,poisonlocale),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"1");
	assertEquals(nadatalength[0],2);
	assertEquals(nadata[1],"a\xc3\xa9z                ");
	assertEquals(nadatalength[1],21);
	assertEquals(nadata[2],"a\xc3\xa9z");
	assertEquals(nadatalength[2],5);
	assertEquals(nadata[3],"a\xc3\xa9z                ");
	assertEquals(nadatalength[3],21);
	assertEquals(nadata[4],"a\xc3\xa9z");
	assertEquals(nadatalength[4],5);
	assertEquals(nadata[5],"a\xc3\xa9z");
	assertEquals(nadatalength[5],5);
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	// On tds 7 the query text goes out as ucs-2, so each raw byte widens
	// to its own code point.  Narrowing back to cp1252 for the
	// non-unicode columns loses U+0082 and U+0097 to '?'.  ASE converts
	// nothing either way.
	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"2");
	assertEquals(nadatalength[0],2);
	assertEquals(nadata[1],(issybase)?
				"a\xe2\x82\xacz               ":
				"a\xe2?\xacz               ");
	assertEquals(nadatalength[1],21);
	assertEquals(nadata[2],(issybase)?"a\xe2\x82\xacz":"a\xe2?\xacz");
	assertEquals(nadatalength[2],6);
	assertEquals(nadata[3],"a\xe2\x82\xacz               ");
	assertEquals(nadatalength[3],21);
	assertEquals(nadata[4],"a\xe2\x82\xacz");
	assertEquals(nadatalength[4],6);
	assertEquals(nadata[5],(issybase)?"a\xe2\x82\xacz":"a\xe2?\xacz");
	assertEquals(nadatalength[5],6);
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"3");
	assertEquals(nadatalength[0],2);
	assertEquals(nadata[1],(issybase)?
				"a\xe6\x97\xa5z               ":
				"a\xe6?\xa5z               ");
	assertEquals(nadatalength[1],21);
	assertEquals(nadata[2],(issybase)?"a\xe6\x97\xa5z":"a\xe6?\xa5z");
	assertEquals(nadatalength[2],6);
	assertEquals(nadata[3],"a\xe6\x97\xa5z               ");
	assertEquals(nadatalength[3],21);
	assertEquals(nadata[4],"a\xe6\x97\xa5z");
	assertEquals(nadatalength[4],6);
	assertEquals(nadata[5],(issybase)?"a\xe6\x97\xa5z":"a\xe6?\xa5z");
	assertEquals(nadatalength[5],6);
	stdoutput.printf("\n");


	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// Converting a text column straight to varbinary errors on mssql,
	// and on ASE it aborts the whole command with Msg 3621, hence the
	// doubled convert on testtext.
	stdoutput.printf("ct_command: select stored bytes\n");
	query="select testid, "
		"convert(varbinary(40),testchar), "
		"convert(varbinary(40),testvarchar), "
		"convert(varbinary(40),testnchar), "
		"convert(varbinary(40),testnvarchar), "
		"convert(varbinary(40),convert(varchar(40),testtext)) "
		"from charsettable order by testid";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	stdoutput.printf("\n");


	stdoutput.printf("ct_res_info: col count\n");
	ncols=0;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,6);
	stdoutput.printf("\n");


	stdoutput.printf("ct_bind:\n");
	for (CS_INT i=0; i<6; i++) {
		bytestring::zero(nadata[i],1024);
		nafmt[i].datatype=CS_CHAR_TYPE;
		nafmt[i].format=CS_FMT_NULLTERM;
		nafmt[i].maxlength=1024;
		nafmt[i].scale=CS_UNUSED;
		nafmt[i].precision=CS_UNUSED;
		nafmt[i].status=CS_UNUSED;
		nafmt[i].count=1;
		nafmt[i].usertype=CS_UNUSED;
		nafmt[i].locale=NULL;
		assertEquals(ct_bind(cmd,i+1,&(nafmt[i]),
						(CS_VOID *)nadata[i],
						&(nadatalength[i]),
						&(nanullindicator[i])),
						CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"1");
	assertEquals(nadata[1],"61c3a97a"
				"20202020202020202020202020202020");
	assertEquals(nadata[2],"61c3a97a");
	assertEquals(nadata[3],(issybase)?
				"61c3a97a"
				"20202020202020202020202020202020":
				"6100c300a9007a00"
				"20002000200020002000200020002000"
				"20002000200020002000200020002000");
	assertEquals(nadata[4],(issybase)?"61c3a97a":"6100c300a9007a00");
	assertEquals(nadata[5],"61c3a97a");
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	// the 3f in the mssql non-unicode columns is a stored '?', so the
	// loss happened on the way in, not on the way out
	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"2");
	assertEquals(nadata[1],(issybase)?
				"61e282ac7a"
				"202020202020202020202020202020":
				"61e23fac7a"
				"202020202020202020202020202020");
	assertEquals(nadata[2],(issybase)?"61e282ac7a":"61e23fac7a");
	assertEquals(nadata[3],(issybase)?
				"61e282ac7a"
				"202020202020202020202020202020":
				"6100e2008200ac007a00"
				"20002000200020002000200020002000"
				"2000200020002000200020002000");
	assertEquals(nadata[4],(issybase)?
				"61e282ac7a":
				"6100e2008200ac007a00");
	assertEquals(nadata[5],(issybase)?"61e282ac7a":"61e23fac7a");
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"3");
	assertEquals(nadata[1],(issybase)?
				"61e697a57a"
				"202020202020202020202020202020":
				"61e63fa57a"
				"202020202020202020202020202020");
	assertEquals(nadata[2],(issybase)?"61e697a57a":"61e63fa57a");
	assertEquals(nadata[3],(issybase)?
				"61e697a57a"
				"202020202020202020202020202020":
				"6100e6009700a5007a00"
				"20002000200020002000200020002000"
				"2000200020002000200020002000");
	assertEquals(nadata[4],(issybase)?
				"61e697a57a":
				"6100e6009700a5007a00");
	assertEquals(nadata[5],(issybase)?"61e697a57a":"61e63fa57a");
	stdoutput.printf("\n");


	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: select datalength\n");
	query="select testid, datalength(testchar), datalength(testvarchar), "
		"datalength(testnchar), datalength(testnvarchar), "
		"datalength(testtext) "
		"from charsettable order by testid";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	stdoutput.printf("\n");


	stdoutput.printf("ct_res_info: col count\n");
	ncols=0;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,6);
	stdoutput.printf("\n");


	stdoutput.printf("ct_bind:\n");
	for (CS_INT i=0; i<6; i++) {
		bytestring::zero(nadata[i],1024);
		nafmt[i].datatype=CS_CHAR_TYPE;
		nafmt[i].format=CS_FMT_NULLTERM;
		nafmt[i].maxlength=1024;
		nafmt[i].scale=CS_UNUSED;
		nafmt[i].precision=CS_UNUSED;
		nafmt[i].status=CS_UNUSED;
		nafmt[i].count=1;
		nafmt[i].usertype=CS_UNUSED;
		nafmt[i].locale=NULL;
		assertEquals(ct_bind(cmd,i+1,&(nafmt[i]),
						(CS_VOID *)nadata[i],
						&(nadatalength[i]),
						&(nanullindicator[i])),
						CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	// mssql spends two bytes per character on the n-types, ase one
	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"1");
	assertEquals(nadata[1],"20");
	assertEquals(nadata[2],"4");
	assertEquals(nadata[3],(issybase)?"20":"40");
	assertEquals(nadata[4],(issybase)?"4":"8");
	assertEquals(nadata[5],"4");
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"2");
	assertEquals(nadata[1],"20");
	assertEquals(nadata[2],"5");
	assertEquals(nadata[3],(issybase)?"20":"40");
	assertEquals(nadata[4],(issybase)?"5":"10");
	assertEquals(nadata[5],"5");
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	stdoutput.printf("row data:\n");
	assertEquals(nadata[0],"3");
	assertEquals(nadata[1],"20");
	assertEquals(nadata[2],"5");
	assertEquals(nadata[3],(issybase)?"20":"40");
	assertEquals(nadata[4],(issybase)?"5":"10");
	assertEquals(nadata[5],"5");
	stdoutput.printf("\n");


	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: drop\n");
	query="drop table charsettable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// same ddl for both servers
	const char	*collatecreate=
			"create table collatetable ("
				"testcollated varchar(20) "
					"collate Latin1_General_BIN, "
				"testdefault varchar(20))";

	if (issybase) {

		// ASE 16 has no per-column collate clause - its collation is a
		// server-wide sort order - so this create is supposed to fail.
		stdoutput.printf("ct_command: create collate\n");
		query=collatecreate;
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_FAIL);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");

	} else {

		query="drop table collatetable";
		ct_command(cmd,CS_LANG_CMD,query,
					charstring::getLength(query),
					CS_UNUSED);
		ct_send(cmd);
		while (ct_results(cmd,&resultstype)==CS_SUCCEED) {}
		ct_cancel(NULL,cmd,CS_CANCEL_ALL);


		stdoutput.printf("ct_command: create collate\n");
		query=collatecreate;
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: select collation\n");
		query="select c.name, c.collation_name from sys.columns c "
			"where c.object_id=object_id('collatetable') "
			"order by c.column_id";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_ROW_RESULT);
		stdoutput.printf("\n");


		stdoutput.printf("ct_res_info: col count\n");
		cscols=2;
		ncols=0;
		assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
		assertEquals(ncols,cscols);
		stdoutput.printf("\n");


		stdoutput.printf("ct_bind:\n");
		for (CS_INT i=0; i<cscols; i++) {
			bytestring::zero(csdata[i],1024);
			csfmt[i].datatype=CS_CHAR_TYPE;
			csfmt[i].format=CS_FMT_NULLTERM;
			csfmt[i].maxlength=1024;
			csfmt[i].scale=CS_UNUSED;
			csfmt[i].precision=CS_UNUSED;
			csfmt[i].status=CS_UNUSED;
			csfmt[i].count=1;
			csfmt[i].usertype=CS_UNUSED;
			csfmt[i].locale=NULL;
			assertEquals(ct_bind(cmd,i+1,&(csfmt[i]),
							(CS_VOID *)csdata[i],
							&(csdatalength[i]),
							&(csnullindicator[i])),
							CS_SUCCEED);
		}
		stdoutput.printf("\n");


		stdoutput.printf("ct_fetch:\n");
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);
		stdoutput.printf("\n");


		stdoutput.printf("row data:\n");
		assertEquals(csdata[0],"testcollated");
		assertEquals(csdata[1],"Latin1_General_BIN");
		stdoutput.printf("\n");


		stdoutput.printf("ct_fetch:\n");
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);
		stdoutput.printf("\n");


		stdoutput.printf("row data:\n");
		assertEquals(csdata[0],"testdefault");
		assertEquals(csdata[1],"SQL_Latin1_General_CP1_CI_AS");
		stdoutput.printf("\n");


		stdoutput.printf("ct_results:\n");
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: select collated\n");
		query="select testcollated from collatetable";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_ROW_RESULT);
		stdoutput.printf("\n");


		// an explicitly collated column reports no collation either -
		// see the poisoned describe above
		stdoutput.printf("ct_describe:\n");
		poisonlocale=NULL;
		assertEquals(cs_loc_alloc(context,&poisonlocale),CS_SUCCEED);
		csfmt[0].locale=poisonlocale;
		assertEquals(ct_describe(cmd,1,&(csfmt[0])),CS_SUCCEED);
		assertTrue(csfmt[0].locale==NULL);
		assertEquals(cs_loc_drop(context,poisonlocale),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_results:\n");
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: drop\n");
		query="drop table collatetable";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");
	}


	stdoutput.printf("cs_con_alloc: second connection\n");
	CS_CONNECTION	*conn2=NULL;
	assertEquals(ct_con_alloc(context,&conn2),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("cs_con_props: second connection\n");
	assertEquals(ct_con_props(conn2,CS_SET,
				CS_USERNAME,(CS_VOID *)user,CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ct_con_props(conn2,CS_SET,
				CS_PASSWORD,(CS_VOID *)password,CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ct_con_props(conn2,CS_SET,
				CS_APPNAME,(CS_VOID *)"SQL Relay Test",
				CS_NULLTERM,(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ct_con_props(conn2,CS_SET,
				CS_PACKETSIZE,(CS_VOID *)&ps,sizeof(ps),
				(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ct_con_props(conn2,CS_SET,
				CS_CLIENTCHARSET,(CS_VOID *)"UTF-8",CS_NULLTERM,
				(CS_INT *)NULL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_connect: second connection\n");
	CS_RETCODE	connected2=ct_connect(conn2,(CS_CHAR *)NULL,(CS_INT)0);
	assertEquals(connected2,CS_SUCCEED);
	stdoutput.printf("\n");


	if (connected2==CS_SUCCEED) {

		// set through ct_con_props, so this one reads back, unlike the
		// primary connection above
		stdoutput.printf("ct_con_props: get client charset\n");
		bytestring::zero(buf,sizeof(buf));
		outlen=-1;
		assertEquals(ct_con_props(conn2,CS_GET,CS_CLIENTCHARSET,
					(CS_VOID *)buf,(CS_INT)sizeof(buf),
					&outlen),CS_SUCCEED);
		assertEquals(outlen,5);
		assertEquals(buf,"UTF-8");
		stdoutput.printf("\n");


		stdoutput.printf("ct_cmd_alloc: cmd2\n");
		CS_COMMAND	*cmd2=NULL;
		assertEquals(ct_cmd_alloc(conn2,&cmd2),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: use db\n");
		stringbuffer	q2;
		q2.append("use ")->append(db);
		query=q2.getString();
		assertEquals(ct_command(cmd2,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd2),CS_SUCCEED);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd2,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		query="drop table charsettable2";
		ct_command(cmd2,CS_LANG_CMD,query,
					charstring::getLength(query),
					CS_UNUSED);
		ct_send(cmd2);
		while (ct_results(cmd2,&resultstype)==CS_SUCCEED) {}
		ct_cancel(NULL,cmd2,CS_CANCEL_ALL);


		stdoutput.printf("ct_command: create\n");
		if (issybase) {
			query="create table charsettable2 ("
					"testid int, "
					"testvarchar varchar(20), "
					"testnvarchar nvarchar(20)"
					") lock datarows";
		} else {
			query="create table charsettable2 ("
					"testid int, "
					"testvarchar varchar(20), "
					"testnvarchar nvarchar(20)"
					")";
		}
		assertEquals(ct_command(cmd2,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd2),CS_SUCCEED);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd2,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		// the same three characters as above, hex escaped
		const char	*charsetinserts2[3]={
			"insert into charsettable2 values (1, "
				"'a\xc3\xa9z', N'a\xc3\xa9z')",
			"insert into charsettable2 values (2, "
				"'a\xe2\x82\xacz', N'a\xe2\x82\xacz')",
			"insert into charsettable2 values (3, "
				"'a\xe6\x97\xa5z', N'a\xe6\x97\xa5z')"
		};


		// ASE's server charset is iso_1, so freetds converts the utf-8
		// client charset down to it and drops the two characters with
		// no iso_1 form.  That leaves the quotes unbalanced, so the
		// mangled statement still goes out and ASE rejects it - the
		// conversion fails client-side, the command fails server-side.
		// Nothing fails on mssql, which takes ucs-2 on tds 7.
		stdoutput.printf("ct_command: insert\n");
		for (CS_INT i=0; i<3; i++) {
			query=charsetinserts2[i];
			assertEquals(ct_command(cmd2,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
			assertEquals(ct_send(cmd2),CS_SUCCEED);
			results=ct_results(cmd2,&resultstype);
			assertEquals(results,CS_SUCCEED);
			if (issybase && i>0) {
				assertEquals(resultstype,CS_CMD_FAIL);
			} else {
				assertEquals(resultstype,CS_CMD_SUCCEED);
				assertEquals(ct_res_info(cmd2,CS_ROW_COUNT,
						(CS_VOID *)&affectedrows,
						CS_UNUSED,
						(CS_INT *)NULL),CS_SUCCEED);
				assertEquals(affectedrows,1);
			}
			results=ct_results(cmd2,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_CMD_DONE);
			results=ct_results(cmd2,&resultstype);
			assertEquals(results,CS_END_RESULTS);
			assertEquals(ct_cancel(NULL,cmd2,CS_CANCEL_ALL),
								CS_SUCCEED);
		}
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: select\n");
		query="select testid, testvarchar, testnvarchar "
			"from charsettable2 order by testid";
		assertEquals(ct_command(cmd2,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd2),CS_SUCCEED);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_ROW_RESULT);
		stdoutput.printf("\n");


		stdoutput.printf("ct_res_info: col count\n");
		cscols=3;
		ncols=0;
		assertEquals(ct_res_info(cmd2,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
		assertEquals(ncols,cscols);
		stdoutput.printf("\n");


		stdoutput.printf("ct_bind:\n");
		for (CS_INT i=0; i<cscols; i++) {
			bytestring::zero(csdata[i],1024);
			csfmt[i].datatype=CS_CHAR_TYPE;
			csfmt[i].format=CS_FMT_NULLTERM;
			csfmt[i].maxlength=1024;
			csfmt[i].scale=CS_UNUSED;
			csfmt[i].precision=CS_UNUSED;
			csfmt[i].status=CS_UNUSED;
			csfmt[i].count=1;
			csfmt[i].usertype=CS_UNUSED;
			csfmt[i].locale=NULL;
			assertEquals(ct_bind(cmd2,i+1,&(csfmt[i]),
							(CS_VOID *)csdata[i],
							&(csdatalength[i]),
							&(csnullindicator[i])),
							CS_SUCCEED);
		}
		stdoutput.printf("\n");


		stdoutput.printf("ct_fetch:\n");
		assertEquals(ct_fetch(cmd2,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);
		stdoutput.printf("\n");


		// both columns come back re-encoded as utf-8, so the bytes
		// match what went in
		stdoutput.printf("row data:\n");
		assertEquals(csdata[0],"1");
		assertEquals(csdatalength[0],2);
		assertEquals(csdata[1],"a\xc3\xa9z");
		assertEquals(csdatalength[1],5);
		assertEquals(csdata[2],"a\xc3\xa9z");
		assertEquals(csdatalength[2],5);
		stdoutput.printf("\n");


		if (!issybase) {

			stdoutput.printf("ct_fetch:\n");
			assertEquals(ct_fetch(cmd2,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
			assertEquals(rowsread,1);
			stdoutput.printf("\n");


			stdoutput.printf("row data:\n");
			assertEquals(csdata[0],"2");
			assertEquals(csdatalength[0],2);
			assertEquals(csdata[1],"a\xe2\x82\xacz");
			assertEquals(csdatalength[1],6);
			assertEquals(csdata[2],"a\xe2\x82\xacz");
			assertEquals(csdatalength[2],6);
			stdoutput.printf("\n");


			stdoutput.printf("ct_fetch:\n");
			assertEquals(ct_fetch(cmd2,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
			assertEquals(rowsread,1);
			stdoutput.printf("\n");


			// cp1252 has no U+65E5, so the varchar copy was
			// already a '?' when it was stored
			stdoutput.printf("row data:\n");
			assertEquals(csdata[0],"3");
			assertEquals(csdatalength[0],2);
			assertEquals(csdata[1],"a?z");
			assertEquals(csdatalength[1],4);
			assertEquals(csdata[2],"a\xe6\x97\xa5z");
			assertEquals(csdatalength[2],6);
			stdoutput.printf("\n");
		}


		stdoutput.printf("ct_results:\n");
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd2,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: drop\n");
		query="drop table charsettable2";
		assertEquals(ct_command(cmd2,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd2),CS_SUCCEED);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd2,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd2,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_cmd_drop: cmd2\n");
		assertEquals(ct_cmd_drop(cmd2),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_close: second connection\n");
		assertEquals(ct_close(conn2,CS_UNUSED),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_con_drop: second connection\n");
		assertEquals(ct_con_drop(conn2),CS_SUCCEED);
		stdoutput.printf("\n");
	}


	stdoutput.printf("\n============== Bulk Load ==============\n\n");


	query="drop table bulktable";
	ct_command(cmd,CS_LANG_CMD,query,charstring::getLength(query),CS_UNUSED);
	ct_send(cmd);
	while (ct_results(cmd,&resultstype)==CS_SUCCEED) {}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);


	CS_BLKDESC	*blk=NULL;
	CS_INT		outrow;


	if (issybase) {

		query="drop table bulktabledol";
		ct_command(cmd,CS_LANG_CMD,query,
				charstring::getLength(query),CS_UNUSED);
		ct_send(cmd);
		while (ct_results(cmd,&resultstype)==CS_SUCCEED) {}
		ct_cancel(NULL,cmd,CS_CANCEL_ALL);


		stdoutput.printf("ct_command: create datarows\n");
		query="create table bulktabledol (testint int null) "
			"lock datarows";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		// ASE refuses a bulk copy into a data-only locked table with
		// msg 4845 - freetds never advertises that capability.
		// blk_init still succeeds; the refusal surfaces at the first
		// blk_rowxfer.
		stdoutput.printf("blk_: data-only locked table\n");
		CS_BLKDESC	*dolblk=NULL;
		assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&dolblk),
								CS_SUCCEED);
		assertEquals(blk_init(dolblk,CS_BLK_IN,
					(CS_CHAR *)"bulktabledol",
					CS_NULLTERM),CS_SUCCEED);
		CS_DATAFMT	dolfmt;
		char		dolvalue[8];
		CS_INT		dollength;
		CS_SMALLINT	dolindicator=0;
		charstring::copy(dolvalue,"1");
		dollength=charstring::getLength(dolvalue);
		dolfmt.datatype=CS_CHAR_TYPE;
		dolfmt.format=CS_FMT_NULLTERM;
		dolfmt.maxlength=sizeof(dolvalue);
		dolfmt.scale=CS_UNUSED;
		dolfmt.precision=CS_UNUSED;
		dolfmt.status=CS_UNUSED;
		dolfmt.count=1;
		dolfmt.usertype=CS_UNUSED;
		dolfmt.locale=NULL;
		assertEquals(blk_bind(dolblk,1,&dolfmt,(CS_VOID *)dolvalue,
					&dollength,&dolindicator),CS_SUCCEED);
		assertEquals(blk_rowxfer(dolblk),CS_FAIL);
		outrow=-1;
		assertEquals(blk_done(dolblk,CS_BLK_ALL,&outrow),CS_FAIL);
		assertEquals(outrow,-1);
		assertEquals(blk_drop(dolblk),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: drop datarows\n");
		query="drop table bulktabledol";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");
	}


	// ASE cannot bulk copy into a data-only locked table, so this one
	// is allpages, unlike the lock datarows creates elsewhere in this
	// file.  Every column is explicitly null so that both servers
	// report CS_CANBENULL - ASE columns are not null by default.
	stdoutput.printf("ct_command: create\n");
	if (issybase) {
		query="create table bulktable ("
				"testint int null, "
				"testchar char(20) null, "
				"testvarchar varchar(20) null, "
				"testfloat float null, "
				"testdecimal decimal(5,2) null, "
				"testdatetime datetime null, "
				"testbinary binary(10) null, "
				"testtext text null"
				") lock allpages";
	} else {
		query="create table bulktable ("
				"testint int null, "
				"testchar char(20) null, "
				"testvarchar varchar(20) null, "
				"testfloat float null, "
				"testdecimal decimal(5,2) null, "
				"testdatetime datetime null, "
				"testbinary binary(10) null, "
				"testtext text null"
				")";
	}
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	if (issybase) {

		// An allpages table still needs an index, or ASE refuses the
		// non-logged bulk copy with msg 4806.  The index forces the
		// logged path, which needs no database option.  A unique
		// clustered index keeps ASE quiet; a nonclustered one adds an
		// informational msg 4852.
		stdoutput.printf("ct_command: create index\n");
		query="create unique clustered index bulktableix "
			"on bulktable (testint)";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");
	}


	stdoutput.printf("blk_alloc:\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("blk_props: get identity before init\n");
	intval=-1;
	outlen=-1;
	assertEquals(blk_props(blk,CS_GET,BLK_IDENTITY,(CS_VOID *)&intval,
				(CS_INT)sizeof(intval),&outlen),CS_SUCCEED);
	assertEquals(intval,CS_FALSE);
	assertEquals(outlen,(CS_INT)sizeof(CS_INT));
	stdoutput.printf("\n");


	// blk_init builds its column metadata with a SET FMTONLY query, an
	// mssql idiom that ASE answers too
	stdoutput.printf("blk_init:\n");
	assertEquals(blk_init(blk,CS_BLK_IN,(CS_CHAR *)"bulktable",
						CS_NULLTERM),CS_SUCCEED);
	stdoutput.printf("\n");


	const char	*blkname[8]={
		"testint","testchar","testvarchar","testfloat",
		"testdecimal","testdatetime","testbinary","testtext"
	};
	CS_INT		blktype[8]={
		CS_INT_TYPE,CS_CHAR_TYPE,CS_CHAR_TYPE,CS_FLOAT_TYPE,
		CS_DECIMAL_TYPE,CS_DATETIME_TYPE,CS_BINARY_TYPE,CS_TEXT_TYPE
	};
	CS_INT		blkmaxlength[8]={
		4,20,20,8,
		(issybase)?4:17,8,10,(issybase)?32768:2147483647
	};
	CS_INT		blkprecision[8]={0,0,0,0,5,0,0,0};
	CS_INT		blkscale[8]={0,0,0,0,2,0,0,0};
	CS_INT		blkusertype[8]={
		(issybase)?7:0,(issybase)?1:0,(issybase)?2:0,(issybase)?8:0,
		(issybase)?26:0,(issybase)?12:0,(issybase)?3:0,(issybase)?19:0
	};
	CS_DATAFMT	blkfmt[8];


	stdoutput.printf("blk_describe:\n");
	for (CS_INT i=0; i<8; i++) {
		bytestring::zero(&(blkfmt[i]),sizeof(CS_DATAFMT));
		assertEquals(blk_describe(blk,i+1,&(blkfmt[i])),CS_SUCCEED);
		assertEquals(blkfmt[i].name,blkname[i]);
		assertEquals(blkfmt[i].namelen,
				charstring::getLength(blkname[i]));
		assertEquals(blkfmt[i].datatype,blktype[i]);
		assertEquals(blkfmt[i].format,0);
		assertEquals(blkfmt[i].maxlength,blkmaxlength[i]);
		assertEquals(blkfmt[i].precision,blkprecision[i]);
		assertEquals(blkfmt[i].scale,blkscale[i]);
		assertEquals(blkfmt[i].status,CS_CANBENULL);
		assertEquals(blkfmt[i].count,1);
		assertEquals(blkfmt[i].usertype,blkusertype[i]);
		assertTrue(blkfmt[i].locale==NULL);
	}
	stdoutput.printf("\n");


	// Both blk_describe and blk_bind segfault unless blk_init
	// succeeded, so those negatives are missing.  An out of range
	// column number is safe.
	stdoutput.printf("blk_describe: colnum out of range\n");
	assertEquals(blk_describe(blk,0,&(blkfmt[0])),CS_FAIL);
	assertEquals(blk_describe(blk,9,&(blkfmt[0])),CS_FAIL);
	stdoutput.printf("\n");


	// blk_default and blk_textxfer are unimplemented stubs in freetds
	// 1.3.3, so outlen is left alone
	stdoutput.printf("blk_default: unimplemented\n");
	outlen=-1;
	assertEquals(blk_default(blk,1,(CS_VOID *)buf,
				(CS_INT)sizeof(buf),&outlen),CS_FAIL);
	assertEquals(outlen,-1);
	stdoutput.printf("\n");


	stdoutput.printf("blk_textxfer: unimplemented\n");
	outlen=-1;
	assertEquals(blk_textxfer(blk,(CS_BYTE *)"x",1,&outlen),CS_FAIL);
	assertEquals(outlen,-1);
	stdoutput.printf("\n");


	stdoutput.printf("blk_props: set and get identity\n");
	intval=CS_TRUE;
	assertEquals(blk_props(blk,CS_SET,BLK_IDENTITY,(CS_VOID *)&intval,
			(CS_INT)sizeof(intval),(CS_INT *)NULL),CS_SUCCEED);
	intval=-1;
	outlen=-1;
	assertEquals(blk_props(blk,CS_GET,BLK_IDENTITY,(CS_VOID *)&intval,
				(CS_INT)sizeof(intval),&outlen),CS_SUCCEED);
	assertEquals(intval,CS_TRUE);
	assertEquals(outlen,(CS_INT)sizeof(CS_INT));
	intval=CS_FALSE;
	assertEquals(blk_props(blk,CS_SET,BLK_IDENTITY,(CS_VOID *)&intval,
			(CS_INT)sizeof(intval),(CS_INT *)NULL),CS_SUCCEED);
	intval=-1;
	outlen=-1;
	assertEquals(blk_props(blk,CS_GET,BLK_IDENTITY,(CS_VOID *)&intval,
				(CS_INT)sizeof(intval),&outlen),CS_SUCCEED);
	assertEquals(intval,CS_FALSE);
	assertEquals(outlen,(CS_INT)sizeof(CS_INT));
	stdoutput.printf("\n");


	// BLK_IDENTITY is the only property freetds knows.  An unknown
	// property, or an unknown action, leaves the buffer and outlen
	// alone.
	stdoutput.printf("blk_props: unknown property and action\n");
	intval=-1;
	outlen=-1;
	assertEquals(blk_props(blk,CS_GET,999,(CS_VOID *)&intval,
				(CS_INT)sizeof(intval),&outlen),CS_FAIL);
	assertEquals(intval,-1);
	assertEquals(outlen,-1);
	assertEquals(blk_props(blk,CS_SET,999,(CS_VOID *)&intval,
			(CS_INT)sizeof(intval),(CS_INT *)NULL),CS_FAIL);
	intval=-1;
	outlen=-1;
	assertEquals(blk_props(blk,999,BLK_IDENTITY,(CS_VOID *)&intval,
				(CS_INT)sizeof(intval),&outlen),CS_FAIL);
	assertEquals(intval,-1);
	assertEquals(outlen,-1);
	stdoutput.printf("\n");


	char		blkvalue[8][64];
	CS_INT		blklength[8];
	CS_SMALLINT	blkindicator[8];
	CS_DATAFMT	blkbindfmt[8];


	// The datalen handed to blk_bind must be strlen exactly, not
	// strlen+1, even with CS_FMT_NULLTERM.  With the terminator
	// included the row still loads and blk_rowxfer still returns
	// CS_SUCCEED, but cs_convert silently mangles one column - a
	// different one on each server.
	stdoutput.printf("blk_bind:\n");
	charstring::copy(blkvalue[0],"1");
	charstring::copy(blkvalue[1],"charvalue");
	charstring::copy(blkvalue[2],"");
	charstring::copy(blkvalue[3],"1.25");
	charstring::copy(blkvalue[4],"123.45");
	charstring::copy(blkvalue[5],"2001-01-01 12:00:00");
	charstring::copy(blkvalue[6],"0123456789");
	charstring::copy(blkvalue[7],"texttexttext");
	for (CS_INT i=0; i<8; i++) {
		blklength[i]=charstring::getLength(blkvalue[i]);
		blkindicator[i]=0;
		blkbindfmt[i].datatype=CS_CHAR_TYPE;
		blkbindfmt[i].format=CS_FMT_NULLTERM;
		blkbindfmt[i].maxlength=(CS_INT)sizeof(blkvalue[i]);
		blkbindfmt[i].scale=CS_UNUSED;
		blkbindfmt[i].precision=CS_UNUSED;
		blkbindfmt[i].status=CS_UNUSED;
		blkbindfmt[i].count=1;
		blkbindfmt[i].usertype=CS_UNUSED;
		blkbindfmt[i].locale=NULL;
		assertEquals(blk_bind(blk,i+1,&(blkbindfmt[i]),
					(CS_VOID *)blkvalue[i],
					&(blklength[i]),
					&(blkindicator[i])),CS_SUCCEED);
	}
	// testvarchar goes in null
	blklength[2]=0;
	blkindicator[2]=-1;
	stdoutput.printf("\n");


	stdoutput.printf("blk_bind: colnum out of range\n");
	assertEquals(blk_bind(blk,0,&(blkbindfmt[0]),
				(CS_VOID *)blkvalue[0],&(blklength[0]),
				&(blkindicator[0])),CS_FAIL);
	assertEquals(blk_bind(blk,9,&(blkbindfmt[0]),
				(CS_VOID *)blkvalue[0],&(blklength[0]),
				&(blkindicator[0])),CS_FAIL);
	stdoutput.printf("\n");


	stdoutput.printf("blk_rowxfer:\n");
	assertEquals(blk_rowxfer(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("blk_done: all\n");
	outrow=-1;
	assertEquals(blk_done(blk,CS_BLK_ALL,&outrow),CS_SUCCEED);
	assertEquals(outrow,1);
	stdoutput.printf("\n");


	stdoutput.printf("blk_drop:\n");
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: select\n");
	query="select testint, testchar, testvarchar, testfloat, "
		"testdecimal, testdatetime, testbinary, testtext "
		"from bulktable order by testint";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	stdoutput.printf("\n");


	stdoutput.printf("ct_res_info: col count\n");
	ncols=0;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,8);
	stdoutput.printf("\n");


	CS_DATAFMT	blkreadfmt[8];
	char		*blkreaddata[8];
	CS_INT		blkreadlength[8];
	CS_SMALLINT	blkreadindicator[8];


	stdoutput.printf("ct_bind:\n");
	for (CS_INT i=0; i<8; i++) {
		blkreaddata[i]=new char[1024];
		bytestring::zero(blkreaddata[i],1024);
		blkreadfmt[i].datatype=CS_CHAR_TYPE;
		blkreadfmt[i].format=CS_FMT_NULLTERM;
		blkreadfmt[i].maxlength=1024;
		blkreadfmt[i].scale=CS_UNUSED;
		blkreadfmt[i].precision=CS_UNUSED;
		blkreadfmt[i].status=CS_UNUSED;
		blkreadfmt[i].count=1;
		blkreadfmt[i].usertype=CS_UNUSED;
		blkreadfmt[i].locale=NULL;
		assertEquals(ct_bind(cmd,i+1,&(blkreadfmt[i]),
					(CS_VOID *)blkreaddata[i],
					&(blkreadlength[i]),
					&(blkreadindicator[i])),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	stdoutput.printf("\n");


	// a char source bound to a binary column is parsed as hex digits,
	// not bytes, so "0123456789" lands as five bytes zero padded to ten
	stdoutput.printf("row data:\n");
	assertEquals(blkreaddata[0],"1");
	assertEquals(blkreadindicator[2],-1);
	assertEquals(blkreadlength[2],0);
	assertEquals(blkreaddata[3],"1.25");
	assertEquals(blkreaddata[4],"123.45");
	assertEquals(blkreaddata[5],"Jan  1 2001 12:00:00:000PM");
	assertEquals(blkreaddata[6],"01234567890000000000");
	assertEquals(blkreaddata[7],"texttexttext");
	stdoutput.printf("\n");


	// ASE stores a nullable char as a varchar, so only mssql blank pads
	stdoutput.printf("row data: char padding\n");
	if (issybase) {
		assertEquals(blkreaddata[1],"charvalue");
		assertEquals(blkreadlength[1],10);
	} else {
		assertEquals(blkreaddata[1],"charvalue           ");
		assertEquals(blkreadlength[1],21);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: delete\n");
	query="delete from bulktable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("blk_alloc: batch\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	assertEquals(blk_init(blk,CS_BLK_IN,(CS_CHAR *)"bulktable",
						CS_NULLTERM),CS_SUCCEED);
	for (CS_INT i=0; i<8; i++) {
		assertEquals(blk_bind(blk,i+1,&(blkbindfmt[i]),
					(CS_VOID *)blkvalue[i],
					&(blklength[i]),
					&(blkindicator[i])),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	// blk_rowxfer copies out of the bound buffers as it is called, so
	// the same buffers carry every row
	stdoutput.printf("blk_rowxfer: row 1\n");
	charstring::copy(blkvalue[0],"1");
	charstring::copy(blkvalue[1],"one");
	charstring::copy(blkvalue[2],"onevar");
	blklength[0]=charstring::getLength(blkvalue[0]);
	blklength[1]=charstring::getLength(blkvalue[1]);
	blklength[2]=charstring::getLength(blkvalue[2]);
	blkindicator[2]=0;
	assertEquals(blk_rowxfer(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	// outrow counts the batch, not the whole bulk copy
	stdoutput.printf("blk_done: batch\n");
	outrow=-1;
	assertEquals(blk_done(blk,CS_BLK_BATCH,&outrow),CS_SUCCEED);
	assertEquals(outrow,1);
	stdoutput.printf("\n");


	stdoutput.printf("blk_rowxfer: rows 2 and 3\n");
	charstring::copy(blkvalue[0],"2");
	charstring::copy(blkvalue[1],"two");
	charstring::copy(blkvalue[2],"twovar");
	blklength[0]=charstring::getLength(blkvalue[0]);
	blklength[1]=charstring::getLength(blkvalue[1]);
	blklength[2]=charstring::getLength(blkvalue[2]);
	assertEquals(blk_rowxfer(blk),CS_SUCCEED);
	charstring::copy(blkvalue[0],"3");
	charstring::copy(blkvalue[1],"three");
	charstring::copy(blkvalue[2],"threevar");
	blklength[0]=charstring::getLength(blkvalue[0]);
	blklength[1]=charstring::getLength(blkvalue[1]);
	blklength[2]=charstring::getLength(blkvalue[2]);
	assertEquals(blk_rowxfer(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("blk_done: all\n");
	outrow=-1;
	assertEquals(blk_done(blk,CS_BLK_ALL,&outrow),CS_SUCCEED);
	assertEquals(outrow,2);
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: select batch\n");
	query="select testint, testchar, testvarchar "
		"from bulktable order by testint";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	for (CS_INT i=0; i<3; i++) {
		bytestring::zero(blkreaddata[i],1024);
		assertEquals(ct_bind(cmd,i+1,&(blkreadfmt[i]),
					(CS_VOID *)blkreaddata[i],
					&(blkreadlength[i]),
					&(blkreadindicator[i])),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	const char	*blkrowint[3]={"1","2","3"};
	const char	*blkrowchar[3]={"one","two","three"};
	const char	*blkrowvarchar[3]={"onevar","twovar","threevar"};
	const char	*blkrowcharpadded[3]={
		"one                 ",
		"two                 ",
		"three               "
	};


	stdoutput.printf("ct_fetch:\n");
	for (CS_INT i=0; i<3; i++) {
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);
		assertEquals(blkreaddata[0],blkrowint[i]);
		assertEquals(blkreaddata[1],
			(issybase)?blkrowchar[i]:blkrowcharpadded[i]);
		assertEquals(blkreaddata[2],blkrowvarchar[i]);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_results:\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: delete\n");
	query="delete from bulktable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	char		blkmultvalue[8][3*64];
	CS_INT		blkmultlength[8][3];
	CS_SMALLINT	blkmultindicator[8][3];
	const char	*blkmultint[3]={"4","5","6"};
	const char	*blkmultchar[3]={"four","five","six"};
	const char	*blkmultvarchar[3]={"fourvar","fivevar","sixvar"};


	// an array bind strides by CS_DATAFMT.maxlength, so every slot here
	// is a full 64 bytes
	stdoutput.printf("blk_bind: array\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	assertEquals(blk_init(blk,CS_BLK_IN,(CS_CHAR *)"bulktable",
						CS_NULLTERM),CS_SUCCEED);
	for (CS_INT r=0; r<3; r++) {
		charstring::copy(blkmultvalue[0]+r*64,blkmultint[r]);
		charstring::copy(blkmultvalue[1]+r*64,blkmultchar[r]);
		charstring::copy(blkmultvalue[2]+r*64,blkmultvarchar[r]);
		charstring::copy(blkmultvalue[3]+r*64,"1.25");
		charstring::copy(blkmultvalue[4]+r*64,"123.45");
		charstring::copy(blkmultvalue[5]+r*64,"2001-01-01 12:00:00");
		charstring::copy(blkmultvalue[6]+r*64,"0123456789");
		charstring::copy(blkmultvalue[7]+r*64,"texttexttext");
		for (CS_INT i=0; i<8; i++) {
			blkmultlength[i][r]=charstring::getLength(
						blkmultvalue[i]+r*64);
			blkmultindicator[i][r]=0;
		}
	}
	for (CS_INT i=0; i<8; i++) {
		blkbindfmt[i].count=3;
		blkbindfmt[i].maxlength=64;
		assertEquals(blk_bind(blk,i+1,&(blkbindfmt[i]),
					(CS_VOID *)blkmultvalue[i],
					&(blkmultlength[i][0]),
					&(blkmultindicator[i][0])),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	// The row count handed to blk_rowxfer_mult is an input, not an
	// output.  A non-zero one overrides CS_DATAFMT.count, and it always
	// reads back 0, so only blk_done's outrow reports what was
	// transferred.
	stdoutput.printf("blk_rowxfer_mult:\n");
	intval=2;
	assertEquals(blk_rowxfer_mult(blk,&intval),CS_SUCCEED);
	assertEquals(intval,0);
	outrow=-1;
	assertEquals(blk_done(blk,CS_BLK_ALL,&outrow),CS_SUCCEED);
	assertEquals(outrow,2);
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: select array\n");
	query="select testint, testchar, testvarchar "
		"from bulktable order by testint";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	for (CS_INT i=0; i<3; i++) {
		bytestring::zero(blkreaddata[i],1024);
		assertEquals(ct_bind(cmd,i+1,&(blkreadfmt[i]),
					(CS_VOID *)blkreaddata[i],
					&(blkreadlength[i]),
					&(blkreadindicator[i])),CS_SUCCEED);
	}
	for (CS_INT i=0; i<2; i++) {
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);
		assertEquals(blkreaddata[0],blkmultint[i]);
		assertEquals(blkreaddata[2],blkmultvarchar[i]);
	}
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// Freetds has no working bulk out.  blk_init and blk_rowxfer both
	// claim success, but no row moves and the connection is left with
	// results pending.  Only ct_cancel on a command clears that -
	// ct_cancel on the connection just turns the refusal from CS_FAIL
	// into CS_CANCELED.
	stdoutput.printf("blk_init: bulk out\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	assertEquals(blk_init(blk,CS_BLK_OUT,(CS_CHAR *)"bulktable",
						CS_NULLTERM),CS_SUCCEED);
	assertEquals(blk_rowxfer(blk),CS_SUCCEED);
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_send: results pending after bulk out\n");
	query="select convert(varchar(20),count(*)) from bulktable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_FAIL);
	assertEquals(ct_cancel(dbconn,NULL,CS_CANCEL_ALL),CS_SUCCEED);
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_CANCELED);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("blk_init: bad direction\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	assertEquals(blk_init(blk,999,(CS_CHAR *)"bulktable",
						CS_NULLTERM),CS_FAIL);
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("blk_init: no such table\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	assertEquals(blk_init(blk,CS_BLK_IN,(CS_CHAR *)"nosuchbulktable",
						CS_NULLTERM),CS_FAIL);
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("blk_done: nothing transferred\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	assertEquals(blk_init(blk,CS_BLK_IN,(CS_CHAR *)"bulktable",
						CS_NULLTERM),CS_SUCCEED);
	outrow=-1;
	assertEquals(blk_done(blk,CS_BLK_ALL,&outrow),CS_FAIL);
	assertEquals(outrow,-1);
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	// Neither an unrecognized type nor CS_BLK_CANCEL does anything.
	// Both claim success, leave outrow alone, and leave the bulk copy
	// open.
	stdoutput.printf("blk_done: unrecognized type and cancel\n");
	assertEquals(blk_alloc(dbconn,BLK_VERSION_100,&blk),CS_SUCCEED);
	assertEquals(blk_init(blk,CS_BLK_IN,(CS_CHAR *)"bulktable",
						CS_NULLTERM),CS_SUCCEED);
	outrow=-1;
	assertEquals(blk_done(blk,999,&outrow),CS_SUCCEED);
	assertEquals(outrow,-1);
	outrow=-1;
	assertEquals(blk_done(blk,CS_BLK_CANCEL,&outrow),CS_SUCCEED);
	assertEquals(outrow,-1);
	assertEquals(blk_drop(blk),CS_SUCCEED);
	stdoutput.printf("\n");


	// unlike the bulk out above, a bulk in leaves the connection good
	// for ordinary commands
	stdoutput.printf("ct_command: after bulk\n");
	query="select convert(varchar(20),count(*)) from bulktable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	bytestring::zero(blkreaddata[0],1024);
	assertEquals(ct_bind(cmd,1,&(blkreadfmt[0]),
				(CS_VOID *)blkreaddata[0],
				&(blkreadlength[0]),
				&(blkreadindicator[0])),CS_SUCCEED);
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	assertEquals(blkreaddata[0],"2");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: drop\n");
	query="drop table bulktable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("\n============= RPC and Prepared ============\n\n");


	query="drop procedure dynproc";
	ct_command(cmd,CS_LANG_CMD,query,charstring::getLength(query),CS_UNUSED);
	ct_send(cmd);
	while (ct_results(cmd,&resultstype)==CS_SUCCEED) {}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);

	query="drop table dyntable";
	ct_command(cmd,CS_LANG_CMD,query,charstring::getLength(query),CS_UNUSED);
	ct_send(cmd);
	while (ct_results(cmd,&resultstype)==CS_SUCCEED) {}
	ct_cancel(NULL,cmd,CS_CANCEL_ALL);


	// Every column is declared null explicitly.  ASE columns are not
	// null by default, and the null-through-indicator execute below
	// would then fail server-side while still reporting
	// CS_CMD_SUCCEED.
	stdoutput.printf("ct_command: create\n");
	if (issybase) {
		query="create table dyntable ("
				"testid int null, "
				"testchar char(20) null, "
				"testvarchar varchar(20) null, "
				"testint int null"
				") lock datarows";
	} else {
		query="create table dyntable ("
				"testid int null, "
				"testchar char(20) null, "
				"testvarchar varchar(20) null, "
				"testint int null"
				")";
	}
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// same procedure text for both servers
	stdoutput.printf("ct_command: create procedure\n");
	query="create procedure dynproc "
		"@inparam int, @outparam int output as "
		"select @outparam = @inparam * 10 "
		"select testid, testchar from dyntable "
		"where testid <= @inparam order by testid "
		"return 42";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// The two backends do not share a wire path here.  On tds 7
	// freetds turns ct_dynamic into the sp_ procs - sp_prepare and
	// sp_unprepare by numeric proc id with procnamelen 0xffff, and
	// sp_execute by name.  On tds 5 it uses ASE's native dynamic sql
	// tokens and no sp_ proc is ever sent.  That is why the
	// expectations below split per backend as often as they do.
	const char	*dyninsertid="dynins";
	const char	*dyninsert="insert into dyntable values (?,?,?,?)";
	const char	*dynselectid="dynsel";
	const char	*dynselect="select testid, testchar, testvarchar, "
					"testint from dyntable "
					"where testid = ?";


	stdoutput.printf("ct_dynamic: prepare insert\n");
	assertEquals(ct_dynamic(cmd,CS_PREPARE,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)dyninsert,CS_NULLTERM),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	stdoutput.printf("\n");


	// ASE ships the input parameter formats with the prepare ack, so
	// it reports the placeholder count here.  MSSQL reports the
	// prepared statement's output column count instead, which is
	// zero for an insert.
	stdoutput.printf("ct_res_info: after prepare\n");
	ncols=-1;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,(issybase)?4:0);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	CS_DATAFMT	dyndesc[4];


	// Freetds answers both describes from cached metadata without
	// going near the wire.  On tds 5 that metadata is real, because
	// ASE sends it unsolicited with the prepare ack.  On tds 7 there
	// is nothing to cache, so describe input reports zero parameters.
	stdoutput.printf("ct_dynamic: describe input on insert\n");
	assertEquals(ct_dynamic(cmd,CS_DESCRIBE_INPUT,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_DESCRIBE_RESULT);
	ncols=-1;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,(issybase)?4:0);

	// ct_describe on a result whose column count is zero segfaults
	// inside libct, so the mssql side stops at the count above rather
	// than describing nothing.
	if (issybase) {
		CS_INT	dyninfmttype[4]={
				CS_INT_TYPE,CS_CHAR_TYPE,
				CS_CHAR_TYPE,CS_INT_TYPE};
		CS_INT	dyninfmtmaxlength[4]={4,20,20,4};
		for (CS_INT i=0; i<4; i++) {
			bytestring::zero(&(dyndesc[i]),sizeof(CS_DATAFMT));
			assertEquals(ct_describe(cmd,i+1,&(dyndesc[i])),
								CS_SUCCEED);
			assertEquals(dyndesc[i].name,"");
			assertEquals(dyndesc[i].namelen,0);
			assertEquals(dyndesc[i].datatype,dyninfmttype[i]);
			assertEquals(dyndesc[i].format,0);
			assertEquals(dyndesc[i].maxlength,
						dyninfmtmaxlength[i]);
			assertEquals(dyndesc[i].precision,0);
			assertEquals(dyndesc[i].scale,0);
			assertEquals(dyndesc[i].status,0);
			assertEquals(dyndesc[i].count,1);
			assertEquals(dyndesc[i].usertype,0);
			assertTrue(dyndesc[i].locale==NULL);
		}
	}

	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	CS_DATAFMT	dynparam[4];
	CS_INT		dynintvalue[4];
	char		dyncharvalue[2][32];


	// The datalen handed to ct_param is strlen exactly.  With
	// strlen+1 both servers store the terminator as data.  Only
	// datatype, maxlength and count have to be set - status
	// CS_INPUTVALUE is not required, and the name is discarded on
	// this path.
	stdoutput.printf("ct_dynamic: execute insert\n");
	const char	*dyncharinput[4]={"one","two","three","four"};
	const char	*dynvarcharinput[4]={"uno","dos","tres","cuatro"};
	for (CS_INT i=0; i<4; i++) {

		assertEquals(ct_dynamic(cmd,CS_EXECUTE,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);

		bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
		dynparam[0].datatype=CS_INT_TYPE;
		dynparam[0].maxlength=4;
		dynparam[0].count=1;
		dynintvalue[0]=i+1;
		assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[0]),
					sizeof(CS_INT),0),CS_SUCCEED);

		bytestring::zero(&(dynparam[1]),sizeof(CS_DATAFMT));
		dynparam[1].datatype=CS_CHAR_TYPE;
		dynparam[1].maxlength=20;
		dynparam[1].count=1;
		charstring::copy(dyncharvalue[0],dyncharinput[i]);

		bytestring::zero(&(dynparam[2]),sizeof(CS_DATAFMT));
		dynparam[2].datatype=CS_CHAR_TYPE;
		dynparam[2].maxlength=20;
		dynparam[2].count=1;
		charstring::copy(dyncharvalue[1],dynvarcharinput[i]);

		// the last row sends both char columns null
		CS_SMALLINT	dynind=(i==3)?-1:0;
		assertEquals(ct_param(cmd,&(dynparam[1]),
				(CS_VOID *)dyncharvalue[0],
				charstring::getLength(dyncharvalue[0]),
				dynind),CS_SUCCEED);
		assertEquals(ct_param(cmd,&(dynparam[2]),
				(CS_VOID *)dyncharvalue[1],
				charstring::getLength(dyncharvalue[1]),
				dynind),CS_SUCCEED);

		bytestring::zero(&(dynparam[3]),sizeof(CS_DATAFMT));
		dynparam[3].datatype=CS_INT_TYPE;
		dynparam[3].maxlength=4;
		dynparam[3].count=1;
		dynintvalue[3]=101+i;
		assertEquals(ct_param(cmd,&(dynparam[3]),
					(CS_VOID *)&(dynintvalue[3]),
					sizeof(CS_INT),0),CS_SUCCEED);

		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		assertEquals(ct_res_info(cmd,CS_ROW_COUNT,
					(CS_VOID *)&affectedrows,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
		assertEquals(affectedrows,1);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	CS_DATAFMT	dynfmt[4];
	char		*dyndata[4];
	CS_INT		dyndatalength[4];
	CS_SMALLINT	dynnullindicator[4];


	// Freetds sends CS_CHAR_TYPE as a bigchar sized at
	// CS_DATAFMT.maxlength, so mssql receives a value already blank
	// padded to 20 and keeps the padding even in the varchar column.
	// ASE's tds 5 parameter carries the real length, and a nullable
	// char is stored as a varchar, so nothing is padded there.
	const char	*dyncharexpect[4]={"one","two","three",NULL};
	const char	*dynvarcharexpect[4]={"uno","dos","tres",NULL};
	CS_INT		dyncharlength[4]={4,4,6,0};
	CS_INT		dynvarcharlength[4]={4,4,5,0};
	if (!issybase) {
		dyncharexpect[0]="one                 ";
		dyncharexpect[1]="two                 ";
		dyncharexpect[2]="three               ";
		dynvarcharexpect[0]="uno                 ";
		dynvarcharexpect[1]="dos                 ";
		dynvarcharexpect[2]="tres                ";
		dyncharlength[0]=21;
		dyncharlength[1]=21;
		dyncharlength[2]=21;
		dynvarcharlength[0]=21;
		dynvarcharlength[1]=21;
		dynvarcharlength[2]=21;
	}


	stdoutput.printf("ct_command: select\n");
	query="select testid, testchar, testvarchar, testint "
		"from dyntable order by testid";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	for (CS_INT i=0; i<4; i++) {
		dyndata[i]=new char[1024];
		bytestring::zero(dyndata[i],1024);
		bytestring::zero(&(dynfmt[i]),sizeof(CS_DATAFMT));
		dynfmt[i].datatype=CS_CHAR_TYPE;
		dynfmt[i].format=CS_FMT_NULLTERM;
		dynfmt[i].maxlength=1024;
		dynfmt[i].count=1;
		assertEquals(ct_bind(cmd,i+1,&(dynfmt[i]),
					(CS_VOID *)dyndata[i],
					&(dyndatalength[i]),
					&(dynnullindicator[i])),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_fetch:\n");
	for (CS_INT i=0; i<4; i++) {

		bytestring::zero(dyndata[1],1024);
		bytestring::zero(dyndata[2],1024);

		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);

		char	dynexpectedid[2];
		dynexpectedid[0]='1'+(char)i;
		dynexpectedid[1]='\0';
		assertEquals(dyndata[0],dynexpectedid);
		assertEquals(dyndatalength[0],2);
		assertEquals(dynnullindicator[0],0);

		if (i==3) {
			assertEquals(dyndata[1],"");
			assertEquals(dyndatalength[1],0);
			assertEquals(dynnullindicator[1],-1);
			assertEquals(dyndata[2],"");
			assertEquals(dyndatalength[2],0);
			assertEquals(dynnullindicator[2],-1);
		} else {
			assertEquals(dyndata[1],dyncharexpect[i]);
			assertEquals(dyndatalength[1],dyncharlength[i]);
			assertEquals(dynnullindicator[1],0);
			assertEquals(dyndata[2],dynvarcharexpect[i]);
			assertEquals(dyndatalength[2],dynvarcharlength[i]);
			assertEquals(dynnullindicator[2],0);
		}

		char	dynexpectedint[4];
		dynexpectedint[0]='1';
		dynexpectedint[1]='0';
		dynexpectedint[2]='1'+(char)i;
		dynexpectedint[3]='\0';
		assertEquals(dyndata[3],dynexpectedint);
		assertEquals(dyndatalength[3],4);
		assertEquals(dynnullindicator[3],0);
	}
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_dynamic: prepare select\n");
	assertEquals(ct_dynamic(cmd,CS_PREPARE,
				(CS_CHAR *)dynselectid,CS_NULLTERM,
				(CS_CHAR *)dynselect,CS_NULLTERM),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_dynamic: describe input on select\n");
	assertEquals(ct_dynamic(cmd,CS_DESCRIBE_INPUT,
				(CS_CHAR *)dynselectid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_DESCRIBE_RESULT);
	ncols=-1;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,(issybase)?1:0);
	if (issybase) {
		bytestring::zero(&(dyndesc[0]),sizeof(CS_DATAFMT));
		assertEquals(ct_describe(cmd,1,&(dyndesc[0])),CS_SUCCEED);
		assertEquals(dyndesc[0].name,"");
		assertEquals(dyndesc[0].namelen,0);
		assertEquals(dyndesc[0].datatype,CS_INT_TYPE);
		assertEquals(dyndesc[0].format,0);
		assertEquals(dyndesc[0].maxlength,4);
		assertEquals(dyndesc[0].precision,0);
		assertEquals(dyndesc[0].scale,0);
		assertEquals(dyndesc[0].status,0);
		assertEquals(dyndesc[0].count,1);
		assertEquals(dyndesc[0].usertype,0);
		assertTrue(dyndesc[0].locale==NULL);
	}
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// The output describe is real on both backends.  Only usertype
	// splits - mssql reports 0 for everything while ASE reports its
	// syscolumns ids, the same split #8779 found for ct_describe.
	const char	*dyncolname[4]={
				"testid","testchar","testvarchar","testint"};
	CS_INT		dyncolnamelen[4]={6,8,11,7};
	CS_INT		dyncoltype[4]={
				CS_INT_TYPE,CS_CHAR_TYPE,
				CS_CHAR_TYPE,CS_INT_TYPE};
	CS_INT		dyncolmaxlength[4]={4,20,20,4};
	CS_INT		dyncolusertype[4]={0,0,0,0};
	if (issybase) {
		dyncolusertype[0]=7;
		dyncolusertype[1]=1;
		dyncolusertype[2]=2;
		dyncolusertype[3]=7;
	}


	stdoutput.printf("ct_dynamic: describe output on select\n");
	assertEquals(ct_dynamic(cmd,CS_DESCRIBE_OUTPUT,
				(CS_CHAR *)dynselectid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_DESCRIBE_RESULT);
	ncols=-1;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,4);
	for (CS_INT i=0; i<4; i++) {
		bytestring::zero(&(dyndesc[i]),sizeof(CS_DATAFMT));
		assertEquals(ct_describe(cmd,i+1,&(dyndesc[i])),CS_SUCCEED);
		assertEquals(dyndesc[i].name,dyncolname[i]);
		assertEquals(dyndesc[i].namelen,dyncolnamelen[i]);
		assertEquals(dyndesc[i].datatype,dyncoltype[i]);
		assertEquals(dyndesc[i].format,0);
		assertEquals(dyndesc[i].maxlength,dyncolmaxlength[i]);
		assertEquals(dyndesc[i].precision,0);
		assertEquals(dyndesc[i].scale,0);
		assertEquals(dyndesc[i].status,CS_UPDATABLE|CS_CANBENULL);
		assertEquals(dyndesc[i].count,1);
		assertEquals(dyndesc[i].usertype,dyncolusertype[i]);
		assertTrue(dyndesc[i].locale==NULL);
	}
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// A prepared select carries an extra CS_CMD_SUCCEED and
	// CS_CMD_DONE pair after the rows that a language select does
	// not - on mssql that is sp_execute's own done, on ASE it is the
	// dynamic exec's.
	stdoutput.printf("ct_dynamic: execute select\n");
	for (CS_INT i=0; i<2; i++) {

		assertEquals(ct_dynamic(cmd,CS_EXECUTE,
				(CS_CHAR *)dynselectid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);

		bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
		dynparam[0].datatype=CS_INT_TYPE;
		dynparam[0].maxlength=4;
		dynparam[0].count=1;
		dynintvalue[0]=i+2;
		assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[0]),
					sizeof(CS_INT),0),CS_SUCCEED);

		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_ROW_RESULT);

		ncols=-1;
		assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
		assertEquals(ncols,4);

		for (CS_INT j=0; j<4; j++) {
			bytestring::zero(dyndata[j],1024);
			assertEquals(ct_bind(cmd,j+1,&(dynfmt[j]),
					(CS_VOID *)dyndata[j],
					&(dyndatalength[j]),
					&(dynnullindicator[j])),CS_SUCCEED);
		}

		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);

		char	dynexpectedid[2];
		dynexpectedid[0]='2'+(char)i;
		dynexpectedid[1]='\0';
		assertEquals(dyndata[0],dynexpectedid);
		assertEquals(dyndata[1],dyncharexpect[i+1]);
		assertEquals(dyndatalength[1],dyncharlength[i+1]);
		assertEquals(dyndata[2],dynvarcharexpect[i+1]);
		assertEquals(dyndatalength[2],dynvarcharlength[i+1]);

		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	// ct_setparam takes the length and indicator by pointer, but the
	// bindings do not survive the next ct_dynamic(CS_EXECUTE), which
	// calls param_clear, so they have to be re-issued for every
	// execute anyway.
	stdoutput.printf("ct_dynamic: execute insert with ct_setparam\n");
	assertEquals(ct_dynamic(cmd,CS_EXECUTE,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);
	CS_INT		dynsetlength[4];
	CS_SMALLINT	dynsetindicator[4]={0,0,0,0};
	dynintvalue[0]=5;
	dynsetlength[0]=sizeof(CS_INT);
	bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
	dynparam[0].datatype=CS_INT_TYPE;
	dynparam[0].maxlength=4;
	dynparam[0].count=1;
	assertEquals(ct_setparam(cmd,&(dynparam[0]),
				(CS_VOID *)&(dynintvalue[0]),
				&(dynsetlength[0]),
				&(dynsetindicator[0])),CS_SUCCEED);
	charstring::copy(dyncharvalue[0],"five");
	dynsetlength[1]=charstring::getLength(dyncharvalue[0]);
	bytestring::zero(&(dynparam[1]),sizeof(CS_DATAFMT));
	dynparam[1].datatype=CS_CHAR_TYPE;
	dynparam[1].maxlength=20;
	dynparam[1].count=1;
	assertEquals(ct_setparam(cmd,&(dynparam[1]),
				(CS_VOID *)dyncharvalue[0],
				&(dynsetlength[1]),
				&(dynsetindicator[1])),CS_SUCCEED);
	charstring::copy(dyncharvalue[1],"cinco");
	dynsetlength[2]=charstring::getLength(dyncharvalue[1]);
	bytestring::zero(&(dynparam[2]),sizeof(CS_DATAFMT));
	dynparam[2].datatype=CS_CHAR_TYPE;
	dynparam[2].maxlength=20;
	dynparam[2].count=1;
	assertEquals(ct_setparam(cmd,&(dynparam[2]),
				(CS_VOID *)dyncharvalue[1],
				&(dynsetlength[2]),
				&(dynsetindicator[2])),CS_SUCCEED);
	dynintvalue[3]=105;
	dynsetlength[3]=sizeof(CS_INT);
	bytestring::zero(&(dynparam[3]),sizeof(CS_DATAFMT));
	dynparam[3].datatype=CS_INT_TYPE;
	dynparam[3].maxlength=4;
	dynparam[3].count=1;
	assertEquals(ct_setparam(cmd,&(dynparam[3]),
				(CS_VOID *)&(dynintvalue[3]),
				&(dynsetlength[3]),
				&(dynsetindicator[3])),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	assertEquals(ct_res_info(cmd,CS_ROW_COUNT,
				(CS_VOID *)&affectedrows,CS_UNUSED,
				(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(affectedrows,1);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: select setparam row\n");
	query="select testchar, testvarchar from dyntable where testid = 5";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	for (CS_INT i=0; i<2; i++) {
		bytestring::zero(dyndata[i],1024);
		assertEquals(ct_bind(cmd,i+1,&(dynfmt[i]),
					(CS_VOID *)dyndata[i],
					&(dyndatalength[i]),
					&(dynnullindicator[i])),CS_SUCCEED);
	}
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	assertEquals(dyndata[0],(issybase)?"five":"five                ");
	assertEquals(dyndatalength[0],(issybase)?5:21);
	assertEquals(dyndata[1],(issybase)?"cinco":"cinco               ");
	assertEquals(dyndatalength[1],(issybase)?6:21);
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// MSSQL answers sp_unprepare with only a return status and a
	// done, so freetds reports no result sets at all.  ASE's dynamic
	// dealloc gets the ordinary pair.
	stdoutput.printf("ct_dynamic: dealloc select\n");
	assertEquals(ct_dynamic(cmd,CS_DEALLOC,
				(CS_CHAR *)dynselectid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	if (issybase) {
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
	}
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_dynamic: dealloc insert\n");
	assertEquals(ct_dynamic(cmd,CS_DEALLOC,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	if (issybase) {
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
	}
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// These four never reach ct_send.  The ones that cannot be
	// covered at all are ct_param with a null CS_DATAFMT,
	// ct_setparam with a null length or indicator pointer,
	// ct_dynamic with a null id and CS_NULLTERM, ct_command with a
	// null rpc name, and ct_describe on a zero column result - every
	// one of them segfaults inside libct.
	stdoutput.printf("ct_dynamic: negatives\n");
	assertEquals(ct_dynamic(cmd,CS_EXECUTE,
				(CS_CHAR *)"dynnosuch",CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_FAIL);
	assertEquals(ct_dynamic(cmd,CS_DEALLOC,
				(CS_CHAR *)"dynnosuch",CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_FAIL);
	assertEquals(ct_dynamic(cmd,CS_DESCRIBE_INPUT,
				(CS_CHAR *)"dynnosuch",CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_FAIL);
	assertEquals(ct_dynamic(cmd,999,
				(CS_CHAR *)"dynnosuch",CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_FAIL);
	stdoutput.printf("\n");


	// Preparing an id that is already live fails at ct_send with no
	// client message and no server message at all.  ct_results then
	// returns CS_FAIL rather than a result type.  This block sits
	// after everything that needs a healthy command, because leaving
	// the failure neither drained nor cancelled desynchronizes every
	// block after it.
	stdoutput.printf("ct_dynamic: prepare a live id twice\n");
	assertEquals(ct_dynamic(cmd,CS_PREPARE,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)dyninsert,CS_NULLTERM),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	assertEquals(ct_dynamic(cmd,CS_PREPARE,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)dyninsert,CS_NULLTERM),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_FAIL);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_FAIL);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	assertEquals(ct_dynamic(cmd,CS_DEALLOC,
				(CS_CHAR *)dyninsertid,CS_NULLTERM,
				(CS_CHAR *)NULL,CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	if (issybase) {
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
	}
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: after dynamic\n");
	query="select convert(varchar(20),count(*)) from dyntable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	bytestring::zero(dyndata[0],1024);
	assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
				(CS_VOID *)dyndata[0],
				&(dyndatalength[0]),
				&(dynnullindicator[0])),CS_SUCCEED);
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	assertEquals(dyndata[0],"5");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// An output parameter's status must be exactly CS_RETURN.
	// CS_INPUTVALUE|CS_RETURN silently does not mark it output,
	// because freetds compares status for equality rather than by
	// bit, and the parameter then never comes back.
	stdoutput.printf("ct_command: rpc dynproc\n");
	assertEquals(ct_command(cmd,CS_RPC_CMD,
				(CS_CHAR *)"dynproc",CS_NULLTERM,
				CS_UNUSED),CS_SUCCEED);
	bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
	dynparam[0].datatype=CS_INT_TYPE;
	dynparam[0].maxlength=4;
	dynparam[0].count=1;
	charstring::copy(dynparam[0].name,"@inparam");
	dynparam[0].namelen=8;
	dynintvalue[0]=2;
	assertEquals(ct_param(cmd,&(dynparam[0]),
				(CS_VOID *)&(dynintvalue[0]),
				sizeof(CS_INT),0),CS_SUCCEED);
	bytestring::zero(&(dynparam[1]),sizeof(CS_DATAFMT));
	dynparam[1].datatype=CS_INT_TYPE;
	dynparam[1].maxlength=4;
	dynparam[1].count=1;
	dynparam[1].status=CS_RETURN;
	charstring::copy(dynparam[1].name,"@outparam");
	dynparam[1].namelen=9;
	dynintvalue[1]=0;
	assertEquals(ct_param(cmd,&(dynparam[1]),
				(CS_VOID *)&(dynintvalue[1]),
				sizeof(CS_INT),0),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_results: rpc rows\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_ROW_RESULT);
	ncols=-1;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,2);
	for (CS_INT i=0; i<2; i++) {
		bytestring::zero(dyndata[i],1024);
		assertEquals(ct_bind(cmd,i+1,&(dynfmt[i]),
					(CS_VOID *)dyndata[i],
					&(dyndatalength[i]),
					&(dynnullindicator[i])),CS_SUCCEED);
	}
	for (CS_INT i=0; i<2; i++) {
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);
		char	dynexpectedid[2];
		dynexpectedid[0]='1'+(char)i;
		dynexpectedid[1]='\0';
		assertEquals(dyndata[0],dynexpectedid);
		assertEquals(dyndata[1],dyncharexpect[i]);
	}
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	stdoutput.printf("\n");


	// The return status arrives before the output parameters on both
	// backends.
	stdoutput.printf("ct_results: rpc return status\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_STATUS_RESULT);
	ncols=-1;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,1);
	bytestring::zero(&(dyndesc[0]),sizeof(CS_DATAFMT));
	assertEquals(ct_describe(cmd,1,&(dyndesc[0])),CS_SUCCEED);
	assertEquals(dyndesc[0].name,"");
	assertEquals(dyndesc[0].namelen,0);
	assertEquals(dyndesc[0].datatype,CS_INT_TYPE);
	assertEquals(dyndesc[0].maxlength,4);
	assertEquals(dyndesc[0].status,0);
	assertEquals(dyndesc[0].count,1);
	bytestring::zero(dyndata[0],1024);
	assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
				(CS_VOID *)dyndata[0],
				&(dyndatalength[0]),
				&(dynnullindicator[0])),CS_SUCCEED);
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	assertEquals(dyndata[0],"42");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	stdoutput.printf("\n");


	// The name comes back only because it was set on the way in.  The
	// returned status is 0, not CS_RETURN, so an output parameter
	// cannot be told from the describe - only from the result type.
	stdoutput.printf("ct_results: rpc output params\n");
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_PARAM_RESULT);
	ncols=-1;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,1);
	bytestring::zero(&(dyndesc[0]),sizeof(CS_DATAFMT));
	assertEquals(ct_describe(cmd,1,&(dyndesc[0])),CS_SUCCEED);
	assertEquals(dyndesc[0].name,"@outparam");
	assertEquals(dyndesc[0].namelen,9);
	assertEquals(dyndesc[0].datatype,CS_INT_TYPE);
	assertEquals(dyndesc[0].maxlength,4);
	assertEquals(dyndesc[0].status,0);
	assertEquals(dyndesc[0].count,1);
	assertEquals(dyndesc[0].usertype,(issybase)?7:0);
	bytestring::zero(dyndata[0],1024);
	assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
				(CS_VOID *)dyndata[0],
				&(dyndatalength[0]),
				&(dynnullindicator[0])),CS_SUCCEED);
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals(rowsread,1);
	assertEquals(dyndata[0],"20");
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");

	// The five procs this ticket names are tds 7 constructs.  ASE has
	// none of them, and every call comes back not found with status
	// -6.  On mssql three of them cannot be driven from ct-lib at all:
	// sp_prepare, sp_prepexec and sp_executesql want
	// ntext/nchar/nvarchar and no CS_DATAFMT.datatype in freetds 1.3.3
	// produces one, so the server rejects the argument type with its
	// own error number 214.
	CS_INT		dynrpcstatus=(issybase)?-6:214;
	const char	*dynrpcparams="@P1 int";
	const char	*dynrpcstmt="select @P1 as dynval";


	stdoutput.printf("ct_command: rpc sp_prepare and sp_prepexec\n");
	for (CS_INT i=0; i<2; i++) {

		assertEquals(ct_command(cmd,CS_RPC_CMD,
				(CS_CHAR *)((i)?"sp_prepexec":"sp_prepare"),
				CS_NULLTERM,CS_UNUSED),CS_SUCCEED);

		bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
		dynparam[0].datatype=CS_INT_TYPE;
		dynparam[0].maxlength=4;
		dynparam[0].count=1;
		dynparam[0].status=CS_RETURN;
		dynintvalue[0]=0;
		assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[0]),
					sizeof(CS_INT),0),CS_SUCCEED);

		bytestring::zero(&(dynparam[1]),sizeof(CS_DATAFMT));
		dynparam[1].datatype=CS_CHAR_TYPE;
		dynparam[1].maxlength=64;
		dynparam[1].count=1;
		assertEquals(ct_param(cmd,&(dynparam[1]),
				(CS_VOID *)dynrpcparams,
				charstring::getLength(dynrpcparams),
				0),CS_SUCCEED);
		assertEquals(ct_param(cmd,&(dynparam[1]),
				(CS_VOID *)dynrpcstmt,
				charstring::getLength(dynrpcstmt),
				0),CS_SUCCEED);

		bytestring::zero(&(dynparam[2]),sizeof(CS_DATAFMT));
		dynparam[2].datatype=CS_INT_TYPE;
		dynparam[2].maxlength=4;
		dynparam[2].count=1;
		dynintvalue[2]=1;
		assertEquals(ct_param(cmd,&(dynparam[2]),
					(CS_VOID *)&(dynintvalue[2]),
					sizeof(CS_INT),0),CS_SUCCEED);

		assertEquals(ct_send(cmd),CS_SUCCEED);

		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_STATUS_RESULT);
		bytestring::zero(dyndata[0],1024);
		assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals((CS_INT)charstring::convertToInteger(dyndata[0]),
							dynrpcstatus);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);

		// mssql got as far as running the procedure, so the
		// output handle comes back, null.  ASE never found the
		// procedure at all, so there is no parameter result.
		if (!issybase) {
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_PARAM_RESULT);
			ncols=-1;
			assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
			assertEquals(ncols,1);
			bytestring::zero(dyndata[0],1024);
			assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
			assertEquals(dynnullindicator[0],-1);
			assertEquals(dyndatalength[0],0);
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
		}

		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_FAIL);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	}
	stdoutput.printf("\n");


	// sp_executesql has no output parameter, so unlike the two above
	// it yields no CS_PARAM_RESULT on mssql.
	stdoutput.printf("ct_command: rpc sp_executesql\n");
	assertEquals(ct_command(cmd,CS_RPC_CMD,
				(CS_CHAR *)"sp_executesql",CS_NULLTERM,
				CS_UNUSED),CS_SUCCEED);
	bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
	dynparam[0].datatype=CS_CHAR_TYPE;
	dynparam[0].maxlength=64;
	dynparam[0].count=1;
	assertEquals(ct_param(cmd,&(dynparam[0]),
			(CS_VOID *)dynrpcstmt,
			charstring::getLength(dynrpcstmt),0),CS_SUCCEED);
	assertEquals(ct_param(cmd,&(dynparam[0]),
			(CS_VOID *)dynrpcparams,
			charstring::getLength(dynrpcparams),0),CS_SUCCEED);
	bytestring::zero(&(dynparam[1]),sizeof(CS_DATAFMT));
	dynparam[1].datatype=CS_INT_TYPE;
	dynparam[1].maxlength=4;
	dynparam[1].count=1;
	dynintvalue[1]=1;
	assertEquals(ct_param(cmd,&(dynparam[1]),
				(CS_VOID *)&(dynintvalue[1]),
				sizeof(CS_INT),0),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_STATUS_RESULT);
	bytestring::zero(dyndata[0],1024);
	assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
				(CS_VOID *)dyndata[0],
				&(dyndatalength[0]),
				&(dynnullindicator[0])),CS_SUCCEED);
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
	assertEquals((CS_INT)charstring::convertToInteger(dyndata[0]),
							dynrpcstatus);
	assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_FAIL);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	if (issybase) {

		// ASE has neither of these either
		stdoutput.printf("ct_command: rpc sp_execute "
						"and sp_unprepare\n");
		for (CS_INT i=0; i<2; i++) {

			assertEquals(ct_command(cmd,CS_RPC_CMD,
				(CS_CHAR *)((i)?"sp_unprepare":"sp_execute"),
				CS_NULLTERM,CS_UNUSED),CS_SUCCEED);

			bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
			dynparam[0].datatype=CS_INT_TYPE;
			dynparam[0].maxlength=4;
			dynparam[0].count=1;
			dynintvalue[0]=1;
			assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[0]),
					sizeof(CS_INT),0),CS_SUCCEED);

			assertEquals(ct_send(cmd),CS_SUCCEED);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_STATUS_RESULT);
			bytestring::zero(dyndata[0],1024);
			assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
			assertEquals(charstring::convertToInteger(
							dyndata[0]),-6);
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_CMD_FAIL);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_CMD_DONE);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_END_RESULTS);
			assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),
								CS_SUCCEED);
		}
		stdoutput.printf("\n");

	} else {

		// A language batch is the only way to get a real handle,
		// since N'' literals are the only route to an nvarchar
		// from this client.  Once there is one, sp_execute and
		// sp_unprepare drive fine by name - they take plain ints.
		stdoutput.printf("ct_command: sp_prepare in a "
						"language batch\n");
		query="declare @dynhandle int "
			"exec sp_prepare @dynhandle output, "
			"N'@P1 int', N'select @P1 as dynval', 1 "
			"select convert(varchar(20),@dynhandle) as dynhandle";
		assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);

		// sp_prepare with options 1 answers with the prepared
		// statement's column metadata as an empty result set
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_ROW_RESULT);
		ncols=-1;
		assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
		assertEquals(ncols,1);
		bytestring::zero(&(dyndesc[0]),sizeof(CS_DATAFMT));
		assertEquals(ct_describe(cmd,1,&(dyndesc[0])),CS_SUCCEED);
		assertEquals(dyndesc[0].name,"dynval");
		assertEquals(dyndesc[0].datatype,CS_INT_TYPE);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);

		// A failed rpc leaves its return status cached in
		// freetds, and the next language batch reports that
		// instead of its own.  The 214 asserted here belongs to
		// the sp_executesql above; run after any other command
		// this batch reports its own 0.  The handle below is not
		// affected.
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_STATUS_RESULT);
		bytestring::zero(dyndata[0],1024);
		assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(dyndata[0],"214");
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);

		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_ROW_RESULT);
		bytestring::zero(dyndata[0],1024);
		assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(rowsread,1);

		// The server picks the handle, so only that it is usable
		// is asserted, never what it equals.
		CS_INT	dynhandle=charstring::convertToInteger(dyndata[0]);
		assertTrue(dynhandle>0);
		assertEquals(dynnullindicator[0],0);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: rpc sp_execute\n");
		for (CS_INT i=0; i<2; i++) {

			assertEquals(ct_command(cmd,CS_RPC_CMD,
					(CS_CHAR *)"sp_execute",CS_NULLTERM,
					CS_UNUSED),CS_SUCCEED);

			bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
			dynparam[0].datatype=CS_INT_TYPE;
			dynparam[0].maxlength=4;
			dynparam[0].count=1;
			dynintvalue[0]=dynhandle;
			assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[0]),
					sizeof(CS_INT),0),CS_SUCCEED);
			dynintvalue[1]=77+i;
			assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[1]),
					sizeof(CS_INT),0),CS_SUCCEED);

			assertEquals(ct_send(cmd),CS_SUCCEED);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_ROW_RESULT);
			bytestring::zero(dyndata[0],1024);
			assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
			assertEquals(rowsread,1);
			assertEquals(charstring::convertToInteger(
						dyndata[0]),77+i);
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_CMD_DONE);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_STATUS_RESULT);
			bytestring::zero(dyndata[0],1024);
			assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
			assertEquals(dyndata[0],"0");
			assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_CMD_SUCCEED);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_SUCCEED);
			assertEquals(resultstype,CS_CMD_DONE);
			results=ct_results(cmd,&resultstype);
			assertEquals(results,CS_END_RESULTS);
			assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),
								CS_SUCCEED);
		}
		stdoutput.printf("\n");


		stdoutput.printf("ct_command: rpc sp_unprepare\n");
		assertEquals(ct_command(cmd,CS_RPC_CMD,
				(CS_CHAR *)"sp_unprepare",CS_NULLTERM,
				CS_UNUSED),CS_SUCCEED);
		bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
		dynparam[0].datatype=CS_INT_TYPE;
		dynparam[0].maxlength=4;
		dynparam[0].count=1;
		dynintvalue[0]=dynhandle;
		assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[0]),
					sizeof(CS_INT),0),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_STATUS_RESULT);
		bytestring::zero(dyndata[0],1024);
		assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(dyndata[0],"0");
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");


		// the handle is gone, so the call that worked twice above
		// now returns the server's own error number
		stdoutput.printf("ct_command: rpc sp_execute "
						"after unprepare\n");
		assertEquals(ct_command(cmd,CS_RPC_CMD,
				(CS_CHAR *)"sp_execute",CS_NULLTERM,
				CS_UNUSED),CS_SUCCEED);
		bytestring::zero(&(dynparam[0]),sizeof(CS_DATAFMT));
		dynparam[0].datatype=CS_INT_TYPE;
		dynparam[0].maxlength=4;
		dynparam[0].count=1;
		dynintvalue[0]=dynhandle;
		assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[0]),
					sizeof(CS_INT),0),CS_SUCCEED);
		dynintvalue[1]=77;
		assertEquals(ct_param(cmd,&(dynparam[0]),
					(CS_VOID *)&(dynintvalue[1]),
					sizeof(CS_INT),0),CS_SUCCEED);
		assertEquals(ct_send(cmd),CS_SUCCEED);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_STATUS_RESULT);
		bytestring::zero(dyndata[0],1024);
		assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(dyndata[0],"8179");
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_FAIL);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_CMD_DONE);
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_END_RESULTS);
		assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
		stdoutput.printf("\n");
	}


	stdoutput.printf("ct_command: rpc no such procedure\n");
	assertEquals(ct_command(cmd,CS_RPC_CMD,
				(CS_CHAR *)"dynnosuchproc",CS_NULLTERM,
				CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	if (issybase) {
		results=ct_results(cmd,&resultstype);
		assertEquals(results,CS_SUCCEED);
		assertEquals(resultstype,CS_STATUS_RESULT);
		bytestring::zero(dyndata[0],1024);
		assertEquals(ct_bind(cmd,1,&(dynfmt[0]),
					(CS_VOID *)dyndata[0],
					&(dyndatalength[0]),
					&(dynnullindicator[0])),CS_SUCCEED);
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_SUCCEED);
		assertEquals(dyndata[0],"-6");
		assertEquals(ct_fetch(cmd,CS_UNUSED,CS_UNUSED,
					CS_UNUSED,&rowsread),CS_END_DATA);
	}
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_FAIL);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: drop procedure\n");
	query="drop procedure dynproc";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_command: drop\n");
	query="drop table dyntable";
	assertEquals(ct_command(cmd,CS_LANG_CMD,
					query,charstring::getLength(query),
					CS_UNUSED),CS_SUCCEED);
	assertEquals(ct_send(cmd),CS_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_SUCCEED);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_SUCCEED);
	assertEquals(resultstype,CS_CMD_DONE);
	results=ct_results(cmd,&resultstype);
	assertEquals(results,CS_END_RESULTS);
	assertEquals(ct_cancel(NULL,cmd,CS_CANCEL_ALL),CS_SUCCEED);
	stdoutput.printf("\n");


	// The sections below are the coverage this test still owes, each
	// tracked by its own ticket.  They are named here rather than left
	// out so the gaps are visible in the output, the way
	// test/protocol/mysql/mysql.cpp names its API sections.

	stdoutput.printf("\n=============== Cursors ===============\n\n");
	// #8790 - no ct_cursor coverage yet
	stdoutput.printf("not covered yet - see trac #8790\n\n");

	stdoutput.printf("\n================ Binds ================\n\n");
	// #8792 - no ct_param coverage yet
	stdoutput.printf("not covered yet - see trac #8792\n\n");

	reportTestStatus();
	return status;
}
