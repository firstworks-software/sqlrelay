extern "C" {
	#include <ctpublic.h>
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


	// The sections below are the coverage this test still owes, each
	// tracked by its own ticket.  They are named here rather than left
	// out so the gaps are visible in the output, the way
	// test/protocol/mysql/mysql.cpp names its API sections.

	stdoutput.printf("\n=============== Cursors ===============\n\n");
	// #8790 - no ct_cursor coverage yet
	stdoutput.printf("not covered yet - see trac #8790\n\n");

	stdoutput.printf("\n============= RPC and Prepared ============\n\n");
	// #8791 - no ct_dynamic or sp_prepare/sp_execute coverage yet
	stdoutput.printf("not covered yet - see trac #8791\n\n");

	stdoutput.printf("\n================ Binds ================\n\n");
	// #8792 - no ct_param coverage yet
	stdoutput.printf("not covered yet - see trac #8792\n\n");

	stdoutput.printf("\n============== Bulk Load ==============\n\n");
	// #8794 - no blk_ coverage yet
	stdoutput.printf("not covered yet - see trac #8794\n\n");

	reportTestStatus();
	return status;
}
