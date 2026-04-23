// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class SapTest : SQLRTest
    {
        public static int Main(string[] args)
        {
            String hostname = System.Net.Dns.GetHostName();
            int dot = hostname.IndexOf('.');
            if (dot > 0) { hostname = hostname.Substring(0, dot); }
            String dumptran = "dump tran " + hostname + " with truncate_only";

            String[] isolationlevels = new String[] { "1", "0", "2", "3" };
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
            UInt16 port;
            String socket;
            UInt16 id;
            String filename;
            UInt64 counter = 0;

            const Int32 LARGE_BUFFER_LENGTH = 255;
            Byte[] largebuffer = new Byte[LARGE_BUFFER_LENGTH + 1];
            String largestring;


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                    "testuser", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "sap");
            Console.WriteLine("");


            // ping
            Console.WriteLine("PING: ");
            assertTrue(con.ping());
            Console.WriteLine("");


            // bind format
            Console.WriteLine("BIND FORMAT: ");
            assertEquals(con.bindFormat(), "@*");
            Console.WriteLine("");


            // nextval format
            Console.WriteLine("NEXTVAL FORMAT: ");
            assertEquals(con.nextvalFormat(), "%s.nextval");
            Console.WriteLine("");


            // isolation levels
            Console.WriteLine("ISOLATION LEVELS: ");
            for (int ili = 0; ili < isolationlevels.Length; ili++)
            {
                assertTrue(con.setIsolationLevel(isolationlevels[ili]));
                assertEquals(con.getIsolationLevel(), isolationlevels[ili]);
                Console.WriteLine("");
            }
            // reset to the default isolation level
            assertTrue(con.setIsolationLevel(isolationlevels[0]));
            Console.WriteLine("");


            // create testtable
            Console.WriteLine("CREATE TESTTABLE: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(dumptran);
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testint int, " +
                "	testsmallint smallint, " +
                "	testtinyint tinyint, " +
                "	testreal real, " +
                "	testfloat float, " +
                "	testdecimal decimal(4,1), " +
                "	testnumeric numeric(4,1), " +
                "	testmoney money, " +
                "	testsmallmoney smallmoney, " +
                "	testdatetime datetime, " +
                "	testsmalldatetime smalldatetime, " +
                "	testchar char(40), " +
                "	testvarchar varchar(40), " +
                "	testbit bit, " +
                "	testtext text) lock datarows"));
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            assertTrue(con.begin());
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
                "	1.1, " +
                "	1.00, " +
                "	1.00, " +
                "	'01-Jan-2001 01:00:00', " +
                "	'01-Jan-2001 01:00:00', " +
                "	'testchar1', " +
                "	'testvarchar1', " +
                "	1, " +
                "	'testtext1')"));
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
                "	@var1, " +
                "	@var2, " +
                "	@var3, " +
                "	@var4, " +
                "	@var5, " +
                "	@var6, " +
                "	@var7, " +
                "	@var8, " +
                "	@var9, " +
                "	@var10, " +
                "	@var11, " +
                "	@var12, " +
                "	@var13, " +
                "	@var14, " +
                "	@var15)");
            assertEquals(cur.countBindVariables(), (UInt16)15);
            cur.inputBind("1", (Int64)2);
            cur.inputBind("2", (Int64)2);
            cur.inputBind("3", (Int64)2);
            cur.inputBind("4", 2.2, 2, 1);
            cur.inputBind("5", 2.2, 2, 1);
            cur.inputBind("6", 2.2, 2, 1);
            cur.inputBind("7", 2.2, 2, 1);
            cur.inputBind("8", 2.00, 3, 2);
            cur.inputBind("9", 2.00, 3, 2);
            cur.inputBind("10", "01-Jan-2002 02:00:00");
            cur.inputBind("11", "01-Jan-2002 02:00:00");
            cur.inputBind("12", "testchar2");
            cur.inputBind("13", "testvarchar2");
            cur.inputBind("14", (Int64)1);
            cur.inputBindClob("15", "testtext2", 9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)3);
            cur.inputBind("2", (Int64)3);
            cur.inputBind("3", (Int64)3);
            cur.inputBind("4", 3.3, 2, 1);
            cur.inputBind("5", 3.3, 2, 1);
            cur.inputBind("6", 3.3, 2, 1);
            cur.inputBind("7", 3.3, 2, 1);
            cur.inputBind("8", 3.00, 3, 2);
            cur.inputBind("9", 3.00, 3, 2);
            cur.inputBind("10", "01-Jan-2003 03:00:00");
            cur.inputBind("11", "01-Jan-2003 03:00:00");
            cur.inputBind("12", "testchar3");
            cur.inputBind("13", "testvarchar3");
            cur.inputBind("14", (Int64)1);
            cur.inputBindClob("15", "testtext3", 9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            // sap doesn't support implicit conversion of string binds to other
            // data types, so arrays of binds don't generally work.
            // Omitting the test.


            // input bind by position with validation
            Console.WriteLine("INPUT BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1", (Int64)4);
            cur.inputBind("2", (Int64)4);
            cur.inputBind("3", (Int64)4);
            cur.inputBind("4", 4.4, 2, 1);
            cur.inputBind("5", 4.4, 2, 1);
            cur.inputBind("6", 4.4, 2, 1);
            cur.inputBind("7", 4.4, 2, 1);
            cur.inputBind("8", 4.00, 3, 2);
            cur.inputBind("9", 4.00, 3, 2);
            cur.inputBind("10", "01-Jan-2004 04:00:00");
            cur.inputBind("11", "01-Jan-2004 04:00:00");
            cur.inputBind("12", "testchar4");
            cur.inputBind("13", "testvarchar4");
            cur.inputBind("14", (Int64)1);
            cur.inputBindClob("15", "testtext4", 9);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by name
            Console.WriteLine("INPUT BIND BY NAME: ");
            cur.clearBinds();
            cur.inputBind("var1", (Int64)5);
            cur.inputBind("var2", (Int64)5);
            cur.inputBind("var3", (Int64)5);
            cur.inputBind("var4", 5.5, 2, 1);
            cur.inputBind("var5", 5.5, 2, 1);
            cur.inputBind("var6", 5.5, 2, 1);
            cur.inputBind("var7", 5.5, 2, 1);
            cur.inputBind("var8", 5.00, 3, 2);
            cur.inputBind("var9", 5.00, 3, 2);
            cur.inputBind("var10", "01-Jan-2005 05:00:00");
            cur.inputBind("var11", "01-Jan-2005 05:00:00");
            cur.inputBind("var12", "testchar5");
            cur.inputBind("var13", "testvarchar5");
            cur.inputBind("var14", (Int64)1);
            cur.inputBindClob("var15", "testtext5", 9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("var1", (Int64)6);
            cur.inputBind("var2", (Int64)6);
            cur.inputBind("var3", (Int64)6);
            cur.inputBind("var4", 6.6, 2, 1);
            cur.inputBind("var5", 6.6, 2, 1);
            cur.inputBind("var6", 6.6, 2, 1);
            cur.inputBind("var7", 6.6, 2, 1);
            cur.inputBind("var8", 6.00, 3, 2);
            cur.inputBind("var9", 6.00, 3, 2);
            cur.inputBind("var10", "01-Jan-2006 06:00:00");
            cur.inputBind("var11", "01-Jan-2006 06:00:00");
            cur.inputBind("var12", "testchar6");
            cur.inputBind("var13", "testvarchar6");
            cur.inputBind("var14", (Int64)1);
            cur.inputBindClob("var15", "testtext6", 9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("var1", (Int64)7);
            cur.inputBind("var2", (Int64)7);
            cur.inputBind("var3", (Int64)7);
            cur.inputBind("var4", 7.7, 2, 1);
            cur.inputBind("var5", 7.7, 2, 1);
            cur.inputBind("var6", 7.7, 2, 1);
            cur.inputBind("var7", 7.7, 2, 1);
            cur.inputBind("var8", 7.00, 3, 2);
            cur.inputBind("var9", 7.00, 3, 2);
            cur.inputBind("var10", "01-Jan-2007 07:00:00");
            cur.inputBind("var11", "01-Jan-2007 07:00:00");
            cur.inputBind("var12", "testchar7");
            cur.inputBind("var13", "testvarchar7");
            cur.inputBind("var14", (Int64)1);
            cur.inputBindClob("var15", "testtext7", 9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by name
            // sap doesn't support implicit conversion of string binds to other
            // data types, so arrays of binds don't generally work.
            // Omitting the test.


            // input bind by name with validation
            Console.WriteLine("INPUT BIND BY NAME WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("var1", (Int64)8);
            cur.inputBind("var2", (Int64)8);
            cur.inputBind("var3", (Int64)8);
            cur.inputBind("var4", 8.8, 2, 1);
            cur.inputBind("var5", 8.8, 2, 1);
            cur.inputBind("var6", 8.8, 2, 1);
            cur.inputBind("var7", 8.8, 2, 1);
            cur.inputBind("var8", 8.00, 3, 2);
            cur.inputBind("var9", 8.00, 3, 2);
            cur.inputBind("var10", "01-Jan-2008 08:00:00");
            cur.inputBind("var11", "01-Jan-2008 08:00:00");
            cur.inputBind("var12", "testchar8");
            cur.inputBind("var13", "testvarchar8");
            cur.inputBind("var14", (Int64)1);
            cur.inputBindClob("var15", "testtext8", 9);
            cur.inputBind("var16", "junkvalue");
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(), (UInt32)15);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName(0), "testint");
            assertEquals(cur.getColumnName(1), "testsmallint");
            assertEquals(cur.getColumnName(2), "testtinyint");
            assertEquals(cur.getColumnName(3), "testreal");
            assertEquals(cur.getColumnName(4), "testfloat");
            assertEquals(cur.getColumnName(5), "testdecimal");
            assertEquals(cur.getColumnName(6), "testnumeric");
            assertEquals(cur.getColumnName(7), "testmoney");
            assertEquals(cur.getColumnName(8), "testsmallmoney");
            assertEquals(cur.getColumnName(9), "testdatetime");
            assertEquals(cur.getColumnName(10), "testsmalldatetime");
            assertEquals(cur.getColumnName(11), "testchar");
            assertEquals(cur.getColumnName(12), "testvarchar");
            assertEquals(cur.getColumnName(13), "testbit");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testint");
            assertEquals(cols[1], "testsmallint");
            assertEquals(cols[2], "testtinyint");
            assertEquals(cols[3], "testreal");
            assertEquals(cols[4], "testfloat");
            assertEquals(cols[5], "testdecimal");
            assertEquals(cols[6], "testnumeric");
            assertEquals(cols[7], "testmoney");
            assertEquals(cols[8], "testsmallmoney");
            assertEquals(cols[9], "testdatetime");
            assertEquals(cols[10], "testsmalldatetime");
            assertEquals(cols[11], "testchar");
            assertEquals(cols[12], "testvarchar");
            assertEquals(cols[13], "testbit");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            assertEquals(cur.getColumnType((UInt32)0), "INT");
            assertEquals(cur.getColumnType("testint"), "INT");
            assertEquals(cur.getColumnType((UInt32)1), "SMALLINT");
            assertEquals(cur.getColumnType("testsmallint"), "SMALLINT");
            assertEquals(cur.getColumnType((UInt32)2), "TINYINT");
            assertEquals(cur.getColumnType("testtinyint"), "TINYINT");
            assertEquals(cur.getColumnType((UInt32)3), "REAL");
            assertEquals(cur.getColumnType("testreal"), "REAL");
            assertEquals(cur.getColumnType((UInt32)4), "FLOAT");
            assertEquals(cur.getColumnType("testfloat"), "FLOAT");
            assertEquals(cur.getColumnType((UInt32)5), "DECIMAL");
            assertEquals(cur.getColumnType("testdecimal"), "DECIMAL");
            assertEquals(cur.getColumnType((UInt32)6), "NUMERIC");
            assertEquals(cur.getColumnType("testnumeric"), "NUMERIC");
            assertEquals(cur.getColumnType((UInt32)7), "MONEY");
            assertEquals(cur.getColumnType("testmoney"), "MONEY");
            assertEquals(cur.getColumnType((UInt32)8), "SMALLMONEY");
            assertEquals(cur.getColumnType("testsmallmoney"), "SMALLMONEY");
            assertEquals(cur.getColumnType((UInt32)9), "DATETIME");
            assertEquals(cur.getColumnType("testdatetime"), "DATETIME");
            assertEquals(cur.getColumnType((UInt32)10), "SMALLDATETIME");
            assertEquals(cur.getColumnType("testsmalldatetime"), "SMALLDATETIME");
            assertEquals(cur.getColumnType((UInt32)11), "CHAR");
            assertEquals(cur.getColumnType("testchar"), "CHAR");
            assertEquals(cur.getColumnType((UInt32)12), "CHAR");
            assertEquals(cur.getColumnType("testvarchar"), "CHAR");
            assertEquals(cur.getColumnType((UInt32)13), "BIT");
            assertEquals(cur.getColumnType("testbit"), "BIT");
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)4);
            assertEquals(cur.getColumnLength("testint"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)2);
            assertEquals(cur.getColumnLength("testsmallint"), (UInt32)2);
            assertEquals(cur.getColumnLength((UInt32)2), (UInt32)1);
            assertEquals(cur.getColumnLength("testtinyint"), (UInt32)1);
            assertEquals(cur.getColumnLength((UInt32)3), (UInt32)4);
            assertEquals(cur.getColumnLength("testreal"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)4), (UInt32)8);
            assertEquals(cur.getColumnLength("testfloat"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)5), (UInt32)35);
            assertEquals(cur.getColumnLength("testdecimal"), (UInt32)35);
            assertEquals(cur.getColumnLength((UInt32)6), (UInt32)35);
            assertEquals(cur.getColumnLength("testnumeric"), (UInt32)35);
            assertEquals(cur.getColumnLength((UInt32)7), (UInt32)8);
            assertEquals(cur.getColumnLength("testmoney"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)8), (UInt32)4);
            assertEquals(cur.getColumnLength("testsmallmoney"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)9), (UInt32)8);
            assertEquals(cur.getColumnLength("testdatetime"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)10), (UInt32)4);
            assertEquals(cur.getColumnLength("testsmalldatetime"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)11), (UInt32)40);
            assertEquals(cur.getColumnLength("testchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)12), (UInt32)40);
            assertEquals(cur.getColumnLength("testvarchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)13), (UInt32)1);
            assertEquals(cur.getColumnLength("testbit"), (UInt32)1);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest("testint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)1), (UInt32)1);
            assertEquals(cur.getLongest("testsmallint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)2), (UInt32)1);
            assertEquals(cur.getLongest("testtinyint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)3), (UInt32)18);
            assertEquals(cur.getLongest("testreal"), (UInt32)18);
            assertEquals(cur.getLongest((UInt32)4), (UInt32)18);
            assertEquals(cur.getLongest("testfloat"), (UInt32)18);
            assertEquals(cur.getLongest((UInt32)5), (UInt32)3);
            assertEquals(cur.getLongest("testdecimal"), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)6), (UInt32)3);
            assertEquals(cur.getLongest("testnumeric"), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)7), (UInt32)4);
            assertEquals(cur.getLongest("testmoney"), (UInt32)4);
            assertEquals(cur.getLongest((UInt32)8), (UInt32)4);
            assertEquals(cur.getLongest("testsmallmoney"), (UInt32)4);
            assertEquals(cur.getLongest((UInt32)9), (UInt32)19);
            assertEquals(cur.getLongest("testdatetime"), (UInt32)19);
            assertEquals(cur.getLongest((UInt32)10), (UInt32)19);
            assertEquals(cur.getLongest("testsmalldatetime"), (UInt32)19);
            assertEquals(cur.getLongest((UInt32)11), (UInt32)40);
            assertEquals(cur.getLongest("testchar"), (UInt32)40);
            assertEquals(cur.getLongest((UInt32)12), (UInt32)12);
            assertEquals(cur.getLongest("testvarchar"), (UInt32)12);
            assertEquals(cur.getLongest((UInt32)13), (UInt32)1);
            assertEquals(cur.getLongest("testbit"), (UInt32)1);
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
            //assertEquals(cur.getField((UInt64)0,(UInt32)3),"1.1");
            //assertEquals(cur.getField((UInt64)0,(UInt32)4),"1.1");
            assertEquals(cur.getField((UInt64)0, (UInt32)5), "1.1");
            assertEquals(cur.getField((UInt64)0, (UInt32)6), "1.1");
            assertEquals(cur.getField((UInt64)0, (UInt32)7), "1.00");
            assertEquals(cur.getField((UInt64)0, (UInt32)8), "1.00");
            assertEquals(cur.getField((UInt64)0, (UInt32)9), "Jan  1 2001  1:00AM");
            assertEquals(cur.getField((UInt64)0, (UInt32)10), "Jan  1 2001  1:00AM");
            assertEquals(cur.getField((UInt64)0, (UInt32)11), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, (UInt32)12), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)13), "1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "8");
            //assertEquals(cur.getField((UInt64)7,(UInt32)3),"8.8");
            //assertEquals(cur.getField((UInt64)7,(UInt32)4),"8.8");
            assertEquals(cur.getField((UInt64)7, (UInt32)5), "8.8");
            assertEquals(cur.getField((UInt64)7, (UInt32)6), "8.8");
            assertEquals(cur.getField((UInt64)7, (UInt32)7), "8.00");
            assertEquals(cur.getField((UInt64)7, (UInt32)8), "8.00");
            assertEquals(cur.getField((UInt64)7, (UInt32)9), "Jan  1 2008  8:00AM");
            assertEquals(cur.getField((UInt64)7, (UInt32)10), "Jan  1 2008  8:00AM");
            assertEquals(cur.getField((UInt64)7, (UInt32)11), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, (UInt32)12), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)13), "1");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)18);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)4), (UInt32)18);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)5), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)6), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)7), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)8), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)9), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)10), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)11), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)12), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)13), (UInt32)1);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)18);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)4), (UInt32)18);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)5), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)6), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)7), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)8), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)9), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)10), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)11), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)12), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)13), (UInt32)1);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "testint"), "1");
            assertEquals(cur.getField((UInt64)0, "testsmallint"), "1");
            assertEquals(cur.getField((UInt64)0, "testtinyint"), "1");
            //assertEquals(cur.getField((UInt64)0,"testreal"),"1.1");
            //assertEquals(cur.getField((UInt64)0,"testfloat"),"1.1");
            assertEquals(cur.getField((UInt64)0, "testdecimal"), "1.1");
            assertEquals(cur.getField((UInt64)0, "testnumeric"), "1.1");
            assertEquals(cur.getField((UInt64)0, "testmoney"), "1.00");
            assertEquals(cur.getField((UInt64)0, "testsmallmoney"), "1.00");
            assertEquals(cur.getField((UInt64)0, "testdatetime"), "Jan  1 2001  1:00AM");
            assertEquals(cur.getField((UInt64)0, "testsmalldatetime"), "Jan  1 2001  1:00AM");
            assertEquals(cur.getField((UInt64)0, "testchar"), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, "testvarchar"), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, "testbit"), "1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "testint"), "8");
            assertEquals(cur.getField((UInt64)7, "testsmallint"), "8");
            assertEquals(cur.getField((UInt64)7, "testtinyint"), "8");
            //assertEquals(cur.getField((UInt64)7,"testreal"),"8.8");
            //assertEquals(cur.getField((UInt64)7,"testfloat"),"8.8");
            assertEquals(cur.getField((UInt64)7, "testdecimal"), "8.8");
            assertEquals(cur.getField((UInt64)7, "testnumeric"), "8.8");
            assertEquals(cur.getField((UInt64)7, "testmoney"), "8.00");
            assertEquals(cur.getField((UInt64)7, "testsmallmoney"), "8.00");
            assertEquals(cur.getField((UInt64)7, "testdatetime"), "Jan  1 2008  8:00AM");
            assertEquals(cur.getField((UInt64)7, "testsmalldatetime"), "Jan  1 2008  8:00AM");
            assertEquals(cur.getField((UInt64)7, "testchar"), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, "testvarchar"), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, "testbit"), "1");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testtinyint"), (UInt32)1);
            //assertEquals(cur.getFieldLength((UInt64)0,"testreal"),(UInt32)3);
            //assertEquals(cur.getFieldLength((UInt64)0,"testfloat"),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testdecimal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testnumeric"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testmoney"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallmoney"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, "testdatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmalldatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, "testchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, "testbit"), (UInt32)1);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testtinyint"), (UInt32)1);
            //assertEquals(cur.getFieldLength((UInt64)7,"testreal"),(UInt32)3);
            //assertEquals(cur.getFieldLength((UInt64)7,"testfloat"),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testdecimal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testnumeric"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testmoney"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallmoney"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, "testdatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmalldatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, "testchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, "testbit"), (UInt32)1);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields = cur.getRow((UInt64)0);
            assertEquals(fields[0], "1");
            assertEquals(fields[1], "1");
            assertEquals(fields[2], "1");
            //assertEquals(fields[3],"1.1");
            //assertEquals(fields[4],"1.1");
            assertEquals(fields[5], "1.1");
            assertEquals(fields[6], "1.1");
            assertEquals(fields[7], "1.00");
            assertEquals(fields[8], "1.00");
            assertEquals(fields[9], "Jan  1 2001  1:00AM");
            assertEquals(fields[10], "Jan  1 2001  1:00AM");
            assertEquals(fields[11], "testchar1                               ");
            assertEquals(fields[12], "testvarchar1");
            assertEquals(fields[13], "1");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens = cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0], (UInt32)1);
            assertEquals(fieldlens[1], (UInt32)1);
            assertEquals(fieldlens[2], (UInt32)1);
            //assertEquals(fieldlens[3],(UInt32)3);
            //assertEquals(fieldlens[4],(UInt32)3);
            assertEquals(fieldlens[5], (UInt32)3);
            assertEquals(fieldlens[6], (UInt32)3);
            assertEquals(fieldlens[7], (UInt32)4);
            assertEquals(fieldlens[8], (UInt32)4);
            assertEquals(fieldlens[9], (UInt32)19);
            assertEquals(fieldlens[10], (UInt32)19);
            assertEquals(fieldlens[11], (UInt32)40);
            assertEquals(fieldlens[12], (UInt32)12);
            assertEquals(fieldlens[13], (UInt32)1);
            Console.WriteLine("");


            // result set buffer size
            Console.WriteLine("RESULT SET BUFFER SIZE: ");
            assertEquals(cur.getResultSetBufferSize(), (UInt64)0);
            cur.setResultSetBufferSize(2);
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // dont get column info
            Console.WriteLine("DONT GET COLUMN INFO: ");
            cur.dontGetColumnInfo();
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            assertEquals(cur.getColumnName(0), (String)null);
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0), (String)null);
            cur.getColumnInfo();
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            assertEquals(cur.getColumnName(0), "testint");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)4);
            assertEquals(cur.getColumnType((UInt32)0), "INT");
            Console.WriteLine("");


            // suspended session
            Console.WriteLine("SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            assertTrue(con.resumeSession(port, socket));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)0), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
            assertEquals(cur.getField((UInt64)3, (UInt32)0), "4");
            assertEquals(cur.getField((UInt64)4, (UInt32)0), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)0), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)0), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
            cur.setResultSetBufferSize(2);
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // cached result set
            Console.WriteLine("CACHED RESULT SET: ");
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");


            // column count for cached result set
            Console.WriteLine("COLUMN COUNT FOR CACHED RESULT SET: ");
            assertEquals(cur.colCount(), (UInt32)15);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName(0), "testint");
            assertEquals(cur.getColumnName(1), "testsmallint");
            assertEquals(cur.getColumnName(2), "testtinyint");
            assertEquals(cur.getColumnName(3), "testreal");
            assertEquals(cur.getColumnName(4), "testfloat");
            assertEquals(cur.getColumnName(5), "testdecimal");
            assertEquals(cur.getColumnName(6), "testnumeric");
            assertEquals(cur.getColumnName(7), "testmoney");
            assertEquals(cur.getColumnName(8), "testsmallmoney");
            assertEquals(cur.getColumnName(9), "testdatetime");
            assertEquals(cur.getColumnName(10), "testsmalldatetime");
            assertEquals(cur.getColumnName(11), "testchar");
            assertEquals(cur.getColumnName(12), "testvarchar");
            assertEquals(cur.getColumnName(13), "testbit");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testint");
            assertEquals(cols[1], "testsmallint");
            assertEquals(cols[2], "testtinyint");
            assertEquals(cols[3], "testreal");
            assertEquals(cols[4], "testfloat");
            assertEquals(cols[5], "testdecimal");
            assertEquals(cols[6], "testnumeric");
            assertEquals(cols[7], "testmoney");
            assertEquals(cols[8], "testsmallmoney");
            assertEquals(cols[9], "testdatetime");
            assertEquals(cols[10], "testsmalldatetime");
            assertEquals(cols[11], "testchar");
            assertEquals(cols[12], "testvarchar");
            assertEquals(cols[13], "testbit");
            Console.WriteLine("");


            // cached result set with result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            cur.setResultSetBufferSize(0);
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
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile2");
            assertTrue(cur.openCachedResultSet("cachefile1"));
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet("cachefile2"));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)0), (String)null);
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // cached result set with suspend and result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH SUSPEND " +
                        "AND RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // finished suspended session
            Console.WriteLine("FINISHED SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
            cur.setResultSetBufferSize(1);
            assertTrue(cur.sendQuery("select * from testtable"));
            for (UInt32 i = 0; cur.getRow((UInt64)i) != null; i++)
            {
                secondcur = new SQLRCursor(con);
                secondcur.setResultSetBufferSize(1);
                assertTrue(secondcur.sendQuery("select * from testtable"));
                secondcur.closeResultSet();
            }
            cur.setResultSetBufferSize(0);
            Console.WriteLine("");


            // commit and rollback
            Console.WriteLine("COMMIT AND ROLLBACK: ");
            secondcon = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                "testuser", "testpassword", 0, 1);
            secondcur = new SQLRCursor(secondcon);
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "0");
            assertTrue(con.commit());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "8");
            assertTrue(con.begin());
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	10, " +
                "	10, " +
                "	10, " +
                "	10.1, " +
                "	10.1, " +
                "	10.1, " +
                "	10.1, " +
                "	10.00, " +
                "	10.00, " +
                "	'01-Jan-2010 10:00:00', " +
                "	'01-Jan-2010 10:00:00', " +
                "	'testchar10', " +
                "	'testvarchar10', " +
                "	10, " +
                "	'testtext10')"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "8");
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	10, " +
                "	10, " +
                "	10, " +
                "	10.1, " +
                "	10.1, " +
                "	10.1, " +
                "	10.1, " +
                "	10.00, " +
                "	10.00, " +
                "	'01-Jan-2010 10:00:00', " +
                "	'01-Jan-2010 10:00:00', " +
                "	'testchar10', " +
                "	'testvarchar10', " +
                "	10, " +
                "	'testtext10')"));
            assertTrue(con.commit());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "9");
            secondcon.endSession();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // individual substitutions
            Console.WriteLine("INDIVIDUAL SUBSTITUTIONS: ");
            cur.prepareQuery("select $(var1),'$(var2)',$(var3)");
            cur.substitution("var1", (Int64)1);
            cur.substitution("var2", "hello");
            cur.substitution("var3", 10.5556, 6, 4);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            Console.WriteLine("");


            // array substitutions
            Console.WriteLine("ARRAY SUBSTITUTIONS: ");
            cur.prepareQuery("select $(var1),$(var2),$(var3)");
            cur.substitution(subvars, subvallongs);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "3");
            Console.WriteLine("");
            cur.prepareQuery("select '$(var1)','$(var2)','$(var3)'");
            cur.substitutions(subvars, subvalstrings);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "hi");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "bye");
            Console.WriteLine("");
            cur.prepareQuery("select $(var1),$(var2),$(var3)");
            cur.substitution(subvars, subvaldoubles, precs, scales);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "10.55");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "10.556");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            Console.WriteLine("");


            // nulls as nulls
            Console.WriteLine("NULLS AS NULLS: ");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery("select NULL,1,NULL"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), (String)null);
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("select NULL,1,NULL"));
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
                "	testclob1 text NULL, " +
                "	testclob2 text NULL, " +
                "	testblob1 image NULL, " +
                "	testblob2 image NULL)"));
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	@var1, " +
                "	@var2, " +
                "	@var3, " +
                "	@var4)");
            cur.inputBindClob("var1", "", 0);
            cur.inputBindClob("var2", null, 0);
            cur.inputBindBlob("var3", System.Text.Encoding.ASCII.GetBytes(""), (UInt32)0);
            cur.inputBindBlob("var4", null, (UInt32)0);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            // sap converts empty strings to a single space.  It's possible that
            // if we had true input bind support on the backend, then this would
            // work correctly, but for now we're faking binds, and inserting an
            // empty string, so we have to check for a single space here.
            assertEquals(cur.getField((UInt64)0, (UInt32)0), " ");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), (String)null);
            // see note above for why we're checking for a single space
            assertEquals(cur.getField((UInt64)0, (UInt32)2), " ");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // long lobs
            Console.WriteLine("LONG LOBS: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(
                "create table testtable (" +
                "	testclob text NULL, " +
                "	testblob image NULL) lock datarows");
            cur.prepareQuery("insert into testtable values (@var1,@var2)");
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebuffer[i] = (Byte)'C';
            }
            largebuffer[LARGE_BUFFER_LENGTH] = (Byte)'\0';
            largestring = new String('C', LARGE_BUFFER_LENGTH);
            cur.inputBindClob("var1", largestring, (UInt32)LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("var2", largebuffer, (UInt32)LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "testclob"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testclob"), largestring);
            assertEquals(cur.getFieldLength((UInt64)0, "testblob"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, "testblob"), largebuffer,
                                LARGE_BUFFER_LENGTH);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // output bind by position
            Console.WriteLine("OUTPUT BIND BY POSITION: ");
            cur.sendQuery("drop procedure testproc");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery(
                "create procedure testproc " +
                "	@out1 int output, " +
                "	@out2 varchar(20) output, " +
                "	@out3 float output, " +
                "	@out4 datetime output, " +
                "	@out5 varchar(20) output as " +
                "select @out1=1, " +
                "	@out2='hello', " +
                "	@out3=2.5, " +
                "	@out4='2001-02-03', " +
                "	@out5=null"));
            cur.prepareQuery("exec testproc");
            assertEquals(cur.countBindVariables(), (UInt16)0);
            cur.defineOutputBindInteger("1");
            cur.defineOutputBindString("2", 20);
            cur.defineOutputBindDouble("3");
            cur.defineOutputBindDate("4");
            cur.defineOutputBindString("5", 20);
            assertTrue(cur.executeQuery());
            numvar = cur.getOutputBindInteger("1");
            stringvar = cur.getOutputBindString("2");
            floatvar = cur.getOutputBindDouble("3");
            year = cur.getOutputBindDateYear("4");
            month = cur.getOutputBindDateMonth("4");
            day = cur.getOutputBindDateDay("4");
            hour = cur.getOutputBindDateHour("4");
            minute = cur.getOutputBindDateMinute("4");
            second = cur.getOutputBindDateSecond("4");
            microsecond = cur.getOutputBindDateMicrosecond("4");
            tz = cur.getOutputBindDateTz("4");
            isnegative = cur.getOutputBindDateIsNegative("4");
            nullvar = cur.getOutputBindString("5");
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
            assertEquals(nullvar, (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // output bind by name
            Console.WriteLine("OUTPUT BIND BY NAME: ");
            cur.sendQuery("drop procedure testproc");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery(
                "create procedure testproc " +
                "	@out1 int output, " +
                "	@out2 varchar(20) output, " +
                "	@out3 float output, " +
                "	@out4 datetime output, " +
                "	@out5 varchar(20) output as " +
                "select @out1=1, " +
                "	@out2='hello', " +
                "	@out3=2.5, " +
                "	@out4='2001-02-03', " +
                "	@out5=null"));
            cur.prepareQuery("exec testproc");
            assertEquals(cur.countBindVariables(), (UInt16)0);
            cur.defineOutputBindInteger("out1");
            cur.defineOutputBindString("out2", 20);
            cur.defineOutputBindDouble("out3");
            cur.defineOutputBindDate("out4");
            cur.defineOutputBindString("out5", 20);
            assertTrue(cur.executeQuery());
            numvar = cur.getOutputBindInteger("out1");
            stringvar = cur.getOutputBindString("out2");
            floatvar = cur.getOutputBindDouble("out3");
            year = cur.getOutputBindDateYear("out4");
            month = cur.getOutputBindDateMonth("out4");
            day = cur.getOutputBindDateDay("out4");
            hour = cur.getOutputBindDateHour("out4");
            minute = cur.getOutputBindDateMinute("out4");
            second = cur.getOutputBindDateSecond("out4");
            microsecond = cur.getOutputBindDateMicrosecond("out4");
            tz = cur.getOutputBindDateTz("out4");
            isnegative = cur.getOutputBindDateIsNegative("out4");
            nullvar = cur.getOutputBindString("out5");
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
            assertEquals(nullvar, (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // output bind by name with validation
            // validateBinds() can't be used for output binds, with sap.  In sap,
            // when executing a procedure, you don't declare any bind variable
            // delimiters in the query.  eg, you just do: "exec testproc", not
            // "exec testproc(@out1,@out2)".  If you call validateBinds(), it won't
            // find any binds in the query, and will filter out any binds that you
            // declare.


            // lob output bind
            // sap doesn't support lobs as output parameters to stored procedures,
            // and there's no way to directly select into a lob bind variable


            // long output bind
            Console.WriteLine("LONG OUTPUT BIND: ");
            cur.sendQuery("drop procedure testproc");
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebuffer[i] = (Byte)'C';
            }
            largebuffer[LARGE_BUFFER_LENGTH] = (Byte)'\0';
            largestring = new String('C', LARGE_BUFFER_LENGTH);
            String query =
                "create procedure testproc " +
                "@bindval varchar(" + LARGE_BUFFER_LENGTH + ") output as " +
                "set @bindval='" + largestring + "'";
            assertTrue(cur.sendQuery(query));
            cur.prepareQuery("exec testproc");
            cur.defineOutputBindString("bindval", LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindLength("bindval"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getOutputBindString("bindval"), largestring);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery("create table testtable (testval int)");
            cur.prepareQuery("insert into testtable values (@testval)");
            cur.inputBind("testval", (Int64)(-1));
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testval from testtable");
            assertEquals(cur.getField((UInt64)0, "testval"), "-1");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // bind validation
            Console.WriteLine("BIND VALIDATION: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(
                "create table testtable (" +
                "	col1 varchar(20), " +
                "	col2 varchar(20), " +
                "	col3 varchar(20))");
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	$(var1), " +
                "	$(var2), " +
                "	$(var3))");
            cur.inputBind("var1", "1");
            cur.inputBind("var2", "2");
            cur.inputBind("var3", "3");
            cur.substitution("var1", "@var1");
            assertTrue(cur.validBind("var1"));
            assertFalse(cur.validBind("var2"));
            assertFalse(cur.validBind("var3"));
            assertFalse(cur.validBind("var4"));
            Console.WriteLine("");
            cur.substitution("var2", "@var2");
            assertTrue(cur.validBind("var1"));
            assertTrue(cur.validBind("var2"));
            assertFalse(cur.validBind("var3"));
            assertFalse(cur.validBind("var4"));
            Console.WriteLine("");
            cur.substitution("var3", "@var3");
            assertTrue(cur.validBind("var1"));
            assertTrue(cur.validBind("var2"));
            assertTrue(cur.validBind("var3"));
            assertFalse(cur.validBind("var4"));
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // rebinding
            Console.WriteLine("REBINDING: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc " +
                "	@in1 int, " +
                "	@out1 int output as " +
                "select @out1=@in1"));
            cur.prepareQuery("exec testproc");
            cur.inputBind("in1", (Int64)1);
            cur.defineOutputBindInteger("out1");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out1"), (Int64)1);
            cur.inputBind("in1", (Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out1"), (Int64)2);
            cur.inputBind("in1", (Int64)3);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out1"), (Int64)3);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // reexecute
            Console.WriteLine("REEXECUTE: ");
            cur.prepareQuery("select 1");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.prepareQuery(
                "begin " +
                "	select cast(@var1 as int) " +
                "end");
            cur.inputBind("var1", (Int64)1);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.inputBind("var1", (Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "2");
            Console.WriteLine("");


            // stored procedure returning no value
            Console.WriteLine("STORED PROCEDURE RETURNING NO VALUE: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc " +
                "	@in1 int, " +
                "	@in2 float, " +
                "	@in3 varchar(20) as " +
                "return"));
            cur.prepareQuery("exec testproc");
            cur.inputBind("in1", (Int64)1);
            cur.inputBind("in2", 1.1, 2, 1);
            cur.inputBind("in3", "hello");
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // stored procedure returning single value
            Console.WriteLine("STORED PROCEDURE RETURNING SINGLE VALUE: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc " +
                "	@in1 int, " +
                "	@in2 float, " +
                "	@in3 varchar(20), " +
                "	@out1 int output as " +
                "select @out1=@in1"));
            cur.prepareQuery("exec testproc");
            cur.inputBind("in1", (Int64)1);
            cur.inputBind("in2", 1.1, 2, 1);
            cur.inputBind("in3", "hello");
            cur.defineOutputBindInteger("out1");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out1"), (Int64)1);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // stored procedure returning multiple values
            Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc @in1 int, " +
                "	@in2 float, " +
                "	@in3 varchar(20), " +
                "	@out1 int output, " +
                "	@out2 float output, " +
                "	@out3 varchar(20) output as " +
                "select @out1=@in1, " +
                "	@out2=@in2, " +
                "	@out3=@in3"));
            cur.prepareQuery("exec testproc");
            cur.inputBind("in1", (Int64)1);
            cur.inputBind("in2", 1.1, 2, 1);
            cur.inputBind("in3", "hello");
            cur.defineOutputBindInteger("out1");
            cur.defineOutputBindDouble("out2");
            cur.defineOutputBindString("out3", 20);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out1"), (Int64)1);
            assertEquals(cur.getOutputBindDouble("out2"), 1.1);
            assertEquals(cur.getOutputBindString("out3"), "hello");
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // stored procedure returning result set
            Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
            cur.sendQuery("drop procedure testselectproc");
            assertTrue(cur.sendQuery(
                "create procedure testselectproc as " +
                "	select 1 " +
                "	union " +
                "	select 2 " +
                "	union " +
                "	select 3 " +
                "	union " +
                "	select 4 " +
                "	union " +
                "	select 5 " +
                "	union " +
                "	select 6 " +
                "	union " +
                "	select 7 " +
                "	union " +
                "	select 8"));
            assertTrue(cur.sendQuery("exec testselectproc"));
            assertEquals(cur.rowCount(), (UInt64)8);
            assertTrue(cur.sendQuery("drop procedure testselectproc"));
            Console.WriteLine("");


            // temporary tables
            Console.WriteLine("TEMPORARY TABLES: ");
            cur.sendQuery("drop table #temptable\n");
            cur.sendQuery("create table #temptable (col1 int)");
            assertTrue(cur.sendQuery("insert into #temptable values (1)"));
            assertTrue(cur.sendQuery("select count(*) from #temptable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            con.endSession();
            Console.WriteLine("");
            assertFalse(cur.sendQuery("select count(*) from #temptable"));
            Console.WriteLine("");


            // encoded binary data
            Console.WriteLine("ENCODED BINARY DATA: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 image)"));
            Byte[] buffer = new Byte[256];
            for (int i = 0; i < 256; i++)
            {
                buffer[i] = (Byte)i;
            }
            System.Text.StringBuilder querystr = new System.Text.StringBuilder();
            querystr.Append("insert into testtable values (0x");
            for (int i = 0; i < buffer.Length; i++)
            {
                querystr.Append(String.Format("{0:x2}", buffer[i]));
            }
            querystr.Append(")");
            assertTrue(cur.sendQuery(querystr.ToString()));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)buffer.Length);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, (UInt32)0), buffer);
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
                    "	(col1 int identity primary key, " +
                    "	col2 int)"));
            assertTrue(cur.sendQuery(
                    "insert into testtable (col2) values (1)"));
            assertEquals(con.getLastInsertId(), (UInt64)1);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // database is schema
            Console.WriteLine("DATABASE IS SCHEMA: ");
            assertFalse(con.getDatabaseIsSchema());
            Console.WriteLine("");


            // catalog list
            Console.WriteLine("CATALOG LIST: ");
            assertTrue(cur.getCatalogList(null));
            assertEquals(cur.getColumnName(0), "Database");
            assertTrue(cur.rowCount() > 0);
            Console.WriteLine("");


            // schema list
            Console.WriteLine("SCHEMA LIST: ");
            cur.sendQuery("drop table testtable");
            // the get schema list query that is used with sap will only return the
            // names of schemas that have at least one database object in them, so
            // to be sure that there is one, we'll create a table
            assertTrue(cur.sendQuery("create table testtable (col1 int)"));
            assertTrue(cur.getSchemaList(null));
            assertEquals(cur.getColumnName(0), "Database");
            assertTrue(cur.rowCount() > 0);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName(0), "table_type");
            assertTrue(cur.rowCount() > 0);
            Boolean found = false;
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
                "	col1 int, " +
                "	col2 int)"));
            assertTrue(cur.sendQuery(
                "create table testtable2 (" +
                "	col1 int, " +
                "	col2 int)"));
            assertTrue(cur.sendQuery(
                "create table testtable3 (" +
                "	col1 int, " +
                "	col2 int)"));
            assertTrue(cur.sendQuery(
                "create table testtable4 (" +
                "	col1 int, " +
                "	col2 int)"));
            assertTrue(cur.getTableList(null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                String name = cur.getField(i, "Tables_in_xxx");
                if (name == "testtable1" ||
                    name == "testtable2" ||
                    name == "testtable3" ||
                    name == "testtable4")
                {
                    counter++;
                }
            }
            assertEquals(counter, (UInt64)4);
            assertTrue(cur.sendQuery("drop table testtable1"));
            assertTrue(cur.sendQuery("drop table testtable2"));
            assertTrue(cur.sendQuery("drop table testtable3"));
            assertTrue(cur.sendQuery("drop table testtable4"));
            Console.WriteLine("");


            // type info list
            Console.WriteLine("TYPE INFO LIST: ");
            assertTrue(cur.getTypeInfoList("int"));
            assertEquals(cur.getColumnName(0), "type_name");
            assertEquals(cur.getColumnName(1), "data_type");
            assertEquals(cur.getColumnName(2), "precision");
            assertEquals(cur.getColumnName(3), "literal_prefix");
            assertEquals(cur.getColumnName(4), "literal_suffix");
            assertEquals(cur.getColumnName(5), "create_params");
            assertEquals(cur.getColumnName(6), "nullable");
            assertEquals(cur.getColumnName(7), "case_sensitive");
            assertEquals(cur.getColumnName(8), "searchable");
            assertEquals(cur.getColumnName(9), "unsigned_attribute");
            assertEquals(cur.getColumnName(10), "fixed_prec_scale");
            assertEquals(cur.getColumnName(11), "auto_increment");
            assertEquals(cur.getColumnName(12), "local_type_name");
            assertEquals(cur.getColumnName(13), "minumum_scale");
            assertEquals(cur.getColumnName(14), "maxiumm_scale");
            assertEquals(cur.getColumnName(15), "sql_data_type");
            assertEquals(cur.getColumnName(16), "sql_datetime_sub");
            assertEquals(cur.getColumnName(17), "num_prec_radix");
            assertEquals(cur.getColumnName(18), "interval_precision");
            assertEquals(cur.getField((UInt64)0, "type_name"), "INT");
            assertEquals(cur.getField((UInt64)0, "data_type"), "4");
            assertEquals(cur.getField((UInt64)0, "precision"), "10");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "INT");
            assertTrue(cur.getTypeInfoList("char"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "CHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "1");
            assertEquals(cur.getField((UInt64)0, "precision"), "255");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "CHAR");
            assertTrue(cur.getTypeInfoList("varchar"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "VARCHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "12");
            assertEquals(cur.getField((UInt64)0, "precision"), "255");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "VARCHAR");
            assertTrue(cur.getTypeInfoList("datetime"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "DATETIME");
            assertEquals(cur.getField((UInt64)0, "data_type"), "93");
            assertEquals(cur.getField((UInt64)0, "precision"), "23");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "DATETIME");
            Console.WriteLine("");


            // column list
            Console.WriteLine("COLUMN LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testint int, " +
                "	testsmallint smallint, " +
                "	testtinyint tinyint, " +
                "	testreal real, " +
                "	testfloat float, " +
                "	testdecimal decimal(4,1), " +
                "	testnumeric numeric(4,1), " +
                "	testmoney money, " +
                "	testsmallmoney smallmoney, " +
                "	testdatetime datetime, " +
                "	testsmalldatetime smalldatetime, " +
                "	testchar char(40), " +
                "	testvarchar varchar(40), " +
                "	testbit bit, " +
                "	testtext text)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertEquals(cur.getColumnName(0), "column_name");
            assertEquals(cur.getColumnName(1), "data_type");
            assertEquals(cur.getColumnName(2), "character_maximum_length");
            assertEquals(cur.getColumnName(3), "numeric_precision");
            assertEquals(cur.getColumnName(4), "numeric_scale");
            assertEquals(cur.getColumnName(5), "is_nullable");
            assertEquals(cur.getColumnName(6), "column_key");
            assertEquals(cur.getColumnName(7), "column_default");
            assertEquals(cur.getColumnName(8), "extra");
            assertTrue(cur.getField((UInt64)0, "column_name") == "testint");
            assertTrue(cur.getField((UInt64)1, "column_name") == "testsmallint");
            assertTrue(cur.getField((UInt64)2, "column_name") == "testtinyint");
            assertTrue(cur.getField((UInt64)3, "column_name") == "testreal");
            assertTrue(cur.getField((UInt64)4, "column_name") == "testfloat");
            assertTrue(cur.getField((UInt64)5, "column_name") == "testdecimal");
            assertTrue(cur.getField((UInt64)6, "column_name") == "testnumeric");
            assertTrue(cur.getField((UInt64)7, "column_name") == "testmoney");
            assertTrue(cur.getField((UInt64)8, "column_name") == "testsmallmoney");
            assertTrue(cur.getField((UInt64)9, "column_name") == "testdatetime");
            assertTrue(cur.getField((UInt64)10, "column_name") == "testsmalldatetime");
            assertTrue(cur.getField((UInt64)11, "column_name") == "testchar");
            assertTrue(cur.getField((UInt64)12, "column_name") == "testvarchar");
            assertTrue(cur.getField((UInt64)13, "column_name") == "testbit");
            assertTrue(cur.getField((UInt64)14, "column_name") == "testtext");
            assertTrue(cur.getField((UInt64)0, "data_type") == "int");
            assertTrue(cur.getField((UInt64)1, "data_type") == "smallint");
            assertTrue(cur.getField((UInt64)2, "data_type") == "tinyint");
            assertTrue(cur.getField((UInt64)3, "data_type") == "real");
            assertTrue(cur.getField((UInt64)4, "data_type") == "float");
            assertTrue(cur.getField((UInt64)5, "data_type") == "decimal");
            assertTrue(cur.getField((UInt64)6, "data_type") == "numeric");
            assertTrue(cur.getField((UInt64)7, "data_type") == "money");
            assertTrue(cur.getField((UInt64)8, "data_type") == "smallmoney");
            assertTrue(cur.getField((UInt64)9, "data_type") == "datetime");
            assertTrue(cur.getField((UInt64)10, "data_type") == "smalldatetime");
            assertTrue(cur.getField((UInt64)11, "data_type") == "char");
            assertTrue(cur.getField((UInt64)12, "data_type") == "varchar");
            assertTrue(cur.getField((UInt64)13, "data_type") == "bit");
            assertTrue(cur.getField((UInt64)14, "data_type") == "text");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // column list - auto_increment, primary key
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int identity primary key, " +
                "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertTrue(cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertFalse(cur.getField((UInt64)1, "extra").Contains("auto_increment"));
            assertFalse(cur.getField((UInt64)1, "column_key").Contains("PRI"));
            Console.WriteLine("");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int primary key, " +
                "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertFalse(cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // primary keys list
            Console.WriteLine("PRIMARY KEYS LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int primary key, " +
                "	col2 int)"));
            assertTrue(cur.getPrimaryKeysList("testtable", null));
            assertEquals(cur.getColumnName(0), "table");
            assertEquals(cur.getColumnName(1), "non_unique");
            assertEquals(cur.getColumnName(2), "key_name");
            assertEquals(cur.getColumnName(3), "seq_in_index");
            assertEquals(cur.getColumnName(4), "column_name");
            assertEquals(cur.getColumnName(5), "collation");
            assertEquals(cur.getColumnName(6), "cardinality");
            assertEquals(cur.getColumnName(7), "sub_part");
            assertEquals(cur.getColumnName(8), "packed");
            assertEquals(cur.getColumnName(9), "null");
            assertEquals(cur.getColumnName(10), "index_type");
            assertEquals(cur.getColumnName(11), "comment");
            assertEquals(cur.getColumnName(12), "index_comment");
            assertEquals(cur.rowCount(), (UInt64)1);
            assertTrue(cur.getField((UInt64)0, "table") == "testtable");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertTrue(cur.getField((UInt64)0, "column_name") == "col1");
            assertTrue(!String.IsNullOrEmpty(cur.getField((UInt64)0, "key_name")));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // key and index list
            Console.WriteLine("KEY AND INDEX LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int primary key, " +
                "	col2 int)"));
            assertTrue(cur.getKeyAndIndexList("testtable", null));
            assertEquals(cur.getColumnName(0), "table");
            assertEquals(cur.getColumnName(1), "non_unique");
            assertEquals(cur.getColumnName(2), "key_name");
            assertEquals(cur.getColumnName(3), "seq_in_index");
            assertEquals(cur.getColumnName(4), "column_name");
            assertEquals(cur.getColumnName(5), "collation");
            assertEquals(cur.getColumnName(6), "cardinality");
            assertEquals(cur.getColumnName(7), "sub_part");
            assertEquals(cur.getColumnName(8), "packed");
            assertEquals(cur.getColumnName(9), "null");
            assertEquals(cur.getColumnName(10), "index_type");
            assertEquals(cur.getColumnName(11), "comment");
            assertEquals(cur.getColumnName(12), "index_comment");
            assertEquals(cur.rowCount(), (UInt64)1);
            assertTrue(cur.getField((UInt64)0, "table") == "testtable");
            assertEquals(cur.getField((UInt64)0, "non_unique"), "FALSE");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertTrue(cur.getField((UInt64)0, "column_name") == "col1");
            assertEquals(cur.getField((UInt64)0, "collation"), "A");
            assertEquals(cur.getField((UInt64)0, "index_type"), "1");
            assertTrue(!String.IsNullOrEmpty(cur.getField((UInt64)0, "key_name")));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // procedure list
            Console.WriteLine("PROCEDURE LIST: ");
            cur.sendQuery("drop procedure testproc1");
            cur.sendQuery("drop procedure testproc2");
            cur.sendQuery("drop procedure testproc3");
            cur.sendQuery("drop procedure testproc4");
            assertTrue(cur.sendQuery(
                "create procedure testproc1 " +
                "	@in1 int, " +
                "	@in2 char(20), " +
                "	@in3 varchar(20), " +
                "	@in4 datetime " +
                "as select 1"));
            assertTrue(cur.sendQuery(
                "create procedure testproc2 " +
                "	@in1 int, " +
                "	@in2 char(20), " +
                "	@in3 varchar(20), " +
                "	@in4 datetime " +
                "as select 1"));
            assertTrue(cur.sendQuery(
                "create procedure testproc3 " +
                "	@in1 int, " +
                "	@in2 char(20), " +
                "	@in3 varchar(20), " +
                "	@in4 datetime " +
                "as select 1"));
            assertTrue(cur.sendQuery(
                "create procedure testproc4 " +
                "	@in1 int, " +
                "	@in2 char(20), " +
                "	@in3 varchar(20), " +
                "	@in4 datetime " +
                "as select 1"));
            assertTrue(cur.getProcedureList(null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                String name = cur.getField(i, "routine_name");
                if (name == "testproc1" ||
                    name == "testproc2" ||
                    name == "testproc3" ||
                    name == "testproc4")
                {
                    counter++;
                }
            }
            assertEquals(counter, (UInt64)4);
            Console.WriteLine("");


            // procedure parameter list
            Console.WriteLine("PROCEDURE PARAMETER LIST: ");
            assertTrue(cur.getProcedureParameterList("testproc1", null));
            assertEquals(cur.getColumnName(0), "parameter_name");
            assertEquals(cur.getColumnName(1), "parameter_mode");
            assertEquals(cur.getColumnName(2), "data_type");
            assertEquals(cur.getColumnName(3), "character_maximum_length");
            assertEquals(cur.getColumnName(4), "ordinal_position");
            assertEquals(cur.rowCount(), (UInt64)4);
            assertEquals(cur.getField((UInt64)0, "parameter_name"), "@in1");
            assertEquals(cur.getField((UInt64)0, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)0, "data_type"), "int");
            assertEquals(cur.getField((UInt64)0, "ordinal_position"), "1");
            assertEquals(cur.getField((UInt64)1, "parameter_name"), "@in2");
            assertEquals(cur.getField((UInt64)1, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)1, "data_type"), "char");
            assertEquals(cur.getField((UInt64)1, "ordinal_position"), "2");
            assertEquals(cur.getField((UInt64)2, "parameter_name"), "@in3");
            assertEquals(cur.getField((UInt64)2, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)2, "data_type"), "varchar");
            assertEquals(cur.getField((UInt64)2, "ordinal_position"), "3");
            assertEquals(cur.getField((UInt64)3, "parameter_name"), "@in4");
            assertEquals(cur.getField((UInt64)3, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)3, "data_type"), "datetime");
            assertEquals(cur.getField((UInt64)3, "ordinal_position"), "4");
            assertTrue(cur.sendQuery("drop procedure testproc1"));
            assertTrue(cur.sendQuery("drop procedure testproc2"));
            assertTrue(cur.sendQuery("drop procedure testproc3"));
            assertTrue(cur.sendQuery("drop procedure testproc4"));
            Console.WriteLine("");


            // invalid queries
            Console.WriteLine("INVALID QUERIES: ");
            assertFalse(cur.sendQuery("select * from testtable order by testint"));
            assertFalse(cur.sendQuery("select * from testtable order by testint"));
            assertFalse(cur.sendQuery("select * from testtable order by testint"));
            assertFalse(cur.sendQuery("select * from testtable order by testint"));
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
