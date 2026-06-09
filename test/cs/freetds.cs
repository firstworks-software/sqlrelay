// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class FreetdsTest : SQLRTest
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
            UInt16 port;
            String socket;
            UInt16 id;
            String filename;

            int LARGE_BUFFER_LENGTH = 8192;
            char[] largebufferchars = new char[LARGE_BUFFER_LENGTH];
            String largebuffer;


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                    "testuser", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "freetds");
            Console.WriteLine("");


            // ping
            Console.WriteLine("PING: ");
            assertTrue(con.ping());
            Console.WriteLine("");


            // transaction state
            Console.WriteLine("TRANSACTION STATE: ");
            assertEquals(con.getDefaultTransactionModel(), "explicit-error");
            assertEquals(con.getTransactionModel(), "explicit-error");
            assertFalse(con.getInTransaction());
            assertTrue(con.getAutoCommit());
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
            foreach (String il in isolationlevels)
            {
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
            cur.sendQuery(dumptran);
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	testint int, "
                + "	testsmallint smallint, "
                + "	testtinyint tinyint, "
                + "	testreal real, "
                + "	testfloat float, "
                + "	testdecimal decimal(4,1), "
                + "	testnumeric numeric(4,1), "
                + "	testmoney money, "
                + "	testsmallmoney smallmoney, "
                + "	testdatetime datetime, "
                + "	testsmalldatetime smalldatetime, "
                + "	testchar char(40), "
                + "	testvarchar varchar(40), "
                + "	testbit bit) lock datarows"));
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            assertTrue(con.begin());
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	1, "
                + "	1, "
                + "	1, "
                + "	1.5, "
                + "	1.5, "
                + "	1.5, "
                + "	1.5, "
                + "	1.00, "
                + "	1.00, "
                + "	'01-Jan-2001 01:00:00', "
                + "	'01-Jan-2001 01:00:00', "
                + "	'testchar1', "
                + "	'testvarchar1', "
                + "	1)"));
            Console.WriteLine("");


            // affected rows
            Console.WriteLine("AFFECTED ROWS: ");
            assertEquals(cur.affectedRows(), (UInt64)1);
            Console.WriteLine("");


            // input bind by position
            Console.WriteLine("INPUT BIND BY POSITION: ");
            cur.prepareQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?)");
            assertEquals(cur.countBindVariables(), (UInt16)14);
            cur.inputBind("1", (Int64)2);
            cur.inputBind("2", (Int64)2);
            cur.inputBind("3", (Int64)2);
            cur.inputBind("4", 2.5, 2, 1);
            cur.inputBind("5", 2.5, 2, 1);
            cur.inputBind("6", 2.5, 2, 1);
            cur.inputBind("7", 2.5, 2, 1);
            cur.inputBind("8", 2.00, 3, 2);
            cur.inputBind("9", 2.00, 3, 2);
            cur.inputBind("10", "01-Jan-2002 02:00:00");
            cur.inputBind("11", "01-Jan-2002 02:00:00");
            cur.inputBind("12", "testchar2");
            cur.inputBind("13", "testvarchar2");
            cur.inputBind("14", (Int64)1);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)3);
            cur.inputBind("2", (Int64)3);
            cur.inputBind("3", (Int64)3);
            cur.inputBind("4", 3.5, 2, 1);
            cur.inputBind("5", 3.5, 2, 1);
            cur.inputBind("6", 3.5, 2, 1);
            cur.inputBind("7", 3.5, 2, 1);
            cur.inputBind("8", 3.00, 3, 2);
            cur.inputBind("9", 3.00, 3, 2);
            cur.inputBind("10", "01-Jan-2003 03:00:00");
            cur.inputBind("11", "01-Jan-2003 03:00:00");
            cur.inputBind("12", "testchar3");
            cur.inputBind("13", "testvarchar3");
            cur.inputBind("14", (Int64)1);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            // freetds doesn't support implicit conversion of string binds to other
            // data types, so arrays of binds don't generally work.


            // input bind by position with validation
            Console.WriteLine("INPUT BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1", (Int64)4);
            cur.inputBind("2", (Int64)4);
            cur.inputBind("3", (Int64)4);
            cur.inputBind("4", 4.5, 2, 1);
            cur.inputBind("5", 4.5, 2, 1);
            cur.inputBind("6", 4.5, 2, 1);
            cur.inputBind("7", 4.5, 2, 1);
            cur.inputBind("8", 4.00, 3, 2);
            cur.inputBind("9", 4.00, 3, 2);
            cur.inputBind("10", "01-Jan-2004 04:00:00");
            cur.inputBind("11", "01-Jan-2004 04:00:00");
            cur.inputBind("12", "testchar4");
            cur.inputBind("13", "testvarchar4");
            cur.inputBind("14", (Int64)1);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by name
            Console.WriteLine("INPUT BIND BY NAME: ");
            cur.clearBinds();
            cur.prepareQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	@var1, "
                + "	@var2, "
                + "	@var3, "
                + "	@var4, "
                + "	@var5, "
                + "	@var6, "
                + "	@var7, "
                + "	@var8, "
                + "	@var9, "
                + "	@var10, "
                + "	@var11, "
                + "	@var12, "
                + "	@var13, "
                + "	@var14)");
            assertEquals(cur.countBindVariables(), (UInt16)14);
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
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("var1", (Int64)6);
            cur.inputBind("var2", (Int64)6);
            cur.inputBind("var3", (Int64)6);
            cur.inputBind("var4", 6.5, 2, 1);
            cur.inputBind("var5", 6.5, 2, 1);
            cur.inputBind("var6", 6.5, 2, 1);
            cur.inputBind("var7", 6.5, 2, 1);
            cur.inputBind("var8", 6.00, 3, 2);
            cur.inputBind("var9", 6.00, 3, 2);
            cur.inputBind("var10", "01-Jan-2006 06:00:00");
            cur.inputBind("var11", "01-Jan-2006 06:00:00");
            cur.inputBind("var12", "testchar6");
            cur.inputBind("var13", "testvarchar6");
            cur.inputBind("var14", (Int64)1);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("var1", (Int64)7);
            cur.inputBind("var2", (Int64)7);
            cur.inputBind("var3", (Int64)7);
            cur.inputBind("var4", 7.5, 2, 1);
            cur.inputBind("var5", 7.5, 2, 1);
            cur.inputBind("var6", 7.5, 2, 1);
            cur.inputBind("var7", 7.5, 2, 1);
            cur.inputBind("var8", 7.00, 3, 2);
            cur.inputBind("var9", 7.00, 3, 2);
            cur.inputBind("var10", "01-Jan-2007 07:00:00");
            cur.inputBind("var11", "01-Jan-2007 07:00:00");
            cur.inputBind("var12", "testchar7");
            cur.inputBind("var13", "testvarchar7");
            cur.inputBind("var14", (Int64)1);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by name
            // freetds doesn't support implicit conversion of string binds to other
            // data types, so arrays of binds don't generally work.


            // input bind by name with validation
            Console.WriteLine("INPUT BIND BY NAME WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("var1", (Int64)8);
            cur.inputBind("var2", (Int64)8);
            cur.inputBind("var3", (Int64)8);
            cur.inputBind("var4", 8.5, 2, 1);
            cur.inputBind("var5", 8.5, 2, 1);
            cur.inputBind("var6", 8.5, 2, 1);
            cur.inputBind("var7", 8.5, 2, 1);
            cur.inputBind("var8", 8.00, 3, 2);
            cur.inputBind("var9", 8.00, 3, 2);
            cur.inputBind("var10", "01-Jan-2008 08:00:00");
            cur.inputBind("var11", "01-Jan-2008 08:00:00");
            cur.inputBind("var12", "testchar8");
            cur.inputBind("var13", "testvarchar8");
            cur.inputBind("var14", (Int64)1);
            cur.inputBind("var15", "junkvalue");
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(), (UInt32)14);
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
            assertEquals(cur.getColumnType(1), "SMALLINT");
            assertEquals(cur.getColumnType("testsmallint"), "SMALLINT");
            assertEquals(cur.getColumnType(2), "TINYINT");
            assertEquals(cur.getColumnType("testtinyint"), "TINYINT");
            assertEquals(cur.getColumnType(3), "REAL");
            assertEquals(cur.getColumnType("testreal"), "REAL");
            assertEquals(cur.getColumnType(4), "FLOAT");
            assertEquals(cur.getColumnType("testfloat"), "FLOAT");
            assertEquals(cur.getColumnType(5), "DECIMAL");
            assertEquals(cur.getColumnType("testdecimal"), "DECIMAL");
            assertEquals(cur.getColumnType(6), "NUMERIC");
            assertEquals(cur.getColumnType("testnumeric"), "NUMERIC");
            assertEquals(cur.getColumnType(7), "MONEY");
            assertEquals(cur.getColumnType("testmoney"), "MONEY");
            assertEquals(cur.getColumnType(8), "SMALLMONEY");
            assertEquals(cur.getColumnType("testsmallmoney"), "SMALLMONEY");
            assertEquals(cur.getColumnType(9), "DATETIME");
            assertEquals(cur.getColumnType("testdatetime"), "DATETIME");
            assertEquals(cur.getColumnType(10), "SMALLDATETIME");
            assertEquals(cur.getColumnType("testsmalldatetime"), "SMALLDATETIME");
            assertEquals(cur.getColumnType(11), "CHAR");
            assertEquals(cur.getColumnType("testchar"), "CHAR");
            assertEquals(cur.getColumnType(12), "CHAR");
            assertEquals(cur.getColumnType("testvarchar"), "CHAR");
            assertEquals(cur.getColumnType(13), "BIT");
            assertEquals(cur.getColumnType("testbit"), "BIT");
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)4);
            assertEquals(cur.getColumnLength("testint"), (UInt32)4);
            assertEquals(cur.getColumnLength(1), (UInt32)2);
            assertEquals(cur.getColumnLength("testsmallint"), (UInt32)2);
            assertEquals(cur.getColumnLength(2), (UInt32)1);
            assertEquals(cur.getColumnLength("testtinyint"), (UInt32)1);
            assertEquals(cur.getColumnLength(3), (UInt32)4);
            assertEquals(cur.getColumnLength("testreal"), (UInt32)4);
            assertEquals(cur.getColumnLength(4), (UInt32)8);
            assertEquals(cur.getColumnLength("testfloat"), (UInt32)8);
            // freetds reports the decimal/numeric display length as 35
            assertEquals(cur.getColumnLength(5),(UInt32)35);
            assertEquals(cur.getColumnLength("testdecimal"),(UInt32)35);
            assertEquals(cur.getColumnLength(6),(UInt32)35);
            assertEquals(cur.getColumnLength("testnumeric"),(UInt32)35);
            assertEquals(cur.getColumnLength(7), (UInt32)8);
            assertEquals(cur.getColumnLength("testmoney"), (UInt32)8);
            assertEquals(cur.getColumnLength(8), (UInt32)4);
            assertEquals(cur.getColumnLength("testsmallmoney"), (UInt32)4);
            assertEquals(cur.getColumnLength(9), (UInt32)8);
            assertEquals(cur.getColumnLength("testdatetime"), (UInt32)8);
            assertEquals(cur.getColumnLength(10), (UInt32)4);
            assertEquals(cur.getColumnLength("testsmalldatetime"), (UInt32)4);
            // char(40)/varchar(40) report the declared length 40 (not multiplied)
            assertEquals(cur.getColumnLength(11),(UInt32)40);
            assertEquals(cur.getColumnLength("testchar"),(UInt32)40);
            assertEquals(cur.getColumnLength(12),(UInt32)40);
            assertEquals(cur.getColumnLength("testvarchar"),(UInt32)40);
            assertEquals(cur.getColumnLength(13), (UInt32)1);
            assertEquals(cur.getColumnLength("testbit"), (UInt32)1);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest("testint"), (UInt32)1);
            assertEquals(cur.getLongest(1), (UInt32)1);
            assertEquals(cur.getLongest("testsmallint"), (UInt32)1);
            assertEquals(cur.getLongest(2), (UInt32)1);
            assertEquals(cur.getLongest("testtinyint"), (UInt32)1);
            assertEquals(cur.getLongest(3), (UInt32)3);
            assertEquals(cur.getLongest("testreal"), (UInt32)3);
            assertEquals(cur.getLongest(4), (UInt32)3);
            assertEquals(cur.getLongest("testfloat"), (UInt32)3);
            assertEquals(cur.getLongest(5), (UInt32)3);
            assertEquals(cur.getLongest("testdecimal"), (UInt32)3);
            assertEquals(cur.getLongest(6), (UInt32)3);
            assertEquals(cur.getLongest("testnumeric"), (UInt32)3);
            assertEquals(cur.getLongest(7), (UInt32)6);
            assertEquals(cur.getLongest("testmoney"), (UInt32)6);
            assertEquals(cur.getLongest(8), (UInt32)6);
            assertEquals(cur.getLongest("testsmallmoney"), (UInt32)6);
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getLongest(9),(UInt32)26);
            assertEquals(cur.getLongest("testdatetime"),(UInt32)26);
            assertEquals(cur.getLongest(10),(UInt32)26);
            assertEquals(cur.getLongest("testsmalldatetime"),(UInt32)26);
            assertEquals(cur.getLongest(11), (UInt32)40);
            assertEquals(cur.getLongest("testchar"), (UInt32)40);
            assertEquals(cur.getLongest(12), (UInt32)12);
            assertEquals(cur.getLongest("testvarchar"), (UInt32)12);
            assertEquals(cur.getLongest(13), (UInt32)1);
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
            assertEquals(cur.getField((UInt64)0, (UInt32)3), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)4), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)5), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)6), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)7), "1.0000");
            assertEquals(cur.getField((UInt64)0, (UInt32)8), "1.0000");
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getField((UInt64)0,(UInt32)9),"Jan  1 2001 01:00:00:000AM");
            assertEquals(cur.getField((UInt64)0,(UInt32)10),"Jan  1 2001 01:00:00:000AM");
            assertEquals(cur.getField((UInt64)0, (UInt32)11), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, (UInt32)12), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)13), "1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)3), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)4), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)5), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)6), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)7), "8.0000");
            assertEquals(cur.getField((UInt64)7, (UInt32)8), "8.0000");
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getField((UInt64)7,(UInt32)9),"Jan  1 2008 08:00:00:000AM");
            assertEquals(cur.getField((UInt64)7,(UInt32)10),"Jan  1 2008 08:00:00:000AM");
            assertEquals(cur.getField((UInt64)7, (UInt32)11), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, (UInt32)12), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)13), "1");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)4), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)5), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)6), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)7), (UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)8), (UInt32)6);
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)9),(UInt32)26);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)10),(UInt32)26);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)11), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)12), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)13), (UInt32)1);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)4), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)5), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)6), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)7), (UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)8), (UInt32)6);
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)9),(UInt32)26);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)10),(UInt32)26);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)11), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)12), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)13), (UInt32)1);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "testint"), "1");
            assertEquals(cur.getField((UInt64)0, "testsmallint"), "1");
            assertEquals(cur.getField((UInt64)0, "testtinyint"), "1");
            assertEquals(cur.getField((UInt64)0, "testreal"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testfloat"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testdecimal"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testnumeric"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testmoney"), "1.0000");
            assertEquals(cur.getField((UInt64)0, "testsmallmoney"), "1.0000");
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getField((UInt64)0,"testdatetime"),"Jan  1 2001 01:00:00:000AM");
            assertEquals(cur.getField((UInt64)0,"testsmalldatetime"),"Jan  1 2001 01:00:00:000AM");
            assertEquals(cur.getField((UInt64)0, "testchar"), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, "testvarchar"), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, "testbit"), "1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "testint"), "8");
            assertEquals(cur.getField((UInt64)7, "testsmallint"), "8");
            assertEquals(cur.getField((UInt64)7, "testtinyint"), "8");
            assertEquals(cur.getField((UInt64)7, "testreal"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testfloat"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testdecimal"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testnumeric"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testmoney"), "8.0000");
            assertEquals(cur.getField((UInt64)7, "testsmallmoney"), "8.0000");
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getField((UInt64)7,"testdatetime"),"Jan  1 2008 08:00:00:000AM");
            assertEquals(cur.getField((UInt64)7,"testsmalldatetime"),"Jan  1 2008 08:00:00:000AM");
            assertEquals(cur.getField((UInt64)7, "testchar"), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, "testvarchar"), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, "testbit"), "1");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testtinyint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testreal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testdecimal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testnumeric"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testmoney"), (UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallmoney"), (UInt32)6);
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getFieldLength((UInt64)0,"testdatetime"),(UInt32)26);
            assertEquals(cur.getFieldLength((UInt64)0,"testsmalldatetime"),(UInt32)26);
            assertEquals(cur.getFieldLength((UInt64)0, "testchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, "testbit"), (UInt32)1);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testtinyint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testreal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testdecimal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testnumeric"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testmoney"), (UInt32)6);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallmoney"), (UInt32)6);
            // freetds datetime rendering for the fixture tds version
            assertEquals(cur.getFieldLength((UInt64)7,"testdatetime"),(UInt32)26);
            assertEquals(cur.getFieldLength((UInt64)7,"testsmalldatetime"),(UInt32)26);
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
            assertEquals(fields[3], "1.5");
            assertEquals(fields[4], "1.5");
            assertEquals(fields[5], "1.5");
            assertEquals(fields[6], "1.5");
            assertEquals(fields[7], "1.0000");
            assertEquals(fields[8], "1.0000");
            // freetds datetime rendering for the fixture tds version
            assertEquals(fields[9],"Jan  1 2001 01:00:00:000AM");
            assertEquals(fields[10],"Jan  1 2001 01:00:00:000AM");
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
            assertEquals(fieldlens[3], (UInt32)3);
            assertEquals(fieldlens[4], (UInt32)3);
            assertEquals(fieldlens[5], (UInt32)3);
            assertEquals(fieldlens[6], (UInt32)3);
            assertEquals(fieldlens[7], (UInt32)6);
            assertEquals(fieldlens[8], (UInt32)6);
            // freetds datetime rendering for the fixture tds version
            assertEquals(fieldlens[9],(UInt32)26);
            assertEquals(fieldlens[10],(UInt32)26);
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
            assertEquals(cur.getColumnName((UInt32)0), "testint");
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
            assertEquals(cur.colCount(), (UInt32)14);
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
            Console.WriteLine("FROM ONE CACHE FILE TO ANOTHER "
                        + "WITH RESULT SET BUFFER SIZE: ");
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
            Console.WriteLine("CACHED RESULT SET WITH SUSPEND "
                        + "AND RESULT SET BUFFER SIZE: ");
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
            // can't do this with freetds
            //cur.setResultSetBufferSize(1);
            assertTrue(cur.sendQuery("select * from testtable"));
            secondcur = new SQLRCursor(con);
            secondcur.setResultSetBufferSize(1);
            UInt64 nestedrows = 0;
            for (UInt32 i = 0; cur.getRow((UInt64)i) != null; i++)
            {
                assertTrue(secondcur.sendQuery("select * from testtable"));
                nestedrows++;
            }
            // the nested selects must not disturb the outer result set
            assertEquals(nestedrows, cur.rowCount());
            secondcur.closeResultSet();
            //cur.setResultSetBufferSize(0);
            assertTrue(con.commit());
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // reset transaction state
            Console.WriteLine("RESET TRANSACTION STATE: ");
            assertTrue(con.commit());
            assertEquals(con.getTransactionModel(), "explicit-error");
            assertTrue(con.getAutoCommit());
            Console.WriteLine("");


            // transaction behavior - implicit
            Console.WriteLine("TRANSACTION BEHAVIOR - implicit: ");
            // sap ase rejects DDL inside a chained-mode (multi-statement) tx
            // unless `sp_dboption ... 'ddl in tran', true` is set on the db;
            // create the table while still in unchained mode, then switch.
            // `lock datarows` is needed so secondcur's count(*) scan doesn't
            // block on the writer's page lock from the in-flight insert
            assertTrue(cur.sendQuery(
                "create table testtable (col1 integer) lock datarows"));
            assertTrue(con.setTransactionModel("implicit"));
            assertEquals(con.getTransactionModel(), "implicit");
            secondcon = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                    "testuser", "testpassword", 0, 1);
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
            // switch back to unchained mode so the drop isn't rejected
            assertTrue(con.setTransactionModel("explicit-error"));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior - explicit
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit: ");
            assertTrue(con.setTransactionModel("explicit"));
            assertEquals(con.getTransactionModel(), "explicit");
            assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
            // switch back to unchained mode so the drop isn't rejected
            assertTrue(con.setTransactionModel("explicit-error"));
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
            assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
            // switch back to unchained mode so the drop isn't rejected
            assertTrue(con.setTransactionModel("explicit-error"));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior - explicit-error
            Console.WriteLine("TRANSACTION BEHAVIOR - explicit-error: ");
            assertTrue(con.setTransactionModel("explicit-error"));
            assertEquals(con.getTransactionModel(), "explicit-error");
            assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
            // commit the open tx so the drop isn't rejected as DDL inside a
            // chained-mode transaction (in explicit-error model, autoCommitOn
            // from inside a tx errors out by design, so commit is the route
            // back to autocommit-on / unchained mode)
            assertTrue(con.commit());
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior - none
            Console.WriteLine("TRANSACTION BEHAVIOR - none: ");
            assertTrue(con.setTransactionModel("none"));
            assertEquals(con.getTransactionModel(), "none");
            assertTrue(cur.sendQuery("create table testtable (col1 integer) lock datarows"));
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
            assertEquals(con.getTransactionModel(), "explicit-error");
            assertTrue(con.getAutoCommit());
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
                "create table testtable ("
                + "	testclob1 text NULL, "
                + "	testclob2 text NULL, "
                + "	testblob1 image NULL, "
                + "	testblob2 image NULL)"));
            cur.prepareQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?)");
            cur.inputBindClob("1", "", 0);
            cur.inputBindClob("2", (String)null, 0);
            cur.inputBindBlob("3", System.Text.Encoding.ASCII.GetBytes(""), (UInt32)0);
            cur.inputBindBlob("4", (Byte[])null, (UInt32)0);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            // sap converts empty strings to a single space.  It's possible that
            // if we had true input bind support on the backend, then this would
            // work correctly, but for now we're faking binds, and inserting an
            // empty string, so we have to check for a single space here.
            assertEquals(cur.getField((UInt64)0, (UInt32)0), " ");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), (String)null);
            // sap doesn't really support inserting an empty string into a binary
            // column.  The minimum that can be inserted is a single \0.  That ends
            // up being interpreted as an empty string here, but it's actualy a
            // single \0 character, not zero characters.
            {
                String fld = cur.getField((UInt64)0, (UInt32)2);
                if (fld == null)
                {
                    fld = "";
                }
                else
                {
                    int nul = fld.IndexOf('\0');
                    if (nul != -1)
                    {
                        fld = fld.Substring(0, nul);
                    }
                }
                assertEquals(fld, "");
            }
            assertEquals(cur.getField((UInt64)0, (UInt32)3), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // long lobs
            Console.WriteLine("LONG LOBS: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(
                "create table testtable ("
                + "	testclob text, "
                + "	testblob image) lock datarows");
            cur.prepareQuery("insert into testtable values (?,?)");
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebufferchars[i] = 'C';
            }
            largebuffer = new String(largebufferchars);
            cur.inputBindClob("1", largebuffer, (UInt32)LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("2", System.Text.Encoding.ASCII.GetBytes(largebuffer), (UInt32)LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "testclob"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testclob"), largebuffer);
            assertEquals(cur.getFieldLength((UInt64)0, "testblob"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, "testblob"),
                                System.Text.Encoding.ASCII.GetBytes(largebuffer),
                                LARGE_BUFFER_LENGTH);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // output bind by position
            // FreeTDS needs to support cursors for this to work


            // output bind by name
            // FreeTDS needs to support cursors for this to work


            // output bind by name with validation
            // Even if FreeTDS supported cursors...
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
            // FreeTDS needs to support cursors for this to work


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
                "create table testtable ("
                + "	col1 varchar(20), "
                + "	col2 varchar(20), "
                + "	col3 varchar(20))");
            cur.prepareQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	$(var1), "
                + "	$(var2), "
                + "	$(var3))");
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
            // FreeTDS needs to support cursors for this to work


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
            cur.prepareQuery("select cast(? as int)");
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
            // FreeTDS needs to support cursors for this to work


            // stored procedure returning single value
            // FreeTDS needs to support cursors for this to work


            // stored procedure returning multiple values
            // FreeTDS needs to support cursors for this to work


            // stored procedure returning result set
            Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
            cur.sendQuery("drop procedure testselectproc");
            assertTrue(cur.sendQuery(
                "create procedure testselectproc as "
                + "       select 1 "
                + "       union "
                + "       select 2 "
                + "       union "
                + "       select 3 "
                + "       union "
                + "       select 4 "
                + "       union "
                + "       select 5 "
                + "       union "
                + "       select 6 "
                + "       union "
                + "       select 7 "
                + "       union "
                + "       select 8"));
            assertTrue(cur.sendQuery("exec testselectproc"));
            assertEquals(cur.rowCount(), (UInt64)8);
            assertTrue(cur.sendQuery("drop procedure testselectproc"));
            Console.WriteLine("");


            // temporary tables
            Console.WriteLine("TEMPORARY TABLES: ");
            cur.sendQuery("drop table #temptable");
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
            for (UInt16 i = 0; i < 256; i++)
            {
                buffer[i] = (Byte)i;
            }
            System.Text.StringBuilder query = new System.Text.StringBuilder();
            query.Append("insert into testtable values (0x");
            for (UInt64 i = 0; i < (UInt64)buffer.Length; i++)
            {
                query.Append(String.Format("{0:x2}", buffer[i]));
            }
            query.Append(")");
            assertTrue(cur.sendQuery(query.ToString()));
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
                    "create table testtable "
                    + "	(col1 int identity primary key, "
                    + "	col2 int)"));
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
            assertInResultSet(cur, "Database", hostname);
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
            assertInResultSet(cur, "Database", "dbo");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName(0), "table_type");
            assertInResultSet(cur, "table_type", "TABLE");
            Console.WriteLine("");


            // table list
            Console.WriteLine("TABLE LIST: ");
            cur.sendQuery("drop table testtable1");
            cur.sendQuery("drop table testtable2");
            cur.sendQuery("drop table testtable3");
            cur.sendQuery("drop table testtable4");
            assertTrue(cur.sendQuery(
                "create table testtable1 ("
                + "	col1 int, "
                + "	col2 int)"));
            assertTrue(cur.sendQuery(
                "create table testtable2 ("
                + "	col1 int, "
                + "	col2 int)"));
            assertTrue(cur.sendQuery(
                "create table testtable3 ("
                + "	col1 int, "
                + "	col2 int)"));
            assertTrue(cur.sendQuery(
                "create table testtable4 ("
                + "	col1 int, "
                + "	col2 int)"));
            assertTrue(cur.getTableList(null));
            assertInResultSet(cur, "Tables_in_xxx", "testtable1");
            assertInResultSet(cur, "Tables_in_xxx", "testtable2");
            assertInResultSet(cur, "Tables_in_xxx", "testtable3");
            assertInResultSet(cur, "Tables_in_xxx", "testtable4");
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
            assertEquals(cur.getField((UInt64)0, "precision"), "8000");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "CHAR");
            assertTrue(cur.getTypeInfoList("varchar"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "VARCHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "12");
            assertEquals(cur.getField((UInt64)0, "precision"), "8000");
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
                "create table testtable ("
                + "	testint int, "
                + "	testsmallint smallint, "
                + "	testtinyint tinyint, "
                + "	testreal real, "
                + "	testfloat float, "
                + "	testdecimal decimal(4,1), "
                + "	testnumeric numeric(4,1), "
                + "	testmoney money, "
                + "	testsmallmoney smallmoney, "
                + "	testdatetime datetime, "
                + "	testsmalldatetime smalldatetime, "
                + "	testchar char(40), "
                + "	testvarchar varchar(40), "
                + "	testbit bit)"));
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
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // column list - auto_increment, primary key
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	col1 int identity primary key, "
                + "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertEquals(cur.getField((UInt64)0, "extra"), "auto_increment");
            assertEquals(cur.getField((UInt64)0, "column_key"), "PRI");
            assertEquals(cur.getField((UInt64)1, "extra"), "");
            assertEquals(cur.getField((UInt64)1, "column_key"), "");
            Console.WriteLine("");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	col1 int primary key, "
                + "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertEquals(cur.getField((UInt64)0, "extra"), "");
            assertEquals(cur.getField((UInt64)0, "column_key"), "PRI");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // primary keys list
            Console.WriteLine("PRIMARY KEYS LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	col1 int primary key, "
                + "	col2 int)"));
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
                "create table testtable ("
                + "	col1 int primary key, "
                + "	col2 int)"));
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
            assertEquals(cur.getField((UInt64)0, "non_unique"), "0");
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
                "create procedure testproc1 "
                + "	@in1 int, "
                + "	@in2 char(20), "
                + "	@in3 varchar(20), "
                + "	@in4 datetime "
                + "as select 1"));
            assertTrue(cur.sendQuery(
                "create procedure testproc2 "
                + "	@in1 int, "
                + "	@in2 char(20), "
                + "	@in3 varchar(20), "
                + "	@in4 datetime "
                + "as select 1"));
            assertTrue(cur.sendQuery(
                "create procedure testproc3 "
                + "	@in1 int, "
                + "	@in2 char(20), "
                + "	@in3 varchar(20), "
                + "	@in4 datetime "
                + "as select 1"));
            assertTrue(cur.sendQuery(
                "create procedure testproc4 "
                + "	@in1 int, "
                + "	@in2 char(20), "
                + "	@in3 varchar(20), "
                + "	@in4 datetime "
                + "as select 1"));
            assertTrue(cur.getProcedureList(null));
            assertInResultSet(cur, "routine_name", "testproc1");
            assertInResultSet(cur, "routine_name", "testproc2");
            assertInResultSet(cur, "routine_name", "testproc3");
            assertInResultSet(cur, "routine_name", "testproc4");
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
