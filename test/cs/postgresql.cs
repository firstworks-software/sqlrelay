// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class PostgresqlTest : SQLRTest
    {
        public static int Main(string[] args)
        {
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
            UInt64 counter = 0;

            Int32 LARGE_BUFFER_LENGTH = 8192;
            Byte[] largebuffer = new Byte[LARGE_BUFFER_LENGTH];


            // instantiation
            con = new SQLRConnection("sqlrelay", 9000, "/tmp/test.socket",
                                    "testuser", "testpassword", 0, 1);
            cur = new SQLRCursor(con);


            // identify
            Console.WriteLine("IDENTIFY: ");
            assertEquals(con.identify(), "postgresql");
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
            assertEquals(con.bindFormat(), "$1");
            Console.WriteLine("");


            // nextval format
            Console.WriteLine("NEXTVAL FORMAT: ");
            assertEquals(con.nextvalFormat(), "nextval('%s')");
            Console.WriteLine("");


            // isolation levels
            String[] isolationlevels = new String[] {
                    "read committed","read uncommitted",
                    "repeatable read","serializable"};
            Console.WriteLine("ISOLATION LEVELS: ");
            for (int ili=0; ili<isolationlevels.Length; ili++) {
                String il=isolationlevels[ili];
                // postgresql requires the isolation level to
                // be the first query of the transaction
                con.begin();
                assertTrue(con.setIsolationLevel(il));
                assertEquals(con.getIsolationLevel(),il);
                con.commit();
                Console.WriteLine("");
            }
            // reset to the default isolation level
            con.begin();
            assertTrue(con.setIsolationLevel(isolationlevels[0]));
            con.commit();
            Console.WriteLine("");


            // create testtable
            Console.WriteLine("CREATE TESTTABLE: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	testint int, "
                + "	testfloat float, "
                + "	testreal real, "
                + "	testsmallint smallint, "
                + "	testchar char(40), "
                + "	testvarchar varchar(40), "
                + "	testdate date, "
                + "	testtime time, "
                + "	testtimestamp timestamp, "
                + "	testtext text, "
                + "	testbytea bytea)"));
            Console.WriteLine("");


            // insert
            Console.WriteLine("INSERT: ");
            assertTrue(con.begin());
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	1, "
                + "	1.5, "
                + "	1.5, "
                + "	1, "
                + "	'testchar1', "
                + "	'testvarchar1', "
                + "	'01/01/2001', "
                + "	'01:00:00', "
                + "	NULL, "
                + "	'testtext1', "
                + "	'testbytea1')"));
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	2, "
                + "	2.5, "
                + "	2.5, "
                + "	2, "
                + "	'testchar2', "
                + "	'testvarchar2', "
                + "	'01/01/2002', "
                + "	'02:00:00', "
                + "	NULL, "
                + "	'testtext2', "
                + "	'testbytea2')"));
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	3, "
                + "	3.5, "
                + "	3.5, "
                + "	3, "
                + "	'testchar3', "
                + "	'testvarchar3', "
                + "	'01/01/2003', "
                + "	'03:00:00', "
                + "	NULL, "
                + "	'testtext3', "
                + "	'testbytea3')"));
            assertTrue(cur.sendQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	4, "
                + "	4.5, "
                + "	4.5, "
                + "	4, "
                + "	'testchar4', "
                + "	'testvarchar4', "
                + "	'01/01/2004', "
                + "	'04:00:00', "
                + "	NULL, "
                + "	'testtext4', "
                + "	'testbytea4')"));
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
                + "	$1, "
                + "	$2, "
                + "	$3, "
                + "	$4, "
                + "	$5, "
                + "	$6, "
                + "	$7, "
                + "	$8, "
                + "	NULL, "
                + "	$9, "
                + "	$10)");
            assertEquals(cur.countBindVariables(), (UInt16)10);
            cur.inputBind("1", (Int64)5);
            cur.inputBind("2", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("3", 5.5, (UInt32)4, (UInt32)2);
            cur.inputBind("4", (Int64)5);
            cur.inputBind("5", "testchar5");
            cur.inputBind("6", "testvarchar5");
            cur.inputBind("7", "01/01/2005");
            cur.inputBind("8", "05:00:00");
            cur.inputBindClob("9", "testtext5", (UInt32)9);
            cur.inputBindBlob("10", System.Text.Encoding.ASCII.GetBytes("testbytea5"), (UInt32)10);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)6);
            cur.inputBind("2", 6.5, (UInt32)4, (UInt32)2);
            cur.inputBind("3", 6.5, (UInt32)4, (UInt32)2);
            cur.inputBind("4", (Int64)6);
            cur.inputBind("5", "testchar6");
            cur.inputBind("6", "testvarchar6");
            cur.inputBind("7", "01/01/2006");
            cur.inputBind("8", "06:00:00");
            cur.inputBindClob("9", "testtext6", (UInt32)9);
            cur.inputBindBlob("10", System.Text.Encoding.ASCII.GetBytes("testbytea6"), (UInt32)10);
            assertTrue(cur.executeQuery());
            cur.clearBinds();
            cur.inputBind("1", (Int64)7);
            cur.inputBind("2", 7.5, (UInt32)4, (UInt32)2);
            cur.inputBind("3", 7.5, (UInt32)4, (UInt32)2);
            cur.inputBind("4", (Int64)7);
            cur.inputBind("5", "testchar7");
            cur.inputBind("6", "testvarchar7");
            cur.inputBind("7", "01/01/2007");
            cur.inputBind("8", "07:00:00");
            cur.inputBindClob("9", "testtext7", (UInt32)9);
            cur.inputBindBlob("10", System.Text.Encoding.ASCII.GetBytes("testbytea8"), (UInt32)10);
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by position
            // postgresql doesn't support implicit conversion of string binds to
            // other data types, so arrays of binds don't generally work.


            // input bind by name
            // postgresql doesn't support bind by name


            // input bind by position with validation
            Console.WriteLine("BIND BY POSITION WITH VALIDATION: ");
            cur.clearBinds();
            cur.inputBind("1", (Int64)8);
            cur.inputBind("2", 8.5, (UInt32)4, (UInt32)2);
            cur.inputBind("3", 8.5, (UInt32)4, (UInt32)2);
            cur.inputBind("4", (Int64)8);
            cur.inputBind("5", "testchar8");
            cur.inputBind("6", "testvarchar8");
            cur.inputBind("7", "01/01/2008");
            cur.inputBind("8", "08:00:00");
            cur.inputBindClob("9", "testtext8", (UInt32)9);
            cur.inputBindClob("10", "testbytea8", (UInt32)10);
            cur.validateBinds();
            assertTrue(cur.executeQuery());
            Console.WriteLine("");


            // array of input binds by name
            // postgresql doesn't support bind by name


            // input bind by name with validation
            // postgresql doesn't support bind by name


            // select
            Console.WriteLine("SELECT: ");
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            Console.WriteLine("");


            // column count
            Console.WriteLine("COLUMN COUNT: ");
            assertEquals(cur.colCount(), (UInt32)11);
            Console.WriteLine("");


            // column names
            Console.WriteLine("COLUMN NAMES: ");
            assertEquals(cur.getColumnName((UInt32)0), "testint");
            assertEquals(cur.getColumnName((UInt32)1), "testfloat");
            assertEquals(cur.getColumnName((UInt32)2), "testreal");
            assertEquals(cur.getColumnName((UInt32)3), "testsmallint");
            assertEquals(cur.getColumnName((UInt32)4), "testchar");
            assertEquals(cur.getColumnName((UInt32)5), "testvarchar");
            assertEquals(cur.getColumnName((UInt32)6), "testdate");
            assertEquals(cur.getColumnName((UInt32)7), "testtime");
            assertEquals(cur.getColumnName((UInt32)8), "testtimestamp");
            assertEquals(cur.getColumnName((UInt32)9), "testtext");
            assertEquals(cur.getColumnName((UInt32)10), "testbytea");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testint");
            assertEquals(cols[1], "testfloat");
            assertEquals(cols[2], "testreal");
            assertEquals(cols[3], "testsmallint");
            assertEquals(cols[4], "testchar");
            assertEquals(cols[5], "testvarchar");
            assertEquals(cols[6], "testdate");
            assertEquals(cols[7], "testtime");
            assertEquals(cols[8], "testtimestamp");
            assertEquals(cols[9], "testtext");
            assertEquals(cols[10], "testbytea");
            Console.WriteLine("");


            // column types
            Console.WriteLine("COLUMN TYPES: ");
            assertEquals(cur.getColumnType((UInt32)0), "int4");
            assertEquals(cur.getColumnType("testint"), "int4");
            assertEquals(cur.getColumnType((UInt32)1), "float8");
            assertEquals(cur.getColumnType("testfloat"), "float8");
            assertEquals(cur.getColumnType((UInt32)2), "float4");
            assertEquals(cur.getColumnType("testreal"), "float4");
            assertEquals(cur.getColumnType((UInt32)3), "int2");
            assertEquals(cur.getColumnType("testsmallint"), "int2");
            assertEquals(cur.getColumnType((UInt32)4), "bpchar");
            assertEquals(cur.getColumnType("testchar"), "bpchar");
            assertEquals(cur.getColumnType((UInt32)5), "varchar");
            assertEquals(cur.getColumnType("testvarchar"), "varchar");
            assertEquals(cur.getColumnType((UInt32)6), "date");
            assertEquals(cur.getColumnType("testdate"), "date");
            assertEquals(cur.getColumnType((UInt32)7), "time");
            assertEquals(cur.getColumnType("testtime"), "time");
            assertEquals(cur.getColumnType((UInt32)8), "timestamp");
            assertEquals(cur.getColumnType("testtimestamp"), "timestamp");
            assertEquals(cur.getColumnType((UInt32)9), "text");
            assertEquals(cur.getColumnType("testtext"), "text");
            assertEquals(cur.getColumnType((UInt32)10), "bytea");
            assertEquals(cur.getColumnType("testbytea"), "bytea");
            Console.WriteLine("");


            // column length
            Console.WriteLine("COLUMN LENGTH: ");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)4);
            assertEquals(cur.getColumnLength("testint"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)1), (UInt32)8);
            assertEquals(cur.getColumnLength("testfloat"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)2), (UInt32)4);
            assertEquals(cur.getColumnLength("testreal"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)3), (UInt32)2);
            assertEquals(cur.getColumnLength("testsmallint"), (UInt32)2);
            assertEquals(cur.getColumnLength((UInt32)4), (UInt32)40);
            assertEquals(cur.getColumnLength("testchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)5), (UInt32)40);
            assertEquals(cur.getColumnLength("testvarchar"), (UInt32)40);
            assertEquals(cur.getColumnLength((UInt32)6), (UInt32)4);
            assertEquals(cur.getColumnLength("testdate"), (UInt32)4);
            assertEquals(cur.getColumnLength((UInt32)7), (UInt32)8);
            assertEquals(cur.getColumnLength("testtime"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)8), (UInt32)8);
            assertEquals(cur.getColumnLength("testtimestamp"), (UInt32)8);
            assertEquals(cur.getColumnLength((UInt32)9), (UInt32)0);
            assertEquals(cur.getColumnLength("testtext"), (UInt32)0);
            assertEquals(cur.getColumnLength((UInt32)10), (UInt32)0);
            assertEquals(cur.getColumnLength("testbytea"), (UInt32)0);
            Console.WriteLine("");


            // longest column
            Console.WriteLine("LONGEST COLUMN: ");
            assertEquals(cur.getLongest((UInt32)0), (UInt32)1);
            assertEquals(cur.getLongest("testint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)1), (UInt32)3);
            assertEquals(cur.getLongest("testfloat"), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)2), (UInt32)3);
            assertEquals(cur.getLongest("testreal"), (UInt32)3);
            assertEquals(cur.getLongest((UInt32)3), (UInt32)1);
            assertEquals(cur.getLongest("testsmallint"), (UInt32)1);
            assertEquals(cur.getLongest((UInt32)4), (UInt32)40);
            assertEquals(cur.getLongest("testchar"), (UInt32)40);
            assertEquals(cur.getLongest((UInt32)5), (UInt32)12);
            assertEquals(cur.getLongest("testvarchar"), (UInt32)12);
            assertEquals(cur.getLongest((UInt32)6), (UInt32)10);
            assertEquals(cur.getLongest("testdate"), (UInt32)10);
            assertEquals(cur.getLongest((UInt32)7), (UInt32)8);
            assertEquals(cur.getLongest("testtime"), (UInt32)8);
            assertEquals(cur.getLongest((UInt32)9), (UInt32)9);
            assertEquals(cur.getLongest("testtext"), (UInt32)9);
            assertEquals(cur.getLongest((UInt32)10), (UInt32)10);
            assertEquals(cur.getLongest("testbytea"), (UInt32)10);
            Console.WriteLine("");


            // row count
            Console.WriteLine("ROW COUNT: ");
            assertEquals(cur.rowCount(), (UInt64)8);
            Console.WriteLine("");


            // total rows
            Console.WriteLine("TOTAL ROWS: ");
            assertEquals(cur.totalRows(), (UInt64)8);
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
            assertEquals(cur.getField((UInt64)0, (UInt32)1), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "1.5");
            assertEquals(cur.getField((UInt64)0, (UInt32)3), "1");
            assertEquals(cur.getField((UInt64)0, (UInt32)4), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, (UInt32)5), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, (UInt32)6), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, (UInt32)7), "01:00:00");
            assertEquals(cur.getField((UInt64)0, (UInt32)9), "testtext1");
            assertEquals(cur.getField((UInt64)0, (UInt32)10), "testbytea1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, (UInt32)0), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)1), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)2), "8.5");
            assertEquals(cur.getField((UInt64)7, (UInt32)3), "8");
            assertEquals(cur.getField((UInt64)7, (UInt32)4), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, (UInt32)5), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, (UInt32)6), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, (UInt32)7), "08:00:00");
            assertEquals(cur.getField((UInt64)7, (UInt32)9), "testtext8");
            assertEquals(cur.getField((UInt64)7, (UInt32)10), "testbytea8");
            Console.WriteLine("");


            // field lengths by index
            Console.WriteLine("FIELD LENGTHS BY INDEX: ");
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)1), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)2), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)3), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)4), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)5), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)6), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)7), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)9), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, (UInt32)10), (UInt32)10);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)0), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)1), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)2), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)3), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)4), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)5), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)6), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)7), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)9), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, (UInt32)10), (UInt32)10);
            Console.WriteLine("");


            // fields by name
            Console.WriteLine("FIELDS BY NAME: ");
            assertEquals(cur.getField((UInt64)0, "testint"), "1");
            assertEquals(cur.getField((UInt64)0, "testfloat"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testreal"), "1.5");
            assertEquals(cur.getField((UInt64)0, "testsmallint"), "1");
            assertEquals(cur.getField((UInt64)0, "testchar"), "testchar1                               ");
            assertEquals(cur.getField((UInt64)0, "testvarchar"), "testvarchar1");
            assertEquals(cur.getField((UInt64)0, "testdate"), "2001-01-01");
            assertEquals(cur.getField((UInt64)0, "testtime"), "01:00:00");
            assertEquals(cur.getField((UInt64)0, "testtext"), "testtext1");
            assertEquals(cur.getField((UInt64)0, "testbytea"), "testbytea1");
            Console.WriteLine("");
            assertEquals(cur.getField((UInt64)7, "testint"), "8");
            assertEquals(cur.getField((UInt64)7, "testfloat"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testreal"), "8.5");
            assertEquals(cur.getField((UInt64)7, "testsmallint"), "8");
            assertEquals(cur.getField((UInt64)7, "testchar"), "testchar8                               ");
            assertEquals(cur.getField((UInt64)7, "testvarchar"), "testvarchar8");
            assertEquals(cur.getField((UInt64)7, "testdate"), "2008-01-01");
            assertEquals(cur.getField((UInt64)7, "testtime"), "08:00:00");
            assertEquals(cur.getField((UInt64)7, "testtext"), "testtext8");
            assertEquals(cur.getField((UInt64)7, "testbytea"), "testbytea8");
            Console.WriteLine("");


            // field lengths by name
            Console.WriteLine("FIELD LENGTHS BY NAME: ");
            assertEquals(cur.getFieldLength((UInt64)0, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testreal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)0, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)0, "testchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)0, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)0, "testdate"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)0, "testtime"), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)0, "testtext"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)0, "testbytea"), (UInt32)10);
            Console.WriteLine("");
            assertEquals(cur.getFieldLength((UInt64)7, "testint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testfloat"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testreal"), (UInt32)3);
            assertEquals(cur.getFieldLength((UInt64)7, "testsmallint"), (UInt32)1);
            assertEquals(cur.getFieldLength((UInt64)7, "testchar"), (UInt32)40);
            assertEquals(cur.getFieldLength((UInt64)7, "testvarchar"), (UInt32)12);
            assertEquals(cur.getFieldLength((UInt64)7, "testdate"), (UInt32)10);
            assertEquals(cur.getFieldLength((UInt64)7, "testtime"), (UInt32)8);
            assertEquals(cur.getFieldLength((UInt64)7, "testtext"), (UInt32)9);
            assertEquals(cur.getFieldLength((UInt64)7, "testbytea"), (UInt32)10);
            Console.WriteLine("");


            // fields by array
            Console.WriteLine("FIELDS BY ARRAY: ");
            fields = cur.getRow((UInt64)0);
            assertEquals(fields[0], "1");
            assertEquals(fields[1], "1.5");
            assertEquals(fields[2], "1.5");
            assertEquals(fields[3], "1");
            assertEquals(fields[4], "testchar1                               ");
            assertEquals(fields[5], "testvarchar1");
            assertEquals(fields[6], "2001-01-01");
            assertEquals(fields[7], "01:00:00");
            assertEquals(fields[9], "testtext1");
            assertEquals(fields[10], "testbytea1");
            Console.WriteLine("");


            // field lengths by array
            Console.WriteLine("FIELD LENGTHS BY ARRAY: ");
            fieldlens = cur.getRowLengths((UInt64)0);
            assertEquals(fieldlens[0], (UInt32)1);
            assertEquals(fieldlens[1], (UInt32)3);
            assertEquals(fieldlens[2], (UInt32)3);
            assertEquals(fieldlens[3], (UInt32)1);
            assertEquals(fieldlens[4], (UInt32)40);
            assertEquals(fieldlens[5], (UInt32)12);
            assertEquals(fieldlens[6], (UInt32)10);
            assertEquals(fieldlens[7], (UInt32)8);
            assertEquals(fieldlens[9], (UInt32)9);
            assertEquals(fieldlens[10], (UInt32)10);
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
            assertEquals(cur.getColumnName((UInt32)0), (String)null);
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)0);
            assertEquals(cur.getColumnType((UInt32)0), (String)null);
            cur.getColumnInfo();
            assertTrue(cur.sendQuery("select * from testtable order by testint"));
            assertEquals(cur.getColumnName((UInt32)0), "testint");
            assertEquals(cur.getColumnLength((UInt32)0), (UInt32)4);
            assertEquals(cur.getColumnType((UInt32)0), "int4");
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
            assertEquals(cur.colCount(), (UInt32)11);
            Console.WriteLine("");


            // column names for cached result set
            Console.WriteLine("COLUMN NAMES FOR CACHED RESULT SET: ");
            assertEquals(cur.getColumnName((UInt32)0), "testint");
            assertEquals(cur.getColumnName((UInt32)1), "testfloat");
            assertEquals(cur.getColumnName((UInt32)2), "testreal");
            assertEquals(cur.getColumnName((UInt32)3), "testsmallint");
            assertEquals(cur.getColumnName((UInt32)4), "testchar");
            assertEquals(cur.getColumnName((UInt32)5), "testvarchar");
            assertEquals(cur.getColumnName((UInt32)6), "testdate");
            assertEquals(cur.getColumnName((UInt32)7), "testtime");
            assertEquals(cur.getColumnName((UInt32)8), "testtimestamp");
            cols = cur.getColumnNames();
            assertEquals(cols[0], "testint");
            assertEquals(cols[1], "testfloat");
            assertEquals(cols[2], "testreal");
            assertEquals(cols[3], "testsmallint");
            assertEquals(cols[4], "testchar");
            assertEquals(cols[5], "testvarchar");
            assertEquals(cols[6], "testdate");
            assertEquals(cols[7], "testtime");
            assertEquals(cols[8], "testtimestamp");
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
            cur.setResultSetBufferSize(1);
            assertTrue(cur.sendQuery("select * from testtable"));
            secondcur = new SQLRCursor(con);
            secondcur.setResultSetBufferSize(1);
            for (UInt32 i = 0; cur.getRow((UInt64)i) != null; i++)
            {
                assertTrue(secondcur.sendQuery("select * from testtable"));
            }
            secondcur.closeResultSet();
            cur.setResultSetBufferSize(0);
            assertTrue(cur.sendQuery("drop table testtable"));
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
            // postgresql DDL is transactional; commit so the table is visible
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
            cur.prepareQuery("select $(var1),'$(var2)',$(var3)");
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
                + "	testclob1 text, "
                + "	testclob2 text, "
                + "	testblob1 bytea, "
                + "	testblob2 bytea)"));
            cur.prepareQuery(
                "insert into "
                + "	testtable "
                + "values ("
                + "	$1, "
                + "	$2, "
                + "	$3, "
                + "	$4)");
            cur.inputBindClob("1", "", (UInt32)0);
            cur.inputBindClob("2", (String)null, (UInt32)0);
            cur.inputBindBlob("3", new Byte[0], (UInt32)0);
            cur.inputBindBlob("4", (Byte[])null, (UInt32)0);
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
                + "	testtext text, "
                + "	testbytea bytea)");
            cur.prepareQuery("insert into testtable values ($1,$2)");
            for (int i = 0; i < LARGE_BUFFER_LENGTH; i++)
            {
                largebuffer[i] = (Byte)'C';
            }
            String largebufferstr = System.Text.Encoding.ASCII.GetString(largebuffer);
            cur.inputBindClob("1", largebufferstr, (UInt32)LARGE_BUFFER_LENGTH);
            cur.inputBindBlob("2", largebuffer, (UInt32)LARGE_BUFFER_LENGTH);
            assertTrue(cur.executeQuery());
            cur.sendQuery("select * from testtable");
            assertEquals(cur.getFieldLength((UInt64)0, "testtext"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getField((UInt64)0, "testtext"), largebufferstr);
            assertEquals(cur.getFieldLength((UInt64)0, "testbytea"), (UInt32)LARGE_BUFFER_LENGTH);
            assertEquals(cur.getFieldAsByteArray((UInt64)0, "testbytea"), largebuffer,
                                LARGE_BUFFER_LENGTH);
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // output bind by position
            // postgresql doesn't support output binds


            // output bind by name
            // postgresql doesn't support output binds


            // output bind by name with validation
            // postgresql doesn't support output binds


            // lob output bind
            // postgresql doesn't support output binds


            // long output bind
            // postgresql doesn't support output binds


            // negative input bind
            Console.WriteLine("NEGATIVE INPUT BIND: ");
            cur.sendQuery("drop table testtable");
            cur.sendQuery("create table testtable (testval int)");
            cur.prepareQuery("insert into testtable values ($1)");
            cur.inputBind("1", (Int64)(-1));
            assertTrue(cur.executeQuery());
            cur.sendQuery("select testval from testtable");
            assertEquals(cur.getField((UInt64)0, "testval"), "-1");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // bind validation
            // postgresql doesn't support bind by name


            // rebinding
            Console.WriteLine("REBINDING: ");
            cur.sendQuery("drop function testfunc(int)");
            assertTrue(cur.sendQuery(
                "create function testfunc(int) returns int as "
                + "	' begin return $1; end;' language plpgsql"));
            cur.prepareQuery("select * from testfunc($1)");
            cur.inputBind("1", (Int64)1);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            cur.inputBind("1", (Int64)2);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "2");
            cur.inputBind("1", (Int64)3);
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "3");
            assertTrue(cur.sendQuery("drop function testfunc(int)"));
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
            cur.prepareQuery("select $1::int");
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
            cur.sendQuery("drop function testfunc(int,float,char(20))");
            assertTrue(cur.sendQuery(
                "create function testfunc("
                + "	int,float,char(20)) "
                + "returns void as ' "
                + "	declare in1 int; "
                + "	in2 float; "
                + "	in3 char(20); "
                + "begin "
                + "	in1:=$1; "
                + "	in2:=$2; "
                + "	in3:=$3; "
                + "	return; "
                + "end;' language plpgsql"));
            cur.prepareQuery("select testfunc($1,$2,$3)");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 1.5, (UInt32)4, (UInt32)2);
            cur.inputBind("3", "hello");
            assertTrue(cur.executeQuery());
            assertTrue(cur.sendQuery("drop function testfunc(int,float,char(20))"));
            Console.WriteLine("");


            // stored procedure returning single value
            Console.WriteLine("STORED PROCEDURE RETURNING SINGLE VALUE: ");
            cur.sendQuery("drop function testfunc(int,float,char(20))");
            assertTrue(cur.sendQuery(
                "create function testfunc(int,float,char(20)) returns int as "
                + "	' begin return $1; end;' language plpgsql"));
            cur.prepareQuery("select * from testfunc($1,$2,$3)");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 1.5, (UInt32)4, (UInt32)2);
            cur.inputBind("3", "hello");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertTrue(cur.sendQuery("drop function testfunc(int,float,char(20))"));
            Console.WriteLine("");


            // stored procedure returning multiple values
            Console.WriteLine("STORED PROCEDURE RETURNING MULTIPLE VALUES: ");
            cur.sendQuery("drop function testfunc(int,float,char(20))");
            assertTrue(cur.sendQuery(
                "create function testfunc("
                + "	int,float,char(20)) "
                + "returns record as ' "
                + "	declare output record; "
                + "begin "
                + "	select $1,$2,$3 into output; "
                + "	return output; "
                + "end;' language plpgsql"));
            cur.prepareQuery(
                "select "
                + "	* "
                + "from "
                + "	testfunc($1,$2,$3) "
                + "	as (col1 int, "
                + "		col2 float, "
                + "		col3 bpchar) ");
            cur.inputBind("1", (Int64)1);
            cur.inputBind("2", 1.5, (UInt32)4, (UInt32)2);
            cur.inputBind("3", "hello");
            assertTrue(cur.executeQuery());
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            assertEquals(cur.getFieldAsDouble((UInt64)0, (UInt32)1), 1.5);
            assertEquals(cur.getField((UInt64)0, (UInt32)2), "hello");
            assertTrue(cur.sendQuery("drop function testfunc(int,float,char(20))"));
            Console.WriteLine("");


            // stored procedure returning result set
            Console.WriteLine("STORED PROCEDURE RETURNING RESULT SET: ");
            cur.sendQuery("drop function testfunc()");
            assertTrue(cur.sendQuery(
                "create function testfunc() "
                + "returns setof record as ' "
                + "	declare output record; "
                + "begin "
                + "	for output in "
                + "		select 1 "
                + "		union "
                + "		select 2 "
                + "		union "
                + "		select 3 "
                + "		union "
                + "		select 4 "
                + "		union "
                + "		select 5 "
                + "		union "
                + "		select 6 "
                + "		union "
                + "		select 7 "
                + "		union "
                + "		select 8 "
                + "	loop "
                + "		return next output; "
                + "	end loop; "
                + "	return; "
                + "end;' language plpgsql"));
            assertTrue(cur.sendQuery(
                "select "
                + "	* "
                + "from "
                + "	testfunc() "
                + "	as (testint int)"));
            assertEquals(cur.rowCount(), (UInt64)8);
            assertTrue(cur.sendQuery("drop function testfunc()"));
            Console.WriteLine("");


            // temporary tables
            Console.WriteLine("TEMPORARY TABLES: ");
            cur.sendQuery("drop table temptable\n");
            cur.sendQuery("create temporary table temptable (col1 int)");
            assertTrue(cur.sendQuery("insert into temptable values (1)"));
            assertTrue(cur.sendQuery("select count(*) from temptable"));
            assertEquals(cur.getField((UInt64)0, (UInt32)0), "1");
            con.endSession();
            Console.WriteLine("");
            assertFalse(cur.sendQuery("select count(*) from temptable"));
            Console.WriteLine("");


            // encoded binary data
            Console.WriteLine("ENCODED BINARY DATA: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery("create table testtable (col1 bytea)"));
            Byte[] buffer = new Byte[256];
            for (int i = 0; i < 256; i++)
            {
                buffer[i] = (Byte)i;
            }
            System.Text.StringBuilder querystr = new System.Text.StringBuilder();
            querystr.Append("insert into testtable values (decode('");
            for (int i = 0; i < buffer.Length; i++)
            {
                querystr.Append(String.Format("{0:x2}", buffer[i]));
            }
            querystr.Append("','hex'))");
            assertTrue(cur.sendQuery(querystr.ToString()));
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
                    "create table testtable "
                    + "	(col1 serial primary key, "
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
            assertEquals(cur.getColumnName((UInt32)0), "Database");
            assertTrue(cur.rowCount() > 0);
            Console.WriteLine("");


            // schema list
            Console.WriteLine("SCHEMA LIST: ");
            assertTrue(cur.getSchemaList(null));
            assertEquals(cur.getColumnName((UInt32)0), "Database");
            assertTrue(cur.rowCount() > 0);
            Console.WriteLine("");


            // table type list
            Console.WriteLine("TABLE TYPE LIST: ");
            assertTrue(cur.getTableTypeList());
            assertEquals(cur.getColumnName((UInt32)0), "table_type");
            Boolean found = false;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                if (cur.getField((UInt64)i, "table_type") == "TABLE")
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
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                String name = cur.getField((UInt64)i, "Tables_in_xxx");
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
            assertEquals(cur.getField((UInt64)0, "precision"), "255");
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
                "create table testtable ("
                + "	testint int, "
                + "	testfloat float, "
                + "	testreal real, "
                + "	testsmallint smallint, "
                + "	testchar char(40), "
                + "	testvarchar varchar(40), "
                + "	testdate date, "
                + "	testtime time, "
                + "	testtimestamp timestamp, "
                + "	testtext text, "
                + "	testbytea bytea)"));
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
            assertEquals(cur.getField((UInt64)0, "column_name"), "testint");
            assertEquals(cur.getField((UInt64)1, "column_name"), "testfloat");
            assertEquals(cur.getField((UInt64)2, "column_name"), "testreal");
            assertEquals(cur.getField((UInt64)3, "column_name"), "testsmallint");
            assertEquals(cur.getField((UInt64)4, "column_name"), "testchar");
            assertEquals(cur.getField((UInt64)5, "column_name"), "testvarchar");
            assertEquals(cur.getField((UInt64)6, "column_name"), "testdate");
            assertEquals(cur.getField((UInt64)7, "column_name"), "testtime");
            assertEquals(cur.getField((UInt64)8, "column_name"), "testtimestamp");
            assertEquals(cur.getField((UInt64)9, "column_name"), "testtext");
            assertEquals(cur.getField((UInt64)10, "column_name"), "testbytea");
            assertEquals(cur.getField((UInt64)0, "data_type"), "integer");
            assertEquals(cur.getField((UInt64)1, "data_type"), "double precision");
            assertEquals(cur.getField((UInt64)2, "data_type"), "real");
            assertEquals(cur.getField((UInt64)3, "data_type"), "smallint");
            assertEquals(cur.getField((UInt64)4, "data_type"), "character");
            assertEquals(cur.getField((UInt64)5, "data_type"), "character varying");
            assertEquals(cur.getField((UInt64)6, "data_type"), "date");
            assertEquals(cur.getField((UInt64)7, "data_type"), "time without time zone");
            assertEquals(cur.getField((UInt64)8, "data_type"), "timestamp without time zone");
            assertEquals(cur.getField((UInt64)9, "data_type"), "text");
            assertEquals(cur.getField((UInt64)10, "data_type"), "bytea");
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // column list - auto_increment, primary key
            Console.WriteLine("COLUMN LIST - auto_increment, primary key: ");
            cur.sendQuery("drop table if exists testtable");
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	col1 serial primary key, "
                + "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertTrue(cur.getField((UInt64)0, "extra") != null &&
                    cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key") != null &&
                    cur.getField((UInt64)0, "column_key").Contains("PRI"));
            assertFalse(cur.getField((UInt64)1, "extra") != null &&
                    cur.getField((UInt64)1, "extra").Contains("auto_increment"));
            assertFalse(cur.getField((UInt64)1, "column_key") != null &&
                    cur.getField((UInt64)1, "column_key").Contains("PRI"));
            Console.WriteLine("");
            assertTrue(cur.sendQuery("drop table testtable"));
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	col1 int primary key, "
                + "	col2 int)"));
            assertTrue(cur.getColumnList("testtable", null));
            assertFalse(cur.getField((UInt64)0, "extra") != null &&
                    cur.getField((UInt64)0, "extra").Contains("auto_increment"));
            assertTrue(cur.getField((UInt64)0, "column_key") != null &&
                    cur.getField((UInt64)0, "column_key").Contains("PRI"));
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
            Console.WriteLine("");


            // key and index list
            Console.WriteLine("KEY AND INDEX LIST: ");
            cur.sendQuery("drop table testtable");
            assertTrue(cur.sendQuery(
                "create table testtable ("
                + "	col1 int primary key, "
                + "	col2 int)"));
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
            assertTrue(cur.getField((UInt64)0, "table") == "testtable");
            assertEquals(cur.getField((UInt64)0, "non_unique"), "f");
            assertEquals(cur.getField((UInt64)0, "seq_in_index"), "1");
            assertTrue(cur.getField((UInt64)0, "column_name") == "col1");
            assertEquals(cur.getField((UInt64)0, "collation"), "A");
            assertEquals(cur.getField((UInt64)0, "index_type"), "3");
            assertTrue(!String.IsNullOrEmpty(cur.getField((UInt64)0, "key_name")));
            assertTrue(cur.sendQuery("drop table testtable"));
            Console.WriteLine("");


            // procedure list
            Console.WriteLine("PROCEDURE LIST: ");
            cur.sendQuery("drop function testproc1(int,char,varchar,date)");
            cur.sendQuery("drop function testproc2(int,char,varchar,date)");
            cur.sendQuery("drop function testproc3(int,char,varchar,date)");
            cur.sendQuery("drop function testproc4(int,char,varchar,date)");
            assertTrue(cur.sendQuery(
                "create function testproc1("
                + "	in1 int, "
                + "	in2 char(20), "
                + "	in3 varchar(20), "
                + "	in4 date) "
                + "returns void "
                + "as 'begin end;' "
                + "language plpgsql"));
            assertTrue(cur.sendQuery(
                "create function testproc2("
                + "	in1 int, "
                + "	in2 char(20), "
                + "	in3 varchar(20), "
                + "	in4 date) "
                + "returns void "
                + "as 'begin end;' "
                + "language plpgsql"));
            assertTrue(cur.sendQuery(
                "create function testproc3("
                + "	in1 int, "
                + "	in2 char(20), "
                + "	in3 varchar(20), "
                + "	in4 date) "
                + "returns void "
                + "as 'begin end;' "
                + "language plpgsql"));
            assertTrue(cur.sendQuery(
                "create function testproc4("
                + "	in1 int, "
                + "	in2 char(20), "
                + "	in3 varchar(20), "
                + "	in4 date) "
                + "returns void "
                + "as 'begin end;' "
                + "language plpgsql"));
            assertTrue(cur.getProcedureList(null));
            counter = 0;
            for (UInt64 i = 0; i < cur.rowCount(); i++)
            {
                String name = cur.getField((UInt64)i, "routine_name");
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
            assertEquals(cur.getField((UInt64)1, "data_type"), "character");
            assertEquals(cur.getField((UInt64)1, "ordinal_position"), "2");
            assertEquals(cur.getField((UInt64)2, "parameter_name"), "in3");
            assertEquals(cur.getField((UInt64)2, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)2, "data_type"), "character varying");
            assertEquals(cur.getField((UInt64)2, "ordinal_position"), "3");
            assertEquals(cur.getField((UInt64)3, "parameter_name"), "in4");
            assertEquals(cur.getField((UInt64)3, "parameter_mode"), "1");
            assertEquals(cur.getField((UInt64)3, "data_type"), "date");
            assertEquals(cur.getField((UInt64)3, "ordinal_position"), "4");
            assertTrue(cur.sendQuery("drop function testproc1(int,char,varchar,date)"));
            assertTrue(cur.sendQuery("drop function testproc2(int,char,varchar,date)"));
            assertTrue(cur.sendQuery("drop function testproc3(int,char,varchar,date)"));
            assertTrue(cur.sendQuery("drop function testproc4(int,char,varchar,date)"));
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
