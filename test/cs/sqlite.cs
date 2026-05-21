// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class SqliteTest : SQLRTest
    {
        public static int Main(string[] args)
        {

            String[] isolationlevels = new String[] { "0", "1" };
            String[] subvars = new String[] { "var1", "var2", "var3" };
            String[] subvalstrings = new String[] { "hi", "hello", "bye" };
            Int64[] subvallongs = new Int64[] { 1, 2, 3 };
            Double[] subvaldoubles = new Double[] { 10.55, 10.556, 10.5556 };
            UInt32[] precs = new UInt32[] { 4, 5, 6 };
            UInt32[] scales = new UInt32[] { 2, 3, 4 };
            String[] cols;
            String[] fields;
            UInt16 port;
            String socket;
            UInt16 id;
            String filename;
            UInt64 counter = 0;
            UInt32[] fieldlens;

            const int LARGE_BUFFER_LENGTH = 8192;
            char[] largebufferchars = new char[LARGE_BUFFER_LENGTH];
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++) {
                largebufferchars[i] = 'C';
            }
            String largebuffer = new String(largebufferchars);


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                "testuser", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "sqlite");
            Console.WriteLine("");


            // db version
            Console.WriteLine("DB VERSION: ");
            int issqlite3 = 1;
            String dbversion = con.dbVersion();
            if (dbversion == null || dbversion == "" || dbversion == "unknown") {
                issqlite3 = 0;
            } else {
                int dotpos = dbversion.IndexOf('.');
                String major = (dotpos > 0) ? dbversion.Substring(0, dotpos) : dbversion;
                int majorversion;
                if (!Int32.TryParse(major, out majorversion) || majorversion < 3) {
                    issqlite3 = 0;
                }
            }
            Console.WriteLine("");


            // ping
            Console.WriteLine("PING: ");
            assertTrue(con.ping());
            Console.WriteLine("");


            // transaction state
            Console.WriteLine("TRANSACTION STATE: ");
            assertEquals(con.getDefaultTransactionModel(), "explicit");
            assertEquals(con.getTransactionModel(), "explicit");
            assertFalse(con.getInTransaction());
            assertTrue(con.getAutoCommit());
            Console.WriteLine("");


            // bind format
            Console.WriteLine("BIND FORMAT: ");
            assertEquals(con.bindFormat(), ":*");
            Console.WriteLine("");


            // nextval format
            Console.WriteLine("NEXTVAL FORMAT: ");
            assertEquals(con.nextvalFormat(), "");
            Console.WriteLine("");


            // isolation levels
            Console.WriteLine("ISOLATION LEVELS: ");
            for (int i = 0; i < isolationlevels.Length; i++) {
                String il = isolationlevels[i];
                assertTrue(con.setIsolationLevel(il));
                assertEquals(con.getIsolationLevel(), il);
                Console.WriteLine("");
            }
            // reset to the default isolation level
            assertTrue(con.setIsolationLevel(isolationlevels[0]));
            Console.WriteLine("");


            // create testtable
            Console.WriteLine("CREATE TESTTABLE: ");
            con.begin();
            cur.sendQuery("drop table if exists testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testint int, " +
                "	testfloat float, " +
                "	testchar char(40), " +
                "	testvarchar varchar(40), " +
                "	testclob clob, " +
                "	testblob blob)"));
            con.commit();
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            assertTrue(con.begin());
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	1, " +
                "	1.1, " +
                "	'testchar1', " +
                "	'testvarchar1', " +
                "	'testclob1', " +
                "	'testblob1')"));
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	2, " +
                "	2.2, " +
                "	'testchar2', " +
                "	'testvarchar2', " +
                "	'testclob2', " +
                "	'testblob2')"));
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	3, " +
                "	3.3, " +
                "	'testchar3', " +
                "	'testvarchar3', " +
                "	'testclob3', " +
                "	'testblob3')"));
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	4, " +
                "	4.4, " +
                "	'testchar4', " +
                "	'testvarchar4', " +
                "	'testclob4', " +
                "	'testblob4')"));
            Console.WriteLine("");


            // affected rows
            Console.WriteLine("AFFECTED ROWS: ");
            assertEquals(cur.affectedRows(), (UInt64)1);
            Console.WriteLine("");


            // input bind by position
            // sqlite doesn't support bind by position


            // array of input binds by position
            // sqlite doesn't support bind by position


            // input bind by position with validation
            // sqlite doesn't support bind by position


            // input bind by name
            Console.WriteLine("INPUT BIND BY NAME: ");
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	:var1, " +
                "	:var2, " +
                "	:var3, " +
                "	:var4, " +
                "	:var5, " +
                "	:var6)");
            assertEquals(cur.countBindVariables(), (UInt16)6);
            cur.inputBind("var1", (Int64)5);
            cur.inputBind("var2", 5.5, (UInt32)4, (UInt32)1);
            cur.inputBind("var3", "testchar5");
            cur.inputBind("var4", "testvarchar5");
            cur.inputBindClob("var5", "testclob5", (UInt32)9);
            cur.inputBindBlob("var6", System.Text.Encoding.ASCII.GetBytes("testblob5"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("var1", (Int64)6);
            cur.inputBind("var2", 6.6, (UInt32)4, (UInt32)1);
            cur.inputBind("var3", "testchar6");
            cur.inputBind("var4", "testvarchar6");
            cur.inputBindClob("var5", "testclob6", (UInt32)9);
            cur.inputBindBlob("var6", System.Text.Encoding.ASCII.GetBytes("testblob6"), (UInt32)9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("var1", (Int64)7);
            cur.inputBind("var2", 7.7, (UInt32)4, (UInt32)1);
            cur.inputBind("var3", "testchar7");
            cur.inputBind("var4", "testvarchar7");
            cur.inputBindClob("var5", "testclob7", (UInt32)9);
            cur.inputBindBlob("var6", System.Text.Encoding.ASCII.GetBytes("testblob7"), (UInt32)9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by name
            // sqlite doesn't support implicit conversion of string binds to other
            // data types, so arrays of binds don't generally work.


            // input bind by name with validation
            Console.WriteLine("INPUT BIND BY NAME WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("var1", (Int64)8);
            cur.inputBind("var2", 8.8, (UInt32)4, (UInt32)1);
            cur.inputBind("var3", "testchar8");
            cur.inputBind("var4", "testvarchar8");
            cur.inputBindClob("var5", "testclob8", (UInt32)9);
            cur.inputBindBlob("var6", System.Text.Encoding.ASCII.GetBytes("testblob8"), (UInt32)9);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(), (UInt32)6);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName((UInt32)0), "testint");
            assertEquals(cur.getColumnName((UInt32)1), "testfloat");
            assertEquals(cur.getColumnName((UInt32)2), "testchar");
            assertEquals(cur.getColumnName((UInt32)3), "testvarchar");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testint");
            assertEquals(cols[1], "testfloat");
            assertEquals(cols[2], "testchar");
            assertEquals(cols[3], "testvarchar");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            if (issqlite3 != 0) {
                assertEquals(cur.getColumnType((UInt32)0), "INTEGER");
                assertEquals(cur.getColumnType("testint"), "INTEGER");
                assertEquals(cur.getColumnType((UInt32)1), "FLOAT");
                assertEquals(cur.getColumnType("testfloat"), "FLOAT");
                assertEquals(cur.getColumnType((UInt32)2), "STRING");
                assertEquals(cur.getColumnType("testchar"), "STRING");
                assertEquals(cur.getColumnType((UInt32)3), "STRING");
                assertEquals(cur.getColumnType("testvarchar"), "STRING");
                assertEquals(cur.getColumnType((UInt32)4), "STRING");
                assertEquals(cur.getColumnType("testclob"), "STRING");
                assertEquals(cur.getColumnType((UInt32)5), "STRING");
                assertEquals(cur.getColumnType("testblob"), "STRING");
            } else {
                assertEquals(cur.getColumnType((UInt32)0), "UNKNOWN");
                assertEquals(cur.getColumnType("testint"), "UNKNOWN");
                assertEquals(cur.getColumnType((UInt32)1), "UNKNOWN");
                assertEquals(cur.getColumnType("testfloat"), "UNKNOWN");
                assertEquals(cur.getColumnType((UInt32)2), "UNKNOWN");
                assertEquals(cur.getColumnType("testchar"), "UNKNOWN");
                assertEquals(cur.getColumnType((UInt32)3), "UNKNOWN");
                assertEquals(cur.getColumnType("testvarchar"), "UNKNOWN");
                assertEquals(cur.getColumnType((UInt32)4), "UNKNOWN");
                assertEquals(cur.getColumnType("testclob"), "UNKNOWN");
                assertEquals(cur.getColumnType((UInt32)5), "UNKNOWN");
                assertEquals(cur.getColumnType("testblob"), "UNKNOWN");
            }
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnLength("testint"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)0);
            assertEquals(cur.getColumnLength("testfloat"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)2), (UInt32)0);
            assertEquals(cur.getColumnLength("testchar"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)3), (UInt32)0);
            assertEquals(cur.getColumnLength("testvarchar"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)4), (UInt32)0);
            assertEquals(cur.getColumnLength("testclob"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)5), (UInt32)0);
            assertEquals(cur.getColumnLength("testblob"), (UInt32)0);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest("testint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)1), (UInt32)3);
            assertEquals(cur.getLongest("testfloat"), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)2), (UInt32)9);
            assertEquals(cur.getLongest("testchar"), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)3), (UInt32)12);
            assertEquals(cur.getLongest("testvarchar"), (UInt32)12);
            assertEquals(cur.getLongest((UInt32)4), (UInt32)9);
            assertEquals(cur.getLongest("testclob"), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)5), (UInt32)9);
            assertEquals(cur.getLongest("testblob"), (UInt32)9);
            Console.WriteLine("");


            // row count
            Console.WriteLine("ROW COUNT: ");
            assertEquals(cur.rowCount(), (UInt64)8);
            Console.WriteLine("");


            // total rows
            Console.WriteLine("TOTAL ROWS: ");
            assertEquals(cur.totalRows(), (issqlite3 != 0) ? (UInt64)0 : (UInt64)8);
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
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1.1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "testchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)4), "testclob1");
            assertEquals(cur.getField((UInt64)0, (UInt32)5), "testblob1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8.8");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "testchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)3), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)4), "testclob8");
            assertEquals(cur.getField((UInt64)7, (UInt32)5), "testblob8");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)4), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)5), (UInt32)9);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)4), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)5), (UInt32)9);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "testint"), "1");
            assertEquals(cur.getField((UInt64)0, "testfloat"), "1.1");
            assertEquals(cur.getField((UInt64)0, "testchar"), "testchar1");
            assertEquals(cur.getField((UInt64)0, "testvarchar"), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, "testclob"), "testclob1");
            assertEquals(cur.getField((UInt64)0, "testblob"), "testblob1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "testint"), "8");
            assertEquals(cur.getField((UInt64)7, "testfloat"), "8.8");
            assertEquals(cur.getField((UInt64)7, "testchar"), "testchar8");
            assertEquals(cur.getField((UInt64)7, "testvarchar"), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, "testclob"), "testclob8");
            assertEquals(cur.getField((UInt64)7, "testblob"), "testblob8");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testchar"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, "testclob"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "testblob"), (UInt32)9);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testchar"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, "testclob"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "testblob"), (UInt32)9);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields = cur.getRow((UInt64)0);
            assertEquals(fields[0], "1");
            assertEquals(fields[1], "1.1");
            assertEquals(fields[2], "testchar1");
            assertEquals(fields[3], "testvarchar1");
            assertEquals(fields[4], "testclob1");
            assertEquals(fields[5], "testblob1");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens = cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0], (UInt32)1);
            assertEquals(fieldlens[1], (UInt32)3);
            assertEquals(fieldlens[2], (UInt32)9);
            assertEquals(fieldlens[3], (UInt32)12);
            assertEquals(fieldlens[4], (UInt32)9);
            assertEquals(fieldlens[5], (UInt32)9);
            Console.WriteLine("");


            // result set buffer size
            Console.WriteLine("RESULT SET BUFFER SIZE: ");
            assertEquals(cur.getResultSetBufferSize(), (UInt64)0);
            cur.setResultSetBufferSize((UInt64)2);
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
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // dont get column info
            Console.WriteLine("DONT GET COLUMN INFO: ");
            cur.dontGetColumnInfo();
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            assertEquals(cur.getColumnName((UInt32)0), (String)null);
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0), (String)null);
            cur.getColumnInfo();
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            assertEquals(cur.getColumnName((UInt32)0), "testint");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0),
                        (issqlite3 != 0) ? "INTEGER" : "UNKNOWN");
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


            // suspended result set
            Console.WriteLine("SUSPENDED RESULT SET: ");
            cur.setResultSetBufferSize((UInt64)2);
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
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // cached result set
            Console.WriteLine("CACHED RESULT SET: ");
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl((UInt32)200);
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");


            // column count for cached result set
            Console.WriteLine("COLUMN COUNT FOR CACHED RESULT SET: ");
            assertEquals(cur.colCount(), (UInt32)6);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName((UInt32)0), "testint");
            assertEquals(cur.getColumnName((UInt32)1), "testfloat");
            assertEquals(cur.getColumnName((UInt32)2), "testchar");
            assertEquals(cur.getColumnName((UInt32)3), "testvarchar");
            assertEquals(cur.getColumnName((UInt32)4), "testclob");
            assertEquals(cur.getColumnName((UInt32)5), "testblob");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testint");
            assertEquals(cols[1], "testfloat");
            assertEquals(cols[2], "testchar");
            assertEquals(cols[3], "testvarchar");
            assertEquals(cols[4], "testclob");
            assertEquals(cols[5], "testblob");
            Console.WriteLine("");


            // cached result set with result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize((UInt64)2);
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl((UInt32)200);
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
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
            cur.setResultSetBufferSize((UInt64)0);
            Console.WriteLine("");


            // finished suspended session
            Console.WriteLine("FINISHED SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery("select * from testtable"));
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
            for (UInt32 i = 0; cur.getRow((UInt64)i) != null; i++) {
                assertTrue(secondcur.sendQuery("select * from testtable"));
            }
            secondcur.closeResultSet();
            cur.setResultSetBufferSize((UInt64)0);
            assertTrue(cur.sendQuery("drop table if exists testtable"));
            Console.WriteLine("");


            // reset transaction state
            Console.WriteLine("RESET TRANSACTION STATE: ");
            assertTrue(con.commit());
            assertEquals(con.getTransactionModel(), "explicit");
            assertTrue(con.getAutoCommit());
            Console.WriteLine("");


            // transaction behavior - implicit
            Console.WriteLine("TRANSACTION BEHAVIOR - implicit: ");
            assertTrue(con.setTransactionModel("implicit"));
            assertEquals(con.getTransactionModel(), "implicit");
            assertTrue(cur.sendQuery("create table testtable (col1 integer)"));
            // sqlite DDL is transactional; commit so the table is visible
            // to the second connection (the commit implicitly starts a new tx)
            assertTrue(con.commit());
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


            // reset transaction behavior
            Console.WriteLine("RESET TRANSACTION BEHAVIOR: ");
            assertTrue(con.setTransactionModel(con.getDefaultTransactionModel()));
            assertEquals(con.getTransactionModel(), "explicit");
            assertTrue(con.getAutoCommit());
            Console.WriteLine("");


            // individual substitutions
            Console.WriteLine("INDIVIDUAL SUBSTITUTIONS: ");
            cur.sendQuery("drop table if exists testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int, " +
                "	col2 char, " +
                "	col3 float)"));
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	$(var1), " +
                "	'$(var2)', " +
                "	$(var3))");
            cur.substitution("var1", (Int64)1);
            cur.substitution("var2", "hello");
            cur.substitution("var3", 10.5556, (UInt32)6, (UInt32)4);
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("select * from testtable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            assertTrue(cur.sendQuery("delete from testtable"));
            Console.WriteLine("");


            // array substitutions
            Console.WriteLine("ARRAY SUBSTITUTIONS: ");
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	'$(var1)', " +
                "	'$(var2)', " +
                "	'$(var3)')");
            cur.substitutions(subvars, subvalstrings);
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("select * from testtable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "hi");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "bye");
            assertTrue(cur.sendQuery("delete from testtable"));
            Console.WriteLine("");
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	$(var1), " +
                "	'$(var2)', " +
                "	$(var3))");
            cur.substitution(subvars, subvallongs);
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("select * from testtable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "3.0");
            assertTrue(cur.sendQuery("delete from testtable"));
            Console.WriteLine("");
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	$(var1), " +
                "	'$(var2)', " +
                "	$(var3))");
            cur.substitution(subvars, subvaldoubles, precs, scales);
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("select * from testtable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "10.55");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "10.556");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            assertTrue(cur.sendQuery("delete from testtable"));
            Console.WriteLine("");


            // nulls as nulls
            Console.WriteLine("NULLS AS NULLS: ");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	1, " +
                "	NULL, " +
                "	NULL)"));
            assertTrue(cur.sendQuery("select * from testtable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), (String)null);
            assertEquals(cur.getField((UInt64)0, (UInt32)2), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("select * from testtable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "");
            assertTrue(cur.sendQuery("drop table if exists testtable"));
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
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	:var1, " +
                "	:var2, " +
                "	:var3, " +
                "	:var4)");
            cur.inputBindClob("var1", "", (UInt32)0);
            cur.inputBindClob("var2", (String)null, (UInt32)0);
            cur.inputBindBlob("var3", System.Text.Encoding.ASCII.GetBytes(""), (UInt32)0);
            cur.inputBindBlob("var4", (Byte[])null, (UInt32)0);
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
                "create table testtable (" +
                "	testclob clob, " +
                "	testblob blob)");
            cur.prepareQuery("insert into testtable values (:clobval,:blobval)");
            cur.inputBindClob("clobval", largebuffer, (UInt32)LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("blobval", System.Text.Encoding.ASCII.GetBytes(largebuffer), (UInt32)LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "testclob"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testclob"), largebuffer);
            assertEquals(cur.getFieldLength((UInt64)0, "testblob"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testblob"), largebuffer,
                                LARGE_BUFFER_LENGTH);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // output bind by position
            // sqlite doesn't support output binds


            // output bind by name
            // sqlite doesn't support output binds


            // output bind by name with validation
            // sqlite doesn't support output binds


            // lob output bind
            // sqlite doesn't support output binds


            // long output bind
            // sqlite doesn't support output binds


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery("create table testtable (testval int)");
            cur.prepareQuery("insert into testtable values (:testval)");
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
            cur.substitution("var1", ":var1");
            assertTrue(cur.validBind("var1"));
            assertFalse(cur.validBind("var2"));
            assertFalse(cur.validBind("var3"));
            assertFalse(cur.validBind("var4"));
            Console.WriteLine("");
            cur.substitution("var2", ":var2");
            assertTrue(cur.validBind("var1"));
            assertTrue(cur.validBind("var2"));
            assertFalse(cur.validBind("var3"));
            assertFalse(cur.validBind("var4"));
            Console.WriteLine("");
            cur.substitution("var3", ":var3");
            assertTrue(cur.validBind("var1"));
            assertTrue(cur.validBind("var2"));
            assertTrue(cur.validBind("var3"));
            assertFalse(cur.validBind("var4"));
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // rebinding
            Console.WriteLine("REBINDING: ");
            cur.prepareQuery("select :val");
            cur.inputBind("val", (Int64)1);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            cur.inputBind("val", (Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "2");
            cur.inputBind("val", (Int64)3);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "3");
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
            cur.prepareQuery("select :var");
            cur.inputBind("var", (Int64)1);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.inputBind("var", (Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "2");
            Console.WriteLine("");


            // stored procedure returning no value
            // sqlite doesn't support stored procedures


            // stored procedure returning single value
            // sqlite doesn't support stored procedures


            // stored procedure returning multiple values
            // sqlite doesn't support stored procedures


            // stored procedure returning result set
            // sqlite doesn't support stored procedures


            // temporary tables
            Console.WriteLine("TEMPORARY TABLES: ");
            cur.sendQuery("drop table if exists temptable\n");
            cur.sendQuery("create temporary table temptable (col1 int)");
            assertTrue(cur.sendQuery("insert into temptable values (1)"));
            assertTrue(cur.sendQuery("select count(*) from temptable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            con.endSession();
            Console.WriteLine("");
            assertFalse(cur.sendQuery("select count(*) from temptable"));
            assertTrue(cur.sendQuery("drop table if exists temptable\n"));
            Console.WriteLine("");


            // encoded binary data
            Console.WriteLine("ENCODED BINARY DATA: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 blob)"));
            Byte[] buffer = new Byte[256];
            for (int i = 0; i < 256; i++) {
                buffer[i] = (Byte)i;
            }
            System.Text.StringBuilder querystr = new System.Text.StringBuilder();
            querystr.Append("insert into testtable values (X'");
            for (int i = 0; i < buffer.Length; i++) {
                querystr.Append(String.Format("{0:x2}", buffer[i]));
            }
            querystr.Append("')");
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
                    "	(col1 integer primary key " +
                    "	autoincrement, " +
                    "	col2 int)"));
            assertTrue(cur.sendQuery(
                    "insert into testtable values (null,1)"));
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
            Console.WriteLine("");


            // schema list
            Console.WriteLine("SCHEMA LIST: ");
            assertTrue(cur.getSchemaList((String)null));
            assertEquals(cur.getColumnName((UInt32)0), "Database");
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName((UInt32)0), "table_type");
            bool found = false;
            for (UInt64 i = 0; i < cur.rowCount(); i++) {
                if (cur.getField(i, "table_type") == "TABLE") {
                    found = true;
                    break;
                }
            }
            assertTrue(found);
            Console.WriteLine("");


            // table list
            Console.WriteLine("TABLE LIST: ");
            cur.sendQuery("drop table if exists testtable1");
            cur.sendQuery("drop table if exists testtable2");
            cur.sendQuery("drop table if exists testtable3");
            cur.sendQuery("drop table if exists testtable4");
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
            assertTrue(cur.getTableList((String)null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++) {
                String name = cur.getField(i, "Tables_in_xxx");
                if (name == "testtable1" ||
                    name == "testtable2" ||
                    name == "testtable3" ||
                    name == "testtable4") {
                    counter++;
                }
            }
            assertEquals(counter, (UInt64)4);
            assertTrue(cur.sendQuery("drop table if exists testtable1"));
            assertTrue(cur.sendQuery("drop table if exists testtable2"));
            assertTrue(cur.sendQuery("drop table if exists testtable3"));
            assertTrue(cur.sendQuery("drop table if exists testtable4"));
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
            assertEquals(cur.getField((UInt64)0, "precision"), "19");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "INTEGER");
            assertTrue(cur.getTypeInfoList("char"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "CHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "1");
            assertEquals(cur.getField((UInt64)0, "precision"), "2147483647");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "CHAR");
            assertTrue(cur.getTypeInfoList("varchar"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "VARCHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "12");
            assertEquals(cur.getField((UInt64)0, "precision"), "2147483647");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "VARCHAR");
            assertTrue(cur.getTypeInfoList("date"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "DATE");
            assertEquals(cur.getField((UInt64)0, "data_type"), "91");
            assertEquals(cur.getField((UInt64)0, "precision"), "10");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "DATE");
            Console.WriteLine("");


            // column list
            Console.WriteLine("COLUMN LIST: ");
            cur.sendQuery("drop table if exists testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testint int, " +
                "	testfloat float, " +
                "	testchar char(40), " +
                "	testvarchar varchar(40), " +
                "	testclob clob, " +
                "	testblob blob)"));
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
            assertEquals(cur.getField((UInt64)0, "column_name"), "testint");
            assertEquals(cur.getField((UInt64)1, "column_name"), "testfloat");
            assertEquals(cur.getField((UInt64)2, "column_name"), "testchar");
            assertEquals(cur.getField((UInt64)3, "column_name"), "testvarchar");
            assertEquals(cur.getField((UInt64)4, "column_name"), "testclob");
            assertEquals(cur.getField((UInt64)5, "column_name"), "testblob");
            assertEquals(cur.getField((UInt64)0, "data_type"), "INT");
            assertEquals(cur.getField((UInt64)1, "data_type"), "FLOAT");
            assertEquals(cur.getField((UInt64)2, "data_type"), "CHAR");
            assertEquals(cur.getField((UInt64)3, "data_type"), "VARCHAR");
            assertEquals(cur.getField((UInt64)4, "data_type"), "CLOB");
            assertEquals(cur.getField((UInt64)5, "data_type"), "BLOB");
            assertTrue(cur.sendQuery("drop table if exists testtable"));
            Console.WriteLine("");


            // column list - auto_increment, primary key
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            cur.sendQuery("drop table if exists testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 integer primary key autoincrement, " +
                "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", (String)null));
            assertTrue(cur.getField((UInt64)0, "extra").IndexOf("auto_increment") != -1);
            assertTrue(cur.getField((UInt64)0, "column_key").IndexOf("PRI") != -1);
            assertFalse(cur.getField((UInt64)1, "extra").IndexOf("auto_increment") != -1);
            assertFalse(cur.getField((UInt64)1, "column_key").IndexOf("PRI") != -1);
            Console.WriteLine("");
            assertTrue(cur.sendQuery("drop table if exists testtable"));
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int primary key, " +
                "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", (String)null));
            assertFalse(cur.getField((UInt64)0, "extra").IndexOf("auto_increment") != -1);
            assertTrue(cur.getField((UInt64)0, "column_key").IndexOf("PRI") != -1);
            assertTrue(cur.sendQuery("drop table if exists testtable"));
            Console.WriteLine("");


            // primary keys list
            Console.WriteLine("PRIMARY KEYS LIST: ");
            cur.sendQuery("drop table if exists testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int primary key, " +
                "	col2 int)"));
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
            assertTrue(cur.sendQuery("drop table if exists testtable"));
            Console.WriteLine("");


            // key and index list
            Console.WriteLine("KEY AND INDEX LIST: ");
            cur.sendQuery("drop table if exists testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 int primary key, " +
                "	col2 int)"));
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
            assertTrue(cur.sendQuery("drop table if exists testtable"));
            Console.WriteLine("");


            // procedure list
            Console.WriteLine("PROCEDURE LIST: ");
            assertTrue(cur.getProcedureList((String)null));
            assertEquals(cur.rowCount(), (UInt64)0);
            Console.WriteLine("");


            // procedure parameter list
            Console.WriteLine("PROCEDURE PARAMETER LIST: ");
            assertTrue(cur.getProcedureParameterList("testproc1", (String)null));
            assertEquals(cur.getColumnName((UInt32)0), "parameter_name");
            assertEquals(cur.getColumnName((UInt32)1), "parameter_mode");
            assertEquals(cur.getColumnName((UInt32)2), "data_type");
            assertEquals(cur.getColumnName((UInt32)3), "character_maximum_length");
            assertEquals(cur.getColumnName((UInt32)4), "ordinal_position");
            assertEquals(cur.rowCount(), (UInt64)0);
            Console.WriteLine("");


            // invalid queries
            Console.WriteLine("INVALID QUERIES: ");
            assertFalse(cur.sendQuery("select * from testtable"));
            assertFalse(cur.sendQuery("select * from testtable"));
            assertFalse(cur.sendQuery("select * from testtable"));
            assertFalse(cur.sendQuery("select * from testtable"));
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
