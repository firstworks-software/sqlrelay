// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class Db2Test : SQLRTest
    {
        public static int Main(string[] args)
        {
            // hostname
            String hostname = System.Net.Dns.GetHostName();
            int dot = hostname.IndexOf('.');
            if (dot > 0) { hostname = hostname.Substring(0, dot); }

            String[] isolationlevels = new String[] { "CS", "UR", "RS", "RR" };
            String[] bindvars = new String[] { "1", "2", "3", "4", "5", "6",
                                               "7", "8", "9", "10", "11", "12" };
            String[] bindvals = new String[] { "7", "7", "7", "7.7", "7.7", "7.7",
                                               "testchar7", "testvarchar7",
                                               "01/01/2007", "07:00:00",
                                               "testclob7", null };
            String[] cols;
            String[] fields;
            UInt32[] fieldlens;
            String[] subvars = new String[] { "var1", "var2", "var3" };
            Int64[] subvallongs = new Int64[] { 1, 2, 3 };
            String[] subvalstrings = new String[] { "hi", "hello", "bye" };
            Double[] subvaldoubles = new Double[] { 10.55, 10.556, 10.5556 };
            UInt32[] precs = new UInt32[] { 4, 5, 6 };
            UInt32[] scales = new UInt32[] { 2, 3, 4 };
            Int64 numvar;
            String stringvar;
            String nullvar;
            Double floatvar;
            Int16 year = 0;
            Int16 month = 0;
            Int16 day = 0;
            Int16 hour = 0;
            Int16 minute = 0;
            Int16 second = 0;
            Int32 microsecond = 0;
            String tz = null;
            Boolean isnegative = false;
            String clobvar;
            UInt32 clobvarlength;
            Byte[] blobvar;
            UInt32 blobvarlength;
            UInt16 port;
            String socket;
            UInt16 id;
            String filename;
            UInt64 counter = 0;

            int LARGE_BUFFER_LENGTH = 20 * 1024;
            char[] largebuffer = new char[LARGE_BUFFER_LENGTH + 1];


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                    "db2inst1", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "db2");
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
            assertEquals(con.bindFormat(), "?");
            Console.WriteLine("");


            // nextval format
            Console.WriteLine("NEXTVAL FORMAT: ");
            assertEquals(con.nextvalFormat(), "(nextval for %s)");
            Console.WriteLine("");


            // isolation levels
            Console.WriteLine("ISOLATION LEVELS: ");
            for (int ili = 0; ili < isolationlevels.Length; ili++)
            {
                String il = isolationlevels[ili];
                assertTrue(con.setIsolationLevel(il));
                assertEquals(con.getIsolationLevel(), il);
                Console.WriteLine("");
            }
            // reset to the default isolation level
            assertTrue(con.setIsolationLevel(isolationlevels[0]));
            Console.WriteLine("");


            // create testtable
            Console.WriteLine("CREATE TESTTABLE: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testsmallint smallint, " +
                "	testint integer, " +
                "	testbigint bigint, " +
                "	testdecimal decimal(10,2), " +
                "	testreal real, " +
                "	testdouble double, " +
                "	testchar char(40), " +
                "	testvarchar varchar(40), " +
                "	testdate date, " +
                "	testtime time, " +
                "	testtimestamp timestamp, " +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	1, " +
                "	1, " +
                "	1, " +
                "	1.1, " +
                "	1.1, " +
                "	1.1, " +
                "	'testchar1', " +
                "	'testvarchar1', " +
                "	'01/01/2001', " +
                "	'01:00:00', " +
                "	NULL, " +
                "	'testclob1', " +
                "	blob('testblob1'))"));
            Console.WriteLine("");


            // affected rows
            Console.WriteLine("AFFECTED ROWS: ");
            assertEquals(cur.affectedRows(), (UInt64)1);
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
                "	NULL, " +
                "	?, " +
                "	?)");
            assertEquals(cur.countBindVariables(), (UInt16)12);
            cur.inputBind("1", (Int64)2);
            cur.inputBind("2", (Int64)2);
            cur.inputBind("3", (Int64)2);
            cur.inputBind("4", 2.2, (UInt32)4, (UInt32)2);
            cur.inputBind("5", 2.2, (UInt32)4, (UInt32)2);
            cur.inputBind("6", 2.2, (UInt32)4, (UInt32)2);
            cur.inputBind("7", "testchar2");
            cur.inputBind("8", "testvarchar2");
            cur.inputBind("9", (Int16)2002, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("10", (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int16)2, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("11", "testclob2", (UInt32)9);
            cur.inputBindBlob("12", System.Text.Encoding.ASCII.GetBytes("testblob2"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)3);
            cur.inputBind("2", (Int64)3);
            cur.inputBind("3", (Int64)3);
            cur.inputBind("4", 3.3, (UInt32)4, (UInt32)2);
            cur.inputBind("5", 3.3, (UInt32)4, (UInt32)2);
            cur.inputBind("6", 3.3, (UInt32)4, (UInt32)2);
            cur.inputBind("7", "testchar3");
            cur.inputBind("8", "testvarchar3");
            cur.inputBind("9", (Int16)2003, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("10", (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int16)3, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("11", "testclob3", (UInt32)9);
            cur.inputBindBlob("12", System.Text.Encoding.ASCII.GetBytes("testblob3"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)4);
            cur.inputBind("2", (Int64)4);
            cur.inputBind("3", (Int64)4);
            cur.inputBind("4", 4.4, (UInt32)4, (UInt32)2);
            cur.inputBind("5", 4.4, (UInt32)4, (UInt32)2);
            cur.inputBind("6", 4.4, (UInt32)4, (UInt32)2);
            cur.inputBind("7", "testchar4");
            cur.inputBind("8", "testvarchar4");
            cur.inputBind("9", (Int16)2004, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("10", (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int16)4, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("11", "testclob4", (UInt32)9);
            cur.inputBindBlob("12", System.Text.Encoding.ASCII.GetBytes("testblob4"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)5);
            cur.inputBind("2", (Int64)5);
            cur.inputBind("3", (Int64)5);
            cur.inputBind("4", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("5", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("6", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("7", "testchar5");
            cur.inputBind("8", "testvarchar5");
            cur.inputBind("9", (Int16)2005, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("10", (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int16)5, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("11", "testclob5", (UInt32)9);
            cur.inputBindBlob("12", System.Text.Encoding.ASCII.GetBytes("testblob5"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)6);
            cur.inputBind("2", (Int64)6);
            cur.inputBind("3", (Int64)6);
            cur.inputBind("4", 6.6, (UInt32)4, (UInt32)2);
            cur.inputBind("5", 6.6, (UInt32)4, (UInt32)2);
            cur.inputBind("6", 6.6, (UInt32)4, (UInt32)2);
            cur.inputBind("7", "testchar6");
            cur.inputBind("8", "testvarchar6");
            cur.inputBind("9", (Int16)2006, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("10", (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int16)6, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("11", "testclob6", (UInt32)9);
            cur.inputBindBlob("12", System.Text.Encoding.ASCII.GetBytes("testblob6"), (UInt32)9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            Console.WriteLine("ARRAY OF INPUT BINDS BY POSITION: ");
            cur.clearBinds();
            cur.inputBind(bindvars, bindvals);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by position with validation
            Console.WriteLine("INPUT BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1", (Int64)8);
            cur.inputBind("2", (Int64)8);
            cur.inputBind("3", (Int64)8);
            cur.inputBind("4", 8.8, (UInt32)4, (UInt32)2);
            cur.inputBind("5", 8.8, (UInt32)4, (UInt32)2);
            cur.inputBind("6", 8.8, (UInt32)4, (UInt32)2);
            cur.inputBind("7", "testchar8");
            cur.inputBind("8", "testvarchar8");
            cur.inputBind("9", (Int16)2008, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("10", (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int16)8, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("11", "testclob8", (UInt32)9);
            cur.inputBindBlob("12", System.Text.Encoding.ASCII.GetBytes("testblob8"), (UInt32)9);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");

            // input bind by name
            // db2 doesn't support bind by name


            // array of input binds by name
            // db2 doesn't support bind by name


            // input bind by name with validation
            // db2 doesn't support bind by name


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(), (UInt32)13);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName((UInt32)0), "TESTSMALLINT");
            assertEquals(cur.getColumnName((UInt32)1), "TESTINT");
            assertEquals(cur.getColumnName((UInt32)2), "TESTBIGINT");
            assertEquals(cur.getColumnName((UInt32)3), "TESTDECIMAL");
            assertEquals(cur.getColumnName((UInt32)4), "TESTREAL");
            assertEquals(cur.getColumnName((UInt32)5), "TESTDOUBLE");
            assertEquals(cur.getColumnName((UInt32)6), "TESTCHAR");
            assertEquals(cur.getColumnName((UInt32)7), "TESTVARCHAR");
            assertEquals(cur.getColumnName((UInt32)8), "TESTDATE");
            assertEquals(cur.getColumnName((UInt32)9), "TESTTIME");
            assertEquals(cur.getColumnName((UInt32)10), "TESTTIMESTAMP");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "TESTSMALLINT");
            assertEquals(cols[1], "TESTINT");
            assertEquals(cols[2], "TESTBIGINT");
            assertEquals(cols[3], "TESTDECIMAL");
            assertEquals(cols[4], "TESTREAL");
            assertEquals(cols[5], "TESTDOUBLE");
            assertEquals(cols[6], "TESTCHAR");
            assertEquals(cols[7], "TESTVARCHAR");
            assertEquals(cols[8], "TESTDATE");
            assertEquals(cols[9], "TESTTIME");
            assertEquals(cols[10], "TESTTIMESTAMP");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            assertEquals(cur.getColumnType((UInt32)0), "SMALLINT");
            assertEquals(cur.getColumnType("TESTSMALLINT"), "SMALLINT");
            assertEquals(cur.getColumnType((UInt32)1), "INTEGER");
            assertEquals(cur.getColumnType("TESTINT"), "INTEGER");
            assertEquals(cur.getColumnType((UInt32)2), "BIGINT");
            assertEquals(cur.getColumnType("TESTBIGINT"), "BIGINT");
            assertEquals(cur.getColumnType((UInt32)3), "DECIMAL");
            assertEquals(cur.getColumnType("TESTDECIMAL"), "DECIMAL");
            assertEquals(cur.getColumnType((UInt32)4), "REAL");
            assertEquals(cur.getColumnType("TESTREAL"), "REAL");
            assertEquals(cur.getColumnType((UInt32)5), "DOUBLE");
            assertEquals(cur.getColumnType("TESTDOUBLE"), "DOUBLE");
            assertEquals(cur.getColumnType((UInt32)6), "CHAR");
            assertEquals(cur.getColumnType("TESTCHAR"), "CHAR");
            assertEquals(cur.getColumnType((UInt32)7), "VARCHAR");
            assertEquals(cur.getColumnType("TESTVARCHAR"), "VARCHAR");
            assertEquals(cur.getColumnType((UInt32)8), "DATE");
            assertEquals(cur.getColumnType("TESTDATE"), "DATE");
            assertEquals(cur.getColumnType((UInt32)9), "TIME");
            assertEquals(cur.getColumnType("TESTTIME"), "TIME");
            assertEquals(cur.getColumnType((UInt32)10), "TIMESTAMP");
            assertEquals(cur.getColumnType("TESTTIMESTAMP"), "TIMESTAMP");
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)2);
            assertEquals(cur.getColumnLength("TESTSMALLINT"), (UInt32)2);
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)4);
            assertEquals(cur.getColumnLength("TESTINT"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)2), (UInt32)8);
            assertEquals(cur.getColumnLength("TESTBIGINT"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)3), (UInt32)12);
            assertEquals(cur.getColumnLength("TESTDECIMAL"), (UInt32)12);
            assertEquals(cur.getColumnLength((UInt32)4), (UInt32)4);
            assertEquals(cur.getColumnLength("TESTREAL"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)5), (UInt32)8);
            assertEquals(cur.getColumnLength("TESTDOUBLE"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)6), (UInt32)40);
            assertEquals(cur.getColumnLength("TESTCHAR"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)7), (UInt32)40);
            assertEquals(cur.getColumnLength("TESTVARCHAR"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)8), (UInt32)6);
            assertEquals(cur.getColumnLength("TESTDATE"), (UInt32)6);
            assertEquals(cur.getColumnLength((UInt32)9), (UInt32)6);
            assertEquals(cur.getColumnLength("TESTTIME"), (UInt32)6);
            assertEquals(cur.getColumnLength((UInt32)10), (UInt32)16);
            assertEquals(cur.getColumnLength("TESTTIMESTAMP"), (UInt32)16);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest("TESTSMALLINT"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)1), (UInt32)1);
            assertEquals(cur.getLongest("TESTINT"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)2), (UInt32)1);
            assertEquals(cur.getLongest("TESTBIGINT"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)3), (UInt32)4);
            assertEquals(cur.getLongest("TESTDECIMAL"), (UInt32)4);
            //assertEquals(cur.getLongest((UInt32)4),(UInt32)3);
            //assertEquals(cur.getLongest("TESTREAL"),(UInt32)3);
            //assertEquals(cur.getLongest((UInt32)5),(UInt32)3);
            //assertEquals(cur.getLongest("TESTDOUBLE"),(UInt32)3);
            assertEquals(cur.getLongest((UInt32)6), (UInt32)40);
            assertEquals(cur.getLongest("TESTCHAR"), (UInt32)40);
            assertEquals(cur.getLongest((UInt32)7), (UInt32)12);
            assertEquals(cur.getLongest("TESTVARCHAR"), (UInt32)12);
            assertEquals(cur.getLongest((UInt32)8), (UInt32)10);
            assertEquals(cur.getLongest("TESTDATE"), (UInt32)10);
            assertEquals(cur.getLongest((UInt32)9), (UInt32)8);
            assertEquals(cur.getLongest("TESTTIME"), (UInt32)8);
            Console.WriteLine("");


            // row count
            Console.WriteLine("ROW COUNT: ");
            assertEquals(cur.rowCount(), (UInt64)8);
            Console.WriteLine("");


            // total rows
            Console.WriteLine("TOTAL ROWS: ");
            assertEquals(cur.totalRows(), (UInt64)0);
            Console.WriteLine("");


            // first row index
            Console.WriteLine("FIRST ROW INDEX: ");
            assertEquals(cur.firstRowIndex(), (UInt64)0);
            Console.WriteLine("");


            // end of result set
            Console.WriteLine("END OF RESULT SET: ");
            assertTrue(cur.endOfResultSet());
            Console.WriteLine("");


            // fields by index
            Console.WriteLine("FIELDS BY INDEX: ");
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), "1.10");
            //assertEquals(cur.getField((UInt64)0,(UInt32)4),"1.1");
            //assertEquals(cur.getField((UInt64)0,(UInt32)5),"1.1");
            assertEquals(cur.getField((UInt64)0, (UInt32)6), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, (UInt32)7), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)8), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, (UInt32)9), "01:00:00");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)3), "8.80");
            //assertEquals(cur.getField((UInt64)7,(UInt32)4),"8.8");
            //assertEquals(cur.getField((UInt64)7,(UInt32)5),"8.8");
            assertEquals(cur.getField((UInt64)7, (UInt32)6), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, (UInt32)7), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)8), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, (UInt32)9), "08:00:00");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)4);
            //assertEquals(cur.getFieldLength((UInt64)0,(UInt32)4),(UInt32)3);
            //assertEquals(cur.getFieldLength((UInt64)0,(UInt32)5),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)6), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)7), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)8), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)9), (UInt32)8);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)4);
            //assertEquals(cur.getFieldLength((UInt64)7,(UInt32)4),(UInt32)3);
            //assertEquals(cur.getFieldLength((UInt64)7,(UInt32)5),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)6), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)7), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)8), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)9), (UInt32)8);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "TESTSMALLINT"), "1");
            assertEquals(cur.getField((UInt64)0, "TESTINT"), "1");
            assertEquals(cur.getField((UInt64)0, "TESTBIGINT"), "1");
            assertEquals(cur.getField((UInt64)0, "TESTDECIMAL"), "1.10");
            //assertEquals(cur.getField((UInt64)0,"TESTREAL"),"1.1");
            //assertEquals(cur.getField((UInt64)0,"TESTDOUBLE"),"1.1");
            assertEquals(cur.getField((UInt64)0, "TESTCHAR"), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, "TESTVARCHAR"), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, "TESTDATE"), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, "TESTTIME"), "01:00:00");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "TESTSMALLINT"), "8");
            assertEquals(cur.getField((UInt64)7, "TESTINT"), "8");
            assertEquals(cur.getField((UInt64)7, "TESTBIGINT"), "8");
            assertEquals(cur.getField((UInt64)7, "TESTDECIMAL"), "8.80");
            //assertEquals(cur.getField((UInt64)7,"TESTREAL"),"8.8");
            //assertEquals(cur.getField((UInt64)7,"TESTDOUBLE"),"8.8");
            assertEquals(cur.getField((UInt64)7, "TESTCHAR"), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, "TESTVARCHAR"), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, "TESTDATE"), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, "TESTTIME"), "08:00:00");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "TESTSMALLINT"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTINT"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTBIGINT"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTDECIMAL"), (UInt32)4);
            //assertEquals(cur.getFieldLength((UInt64)0,"TESTREAL"),(UInt32)3);
            //assertEquals(cur.getFieldLength((UInt64)0,"TESTDOUBLE"),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTCHAR"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTVARCHAR"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTDATE"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTTIME"), (UInt32)8);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "TESTSMALLINT"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTINT"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTBIGINT"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTDECIMAL"), (UInt32)4);
            //assertEquals(cur.getFieldLength((UInt64)7,"TESTREAL"),(UInt32)3);
            //assertEquals(cur.getFieldLength((UInt64)7,"TESTDOUBLE"),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTCHAR"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTVARCHAR"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTDATE"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTTIME"), (UInt32)8);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields = cur.getRow((UInt64)0);
            assertEquals(fields[0], "1");
            assertEquals(fields[1], "1");
            assertEquals(fields[2], "1");
            assertEquals(fields[3], "1.10");
            //assertEquals(fields[4],"1.1");
            //assertEquals(fields[5],"1.1");
            assertEquals(fields[6], "testchar1                               ");
            assertEquals(fields[7], "testvarchar1");
            assertEquals(fields[8], "2001-01-01");
            assertEquals(fields[9], "01:00:00");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens = cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0], (UInt32)1);
            assertEquals(fieldlens[1], (UInt32)1);
            assertEquals(fieldlens[2], (UInt32)1);
            assertEquals(fieldlens[3], (UInt32)4);
            //assertEquals(fieldlens[4],(UInt32)3);
            //assertEquals(fieldlens[5],(UInt32)3);
            assertEquals(fieldlens[6], (UInt32)40);
            assertEquals(fieldlens[7], (UInt32)12);
            assertEquals(fieldlens[8], (UInt32)10);
            assertEquals(fieldlens[9], (UInt32)8);
            Console.WriteLine("");


            // result set buffer size
            Console.WriteLine("RESULT SET BUFFER SIZE: ");
            assertEquals(cur.getResultSetBufferSize(), (UInt64)0);
            cur.setResultSetBufferSize((UInt64)2);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertEquals(cur.getResultSetBufferSize(), (UInt64)2);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)0);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)0), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)2);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)4);
            assertEquals(cur.getField((UInt64)6, (UInt32)0), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)8);
            assertTrue(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            cur.setResultSetBufferSize((UInt64)0);
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
                "	testsmallint "));
            assertEquals(cur.getColumnName((UInt32)0), (String)null);
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0), (String)null);
            cur.getColumnInfo();
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertEquals(cur.getColumnName((UInt32)0), "TESTSMALLINT");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)2);
            assertEquals(cur.getColumnType((UInt32)0), "SMALLINT");
            Console.WriteLine("");


            // suspended session
            Console.WriteLine("SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            assertTrue(con.resumeSession(port, socket));
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)0), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
            assertEquals(cur.getField((UInt64)3, (UInt32)0), "4");
            assertEquals(cur.getField((UInt64)4, (UInt32)0), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)0), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)0), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            assertTrue(con.resumeSession(port, socket));
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)0), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
            assertEquals(cur.getField((UInt64)3, (UInt32)0), "4");
            assertEquals(cur.getField((UInt64)4, (UInt32)0), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)0), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)0), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            assertTrue(con.resumeSession(port, socket));
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)0), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
            assertEquals(cur.getField((UInt64)3, (UInt32)0), "4");
            assertEquals(cur.getField((UInt64)4, (UInt32)0), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)0), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)0), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");


            // suspended result set
            Console.WriteLine("SUSPENDED RESULT SET: ");
            cur.setResultSetBufferSize((UInt64)2);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
            id = cur.getResultSetId();
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            assertTrue(con.resumeSession(port, socket));
            assertTrue(cur.resumeResultSet(id));
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)4);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)6);
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)8);
            assertTrue(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // cached result set
            Console.WriteLine("CACHED RESULT SET: ");
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl((UInt32)200);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");


            // column count for cached result set
            Console.WriteLine("COLUMN COUNT FOR CACHED RESULT SET: ");
            assertEquals(cur.colCount(), (UInt32)13);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName((UInt32)0), "TESTSMALLINT");
            assertEquals(cur.getColumnName((UInt32)1), "TESTINT");
            assertEquals(cur.getColumnName((UInt32)2), "TESTBIGINT");
            assertEquals(cur.getColumnName((UInt32)3), "TESTDECIMAL");
            assertEquals(cur.getColumnName((UInt32)4), "TESTREAL");
            assertEquals(cur.getColumnName((UInt32)5), "TESTDOUBLE");
            assertEquals(cur.getColumnName((UInt32)6), "TESTCHAR");
            assertEquals(cur.getColumnName((UInt32)7), "TESTVARCHAR");
            assertEquals(cur.getColumnName((UInt32)8), "TESTDATE");
            assertEquals(cur.getColumnName((UInt32)9), "TESTTIME");
            assertEquals(cur.getColumnName((UInt32)10), "TESTTIMESTAMP");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "TESTSMALLINT");
            assertEquals(cols[1], "TESTINT");
            assertEquals(cols[2], "TESTBIGINT");
            assertEquals(cols[3], "TESTDECIMAL");
            assertEquals(cols[4], "TESTREAL");
            assertEquals(cols[5], "TESTDOUBLE");
            assertEquals(cols[6], "TESTCHAR");
            assertEquals(cols[7], "TESTVARCHAR");
            assertEquals(cols[8], "TESTDATE");
            assertEquals(cols[9], "TESTTIME");
            assertEquals(cols[10], "TESTTIMESTAMP");
            Console.WriteLine("");


            // cached result set with result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize((UInt64)2);
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl((UInt32)200);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // from one cache file to another
            Console.WriteLine("FROM ONE CACHE FILE TO ANOTHER: ");
            cur.cacheToFile("cachefile2");
            assertTrue(cur.openCachedResultSet("cachefile1"));
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet("cachefile2"));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            Console.WriteLine("");


            // from one cache file to another with result set buffer size
            Console.WriteLine("FROM ONE CACHE FILE TO ANOTHER " +
                        "WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize((UInt64)2);
            cur.cacheToFile("cachefile2");
            assertTrue(cur.openCachedResultSet("cachefile1"));
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet("cachefile2"));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // cached result set with suspend and result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH SUSPEND " +
                        "AND RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize((UInt64)2);
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl((UInt32)200);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            id = cur.getResultSetId();
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            Console.WriteLine("");
            assertTrue(con.resumeSession(port, socket));
            assertTrue(cur.resumeCachedResultSet(id, filename));
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)4);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)6);
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)8);
            assertTrue(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            cur.cacheOff();
            Console.WriteLine("");
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // finished suspended session
            Console.WriteLine("FINISHED SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testint"));
            assertEquals(cur.getField((UInt64)4, (UInt32)0), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)0), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)0), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            id = cur.getResultSetId();
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            assertTrue(con.resumeSession(port, socket));
            assertTrue(cur.resumeResultSet(id));
            assertEquals(cur.getField((UInt64)4, (UInt32)0), (String)null);
            assertEquals(cur.getField((UInt64)5, (UInt32)0), (String)null);
            assertEquals(cur.getField((UInt64)6, (UInt32)0), (String)null);
            assertEquals(cur.getField((UInt64)7, (UInt32)0), (String)null);
            Console.WriteLine("");


            // nested selects
            Console.WriteLine("NESTED SELECTS: ");
            cur.setResultSetBufferSize((UInt64)1);
            assertTrue(cur.sendQuery("select * from testtable"));
            secondcur = new SQLRCursor(con);
            secondcur.setResultSetBufferSize((UInt64)1);
            for (UInt32 i = 0; cur.getRow((UInt64)i) != null; i++)
            {
                assertTrue(secondcur.sendQuery("select * from testtable"));
            }
            secondcur.closeResultSet();
            cur.setResultSetBufferSize((UInt64)0);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // reset transaction state
            Console.WriteLine("RESET TRANSACTION STATE: ");
            assertTrue(con.commit());
            assertEquals(con.getTransactionModel(), "implicit");
            assertFalse(con.getAutoCommit());
            Console.WriteLine("");


            // transaction behavior - implicit
            Console.WriteLine("TRANSACTION BEHAVIOR - implicit: ");
            assertTrue(con.setTransactionModel("implicit"));
            assertEquals(con.getTransactionModel(), "implicit");
            assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
            // db2 DDL is transactional; commit so the table is visible to the
            // second connection (the commit implicitly starts a new tx)
            assertTrue(con.commit());
            secondcon = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                "db2inst1", "testpassword", 0, 1);
            secondcur = new SQLRCursor(secondcon);
            // session is in a transaction; insert is not visible until commit
            assertTrue(con.getInTransaction());
            assertFalse(con.getAutoCommit());
            assertTrue(cur.sendQuery("insert into testtable values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "0");
            // commit makes it visible, and implicitly starts a new transaction
            assertTrue(con.commit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // rollback discards, and implicitly starts a new transaction
            assertTrue(cur.sendQuery("insert into testtable values (2)"));
            assertTrue(con.rollback());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // autoCommitOn takes effect immediately
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertFalse(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable values (3)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "2");
            // autoCommitOff takes effect immediately
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            secondcur.closeResultSet();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior - explicit
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit: ");
            assertTrue(con.setTransactionModel("explicit"));
            assertEquals(con.getTransactionModel(), "explicit");
            assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
            // begin starts a new transaction; insert is not visible until commit
            assertTrue(con.begin());
            assertTrue(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "0");
            // commit makes it visible; no new transaction is started
            assertTrue(con.commit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // begin, insert, rollback discards; no new transaction is started
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable values (2)"));
            assertTrue(con.rollback());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // autoCommitOn takes effect immediately
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(cur.sendQuery("insert into testtable values (3)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "2");
            // autoCommitOff takes effect immediately
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            secondcur.closeResultSet();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior - explicit-deferred
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit-deferred: ");
            assertTrue(con.setTransactionModel("explicit-deferred"));
            assertEquals(con.getTransactionModel(), "explicit-deferred");
            // switch to autocommit-on so the begin/commit cycles below
            // bracket explicit transactions (autocommit-off semantics are
            // exercised at the end of this block)
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
            // begin starts a transaction; commit makes it visible
            assertTrue(con.begin());
            assertTrue(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable values (1)"));
            assertTrue(con.commit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // begin, insert, rollback discards
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable values (2)"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // during a transaction started by begin(), autoCommitOn is a
            // no-op: the autocommit setting takes effect after the user
            // explicitly commits/rollbacks the tx (mysql-native semantic)
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable values (3)"));
            assertTrue(con.autoCommitOn());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // explicit commit ends the tx; autocommit-on now takes effect
            assertTrue(con.commit());
            assertTrue(con.getAutoCommit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "2");
            // autocommit is on; subsequent inserts are visible immediately
            assertTrue(cur.sendQuery("insert into testtable values (4)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "3");
            // autoCommitOff takes effect immediately when not in a transaction
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            // autocommit-off persists across commit/rollback; each commit or
            // rollback ends the current implicit tx and a new one starts for
            // the next statement
            assertTrue(cur.sendQuery("insert into testtable values (5)"));
            assertTrue(con.commit());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "4");
            assertTrue(cur.sendQuery("insert into testtable values (6)"));
            assertTrue(con.rollback());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "4");
            // autoCommitOff during a transaction changes the variable
            // immediately but the in-flight tx continues; only after the
            // next explicit commit/rollback does the new autocommit-off
            // setting drop us into a new implicit tx (mysql-asymmetric
            // semantic)
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable values (7)"));
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "4");
            assertTrue(con.commit());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "5");
            secondcur.closeResultSet();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior - explicit-error
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit-error: ");
            assertTrue(con.setTransactionModel("explicit-error"));
            assertEquals(con.getTransactionModel(), "explicit-error");
            assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
            // begin, insert, commit
            assertTrue(con.begin());
            assertTrue(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable values (1)"));
            assertTrue(con.commit());
            assertFalse(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // begin, insert, rollback
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable values (2)"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // while in a transaction, autoCommitOn/Off throw an error
            assertTrue(con.begin());
            assertFalse(con.autoCommitOn());
            assertFalse(con.autoCommitOff());
            assertTrue(con.commit());
            // outside of a transaction, autoCommitOn takes effect immediately
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(cur.sendQuery("insert into testtable values (3)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "2");
            // autoCommitOff takes effect immediately
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            secondcur.closeResultSet();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior - none
            Console.WriteLine("TRANSACTION BEHAVIOR - none: ");
            assertTrue(con.setTransactionModel("none"));
            assertEquals(con.getTransactionModel(), "none");
            assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
            // no transactions; everything is visible immediately
            assertTrue(con.getAutoCommit());
            assertFalse(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
            // commit and rollback are no-ops
            assertTrue(con.commit());
            assertTrue(cur.sendQuery("insert into testtable values (2)"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "2");
            // autocommit is always on; autoCommitOff is an error
            assertFalse(con.autoCommitOff());
            assertTrue(con.getAutoCommit());
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            secondcur.closeResultSet();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // reset transaction behavior
            Console.WriteLine("RESET TRANSACTION BEHAVIOR: ");
            assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
            assertEquals(con.getTransactionModel(), "implicit");
            assertFalse(con.getAutoCommit());
            Console.WriteLine("");


            // individual substitutions
            Console.WriteLine("INDIVIDUAL SUBSTITUTIONS: ");
            cur.prepareQuery("values ($(var1),'$(var2)','$(var3)')");
            cur.substitution("var1", (Int64)1);
            cur.substitution("var2", "hello");
            cur.substitution("var3", 10.5556, (UInt32)6, (UInt32)4);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            Console.WriteLine("");


            // array substitutions
            Console.WriteLine("ARRAY SUBSTITUTIONS: ");
            cur.prepareQuery("values ('$(var1)','$(var2)','$(var3)')");
            cur.substitutions(subvars, subvalstrings);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "hi");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "bye");
            Console.WriteLine("");
            cur.prepareQuery("values ($(var1),$(var2),$(var3))");
            cur.substitution(subvars, subvallongs);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "3");
            Console.WriteLine("");
            cur.prepareQuery("values ($(var1),$(var2),$(var3))");
            cur.substitution(subvars, subvaldoubles, precs, scales);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "10.55");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "10.556");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            Console.WriteLine("");


            // nulls as nulls
            Console.WriteLine("NULLS AS NULLS: ");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery("select NULL,1,NULL from sysibm.sysdummy1"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), (String)null);
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("select NULL,1,NULL from sysibm.sysdummy1"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "");
            Console.WriteLine("");


            // null and empty lobs
            Console.WriteLine("NULL AND EMPTY LOBS: ");
            cur.getNullsAsNulls();
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testclob1 clob, " +
                "	testclob2 clob, " +
                "	testblob1 blob, " +
                "	testblob2 blob)"));
            assertTrue(con.commit());
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?)");
            cur.inputBindClob("1", "", (UInt32)0);
            cur.inputBindClob("2", (String)null, (UInt32)0);
            cur.inputBindBlob("3", System.Text.Encoding.ASCII.GetBytes(""), (UInt32)0);
            cur.inputBindBlob("4", (Byte[])null, (UInt32)0);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), (String)null);
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // long lobs
            Console.WriteLine("LONG LOBS: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(con.commit());
            cur.prepareQuery("insert into testtable values (?,?)");
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebuffer[i] = 'C';
            }
            largebuffer[LARGE_BUFFER_LENGTH] = '\0';
            String largebufferstr = new String(largebuffer, 0, LARGE_BUFFER_LENGTH);
            Byte[] largebufferbytes = System.Text.Encoding.ASCII.GetBytes(largebufferstr);
            cur.inputBindClob("1", largebufferstr, (UInt32)LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("2", largebufferbytes, (UInt32)LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "TESTCLOB"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "TESTCLOB"), largebufferstr);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTBLOB"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, "TESTBLOB"), largebufferbytes,
                                LARGE_BUFFER_LENGTH);
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // output bind by position
            Console.WriteLine("OUTPUT BIND BY POSITION: ");
            cur.sendQuery("drop procedure testproc");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	out out1 int, " +
                "	out out2 varchar(20), " +
                "	out out3 double, " +
                "	out out4 date, " +
                "	out out5 varchar(20)) " +
                "language sql " +
                "begin " +
                "	set out1 = 1; " +
                "	set out2 = 'hello'; " +
                "	set out3 = 2.5; " +
                "	set out4 = '2001-02-03'; " +
                "	set out5 = null; " +
                "end"));
            assertTrue(con.commit());
            cur.prepareQuery("call testproc(?,?,?,?,?)");
            assertEquals(cur.countBindVariables(), (UInt16)5);
            cur.defineOutputBindInteger("1");
            cur.defineOutputBindString("2", (UInt32)20);
            cur.defineOutputBindDouble("3");
            cur.defineOutputBindDate("4");
            cur.defineOutputBindString("5", (UInt32)20);
            assertTrue(cur.executeQuery());
            numvar = cur.getOutputBindInteger("1");
            stringvar = cur.getOutputBindString("2");
            floatvar = cur.getOutputBindDouble("3");
            cur.getOutputBindDate("4", out year, out month, out day,
                        out hour, out minute, out second, out microsecond, out tz,
                        out isnegative);
            assertEquals(numvar, (Int64)1);
            assertEquals(stringvar, "hello");
            assertEquals(floatvar, 2.5);
            assertEquals(year, (Int16)2001);
            assertEquals(month, (Int16)2);
            assertEquals(day, (Int16)3);
            assertEquals(hour, (Int16)0);
            assertEquals(minute, (Int16)0);
            assertEquals(second, (Int16)0);
            assertEquals(microsecond, (Int32)0);
            assertEquals(tz, "");
            assertEquals(isnegative, false);
            nullvar = cur.getOutputBindString("5");
            assertEquals(nullvar, (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // output bind by name
            // db2 doesn't support bind by name


            // output bind by name with validation
            // db2 doesn't support bind by name


            // lob output bind
            Console.WriteLine("LOB OUTPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(
                "create table testtable (" +
                "	testclob clob, " +
                "	testblob blob)");
            assertTrue(con.commit());
            cur.prepareQuery("insert into testtable values ('hello',?)");
            cur.inputBindBlob("1", System.Text.Encoding.ASCII.GetBytes("hello"), (UInt32)5);
            assertTrue(cur.executeQuery());
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	out out1 clob, " +
                "	out out2 blob) " +
                "language sql " +
                "begin " +
                "	select testclob into out1 from testtable; " +
                "	select testblob into out2 from testtable; " +
                "end"));
            assertTrue(con.commit());
            cur.prepareQuery("call testproc(?,?)");
            cur.defineOutputBindClob("1");
            cur.defineOutputBindBlob("2");
            assertTrue(cur.executeQuery());
            clobvar = cur.getOutputBindClob("1");
            clobvarlength = cur.getOutputBindLength("1");
            blobvar = cur.getOutputBindBlob("2");
            blobvarlength = cur.getOutputBindLength("2");
            assertEquals(clobvar, "hello", 5);
            assertEquals(clobvarlength, (UInt32)5);
            assertEquals(blobvar, System.Text.Encoding.ASCII.GetBytes("hello"), 5);
            assertEquals(blobvarlength, (UInt32)5);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // long output bind
            Console.WriteLine("LONG OUTPUT BIND: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in in1 clob, " +
                "	out out1 clob) " +
                "language sql " +
                "begin " +
                "	set out1 = in1; " +
                "end"));
            assertTrue(con.commit());
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebuffer[i] = 'C';
            }
            largebuffer[LARGE_BUFFER_LENGTH] = '\0';
            largebufferstr = new String(largebuffer, 0, LARGE_BUFFER_LENGTH);
            cur.prepareQuery("call testproc(?,?)");
            cur.inputBindClob("1", largebufferstr, (UInt32)LARGE_BUFFER_LENGTH);
            cur.defineOutputBindClob("2");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindLength("2"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getOutputBindClob("2"), largebufferstr);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery("create table testtable (testval integer)");
            assertTrue(con.commit());
            cur.prepareQuery("insert into testtable values (?)");
            cur.inputBind("1", (Int64)(-1));
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testval from testtable");
            assertEquals(cur.getField((UInt64)0, "TESTVAL"), "-1");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // bind validation
            // db2 doesn't support bind by name


            // rebinding
            Console.WriteLine("REBINDING: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in in1 int, " +
                "	out out1 int) " +
                "language sql " +
                "begin " +
                "	set out1 = in1; " +
                "end"));
            assertTrue(con.commit());
            cur.prepareQuery("call testproc(?,?)");
            cur.inputBind("1", (Int64)1);
            cur.defineOutputBindInteger("2");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("2"), (Int64)1);
            cur.inputBind("1", (Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("2"), (Int64)2);
            cur.inputBind("1", (Int64)3);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("2"), (Int64)3);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // reexecute
            Console.WriteLine("REEXECUTE: ");
            cur.prepareQuery("select 1 from sysibm.sysdummy1");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.prepareQuery("select cast(? as integer) from sysibm.sysdummy1");
            cur.inputBind("1", (Int64)1);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.inputBind("1", (Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "2");
            Console.WriteLine("");


            // stored procedure returning no value
            Console.WriteLine("STORED PROCEDURE RETURNING NO VALUE: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in in1 int, " +
                "	in in2 double, " +
                "	in in3 varchar(20)) " +
                "language sql " +
                "begin " +
                "	return; " +
                "end"));
            assertTrue(con.commit());
            cur.prepareQuery("call testproc(?,?,?)");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 1.1, (UInt32)2, (UInt32)1);
            cur.inputBind("3", "hello");
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // stored procedure returning single value
            Console.WriteLine("STORED PROCEDURE RETURNING SINGLE VALUE: ");
            cur.sendQuery("drop function testfunc");
            assertTrue(cur.sendQuery(
                "create function testfunc(" +
                "	in1 int, " +
                "	in2 double, " +
                "	in3 varchar(20)) " +
                "returns int " +
                "language sql " +
                "begin " +
                "	return in1; " +
                "end"));
            assertTrue(con.commit());
            cur.prepareQuery("select testfunc(?,?,?) from sysibm.sysdummy1");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 1.1, (UInt32)2, (UInt32)1);
            cur.inputBind("3", "hello");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertTrue(cur.sendQuery("drop function testfunc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // stored procedure returning multiple values
            Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in in1 int, " +
                "	in in2 double, " +
                "	in in3 varchar(20), " +
                "	in in4 clob, " +
                "	in in5 blob, " +
                "	out out1 int, " +
                "	out out2 double, " +
                "	out out3 varchar(20), " +
                "	out out4 clob, " +
                "	out out5 blob) " +
                "language sql " +
                "begin " +
                "	set out1 = in1; " +
                "	set out2 = in2; " +
                "	set out3 = in3; " +
                "	set out4 = in4; " +
                "	set out5 = in5; " +
                "end"));
            assertTrue(con.commit());
            cur.prepareQuery("call testproc(?,?,?,?,?,?,?,?,?,?)");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 1.1, (UInt32)2, (UInt32)1);
            cur.inputBind("3", "hello");
            cur.inputBindClob("4", "clob", (UInt32)4);
            cur.inputBindBlob("5", System.Text.Encoding.ASCII.GetBytes("blob"), (UInt32)4);
            cur.defineOutputBindInteger("6");
            cur.defineOutputBindDouble("7");
            cur.defineOutputBindString("8", (UInt32)20);
            cur.defineOutputBindClob("9");
            cur.defineOutputBindBlob("10");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("6"), (Int64)1);
            assertEquals(cur.getOutputBindDouble("7"), 1.1);
            assertEquals(cur.getOutputBindString("8"), "hello");
            assertEquals(cur.getOutputBindClob("9"), "clob");
            assertEquals(cur.getOutputBindBlob("10"), System.Text.Encoding.ASCII.GetBytes("blob"));
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // stored procedure returning result set
            Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc() " +
                "result set 1 " +
                "language sql " +
                "begin " +
                "	declare c1 cursor with return for " +
                "		select 1 from sysibm.sysdummy1 " +
                "		union " +
                "		select 2 from sysibm.sysdummy1 " +
                "		union " +
                "		select 3 from sysibm.sysdummy1 " +
                "		union " +
                "		select 4 from sysibm.sysdummy1 " +
                "		union " +
                "		select 5 from sysibm.sysdummy1 " +
                "		union " +
                "		select 6 from sysibm.sysdummy1 " +
                "		union " +
                "		select 7 from sysibm.sysdummy1 " +
                "		union " +
                "		select 8 from sysibm.sysdummy1; " +
                "	open c1; " +
                "end"));
            assertTrue(con.commit());
            assertTrue(cur.sendQuery("call testproc()"));
            assertEquals(cur.rowCount(), (UInt64)8);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // temporary tables
            Console.WriteLine("TEMPORARY TABLES: ");
            cur.sendQuery("drop table session.temptable");
            assertTrue(cur.sendQuery(
                "declare global temporary table session.temptable (" +
                "	col1 int "+
                ") not logged"));
            assertTrue(cur.sendQuery("insert into session.temptable values (1)"));
            assertTrue(cur.sendQuery("select count(*) from session.temptable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            con.endSession();
            Console.WriteLine("");
            assertFalse(cur.sendQuery("select count(*) from session.temptable"));
            Console.WriteLine("");


            // encoded binary data
            Console.WriteLine("ENCODED BINARY DATA: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 blob)"));
            Byte[] buffer = new Byte[256];
            for (int i = 0; i < 256; i++)
            {
                buffer[i] = (Byte)i;
            }
            System.Text.StringBuilder query = new System.Text.StringBuilder();
            query.Append("insert into testtable values (blob(X'");
            for (int i = 0; i < buffer.Length; i++)
            {
                query.Append(String.Format("{0:x2}", buffer[i]));
            }
            query.Append("'))");
            assertTrue(cur.sendQuery(query.ToString()));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)buffer.Length);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, (UInt32)0), buffer, buffer.Length);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // quotes
            Console.WriteLine("QUOTES: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"));
            assertTrue(cur.sendQuery("insert into testtable values ('''''')"));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "''");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // last insert id
            Console.WriteLine("LAST INSERT ID: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                    "create table testtable " +
                    "	(col1 int not null " +
                    "	generated always as identity, " +
                    "	col2 int, " +
                    "	primary key(col1))"));
            assertTrue(cur.sendQuery(
                    "insert into testtable (col2) values (1)"));
            assertEquals(con.getLastInsertId(), (UInt64)1);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // database is schema
            Console.WriteLine("DATABASE IS SCHEMA: ");
            assertTrue(con.getDatabaseIsSchema());
            Console.WriteLine("");


            // catalog list
            Console.WriteLine("CATALOG LIST: ");
            assertTrue(cur.getCatalogList(null));
            assertEquals(cur.getColumnName((UInt32)0), "Database");
            assertEquals(cur.rowCount(), (UInt64)0);
            Console.WriteLine("");


            // schema list
            Console.WriteLine("SCHEMA LIST: ");
            assertTrue(cur.getSchemaList(null));
            assertEquals(cur.getColumnName((UInt32)0), "Database");
            Boolean found = false;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                if (cur.getField(i, "Database") == "DB2INST1")
                {
                    found = true;
                    break;
                }
            }
            assertTrue(found);
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName((UInt32)0), "table_type");
            found = false;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                if (cur.getField(i, "table_type") == "TABLE")
                {
                    found = true;
                    break;
                }
            }
            assertTrue(found);
            Console.WriteLine("");


            // table list
            Console.WriteLine("TABLE LIST: ");
            cur.sendQuery("drop table testtable1");
            cur.sendQuery("drop table testtable2");
            cur.sendQuery("drop table testtable3");
            cur.sendQuery("drop table testtable4");
            assertTrue(cur.sendQuery(
                "create table testtable1 (" +
                "	col1 integer, " +
                "	col2 integer)"));
            assertTrue(cur.sendQuery(
                "create table testtable2 (" +
                "	col1 integer, " +
                "	col2 integer)"));
            assertTrue(cur.sendQuery(
                "create table testtable3 (" +
                "	col1 integer, " +
                "	col2 integer)"));
            assertTrue(cur.sendQuery(
                "create table testtable4 (" +
                "	col1 integer, " +
                "	col2 integer)"));
            assertTrue(con.commit());
            assertTrue(cur.getTableList(null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                String name = cur.getField(i, "Tables_in_xxx");
                if (name == "TESTTABLE1" ||
                    name == "TESTTABLE2" ||
                    name == "TESTTABLE3" ||
                    name == "TESTTABLE4")
                {
                    counter++;
                }
            }
            assertEquals(counter, (UInt64)4);
            assertTrue(cur.sendQuery("drop table testtable1"));
            assertTrue(cur.sendQuery("drop table testtable2"));
            assertTrue(cur.sendQuery("drop table testtable3"));
            assertTrue(cur.sendQuery("drop table testtable4"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // type info list
            Console.WriteLine("TYPE INFO LIST: ");
            assertTrue(cur.getTypeInfoList("integer"));
            assertEquals(cur.getColumnName((UInt32)0), "type_name");
            assertEquals(cur.getColumnName((UInt32)1), "data_type");
            assertEquals(cur.getColumnName((UInt32)2), "precision");
            assertEquals(cur.getColumnName((UInt32)3), "literal_prefix");
            assertEquals(cur.getColumnName((UInt32)4), "literal_suffix");
            assertEquals(cur.getColumnName((UInt32)5), "create_params");
            assertEquals(cur.getColumnName((UInt32)6), "nullable");
            assertEquals(cur.getColumnName((UInt32)7), "case_sensitive");
            assertEquals(cur.getColumnName((UInt32)8), "searchable");
            assertEquals(cur.getColumnName((UInt32)9), "unsigned_attribute");
            assertEquals(cur.getColumnName((UInt32)10), "fixed_prec_scale");
            assertEquals(cur.getColumnName((UInt32)11), "auto_increment");
            assertEquals(cur.getColumnName((UInt32)12), "local_type_name");
            assertEquals(cur.getColumnName((UInt32)13), "minumum_scale");
            assertEquals(cur.getColumnName((UInt32)14), "maxiumm_scale");
            assertEquals(cur.getColumnName((UInt32)15), "sql_data_type");
            assertEquals(cur.getColumnName((UInt32)16), "sql_datetime_sub");
            assertEquals(cur.getColumnName((UInt32)17), "num_prec_radix");
            assertEquals(cur.getColumnName((UInt32)18), "interval_precision");
            assertEquals(cur.getField((UInt64)0, "type_name"), "INTEGER");
            assertEquals(cur.getField((UInt64)0, "data_type"), "4");
            assertEquals(cur.getField((UInt64)0, "precision"), "10");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "INTEGER");
            assertTrue(cur.getTypeInfoList("char"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "CHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "1");
            assertEquals(cur.getField((UInt64)0, "precision"), "254");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "CHAR");
            assertTrue(cur.getTypeInfoList("varchar"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "VARCHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "12");
            assertEquals(cur.getField((UInt64)0, "precision"), "32672");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "VARCHAR");
            assertTrue(cur.getTypeInfoList("date"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "DATE");
            assertEquals(cur.getField((UInt64)0, "data_type"), "91");
            assertEquals(cur.getField((UInt64)0, "precision"), "10");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "DATE");
            Console.WriteLine("");


            // column list
            Console.WriteLine("COLUMN LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testsmallint smallint, " +
                "	testint integer, " +
                "	testbigint bigint, " +
                "	testdecimal decimal(10,2), " +
                "	testreal real, " +
                "	testdouble double, " +
                "	testchar char(40), " +
                "	testvarchar varchar(40), " +
                "	testdate date, " +
                "	testtime time, " +
                "	testtimestamp timestamp, " +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(con.commit());
            assertTrue(cur.getColumnList("testtable", null));
            assertEquals(cur.getColumnName((UInt32)0), "column_name");
            assertEquals(cur.getColumnName((UInt32)1), "data_type");
            assertEquals(cur.getColumnName((UInt32)2), "character_maximum_length");
            assertEquals(cur.getColumnName((UInt32)3), "numeric_precision");
            assertEquals(cur.getColumnName((UInt32)4), "numeric_scale");
            assertEquals(cur.getColumnName((UInt32)5), "is_nullable");
            assertEquals(cur.getColumnName((UInt32)6), "column_key");
            assertEquals(cur.getColumnName((UInt32)7), "column_default");
            assertEquals(cur.getColumnName((UInt32)8), "extra");
            assertEquals(cur.getField((UInt64)0, "column_name"), "TESTSMALLINT");
            assertEquals(cur.getField((UInt64)1, "column_name"), "TESTINT");
            assertEquals(cur.getField((UInt64)2, "column_name"), "TESTBIGINT");
            assertEquals(cur.getField((UInt64)3, "column_name"), "TESTDECIMAL");
            assertEquals(cur.getField((UInt64)4, "column_name"), "TESTREAL");
            assertEquals(cur.getField((UInt64)5, "column_name"), "TESTDOUBLE");
            assertEquals(cur.getField((UInt64)6, "column_name"), "TESTCHAR");
            assertEquals(cur.getField((UInt64)7, "column_name"), "TESTVARCHAR");
            assertEquals(cur.getField((UInt64)8, "column_name"), "TESTDATE");
            assertEquals(cur.getField((UInt64)9, "column_name"), "TESTTIME");
            assertEquals(cur.getField((UInt64)10, "column_name"), "TESTTIMESTAMP");
            assertEquals(cur.getField((UInt64)11, "column_name"), "TESTCLOB");
            assertEquals(cur.getField((UInt64)12, "column_name"), "TESTBLOB");
            assertEquals(cur.getField((UInt64)0, "data_type"), "SMALLINT");
            assertEquals(cur.getField((UInt64)1, "data_type"), "INTEGER");
            assertEquals(cur.getField((UInt64)2, "data_type"), "BIGINT");
            assertEquals(cur.getField((UInt64)3, "data_type"), "DECIMAL");
            assertEquals(cur.getField((UInt64)4, "data_type"), "REAL");
            assertEquals(cur.getField((UInt64)5, "data_type"), "DOUBLE");
            assertEquals(cur.getField((UInt64)6, "data_type"), "CHARACTER");
            assertEquals(cur.getField((UInt64)7, "data_type"), "VARCHAR");
            assertEquals(cur.getField((UInt64)8, "data_type"), "DATE");
            assertEquals(cur.getField((UInt64)9, "data_type"), "TIME");
            assertEquals(cur.getField((UInt64)10, "data_type"), "TIMESTAMP");
            assertEquals(cur.getField((UInt64)11, "data_type"), "CLOB");
            assertEquals(cur.getField((UInt64)12, "data_type"), "BLOB");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // column list - auto_increment, primary key
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int generated always as identity primary key, " +
                "	col2 int)"));
            assertTrue(con.commit());
            assertTrue(cur.getColumnList("testtable", null));
            assertTrue(cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertFalse(cur.getField((UInt64)1, "extra") != null && cur.getField((UInt64)1, "extra").Contains("auto_increment"));
            assertFalse(cur.getField((UInt64)1, "column_key") != null && cur.getField((UInt64)1, "column_key").Contains("PRI"));
            Console.WriteLine("");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int not null primary key, " +
                "	col2 int)"));
            assertTrue(con.commit());
            assertTrue(cur.getColumnList("testtable", null));
            assertFalse(cur.getField((UInt64)0, "extra") != null && cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // primary keys list
            Console.WriteLine("PRIMARY KEYS LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int not null primary key, " +
                "	col2 int)"));
            assertTrue(con.commit());
            assertTrue(cur.getPrimaryKeysList("testtable", null));
            assertEquals(cur.getColumnName((UInt32)0), "table");
            assertEquals(cur.getColumnName((UInt32)1), "non_unique");
            assertEquals(cur.getColumnName((UInt32)2), "key_name");
            assertEquals(cur.getColumnName((UInt32)3), "seq_in_index");
            assertEquals(cur.getColumnName((UInt32)4), "column_name");
            assertEquals(cur.getColumnName((UInt32)5), "collation");
            assertEquals(cur.getColumnName((UInt32)6), "cardinality");
            assertEquals(cur.getColumnName((UInt32)7), "sub_part");
            assertEquals(cur.getColumnName((UInt32)8), "packed");
            assertEquals(cur.getColumnName((UInt32)9), "null");
            assertEquals(cur.getColumnName((UInt32)10), "index_type");
            assertEquals(cur.getColumnName((UInt32)11), "comment");
            assertEquals(cur.getColumnName((UInt32)12), "index_comment");
            assertEquals(cur.rowCount(), (UInt64)1);
            assertTrue(cur.getField((UInt64)0, "table") == "TESTTABLE");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertTrue(cur.getField((UInt64)0, "column_name") == "COL1");
            assertTrue(!String.IsNullOrEmpty(cur.getField((UInt64)0, "key_name")));
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // key and index list
            Console.WriteLine("KEY AND INDEX LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int not null primary key, " +
                "	col2 int)"));
            assertTrue(con.commit());
            assertTrue(cur.getKeyAndIndexList("testtable", null));
            assertEquals(cur.getColumnName((UInt32)0), "table");
            assertEquals(cur.getColumnName((UInt32)1), "non_unique");
            assertEquals(cur.getColumnName((UInt32)2), "key_name");
            assertEquals(cur.getColumnName((UInt32)3), "seq_in_index");
            assertEquals(cur.getColumnName((UInt32)4), "column_name");
            assertEquals(cur.getColumnName((UInt32)5), "collation");
            assertEquals(cur.getColumnName((UInt32)6), "cardinality");
            assertEquals(cur.getColumnName((UInt32)7), "sub_part");
            assertEquals(cur.getColumnName((UInt32)8), "packed");
            assertEquals(cur.getColumnName((UInt32)9), "null");
            assertEquals(cur.getColumnName((UInt32)10), "index_type");
            assertEquals(cur.getColumnName((UInt32)11), "comment");
            assertEquals(cur.getColumnName((UInt32)12), "index_comment");
            assertEquals(cur.rowCount(), (UInt64)1);
            assertTrue(cur.getField((UInt64)0, "table") == "TESTTABLE");
            assertEquals(cur.getField((UInt64)0, "non_unique"), "0");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertTrue(cur.getField((UInt64)0, "column_name") == "COL1");
            assertEquals(cur.getField((UInt64)0, "collation"), "A");
            assertEquals(cur.getField((UInt64)0, "index_type"), "3");
            assertTrue(!String.IsNullOrEmpty(cur.getField((UInt64)0, "key_name")));
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // procedure list
            Console.WriteLine("PROCEDURE LIST: ");
            cur.sendQuery("drop procedure testproc1");
            cur.sendQuery("drop procedure testproc2");
            cur.sendQuery("drop procedure testproc3");
            cur.sendQuery("drop procedure testproc4");
            assertTrue(cur.sendQuery(
                "create procedure testproc1(" +
                "	in in1 integer, " +
                "	in in2 char(20), " +
                "	in in3 varchar(20), " +
                "	in in4 date) " +
                "language sql begin end"));
            assertTrue(cur.sendQuery(
                "create procedure testproc2(" +
                "	in in1 integer, " +
                "	in in2 char(20), " +
                "	in in3 varchar(20), " +
                "	in in4 date) " +
                "language sql begin end"));
            assertTrue(cur.sendQuery(
                "create procedure testproc3(" +
                "	in in1 integer, " +
                "	in in2 char(20), " +
                "	in in3 varchar(20), " +
                "	in in4 date) " +
                "language sql begin end"));
            assertTrue(cur.sendQuery(
                "create procedure testproc4(" +
                "	in in1 integer, " +
                "	in in2 char(20), " +
                "	in in3 varchar(20), " +
                "	in in4 date) " +
                "language sql begin end"));
            assertTrue(con.commit());
            assertTrue(cur.getProcedureList(null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                String name = cur.getField(i, "routine_name");
                if (name == "TESTPROC1" ||
                    name == "TESTPROC2" ||
                    name == "TESTPROC3" ||
                    name == "TESTPROC4")
                {
                    counter++;
                }
            }
            assertEquals(counter, (UInt64)4);
            Console.WriteLine("");


            // procedure parameter list
            Console.WriteLine("PROCEDURE PARAMETER LIST: ");
            assertTrue(cur.getProcedureParameterList("testproc1", null));
            assertEquals(cur.getColumnName((UInt32)0), "parameter_name");
            assertEquals(cur.getColumnName((UInt32)1), "parameter_mode");
            assertEquals(cur.getColumnName((UInt32)2), "data_type");
            assertEquals(cur.getColumnName((UInt32)3), "character_maximum_length");
            assertEquals(cur.getColumnName((UInt32)4), "ordinal_position");
            assertEquals(cur.rowCount(), (UInt64)4);
            assertEquals(cur.getField((UInt64)0, "parameter_name"), "IN1");
            assertEquals(cur.getField((UInt64)0, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)0, "data_type"), "INTEGER");
            assertEquals(cur.getField((UInt64)0, "ordinal_position"), "1");
            assertEquals(cur.getField((UInt64)1, "parameter_name"), "IN2");
            assertEquals(cur.getField((UInt64)1, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)1, "data_type"), "CHARACTER");
            assertEquals(cur.getField((UInt64)1, "ordinal_position"), "2");
            assertEquals(cur.getField((UInt64)2, "parameter_name"), "IN3");
            assertEquals(cur.getField((UInt64)2, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)2, "data_type"), "VARCHAR");
            assertEquals(cur.getField((UInt64)2, "ordinal_position"), "3");
            assertEquals(cur.getField((UInt64)3, "parameter_name"), "IN4");
            assertEquals(cur.getField((UInt64)3, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)3, "data_type"), "DATE");
            assertEquals(cur.getField((UInt64)3, "ordinal_position"), "4");
            assertTrue(cur.sendQuery("drop procedure testproc1"));
            assertTrue(cur.sendQuery("drop procedure testproc2"));
            assertTrue(cur.sendQuery("drop procedure testproc3"));
            assertTrue(cur.sendQuery("drop procedure testproc4"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // invalid queries
            Console.WriteLine("INVALID QUERIES: ");
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            Console.WriteLine("");
            assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
            assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
            assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
            assertFalse(cur.sendQuery("insert into testtable values (1,2,3,4)"));
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
