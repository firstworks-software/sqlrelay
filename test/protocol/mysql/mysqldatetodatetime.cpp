#include <mysql.h>
#include <rudiments/process.h>
#include <rudiments/charstring.h>
#include <rudiments/bytestring.h>
#include <rudiments/environment.h>
#include <rudiments/stdio.h>
#include <config.h>

#include "asserts.cpp"

MYSQL		mysql;
MYSQL_RES	*result;
MYSQL_FIELD	*field;
MYSQL_ROW	row;

int	main(int argc, char **argv) {

	#ifdef HAVE_MYSQL_STMT_PREPARE

	const char	*host;
	const char	*port;
	const char	*socket;
	const char	*user;
	const char	*password;
	const char	*db;
	// to run against a real mysql instance, provide a host name
	// eg: ./mysql db64
	if (argc==2) {
		host=argv[1];
		db="testdb";
	} else {
		host="127.0.0.1";
		db="";
	}
	port="3306";
	socket="/var/lib/mysql/mysql.sock";
	user="testuser";
	password="testpassword";


	stdoutput.printf("\n============ Traditional API ============\n\n");

	#ifdef HAVE_MYSQL_REAL_CONNECT_FOR_SURE
		stdoutput.printf("mysql_init\n");
		assertEquals((long)mysql_init(&mysql),(long)&mysql);
		stdoutput.printf("\n");
		stdoutput.printf("mysql_real_connect\n");
		#if MYSQL_VERSION_ID>=32200
			assertEquals((long)mysql_real_connect(
						&mysql,host,user,password,db,
						charstring::convertToInteger(port),
						socket,0),(long)&mysql);
		#else
			assertEquals((long)mysql_real_connect(
						&mysql,host,user,password,
						charstring::convertToInteger(port),
						socket,0),(long)&mysql);
			if (!charstring::isNullOrEmpty(db)) {
				assertEquals(mysql_select_db(&mysql,db),0);
			}
		#endif
	#else
		assertEquals((long)mysql_connect(&mysql,host,
						user,password),
						(long)mysql);
	#endif
	stdoutput.printf("\n");

	const char	*query="drop table testtable";
	mysql_real_query(&mysql,query,charstring::getLength(query));

	stdoutput.printf("create\n");
	query="create table testtable (col1 date)";
	assertEquals(mysql_real_query(&mysql,query,charstring::getLength(query)),0);
	stdoutput.printf("\n");

	stdoutput.printf("list fields\n");
	result=mysql_list_fields(&mysql,"testtable",NULL);
	field=mysql_fetch_field_direct(result,0);
	assertEquals(field->type,MYSQL_TYPE_DATETIME);
	stdoutput.printf("\n");

	stdoutput.printf("alter nls_date_format\n");
	query="alter session set nls_date_format='YYYY-MM-DD HH24:MI:SS'";
	assertEquals(mysql_real_query(&mysql,query,charstring::getLength(query)),0);

	stdoutput.printf("insert\n");
	query="insert into testtable values ('2001-01-01 01:00:00')";
	assertEquals(mysql_real_query(&mysql,query,charstring::getLength(query)),0);
	stdoutput.printf("\n");

	stdoutput.printf("select\n");
	query="select * from testtable";
	assertEquals(mysql_real_query(&mysql,query,charstring::getLength(query)),0);
	field=mysql_fetch_field_direct(result,0);
	assertEquals(field->type,MYSQL_TYPE_DATETIME);
	stdoutput.printf("\n");

	stdoutput.printf("drop\n");
	query="drop table testtable";
	assertEquals(mysql_real_query(&mysql,query,charstring::getLength(query)),0);
	assertEquals(mysql_info(&mysql),NULL);
	stdoutput.printf("\n");

	mysql_close(&mysql);

	#else

	stdoutput.printf("\n====== MySQL Client Too Old to Test ======\n\n");

	#endif

	reportTestStatus();
	return status;
}
