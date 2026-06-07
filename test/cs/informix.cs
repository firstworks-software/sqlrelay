// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class InformixTest : SQLRTest
    {
        public static int Main(string[] args)
        {

            String[] isolationlevels = new String[] {
                        "committed read","dirty read",
                        "cursor stability","repeatable read"};
            String[] bindvars = new String[] {"1","2","3","4",
                        "5","6","7","8","9","10",
                        "11","12","13","14","15","16"};
            String[] bindvals = new String[] {"t","7","7","7","7",
                        "7.5","7.5","7.5","7.5",
                        "testchar7","testnchar7",
                        "testvarchar7","testnvarchar7",
                        "testlvarchar7","01/01/2007",
                        "2007-01-01 07:00:00"};
            String[] cols;
            String[] fields;
            UInt32[] fieldlens;
            String[] subvars = new String[] {"var1","var2","var3"};
            Int64[] subvallongs = new Int64[] {1,2,3};
            String[] subvalstrings = new String[] {"hi","hello","bye"};
            Double[] subvaldoubles = new Double[] {10.55,10.556,10.5556};
            UInt32[] precs = new UInt32[] {4,5,6};
            UInt32[] scales = new UInt32[] {2,3,4};
            Int64 numvar;
            String stringvar;
            String nullvar;
            String clobvar;
            UInt32 clobvarlength;
            Byte[] blobvar;
            UInt32 blobvarlength;
            Double floatvar;
            UInt16 port;
            String socket;
            UInt16 id;
            String filename;
            UInt64 counter = 0;

            Int32 LARGE_BUFFER_LENGTH = 20 * 1024;
            char[] largebufferchars = new char[LARGE_BUFFER_LENGTH + 1];
            String largebuffer;


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                    "testuser", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "informix");
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
            assertEquals(con.nextvalFormat(), "%s.nextval");
            Console.WriteLine("");


            // isolation levels
            Console.WriteLine("ISOLATION LEVELS: ");
            for (int i = 0; i < isolationlevels.Length; i++)
            {
                // you can set the isolation level, but to get it, you have to
                // have permissions to read from sysmaster:syssqlcurses
                assertTrue(con.setIsolationLevel(isolationlevels[i]));
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
                "	testboolean boolean, " +
                "	testsmallint smallint, " +
                "	testint integer, " +
                "	testbigint bigint, " +
                "	testint8 int8, " +
                "	testdecimal decimal(10,2), " +
                "	testmoney money, " +
                "	testsmallfloat smallfloat, " +
                "	testfloat float, " +
                "	testchar char(40), " +
                "	testnchar nchar(40), " +
                "	testvarchar varchar(40), " +
                "	testnvarchar nvarchar(40), " +
                "	testlvarchar lvarchar(40), " +
                "	testdate date, " +
                "	testdatetime datetime year to second, " +
                "	testtext text, " +
                "	testbyte byte)"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	't', " +
                "	1, " +
                "	1, " +
                "	1, " +
                "	1, " +
                "	1.5, " +
                "	1.5, " +
                "	1.5, " +
                "	1.5, " +
                "	'testchar1', " +
                "	'testnchar1', " +
                "	'testvarchar1', " +
                "	'testnvarchar1', " +
                "	'testlvarchar1', " +
                "	'01/01/2001', " +
                "	'2001-01-01 01:00:00', " +
                "	'testtext1', " +
                "	null)"));
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
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?)");
            assertEquals(cur.countBindVariables(), (UInt16)18);
            cur.inputBind("1", "t");
            cur.inputBind("2", (Int64)2);
            cur.inputBind("3", (Int64)2);
            cur.inputBind("4", (Int64)2);
            cur.inputBind("5", (Int64)2);
            cur.inputBind("6", 2.5, (UInt32)4, (UInt32)2);
            cur.inputBind("7", 2.5, (UInt32)4, (UInt32)2);
            cur.inputBind("8", 2.5, (UInt32)4, (UInt32)2);
            cur.inputBind("9", 2.5, (UInt32)4, (UInt32)2);
            cur.inputBind("10", "testchar2");
            cur.inputBind("11", "testnchar2");
            cur.inputBind("12", "testvarchar2");
            cur.inputBind("13", "testnvarchar2");
            cur.inputBind("14", "testlvarchar2");
            cur.inputBind("15", (Int16)2002, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("16", (Int16)2002, (Int16)1, (Int16)1, (Int16)2, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("17", "testtext2", (UInt32)9);
            cur.inputBindBlob("18", System.Text.Encoding.ASCII.GetBytes("testbyte2"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", "t");
            cur.inputBind("2", (Int64)3);
            cur.inputBind("3", (Int64)3);
            cur.inputBind("4", (Int64)3);
            cur.inputBind("5", (Int64)3);
            cur.inputBind("6", 3.5, (UInt32)4, (UInt32)2);
            cur.inputBind("7", 3.5, (UInt32)4, (UInt32)2);
            cur.inputBind("8", 3.5, (UInt32)4, (UInt32)2);
            cur.inputBind("9", 3.5, (UInt32)4, (UInt32)2);
            cur.inputBind("10", "testchar3");
            cur.inputBind("11", "testnchar3");
            cur.inputBind("12", "testvarchar3");
            cur.inputBind("13", "testnvarchar3");
            cur.inputBind("14", "testlvarchar3");
            cur.inputBind("15", (Int16)2003, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("16", (Int16)2003, (Int16)1, (Int16)1, (Int16)3, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("17", "testtext3", (UInt32)9);
            cur.inputBindBlob("18", System.Text.Encoding.ASCII.GetBytes("testbyte3"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", "t");
            cur.inputBind("2", (Int64)4);
            cur.inputBind("3", (Int64)4);
            cur.inputBind("4", (Int64)4);
            cur.inputBind("5", (Int64)4);
            cur.inputBind("6", 4.5, (UInt32)4, (UInt32)2);
            cur.inputBind("7", 4.5, (UInt32)4, (UInt32)2);
            cur.inputBind("8", 4.5, (UInt32)4, (UInt32)2);
            cur.inputBind("9", 4.5, (UInt32)4, (UInt32)2);
            cur.inputBind("10", "testchar4");
            cur.inputBind("11", "testnchar4");
            cur.inputBind("12", "testvarchar4");
            cur.inputBind("13", "testnvarchar4");
            cur.inputBind("14", "testlvarchar4");
            cur.inputBind("15", (Int16)2004, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("16", (Int16)2004, (Int16)1, (Int16)1, (Int16)4, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("17", "testtext4", (UInt32)9);
            cur.inputBindBlob("18", System.Text.Encoding.ASCII.GetBytes("testbyte4"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", "t");
            cur.inputBind("2", (Int64)5);
            cur.inputBind("3", (Int64)5);
            cur.inputBind("4", (Int64)5);
            cur.inputBind("5", (Int64)5);
            cur.inputBind("6", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("7", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("8", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("9", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("10", "testchar5");
            cur.inputBind("11", "testnchar5");
            cur.inputBind("12", "testvarchar5");
            cur.inputBind("13", "testnvarchar5");
            cur.inputBind("14", "testlvarchar5");
            cur.inputBind("15", (Int16)2005, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("16", (Int16)2005, (Int16)1, (Int16)1, (Int16)5, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("17", "testtext5", (UInt32)9);
            cur.inputBindBlob("18", System.Text.Encoding.ASCII.GetBytes("testbyte5"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", "t");
            cur.inputBind("2", (Int64)6);
            cur.inputBind("3", (Int64)6);
            cur.inputBind("4", (Int64)6);
            cur.inputBind("5", (Int64)6);
            cur.inputBind("6", 6.5, (UInt32)4, (UInt32)2);
            cur.inputBind("7", 6.5, (UInt32)4, (UInt32)2);
            cur.inputBind("8", 6.5, (UInt32)4, (UInt32)2);
            cur.inputBind("9", 6.5, (UInt32)4, (UInt32)2);
            cur.inputBind("10", "testchar6");
            cur.inputBind("11", "testnchar6");
            cur.inputBind("12", "testvarchar6");
            cur.inputBind("13", "testnvarchar6");
            cur.inputBind("14", "testlvarchar6");
            cur.inputBind("15", (Int16)2006, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("16", (Int16)2006, (Int16)1, (Int16)1, (Int16)6, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("17", "testtext6", (UInt32)9);
            cur.inputBindBlob("18", System.Text.Encoding.ASCII.GetBytes("testbyte6"), (UInt32)9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            Console.WriteLine("ARRAY OF INPUT BINDS BY POSITION: ");
            cur.clearBinds();
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
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	?, " +
                "	null, " +
                "	null)");
            cur.inputBind(bindvars, bindvals);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by position with validation
            Console.WriteLine("INPUT BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1", "t");
            cur.inputBind("2", (Int64)8);
            cur.inputBind("3", (Int64)8);
            cur.inputBind("4", (Int64)8);
            cur.inputBind("5", (Int64)8);
            cur.inputBind("6", 8.5, (UInt32)4, (UInt32)2);
            cur.inputBind("7", 8.5, (UInt32)4, (UInt32)2);
            cur.inputBind("8", 8.5, (UInt32)4, (UInt32)2);
            cur.inputBind("9", 8.5, (UInt32)4, (UInt32)2);
            cur.inputBind("10", "testchar8");
            cur.inputBind("11", "testnchar8");
            cur.inputBind("12", "testvarchar8");
            cur.inputBind("13", "testnvarchar8");
            cur.inputBind("14", "testlvarchar8");
            cur.inputBind("15", (Int16)2008, (Int16)1, (Int16)1, (Int16)(-1), (Int16)(-1), (Int16)(-1), (Int32)(-1), (String)null, false);
            cur.inputBind("16", (Int16)2008, (Int16)1, (Int16)1, (Int16)8, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBindClob("17", "testtext8", (UInt32)9);
            cur.inputBindBlob("18", System.Text.Encoding.ASCII.GetBytes("testbyte8"), (UInt32)9);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by name
            // informix doesn't support bind by name


            // array of input binds by name
            // informix doesn't support bind by name


            // input bind by name with validation
            // informix doesn't support bind by name


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
            assertEquals(cur.colCount(), (UInt32)18);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName((UInt32)0), "testboolean");
            assertEquals(cur.getColumnName((UInt32)1), "testsmallint");
            assertEquals(cur.getColumnName((UInt32)2), "testint");
            assertEquals(cur.getColumnName((UInt32)3), "testbigint");
            assertEquals(cur.getColumnName((UInt32)4), "testint8");
            assertEquals(cur.getColumnName((UInt32)5), "testdecimal");
            assertEquals(cur.getColumnName((UInt32)6), "testmoney");
            assertEquals(cur.getColumnName((UInt32)7), "testsmallfloat");
            assertEquals(cur.getColumnName((UInt32)8), "testfloat");
            assertEquals(cur.getColumnName((UInt32)9), "testchar");
            assertEquals(cur.getColumnName((UInt32)10), "testnchar");
            assertEquals(cur.getColumnName((UInt32)11), "testvarchar");
            assertEquals(cur.getColumnName((UInt32)12), "testnvarchar");
            assertEquals(cur.getColumnName((UInt32)13), "testlvarchar");
            assertEquals(cur.getColumnName((UInt32)14), "testdate");
            assertEquals(cur.getColumnName((UInt32)15), "testdatetime");
            assertEquals(cur.getColumnName((UInt32)16), "testtext");
            assertEquals(cur.getColumnName((UInt32)17), "testbyte");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testboolean");
            assertEquals(cols[1], "testsmallint");
            assertEquals(cols[2], "testint");
            assertEquals(cols[3], "testbigint");
            assertEquals(cols[4], "testint8");
            assertEquals(cols[5], "testdecimal");
            assertEquals(cols[6], "testmoney");
            assertEquals(cols[7], "testsmallfloat");
            assertEquals(cols[8], "testfloat");
            assertEquals(cols[9], "testchar");
            assertEquals(cols[10], "testnchar");
            assertEquals(cols[11], "testvarchar");
            assertEquals(cols[12], "testnvarchar");
            assertEquals(cols[13], "testlvarchar");
            assertEquals(cols[14], "testdate");
            assertEquals(cols[15], "testdatetime");
            assertEquals(cols[16], "testtext");
            assertEquals(cols[17], "testbyte");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            assertEquals(cur.getColumnType((UInt32)0), "BOOLEAN");
            assertEquals(cur.getColumnType("testboolean"), "BOOLEAN");
            assertEquals(cur.getColumnType((UInt32)1), "SMALLINT");
            assertEquals(cur.getColumnType("testsmallint"), "SMALLINT");
            assertEquals(cur.getColumnType((UInt32)2), "INTEGER");
            assertEquals(cur.getColumnType("testint"), "INTEGER");
            assertEquals(cur.getColumnType((UInt32)3), "BIGINT");
            assertEquals(cur.getColumnType("testbigint"), "BIGINT");
            assertEquals(cur.getColumnType((UInt32)4), "INT8");
            assertEquals(cur.getColumnType("testint8"), "INT8");
            assertEquals(cur.getColumnType((UInt32)5), "DECIMAL");
            assertEquals(cur.getColumnType("testdecimal"), "DECIMAL");
            //assertEquals(cur.getColumnType((UInt32)6),"MONEY");
            //assertEquals(cur.getColumnType("testmoney"),"MONEY");
            assertEquals(cur.getColumnType((UInt32)6), "DECIMAL");
            assertEquals(cur.getColumnType("testmoney"), "DECIMAL");
            assertEquals(cur.getColumnType((UInt32)7), "SMALLFLOAT");
            assertEquals(cur.getColumnType("testsmallfloat"), "SMALLFLOAT");
            assertEquals(cur.getColumnType((UInt32)8), "FLOAT");
            assertEquals(cur.getColumnType("testfloat"), "FLOAT");
            assertEquals(cur.getColumnType((UInt32)9), "CHAR");
            assertEquals(cur.getColumnType("testchar"), "CHAR");
            //assertEquals(cur.getColumnType((UInt32)10),"NCHAR");
            //assertEquals(cur.getColumnType("testnchar"),"NCHAR");
            assertEquals(cur.getColumnType((UInt32)10), "CHAR");
            assertEquals(cur.getColumnType("testnchar"), "CHAR");
            assertEquals(cur.getColumnType((UInt32)11), "VARCHAR");
            assertEquals(cur.getColumnType("testvarchar"), "VARCHAR");
            //assertEquals(cur.getColumnType((UInt32)12),"NVARCHAR");
            //assertEquals(cur.getColumnType("testnvarchar"),"NVARCHAR");
            assertEquals(cur.getColumnType((UInt32)12), "VARCHAR");
            assertEquals(cur.getColumnType("testnvarchar"), "VARCHAR");
            //assertEquals(cur.getColumnType((UInt32)13),"LVARCHAR");
            //assertEquals(cur.getColumnType("testlvarchar"),"LVARCHAR");
            assertEquals(cur.getColumnType((UInt32)13), "VARCHAR");
            assertEquals(cur.getColumnType("testlvarchar"), "VARCHAR");
            assertEquals(cur.getColumnType((UInt32)14), "DATE");
            assertEquals(cur.getColumnType("testdate"), "DATE");
            assertEquals(cur.getColumnType((UInt32)15), "DATETIME");
            assertEquals(cur.getColumnType("testdatetime"), "DATETIME");
            assertEquals(cur.getColumnType((UInt32)16), "TEXT");
            assertEquals(cur.getColumnType("testtext"), "TEXT");
            assertEquals(cur.getColumnType((UInt32)17), "BYTE");
            assertEquals(cur.getColumnType("testbyte"), "BYTE");
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)1);
            assertEquals(cur.getColumnLength("testboolean"), (UInt32)1);
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)5);
            assertEquals(cur.getColumnLength("testsmallint"), (UInt32)5);
            assertEquals(cur.getColumnLength((UInt32)2), (UInt32)10);
            assertEquals(cur.getColumnLength("testint"), (UInt32)10);
            assertEquals(cur.getColumnLength((UInt32)3), (UInt32)20);
            assertEquals(cur.getColumnLength("testbigint"), (UInt32)20);
            assertEquals(cur.getColumnLength((UInt32)4), (UInt32)20);
            assertEquals(cur.getColumnLength("testint8"), (UInt32)20);
            assertEquals(cur.getColumnLength((UInt32)5), (UInt32)10);
            assertEquals(cur.getColumnLength("testdecimal"), (UInt32)10);
            assertEquals(cur.getColumnLength((UInt32)6), (UInt32)16);
            assertEquals(cur.getColumnLength("testmoney"), (UInt32)16);
            assertEquals(cur.getColumnLength((UInt32)7), (UInt32)7);
            assertEquals(cur.getColumnLength("testsmallfloat"), (UInt32)7);
            assertEquals(cur.getColumnLength((UInt32)8), (UInt32)15);
            assertEquals(cur.getColumnLength("testfloat"), (UInt32)15);
            assertEquals(cur.getColumnLength((UInt32)9), (UInt32)40);
            assertEquals(cur.getColumnLength("testchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)10), (UInt32)40);
            assertEquals(cur.getColumnLength("testnchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)11), (UInt32)40);
            assertEquals(cur.getColumnLength("testvarchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)12), (UInt32)40);
            assertEquals(cur.getColumnLength("testnvarchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)13), (UInt32)40);
            assertEquals(cur.getColumnLength("testlvarchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)14), (UInt32)10);
            assertEquals(cur.getColumnLength("testdate"), (UInt32)10);
            assertEquals(cur.getColumnLength((UInt32)15), (UInt32)19);
            assertEquals(cur.getColumnLength("testdatetime"), (UInt32)19);
            assertEquals(cur.getColumnLength((UInt32)16), (UInt32)2147483647);
            assertEquals(cur.getColumnLength("testtext"), (UInt32)2147483647);
            //assertEquals(cur.getColumnLength((UInt32)17),(UInt32)2157483647);
            //assertEquals(cur.getColumnLength("testbyte"),(UInt32)2157483647);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest("testboolean"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)1), (UInt32)1);
            assertEquals(cur.getLongest("testsmallint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)2), (UInt32)1);
            assertEquals(cur.getLongest("testint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)3), (UInt32)1);
            assertEquals(cur.getLongest("testbigint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)4), (UInt32)1);
            assertEquals(cur.getLongest("testint8"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)5), (UInt32)4);
            assertEquals(cur.getLongest("testdecimal"), (UInt32)4);
            assertEquals(cur.getLongest((UInt32)6), (UInt32)4);
            assertEquals(cur.getLongest("testmoney"), (UInt32)4);
            assertEquals(cur.getLongest((UInt32)7), (UInt32)3);
            assertEquals(cur.getLongest("testsmallfloat"), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)8), (UInt32)3);
            assertEquals(cur.getLongest("testfloat"), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)9), (UInt32)40);
            assertEquals(cur.getLongest("testchar"), (UInt32)40);
            assertEquals(cur.getLongest((UInt32)10), (UInt32)40);
            assertEquals(cur.getLongest("testnchar"), (UInt32)40);
            assertEquals(cur.getLongest((UInt32)11), (UInt32)12);
            assertEquals(cur.getLongest("testvarchar"), (UInt32)12);
            assertEquals(cur.getLongest((UInt32)12), (UInt32)13);
            assertEquals(cur.getLongest("testnvarchar"), (UInt32)13);
            assertEquals(cur.getLongest((UInt32)13), (UInt32)13);
            assertEquals(cur.getLongest("testlvarchar"), (UInt32)13);
            assertEquals(cur.getLongest((UInt32)14), (UInt32)10);
            assertEquals(cur.getLongest("testdate"), (UInt32)10);
            assertEquals(cur.getLongest((UInt32)15), (UInt32)19);
            assertEquals(cur.getLongest("testdatetime"), (UInt32)19);
            assertEquals(cur.getLongest((UInt32)16), (UInt32)9);
            assertEquals(cur.getLongest("testtext"), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)17), (UInt32)9);
            assertEquals(cur.getLongest("testbyte"), (UInt32)9);
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
            assertEquals(cur.getField((UInt64)0, (UInt32)3), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)4), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)5), "1.50");
            assertEquals(cur.getField((UInt64)0, (UInt32)6), "1.50");
            assertEquals(cur.getField((UInt64)0, (UInt32)7), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)8), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)9), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, (UInt32)10), "testnchar1                              ");
            assertEquals(cur.getField((UInt64)0, (UInt32)11), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)12), "testnvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)13), "testlvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)14), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, (UInt32)15), "2001-01-01 01:00:00");
            assertEquals(cur.getField((UInt64)0, (UInt32)16), "testtext1");
            assertEquals(cur.getField((UInt64)0, (UInt32)17), "");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)3), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)4), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)5), "8.50");
            assertEquals(cur.getField((UInt64)7, (UInt32)6), "8.50");
            assertEquals(cur.getField((UInt64)7, (UInt32)7), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)8), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)9), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, (UInt32)10), "testnchar8                              ");
            assertEquals(cur.getField((UInt64)7, (UInt32)11), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)12), "testnvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)13), "testlvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)14), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, (UInt32)15), "2008-01-01 08:00:00");
            assertEquals(cur.getField((UInt64)7, (UInt32)16), "");
            assertEquals(cur.getField((UInt64)7, (UInt32)17), "");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)4), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)5), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)6), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)7), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)8), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)9), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)10), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)11), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)12), (UInt32)13);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)14), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)15), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)16), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)17), (UInt32)0);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)4), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)5), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)6), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)7), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)8), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)9), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)10), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)11), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)12), (UInt32)13);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)14), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)15), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)16), (UInt32)0);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)17), (UInt32)0);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "testboolean"), "1");
            assertEquals(cur.getField((UInt64)0, "testsmallint"), "1");
            assertEquals(cur.getField((UInt64)0, "testint"), "1");
            assertEquals(cur.getField((UInt64)0, "testbigint"), "1");
            assertEquals(cur.getField((UInt64)0, "testint8"), "1");
            assertEquals(cur.getField((UInt64)0, "testdecimal"), "1.50");
            assertEquals(cur.getField((UInt64)0, "testmoney"), "1.50");
            assertEquals(cur.getField((UInt64)0, "testsmallfloat"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testfloat"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testchar"), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, "testnchar"), "testnchar1                              ");
            assertEquals(cur.getField((UInt64)0, "testvarchar"), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, "testnvarchar"), "testnvarchar1");
            assertEquals(cur.getField((UInt64)0, "testlvarchar"), "testlvarchar1");
            assertEquals(cur.getField((UInt64)0, "testdate"), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, "testdatetime"), "2001-01-01 01:00:00");
            assertEquals(cur.getField((UInt64)0, "testtext"), "testtext1");
            assertEquals(cur.getField((UInt64)0, "testbyte"), "");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "testboolean"), "1");
            assertEquals(cur.getField((UInt64)7, "testsmallint"), "8");
            assertEquals(cur.getField((UInt64)7, "testint"), "8");
            assertEquals(cur.getField((UInt64)7, "testbigint"), "8");
            assertEquals(cur.getField((UInt64)7, "testint8"), "8");
            assertEquals(cur.getField((UInt64)7, "testdecimal"), "8.50");
            assertEquals(cur.getField((UInt64)7, "testmoney"), "8.50");
            assertEquals(cur.getField((UInt64)7, "testsmallfloat"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testfloat"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testchar"), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, "testnchar"), "testnchar8                              ");
            assertEquals(cur.getField((UInt64)7, "testvarchar"), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, "testnvarchar"), "testnvarchar8");
            assertEquals(cur.getField((UInt64)7, "testlvarchar"), "testlvarchar8");
            assertEquals(cur.getField((UInt64)7, "testdate"), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, "testdatetime"), "2008-01-01 08:00:00");
            assertEquals(cur.getField((UInt64)7, "testtext"), "");
            assertEquals(cur.getField((UInt64)7, "testbyte"), "");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "testboolean"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testbigint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testint8"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testdecimal"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, "testmoney"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, "testnchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, "testnvarchar"), (UInt32)13);
            assertEquals(cur.getFieldLength((UInt64)0, "testlvarchar"), (UInt32)13);
            assertEquals(cur.getFieldLength((UInt64)0, "testdate"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, "testdatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, "testtext"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "testbyte"), (UInt32)0);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "testboolean"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testbigint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testint8"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testdecimal"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, "testmoney"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, "testnchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, "testnvarchar"), (UInt32)13);
            assertEquals(cur.getFieldLength((UInt64)7, "testlvarchar"), (UInt32)13);
            assertEquals(cur.getFieldLength((UInt64)7, "testdate"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, "testdatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, "testtext"), (UInt32)0);
            assertEquals(cur.getFieldLength((UInt64)7, "testbyte"), (UInt32)0);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields = cur.getRow((UInt64)0);
            assertEquals(fields[0], "1");
            assertEquals(fields[1], "1");
            assertEquals(fields[2], "1");
            assertEquals(fields[3], "1");
            assertEquals(fields[4], "1");
            assertEquals(fields[5], "1.50");
            assertEquals(fields[6], "1.50");
            assertEquals(fields[7], "1.5");
            assertEquals(fields[8], "1.5");
            assertEquals(fields[9], "testchar1                               ");
            assertEquals(fields[10], "testnchar1                              ");
            assertEquals(fields[11], "testvarchar1");
            assertEquals(fields[12], "testnvarchar1");
            assertEquals(fields[13], "testlvarchar1");
            assertEquals(fields[14], "2001-01-01");
            assertEquals(fields[15], "2001-01-01 01:00:00");
            assertEquals(fields[16], "testtext1");
            assertEquals(fields[17], "");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens = cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0], (UInt32)1);
            assertEquals(fieldlens[1], (UInt32)1);
            assertEquals(fieldlens[2], (UInt32)1);
            assertEquals(fieldlens[3], (UInt32)1);
            assertEquals(fieldlens[4], (UInt32)1);
            assertEquals(fieldlens[5], (UInt32)4);
            assertEquals(fieldlens[6], (UInt32)4);
            assertEquals(fieldlens[7], (UInt32)3);
            assertEquals(fieldlens[8], (UInt32)3);
            assertEquals(fieldlens[9], (UInt32)40);
            assertEquals(fieldlens[10], (UInt32)40);
            assertEquals(fieldlens[11], (UInt32)12);
            assertEquals(fieldlens[12], (UInt32)13);
            assertEquals(fieldlens[14], (UInt32)10);
            assertEquals(fieldlens[15], (UInt32)19);
            assertEquals(fieldlens[16], (UInt32)9);
            assertEquals(fieldlens[17], (UInt32)0);
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
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)1), "3");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)2);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)4);
            assertEquals(cur.getField((UInt64)6, (UInt32)1), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            assertEquals(cur.getField((UInt64)8, (UInt32)1), (String)null);
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
            assertEquals(cur.getColumnName((UInt32)1), (String)null);
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)1), (String)null);
            cur.getColumnInfo();
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testsmallint "));
            assertEquals(cur.getColumnName((UInt32)1), "testsmallint");
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)5);
            assertEquals(cur.getColumnType((UInt32)1), "SMALLINT");
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
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)1), "3");
            assertEquals(cur.getField((UInt64)3, (UInt32)1), "4");
            assertEquals(cur.getField((UInt64)4, (UInt32)1), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)1), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)1), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
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
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)1), "3");
            assertEquals(cur.getField((UInt64)3, (UInt32)1), "4");
            assertEquals(cur.getField((UInt64)4, (UInt32)1), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)1), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)1), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
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
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)1), "3");
            assertEquals(cur.getField((UInt64)3, (UInt32)1), "4");
            assertEquals(cur.getField((UInt64)4, (UInt32)1), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)1), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)1), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
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
            assertEquals(cur.getField((UInt64)2, (UInt32)1), "3");
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
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            assertEquals(cur.getField((UInt64)8, (UInt32)1), (String)null);
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
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            Console.WriteLine("");


            // column count for cached result set
            Console.WriteLine("COLUMN COUNT FOR CACHED RESULT SET: ");
            assertEquals(cur.colCount(), (UInt32)18);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName((UInt32)0), "testboolean");
            assertEquals(cur.getColumnName((UInt32)1), "testsmallint");
            assertEquals(cur.getColumnName((UInt32)2), "testint");
            assertEquals(cur.getColumnName((UInt32)3), "testbigint");
            assertEquals(cur.getColumnName((UInt32)4), "testint8");
            assertEquals(cur.getColumnName((UInt32)5), "testdecimal");
            assertEquals(cur.getColumnName((UInt32)6), "testmoney");
            assertEquals(cur.getColumnName((UInt32)7), "testsmallfloat");
            assertEquals(cur.getColumnName((UInt32)8), "testfloat");
            assertEquals(cur.getColumnName((UInt32)9), "testchar");
            assertEquals(cur.getColumnName((UInt32)10), "testnchar");
            assertEquals(cur.getColumnName((UInt32)11), "testvarchar");
            assertEquals(cur.getColumnName((UInt32)12), "testnvarchar");
            assertEquals(cur.getColumnName((UInt32)13), "testlvarchar");
            assertEquals(cur.getColumnName((UInt32)14), "testdate");
            assertEquals(cur.getColumnName((UInt32)15), "testdatetime");
            assertEquals(cur.getColumnName((UInt32)16), "testtext");
            assertEquals(cur.getColumnName((UInt32)17), "testbyte");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testboolean");
            assertEquals(cols[1], "testsmallint");
            assertEquals(cols[2], "testint");
            assertEquals(cols[3], "testbigint");
            assertEquals(cols[4], "testint8");
            assertEquals(cols[5], "testdecimal");
            assertEquals(cols[6], "testmoney");
            assertEquals(cols[7], "testsmallfloat");
            assertEquals(cols[8], "testfloat");
            assertEquals(cols[9], "testchar");
            assertEquals(cols[10], "testnchar");
            assertEquals(cols[11], "testvarchar");
            assertEquals(cols[12], "testnvarchar");
            assertEquals(cols[13], "testlvarchar");
            assertEquals(cols[14], "testdate");
            assertEquals(cols[15], "testdatetime");
            assertEquals(cols[16], "testtext");
            assertEquals(cols[17], "testbyte");
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
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)1), (String)null);
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // from one cache file to another
            Console.WriteLine("FROM ONE CACHE FILE TO ANOTHER: ");
            cur.cacheToFile("cachefile2");
            assertTrue(cur.openCachedResultSet("cachefile1"));
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet("cachefile2"));
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)1), (String)null);
            Console.WriteLine("");


            // from one cache file to another with result set buffer size
            Console.WriteLine("FROM ONE CACHE FILE TO ANOTHER " +
                        "WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize((UInt64)2);
            cur.cacheToFile("cachefile2");
            assertTrue(cur.openCachedResultSet("cachefile1"));
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet("cachefile2"));
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)1), (String)null);
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
            assertEquals(cur.getField((UInt64)2, (UInt32)1), "3");
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
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)6);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            assertEquals(cur.getField((UInt64)8, (UInt32)1), (String)null);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)8);
            assertTrue(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)8);
            cur.cacheOff();
            Console.WriteLine("");
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)8, (UInt32)1), (String)null);
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // finished suspended session
            Console.WriteLine("FINISHED SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            assertEquals(cur.getField((UInt64)4, (UInt32)1), "5");
            assertEquals(cur.getField((UInt64)5, (UInt32)1), "6");
            assertEquals(cur.getField((UInt64)6, (UInt32)1), "7");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            id = cur.getResultSetId();
            cur.suspendResultSet();
            assertTrue(con.suspendSession());
            port = con.getConnectionPort();
            socket = con.getConnectionSocket();
            assertTrue(con.resumeSession(port, socket));
            assertTrue(cur.resumeResultSet(id));
            assertEquals(cur.getField((UInt64)4, (UInt32)1), (String)null);
            assertEquals(cur.getField((UInt64)5, (UInt32)1), (String)null);
            assertEquals(cur.getField((UInt64)6, (UInt32)1), (String)null);
            assertEquals(cur.getField((UInt64)7, (UInt32)1), (String)null);
            Console.WriteLine("");


            // nested selects
            Console.WriteLine("NESTED SELECTS: ");
            cur.setResultSetBufferSize((UInt64)1);
            assertTrue(cur.sendQuery("select * from testtable"));
            secondcur = new SQLRCursor(con);
            secondcur.setResultSetBufferSize((UInt64)1);
            for (UInt64 i = 0; cur.getRow(i) != null; i++)
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
            // Informix has no MVCC option -- the isolation level is either dirty
            // reads (where the second connection sees uncommitted rows) or
            // committed read (where it blocks or errors on locked rows) -- so
            // the visibility assertions below may need to be revisited
            Console.WriteLine("TRANSACTION BEHAVIOR - implicit: ");
            assertTrue(con.setTransactionModel("implicit"));
            assertEquals(con.getTransactionModel(), "implicit");
            assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
            // informix DDL is transactional in logged mode; commit so the table
            // is visible to the second connection (commit implicitly starts a
            // new tx)
            assertTrue(con.commit());
            secondcon = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                        "testuser", "testpassword", 0, 1);
            secondcur = new SQLRCursor(secondcon);
            // Informix has no MVCC; under default committed-read isolation,
            // secondcur's catalog/data read errors with "Cannot get system
            // information for table" while cur holds row locks from the
            // in-flight tx.  Use dirty-read on secondcur so it sees the
            // uncommitted writes — the test then verifies dirty-read
            // semantics instead of MVCC visibility.
            assertTrue(secondcur.sendQuery("set isolation to dirty read"));
            // session is in a transaction; insert is visible via dirty read
            assertTrue(con.getInTransaction());
            assertFalse(con.getAutoCommit());
            assertTrue(cur.sendQuery("insert into testtable values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
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
            // see note above re: informix dirty-read workaround
            assertTrue(secondcur.sendQuery("set isolation to dirty read"));
            // begin starts a new transaction; insert is visible via dirty read
            assertTrue(con.begin());
            assertTrue(con.getInTransaction());
            assertTrue(cur.sendQuery("insert into testtable values (1)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "1");
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
            // see note in - implicit section re: informix dirty-read workaround
            assertTrue(secondcur.sendQuery("set isolation to dirty read"));
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
            // explicitly commits/rollbacks the tx (mysql-native semantic).
            // dirty-read on secondcur sees the in-flight insert (count=2)
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable values (3)"));
            assertTrue(con.autoCommitOn());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "2");
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
            // dirty-read on secondcur sees the in-flight insert (count=5)
            assertTrue(con.autoCommitOn());
            assertTrue(con.getAutoCommit());
            assertTrue(con.begin());
            assertTrue(cur.sendQuery("insert into testtable values (7)"));
            assertTrue(con.autoCommitOff());
            assertFalse(con.getAutoCommit());
            assertTrue(con.getInTransaction());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "5");
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
            cur.prepareQuery(
                "select " +
                "	$(var1), " +
                "	'$(var2)', " +
                "	'$(var3)' " +
                "from " +
                "	sysmaster:sysdual ");
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
            cur.prepareQuery(
                "select " +
                "	'$(var1)', " +
                "	'$(var2)', " +
                "	'$(var3)' " +
                "from " +
                "	sysmaster:sysdual ");
            cur.substitutions(subvars, subvalstrings);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "hi");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "bye");
            Console.WriteLine("");
            cur.prepareQuery(
                "select " +
                "	$(var1), " +
                "	$(var2), " +
                "	$(var3) " +
                "from " +
                "	sysmaster:sysdual ");
            cur.substitution(subvars, subvallongs);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "3");
            Console.WriteLine("");
            cur.prepareQuery(
                "select " +
                "	$(var1), " +
                "	$(var2), " +
                "	$(var3) " +
                "from " +
                "	sysmaster:sysdual ");
            cur.substitution(subvars, subvaldoubles, precs, scales);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "10.55");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "10.556");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            Console.WriteLine("");


            // nulls as nulls
            Console.WriteLine("NULLS AS NULLS: ");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery(
                "select NULL::int,1,NULL::int from sysmaster:sysdual"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), (String)null);
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery(
                "select NULL::int,1,NULL::int from sysmaster:sysdual"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "");
            Console.WriteLine("");


            // output bind by position
            Console.WriteLine("OUTPUT BIND BY POSITION: ");
            cur.sendQuery("drop procedure testproc");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	out out1 int, " +
                "	out out2 varchar(20), " +
                "	out out3 float, " +
                "	out out4 varchar(20)) " +
                "let out1 = 1; " +
                "	let out2 = 'hello'; " +
                "	let out3 = 2.5; " +
                "	let out4 = null; " +
                "end procedure;"));
            assertTrue(con.commit());
            cur.prepareQuery("{call testproc(?,?,?,?)}");
            assertEquals(cur.countBindVariables(), (UInt16)4);
            cur.defineOutputBindInteger("1");
            cur.defineOutputBindString("2", (UInt32)20);
            cur.defineOutputBindDouble("3");
            cur.defineOutputBindString("4", (UInt32)20);
            assertTrue(cur.executeQuery());
            numvar = cur.getOutputBindInteger("1");
            stringvar = cur.getOutputBindString("2");
            floatvar = cur.getOutputBindDouble("3");
            nullvar = cur.getOutputBindString("4");
            assertEquals(numvar, (Int64)1);
            assertEquals(stringvar, "hello");
            assertEquals(floatvar, 2.5);
            assertEquals(nullvar, (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // output bind by name
            // informix doesn't support bind by name


            // output bind by name with validation
            // informix doesn't support bind by name


            // lob output bind
            Console.WriteLine("LOB OUTPUT BIND: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(con.commit());
            cur.prepareQuery("insert into testtable values (?,?)");
            cur.inputBindClob("1", "hello", (UInt32)5);
            cur.inputBindBlob("2", System.Text.Encoding.ASCII.GetBytes("hello"), (UInt32)5);
            assertTrue(cur.executeQuery());
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	out out1 clob, " +
                "	out out2 blob) " +
                "select testclob, testblob " +
                "	into out1, out2 " +
                "	from testtable; " +
                "end procedure;"));
            assertTrue(con.commit());
            cur.prepareQuery("{call testproc(?,?)}");
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
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebufferchars[i] = 'C';
            }
            largebufferchars[LARGE_BUFFER_LENGTH] = '\0';
            largebuffer = new String(largebufferchars, 0, LARGE_BUFFER_LENGTH);
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in1 clob, " +
                "	out out1 clob) " +
                "let out1 = in1; " +
                "	end procedure;"));
            assertTrue(con.commit());
            cur.prepareQuery("{call testproc(?,?)}");
            cur.inputBindClob("1", largebuffer, (UInt32)LARGE_BUFFER_LENGTH);
            cur.defineOutputBindClob("2");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindLength("2"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getOutputBindClob("2"), largebuffer);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery("create table testtable (testval int)");
            assertTrue(con.commit());
            cur.prepareQuery("insert into testtable values (?)");
            cur.inputBind("1", (Int64)(-1));
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testval from testtable");
            assertEquals(cur.getField((UInt64)0, "testval"), "-1");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // bind validation
            // informix doesn't support bind by name

            // rebinding
            Console.WriteLine("REBINDING: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in1 int, " +
                "	out out1 int) " +
                "let out1 = in1; " +
                "end procedure;"));
            assertTrue(con.commit());
            cur.prepareQuery("{call testproc(?,?)}");
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
            cur.prepareQuery("select 1 from sysmaster:sysdual");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.prepareQuery("select ?::int from sysmaster:sysdual");
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
                "	in1 int, " +
                "	in2 float, " +
                "	in3 varchar(20)) " +
                "end procedure;"));
            assertTrue(con.commit());
            cur.prepareQuery("{call testproc(?,?,?)}");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 2.5, (UInt32)2, (UInt32)1);
            cur.inputBind("3", "hello");
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // stored procedure returning single value
            Console.WriteLine("STORED PROCEDURE RETURNING SINGLE VALUE: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in1 int, " +
                "	in2 float, " +
                "	in3 varchar(20), " +
                "	out out1 int) " +
                "let out1 = in1; " +
                "end procedure;"));
            assertTrue(con.commit());
            cur.prepareQuery("{call testproc(?,?,?,?)}");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 2.5, (UInt32)2, (UInt32)1);
            cur.inputBind("3", "hello");
            cur.defineOutputBindInteger("4");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("4"), (Int64)1);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // stored procedure returning multiple values
            Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc(" +
                "	in1 int, " +
                "	in2 float, " +
                "	in3 varchar(20), " +
                "	out out1 int, " +
                "	out out2 float, " +
                "	out out3 varchar(20)) " +
                "let out1 = in1; " +
                "	let out2 = in2; " +
                "	let out3 = in3; " +
                "end procedure;"));
            assertTrue(con.commit());
            cur.prepareQuery("{call testproc(?,?,?,?,?,?)}");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 2.5, (UInt32)2, (UInt32)1);
            cur.inputBind("3", "hello");
            cur.defineOutputBindInteger("4");
            cur.defineOutputBindDouble("5");
            cur.defineOutputBindString("6", (UInt32)20);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("4"), (Int64)1);
            assertEquals(cur.getOutputBindDouble("5"), 2.5);
            assertEquals(cur.getOutputBindString("6"), "hello");
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // stored procedure returning result set
            Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create procedure testproc() " +
                "returning boolean, smallint, varchar(40); " +
                "	define out1 boolean; " +
                "	define out2 smallint; " +
                "	define out3 varchar(40); " +
                "	foreach " +
                "		select " +
                "			testboolean, " +
                "			testsmallint, " +
                "			testvarchar " +
                "		into out1,out2,out3 " +
                "		from ( " +
                "			select " +
                "				't' as testboolean, " +
                "				1 as testsmallint, " +
                "				'1' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "			union " +
                "			select " +
                "				't' as testboolean, " +
                "				2 as testsmallint, " +
                "				'2' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "			union " +
                "			select " +
                "				't' as testboolean, " +
                "				3 as testsmallint, " +
                "				'3' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "			union " +
                "			select " +
                "				't' as testboolean, " +
                "				4 as testsmallint, " +
                "				'4' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "			union " +
                "			select " +
                "				't' as testboolean, " +
                "				5 as testsmallint, " +
                "				'5' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "			union " +
                "			select " +
                "				't' as testboolean, " +
                "				6 as testsmallint, " +
                "				'6' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "			union " +
                "			select " +
                "				't' as testboolean, " +
                "				7 as testsmallint, " +
                "				'7' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "			union " +
                "			select " +
                "				't' as testboolean, " +
                "				8 as testsmallint, " +
                "				'8' as testvarchar " +
                "			from " +
                "				sysmaster:sysdual " +
                "		) " +
                "	return out1,out2,out3 " +
                "	with resume; " +
                "	end foreach; " +
                "	end procedure;"));
            assertTrue(con.commit());
            assertTrue(cur.sendQuery("{call testproc()}"));
            assertEquals(cur.rowCount(), (UInt64)8);
            assertTrue(cur.sendQuery("drop procedure testproc"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // null and empty lobs
            Console.WriteLine("NULL AND EMPTY LOBS: ");
            cur.sendQuery("drop table testtable");
            cur.getNullsAsNulls();
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
            // informix returns a single \0 for an empty lob; the C/C++
            // tests pass via strcmp (which stops at \0), so truncate at
            // the first \0 here.
            String f0 = cur.getField((UInt64)0, (UInt32)0);
            if (f0 != null) { int nul = f0.IndexOf('\0'); if (nul >= 0) f0 = f0.Substring(0, nul); }
            assertEquals(f0, "");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), (String)null);
            String f2 = cur.getField((UInt64)0, (UInt32)2);
            if (f2 != null) { int nul = f2.IndexOf('\0'); if (nul >= 0) f2 = f2.Substring(0, nul); }
            assertEquals(f2, "");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // long lobs
            Console.WriteLine("LONG LOBS: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(
                "create table testtable (" +
                "	testtext text, " +
                "	testbyte byte)");
            assertTrue(con.commit());
            cur.prepareQuery("insert into testtable values (?,?)");
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebufferchars[i] = 'C';
            }
            largebufferchars[LARGE_BUFFER_LENGTH] = '\0';
            largebuffer = new String(largebufferchars, 0, LARGE_BUFFER_LENGTH);
            cur.inputBindClob("1", largebuffer, (UInt32)LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("2", System.Text.Encoding.ASCII.GetBytes(largebuffer), (UInt32)LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "testtext"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testtext"), largebuffer);
            assertEquals(cur.getFieldLength((UInt64)0, "testbyte"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, "testbyte"), System.Text.Encoding.ASCII.GetBytes(largebuffer),
                                LARGE_BUFFER_LENGTH);
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // temporary tables
            Console.WriteLine("TEMPORARY TABLES: ");
            cur.sendQuery("drop table temptable");
            cur.sendQuery(
                "create temp table temptable (col1 int)");
            assertTrue(cur.sendQuery("insert into temptable values (1)"));
            assertTrue(cur.sendQuery("select count(*) from temptable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            con.endSession();
            Console.WriteLine("");
            assertFalse(cur.sendQuery("select count(*) from temptable"));
            Console.WriteLine("");


            // encoded binary data
            // informix doesn't support encoded binary data


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
                    "	(col1 serial primary key, " +
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
            assertTrue(cur.getCatalogList((String)null));
            assertEquals(cur.getColumnName((UInt32)0), "Database");
            assertTrue(cur.rowCount() > 0);
            Console.WriteLine("");


            // schema list
            Console.WriteLine("SCHEMA LIST: ");
            // informix requires that a table exist that is
            // owned by a user for the user to be reported
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 integer, " +
                "	col2 integer)"));
            assertTrue(con.commit());
            assertTrue(cur.getSchemaList((String)null));
            assertEquals(cur.getColumnName((UInt32)0), "Database");
            assertTrue(cur.rowCount() > 0);
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName((UInt32)0), "table_type");
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
            assertTrue(cur.getTableList((String)null));
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
            assertEquals(cur.getField((UInt64)0, "precision"), "32767");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "CHAR");
            assertTrue(cur.getTypeInfoList("varchar"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "VARCHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "12");
            assertEquals(cur.getField((UInt64)0, "precision"), "255");
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
                "	testboolean boolean, " +
                "	testsmallint smallint, " +
                "	testint integer, " +
                "	testbigint bigint, " +
                "	testint8 int8, " +
                "	testdecimal decimal(10,2), " +
                "	testmoney money, " +
                "	testsmallfloat smallfloat, " +
                "	testfloat float, " +
                "	testchar char(40), " +
                "	testnchar nchar(40), " +
                "	testvarchar varchar(40), " +
                "	testnvarchar nvarchar(40), " +
                "	testlvarchar lvarchar(40), " +
                "	testdate date, " +
                "	testdatetime datetime year to second, " +
                "	testtext text, " +
                "	testbyte byte)"));
            assertTrue(con.commit());
            assertTrue(cur.getColumnList("testtable", (String)null));
            assertEquals(cur.getColumnName((UInt32)0), "column_name");
            assertEquals(cur.getColumnName((UInt32)1), "data_type");
            assertEquals(cur.getColumnName((UInt32)2), "character_maximum_length");
            assertEquals(cur.getColumnName((UInt32)3), "numeric_precision");
            assertEquals(cur.getColumnName((UInt32)4), "numeric_scale");
            assertEquals(cur.getColumnName((UInt32)5), "is_nullable");
            assertEquals(cur.getColumnName((UInt32)6), "column_key");
            assertEquals(cur.getColumnName((UInt32)7), "column_default");
            assertEquals(cur.getColumnName((UInt32)8), "extra");
            assertEquals(cur.getField((UInt64)0, "column_name"), "testboolean");
            assertEquals(cur.getField((UInt64)1, "column_name"), "testsmallint");
            assertEquals(cur.getField((UInt64)2, "column_name"), "testint");
            assertEquals(cur.getField((UInt64)3, "column_name"), "testbigint");
            assertEquals(cur.getField((UInt64)4, "column_name"), "testint8");
            assertEquals(cur.getField((UInt64)5, "column_name"), "testdecimal");
            assertEquals(cur.getField((UInt64)6, "column_name"), "testmoney");
            assertEquals(cur.getField((UInt64)7, "column_name"), "testsmallfloat");
            assertEquals(cur.getField((UInt64)8, "column_name"), "testfloat");
            assertEquals(cur.getField((UInt64)9, "column_name"), "testchar");
            assertEquals(cur.getField((UInt64)10, "column_name"), "testnchar");
            assertEquals(cur.getField((UInt64)11, "column_name"), "testvarchar");
            assertEquals(cur.getField((UInt64)12, "column_name"), "testnvarchar");
            assertEquals(cur.getField((UInt64)13, "column_name"), "testlvarchar");
            assertEquals(cur.getField((UInt64)14, "column_name"), "testdate");
            assertEquals(cur.getField((UInt64)15, "column_name"), "testdatetime");
            assertEquals(cur.getField((UInt64)16, "column_name"), "testtext");
            assertEquals(cur.getField((UInt64)17, "column_name"), "testbyte");
            assertEquals(cur.getField((UInt64)0, "data_type"), "BOOLEAN");
            assertEquals(cur.getField((UInt64)1, "data_type"), "SMALLINT");
            assertEquals(cur.getField((UInt64)2, "data_type"), "INTEGER");
            assertEquals(cur.getField((UInt64)3, "data_type"), "BIGINT");
            assertEquals(cur.getField((UInt64)4, "data_type"), "INT8");
            assertEquals(cur.getField((UInt64)5, "data_type"), "DECIMAL");
            assertEquals(cur.getField((UInt64)6, "data_type"), "MONEY");
            assertEquals(cur.getField((UInt64)7, "data_type"), "SMALLFLOAT");
            assertEquals(cur.getField((UInt64)8, "data_type"), "FLOAT");
            assertEquals(cur.getField((UInt64)9, "data_type"), "CHAR");
            assertEquals(cur.getField((UInt64)10, "data_type"), "NCHAR");
            assertEquals(cur.getField((UInt64)11, "data_type"), "VARCHAR");
            assertEquals(cur.getField((UInt64)12, "data_type"), "NVARCHAR");
            assertEquals(cur.getField((UInt64)13, "data_type"), "LVARCHAR");
            assertEquals(cur.getField((UInt64)14, "data_type"), "DATE");
            assertEquals(cur.getField((UInt64)15, "data_type"), "DATETIME");
            assertEquals(cur.getField((UInt64)16, "data_type"), "TEXT");
            assertEquals(cur.getField((UInt64)17, "data_type"), "BYTE");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // column list - auto_increment, primary key
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 serial primary key, " +
                "	col2 int)"));
            assertTrue(con.commit());
            assertTrue(cur.getColumnList("testtable", (String)null));
            assertTrue(cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertFalse(cur.getField((UInt64)1, "extra").Contains("auto_increment"));
            assertFalse(cur.getField((UInt64)1, "column_key").Contains("PRI"));
            Console.WriteLine("");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int primary key, " +
                "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", (String)null));
            assertFalse(cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // primary keys list
            Console.WriteLine("PRIMARY KEYS LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 integer primary key, " +
                "	col2 integer)"));
            assertTrue(con.commit());
            assertTrue(cur.getPrimaryKeysList("testtable", (String)null));
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
            assertTrue(cur.getField((UInt64)0, "table") == "testtable");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertTrue(cur.getField((UInt64)0, "column_name") == "col1");
            assertTrue(!String.IsNullOrEmpty(cur.getField((UInt64)0, "key_name")));
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(con.commit());
            Console.WriteLine("");


            // key and index list
            Console.WriteLine("KEY AND INDEX LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 integer primary key, " +
                "	col2 integer)"));
            assertTrue(con.commit());
            assertTrue(cur.getKeyAndIndexList("testtable", (String)null));
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
            assertTrue(cur.getField((UInt64)0, "table") == "testtable");
            assertEquals(cur.getField((UInt64)0, "non_unique"), "0");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertTrue(cur.getField((UInt64)0, "column_name") == "col1");
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
                "	in1 integer, " +
                "	in2 char(20), " +
                "	in3 varchar(20), " +
                "	in4 date) " +
                "define x integer; " +
                "let x = 1; " +
                "end procedure;"));
            assertTrue(cur.sendQuery(
                "create procedure testproc2(" +
                "	in1 integer, " +
                "	in2 char(20), " +
                "	in3 varchar(20), " +
                "	in4 date) " +
                "define x integer; " +
                "let x = 1; " +
                "end procedure;"));
            assertTrue(cur.sendQuery(
                "create procedure testproc3(" +
                "	in1 integer, " +
                "	in2 char(20), " +
                "	in3 varchar(20), " +
                "	in4 date) " +
                "define x integer; " +
                "let x = 1; " +
                "end procedure;"));
            assertTrue(cur.sendQuery(
                "create procedure testproc4(" +
                "	in1 integer, " +
                "	in2 char(20), " +
                "	in3 varchar(20), " +
                "	in4 date) " +
                "define x integer; " +
                "let x = 1; " +
                "end procedure;"));
            assertTrue(con.commit());
            assertTrue(cur.getProcedureList((String)null));
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
            assertTrue(cur.getProcedureParameterList("testproc1", (String)null));
            assertEquals(cur.getColumnName((UInt32)0), "parameter_name");
            assertEquals(cur.getColumnName((UInt32)1), "parameter_mode");
            assertEquals(cur.getColumnName((UInt32)2), "data_type");
            assertEquals(cur.getColumnName((UInt32)3), "character_maximum_length");
            assertEquals(cur.getColumnName((UInt32)4), "ordinal_position");
            assertEquals(cur.rowCount(), (UInt64)4);
            assertEquals(cur.getField((UInt64)0, "parameter_name"), "in1");
            assertEquals(cur.getField((UInt64)0, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)0, "data_type"), "integer");
            assertEquals(cur.getField((UInt64)0, "ordinal_position"), "1");
            assertEquals(cur.getField((UInt64)1, "parameter_name"), "in2");
            assertEquals(cur.getField((UInt64)1, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)1, "data_type"), "char");
            assertEquals(cur.getField((UInt64)1, "ordinal_position"), "2");
            assertEquals(cur.getField((UInt64)2, "parameter_name"), "in3");
            assertEquals(cur.getField((UInt64)2, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)2, "data_type"), "varchar");
            assertEquals(cur.getField((UInt64)2, "ordinal_position"), "3");
            assertEquals(cur.getField((UInt64)3, "parameter_name"), "in4");
            assertEquals(cur.getField((UInt64)3, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)3, "data_type"), "date");
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
