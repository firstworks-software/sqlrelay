// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class FirebirdTest : SQLRTest
    {
        public static int Main(string[] args)
        {

            String[] bindvars = new String[] { "1","2","3","4","5","6",
                                    "7","8","9","10","11","12" };
            String[] bindvals = new String[] { "7","7","7.5","7.5","7.5","7.5",
                                    "01-JAN-2007","07:00:00",
                                    "testchar7","testvarchar7",null,"testblob7" };
            String[] cols;
            String[] fields;
            UInt32[] fieldlens;
            String[] subvars = new String[] { "var1","var2","var3" };
            Int64[] subvallongs = new Int64[] { 1,2,3 };
            String[] subvalstrings = new String[] { "hi","hello","bye" };
            Double[] subvaldoubles = new Double[] { 10.55,10.556,10.5556 };
            UInt32[] precs = new UInt32[] { 4,5,6 };
            UInt32[] scales = new UInt32[] { 2,3,4 };
            UInt16 port;
            String socket;
            UInt16 id;
            String filename;

            int LARGE_BUFFER_LENGTH = 20*1024;
            String largebuffer;


            // instantiation
            con = new SQLRConnection("sqlrelay", 9009, "/tmp/firebird.socket",
                                    "testuser", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(),"firebird");
            Console.WriteLine("");


            // ping
            Console.WriteLine("PING: ");
            assertTrue(con.ping());
            Console.WriteLine("");


            // transaction state
            Console.WriteLine("TRANSACTION STATE: ");
            assertEquals(con.getDefaultTransactionModel(), "implicit");
            assertEquals(con.getTransactionModel(), "implicit");
            assertTrue(con.getInTransaction());
            assertFalse(con.getAutoCommit());
            Console.WriteLine("");


            // bind format
            Console.WriteLine("BIND FORMAT: ");
            assertEquals(con.bindFormat(),"?");
            Console.WriteLine("");


            // nextval format
            Console.WriteLine("NEXTVAL FORMAT: ");
            assertEquals(con.nextvalFormat(),"next value for %s");
            Console.WriteLine("");


            // isolation levels
            Console.WriteLine("ISOLATION LEVELS: ");
            // though firebird does support a "set transaction ..." statement to
            // set the isolation level, it looks like, in firebird, you can really
            // only set it through the TPB at the start of a transaction, so
            // attempts to set it should fail
            assertFalse(con.setIsolationLevel("read committed"));
            assertEquals(con.getIsolationLevel(),"read committed");
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            cur.sendQuery("delete from testtable");
            con.commit();
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	1, " +
                "	1, " +
                "	1.5, " +
                "	1.5, " +
                "	1.5, " +
                "	1.5, " +
                "	'01-JAN-2001', " +
                "	'01:00:00', " +
                "	'testchar1', " +
                "	'testvarchar1', " +
                "	NULL, " +
                "	'testblob1')"));
            Console.WriteLine("");


            // affected rows
            Console.WriteLine("AFFECTED ROWS: ");
            assertEquals(cur.affectedRows(),(UInt64)1);
            Console.WriteLine("");


            // input bind by position
            Console.WriteLine("INPUT BIND BY POSITION: ");
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?)");
            assertEquals(cur.countBindVariables(),(UInt16)12);
            cur.inputBind("1",(Int64)2);
            cur.inputBind("2",(Int64)2);
            cur.inputBind("3",2.5,2,1);
            cur.inputBind("4",2.5,2,1);
            cur.inputBind("5",2.5,2,1);
            cur.inputBind("6",2.5,2,1);
            cur.inputBind("7",(Int16)2002,(Int16)1,(Int16)1,(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int32)(-1),(String)null,false);
            cur.inputBind("8",(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int16)2,(Int16)0,(Int16)0,(Int32)0,(String)null,false);
            cur.inputBind("9","testchar2");
            cur.inputBind("10","testvarchar2");
            cur.inputBind("11",(String)null);
            cur.inputBindBlob("12",System.Text.Encoding.ASCII.GetBytes("testblob2"),(UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1",(Int64)3);
            cur.inputBind("2",(Int64)3);
            cur.inputBind("3",3.5,2,1);
            cur.inputBind("4",3.5,2,1);
            cur.inputBind("5",3.5,2,1);
            cur.inputBind("6",3.5,2,1);
            cur.inputBind("7",(Int16)2003,(Int16)1,(Int16)1,(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int32)(-1),(String)null,false);
            cur.inputBind("8",(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int16)3,(Int16)0,(Int16)0,(Int32)0,(String)null,false);
            cur.inputBind("9","testchar3");
            cur.inputBind("10","testvarchar3");
            cur.inputBind("11",(String)null);
            cur.inputBindBlob("12",System.Text.Encoding.ASCII.GetBytes("testblob3"),(UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1",(Int64)4);
            cur.inputBind("2",(Int64)4);
            cur.inputBind("3",4.5,2,1);
            cur.inputBind("4",4.5,2,1);
            cur.inputBind("5",4.5,2,1);
            cur.inputBind("6",4.5,2,1);
            cur.inputBind("7",(Int16)2004,(Int16)1,(Int16)1,(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int32)(-1),(String)null,false);
            cur.inputBind("8",(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int16)4,(Int16)0,(Int16)0,(Int32)0,(String)null,false);
            cur.inputBind("9","testchar4");
            cur.inputBind("10","testvarchar4");
            cur.inputBind("11",(String)null);
            cur.inputBindBlob("12",System.Text.Encoding.ASCII.GetBytes("testblob4"),(UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1",(Int64)5);
            cur.inputBind("2",(Int64)5);
            cur.inputBind("3",5.5,2,1);
            cur.inputBind("4",5.5,2,1);
            cur.inputBind("5",5.5,2,1);
            cur.inputBind("6",5.5,2,1);
            cur.inputBind("7",(Int16)2005,(Int16)1,(Int16)1,(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int32)(-1),(String)null,false);
            cur.inputBind("8",(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int16)5,(Int16)0,(Int16)0,(Int32)0,(String)null,false);
            cur.inputBind("9","testchar5");
            cur.inputBind("10","testvarchar5");
            cur.inputBind("11",(String)null);
            cur.inputBindBlob("12",System.Text.Encoding.ASCII.GetBytes("testblob5"),(UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1",(Int64)6);
            cur.inputBind("2",(Int64)6);
            cur.inputBind("3",6.5,2,1);
            cur.inputBind("4",6.5,2,1);
            cur.inputBind("5",6.5,2,1);
            cur.inputBind("6",6.5,2,1);
            cur.inputBind("7",(Int16)2006,(Int16)1,(Int16)1,(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int32)(-1),(String)null,false);
            cur.inputBind("8",(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int16)6,(Int16)0,(Int16)0,(Int32)0,(String)null,false);
            cur.inputBind("9","testchar6");
            cur.inputBind("10","testvarchar6");
            cur.inputBind("11",(String)null);
            cur.inputBindBlob("12",System.Text.Encoding.ASCII.GetBytes("testblob6"),(UInt32)9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            Console.WriteLine("ARRAY OF INPUT BINDS BY POSITION: ");
            cur.clearBinds();
            cur.inputBind(bindvars,bindvals);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by position with validation
            Console.WriteLine("INPUT BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1",(Int64)8);
            cur.inputBind("2",(Int64)8);
            cur.inputBind("3",8.5,2,1);
            cur.inputBind("4",8.5,2,1);
            cur.inputBind("5",8.5,2,1);
            cur.inputBind("6",8.5,2,1);
            cur.inputBind("7",(Int16)2008,(Int16)1,(Int16)1,(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int32)(-1),(String)null,false);
            cur.inputBind("8",(Int16)(-1),(Int16)(-1),(Int16)(-1),(Int16)8,(Int16)0,(Int16)0,(Int32)0,(String)null,false);
            cur.inputBind("9","testchar8");
            cur.inputBind("10","testvarchar8");
            cur.inputBind("11",(String)null);
            cur.inputBindBlob("12",System.Text.Encoding.ASCII.GetBytes("testblob8"),(UInt32)9);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by name
            // firebird doesn't support bind by name


            // array of input binds by name
            // firebird doesn't support bind by name


            // input bind by name with validation
            // firebird doesn't support bind by name


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(),(UInt32)12);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName(0),"TESTINTEGER");
            assertEquals(cur.getColumnName(1),"TESTSMALLINT");
            assertEquals(cur.getColumnName(2),"TESTDECIMAL");
            assertEquals(cur.getColumnName(3),"TESTNUMERIC");
            assertEquals(cur.getColumnName(4),"TESTFLOAT");
            assertEquals(cur.getColumnName(5),"TESTDOUBLE");
            assertEquals(cur.getColumnName(6),"TESTDATE");
            assertEquals(cur.getColumnName(7),"TESTTIME");
            assertEquals(cur.getColumnName(8),"TESTCHAR");
            assertEquals(cur.getColumnName(9),"TESTVARCHAR");
            assertEquals(cur.getColumnName(10),"TESTTIMESTAMP");
            assertEquals(cur.getColumnName(11),"TESTBLOB");
            cols=cur.getColumnNames();
            assertEquals(cols[0],"TESTINTEGER");
            assertEquals(cols[1],"TESTSMALLINT");
            assertEquals(cols[2],"TESTDECIMAL");
            assertEquals(cols[3],"TESTNUMERIC");
            assertEquals(cols[4],"TESTFLOAT");
            assertEquals(cols[5],"TESTDOUBLE");
            assertEquals(cols[6],"TESTDATE");
            assertEquals(cols[7],"TESTTIME");
            assertEquals(cols[8],"TESTCHAR");
            assertEquals(cols[9],"TESTVARCHAR");
            assertEquals(cols[10],"TESTTIMESTAMP");
            assertEquals(cols[11],"TESTBLOB");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            assertEquals(cur.getColumnType((UInt32)0),"INTEGER");
            assertEquals(cur.getColumnType("TESTINTEGER"),"INTEGER");
            assertEquals(cur.getColumnType((UInt32)1),"SMALLINT");
            assertEquals(cur.getColumnType("TESTSMALLINT"),"SMALLINT");
            assertEquals(cur.getColumnType((UInt32)2),"DECIMAL");
            assertEquals(cur.getColumnType("TESTDECIMAL"),"DECIMAL");
            assertEquals(cur.getColumnType((UInt32)3),"NUMERIC");
            assertEquals(cur.getColumnType("TESTNUMERIC"),"NUMERIC");
            assertEquals(cur.getColumnType((UInt32)4),"FLOAT");
            assertEquals(cur.getColumnType("TESTFLOAT"),"FLOAT");
            assertEquals(cur.getColumnType((UInt32)5),"DOUBLE PRECISION");
            assertEquals(cur.getColumnType("TESTDOUBLE"),"DOUBLE PRECISION");
            assertEquals(cur.getColumnType((UInt32)6),"DATE");
            assertEquals(cur.getColumnType("TESTDATE"),"DATE");
            assertEquals(cur.getColumnType((UInt32)7),"TIME");
            assertEquals(cur.getColumnType("TESTTIME"),"TIME");
            assertEquals(cur.getColumnType((UInt32)8),"CHAR");
            assertEquals(cur.getColumnType("TESTCHAR"),"CHAR");
            assertEquals(cur.getColumnType((UInt32)9),"VARCHAR");
            assertEquals(cur.getColumnType("TESTVARCHAR"),"VARCHAR");
            assertEquals(cur.getColumnType((UInt32)10),"TIMESTAMP");
            assertEquals(cur.getColumnType("TESTTIMESTAMP"),"TIMESTAMP");
            assertEquals(cur.getColumnType((UInt32)11),"BLOB");
            assertEquals(cur.getColumnType("TESTBLOB"),"BLOB");
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0),(UInt32)4);
            assertEquals(cur.getColumnLength("TESTINTEGER"),(UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)1),(UInt32)2);
            assertEquals(cur.getColumnLength("TESTSMALLINT"),(UInt32)2);
            assertEquals(cur.getColumnLength((UInt32)2),(UInt32)8);
            assertEquals(cur.getColumnLength("TESTDECIMAL"),(UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)3),(UInt32)8);
            assertEquals(cur.getColumnLength("TESTNUMERIC"),(UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)4),(UInt32)4);
            assertEquals(cur.getColumnLength("TESTFLOAT"),(UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)5),(UInt32)8);
            assertEquals(cur.getColumnLength("TESTDOUBLE"),(UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)6),(UInt32)4);
            assertEquals(cur.getColumnLength("TESTDATE"),(UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)7),(UInt32)4);
            assertEquals(cur.getColumnLength("TESTTIME"),(UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)8),(UInt32)50);
            assertEquals(cur.getColumnLength("TESTCHAR"),(UInt32)50);
            assertEquals(cur.getColumnLength((UInt32)9),(UInt32)50);
            assertEquals(cur.getColumnLength("TESTVARCHAR"),(UInt32)50);
            assertEquals(cur.getColumnLength((UInt32)10),(UInt32)8);
            assertEquals(cur.getColumnLength("TESTTIMESTAMP"),(UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)11),(UInt32)8);
            assertEquals(cur.getColumnLength("TESTBLOB"),(UInt32)8);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0),(UInt32)1);
            assertEquals(cur.getLongest("TESTINTEGER"),(UInt32)1);
            assertEquals(cur.getLongest((UInt32)1),(UInt32)1);
            assertEquals(cur.getLongest("TESTSMALLINT"),(UInt32)1);
            assertEquals(cur.getLongest((UInt32)2),(UInt32)4);
            assertEquals(cur.getLongest("TESTDECIMAL"),(UInt32)4);
            assertEquals(cur.getLongest((UInt32)3),(UInt32)4);
            assertEquals(cur.getLongest("TESTNUMERIC"),(UInt32)4);
            assertEquals(cur.getLongest((UInt32)4),(UInt32)6);
            assertEquals(cur.getLongest("TESTFLOAT"),(UInt32)6);
            assertEquals(cur.getLongest((UInt32)5),(UInt32)6);
            assertEquals(cur.getLongest("TESTDOUBLE"),(UInt32)6);
            assertEquals(cur.getLongest((UInt32)6),(UInt32)10);
            assertEquals(cur.getLongest("TESTDATE"),(UInt32)10);
            assertEquals(cur.getLongest((UInt32)7),(UInt32)8);
            assertEquals(cur.getLongest("TESTTIME"),(UInt32)8);
            assertEquals(cur.getLongest((UInt32)8),(UInt32)50);
            assertEquals(cur.getLongest("TESTCHAR"),(UInt32)50);
            assertEquals(cur.getLongest((UInt32)9),(UInt32)12);
            assertEquals(cur.getLongest("TESTVARCHAR"),(UInt32)12);
            assertEquals(cur.getLongest((UInt32)10),(UInt32)0);
            assertEquals(cur.getLongest("TESTTIMESTAMP"),(UInt32)0);
            assertEquals(cur.getLongest((UInt32)11),(UInt32)9);
            assertEquals(cur.getLongest("TESTBLOB"),(UInt32)9);
            Console.WriteLine("");


            // row count
            Console.WriteLine("ROW COUNT: ");
            assertEquals(cur.rowCount(),(UInt64)8);
            Console.WriteLine("");


            // total rows
            Console.WriteLine("TOTAL ROWS: ");
            assertEquals(cur.totalRows(),(UInt64)0);
            Console.WriteLine("");


            // first row index
            Console.WriteLine("FIRST ROW INDEX: ");
            assertEquals(cur.firstRowIndex(),(UInt64)0);
            Console.WriteLine("");


            // end of result set
            Console.WriteLine("END OF RESULT SET: ");
            assertTrue(cur.endOfResultSet());
            Console.WriteLine("");


            // fields by index
            Console.WriteLine("FIELDS BY INDEX: ");
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),"1");
            assertEquals(cur.getField((UInt64)0,(UInt32)2),"1.50");
            assertEquals(cur.getField((UInt64)0,(UInt32)3),"1.50");
            assertEquals(cur.getField((UInt64)0,(UInt32)4),"1.5000");
            assertEquals(cur.getField((UInt64)0,(UInt32)5),"1.5000");
            assertEquals(cur.getField((UInt64)0,(UInt32)6),"2001-01-01");
            assertEquals(cur.getField((UInt64)0,(UInt32)7),"01:00:00");
            assertEquals(cur.getField((UInt64)0,(UInt32)8),"testchar1                                         ");
            assertEquals(cur.getField((UInt64)0,(UInt32)9),"testvarchar1");
            assertEquals(cur.getField((UInt64)0,(UInt32)11),"testblob1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            assertEquals(cur.getField((UInt64)7,(UInt32)1),"8");
            assertEquals(cur.getField((UInt64)7,(UInt32)2),"8.50");
            assertEquals(cur.getField((UInt64)7,(UInt32)3),"8.50");
            assertEquals(cur.getField((UInt64)7,(UInt32)4),"8.5000");
            assertEquals(cur.getField((UInt64)7,(UInt32)5),"8.5000");
            assertEquals(cur.getField((UInt64)7,(UInt32)6),"2008-01-01");
            assertEquals(cur.getField((UInt64)7,(UInt32)7),"08:00:00");
            assertEquals(cur.getField((UInt64)7,(UInt32)8),"testchar8                                         ");
            assertEquals(cur.getField((UInt64)7,(UInt32)9),"testvarchar8");
            assertEquals(cur.getField((UInt64)7,(UInt32)11),"testblob8");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)0),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)1),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)2),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)3),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)4),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)5),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)6),(UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)7),(UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)8),(UInt32)50);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)9),(UInt32)12);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)0),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)1),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)2),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)3),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)4),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)5),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)6),(UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)7),(UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)8),(UInt32)50);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)9),(UInt32)12);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0,"TESTINTEGER"),"1");
            assertEquals(cur.getField((UInt64)0,"TESTSMALLINT"),"1");
            assertEquals(cur.getField((UInt64)0,"TESTDECIMAL"),"1.50");
            assertEquals(cur.getField((UInt64)0,"TESTNUMERIC"),"1.50");
            assertEquals(cur.getField((UInt64)0,"TESTFLOAT"),"1.5000");
            assertEquals(cur.getField((UInt64)0,"TESTDOUBLE"),"1.5000");
            assertEquals(cur.getField((UInt64)0,"TESTDATE"),"2001-01-01");
            assertEquals(cur.getField((UInt64)0,"TESTTIME"),"01:00:00");
            assertEquals(cur.getField((UInt64)0,"TESTCHAR"),"testchar1                                         ");
            assertEquals(cur.getField((UInt64)0,"TESTVARCHAR"),"testvarchar1");
            assertEquals(cur.getField((UInt64)0,"TESTBLOB"),"testblob1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7,"TESTINTEGER"),"8");
            assertEquals(cur.getField((UInt64)7,"TESTSMALLINT"),"8");
            assertEquals(cur.getField((UInt64)7,"TESTDECIMAL"),"8.50");
            assertEquals(cur.getField((UInt64)7,"TESTNUMERIC"),"8.50");
            assertEquals(cur.getField((UInt64)7,"TESTFLOAT"),"8.5000");
            assertEquals(cur.getField((UInt64)7,"TESTDOUBLE"),"8.5000");
            assertEquals(cur.getField((UInt64)7,"TESTDATE"),"2008-01-01");
            assertEquals(cur.getField((UInt64)7,"TESTTIME"),"08:00:00");
            assertEquals(cur.getField((UInt64)7,"TESTCHAR"),"testchar8                                         ");
            assertEquals(cur.getField((UInt64)7,"TESTVARCHAR"),"testvarchar8");
            assertEquals(cur.getField((UInt64)7,"TESTBLOB"),"testblob8");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0,"TESTINTEGER"),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTSMALLINT"),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTDECIMAL"),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTNUMERIC"),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTFLOAT"),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTDOUBLE"),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTDATE"),(UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTTIME"),(UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTCHAR"),(UInt32)50);
            assertEquals(cur.getFieldLength((UInt64)0,"TESTVARCHAR"),(UInt32)12);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7,"TESTINTEGER"),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTSMALLINT"),(UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTDECIMAL"),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTNUMERIC"),(UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTFLOAT"),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTDOUBLE"),(UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTDATE"),(UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTTIME"),(UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTCHAR"),(UInt32)50);
            assertEquals(cur.getFieldLength((UInt64)7,"TESTVARCHAR"),(UInt32)12);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields=cur.getRow((UInt64)0);
            assertEquals(fields[0],"1");
            assertEquals(fields[1],"1");
            assertEquals(fields[2],"1.50");
            assertEquals(fields[3],"1.50");
            assertEquals(fields[4],"1.5000");
            assertEquals(fields[5],"1.5000");
            assertEquals(fields[6],"2001-01-01");
            assertEquals(fields[7],"01:00:00");
            assertEquals(fields[8],"testchar1                                         ");
            assertEquals(fields[9],"testvarchar1");
            assertEquals(fields[11],"testblob1");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens=cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0],(UInt32)1);
            assertEquals(fieldlens[1],(UInt32)1);
            assertEquals(fieldlens[2],(UInt32)4);
            assertEquals(fieldlens[3],(UInt32)4);
            assertEquals(fieldlens[4],(UInt32)6);
            assertEquals(fieldlens[5],(UInt32)6);
            assertEquals(fieldlens[6],(UInt32)10);
            assertEquals(fieldlens[7],(UInt32)8);
            assertEquals(fieldlens[8],(UInt32)50);
            assertEquals(fieldlens[9],(UInt32)12);
            Console.WriteLine("");


            // result set buffer size
            Console.WriteLine("RESULT SET BUFFER SIZE: ");
            assertEquals(cur.getResultSetBufferSize(),(UInt64)0);
            cur.setResultSetBufferSize(2);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            assertEquals(cur.getResultSetBufferSize(),(UInt64)2);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)0);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)2);
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)1,(UInt32)0),"2");
            assertEquals(cur.getField((UInt64)2,(UInt32)0),"3");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)2);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)4);
            assertEquals(cur.getField((UInt64)6,(UInt32)0),"7");
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)8);
            assertEquals(cur.getField((UInt64)8,(UInt32)0),(String)null);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)8);
            assertTrue(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)8);
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // dont get column info
            Console.WriteLine("DONT GET COLUMN INFO: ");
            cur.dontGetColumnInfo();
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            assertEquals(cur.getColumnName(0),(String)null);
            assertEquals(cur.getColumnLength((UInt32)0),(UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0),(String)null);
            cur.getColumnInfo();
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            assertEquals(cur.getColumnName(0),"TESTINTEGER");
            assertEquals(cur.getColumnLength((UInt32)0),(UInt32)4);
            assertEquals(cur.getColumnType((UInt32)0),"INTEGER");
            Console.WriteLine("");


            // suspended session
            Console.WriteLine("SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port=con.getConnectionPort();
            socket=con.getConnectionSocket();
            assertTrue(con.resumeSession(port,socket));
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)1,(UInt32)0),"2");
            assertEquals(cur.getField((UInt64)2,(UInt32)0),"3");
            assertEquals(cur.getField((UInt64)3,(UInt32)0),"4");
            assertEquals(cur.getField((UInt64)4,(UInt32)0),"5");
            assertEquals(cur.getField((UInt64)5,(UInt32)0),"6");
            assertEquals(cur.getField((UInt64)6,(UInt32)0),"7");
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            Console.WriteLine("");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port=con.getConnectionPort();
            socket=con.getConnectionSocket();
            assertTrue(con.resumeSession(port,socket));
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)1,(UInt32)0),"2");
            assertEquals(cur.getField((UInt64)2,(UInt32)0),"3");
            assertEquals(cur.getField((UInt64)3,(UInt32)0),"4");
            assertEquals(cur.getField((UInt64)4,(UInt32)0),"5");
            assertEquals(cur.getField((UInt64)5,(UInt32)0),"6");
            assertEquals(cur.getField((UInt64)6,(UInt32)0),"7");
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            Console.WriteLine("");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port=con.getConnectionPort();
            socket=con.getConnectionSocket();
            assertTrue(con.resumeSession(port,socket));
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)1,(UInt32)0),"2");
            assertEquals(cur.getField((UInt64)2,(UInt32)0),"3");
            assertEquals(cur.getField((UInt64)3,(UInt32)0),"4");
            assertEquals(cur.getField((UInt64)4,(UInt32)0),"5");
            assertEquals(cur.getField((UInt64)5,(UInt32)0),"6");
            assertEquals(cur.getField((UInt64)6,(UInt32)0),"7");
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            Console.WriteLine("");


            // suspended result set
            Console.WriteLine("SUSPENDED RESULT SET: ");
            cur.setResultSetBufferSize(2);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            assertEquals(cur.getField((UInt64)2,(UInt32)0),"3");
            id=cur.getResultSetId();
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port=con.getConnectionPort();
            socket=con.getConnectionSocket();
            assertTrue(con.resumeSession(port,socket));
            assertTrue(cur.resumeResultSet(id));
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)4);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)6);
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)8);
            assertEquals(cur.getField((UInt64)8,(UInt32)0),(String)null);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)8);
            assertTrue(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)8);
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // cached result set
            Console.WriteLine("CACHED RESULT SET: ");
            cur.cacheToFile("cachefile1-firebird");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            filename=cur.getCacheFileName();
            assertEquals(filename,"cachefile1-firebird");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            Console.WriteLine("");


            // column count for cached result set
            Console.WriteLine("COLUMN COUNT FOR CACHED RESULT SET: ");
            assertEquals(cur.colCount(),(UInt32)12);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName(0),"TESTINTEGER");
            assertEquals(cur.getColumnName(1),"TESTSMALLINT");
            assertEquals(cur.getColumnName(2),"TESTDECIMAL");
            assertEquals(cur.getColumnName(3),"TESTNUMERIC");
            assertEquals(cur.getColumnName(4),"TESTFLOAT");
            assertEquals(cur.getColumnName(5),"TESTDOUBLE");
            assertEquals(cur.getColumnName(6),"TESTDATE");
            assertEquals(cur.getColumnName(7),"TESTTIME");
            assertEquals(cur.getColumnName(8),"TESTCHAR");
            assertEquals(cur.getColumnName(9),"TESTVARCHAR");
            assertEquals(cur.getColumnName(10),"TESTTIMESTAMP");
            assertEquals(cur.getColumnName(11),"TESTBLOB");
            cols=cur.getColumnNames();
            assertEquals(cols[0],"TESTINTEGER");
            assertEquals(cols[1],"TESTSMALLINT");
            assertEquals(cols[2],"TESTDECIMAL");
            assertEquals(cols[3],"TESTNUMERIC");
            assertEquals(cols[4],"TESTFLOAT");
            assertEquals(cols[5],"TESTDOUBLE");
            assertEquals(cols[6],"TESTDATE");
            assertEquals(cols[7],"TESTTIME");
            assertEquals(cols[8],"TESTCHAR");
            assertEquals(cols[9],"TESTVARCHAR");
            assertEquals(cols[10],"TESTTIMESTAMP");
            assertEquals(cols[11],"TESTBLOB");
            Console.WriteLine("");


            // cached result set with result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile1-firebird");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            filename=cur.getCacheFileName();
            assertEquals(filename,"cachefile1-firebird");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            assertEquals(cur.getField((UInt64)8,(UInt32)0),(String)null);
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // from one cache file to another
            Console.WriteLine("FROM ONE CACHE FILE TO ANOTHER: ");
            cur.cacheToFile("cachefile2-firebird");
            assertTrue(cur.openCachedResultSet("cachefile1-firebird"));
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet("cachefile2-firebird"));
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            assertEquals(cur.getField((UInt64)8,(UInt32)0),(String)null);
            Console.WriteLine("");


            // from one cache file to another with result set buffer size
            Console.WriteLine("FROM ONE CACHE FILE TO ANOTHER " +
                        "WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile2-firebird");
            assertTrue(cur.openCachedResultSet("cachefile1-firebird"));
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet("cachefile2-firebird"));
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            assertEquals(cur.getField((UInt64)8,(UInt32)0),(String)null);
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // cached result set with suspend and result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH SUSPEND " +
                        "AND RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile1-firebird");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            assertEquals(cur.getField((UInt64)2,(UInt32)0),"3");
            filename=cur.getCacheFileName();
            assertEquals(filename,"cachefile1-firebird");
            id=cur.getResultSetId();
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port=con.getConnectionPort();
            socket=con.getConnectionSocket();
            Console.WriteLine("");
            assertTrue(con.resumeSession(port,socket));
            assertTrue(cur.resumeCachedResultSet(id,filename));
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)4);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)6);
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)8);
            assertEquals(cur.getField((UInt64)8,(UInt32)0),(String)null);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(),(UInt64)8);
            assertTrue(cur.endOfResultSet());
            assertEquals(cur.rowCount(),(UInt64)8);
            cur.cacheOff();
            Console.WriteLine("");
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            assertEquals(cur.getField((UInt64)8,(UInt32)0),(String)null);
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // finished suspended session
            Console.WriteLine("FINISHED SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testinteger "));
            assertEquals(cur.getField((UInt64)4,(UInt32)0),"5");
            assertEquals(cur.getField((UInt64)5,(UInt32)0),"6");
            assertEquals(cur.getField((UInt64)6,(UInt32)0),"7");
            assertEquals(cur.getField((UInt64)7,(UInt32)0),"8");
            id=cur.getResultSetId();
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port=con.getConnectionPort();
            socket=con.getConnectionSocket();
            assertTrue(con.resumeSession(port,socket));
            assertTrue(cur.resumeResultSet(id));
            assertEquals(cur.getField((UInt64)4,(UInt32)0),(String)null);
            assertEquals(cur.getField((UInt64)5,(UInt32)0),(String)null);
            assertEquals(cur.getField((UInt64)6,(UInt32)0),(String)null);
            assertEquals(cur.getField((UInt64)7,(UInt32)0),(String)null);
            Console.WriteLine("");


            // nested selects
            Console.WriteLine("NESTED SELECTS: ");
            cur.setResultSetBufferSize(1);
            assertTrue(cur.sendQuery("select * from testtable"));
            secondcur=new SQLRCursor(con);
            secondcur.setResultSetBufferSize(1);
            UInt64 nestedrows = 0;
            for (UInt32 i=0; cur.getRow((UInt64)i)!=null; i++) {
                assertTrue(secondcur.sendQuery("select * from testtable"));
                nestedrows++;
            }
            // the nested selects must not disturb the outer result set
            assertEquals(nestedrows, cur.rowCount());
            secondcur.closeResultSet();
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // reset transaction state
            Console.WriteLine("RESET TRANSACTION STATE: ");
            assertTrue(con.commit());
            assertEquals(con.getTransactionModel(),"implicit");
            assertFalse(con.getAutoCommit());
            Console.WriteLine("");


            // transaction behavior - implicit
            Console.WriteLine("TRANSACTION BEHAVIOR - implicit: ");
            assertTrue(con.setTransactionModel("implicit"));
            assertEquals(con.getTransactionModel(),"implicit");
            // truncate testtable so this section starts with it empty;
            // firebird DDL on the table here would otherwise hit cursor-state
            // issues at the next commit, so we reuse the existing schema and
            // just write to one column (testinteger)
            assertTrue(cur.sendQuery("delete from testtable"));
            // commit so the truncation is visible to the second connection
            // (the commit implicitly starts a new tx)
            assertTrue(con.commit());
            secondcon=new SQLRConnection("sqlrelay",9009,"/tmp/firebird.socket",
                                    "testuser","testpassword",0,1);
            secondcur=new SQLRCursor(secondcon);
            // session is in a transaction; insert is not visible until commit
            assertTrue(con.getInTransaction());
            assertFalse(con.getAutoCommit());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"0");
            // commit makes it visible, and implicitly starts a new transaction
            assertTrue(con.commit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // rollback discards, and implicitly starts a new transaction
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
            assertTrue(con.rollback());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // autoCommitOn takes effect immediately
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertFalse(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"2");
            // autoCommitOff takes effect immediately
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            secondcur.closeResultSet();
            Console.WriteLine("");


            // transaction behavior - explicit
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit: ");
            assertTrue(con.setTransactionModel("explicit"));
            assertEquals(con.getTransactionModel(),"explicit");
            // truncate testtable so this section starts with it empty (delete
            // autocommits here since explicit-model defaults to autocommit-on)
            assertTrue(cur.sendQuery("delete from testtable"));
            // begin starts a new transaction; insert is not visible until commit
            assertTrue(con.begin());
            assertTrue(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"0");
            // commit makes it visible; no new transaction is started
            assertTrue(con.commit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // begin, insert, rollback discards; no new transaction is started
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
            assertTrue(con.rollback());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // autoCommitOn takes effect immediately
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"2");
            // autoCommitOff takes effect immediately
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            secondcur.closeResultSet();
            Console.WriteLine("");


            // transaction behavior - explicit-deferred
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit-deferred: ");
            assertTrue(con.setTransactionModel("explicit-deferred"));
            assertEquals(con.getTransactionModel(),"explicit-deferred");
            // switch to autocommit-on so the begin/commit cycles below
            // bracket explicit transactions (autocommit-off semantics are
            // exercised at the end of this block)
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            // truncate testtable so this section starts with it empty
            assertTrue(cur.sendQuery("delete from testtable"));
            // begin starts a transaction; commit makes it visible
            assertTrue(con.begin());
            assertTrue(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
            assertTrue(con.commit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // begin, insert, rollback discards
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // during a transaction started by begin(), autoCommitOn is a
            // no-op: the autocommit setting takes effect after the user
            // explicitly commits/rollbacks the tx (mysql-native semantic)
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
            assertTrue(con.autoCommitOn());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // explicit commit ends the tx; autocommit-on now takes effect
            assertTrue(con.commit());
            assertTrue(con.getAutoCommit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"2");
            // autocommit is on; subsequent inserts are visible immediately
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (4)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"3");
            // autoCommitOff takes effect immediately when not in a transaction
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            // autocommit-off persists across commit/rollback; each commit or
            // rollback ends the current implicit tx and a new one starts for
            // the next statement
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (5)"));
            assertTrue(con.commit());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"4");
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (6)"));
            assertTrue(con.rollback());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"4");
            // autoCommitOff during a transaction changes the variable
            // immediately but the in-flight tx continues; only after the
            // next explicit commit/rollback does the new autocommit-off
            // setting drop us into a new implicit tx (mysql-asymmetric
            // semantic)
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (7)"));
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"4");
            assertTrue(con.commit());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"5");
            secondcur.closeResultSet();
            Console.WriteLine("");


            // transaction behavior - explicit-error
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit-error: ");
            assertTrue(con.setTransactionModel("explicit-error"));
            assertEquals(con.getTransactionModel(),"explicit-error");
            // truncate testtable so this section starts with it empty
            assertTrue(cur.sendQuery("delete from testtable"));
            // begin, insert, commit
            assertTrue(con.begin());
            assertTrue(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
            assertTrue(con.commit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // begin, insert, rollback
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // while in a transaction, autoCommitOn/Off throw an error
            assertTrue(con.begin());
            assertFalse(con.autoCommitOn());
            assertFalse(con.autoCommitOff());
            assertTrue(con.commit());
            // outside of a transaction, autoCommitOn takes effect immediately
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (3)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"2");
            // autoCommitOff takes effect immediately
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            secondcur.closeResultSet();
            Console.WriteLine("");


            // transaction behavior - none
            Console.WriteLine("TRANSACTION BEHAVIOR - none: ");
            assertTrue(con.setTransactionModel("none"));
            assertEquals(con.getTransactionModel(),"none");
            // truncate testtable so this section starts with it empty
            assertTrue(cur.sendQuery("delete from testtable"));
            // no transactions; everything is visible immediately
            assertTrue(con.getAutoCommit());
            assertFalse(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"1");
            // commit and rollback are no-ops
            assertTrue(con.commit());
            assertTrue(cur.sendQuery("insert into testtable (testinteger) values (2)"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0,(UInt32)0),"2");
            // autocommit is always on; autoCommitOff is an error
            assertFalse(con.autoCommitOff());
            assertTrue(con.getAutoCommit());
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            secondcur.closeResultSet();
            Console.WriteLine("");


            // reset transaction behavior
            Console.WriteLine("RESET TRANSACTION BEHAVIOR: ");
            assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
            assertEquals(con.getTransactionModel(),"implicit");
            assertFalse(con.getAutoCommit());
            Console.WriteLine("");


            // individual substitutions
            Console.WriteLine("INDIVIDUAL SUBSTITUTIONS: ");
            cur.prepareQuery("select $(var1),'$(var2)',$(var3) from rdb$database");
            cur.substitution("var1",(Int64)1);
            cur.substitution("var2","hello");
            cur.substitution("var3",10.5556,6,4);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),"hello");
            assertEquals(cur.getField((UInt64)0,(UInt32)2),"10.5556");
            Console.WriteLine("");


            // array substitutions
            Console.WriteLine("ARRAY SUBSTITUTIONS: ");
            cur.prepareQuery(
                "select " +
                "	'$(var1)', " +
                "	'$(var2)', " +
                "	'$(var3)' " +
                "from " +
                "	rdb$database ");
            cur.substitutions(subvars,subvalstrings);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"hi");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),"hello");
            assertEquals(cur.getField((UInt64)0,(UInt32)2),"bye");
            Console.WriteLine("");
            cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
            cur.substitution(subvars,subvallongs);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),"2");
            assertEquals(cur.getField((UInt64)0,(UInt32)2),"3");
            Console.WriteLine("");
            cur.prepareQuery("select $(var1),$(var2),$(var3) from rdb$database");
            cur.substitution(subvars,subvaldoubles,precs,scales);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"10.55");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),"10.556");
            assertEquals(cur.getField((UInt64)0,(UInt32)2),"10.5556");
            Console.WriteLine("");


            // nulls as nulls
            Console.WriteLine("NULLS AS NULLS: ");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery("select 1,NULL,NULL from rdb$database"));
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),(String)null);
            assertEquals(cur.getField((UInt64)0,(UInt32)2),(String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("select 1,NULL,NULL from rdb$database"));
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),"");
            assertEquals(cur.getField((UInt64)0,(UInt32)2),"");
            Console.WriteLine("");


            // null and empty lobs
            Console.WriteLine("NULL AND EMPTY LOBS: ");
            cur.getNullsAsNulls();
            cur.sendQuery("delete from testtable1");
            cur.prepareQuery("insert into testtable1 values (?)");
            cur.inputBindBlob("1",System.Text.Encoding.ASCII.GetBytes(""),(UInt32)0);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testblob from testtable1");
            assertEquals(cur.getField((UInt64)0,"TESTBLOB"),"");
            cur.sendQuery("delete from testtable1");
            cur.prepareQuery("insert into testtable1 values (?)");
            cur.inputBindBlob("1",(Byte[])null,(UInt32)0);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testblob from testtable1");
            assertEquals(cur.getField((UInt64)0,"TESTBLOB"),(String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("delete from testtable1"));
            Console.WriteLine("");


            // long lobs
            Console.WriteLine("LONG LOBS: ");
            cur.sendQuery("delete from testtable1");
            cur.prepareQuery("insert into testtable1 values (?)");
            largebuffer=new String('C',LARGE_BUFFER_LENGTH);
            cur.inputBindClob("1",largebuffer,(UInt32)LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testblob from testtable1");
            assertEquals(cur.getFieldLength((UInt64)0,"TESTBLOB"),(UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0,"TESTBLOB"),largebuffer);
            assertTrue(cur.sendQuery("delete from testtable1"));
            Console.WriteLine("");


            // output bind by position
            Console.WriteLine("OUTPUT BIND BY POSITION: ");
            cur.getNullsAsNulls();
            cur.prepareQuery("execute procedure testproc ?, ?, ?, ?");
            cur.inputBind("1",(Int64)1);
            cur.inputBind("2",1.5,2,1);
            cur.inputBind("3","hello");
            cur.inputBindBlob("4",System.Text.Encoding.ASCII.GetBytes("blob"),(UInt32)4);
            cur.defineOutputBindInteger("1");
            cur.defineOutputBindDouble("2");
            cur.defineOutputBindString("3",20);
            cur.defineOutputBindBlob("4");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("1"),(Int64)1);
            double d=cur.getOutputBindDouble("2");
            assertEquals(d,1.5);
            assertEquals(cur.getOutputBindString("3"),"hello               ");
            assertEquals(cur.getOutputBindBlob("4"),System.Text.Encoding.ASCII.GetBytes("blob"));
            cur.getNullsAsEmptyStrings();
            Console.WriteLine("");


            // output bind by name
            // firebird doesn't support bind by name


            // output bind by name with validation
            // firebird doesn't support bind by name


            // lob output bind
            Console.WriteLine("LOB OUTPUT BIND: ");
            cur.prepareQuery("execute procedure testproc1 ?");
            cur.inputBindBlob("1",System.Text.Encoding.ASCII.GetBytes("hello"),(UInt32)5);
            cur.defineOutputBindBlob("1");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindBlob("1"),System.Text.Encoding.ASCII.GetBytes("hello"),5);
            assertEquals(cur.getOutputBindLength("1"),(UInt32)5);
            Console.WriteLine("");


            // long output bind
            Console.WriteLine("LONG OUTPUT BIND: ");
            largebuffer=new String('C',LARGE_BUFFER_LENGTH);
            cur.prepareQuery("execute procedure testproc1 ?");
            cur.inputBindBlob("1",System.Text.Encoding.ASCII.GetBytes(largebuffer),(UInt32)LARGE_BUFFER_LENGTH);
            cur.defineOutputBindBlob("1");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindLength("1"),(UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getOutputBindBlob("1"),System.Text.Encoding.ASCII.GetBytes(largebuffer),
                                LARGE_BUFFER_LENGTH);
            Console.WriteLine("");


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.prepareQuery("select cast(? as integer) from rdb$database");
            cur.inputBind("1",(Int64)(-1));
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"-1");
            Console.WriteLine("");


            // bind validation
            // firebird doesn't support bind by name


            // rebinding
            Console.WriteLine("REBINDING: ");
            cur.prepareQuery("execute procedure testproc ?, ?, ?, ?");
            cur.inputBind("1",(Int64)1);
            cur.inputBind("2",1.5,2,1);
            cur.inputBind("3","hello");
            cur.inputBindBlob("4",System.Text.Encoding.ASCII.GetBytes("blob"),(UInt32)4);
            cur.defineOutputBindInteger("1");
            cur.defineOutputBindDouble("2");
            cur.defineOutputBindString("3",20);
            cur.defineOutputBindBlob("4");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("1"),(Int64)1);
            cur.inputBind("1",(Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("1"),(Int64)2);
            cur.inputBind("1",(Int64)3);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("1"),(Int64)3);
            Console.WriteLine("");


            // reexecute
            Console.WriteLine("REEXECUTE: ");
            cur.prepareQuery("select 1 from rdb$database");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(),(UInt64)1);
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(),(UInt64)1);
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            Console.WriteLine("");
            cur.prepareQuery("select cast(? as int) from rdb$database");
            cur.inputBind("1",(Int64)1);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(),(UInt64)1);
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(),(UInt64)1);
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            Console.WriteLine("");
            cur.inputBind("1",(Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(),(UInt64)1);
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"2");
            Console.WriteLine("");


            // stored procedure returning no value
            Console.WriteLine("STORED PROCEDURE RETURNING NO VALUE: ");
            cur.prepareQuery(
                "execute block (in1 int = ?, " +
                "	in2 double precision = ?, " +
                "	in3 varchar(20) = ?) " +
                "as " +
                "begin " +
                "end");
            cur.inputBind("1",(Int64)1);
            cur.inputBind("2",1.5,2,1);
            cur.inputBind("3","hello");
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // stored procedure returning single value
            Console.WriteLine("STORED PROCEDURE RETURNING SINGLE VALUE: ");
            cur.prepareQuery(
                "execute block (in1 int = ?, " +
                "	in2 double precision = ?, " +
                "	in3 varchar(20) = ?) " +
                "returns (out1 int) " +
                "as " +
                "begin " +
                "	out1 = in1; " +
                "	suspend; " +
                "end");
            cur.inputBind("1",(Int64)1);
            cur.inputBind("2",1.5,2,1);
            cur.inputBind("3","hello");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            Console.WriteLine("");


            // stored procedure returning multiple values
            Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
            cur.prepareQuery(
                "execute block (in1 int = ?, " +
                "	in2 double precision = ?, " +
                "	in3 varchar(20) = ?) " +
                "returns (out1 int, " +
                "	out2 double precision, " +
                "	out3 varchar(20)) " +
                "as " +
                "begin " +
                "	out1 = in1; " +
                "	out2 = in2; " +
                "	out3 = in3; " +
                "	suspend; " +
                "end");
            cur.inputBind("1",(Int64)1);
            cur.inputBind("2",1.5,2,1);
            cur.inputBind("3","hello");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"1");
            assertEquals(cur.getField((UInt64)0,(UInt32)1),"1.5000");
            assertEquals(cur.getField((UInt64)0,(UInt32)2),"hello");
            Console.WriteLine("");


            // stored procedure returning result set
            Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
            cur.prepareQuery(
                "execute block " +
                "returns (out1 int) " +
                "as " +
                "declare i int; " +
                "begin " +
                "	i = 1; " +
                "	while (i <= 8) do " +
                "	begin " +
                "		out1 = i; " +
                "		suspend; " +
                "		i = i + 1; " +
                "	end " +
                "end");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(),(UInt64)8);
            Console.WriteLine("");


            // temporary tables
            // firebird supports temporary tables, but we're omitting this for now


            // encoded binary data
            // firebird doesn't support encoded binary data


            // quotes
            Console.WriteLine("QUOTES: ");
            cur.sendQuery("delete from table testtable1");
            assertTrue(cur.sendQuery(
                    "insert into testtable1 values ('''''')"));
            assertTrue(cur.sendQuery("select testblob from testtable1"));
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)0),(UInt32)2);
            assertEquals(cur.getField((UInt64)0,(UInt32)0),"''");
            assertTrue(cur.sendQuery("delete from testtable1"));
            Console.WriteLine("");


            // last insert id
            // firebird doesn't support auto-increment


            // database is schema
            Console.WriteLine("DATABASE IS SCHEMA: ");
            assertFalse(con.getDatabaseIsSchema());
            Console.WriteLine("");


            // catalog list
            Console.WriteLine("CATALOG LIST: ");
            assertTrue(cur.getCatalogList(null));
            assertEquals(cur.getColumnName(0),"Database");
            assertEquals(cur.rowCount(), (UInt64)0);
            Console.WriteLine("");


            // schema list
            Console.WriteLine("SCHEMA LIST: ");
            assertTrue(cur.getSchemaList(null));
            assertEquals(cur.getColumnName(0),"Database");
            // firebird has no schemas
            assertEquals(cur.rowCount(), (UInt64)0);
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName(0),"table_type");
            assertInResultSet(cur, "table_type", "TABLE");
            Console.WriteLine("");


            // table list
            Console.WriteLine("TABLE LIST: ");
            assertTrue(cur.getTableList(null));
            assertInResultSet(cur, "Tables_in_xxx", "TESTTABLE1");
            assertInResultSet(cur, "Tables_in_xxx", "TESTTABLE2");
            assertInResultSet(cur, "Tables_in_xxx", "TESTTABLE3");
            Console.WriteLine("");


            // type info list
            Console.WriteLine("TYPE INFO LIST: ");
            assertTrue(cur.getTypeInfoList("integer"));
            assertEquals(cur.getColumnName(0),"type_name");
            assertEquals(cur.getColumnName(1),"data_type");
            assertEquals(cur.getColumnName(2),"precision");
            assertEquals(cur.getColumnName(3),"literal_prefix");
            assertEquals(cur.getColumnName(4),"literal_suffix");
            assertEquals(cur.getColumnName(5),"create_params");
            assertEquals(cur.getColumnName(6),"nullable");
            assertEquals(cur.getColumnName(7),"case_sensitive");
            assertEquals(cur.getColumnName(8),"searchable");
            assertEquals(cur.getColumnName(9),"unsigned_attribute");
            assertEquals(cur.getColumnName(10),"fixed_prec_scale");
            assertEquals(cur.getColumnName(11),"auto_increment");
            assertEquals(cur.getColumnName(12),"local_type_name");
            assertEquals(cur.getColumnName(13),"minumum_scale");
            assertEquals(cur.getColumnName(14),"maxiumm_scale");
            assertEquals(cur.getColumnName(15),"sql_data_type");
            assertEquals(cur.getColumnName(16),"sql_datetime_sub");
            assertEquals(cur.getColumnName(17),"num_prec_radix");
            assertEquals(cur.getColumnName(18),"interval_precision");
            assertEquals(cur.getField((UInt64)0,"type_name"),"INTEGER");
            assertEquals(cur.getField((UInt64)0,"data_type"),"4");
            assertEquals(cur.getField((UInt64)0,"precision"),"10");
            assertEquals(cur.getField((UInt64)0,"local_type_name"),"INTEGER");
            assertTrue(cur.getTypeInfoList("char"));
            assertEquals(cur.getField((UInt64)0,"type_name"),"CHAR");
            assertEquals(cur.getField((UInt64)0,"data_type"),"1");
            assertEquals(cur.getField((UInt64)0,"precision"),"32767");
            assertEquals(cur.getField((UInt64)0,"local_type_name"),"CHAR");
            assertTrue(cur.getTypeInfoList("varchar"));
            assertEquals(cur.getField((UInt64)0,"type_name"),"VARCHAR");
            assertEquals(cur.getField((UInt64)0,"data_type"),"12");
            assertEquals(cur.getField((UInt64)0,"precision"),"32765");
            assertEquals(cur.getField((UInt64)0,"local_type_name"),"VARCHAR");
            assertTrue(cur.getTypeInfoList("date"));
            assertEquals(cur.getField((UInt64)0,"type_name"),"DATE");
            assertEquals(cur.getField((UInt64)0,"data_type"),"91");
            assertEquals(cur.getField((UInt64)0,"precision"),"10");
            assertEquals(cur.getField((UInt64)0,"local_type_name"),"DATE");
            Console.WriteLine("");


            // column list
            Console.WriteLine("COLUMN LIST: ");
            assertTrue(cur.getColumnList("testtable",null));
            assertEquals(cur.getColumnName(0),"column_name");
            assertEquals(cur.getColumnName(1),"data_type");
            assertEquals(cur.getColumnName(2),"character_maximum_length");
            assertEquals(cur.getColumnName(3),"numeric_precision");
            assertEquals(cur.getColumnName(4),"numeric_scale");
            assertEquals(cur.getColumnName(5),"is_nullable");
            assertEquals(cur.getColumnName(6),"column_key");
            assertEquals(cur.getColumnName(7),"column_default");
            assertEquals(cur.getColumnName(8),"extra");
            assertTrue(cur.getField((UInt64)0,"column_name")=="TESTINTEGER");
            assertTrue(cur.getField((UInt64)1,"column_name")=="TESTSMALLINT");
            assertTrue(cur.getField((UInt64)2,"column_name")=="TESTDECIMAL");
            assertTrue(cur.getField((UInt64)3,"column_name")=="TESTNUMERIC");
            assertTrue(cur.getField((UInt64)4,"column_name")=="TESTFLOAT");
            assertTrue(cur.getField((UInt64)5,"column_name")=="TESTDOUBLE");
            assertTrue(cur.getField((UInt64)6,"column_name")=="TESTDATE");
            assertTrue(cur.getField((UInt64)7,"column_name")=="TESTTIME");
            assertTrue(cur.getField((UInt64)8,"column_name")=="TESTCHAR");
            assertTrue(cur.getField((UInt64)9,"column_name")=="TESTVARCHAR");
            assertTrue(cur.getField((UInt64)10,"column_name")=="TESTTIMESTAMP");
            assertTrue(cur.getField((UInt64)11,"column_name")=="TESTBLOB");
            assertTrue(cur.getField((UInt64)0,"data_type")=="INTEGER");
            assertTrue(cur.getField((UInt64)1,"data_type")=="SMALLINT");
            assertTrue(cur.getField((UInt64)2,"data_type")=="DECIMAL");
            assertTrue(cur.getField((UInt64)3,"data_type")=="NUMERIC");
            assertTrue(cur.getField((UInt64)4,"data_type")=="FLOAT");
            assertTrue(cur.getField((UInt64)5,"data_type")=="DOUBLE PRECISION");
            assertTrue(cur.getField((UInt64)6,"data_type")=="DATE");
            assertTrue(cur.getField((UInt64)7,"data_type")=="TIME");
            assertTrue(cur.getField((UInt64)8,"data_type")=="CHAR");
            assertTrue(cur.getField((UInt64)9,"data_type")=="VARCHAR");
            assertTrue(cur.getField((UInt64)10,"data_type")=="TIMESTAMP");
            assertTrue(cur.getField((UInt64)11,"data_type")=="BLOB SUB_TYPE BINARY");
            Console.WriteLine("");


            // column list - auto_increment, primary key
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            assertTrue(cur.getColumnList("testtable2",null));
            assertEquals(cur.getField((UInt64)0,"extra"), "auto_increment");
            assertEquals(cur.getField((UInt64)0,"column_key"), "PRI");
            assertEquals(cur.getField((UInt64)1,"extra"), "");
            assertEquals(cur.getField((UInt64)1,"column_key"), "");
            Console.WriteLine("");
            assertTrue(cur.getColumnList("testtable3",null));
            assertEquals(cur.getField((UInt64)0,"extra"), "");
            assertEquals(cur.getField((UInt64)0,"column_key"), "PRI");
            Console.WriteLine("");


            // primary keys list
            Console.WriteLine("PRIMARY KEYS LIST: ");
            assertTrue(cur.getPrimaryKeysList("testtable2",null));
            assertEquals(cur.getColumnName(0),"table");
            assertEquals(cur.getColumnName(1),"non_unique");
            assertEquals(cur.getColumnName(2),"key_name");
            assertEquals(cur.getColumnName(3),"seq_in_index");
            assertEquals(cur.getColumnName(4),"column_name");
            assertEquals(cur.getColumnName(5),"collation");
            assertEquals(cur.getColumnName(6),"cardinality");
            assertEquals(cur.getColumnName(7),"sub_part");
            assertEquals(cur.getColumnName(8),"packed");
            assertEquals(cur.getColumnName(9),"null");
            assertEquals(cur.getColumnName(10),"index_type");
            assertEquals(cur.getColumnName(11),"comment");
            assertEquals(cur.getColumnName(12),"index_comment");
            assertEquals(cur.rowCount(),(UInt64)1);
            assertTrue(cur.getField((UInt64)0,"table")=="TESTTABLE2");
            assertEquals(cur.getField((UInt64)0,"seq_in_index"),"1");
            assertTrue(cur.getField((UInt64)0,"column_name")=="COL1");
            assertStartsWith(cur.getField((UInt64)0,"key_name"), "INTEG_");
            Console.WriteLine("");


            // key and index list
            Console.WriteLine("KEY AND INDEX LIST: ");
            assertTrue(cur.getKeyAndIndexList("testtable2",null));
            assertEquals(cur.getColumnName(0),"table");
            assertEquals(cur.getColumnName(1),"non_unique");
            assertEquals(cur.getColumnName(2),"key_name");
            assertEquals(cur.getColumnName(3),"seq_in_index");
            assertEquals(cur.getColumnName(4),"column_name");
            assertEquals(cur.getColumnName(5),"collation");
            assertEquals(cur.getColumnName(6),"cardinality");
            assertEquals(cur.getColumnName(7),"sub_part");
            assertEquals(cur.getColumnName(8),"packed");
            assertEquals(cur.getColumnName(9),"null");
            assertEquals(cur.getColumnName(10),"index_type");
            assertEquals(cur.getColumnName(11),"comment");
            assertEquals(cur.getColumnName(12),"index_comment");
            assertEquals(cur.rowCount(),(UInt64)1);
            assertTrue(cur.getField((UInt64)0,"table")=="TESTTABLE2");
            assertEquals(cur.getField((UInt64)0,"non_unique"),"0");
            assertEquals(cur.getField((UInt64)0,"seq_in_index"),"1");
            assertTrue(cur.getField((UInt64)0,"column_name")=="COL1");
            assertEquals(cur.getField((UInt64)0,"collation"),"A");
            assertEquals(cur.getField((UInt64)0,"index_type"),"3");
            assertStartsWith(cur.getField((UInt64)0,"key_name"), "RDB$PRIMARY");
            Console.WriteLine("");


            // procedure list
            Console.WriteLine("PROCEDURE LIST: ");
            assertTrue(cur.getProcedureList(null));
            assertInResultSet(cur, "routine_name", "TESTPROC");
            assertInResultSet(cur, "routine_name", "TESTPROC1");
            Console.WriteLine("");


            // procedure parameter list
            Console.WriteLine("PROCEDURE PARAMETER LIST: ");
            assertTrue(cur.getProcedureParameterList("testproc",null));
            assertEquals(cur.getColumnName(0),"parameter_name");
            assertEquals(cur.getColumnName(1),"parameter_mode");
            assertEquals(cur.getColumnName(2),"data_type");
            assertEquals(cur.getColumnName(3),"character_maximum_length");
            assertEquals(cur.getColumnName(4),"ordinal_position");
            assertEquals(cur.rowCount(),(UInt64)8);
            assertEquals(cur.getField((UInt64)0,"parameter_name"),"OUT1");
            assertEquals(cur.getField((UInt64)0,"parameter_mode"),"4");
            assertEquals(cur.getField((UInt64)0,"data_type"),"INTEGER");
            assertEquals(cur.getField((UInt64)0,"ordinal_position"),"1");
            assertEquals(cur.getField((UInt64)1,"parameter_name"),"OUT2");
            assertEquals(cur.getField((UInt64)1,"parameter_mode"),"4");
            assertEquals(cur.getField((UInt64)1,"data_type"),"FLOAT");
            assertEquals(cur.getField((UInt64)1,"ordinal_position"),"2");
            assertEquals(cur.getField((UInt64)2,"parameter_name"),"OUT3");
            assertEquals(cur.getField((UInt64)2,"parameter_mode"),"4");
            assertEquals(cur.getField((UInt64)2,"data_type"),"VARCHAR");
            assertEquals(cur.getField((UInt64)2,"ordinal_position"),"3");
            assertEquals(cur.getField((UInt64)3,"parameter_name"),"OUT4");
            assertEquals(cur.getField((UInt64)3,"parameter_mode"),"4");
            assertEquals(cur.getField((UInt64)3,"data_type"),"BLOB SUB_TYPE BINARY");
            assertEquals(cur.getField((UInt64)3,"ordinal_position"),"4");
            assertEquals(cur.getField((UInt64)4,"parameter_name"),"IN1");
            assertEquals(cur.getField((UInt64)4,"parameter_mode"),"1");
            assertEquals(cur.getField((UInt64)4,"data_type"),"INTEGER");
            assertEquals(cur.getField((UInt64)4,"ordinal_position"),"1");
            assertEquals(cur.getField((UInt64)5,"parameter_name"),"IN2");
            assertEquals(cur.getField((UInt64)5,"parameter_mode"),"1");
            assertEquals(cur.getField((UInt64)5,"data_type"),"FLOAT");
            assertEquals(cur.getField((UInt64)5,"ordinal_position"),"2");
            assertEquals(cur.getField((UInt64)6,"parameter_name"),"IN3");
            assertEquals(cur.getField((UInt64)6,"parameter_mode"),"1");
            assertEquals(cur.getField((UInt64)6,"data_type"),"VARCHAR");
            assertEquals(cur.getField((UInt64)6,"ordinal_position"),"3");
            assertEquals(cur.getField((UInt64)7,"parameter_name"),"IN4");
            assertEquals(cur.getField((UInt64)7,"parameter_mode"),"1");
            assertEquals(cur.getField((UInt64)7,"data_type"),"BLOB SUB_TYPE BINARY");
            assertEquals(cur.getField((UInt64)7,"ordinal_position"),"4");
            Console.WriteLine("");


            // invalid queries
            Console.WriteLine("INVALID QUERIES: ");
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable1 " +
                "order by " +
                "	testinteger "));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable1 " +
                "order by " +
                "	testinteger "));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable1 " +
                "order by " +
                "	testinteger "));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable1 " +
                "order by " +
                "	testinteger "));
            Console.WriteLine("");
            assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
            assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
            assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
            assertFalse(cur.sendQuery("insert into testtable1 values (1,2,3,4)"));
            Console.WriteLine("");
            assertFalse(cur.sendQuery("create table testtable"));
            assertFalse(cur.sendQuery("create table testtable"));
            assertFalse(cur.sendQuery("create table testtable"));
            assertFalse(cur.sendQuery("create table testtable"));
            Console.WriteLine("");

            reportTestStatus();

            return status;
        }
    }
}
