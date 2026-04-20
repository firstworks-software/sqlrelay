// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class KrbTest : SQLRTest
    {
        public static int Main(string[] args)
        {

            String[] isolationlevels = new String[] {
                        "READ COMMITTED","SERIALIZABLE" };
            String[] bindvars = new String[] { "1","2","3","4","5" };
            String[] bindvals = new String[] { "4","testchar4",
                        "testvarchar4","01-JAN-2004","testlong4" };
            String[] arraybindvars = new String[] { "var1","var2","var3",
                        "var4","var5" };
            String[] arraybindvals = new String[] { "7","testchar7",
                        "testvarchar7","01-JAN-2007","testlong7" };
            Int64 numvar;
            String stringvar;
            Double floatvar;
            Int16 year = 0;
            Int16 month = 0;
            Int16 day = 0;
            Int16 hour = 0;
            Int16 minute = 0;
            Int16 second = 0;
            Int32 microsecond = 0;
            String tz = null;
            String nullvar;
            Boolean isnegative = false;
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
            String clobvar;
            UInt32 clobvarlength;
            Byte[] blobvar;
            UInt32 blobvarlength;
            UInt16 counter;
            const int LARGE_BUFFER_LENGTH = 8192;
            char[] largebuffer = new char[LARGE_BUFFER_LENGTH];


            // hostname
            String hostname = System.Net.Dns.GetHostName();
            int dot = hostname.IndexOf('.');
            if (dot > 0) { hostname = hostname.Substring(0, dot); }


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                    "", "", 0, 1);
            cur = new SQLRCursor(con);
            con.enableKerberos(null, null, null);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "oracle");
            Console.WriteLine("");


            // ping
            Console.WriteLine("PING: ");
            assertTrue(con.ping());
            Console.WriteLine("");


            // bind format
            Console.WriteLine("BIND FORMAT: ");
            assertEquals(con.bindFormat(), ":*");
            Console.WriteLine("");


            // nextval format
            Console.WriteLine("NEXTVAL FORMAT: ");
            assertEquals(con.nextvalFormat(), "%s.nextval");
            Console.WriteLine("");


            // isolation levels
            Console.WriteLine("ISOLATION LEVELS: ");
            for (int i = 0; i < isolationlevels.Length; i++) {
                // oracle requires the isolation level to
                // be the first query of the transaction
                assertTrue(con.commit());
                // you can set the isolation level, but to get it, you have to
                // have permisisons to read from sys.v_$session and
                // sys.v_$transaction
                assertTrue(con.setIsolationLevel(isolationlevels[i]));
                Console.WriteLine("");
            }
            // reset to the default isolation level
            assertTrue(con.commit());
            assertTrue(con.setIsolationLevel(isolationlevels[0]));
            Console.WriteLine("");


            // create testtable
            Console.WriteLine("CREATE TESTTABLE: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testnumber number, " +
                "	testchar char(40), " +
                "	testvarchar varchar2(40), " +
                "	testdate date, " +
                "	testlong long, " +
                "	testclob clob, " +
                "	testblob blob)"));
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	1, " +
                "	'testchar1', " +
                "	'testvarchar1', " +
                "	'01-JAN-2001', " +
                "	'testlong1', " +
                "	'testclob1', " +
                "	empty_blob())"));
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
                "	:var1, " +
                "	:var2, " +
                "	:var3, " +
                "	:var4, " +
                "	:var5, " +
                "	:var6, " +
                "	:var7)");
            assertEquals(cur.countBindVariables(), (UInt16)7);
            cur.inputBind("1", 2);
            cur.inputBind("2", "testchar2");
            cur.inputBind("3", "testvarchar2");
            cur.inputBind("4", 2002, 1, 1, 0, 0, 0, 0, null, false);
            cur.inputBind("5", "testlong2");
            cur.inputBindClob("6", "testclob2", 9);
            cur.inputBindBlob("7", System.Text.Encoding.ASCII.GetBytes("testblob2"), 9);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", 3);
            cur.inputBind("2", "testchar3");
            cur.inputBind("3", "testvarchar3");
            cur.inputBind("4", 2003, 1, 1, 0, 0, 0, 0, null, false);
            cur.inputBind("5", "testlong3");
            cur.inputBindClob("6", "testclob3", 9);
            cur.inputBindBlob("7", System.Text.Encoding.ASCII.GetBytes("testblob3"), 9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            Console.WriteLine("ARRAY OF INPUT BINDS BY POSITION: ");
            cur.clearBinds();
            cur.inputBind(bindvars, bindvals);
            cur.inputBindClob("6", "testclob4", 9);
            cur.inputBindBlob("7", System.Text.Encoding.ASCII.GetBytes("testblob4"), 9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by position with validation
            Console.WriteLine("INPUT BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1", 5);
            cur.inputBind("2", "testchar5");
            cur.inputBind("3", "testvarchar5");
            cur.inputBind("4", 2005, 1, 1, 0, 0, 0, 0, null, false);
            cur.inputBind("5", "testlong5");
            cur.inputBindClob("6", "testclob5", 9);
            cur.inputBindBlob("7", System.Text.Encoding.ASCII.GetBytes("testblob5"), 9);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            cur.clearBinds();


            // input bind by name
            Console.WriteLine("INPUT BIND BY NAME: ");
            cur.clearBinds();
            cur.inputBind("var1", 6);
            cur.inputBind("var2", "testchar6");
            cur.inputBind("var3", "testvarchar6");
            cur.inputBind("var4", 2006, 1, 1, 0, 0, 0, 0, null, false);
            cur.inputBind("var5", "testlong6");
            cur.inputBindClob("var6", "testclob6", 9);
            cur.inputBindBlob("var7", System.Text.Encoding.ASCII.GetBytes("testblob6"), 9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by name
            Console.WriteLine("ARRAY OF INPUT BINDS BY NAME: ");
            cur.clearBinds();
            cur.inputBind(arraybindvars, arraybindvals);
            cur.inputBindClob("var6", "testclob7", 9);
            cur.inputBindBlob("var7", System.Text.Encoding.ASCII.GetBytes("testblob7"), 9);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // input bind by name with validation
            Console.WriteLine("INPUT BIND BY NAME WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("var1", 8);
            cur.inputBind("var2", "testchar8");
            cur.inputBind("var3", "testvarchar8");
            cur.inputBind("var4", 2008, 1, 1, 0, 0, 0, 0, null, false);
            cur.inputBind("var5", "testlong8");
            cur.inputBindClob("var6", "testclob8", 9);
            cur.inputBindBlob("var7", System.Text.Encoding.ASCII.GetBytes("testblob8"), 9);
            cur.inputBind("var9", "junkvalue");
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(), (UInt32)7);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName(0), "TESTNUMBER");
            assertEquals(cur.getColumnName(1), "TESTCHAR");
            assertEquals(cur.getColumnName(2), "TESTVARCHAR");
            assertEquals(cur.getColumnName(3), "TESTDATE");
            assertEquals(cur.getColumnName(4), "TESTLONG");
            assertEquals(cur.getColumnName(5), "TESTCLOB");
            assertEquals(cur.getColumnName(6), "TESTBLOB");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "TESTNUMBER");
            assertEquals(cols[1], "TESTCHAR");
            assertEquals(cols[2], "TESTVARCHAR");
            assertEquals(cols[3], "TESTDATE");
            assertEquals(cols[4], "TESTLONG");
            assertEquals(cols[5], "TESTCLOB");
            assertEquals(cols[6], "TESTBLOB");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            assertEquals(cur.getColumnType((UInt32)0), "NUMBER");
            assertEquals(cur.getColumnType("TESTNUMBER"), "NUMBER");
            assertEquals(cur.getColumnType((UInt32)1), "CHAR");
            assertEquals(cur.getColumnType("TESTCHAR"), "CHAR");
            assertEquals(cur.getColumnType((UInt32)2), "VARCHAR2");
            assertEquals(cur.getColumnType("TESTVARCHAR"), "VARCHAR2");
            assertEquals(cur.getColumnType((UInt32)3), "DATE");
            assertEquals(cur.getColumnType("TESTDATE"), "DATE");
            assertEquals(cur.getColumnType((UInt32)4), "LONG");
            assertEquals(cur.getColumnType("TESTLONG"), "LONG");
            assertEquals(cur.getColumnType((UInt32)5), "CLOB");
            assertEquals(cur.getColumnType("TESTCLOB"), "CLOB");
            assertEquals(cur.getColumnType((UInt32)6), "BLOB");
            assertEquals(cur.getColumnType("TESTBLOB"), "BLOB");
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)22);
            assertEquals(cur.getColumnLength("TESTNUMBER"), (UInt32)22);
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)40);
            assertEquals(cur.getColumnLength("TESTCHAR"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)2), (UInt32)40);
            assertEquals(cur.getColumnLength("TESTVARCHAR"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)3), (UInt32)7);
            assertEquals(cur.getColumnLength("TESTDATE"), (UInt32)7);
            assertEquals(cur.getColumnLength((UInt32)4), (UInt32)0);
            assertEquals(cur.getColumnLength("TESTLONG"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)5), (UInt32)0);
            assertEquals(cur.getColumnLength("TESTCLOB"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)6), (UInt32)0);
            assertEquals(cur.getColumnLength("TESTBLOB"), (UInt32)0);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest("TESTNUMBER"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)1), (UInt32)40);
            assertEquals(cur.getLongest("TESTCHAR"), (UInt32)40);
            assertEquals(cur.getLongest((UInt32)2), (UInt32)12);
            assertEquals(cur.getLongest("TESTVARCHAR"), (UInt32)12);
            assertEquals(cur.getLongest((UInt32)3), (UInt32)9);
            assertEquals(cur.getLongest("TESTDATE"), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)4), (UInt32)9);
            assertEquals(cur.getLongest("TESTLONG"), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)5), (UInt32)9);
            assertEquals(cur.getLongest("TESTCLOB"), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)6), (UInt32)9);
            assertEquals(cur.getLongest("TESTBLOB"), (UInt32)9);
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
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), "01-JAN-01");
            assertEquals(cur.getField((UInt64)0, (UInt32)4), "testlong1");
            assertEquals(cur.getField((UInt64)0, (UInt32)5), "testclob1");
            assertEquals(cur.getField((UInt64)0, (UInt32)6), "");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)3), "01-JAN-08");
            assertEquals(cur.getField((UInt64)7, (UInt32)4), "testlong8");
            assertEquals(cur.getField((UInt64)7, (UInt32)5), "testclob8");
            assertEquals(cur.getField((UInt64)7, (UInt32)6), "testblob8");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)4), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)5), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)6), (UInt32)0);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)4), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)5), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)6), (UInt32)9);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "TESTNUMBER"), "1");
            assertEquals(cur.getField((UInt64)0, "TESTCHAR"), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, "TESTVARCHAR"), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, "TESTDATE"), "01-JAN-01");
            assertEquals(cur.getField((UInt64)0, "TESTLONG"), "testlong1");
            assertEquals(cur.getField((UInt64)0, "TESTCLOB"), "testclob1");
            assertEquals(cur.getField((UInt64)0, "TESTBLOB"), "");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "TESTNUMBER"), "8");
            assertEquals(cur.getField((UInt64)7, "TESTCHAR"), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, "TESTVARCHAR"), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, "TESTDATE"), "01-JAN-08");
            assertEquals(cur.getField((UInt64)7, "TESTLONG"), "testlong8");
            assertEquals(cur.getField((UInt64)7, "TESTCLOB"), "testclob8");
            assertEquals(cur.getField((UInt64)7, "TESTBLOB"), "testblob8");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "TESTNUMBER"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTCHAR"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTVARCHAR"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTDATE"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTLONG"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTCLOB"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTBLOB"), (UInt32)0);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "TESTNUMBER"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTCHAR"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTVARCHAR"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTDATE"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTLONG"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTCLOB"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "TESTBLOB"), (UInt32)9);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields = cur.getRow((UInt64)0);
            assertEquals(fields[0], "1");
            assertEquals(fields[1], "testchar1                               ");
            assertEquals(fields[2], "testvarchar1");
            assertEquals(fields[3], "01-JAN-01");
            assertEquals(fields[4], "testlong1");
            assertEquals(fields[5], "testclob1");
            assertEquals(fields[6], "");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens = cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0], (UInt32)1);
            assertEquals(fieldlens[1], (UInt32)40);
            assertEquals(fieldlens[2], (UInt32)12);
            assertEquals(fieldlens[3], (UInt32)9);
            assertEquals(fieldlens[4], (UInt32)9);
            assertEquals(fieldlens[5], (UInt32)9);
            assertEquals(fieldlens[6], (UInt32)0);
            Console.WriteLine("");


            // result set buffer size
            Console.WriteLine("RESULT SET BUFFER SIZE: ");
            assertEquals(cur.getResultSetBufferSize(), (UInt64)0);
            cur.setResultSetBufferSize(2);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            assertEquals(cur.getResultSetBufferSize(), (UInt64)2);
            Console.WriteLine("");
            assertEquals(cur.firstRowIndex(), (UInt64)0);
            assertFalse(cur.endOfResultSet());
            assertEquals(cur.rowCount(), (UInt64)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)1, (UInt32)0), "2");
            assertEquals(cur.getField((UInt64)2, (UInt32)0), "3");
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
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            assertEquals(cur.getColumnName(0), (String)null);
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0), (String)null);
            cur.getColumnInfo();
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            assertEquals(cur.getColumnName(0), "TESTNUMBER");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)22);
            assertEquals(cur.getColumnType((UInt32)0), "NUMBER");
            Console.WriteLine("");


            // suspended session
            Console.WriteLine("SUSPENDED SESSION: ");
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
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
                "	testnumber"));
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
                "	testnumber"));
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
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
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
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            filename = cur.getCacheFileName();
            assertEquals(filename, "cachefile1");
            cur.cacheOff();
            assertTrue(cur.openCachedResultSet(filename));
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            Console.WriteLine("");


            // column count for cached result set
            Console.WriteLine("COLUMN COUNT FOR CACHED RESULT SET: ");
            assertEquals(cur.colCount(), (UInt32)7);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName(0), "TESTNUMBER");
            assertEquals(cur.getColumnName(1), "TESTCHAR");
            assertEquals(cur.getColumnName(2), "TESTVARCHAR");
            assertEquals(cur.getColumnName(3), "TESTDATE");
            assertEquals(cur.getColumnName(4), "TESTLONG");
            assertEquals(cur.getColumnName(5), "TESTCLOB");
            assertEquals(cur.getColumnName(6), "TESTBLOB");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "TESTNUMBER");
            assertEquals(cols[1], "TESTCHAR");
            assertEquals(cols[2], "TESTVARCHAR");
            assertEquals(cols[3], "TESTDATE");
            assertEquals(cols[4], "TESTLONG");
            assertEquals(cols[5], "TESTCLOB");
            assertEquals(cols[6], "TESTBLOB");
            Console.WriteLine("");


            // cached result set with result set buffer size
            Console.WriteLine("CACHED RESULT SET WITH RESULT SET BUFFER SIZE: ");
            cur.setResultSetBufferSize(2);
            cur.cacheToFile("cachefile1");
            cur.setCacheTtl(200);
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
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
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
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
            assertTrue(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
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
            for (UInt32 i = 0; cur.getRow((UInt64)i) != null; i++) {
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
                                    "", "", 0, 1);
            secondcur = new SQLRCursor(secondcon);
            secondcon.enableKerberos(null, null, null);
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "0");
            assertTrue(con.commit());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "8");
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	10, " +
                "	'testchar10', " +
                "	'testvarchar10', " +
                "	'01-JAN-2010', " +
                "	'testlong10', " +
                "	'testclob10', " +
                "	NULL)"));
            assertTrue(con.rollback());
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "8");
            assertTrue(con.autoCommitOn());
            assertTrue(cur.sendQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	10, " +
                "	'testchar10', " +
                "	'testvarchar10', " +
                "	'01-JAN-2010', " +
                "	'testlong10', " +
                "	'testclob10', " +
                "	NULL)"));
            assertTrue(secondcur.sendQuery("select count(*) from testtable"));
            assertEquals(secondcur.getField((UInt64)0, (UInt32)0), "9");
            secondcon.endSession();
            assertTrue(con.autoCommitOff());
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // individual substitutions
            Console.WriteLine("INDIVIDUAL SUBSTITUTIONS: ");
            cur.prepareQuery("select $(var1),'$(var2)',$(var3) from dual");
            cur.substitution("var1", "$(var11)");
            cur.substitution("var2", "$(var21)");
            cur.substitution("var3", "$(var31)");
            cur.substitution("var11", "$(var111)");
            cur.substitution("var21", "$(var211)");
            cur.substitution("var31", "$(var311)");
            cur.substitution("var111", 1);
            cur.substitution("var211", "hello");
            cur.substitution("var311", 10.5556, 6, 4);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            Console.WriteLine("");


            // array substitutions
            Console.WriteLine("ARRAY SUBSTITUTIONS: ");
            cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
            cur.substitution(subvars, subvallongs);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "2");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "3");
            Console.WriteLine("");
            cur.prepareQuery("select '$(var1)','$(var2)','$(var3)' from dual");
            cur.substitutions(subvars, subvalstrings);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "hi");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "hello");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "bye");
            Console.WriteLine("");
            cur.prepareQuery("select $(var1),$(var2),$(var3) from dual");
            cur.substitution(subvars, subvaldoubles, precs, scales);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "10.55");
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "10.556");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "10.5556");
            Console.WriteLine("");


            // nulls as nulls
            Console.WriteLine("NULLS AS NULLS: ");
            cur.getNullsAsNulls();
            assertTrue(cur.sendQuery("select NULL,1,NULL from dual"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), (String)null);
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), (String)null);
            cur.getNullsAsEmptyStrings();
            assertTrue(cur.sendQuery("select NULL,1,NULL from dual"));
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
            cur.prepareQuery(
                "insert into " +
                "	testtable " +
                "values (" +
                "	:var1, " +
                "	:var2, " +
                "	:var3, " +
                "	:var4)");
            cur.inputBindClob("var1", "", 0);
            cur.inputBindClob("var2", null, 0);
            cur.inputBindBlob("var3", System.Text.Encoding.ASCII.GetBytes(""), 0);
            cur.inputBindBlob("var4", null, 0);
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
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++) {
                largebuffer[i] = 'C';
            }
            String largebufferstr = new String(largebuffer);
            Byte[] largebufferbytes = System.Text.Encoding.ASCII.GetBytes(largebufferstr);
            cur.inputBindClob("clobval", largebufferstr, LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("blobval", largebufferbytes, LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "TESTCLOB"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "TESTCLOB"), largebufferstr);
            assertEquals(cur.getFieldLength((UInt64)0, "TESTBLOB"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, "TESTBLOB"), largebufferbytes,
                                LARGE_BUFFER_LENGTH);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // output bind by position
            Console.WriteLine("OUTPUT BIND BY POSITION: ");
            cur.getNullsAsNulls();
            cur.prepareQuery(
                "begin " +
                "	:numvar:=1; " +
                "	:stringvar:='hello'; " +
                "	:floatvar:=2.5; " +
                "	:datevar:='03-FEB-2001'; " +
                "	:nullvar:=null; " +
                "end;");
            assertEquals(cur.countBindVariables(), (UInt16)5);
            cur.defineOutputBindInteger("1");
            cur.defineOutputBindString("2", 10);
            cur.defineOutputBindDouble("3");
            cur.defineOutputBindDate("4");
            cur.defineOutputBindString("5", 10);
            assertTrue(cur.executeQuery());
            numvar = cur.getOutputBindInteger("1");
            stringvar = cur.getOutputBindString("2");
            floatvar = cur.getOutputBindDouble("3");
            cur.getOutputBindDate("4", out year, out month, out day,
                        out hour, out minute, out second, out microsecond, out tz,
                        out isnegative);
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
            Console.WriteLine("");


            // output bind by name
            Console.WriteLine("OUTPUT BIND BY NAME: ");
            cur.getNullsAsNulls();
            cur.clearBinds();
            cur.defineOutputBindInteger("numvar");
            cur.defineOutputBindString("stringvar", 10);
            cur.defineOutputBindDouble("floatvar");
            cur.defineOutputBindDate("datevar");
            cur.defineOutputBindString("nullvar", 10);
            assertTrue(cur.executeQuery());
            numvar = cur.getOutputBindInteger("numvar");
            stringvar = cur.getOutputBindString("stringvar");
            floatvar = cur.getOutputBindDouble("floatvar");
            cur.getOutputBindDate("datevar", out year, out month, out day,
                        out hour, out minute, out second, out microsecond, out tz,
                        out isnegative);
            nullvar = cur.getOutputBindString("nullvar");
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
            Console.WriteLine("");


            // output bind by name with validation
            Console.WriteLine("OUTPUT BIND BY NAME WITH VALIDATION: ");
            cur.getNullsAsNulls();
            cur.clearBinds();
            cur.defineOutputBindInteger("numvar");
            cur.defineOutputBindString("stringvar", 10);
            cur.defineOutputBindDouble("floatvar");
            cur.defineOutputBindDate("datevar");
            cur.defineOutputBindString("nullvar", 10);
            cur.defineOutputBindString("dummyvar", 10);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            numvar = cur.getOutputBindInteger("numvar");
            stringvar = cur.getOutputBindString("stringvar");
            floatvar = cur.getOutputBindDouble("floatvar");
            cur.getOutputBindDate("datevar", out year, out month, out day,
                        out hour, out minute, out second, out microsecond, out tz,
                        out isnegative);
            nullvar = cur.getOutputBindString("nullvar");
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
            Console.WriteLine("");


            // lob output bind
            Console.WriteLine("LOB OUTPUT BIND: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testclob clob, " +
                "	testblob blob)"));
            cur.prepareQuery("insert into testtable values ('hello',:var1)");
            cur.inputBindBlob("var1", System.Text.Encoding.ASCII.GetBytes("hello"), 5);
            assertTrue(cur.executeQuery());
            cur.prepareQuery(
                "begin " +
                "	select testclob into :clobvar from testtable; " +
                "	select testblob into :blobvar from testtable; " +
                "end;");
            cur.defineOutputBindClob("clobvar");
            cur.defineOutputBindBlob("blobvar");
            assertTrue(cur.executeQuery());
            clobvar = cur.getOutputBindClob("clobvar");
            clobvarlength = cur.getOutputBindLength("clobvar");
            blobvar = cur.getOutputBindBlob("blobvar");
            blobvarlength = cur.getOutputBindLength("blobvar");
            assertEquals(clobvar, "hello", 5);
            assertEquals(clobvarlength, (UInt32)5);
            assertEquals(blobvar, System.Text.Encoding.ASCII.GetBytes("hello"), 5);
            assertEquals(blobvarlength, (UInt32)5);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // long output bind
            Console.WriteLine("LONG OUTPUT BIND: ");
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++) {
                largebuffer[i] = 'C';
            }
            String largebufferstr2 = new String(largebuffer);
            String query = "begin :bindval:='" + largebufferstr2 + "'; end;";
            cur.prepareQuery(query);
            cur.defineOutputBindString("bindval", LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindLength("bindval"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getOutputBindString("bindval"), largebufferstr2);
            Console.WriteLine("");


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery("create table testtable (testval number)");
            cur.prepareQuery("insert into testtable values (:testval)");
            cur.inputBind("testval", -1);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testval from testtable");
            assertEquals(cur.getField((UInt64)0, "TESTVAL"), "-1");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // bind validation
            Console.WriteLine("BIND VALIDATION: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery(
                "create table testtable (" +
                "	col1 varchar2(20), " +
                "	col2 varchar2(20), " +
                "	col3 varchar2(20))");
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
            cur.prepareQuery(
                "begin " +
                "	:out:= :in; " +
                "end;");
            cur.inputBind("in", 1);
            cur.defineOutputBindInteger("out");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out"), (Int64)1);
            cur.inputBind("in", 2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out"), (Int64)2);
            cur.inputBind("in", 3);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out"), (Int64)3);
            Console.WriteLine("");


            // reexecute
            Console.WriteLine("REEXECUTE: ");
            cur.prepareQuery("select 1 from dual");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.prepareQuery("select :var from dual");
            cur.inputBind("var", 1);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            Console.WriteLine("");
            cur.inputBind("var", 2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.rowCount(), (UInt64)1);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "2");
            Console.WriteLine("");


            // stored procedure returning no value
            Console.WriteLine("STORED PROCEDURE RETURNING NO VALUE: ");
            cur.sendQuery("drop function testproc");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create or replace " +
                "procedure testproc(" +
                "	in1 in number, " +
                "	in2 in number, " +
                "	in3 in varchar2) " +
                "is " +
                "begin " +
                "	return; " +
                "end;"));
            cur.prepareQuery("begin testproc(:in1,:in2,:in3); end;");
            cur.inputBind("in1", 1);
            cur.inputBind("in2", 1.1, 2, 1);
            cur.inputBind("in3", "hello");
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("drop procedure testproc"));
            Console.WriteLine("");


            // stored procedure returning single value
            Console.WriteLine("STORED PROCEDURE RETURNING SINGLE VALUE: ");
            cur.sendQuery("drop function testproc");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create or replace " +
                "function testproc(" +
                "	in1 in number, " +
                "	in2 in number, " +
                "	in3 in varchar2) " +
                "	return number " +
                "is " +
                "begin " +
                "	return in1; " +
                "end;"));
            cur.prepareQuery("select testproc(:in1,:in2,:in3) from dual");
            cur.inputBind("in1", 1);
            cur.inputBind("in2", 1.1, 2, 1);
            cur.inputBind("in3", "hello");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            cur.prepareQuery(
                "begin " +
                "	:out1:=testproc(:in1,:in2,:in3); " +
                "end;");
            cur.inputBind("in1", 1);
            cur.inputBind("in2", 1.1, 2, 1);
            cur.inputBind("in3", "hello");
            cur.defineOutputBindInteger("out1");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getOutputBindInteger("out1"), (Int64)1);
            assertTrue(cur.sendQuery("drop function testproc"));
            Console.WriteLine("");


            // stored procedure returning multiple values
            Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
            cur.sendQuery("drop function testproc");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create or replace " +
                "procedure testproc(" +
                "	in1 in number, " +
                "	in2 in number, " +
                "	in3 in varchar2, " +
                "	out1 out number, " +
                "	out2 out number, " +
                "	out3 out varchar2) " +
                "is " +
                "begin " +
                "	out1:=in1; " +
                "	out2:=in2; " +
                "	out3:=in3; " +
                "end;"));
            cur.prepareQuery(
                "begin " +
                "	testproc(:in1,:in2,:in3,:out1,:out2,:out3); " +
                "end;");
            cur.inputBind("in1", 1);
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
            cur.sendQuery("drop package types");
            cur.sendQuery("drop function testproc");
            cur.sendQuery("drop procedure testproc");
            assertTrue(cur.sendQuery(
                "create or replace package types is " +
                "	type cursorType is ref cursor; " +
                "end;"));
            assertTrue(cur.sendQuery(
                "create or replace " +
                "function testproc(value in number) " +
                "	return types.cursortype " +
                "is " +
                "	l_cursor    types.cursorType; " +
                "begin " +
                "	open l_cursor for " +
                "		select " +
                "			* " +
                "		from " +
                "			( " +
                "			select 1 as testnumber from dual " +
                "			union " +
                "			select 2 as testnumber from dual " +
                "			union " +
                "			select 3 as testnumber from dual " +
                "			union " +
                "			select 4 as testnumber from dual " +
                "			union " +
                "			select 5 as testnumber from dual " +
                "			union " +
                "			select 6 as testnumber from dual " +
                "			union " +
                "			select 7 as testnumber from dual " +
                "			union " +
                "			select 8 as testnumber from dual " +
                "			) " +
                "		where " +
                "			testnumber>value; " +
                "	return l_cursor; " +
                "end;"));
            cur.prepareQuery(
                "begin " +
                "	:curs1:=testproc(5); " +
                "	:curs2:=testproc(0); " +
                "end;");
            cur.defineOutputBindCursor("curs1");
            cur.defineOutputBindCursor("curs2");
            assertTrue(cur.executeQuery());
            SQLRCursor bindcur1 = cur.getOutputBindCursor("curs1");
            assertTrue(bindcur1.fetchFromBindCursor());
            assertEquals(bindcur1.getField((UInt64)0, (UInt32)0), "6");
            assertEquals(bindcur1.getField((UInt64)1, (UInt32)0), "7");
            assertEquals(bindcur1.getField((UInt64)2, (UInt32)0), "8");
            SQLRCursor bindcur2 = cur.getOutputBindCursor("curs2");
            assertTrue(bindcur2.fetchFromBindCursor());
            assertEquals(bindcur2.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(bindcur2.getField((UInt64)1, (UInt32)0), "2");
            assertEquals(bindcur2.getField((UInt64)2, (UInt32)0), "3");
            assertTrue(cur.sendQuery("drop function testproc"));
            assertTrue(cur.sendQuery("drop package types"));
            Console.WriteLine("");


            // temporary tables
            Console.WriteLine("TEMPORARY TABLES: ");
            cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete");
            cur.substitution("HOSTNAME", hostname);
            cur.executeQuery();
            cur.prepareQuery(
                "create global temporary table $(HOSTNAME)_temptabledelete ( " +
                "	col1 number " +
                ") on commit delete rows");
            cur.substitution("HOSTNAME", hostname);
            cur.executeQuery();
            cur.prepareQuery("insert into $(HOSTNAME)_temptabledelete values (1)");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            cur.prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertTrue(con.commit());
            cur.prepareQuery("select count(*) from $(HOSTNAME)_temptabledelete");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "0");
            cur.prepareQuery("drop table $(HOSTNAME)_temptabledelete");
            cur.substitution("HOSTNAME", hostname);
            cur.executeQuery();
            Console.WriteLine("");
            cur.prepareQuery("truncate table $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            cur.executeQuery();
            cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            cur.executeQuery();
            cur.prepareQuery(
                "create global temporary table $(HOSTNAME)_temptablepreserve (" +
                "	col1 number " +
                ") on commit preserve rows");
            cur.substitution("HOSTNAME", hostname);
            cur.executeQuery();
            cur.prepareQuery(
                "insert into " +
                "	$(HOSTNAME)_temptablepreserve " +
                "values (1)");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertTrue(con.commit());
            cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            con.endSession();
            Console.WriteLine("");
            cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "0");
            cur.prepareQuery("truncate table $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            System.Threading.Thread.Sleep(2 * 1000);
            cur.prepareQuery("drop table $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            assertTrue(cur.executeQuery());
            cur.prepareQuery("select count(*) from $(HOSTNAME)_temptablepreserve");
            cur.substitution("HOSTNAME", hostname);
            assertFalse(cur.executeQuery());
            Console.WriteLine("");


            // encoded binary data
            Console.WriteLine("ENCODED BINARY DATA: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 blob)"));
            Byte[] buffer = new Byte[256];
            for (UInt16 i = 0; i < 256; i++) {
                buffer[i] = (Byte)i;
            }
            System.Text.StringBuilder querystr = new System.Text.StringBuilder();
            querystr.Append("insert into testtable values ('");
            for (UInt64 i = 0; i < (UInt64)buffer.Length; i++) {
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
            assertTrue(cur.sendQuery("create table testtable (col1 varchar2(4))"));
            assertTrue(cur.sendQuery("insert into testtable values ('''''')"));
            assertTrue(cur.sendQuery("select col1 from testtable"));
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)2);
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "''");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // last insert id
            // oracle doesn't support auto-increment


            // database is schema
            Console.WriteLine("DATABASE IS SCHEMA: ");
            assertTrue(con.getDatabaseIsSchema());
            Console.WriteLine("");


            // catalog list
            Console.WriteLine("CATALOG LIST: ");
            assertTrue(cur.getCatalogList(null));
            assertEquals(cur.getColumnName(0), "Database");
            assertEquals(cur.rowCount(), (UInt64)0);
            Console.WriteLine("");


            // schema list
            Console.WriteLine("SCHEMA LIST: ");
            assertTrue(cur.getSchemaList(null));
            assertEquals(cur.getColumnName(0), "Database");
            Boolean found = false;
            for (UInt64 i = 0; i < cur.rowCount(); i++) {
                if (String.Equals(cur.getField(i, "Database"), hostname,
                        StringComparison.OrdinalIgnoreCase)) {
                    found = true;
                    break;
                }
            }
            assertTrue(found);
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName(0), "table_type");
            assertEquals(cur.getField((UInt64)0, "table_type"), "SYNONYM");
            assertEquals(cur.getField((UInt64)1, "table_type"), "TABLE");
            assertEquals(cur.getField((UInt64)2, "table_type"), "VIEW");
            Console.WriteLine("");


            // table list
            Console.WriteLine("TABLE LIST: ");
            cur.sendQuery("drop table testtable1");
            cur.sendQuery("drop table testtable2");
            cur.sendQuery("drop table testtable3");
            cur.sendQuery("drop table testtable4");
            assertTrue(cur.sendQuery(
                "create table testtable1 (" +
                "	testnumber number, " +
                "	testchar char(40), " +
                "	testvarchar varchar2(40), " +
                "	testdate date, " +
                "	testlong long, " +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(cur.sendQuery(
                "create table testtable2 (" +
                "	testnumber number, " +
                "	testchar char(40), " +
                "	testvarchar varchar2(40), " +
                "	testdate date, " +
                "	testlong long, " +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(cur.sendQuery(
                "create table testtable3 (" +
                "	testnumber number, " +
                "	testchar char(40), " +
                "	testvarchar varchar2(40), " +
                "	testdate date, " +
                "	testlong long, " +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(cur.sendQuery(
                "create table testtable4 (" +
                "	testnumber number, " +
                "	testchar char(40), " +
                "	testvarchar varchar2(40), " +
                "	testdate date, " +
                "	testlong long, " +
                "	testclob clob, " +
                "	testblob blob)"));
            assertTrue(cur.getTableList(null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++) {
                String name = cur.getField(i, "Tables_in_xxx");
                if (name == "TESTTABLE1" ||
                    name == "TESTTABLE2" ||
                    name == "TESTTABLE3" ||
                    name == "TESTTABLE4") {
                    counter++;
                }
            }
            assertEquals(counter, (UInt16)4);
            assertTrue(cur.sendQuery("drop table testtable1"));
            assertTrue(cur.sendQuery("drop table testtable2"));
            assertTrue(cur.sendQuery("drop table testtable3"));
            assertTrue(cur.sendQuery("drop table testtable4"));
            Console.WriteLine("");


            // type info list
            Console.WriteLine("TYPE INFO LIST: ");
            assertTrue(cur.getTypeInfoList("number"));
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
            assertEquals(cur.getField((UInt64)0, "type_name"), "NUMBER");
            assertEquals(cur.getField((UInt64)0, "data_type"), "-7");
            assertEquals(cur.getField((UInt64)0, "precision"), "1");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "NUMBER");
            assertTrue(cur.getTypeInfoList("char"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "CHAR");
            assertEquals(cur.getField((UInt64)0, "data_type"), "1");
            assertEquals(cur.getField((UInt64)0, "precision"), "2000");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "CHAR");
            assertTrue(cur.getTypeInfoList("varchar2"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "VARCHAR2");
            assertEquals(cur.getField((UInt64)0, "data_type"), "12");
            assertEquals(cur.getField((UInt64)0, "precision"), "32767");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "VARCHAR2");
            assertTrue(cur.getTypeInfoList("date"));
            assertEquals(cur.getField((UInt64)0, "type_name"), "DATE");
            assertEquals(cur.getField((UInt64)0, "data_type"), "92");
            assertEquals(cur.getField((UInt64)0, "precision"), "7");
            assertEquals(cur.getField((UInt64)0, "local_type_name"), "DATE");
            Console.WriteLine("");


            // column list
            Console.WriteLine("COLUMN LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	testnumber number, " +
                "	testchar char(40), " +
                "	testvarchar varchar2(40), " +
                "	testdate date, " +
                "	testlong long, " +
                "	testclob clob, " +
                "	testblob blob)"));
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
            assertEquals(cur.getField((UInt64)0, "column_name"), "TESTNUMBER");
            assertEquals(cur.getField((UInt64)1, "column_name"), "TESTCHAR");
            assertEquals(cur.getField((UInt64)2, "column_name"), "TESTVARCHAR");
            assertEquals(cur.getField((UInt64)3, "column_name"), "TESTDATE");
            assertEquals(cur.getField((UInt64)4, "column_name"), "TESTLONG");
            assertEquals(cur.getField((UInt64)5, "column_name"), "TESTCLOB");
            assertEquals(cur.getField((UInt64)6, "column_name"), "TESTBLOB");
            assertEquals(cur.getField((UInt64)0, "data_type"), "NUMBER");
            assertEquals(cur.getField((UInt64)1, "data_type"), "CHAR");
            assertEquals(cur.getField((UInt64)2, "data_type"), "VARCHAR2");
            assertEquals(cur.getField((UInt64)3, "data_type"), "DATE");
            assertEquals(cur.getField((UInt64)4, "data_type"), "LONG");
            assertEquals(cur.getField((UInt64)5, "data_type"), "CLOB");
            assertEquals(cur.getField((UInt64)6, "data_type"), "BLOB");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // column list - auto_increment, primary key
            // oracle doesn't support auto_increment
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 number primary key, " +
                "	col2 number)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertTrue(cur.getField((UInt64)0, "column_key") != null && cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertFalse(cur.getField((UInt64)1, "column_key") != null && cur.getField((UInt64)1, "column_key").Contains("PRI"));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // primary keys list
            Console.WriteLine("PRIMARY KEYS LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 number primary key, " +
                "	col2 number)"));
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
            assertEquals(cur.getField((UInt64)0, "table"), "TESTTABLE");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertEquals(cur.getField((UInt64)0, "column_name"), "COL1");
            assertTrue(!String.IsNullOrEmpty(cur.getField((UInt64)0, "key_name")));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // key and index list
            Console.WriteLine("KEY AND INDEX LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable (" +
                "	col1 number primary key, " +
                "	col2 number)"));
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
            assertEquals(cur.getField((UInt64)0, "table"), "TESTTABLE");
            assertEquals(cur.getField((UInt64)0, "non_unique"), "0");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertEquals(cur.getField((UInt64)0, "column_name"), "COL1");
            assertEquals(cur.getField((UInt64)0, "collation"), "A");
            assertEquals(cur.getField((UInt64)0, "index_type"), "3");
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
                "create procedure testproc1(" +
                "	in1 in number, " +
                "	in2 in char, " +
                "	in3 in varchar2, " +
                "	in4 in date) as " +
                "begin " +
                "	null; " +
                "end;"));
            assertTrue(cur.sendQuery(
                "create procedure testproc2(" +
                "	in1 in number, " +
                "	in2 in char, " +
                "	in3 in varchar2, " +
                "	in4 in date) as " +
                "begin " +
                "	null; " +
                "end;"));
            assertTrue(cur.sendQuery(
                "create procedure testproc3(" +
                "	in1 in number, " +
                "	in2 in char, " +
                "	in3 in varchar2, " +
                "	in4 in date) as " +
                "begin " +
                "	null; " +
                "end;"));
            assertTrue(cur.sendQuery(
                "create procedure testproc4(" +
                "	in1 in number, " +
                "	in2 in char, " +
                "	in3 in varchar2, " +
                "	in4 in date) as " +
                "begin " +
                "	null; " +
                "end;"));
            assertTrue(cur.getProcedureList(null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++) {
                String name = cur.getField(i, "routine_name");
                if (name == "TESTPROC1" ||
                    name == "TESTPROC2" ||
                    name == "TESTPROC3" ||
                    name == "TESTPROC4") {
                    counter++;
                }
            }
            assertEquals(counter, (UInt16)4);
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
            assertEquals(cur.getField((UInt64)0, "parameter_name"), "IN1");
            assertEquals(cur.getField((UInt64)0, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)0, "data_type"), "NUMBER");
            assertEquals(cur.getField((UInt64)0, "ordinal_position"), "1");
            assertEquals(cur.getField((UInt64)1, "parameter_name"), "IN2");
            assertEquals(cur.getField((UInt64)1, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)1, "data_type"), "CHAR");
            assertEquals(cur.getField((UInt64)1, "ordinal_position"), "2");
            assertEquals(cur.getField((UInt64)2, "parameter_name"), "IN3");
            assertEquals(cur.getField((UInt64)2, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)2, "data_type"), "VARCHAR2");
            assertEquals(cur.getField((UInt64)2, "ordinal_position"), "3");
            assertEquals(cur.getField((UInt64)3, "parameter_name"), "IN4");
            assertEquals(cur.getField((UInt64)3, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)3, "data_type"), "DATE");
            assertEquals(cur.getField((UInt64)3, "ordinal_position"), "4");
            assertTrue(cur.sendQuery("drop procedure testproc1"));
            assertTrue(cur.sendQuery("drop procedure testproc2"));
            assertTrue(cur.sendQuery("drop procedure testproc3"));
            assertTrue(cur.sendQuery("drop procedure testproc4"));
            Console.WriteLine("");


            // invalid queries
            Console.WriteLine("INVALID QUERIES: ");
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
            assertFalse(cur.sendQuery(
                "select " +
                "	* " +
                "from " +
                "	testtable " +
                "order by " +
                "	testnumber"));
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
