// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using System.Text;
using SQLRClient;

namespace SQLRClientTest
{
    class MysqlTest : SQLRTest
    {
        public static int Main(string[] args)
        {
            String[] isolationlevels = new String[] {
                        "REPEATABLE-READ","READ-UNCOMMITTED",
                        "READ-COMMITTED","SERIALIZABLE" };
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

            const Int32 LARGE_BUFFER_LENGTH = 8192;
            StringBuilder largebufferbuilder = new StringBuilder(LARGE_BUFFER_LENGTH);
            for (Int32 i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebufferbuilder.Append('C');
            }
            String largebuffer = largebufferbuilder.ToString();


            // hostname
            String hostname = System.Net.Dns.GetHostName();
            int dot = hostname.IndexOf('.');
            if (dot > 0) { hostname = hostname.Substring(0, dot); }


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                "testuser", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "mysql");
            Console.WriteLine("");


            // db version
            Console.WriteLine("DB VERSION: ");
            String dbversion = con.dbVersion();
            UInt32 majorversion = (UInt32)(dbversion[0] - '0');
            Console.WriteLine("");


            // ping
            Console.WriteLine("PING: ");
            assertTrue(con.ping());
            Console.WriteLine("");


            // transaction state
            Console.WriteLine("TRANSACTION STATE: ");
            assertEquals(con.getDefaultTransactionModel(), "explicit-deferred");
            assertEquals(con.getTransactionModel(), "explicit-deferred");
            assertFalse(con.getInTransaction());
            assertTrue(con.getAutoCommit());
            Console.WriteLine("");


            // bind format
            Console.WriteLine("BIND FORMAT: ");
            if (majorversion > 3)
            {
                assertEquals(con.bindFormat(), "?");
            }
            else
            {
                assertEquals(con.bindFormat(), ":*");
            }
            Console.WriteLine("");


            // nextval format
            Console.WriteLine("NEXTVAL FORMAT: ");
            assertEquals(con.nextvalFormat(), "");
            Console.WriteLine("");


            // isolation levels
            // (mysql before 4.0 doesn't support setting the isolation level)
            Console.WriteLine("ISOLATION LEVELS: ");
            if (majorversion > 3)
            {
                foreach (String il in isolationlevels)
                {
                    assertTrue(con.setIsolationLevel(il));
                    assertEquals(con.getIsolationLevel(), il);
                    Console.WriteLine("");
                }
                // reset to the default isolation level
                assertTrue(con.setIsolationLevel(isolationlevels[0]));
            }
            Console.WriteLine("");


            // create testtable
            Console.WriteLine("CREATE TESTTABLE: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	testtinyint tinyint, "
                + "	testsmallint smallint, "
                + "	testmediumint mediumint, "
                + "	testint int, "
                + "	testbigint bigint, "
                + "	testfloat float, "
                + "	testreal real, "
                + "	testdecimal decimal(2,1), "
                + "	testdate date, "
                + "	testtime time, "
                + "	testdatetime datetime, "
                + "	testyear year, "
                + "	testchar char(40), "
                + "	testvarchar varchar(40), "
                + "	testtext text, "
                + "	testtinytext tinytext, "
                + "	testmediumtext mediumtext, "
                + "	testlongtext longtext, "
                + "	testblob blob, "
                + "	testtinyblob tinyblob, "
                + "	testmediumblob mediumblob, "
                + "	testlongblob longblob, "
                + "	testtimestamp timestamp)"));
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
                + "	1, "
                + "	1, "
                + "	1.5, "
                + "	1.5, "
                + "	1.5, "
                + "	'2001-01-01', "
                + "	'01:00:00', "
                + "	'2001-01-01 01:00:00', "
                + "	'2001', "
                + "	'char1', "
                + "	'varchar1', "
                + "	'text1', "
                + "	'tinytext1', "
                + "	'mediumtext1', "
                + "	'longtext1', "
                + "	'blob1', "
                + "	'tinyblob1', "
                + "	'mediumblob1', "
                + "	'longblob1', "
                + "	NULL)"));
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	2, "
                + "	2, "
                + "	2, "
                + "	2, "
                + "	2, "
                + "	2.5, "
                + "	2.5, "
                + "	2.5, "
                + "	'2002-01-01', "
                + "	'02:00:00', "
                + "	'2002-01-01 02:00:00', "
                + "	'2002', "
                + "	'char2', "
                + "	'varchar2', "
                + "	'text2', "
                + "	'tinytext2', "
                + "	'mediumtext2', "
                + "	'longtext2', "
                + "	'blob2', "
                + "	'tinyblob2', "
                + "	'mediumblob2', "
                + "	'longblob2', "
                + "	NULL)"));
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	3, "
                + "	3, "
                + "	3, "
                + "	3, "
                + "	3, "
                + "	3.5, "
                + "	3.5, "
                + "	3.5, "
                + "	'2003-01-01', "
                + "	'03:00:00', "
                + "	'2003-01-01 03:00:00', "
                + "	'2003', "
                + "	'char3', "
                + "	'varchar3', "
                + "	'text3', "
                + "	'tinytext3', "
                + "	'mediumtext3', "
                + "	'longtext3', "
                + "	'blob3', "
                + "	'tinyblob3', "
                + "	'mediumblob3', "
                + "	'longblob3', "
                + "	NULL)"));
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	4, "
                + "	4, "
                + "	4, "
                + "	4, "
                + "	4, "
                + "	4.5, "
                + "	4.5, "
                + "	4.5, "
                + "	'2004-01-01', "
                + "	'04:00:00', "
                + "	'2004-01-01 04:00:00', "
                + "	'2004', "
                + "	'char4', "
                + "	'varchar4', "
                + "	'text4', "
                + "	'tinytext4', "
                + "	'mediumtext4', "
                + "	'longtext4', "
                + "	'blob4', "
                + "	'tinyblob4', "
                + "	'mediumblob4', "
                + "	'longblob4', "
                + "	NULL)"));
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
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	?, "
                + "	NULL)");
            assertEquals(cur.countBindVariables(), (UInt16)22);
            cur.inputBind("1", (Int64)5);
            cur.inputBind("2", (Int64)5);
            cur.inputBind("3", (Int64)5);
            cur.inputBind("4", (Int64)5);
            cur.inputBind("5", (Int64)5);
            cur.inputBind("6", 5.5, 2, 1);
            cur.inputBind("7", 5.5, 2, 1);
            cur.inputBind("8", 5.5, 2, 1);
            cur.inputBind("9", "2005-01-01");
            cur.inputBind("10", "05:00:00");
            cur.inputBind("11", (Int16)2005, (Int16)1, (Int16)1, (Int16)5, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBind("12", "2005");
            cur.inputBind("13", "char5");
            cur.inputBind("14", "varchar5");
            cur.inputBindClob("15", "text5", 5);
            cur.inputBindClob("16", "tinytext5", 9);
            cur.inputBindClob("17", "mediumtext5", 11);
            cur.inputBindClob("18", "longtext5", 9);
            cur.inputBindBlob("19", Encoding.ASCII.GetBytes("blob5"), 5);
            cur.inputBindBlob("20", Encoding.ASCII.GetBytes("tinyblob5"), 9);
            cur.inputBindBlob("21", Encoding.ASCII.GetBytes("mediumblob5"), 11);
            cur.inputBindBlob("22", Encoding.ASCII.GetBytes("longblob5"), 9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)6);
            cur.inputBind("2", (Int64)6);
            cur.inputBind("3", (Int64)6);
            cur.inputBind("4", (Int64)6);
            cur.inputBind("5", (Int64)6);
            cur.inputBind("6", 6.5, 2, 1);
            cur.inputBind("7", 6.5, 2, 1);
            cur.inputBind("8", 6.5, 2, 1);
            cur.inputBind("9", "2006-01-01");
            cur.inputBind("10", "06:00:00");
            cur.inputBind("11", (Int16)2006, (Int16)1, (Int16)1, (Int16)6, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBind("12", "2006");
            cur.inputBind("13", "char6");
            cur.inputBind("14", "varchar6");
            cur.inputBindClob("15", "text6", 5);
            cur.inputBindClob("16", "tinytext6", 9);
            cur.inputBindClob("17", "mediumtext6", 11);
            cur.inputBindClob("18", "longtext6", 9);
            cur.inputBindBlob("19", Encoding.ASCII.GetBytes("blob6"), 5);
            cur.inputBindBlob("20", Encoding.ASCII.GetBytes("tinyblob6"), 9);
            cur.inputBindBlob("21", Encoding.ASCII.GetBytes("mediumblob6"), 11);
            cur.inputBindBlob("22", Encoding.ASCII.GetBytes("longblob6"), 9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)7);
            cur.inputBind("2", (Int64)7);
            cur.inputBind("3", (Int64)7);
            cur.inputBind("4", (Int64)7);
            cur.inputBind("5", (Int64)7);
            cur.inputBind("6", 7.5, 2, 1);
            cur.inputBind("7", 7.5, 2, 1);
            cur.inputBind("8", 7.5, 2, 1);
            cur.inputBind("9", "2007-01-01");
            cur.inputBind("10", "07:00:00");
            cur.inputBind("11", (Int16)2007, (Int16)1, (Int16)1, (Int16)7, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBind("12", "2007");
            cur.inputBind("13", "char7");
            cur.inputBind("14", "varchar7");
            cur.inputBindClob("15", "text7", 5);
            cur.inputBindClob("16", "tinytext7", 9);
            cur.inputBindClob("17", "mediumtext7", 11);
            cur.inputBindClob("18", "longtext7", 9);
            cur.inputBindBlob("19", Encoding.ASCII.GetBytes("blob7"), 5);
            cur.inputBindBlob("20", Encoding.ASCII.GetBytes("tinyblob7"), 9);
            cur.inputBindBlob("21", Encoding.ASCII.GetBytes("mediumblob7"), 11);
            cur.inputBindBlob("22", Encoding.ASCII.GetBytes("longblob7"), 9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            // mysql doesn't support implicit conversion of string binds to other
            // data types, so arrays of binds don't generally work.


            // input bind by position with validation
            Console.WriteLine("BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1", (Int64)8);
            cur.inputBind("2", (Int64)8);
            cur.inputBind("3", (Int64)8);
            cur.inputBind("4", (Int64)8);
            cur.inputBind("5", (Int64)8);
            cur.inputBind("6", 8.5, 2, 1);
            cur.inputBind("7", 8.5, 2, 1);
            cur.inputBind("8", 8.5, 2, 1);
            cur.inputBind("9", "2008-01-01");
            cur.inputBind("10", "08:00:00");
            cur.inputBind("11", (Int16)2008, (Int16)1, (Int16)1, (Int16)8, (Int16)0, (Int16)0, (Int32)0, (String)null, false);
            cur.inputBind("12", "2008");
            cur.inputBind("13", "char8");
            cur.inputBind("14", "varchar8");
            cur.inputBindClob("15", "text8", 5);
            cur.inputBindClob("16", "tinytext8", 9);
            cur.inputBindClob("17", "mediumtext8", 11);
            cur.inputBindClob("18", "longtext8", 9);
            cur.inputBindBlob("19", Encoding.ASCII.GetBytes("blob8"), 5);
            cur.inputBindBlob("20", Encoding.ASCII.GetBytes("tinyblob8"), 9);
            cur.inputBindBlob("21", Encoding.ASCII.GetBytes("mediumblob8"), 11);
            cur.inputBindBlob("22", Encoding.ASCII.GetBytes("longblob8"), 9);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by name
            // mysql doesn't support bind by name


            // array of input binds by name
            // mysql doesn't support bind by name


            // input bind by name with validation
            // mysql doesn't support bind by name


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(), (UInt32)23);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName(0), "testtinyint");
            assertEquals(cur.getColumnName(1), "testsmallint");
            assertEquals(cur.getColumnName(2), "testmediumint");
            assertEquals(cur.getColumnName(3), "testint");
            assertEquals(cur.getColumnName(4), "testbigint");
            assertEquals(cur.getColumnName(5), "testfloat");
            assertEquals(cur.getColumnName(6), "testreal");
            assertEquals(cur.getColumnName(7), "testdecimal");
            assertEquals(cur.getColumnName(8), "testdate");
            assertEquals(cur.getColumnName(9), "testtime");
            assertEquals(cur.getColumnName(10), "testdatetime");
            assertEquals(cur.getColumnName(11), "testyear");
            assertEquals(cur.getColumnName(12), "testchar");
            assertEquals(cur.getColumnName(13), "testvarchar");
            assertEquals(cur.getColumnName(14), "testtext");
            assertEquals(cur.getColumnName(15), "testtinytext");
            assertEquals(cur.getColumnName(16), "testmediumtext");
            assertEquals(cur.getColumnName(17), "testlongtext");
            assertEquals(cur.getColumnName(18), "testblob");
            assertEquals(cur.getColumnName(19), "testtinyblob");
            assertEquals(cur.getColumnName(20), "testmediumblob");
            assertEquals(cur.getColumnName(21), "testlongblob");
            assertEquals(cur.getColumnName(22), "testtimestamp");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testtinyint");
            assertEquals(cols[1], "testsmallint");
            assertEquals(cols[2], "testmediumint");
            assertEquals(cols[3], "testint");
            assertEquals(cols[4], "testbigint");
            assertEquals(cols[5], "testfloat");
            assertEquals(cols[6], "testreal");
            assertEquals(cols[7], "testdecimal");
            assertEquals(cols[8], "testdate");
            assertEquals(cols[9], "testtime");
            assertEquals(cols[10], "testdatetime");
            assertEquals(cols[11], "testyear");
            assertEquals(cols[12], "testchar");
            assertEquals(cols[13], "testvarchar");
            assertEquals(cols[14], "testtext");
            assertEquals(cols[15], "testtinytext");
            assertEquals(cols[16], "testmediumtext");
            assertEquals(cols[17], "testlongtext");
            assertEquals(cols[18], "testblob");
            assertEquals(cols[19], "testtinyblob");
            assertEquals(cols[20], "testmediumblob");
            assertEquals(cols[21], "testlongblob");
            assertEquals(cols[22], "testtimestamp");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            assertEquals(cur.getColumnType((UInt32)0), "TINYINT");
            assertEquals(cur.getColumnType((UInt32)1), "SMALLINT");
            assertEquals(cur.getColumnType((UInt32)2), "MEDIUMINT");
            assertEquals(cur.getColumnType((UInt32)3), "INT");
            assertEquals(cur.getColumnType((UInt32)4), "BIGINT");
            assertEquals(cur.getColumnType((UInt32)5), "FLOAT");
            assertEquals(cur.getColumnType((UInt32)6), "REAL");
            assertEquals(cur.getColumnType((UInt32)7), "DECIMAL");
            assertEquals(cur.getColumnType((UInt32)8), "DATE");
            assertEquals(cur.getColumnType((UInt32)9), "TIME");
            assertEquals(cur.getColumnType((UInt32)10), "DATETIME");
            assertEquals(cur.getColumnType((UInt32)11), "YEAR");
            if (majorversion == 3)
            {
                assertEquals(cur.getColumnType((UInt32)12), "VARSTRING");
            }
            else
            {
                assertEquals(cur.getColumnType((UInt32)12), "STRING");
            }
            assertEquals(cur.getColumnType((UInt32)13), "VARSTRING");
            assertEquals(cur.getColumnType((UInt32)14), "TEXT");
            assertEquals(cur.getColumnType((UInt32)15), "TINYTEXT");
            assertEquals(cur.getColumnType((UInt32)16), "MEDIUMTEXT");
            assertEquals(cur.getColumnType((UInt32)17), "LONGTEXT");
            assertEquals(cur.getColumnType((UInt32)18), "BLOB");
            assertEquals(cur.getColumnType((UInt32)19), "TINYBLOB");
            assertEquals(cur.getColumnType((UInt32)20), "MEDIUMBLOB");
            assertEquals(cur.getColumnType((UInt32)21), "LONGBLOB");
            assertEquals(cur.getColumnType((UInt32)22), "TIMESTAMP");
            assertEquals(cur.getColumnType("testtinyint"), "TINYINT");
            assertEquals(cur.getColumnType("testsmallint"), "SMALLINT");
            assertEquals(cur.getColumnType("testmediumint"), "MEDIUMINT");
            assertEquals(cur.getColumnType("testint"), "INT");
            assertEquals(cur.getColumnType("testbigint"), "BIGINT");
            assertEquals(cur.getColumnType("testfloat"), "FLOAT");
            assertEquals(cur.getColumnType("testreal"), "REAL");
            assertEquals(cur.getColumnType("testdecimal"), "DECIMAL");
            assertEquals(cur.getColumnType("testdate"), "DATE");
            assertEquals(cur.getColumnType("testtime"), "TIME");
            assertEquals(cur.getColumnType("testdatetime"), "DATETIME");
            assertEquals(cur.getColumnType("testyear"), "YEAR");
            if (majorversion == 3)
            {
                assertEquals(cur.getColumnType("testchar"), "VARSTRING");
            }
            else
            {
                assertEquals(cur.getColumnType("testchar"), "STRING");
            }
            assertEquals(cur.getColumnType("testvarchar"), "VARSTRING");
            assertEquals(cur.getColumnType("testtext"), "TEXT");
            assertEquals(cur.getColumnType("testtinytext"), "TINYTEXT");
            assertEquals(cur.getColumnType("testmediumtext"), "MEDIUMTEXT");
            assertEquals(cur.getColumnType("testlongtext"), "LONGTEXT");
            assertEquals(cur.getColumnType("testblob"), "BLOB");
            assertEquals(cur.getColumnType("testtinyblob"), "TINYBLOB");
            assertEquals(cur.getColumnType("testmediumblob"), "MEDIUMBLOB");
            assertEquals(cur.getColumnType("testlongblob"), "LONGBLOB");
            assertEquals(cur.getColumnType("testtimestamp"), "TIMESTAMP");
            Console.WriteLine("");


            // mysql before 4 reports column lengths differently (charset)
            if (majorversion > 3)
            {
                // column length
                Console.WriteLine("COLUMN LENGTH: ");
                assertEquals(cur.getColumnLength((UInt32)0), (UInt32)1);
                assertEquals(cur.getColumnLength((UInt32)1), (UInt32)2);
                assertEquals(cur.getColumnLength((UInt32)2), (UInt32)3);
                assertEquals(cur.getColumnLength((UInt32)3), (UInt32)4);
                assertEquals(cur.getColumnLength((UInt32)4), (UInt32)8);
                assertEquals(cur.getColumnLength((UInt32)5), (UInt32)4);
                assertEquals(cur.getColumnLength((UInt32)6), (UInt32)8);
                assertEquals(cur.getColumnLength((UInt32)7), (UInt32)6);
                assertEquals(cur.getColumnLength((UInt32)8), (UInt32)3);
                assertEquals(cur.getColumnLength((UInt32)9), (UInt32)3);
                assertEquals(cur.getColumnLength((UInt32)10), (UInt32)8);
                assertEquals(cur.getColumnLength((UInt32)11), (UInt32)1);
                // testchar/testvarchar are char(40)/varchar(40); the connection
                // charset is latin1 (1 byte/char) so the lengths are 40/41
                assertEquals(cur.getColumnLength((UInt32)12), (UInt32)40);
                assertEquals(cur.getColumnLength((UInt32)13), (UInt32)41);
                assertEquals(cur.getColumnLength((UInt32)14), (UInt32)65535);
                assertEquals(cur.getColumnLength((UInt32)15), (UInt32)255);
                assertEquals(cur.getColumnLength((UInt32)16), (UInt32)16777215);
                assertEquals(cur.getColumnLength((UInt32)17), (UInt32)2147483647);
                assertEquals(cur.getColumnLength((UInt32)18), (UInt32)65535);
                assertEquals(cur.getColumnLength((UInt32)19), (UInt32)255);
                assertEquals(cur.getColumnLength((UInt32)20), (UInt32)16777215);
                assertEquals(cur.getColumnLength((UInt32)21), (UInt32)2147483647);
                assertEquals(cur.getColumnLength((UInt32)22), (UInt32)4);
                assertEquals(cur.getColumnLength("testtinyint"), (UInt32)1);
                assertEquals(cur.getColumnLength("testsmallint"), (UInt32)2);
                assertEquals(cur.getColumnLength("testmediumint"), (UInt32)3);
                assertEquals(cur.getColumnLength("testint"), (UInt32)4);
                assertEquals(cur.getColumnLength("testbigint"), (UInt32)8);
                assertEquals(cur.getColumnLength("testfloat"), (UInt32)4);
                assertEquals(cur.getColumnLength("testreal"), (UInt32)8);
                assertEquals(cur.getColumnLength("testdecimal"), (UInt32)6);
                assertEquals(cur.getColumnLength("testdate"), (UInt32)3);
                assertEquals(cur.getColumnLength("testtime"), (UInt32)3);
                assertEquals(cur.getColumnLength("testdatetime"), (UInt32)8);
                assertEquals(cur.getColumnLength("testyear"), (UInt32)1);
                // testchar/testvarchar are char(40)/varchar(40); the connection
                // charset is latin1 (1 byte/char) so the lengths are 40/41
                assertEquals(cur.getColumnLength("testchar"), (UInt32)40);
                assertEquals(cur.getColumnLength("testvarchar"), (UInt32)41);
                assertEquals(cur.getColumnLength("testtext"), (UInt32)65535);
                assertEquals(cur.getColumnLength("testtinytext"), (UInt32)255);
                assertEquals(cur.getColumnLength("testmediumtext"), (UInt32)16777215);
                assertEquals(cur.getColumnLength("testlongtext"), (UInt32)2147483647);
                assertEquals(cur.getColumnLength("testblob"), (UInt32)65535);
                assertEquals(cur.getColumnLength("testtinyblob"), (UInt32)255);
                assertEquals(cur.getColumnLength("testmediumblob"), (UInt32)16777215);
                assertEquals(cur.getColumnLength("testlongblob"), (UInt32)2147483647);
                assertEquals(cur.getColumnLength("testtimestamp"), (UInt32)4);
                Console.WriteLine("");
            }


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)1), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)2), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)3), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)4), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)5),(UInt32)3);
            assertEquals(cur.getLongest((UInt32)6), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)7), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)8), (UInt32)10);
            assertEquals(cur.getLongest((UInt32)9), (UInt32)8);
            assertEquals(cur.getLongest((UInt32)10), (UInt32)19);
            assertEquals(cur.getLongest((UInt32)11), (UInt32)4);
            assertEquals(cur.getLongest((UInt32)12), (UInt32)5);
            assertEquals(cur.getLongest((UInt32)13), (UInt32)8);
            assertEquals(cur.getLongest((UInt32)14), (UInt32)5);
            assertEquals(cur.getLongest((UInt32)15), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)16), (UInt32)11);
            assertEquals(cur.getLongest((UInt32)17), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)18), (UInt32)5);
            assertEquals(cur.getLongest((UInt32)19), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)20), (UInt32)11);
            assertEquals(cur.getLongest((UInt32)21), (UInt32)9);
            if (majorversion == 3)
            {
                assertEquals(cur.getLongest((UInt32)22), (UInt32)14);
            }
            else
            {
                assertEquals(cur.getLongest((UInt32)22), (UInt32)19);
            }
            assertEquals(cur.getLongest("testtinyint"), (UInt32)1);
            assertEquals(cur.getLongest("testsmallint"), (UInt32)1);
            assertEquals(cur.getLongest("testmediumint"), (UInt32)1);
            assertEquals(cur.getLongest("testint"), (UInt32)1);
            assertEquals(cur.getLongest("testbigint"), (UInt32)1);
            assertEquals(cur.getLongest("testfloat"), (UInt32)3);
            assertEquals(cur.getLongest("testreal"), (UInt32)3);
            assertEquals(cur.getLongest("testdecimal"), (UInt32)3);
            assertEquals(cur.getLongest("testdate"), (UInt32)10);
            assertEquals(cur.getLongest("testtime"), (UInt32)8);
            assertEquals(cur.getLongest("testdatetime"), (UInt32)19);
            assertEquals(cur.getLongest("testyear"), (UInt32)4);
            assertEquals(cur.getLongest("testchar"), (UInt32)5);
            assertEquals(cur.getLongest("testvarchar"), (UInt32)8);
            assertEquals(cur.getLongest("testtext"), (UInt32)5);
            assertEquals(cur.getLongest("testtinytext"), (UInt32)9);
            assertEquals(cur.getLongest("testmediumtext"), (UInt32)11);
            assertEquals(cur.getLongest("testlongtext"), (UInt32)9);
            assertEquals(cur.getLongest("testblob"), (UInt32)5);
            assertEquals(cur.getLongest("testtinyblob"), (UInt32)9);
            assertEquals(cur.getLongest("testmediumblob"), (UInt32)11);
            assertEquals(cur.getLongest("testlongblob"), (UInt32)9);
            if (majorversion == 3)
            {
                assertEquals(cur.getLongest("testtimestamp"), (UInt32)14);
            }
            else
            {
                assertEquals(cur.getLongest("testtimestamp"), (UInt32)19);
            }
            Console.WriteLine("");


            // row count
            Console.WriteLine("ROW COUNT: ");
            assertEquals(cur.rowCount(), (UInt64)8);
            Console.WriteLine("");


            // total rows
            Console.WriteLine("TOTAL ROWS: ");
            // older versions of mysql know this
            //assertEquals(cur.totalRows(),(UInt64)0);
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
            assertEquals(cur.getField((UInt64)0, (UInt32)5), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)6), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)7), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)8), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, (UInt32)9), "01:00:00");
            assertEquals(cur.getField((UInt64)0, (UInt32)10), "2001-01-01 01:00:00");
            assertEquals(cur.getField((UInt64)0, (UInt32)11), "2001");
            assertEquals(cur.getField((UInt64)0, (UInt32)12), "char1");
            assertEquals(cur.getField((UInt64)0, (UInt32)13), "varchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)14), "text1");
            assertEquals(cur.getField((UInt64)0, (UInt32)15), "tinytext1");
            assertEquals(cur.getField((UInt64)0, (UInt32)16), "mediumtext1");
            assertEquals(cur.getField((UInt64)0, (UInt32)17), "longtext1");
            assertEquals(cur.getField((UInt64)0, (UInt32)18), "blob1");
            assertEquals(cur.getField((UInt64)0, (UInt32)19), "tinyblob1");
            assertEquals(cur.getField((UInt64)0, (UInt32)20), "mediumblob1");
            assertEquals(cur.getField((UInt64)0, (UInt32)21), "longblob1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)3), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)4), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)5), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)6), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)7), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)8), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, (UInt32)9), "08:00:00");
            assertEquals(cur.getField((UInt64)7, (UInt32)10), "2008-01-01 08:00:00");
            assertEquals(cur.getField((UInt64)7, (UInt32)11), "2008");
            assertEquals(cur.getField((UInt64)7, (UInt32)12), "char8");
            assertEquals(cur.getField((UInt64)7, (UInt32)13), "varchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)14), "text8");
            assertEquals(cur.getField((UInt64)7, (UInt32)15), "tinytext8");
            assertEquals(cur.getField((UInt64)7, (UInt32)16), "mediumtext8");
            assertEquals(cur.getField((UInt64)7, (UInt32)17), "longtext8");
            assertEquals(cur.getField((UInt64)7, (UInt32)18), "blob8");
            assertEquals(cur.getField((UInt64)7, (UInt32)19), "tinyblob8");
            assertEquals(cur.getField((UInt64)7, (UInt32)20), "mediumblob8");
            assertEquals(cur.getField((UInt64)7, (UInt32)21), "longblob8");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)4), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0,(UInt32)5),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)6), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)7), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)8), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)9), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)10), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)11), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)12), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)13), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)14), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)15), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)16), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)17), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)18), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)19), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)20), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)21), (UInt32)9);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)4), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7,(UInt32)5),(UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)6), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)7), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)8), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)9), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)10), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)11), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)12), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)13), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)14), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)15), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)16), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)17), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)18), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)19), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)20), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)21), (UInt32)9);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "testtinyint"), "1");
            assertEquals(cur.getField((UInt64)0, "testsmallint"), "1");
            assertEquals(cur.getField((UInt64)0, "testmediumint"), "1");
            assertEquals(cur.getField((UInt64)0, "testint"), "1");
            assertEquals(cur.getField((UInt64)0, "testbigint"), "1");
            assertEquals(cur.getField((UInt64)0, "testfloat"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testreal"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testdecimal"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testdate"), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, "testtime"), "01:00:00");
            assertEquals(cur.getField((UInt64)0, "testdatetime"), "2001-01-01 01:00:00");
            assertEquals(cur.getField((UInt64)0, "testyear"), "2001");
            assertEquals(cur.getField((UInt64)0, "testchar"), "char1");
            assertEquals(cur.getField((UInt64)0, "testvarchar"), "varchar1");
            assertEquals(cur.getField((UInt64)0, "testtext"), "text1");
            assertEquals(cur.getField((UInt64)0, "testtinytext"), "tinytext1");
            assertEquals(cur.getField((UInt64)0, "testmediumtext"), "mediumtext1");
            assertEquals(cur.getField((UInt64)0, "testlongtext"), "longtext1");
            assertEquals(cur.getField((UInt64)0, "testblob"), "blob1");
            assertEquals(cur.getField((UInt64)0, "testlongblob"), "longblob1");
            assertEquals(cur.getField((UInt64)0, "testtinyblob"), "tinyblob1");
            assertEquals(cur.getField((UInt64)0, "testmediumblob"), "mediumblob1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "testtinyint"), "8");
            assertEquals(cur.getField((UInt64)7, "testsmallint"), "8");
            assertEquals(cur.getField((UInt64)7, "testmediumint"), "8");
            assertEquals(cur.getField((UInt64)7, "testint"), "8");
            assertEquals(cur.getField((UInt64)7, "testbigint"), "8");
            assertEquals(cur.getField((UInt64)7, "testfloat"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testreal"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testdecimal"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testdate"), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, "testtime"), "08:00:00");
            assertEquals(cur.getField((UInt64)7, "testdatetime"), "2008-01-01 08:00:00");
            assertEquals(cur.getField((UInt64)7, "testyear"), "2008");
            assertEquals(cur.getField((UInt64)7, "testchar"), "char8");
            assertEquals(cur.getField((UInt64)7, "testvarchar"), "varchar8");
            assertEquals(cur.getField((UInt64)7, "testtext"), "text8");
            assertEquals(cur.getField((UInt64)7, "testtinytext"), "tinytext8");
            assertEquals(cur.getField((UInt64)7, "testmediumtext"), "mediumtext8");
            assertEquals(cur.getField((UInt64)7, "testlongtext"), "longtext8");
            assertEquals(cur.getField((UInt64)7, "testblob"), "blob8");
            assertEquals(cur.getField((UInt64)7, "testlongblob"), "longblob8");
            assertEquals(cur.getField((UInt64)7, "testtinyblob"), "tinyblob8");
            assertEquals(cur.getField((UInt64)7, "testmediumblob"), "mediumblob8");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "testtinyint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testmediumint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testbigint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testreal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testdecimal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testdate"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, "testtime"), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0, "testdatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)0, "testyear"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)0, "testchar"), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)0, "testvarchar"), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0, "testtext"), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)0, "testtinytext"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "testmediumtext"), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)0, "testlongtext"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "testblob"), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)0, "testtinyblob"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "testmediumblob"), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)0, "testlongblob"), (UInt32)9);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "testtinyint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testmediumint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testbigint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testreal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testdecimal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testdate"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, "testtime"), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7, "testdatetime"), (UInt32)19);
            assertEquals(cur.getFieldLength((UInt64)7, "testyear"), (UInt32)4);
            assertEquals(cur.getFieldLength((UInt64)7, "testchar"), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)7, "testvarchar"), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7, "testtext"), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)7, "testtinytext"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "testmediumtext"), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)7, "testlongtext"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "testblob"), (UInt32)5);
            assertEquals(cur.getFieldLength((UInt64)7, "testtinyblob"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "testmediumblob"), (UInt32)11);
            assertEquals(cur.getFieldLength((UInt64)7, "testlongblob"), (UInt32)9);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields = cur.getRow((UInt64)0);
            assertEquals(fields[0], "1");
            assertEquals(fields[1], "1");
            assertEquals(fields[2], "1");
            assertEquals(fields[3], "1");
            assertEquals(fields[4], "1");
            assertEquals(fields[5], "1.5");
            assertEquals(fields[6], "1.5");
            assertEquals(fields[7], "1.5");
            assertEquals(fields[8], "2001-01-01");
            assertEquals(fields[9], "01:00:00");
            assertEquals(fields[10], "2001-01-01 01:00:00");
            assertEquals(fields[11], "2001");
            assertEquals(fields[12], "char1");
            assertEquals(fields[13], "varchar1");
            assertEquals(fields[14], "text1");
            assertEquals(fields[15], "tinytext1");
            assertEquals(fields[16], "mediumtext1");
            assertEquals(fields[17], "longtext1");
            assertEquals(fields[18], "blob1");
            assertEquals(fields[19], "tinyblob1");
            assertEquals(fields[20], "mediumblob1");
            assertEquals(fields[21], "longblob1");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens = cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0], (UInt32)1);
            assertEquals(fieldlens[1], (UInt32)1);
            assertEquals(fieldlens[2], (UInt32)1);
            assertEquals(fieldlens[3], (UInt32)1);
            assertEquals(fieldlens[4], (UInt32)1);
            assertEquals(fieldlens[5],(UInt32)3);
            assertEquals(fieldlens[6], (UInt32)3);
            assertEquals(fieldlens[7], (UInt32)3);
            assertEquals(fieldlens[8], (UInt32)10);
            assertEquals(fieldlens[9], (UInt32)8);
            assertEquals(fieldlens[10], (UInt32)19);
            assertEquals(fieldlens[11], (UInt32)4);
            assertEquals(fieldlens[12], (UInt32)5);
            assertEquals(fieldlens[13], (UInt32)8);
            assertEquals(fieldlens[14], (UInt32)5);
            assertEquals(fieldlens[15], (UInt32)9);
            assertEquals(fieldlens[16], (UInt32)11);
            assertEquals(fieldlens[17], (UInt32)9);
            assertEquals(fieldlens[18], (UInt32)5);
            assertEquals(fieldlens[19], (UInt32)9);
            assertEquals(fieldlens[20], (UInt32)11);
            assertEquals(fieldlens[21], (UInt32)9);
            Console.WriteLine("");


            // result set buffer size
            Console.WriteLine("RESULT SET BUFFER SIZE: ");
            assertEquals(cur.getResultSetBufferSize(), (UInt64)0);
            cur.setResultSetBufferSize(2);
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
            assertEquals(cur.getColumnName(0), (String)null);
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0), (String)null);
            Console.WriteLine("");
            cur.getColumnInfo();
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
            assertEquals(cur.getColumnName(0), "testtinyint");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)1);
            assertEquals(cur.getColumnType((UInt32)0), "TINYINT");
            Console.WriteLine("");


            // suspended session
            Console.WriteLine("SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");


            // column count for cached result set
            Console.WriteLine("COLUMN COUNT FOR CACHED RESULT SET: ");
            assertEquals(cur.colCount(), (UInt32)23);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName(0), "testtinyint");
            assertEquals(cur.getColumnName(1), "testsmallint");
            assertEquals(cur.getColumnName(2), "testmediumint");
            assertEquals(cur.getColumnName(3), "testint");
            assertEquals(cur.getColumnName(4), "testbigint");
            assertEquals(cur.getColumnName(5), "testfloat");
            assertEquals(cur.getColumnName(6), "testreal");
            assertEquals(cur.getColumnName(7), "testdecimal");
            assertEquals(cur.getColumnName(8), "testdate");
            assertEquals(cur.getColumnName(9), "testtime");
            assertEquals(cur.getColumnName(10), "testdatetime");
            assertEquals(cur.getColumnName(11), "testyear");
            assertEquals(cur.getColumnName(12), "testchar");
            assertEquals(cur.getColumnName(13), "testvarchar");
            assertEquals(cur.getColumnName(14), "testtext");
            assertEquals(cur.getColumnName(15), "testtinytext");
            assertEquals(cur.getColumnName(16), "testmediumtext");
            assertEquals(cur.getColumnName(17), "testlongtext");
            assertEquals(cur.getColumnName(18), "testblob");
            assertEquals(cur.getColumnName(19), "testtinyblob");
            assertEquals(cur.getColumnName(20), "testmediumblob");
            assertEquals(cur.getColumnName(21), "testlongblob");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testtinyint");
            assertEquals(cols[1], "testsmallint");
            assertEquals(cols[2], "testmediumint");
            assertEquals(cols[3], "testint");
            assertEquals(cols[4], "testbigint");
            assertEquals(cols[5], "testfloat");
            assertEquals(cols[6], "testreal");
            assertEquals(cols[7], "testdecimal");
            assertEquals(cols[8], "testdate");
            assertEquals(cols[9], "testtime");
            assertEquals(cols[10], "testdatetime");
            assertEquals(cols[11], "testyear");
            assertEquals(cols[12], "testchar");
            assertEquals(cols[13], "testvarchar");
            assertEquals(cols[14], "testtext");
            assertEquals(cols[15], "testtinytext");
            assertEquals(cols[16], "testmediumtext");
            assertEquals(cols[17], "testlongtext");
            assertEquals(cols[18], "testblob");
            assertEquals(cols[19], "testtinyblob");
            assertEquals(cols[20], "testmediumblob");
            assertEquals(cols[21], "testlongblob");
            Console.WriteLine("");


            // cached result set with result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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
            // can't do this with mysql
            //cur.setResultSetBufferSize(1);
            assertTrue(cur.sendQuery("select * from testtable"));
            secondcur = new SQLRCursor(con);
            secondcur.setResultSetBufferSize(1);
            UInt64 nestedrows = 0;
            for (UInt64 i = 0; cur.getRow(i) != null; i++)
            {
                assertTrue(secondcur.sendQuery("select * from testtable"));
                nestedrows++;
            }
            // the nested selects must not disturb the outer result set
            assertEquals(nestedrows, cur.rowCount());
            secondcur.closeResultSet();
            //cur.setResultSetBufferSize(0);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // transaction behavior differs on mysql before 4
            if (majorversion > 3)
            {
                // reset transaction state
                Console.WriteLine("RESET TRANSACTION STATE: ");
                assertTrue(con.commit());
                assertEquals(con.getTransactionModel(), "explicit-deferred");
                assertTrue(con.getAutoCommit());
                Console.WriteLine("");


                // transaction behavior - implicit
                Console.WriteLine("TRANSACTION BEHAVIOR - implicit: ");
                assertTrue(con.setTransactionModel("implicit"));
                assertEquals(con.getTransactionModel(), "implicit");
                assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
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
            }


            // reset transaction behavior
            // (mysql before 4 has limited transaction support)
            Console.WriteLine("RESET TRANSACTION BEHAVIOR: ");
            if (majorversion > 3)
            {
                assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
                assertEquals(con.getTransactionModel(), "explicit-deferred");
                assertTrue(con.getAutoCommit());
            }
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
                + "	testclob1 longtext, "
                + "	testclob2 longtext, "
                + "	testblob1 longblob, "
                + "	testblob2 longblob)"));
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
            cur.inputBindBlob("3", new Byte[0], 0);
            cur.inputBindBlob("4", (Byte[])null, 0);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), (String)null);
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // long lobs
            Console.WriteLine("LONG LOBS: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(
                "create table testtable ("
                + "	testtext longtext, "
                + "	testblob longblob)");
            cur.prepareQuery("insert into testtable values (?,?)");
            cur.inputBindClob("1", largebuffer, LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("2", Encoding.ASCII.GetBytes(largebuffer), LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "testtext"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testtext"), largebuffer);
            assertEquals(cur.getFieldLength((UInt64)0, "testblob"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testblob"), largebuffer);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // output bind by position
            // mysql doesn't support output binds

            // output bind by name
            // mysql doesn't support bind by name


            // output bind by name with validation
            // mysql doesn't support bind by name


            // lob output bind
            // mysql doesn't support output binds


            // long output bind
            // mysql doesn't support output binds


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery("create table testtable (testval int)");
            cur.prepareQuery("insert into testtable values (?)");
            cur.inputBind("1", (Int64)(-1));
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testval from testtable");
            assertEquals(cur.getField((UInt64)0, "testval"), "-1");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // bind validation
            // mysql doesn't support bind by name


            // mysql before 5.0 has no stored procedures
            if (majorversion > 3)
            {
                // rebinding
                Console.WriteLine("REBINDING: ");
                cur.sendQuery("drop procedure testproc");
                assertTrue(cur.sendQuery(
                    "create procedure testproc("
                    + "	in in1 int) "
                    + "begin "
                    + "	select in1; "
                    + "end"));
                cur.prepareQuery("call testproc(?)");
                cur.inputBind("1", (Int64)1);
                assertTrue(cur.executeQuery());
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
                cur.inputBind("1", (Int64)2);
                assertTrue(cur.executeQuery());
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "2");
                cur.inputBind("1", (Int64)3);
                assertTrue(cur.executeQuery());
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "3");
                assertTrue(cur.sendQuery("drop procedure testproc"));
                Console.WriteLine("");
            }


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
            cur.prepareQuery("select ?");
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


            // mysql before 5.0 has no stored procedures
            if (majorversion > 3)
            {
                // stored procedure returning no value
                Console.WriteLine("STORED PROCEDURE RETURNING NO VALUE: ");
                cur.sendQuery("drop procedure testproc");
                assertTrue(cur.sendQuery(
                    "create procedure testproc("
                    + "	in in1 int, "
                    + "	in in2 double, "
                    + "	in in3 varchar(20)) "
                    + "begin "
                    + "end"));
                cur.prepareQuery("call testproc(?,?,?)");
                cur.inputBind("1", (Int64)1);
                cur.inputBind("2", 1.5, 2, 1);
                cur.inputBind("3", "hello");
                assertTrue(cur.executeQuery());
                assertTrue(cur.sendQuery("drop procedure testproc"));
                Console.WriteLine("");


                // stored procedure returning single value
                Console.WriteLine("STORED PROCEDURE RETURNING SINGLE VALUE: ");
                cur.sendQuery("drop procedure testproc");
                assertTrue(cur.sendQuery(
                    "create procedure testproc("
                    + "	in in1 int, "
                    + "	in in2 double, "
                    + "	in in3 varchar(20)) "
                    + "begin "
                    + "	select in1; "
                    + "end"));
                cur.prepareQuery("call testproc(?,?,?)");
                cur.inputBind("1", (Int64)1);
                cur.inputBind("2", 1.5, 2, 1);
                cur.inputBind("3", "hello");
                assertTrue(cur.executeQuery());
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
                assertTrue(cur.sendQuery("drop procedure testproc"));
                Console.WriteLine("");


                // stored procedure returning multiple values
                Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
                cur.sendQuery("drop procedure testproc");
                assertTrue(cur.sendQuery(
                    "create procedure testproc("
                    + "	in in1 int, "
                    + "	in in2 double, "
                    + "	in in3 varchar(20)) "
                    + "begin "
                    + "	select in1, in2, in3; "
                    + "end"));
                cur.prepareQuery("call testproc(?,?,?)");
                cur.inputBind("1", (Int64)1);
                cur.inputBind("2", 1.5, 2, 1);
                cur.inputBind("3", "hello");
                assertTrue(cur.executeQuery());
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
                assertEquals(cur.getField((UInt64)0, (UInt32)1), "1.5");
                assertEquals(cur.getField((UInt64)0, (UInt32)2), "hello");
                assertTrue(cur.sendQuery("drop procedure testproc"));
                Console.WriteLine("");


                // stored procedure returning result set
                Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
                cur.sendQuery("drop procedure testselectproc");
                assertTrue(cur.sendQuery(
                    "create procedure testselectproc() "
                    + "begin "
                    + "	select 1 "
                    + "	union "
                    + "	select 2 "
                    + "	union "
                    + "	select 3 "
                    + "	union "
                    + "	select 4 "
                    + "	union "
                    + "	select 5 "
                    + "	union "
                    + "	select 6 "
                    + "	union "
                    + "	select 7 "
                    + "	union "
                    + "	select 8; "
                    + "end"));
                assertTrue(cur.sendQuery("call testselectproc()"));
                assertEquals(cur.rowCount(), (UInt64)8);
                assertTrue(cur.sendQuery("drop procedure testselectproc"));
                Console.WriteLine("");


                // temporary tables
                Console.WriteLine("TEMPORARY TABLES: ");
                cur.sendQuery("drop table temptable");
                cur.sendQuery("create temporary table temptable (col1 int)");
                assertTrue(cur.sendQuery("insert into temptable values (1)"));
                assertTrue(cur.sendQuery("select count(*) from temptable"));
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
                con.endSession();
                Console.WriteLine("");
                assertFalse(cur.sendQuery("select count(*) from temptable"));
                Console.WriteLine("");
            }

            if (majorversion > 3)
            {

                // stored procedure returning no value
                Console.WriteLine("STORED PROCEDURE RETURNING NO VALUE: ");
                cur.sendQuery("drop procedure if exists testproc");
                assertTrue(cur.sendQuery(
                    "create procedure testproc("
                    + "	in in1 int, "
                    + "	in in2 float, "
                    + "	in in3 char(20)) "
                    + "begin "
                    + "	select in1, in2, in3; "
                    + "end;"));
                cur.prepareQuery("call testproc(?,?,?)");
                cur.inputBind("1", (Int64)1);
                cur.inputBind("2", 1.5, 4, 2);
                cur.inputBind("3", "hello");
                assertTrue(cur.executeQuery());
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
                assertEquals(cur.getField((UInt64)0, (UInt32)1), "1.5");
                assertEquals(cur.getField((UInt64)0, (UInt32)2), "hello");
                cur.sendQuery("drop procedure testproc");
                Console.WriteLine("");


                // stored procedure returning one value
                Console.WriteLine("FUNCTIONS: ");
                cur.sendQuery("drop function if exists testfunc");
                assertTrue(cur.sendQuery(
                    "create function testfunc(in1 int, in2 "
                    + "	int) returns int return in1+in2;"));
                cur.prepareQuery("select testfunc(?,?)");
                cur.inputBind("1", (Int64)10);
                cur.inputBind("2", (Int64)20);
                assertTrue(cur.executeQuery());
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "30");
                cur.sendQuery("drop function if exists testfunc");
                Console.WriteLine("");


                // stored procedure returning multiple values
                Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
                cur.sendQuery("drop procedure if exists testproc");
                assertTrue(cur.sendQuery(
                    "create procedure testproc("
                    + "	out out1 int, "
                    + "	out out2 float, "
                    + "	out out3 char(20)) "
                    + "begin "
                    + "	select 1, 2.5, 'hello' "
                    + "		into out1, out2, out3; "
                    + "end;"));
                assertTrue(cur.sendQuery("set @out1=0, @out2=0.0, @out3=''"));
                assertTrue(cur.sendQuery("call testproc(@out1,@out2,@out3)"));
                assertTrue(cur.sendQuery("select @out1, @out2, @out3"));
                assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
                assertEquals(cur.getFieldAsDouble((UInt64)0, (UInt32)1), 2.5);
                assertEquals(cur.getField((UInt64)0, (UInt32)2), "hello");
                cur.sendQuery("drop procedure testproc");
                Console.WriteLine("");


                // stored procedure returning result set
                Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
                cur.sendQuery("drop procedure if exists testselectproc");
                assertTrue(cur.sendQuery(
                    "create procedure testselectproc() "
                    + "begin "
                    + "	select 1 "
                    + "	union "
                    + "	select 2 "
                    + "	union "
                    + "	select 3 "
                    + "	union "
                    + "	select 4 "
                    + "	union "
                    + "	select 5 "
                    + "	union "
                    + "	select 6 "
                    + "	union "
                    + "	select 7 "
                    + "	union "
                    + "	select 8; "
                    + "end"));
                assertTrue(cur.sendQuery("call testselectproc()"));
                assertEquals(cur.rowCount(), (UInt64)8);
                cur.sendQuery("drop procedure testselectproc");
                Console.WriteLine("");
            }


            if (majorversion > 3)
            {

                // encoded binary data - all chars - \-escaped
                //
                // NOTE: The native C# SQLRCursor.sendQuery marshals String
                // to the native char* via platform-default ANSI marshaling,
                // which may not losslessly pass raw bytes 0x80-0xFF. To
                // verify the round-tripped bytes we read the column back
                // via server-side HEX() (same approach as the Node.js port)
                // rather than comparing raw bytes client-side.
                Console.WriteLine("ENCODED BINARY DATA - "
                            + "all chars - \\-escaped: ");
                cur.sendQuery("drop table testtable");
                assertTrue(cur.sendQuery(
                    "create table testtable (col1 longblob)"));
                Byte[] buffer = new Byte[256];
                for (Int32 i = 0; i < 256; i++)
                {
                    buffer[i] = (Byte)i;
                }
                // Build the INSERT as a raw byte array so that bytes
                // 0-255 (including NUL) pass through to MySQL unmolested.
                // String-marshaling via Mono's default UTF-8 path would
                // expand bytes >= 0x80 and truncate at embedded NULs.
                System.Collections.Generic.List<Byte> qbuf =
                    new System.Collections.Generic.List<Byte>();
                qbuf.AddRange(System.Text.Encoding.ASCII.GetBytes(
                    "insert into testtable values (_binary'"));
                for (Int32 i = 0; i < buffer.Length; i++)
                {
                    if (buffer[i] == (Byte)'\'') { qbuf.Add((Byte)'\\'); }
                    if (buffer[i] == (Byte)'\\') { qbuf.Add((Byte)'\\'); }
                    qbuf.Add(buffer[i]);
                }
                qbuf.AddRange(System.Text.Encoding.ASCII.GetBytes("')"));
                Byte[] querybytes = qbuf.ToArray();
                assertTrue(cur.sendQuery(querybytes, (UInt32)querybytes.Length));
                assertTrue(cur.sendQuery("select hex(col1) from testtable"));
                StringBuilder expectedhex = new StringBuilder();
                for (Int32 i = 0; i < buffer.Length; i++)
                {
                    expectedhex.Append(buffer[i].ToString("x2"));
                }
                String expectedhexstr = expectedhex.ToString();
                assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)expectedhexstr.Length);
                assertEquals(cur.getField((UInt64)0, (UInt32)0).ToLower(), expectedhexstr);
                assertTrue(cur.sendQuery("drop table testtable"));
                Console.WriteLine("");


                // encoded binary data - (null)"" - unescaped
                Console.WriteLine("ENCODED BINARY DATA - "
                            + "(null)\"\" - unescaped: ");
                cur.sendQuery("drop table testtable");
                assertTrue(cur.sendQuery(
                    "create table testtable (col1 longblob)"));
                Byte[] q2 = new Byte[43];
                System.Text.Encoding.ASCII.GetBytes(
                    "insert into testtable values (_binary'", 0, 38, q2, 0);
                q2[38] = 0x00; q2[39] = (Byte)'"'; q2[40] = (Byte)'"';
                q2[41] = (Byte)'\''; q2[42] = (Byte)')';
                assertTrue(cur.sendQuery(q2, (UInt32)q2.Length));
                assertTrue(cur.sendQuery("select hex(col1) from testtable"));
                assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)6);
                assertEquals(cur.getField((UInt64)0, (UInt32)0).ToLower(), "002222");
                assertTrue(cur.sendQuery("drop table testtable"));
                Console.WriteLine("");


                // encoded binary data - (null)"" - \-escaped
                Console.WriteLine("ENCODED BINARY DATA - "
                            + "\\(null)\\\"\\\" - \\-escaped: ");
                cur.sendQuery("drop table testtable");
                assertTrue(cur.sendQuery(
                    "create table testtable (col1 longblob)"));
                Byte[] q3 = new Byte[46];
                System.Text.Encoding.ASCII.GetBytes(
                    "insert into testtable values (_binary'", 0, 38, q3, 0);
                q3[38] = (Byte)'\\'; q3[39] = 0x00;
                q3[40] = (Byte)'\\'; q3[41] = (Byte)'"';
                q3[42] = (Byte)'\\'; q3[43] = (Byte)'"';
                q3[44] = (Byte)'\''; q3[45] = (Byte)')';
                assertTrue(cur.sendQuery(q3, (UInt32)q3.Length));
                assertTrue(cur.sendQuery("select hex(col1) from testtable"));
                assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)6);
                assertEquals(cur.getField((UInt64)0, (UInt32)0).ToLower(), "002222");
                assertTrue(cur.sendQuery("drop table testtable"));
                Console.WriteLine("");
            }


            // quotes - '' - ''-escaped
            Console.WriteLine("QUOTES - '' - ''-escaped: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"));
            assertTrue(cur.sendQuery("insert into testtable values ('''''')"));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "''");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // quotes - '' - '',\-escaped
            Console.WriteLine("QUOTES - '' - '',\\-escaped: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"));
            assertTrue(cur.sendQuery("insert into testtable values ('''\\'')"));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "''");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // quotes - '' - \,''-escaped
            Console.WriteLine("QUOTES - '' - \\,''-escaped: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"));
            assertTrue(cur.sendQuery("insert into testtable values ('\\'''')"));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "''");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // quotes - \\' - \-escaped
            Console.WriteLine("QUOTES - \\\\' - \\-escaped: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"));
            assertTrue(cur.sendQuery("insert into testtable values ('\\\\\\'')"));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "\\\'", 2);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // quotes - "" - unescaped
            Console.WriteLine("QUOTES - \"\" - unescaped: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 varchar(4))"));
            assertTrue(cur.sendQuery("insert into testtable values ('\"\"')"));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "\"\"");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // quotes - random - '',\-escaped
            //
            // NOTE: See the ENCODED BINARY DATA section note above. We
            // build a random byte buffer of [', ", \, \0] and verify the
            // round-tripped bytes via server-side HEX() so that embedded
            // \0 doesn't truncate the field compare through the binding.
            Console.WriteLine("QUOTES - random - '',\\-escaped: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable "
                            + "(col1 varchar(255))"));
            Random r1 = new Random();
            Random r2 = new Random();
            Byte[] qbuffer = new Byte[255];
            Byte[] ch = new Byte[] { (Byte)'\'', (Byte)'"', (Byte)'\\', (Byte)'\0' };
            for (Int32 i = 0; i < qbuffer.Length; i++)
            {
                qbuffer[i] = ch[r1.Next(0, 4)];
            }
            // Build as a byte buffer — embedded \0 would truncate the
            // query at the native boundary if we used a C# String.
            System.Collections.Generic.List<Byte> qqbuf =
                new System.Collections.Generic.List<Byte>();
            qqbuf.AddRange(System.Text.Encoding.ASCII.GetBytes(
                "insert into testtable values ('"));
            for (Int32 i = 0; i < qbuffer.Length; i++)
            {
                if (qbuffer[i] == (Byte)'\'')
                {
                    // randomly escape with \ or ''
                    if (r2.Next(0, 2) != 0) { qqbuf.Add((Byte)'\''); }
                    else                    { qqbuf.Add((Byte)'\\'); }
                }
                if (qbuffer[i] == (Byte)'"')
                {
                    // randomly escape with \ or don't escape
                    if (r2.Next(0, 2) != 0) { qqbuf.Add((Byte)'\\'); }
                }
                if (qbuffer[i] == (Byte)'\\')
                {
                    // escape with backslash
                    qqbuf.Add((Byte)'\\');
                }
                qqbuf.Add(qbuffer[i]);
            }
            qqbuf.AddRange(System.Text.Encoding.ASCII.GetBytes("')"));
            Byte[] qquerybytes = qqbuf.ToArray();
            assertTrue(cur.sendQuery(qquerybytes, (UInt32)qquerybytes.Length));
            assertTrue(cur.sendQuery("select hex(col1) from testtable"));
            StringBuilder qexpectedhex = new StringBuilder();
            for (Int32 i = 0; i < qbuffer.Length; i++)
            {
                qexpectedhex.Append(qbuffer[i].ToString("x2"));
            }
            String qexpectedhexstr = qexpectedhex.ToString();
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)qexpectedhexstr.Length);
            assertEquals(cur.getField((UInt64)0, (UInt32)0).ToLower(), qexpectedhexstr);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // last insert id
            Console.WriteLine("LAST INSERT ID: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                    "create table testtable "
                    + "	(col1 int primary key auto_increment, "
                    + "	col2 int)"));
            assertTrue(cur.sendQuery("insert into testtable values (null,1)"));
            assertEquals(con.getLastInsertId(), (UInt64)1);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // database is schema
            Console.WriteLine("DATABASE IS SCHEMA: ");
            assertFalse(con.getDatabaseIsSchema());
            Console.WriteLine("");


            // mysql before 5.0 has no information_schema for these metadata queries
            if (majorversion > 3)
            {
                // catalog list
                Console.WriteLine("CATALOG LIST: ");
                assertTrue(cur.getCatalogList(null));
                assertEquals(cur.getColumnName(0), "Database");
                assertInResultSet(cur, "Database", hostname);
                Console.WriteLine("");


                // schema list
                Console.WriteLine("SCHEMA LIST: ");
                assertTrue(cur.getSchemaList(null));
                assertEquals(cur.getColumnName(0), "Database");
                // mysql has no schemas
                assertEquals(cur.rowCount(), (UInt64)0);
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
                assertEquals(cur.getField((UInt64)0, "precision"), "255");
                assertEquals(cur.getField((UInt64)0, "local_type_name"), "CHAR");
                assertTrue(cur.getTypeInfoList("varchar"));
                assertEquals(cur.getField((UInt64)0, "type_name"), "VARCHAR");
                assertEquals(cur.getField((UInt64)0, "data_type"), "12");
                assertEquals(cur.getField((UInt64)0, "precision"), "65535");
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
                    "create table testtable ("
                    + "	testtinyint tinyint, "
                    + "	testsmallint smallint, "
                    + "	testmediumint mediumint, "
                    + "	testint int, "
                    + "	testbigint bigint, "
                    + "	testfloat float, "
                    + "	testreal real, "
                    + "	testdecimal decimal(2,1), "
                    + "	testdate date, "
                    + "	testtime time, "
                    + "	testdatetime datetime, "
                    + "	testyear year, "
                    + "	testchar char(40), "
                    + "	testvarchar varchar(40), "
                    + "	testtext text, "
                    + "	testtinytext tinytext, "
                    + "	testmediumtext mediumtext, "
                    + "	testlongtext longtext, "
                    + "	testblob blob, "
                    + "	testtinyblob tinyblob, "
                    + "	testmediumblob mediumblob, "
                    + "	testlongblob longblob, "
                    + "	testtimestamp timestamp)"));
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
                assertEquals(cur.getField((UInt64)0, "column_name"), "testtinyint");
                assertEquals(cur.getField((UInt64)1, "column_name"), "testsmallint");
                assertEquals(cur.getField((UInt64)2, "column_name"), "testmediumint");
                assertEquals(cur.getField((UInt64)3, "column_name"), "testint");
                assertEquals(cur.getField((UInt64)4, "column_name"), "testbigint");
                assertEquals(cur.getField((UInt64)5, "column_name"), "testfloat");
                assertEquals(cur.getField((UInt64)6, "column_name"), "testreal");
                assertEquals(cur.getField((UInt64)7, "column_name"), "testdecimal");
                assertEquals(cur.getField((UInt64)8, "column_name"), "testdate");
                assertEquals(cur.getField((UInt64)9, "column_name"), "testtime");
                assertEquals(cur.getField((UInt64)10, "column_name"), "testdatetime");
                assertEquals(cur.getField((UInt64)11, "column_name"), "testyear");
                assertEquals(cur.getField((UInt64)12, "column_name"), "testchar");
                assertEquals(cur.getField((UInt64)13, "column_name"), "testvarchar");
                assertEquals(cur.getField((UInt64)14, "column_name"), "testtext");
                assertEquals(cur.getField((UInt64)15, "column_name"), "testtinytext");
                assertEquals(cur.getField((UInt64)16, "column_name"), "testmediumtext");
                assertEquals(cur.getField((UInt64)17, "column_name"), "testlongtext");
                assertEquals(cur.getField((UInt64)18, "column_name"), "testblob");
                assertEquals(cur.getField((UInt64)19, "column_name"), "testtinyblob");
                assertEquals(cur.getField((UInt64)20, "column_name"), "testmediumblob");
                assertEquals(cur.getField((UInt64)21, "column_name"), "testlongblob");
                assertEquals(cur.getField((UInt64)22, "column_name"), "testtimestamp");
                assertEquals(cur.getField((UInt64)0, "data_type"), "TINYINT");
                assertEquals(cur.getField((UInt64)1, "data_type"), "SMALLINT");
                assertEquals(cur.getField((UInt64)2, "data_type"), "MEDIUMINT");
                assertEquals(cur.getField((UInt64)3, "data_type"), "INT");
                assertEquals(cur.getField((UInt64)4, "data_type"), "BIGINT");
                assertEquals(cur.getField((UInt64)5, "data_type"), "FLOAT");
                assertEquals(cur.getField((UInt64)6, "data_type"), "DOUBLE"); // not "REAL"
                assertEquals(cur.getField((UInt64)7, "data_type"), "DECIMAL");
                assertEquals(cur.getField((UInt64)8, "data_type"), "DATE");
                assertEquals(cur.getField((UInt64)9, "data_type"), "TIME");
                assertEquals(cur.getField((UInt64)10, "data_type"), "DATETIME");
                assertEquals(cur.getField((UInt64)11, "data_type"), "YEAR");
                assertEquals(cur.getField((UInt64)12, "data_type"), "CHAR");
                assertEquals(cur.getField((UInt64)13, "data_type"), "VARCHAR");
                assertEquals(cur.getField((UInt64)14, "data_type"), "TEXT");
                assertEquals(cur.getField((UInt64)15, "data_type"), "TINYTEXT");
                assertEquals(cur.getField((UInt64)16, "data_type"), "MEDIUMTEXT");
                assertEquals(cur.getField((UInt64)17, "data_type"), "LONGTEXT");
                assertEquals(cur.getField((UInt64)18, "data_type"), "BLOB");
                assertEquals(cur.getField((UInt64)19, "data_type"), "TINYBLOB");
                assertEquals(cur.getField((UInt64)20, "data_type"), "MEDIUMBLOB");
                assertEquals(cur.getField((UInt64)21, "data_type"), "LONGBLOB");
                assertEquals(cur.getField((UInt64)22, "data_type"), "TIMESTAMP");
                assertTrue(cur.sendQuery("drop table testtable"));
                Console.WriteLine("");


                // column list - auto_increment, primary key
                Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
                cur.sendQuery("drop table testtable");
                assertTrue(cur.sendQuery(
                    "create table testtable ("
                    + "	col1 int auto_increment primary key, "
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
                assertEquals(cur.getField((UInt64)0, "key_name"), "PRIMARY");
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
                assertEquals(cur.getField((UInt64)0, "non_unique"), "false");
                assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
                assertTrue(cur.getField((UInt64)0, "column_name") == "col1");
                assertEquals(cur.getField((UInt64)0, "collation"), "A");
                assertEquals(cur.getField((UInt64)0, "index_type"), "3");
                assertEquals(cur.getField((UInt64)0, "key_name"), "PRIMARY");
                assertTrue(cur.sendQuery("drop table testtable"));
                Console.WriteLine("");


                // procedure list
                Console.WriteLine("PROCEDURE LIST: ");
                cur.sendQuery("drop procedure testproc1");
                cur.sendQuery("drop procedure testproc2");
                cur.sendQuery("drop procedure testproc3");
                cur.sendQuery("drop procedure testproc4");
                assertTrue(cur.sendQuery(
                    "create procedure testproc1("
                    + "	in in1 int, "
                    + "	in in2 char(20), "
                    + "	in in3 varchar(20), "
                    + "	in in4 date) "
                    + "begin end"));
                assertTrue(cur.sendQuery(
                    "create procedure testproc2("
                    + "	in in1 int, "
                    + "	in in2 char(20), "
                    + "	in in3 varchar(20), "
                    + "	in in4 date) "
                    + "begin end"));
                assertTrue(cur.sendQuery(
                    "create procedure testproc3("
                    + "	in in1 int, "
                    + "	in in2 char(20), "
                    + "	in in3 varchar(20), "
                    + "	in in4 date) "
                    + "begin end"));
                assertTrue(cur.sendQuery(
                    "create procedure testproc4("
                    + "	in in1 int, "
                    + "	in in2 char(20), "
                    + "	in in3 varchar(20), "
                    + "	in in4 date) "
                    + "begin end"));
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
                assertEquals(cur.getField((UInt64)0, "parameter_name"), "in1");
                assertEquals(cur.getField((UInt64)0, "parameter_mode"), "1");
                assertEquals(cur.getField((UInt64)0, "data_type"), "INT");
                assertEquals(cur.getField((UInt64)0, "ordinal_position"), "1");
                assertEquals(cur.getField((UInt64)1, "parameter_name"), "in2");
                assertEquals(cur.getField((UInt64)1, "parameter_mode"), "1");
                assertEquals(cur.getField((UInt64)1, "data_type"), "CHAR");
                assertEquals(cur.getField((UInt64)1, "ordinal_position"), "2");
                assertEquals(cur.getField((UInt64)2, "parameter_name"), "in3");
                assertEquals(cur.getField((UInt64)2, "parameter_mode"), "1");
                assertEquals(cur.getField((UInt64)2, "data_type"), "VARCHAR");
                assertEquals(cur.getField((UInt64)2, "ordinal_position"), "3");
                assertEquals(cur.getField((UInt64)3, "parameter_name"), "in4");
                assertEquals(cur.getField((UInt64)3, "parameter_mode"), "1");
                assertEquals(cur.getField((UInt64)3, "data_type"), "DATE");
                assertEquals(cur.getField((UInt64)3, "ordinal_position"), "4");
                assertTrue(cur.sendQuery("drop procedure testproc1"));
                assertTrue(cur.sendQuery("drop procedure testproc2"));
                assertTrue(cur.sendQuery("drop procedure testproc3"));
                assertTrue(cur.sendQuery("drop procedure testproc4"));
                Console.WriteLine("");
            }


            // invalid queries
            Console.WriteLine("INVALID QUERIES: ");
            assertFalse(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
            assertFalse(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
            assertFalse(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
            assertFalse(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testtable "
                + "order by "
                + "	testtinyint "));
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

            cur = null;
            con = null;

            reportTestStatus();

            return status;
        }
    }
}
