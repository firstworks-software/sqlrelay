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

	// pass "native" to test a real sql server/sybase instance
	// instead of sqlrelay's tds protocol
	bool	issqlrelay=!(argc==2 && !charstring::compare(argv[1],"native"));
	if (issqlrelay) {
		server="localhost";
		db="";
	} else {
		// short hostname, matching the db the native odbc tests use
		char	*hostname=sys::getHostName();
		char	*dot=(char *)charstring::findFirstOrEnd(hostname,'.');
		*dot='\0';
		server="mssql";
		db=hostname;
	}
	user="testuser";
	password="testpassword";


	CS_CONTEXT	*context=NULL;
	CS_CONNECTION	*dbconn=NULL;

	environment::setValue("DSQUERY",server);

	stdoutput.printf("\n================ Login ================\n\n");

	stdoutput.printf("cs_ctx_alloc\n");
	assertEquals(cs_ctx_alloc(CS_VERSION_100,&context),CS_SUCCEED);
	stdoutput.printf("\n");


	stdoutput.printf("ct_init\n");
	assertEquals(ct_init(context,CS_VERSION_100),CS_SUCCEED);
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
	uint16_t	ps=4096;
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
	assertEquals(ct_connect(dbconn,(CS_CHAR *)NULL,(CS_INT)0),CS_SUCCEED);
	stdoutput.printf("\n\n");



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


	stdoutput.printf("ct_res_info: col count\n");
	CS_INT	ncols;
	assertEquals(ct_res_info(cmd,CS_NUMDATA,
					(CS_VOID *)&ncols,CS_UNUSED,
					(CS_INT *)NULL),CS_SUCCEED);
	assertEquals(ncols,28);
	stdoutput.printf("\n");


	stdoutput.printf("ct_bind:\n");
	CS_DATAFMT	column[28];
	char		*data[28];
	CS_INT		*datalength[28];
	CS_SMALLINT	*nullindicator[28];
	for (CS_INT i=0; i<ncols; i++) {
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
		assertEquals(ct_bind(cmd,i+1,&(column[i]),
						(CS_VOID *)data[i],
						datalength[i],
						nullindicator[i]),
						CS_SUCCEED);
	}
	stdoutput.printf("\n");


	stdoutput.printf("ct_describe:\n");
	for (CS_INT i=0; i<ncols; i++) {
		assertEquals(ct_describe(cmd,i+1,&(column[i])),CS_SUCCEED);
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
	assertEquals(column[0].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[1].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[2].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[3].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[4].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[5].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[6].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[7].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[8].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[9].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[10].usertype,CS_CHAR_TYPE);
	stdoutput.printf("\n");

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
	assertEquals(column[12].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[13].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[14].usertype,CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[15].name);
	assertEquals(column[15].name,"testtime");
	assertEquals(column[15].datatype,CS_BIGTIME_TYPE);
	assertEquals(column[15].format,CS_FMT_NULLTERM);
	// FIXME: 16/7/7 direct, 64/0/0 via relay
	//assertEquals(column[15].maxlength,16);
	//assertEquals(column[15].precision,7);
	//assertEquals(column[15].scale,7);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[15].status,CS_UNUSED);
	assertEquals(column[15].count,1);
	assertEquals(column[15].usertype,CS_CHAR_TYPE);
	stdoutput.printf("\n");

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
	assertEquals(column[18].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[19].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[20].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[21].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[22].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[23].usertype,CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[24].name);
	assertEquals(column[24].name,"testxml");
	//assertEquals(column[24].datatype,CS_XML_TYPE);
	assertEquals(column[24].datatype,CS_LONGCHAR_TYPE);
	assertEquals(column[24].format,CS_FMT_NULLTERM);
	// maxlength limited by maxfieldlength via relay, but not directly
	//assertEquals(column[24].maxlength,131068);
	assertEquals(column[24].precision,0);
	assertEquals(column[24].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[24].status,CS_UNUSED);
	assertEquals(column[24].count,1);
	assertEquals(column[24].usertype,CS_CHAR_TYPE);
	stdoutput.printf("\n");

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
	assertEquals(column[25].usertype,CS_CHAR_TYPE);
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
	assertEquals(column[26].usertype,CS_CHAR_TYPE);
	stdoutput.printf("\n");

	stdoutput.printf("%s\n",column[27].name);
	assertEquals(column[27].name,"testntext");
	assertEquals(column[27].datatype,CS_TEXT_TYPE);
	assertEquals(column[27].format,CS_FMT_NULLTERM);
	// maxlength limited by maxfieldlength via relay, but not directly
	//assertEquals(column[27].maxlength,131068);
	assertEquals(column[27].precision,0);
	assertEquals(column[27].scale,0);
	// FIXME: 48 direct, 0 via relay
	//assertEquals(column[27].status,CS_UNUSED);
	assertEquals(column[27].count,1);
	assertEquals(column[27].usertype,CS_CHAR_TYPE);
	stdoutput.printf("\n");


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
	assertEquals(data[11],"01020304-0102-0304-0102-030401020304");
	assertEquals(*(datalength[11]),37);
	assertEquals(*(nullindicator[11]),0);
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
	assertEquals(data[24],"xml1");
	assertEquals(*(datalength[24]),5);
	assertEquals(*(nullindicator[24]),0);
	assertEquals(data[25],"text1");
	assertEquals(*(datalength[25]),6);
	assertEquals(*(nullindicator[25]),0);
	assertEquals(data[26],"696d61676531");
	assertEquals(*(datalength[26]),13);
	assertEquals(*(nullindicator[26]),0);
	assertEquals(data[27],"ntext1");
	assertEquals(*(datalength[27]),7);
	assertEquals(*(nullindicator[27]),0);
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
	assertEquals(data[11],"01020304-0102-0304-0102-030401020304");
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
	assertEquals(data[24],"xml2");
	assertEquals(data[25],"text2");
	assertEquals(data[26],"696d61676532");
	assertEquals(data[27],"ntext2");
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

	reportTestStatus();
	return status;
}
