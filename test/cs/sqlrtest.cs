// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class SQLRTest
    {
        protected static int status=0;

        protected static SQLRConnection con=null;
        protected static SQLRCursor cur=null;
        protected static SQLRConnection secondcon=null;
        protected static SQLRCursor secondcur=null;

        protected static String success="\u001B[32msuccess\u001B[0m";
        protected static String failure="\u001B[31mfailure\u001B[0m";
        protected static String alltestssucceeded="\n\u001B[34mAll tests succeeded\u001B[0m";
        protected static String sometestsfailed="\n\u001B[38;5;208mSome tests failed\u001B[0m";

        protected static void printErrors()
        {
            if (cur != null)
            {
                String err = cur.errorMessage();
                if (!String.IsNullOrEmpty(err))
                {
                    Console.WriteLine(err);
                    return;
                }
            }
            if (secondcur != null)
            {
                String err = secondcur.errorMessage();
                if (!String.IsNullOrEmpty(err))
                {
                    Console.WriteLine(err);
                    return;
                }
            }
            if (con != null)
            {
                String err = con.errorMessage();
                if (!String.IsNullOrEmpty(err))
                {
                    Console.WriteLine(err);
                    return;
                }
            }
            if (secondcon != null)
            {
                String err = secondcon.errorMessage();
                if (!String.IsNullOrEmpty(err))
                {
                    Console.WriteLine(err);
                    return;
                }
            }
        }

        protected static void pass()
        {
            Console.Write(success+" ");
            Console.Out.Flush();
        }

        protected static void fail(Object actual, Object expected)
        {
            Console.WriteLine(failure);
            Console.WriteLine("\"" + actual + "\" != \"" + expected + "\"");
            printErrors();
            Console.Out.Flush();
            status=1;
        }

        protected static void assertEquals(String actual, String expected)
        {
            if (expected == null)
            {
                if (actual == null) { pass(); } else { fail(actual,expected); }
                return;
            }
            if (actual == expected || (actual != null && actual == expected))
            {
                pass();
            }
            else if (actual != null && actual.Equals(expected))
            {
                pass();
            }
            else
            {
                fail(actual,expected);
            }
        }

        protected static void assertEquals(String actual, String expected, Int32 size)
        {
            if (expected == null)
            {
                if (actual == null) { pass(); } else { fail(actual,expected); }
                return;
            }
            if (actual != null &&
                actual.Length >= size &&
                expected.Length >= size &&
                actual.Substring(0,size) == expected.Substring(0,size))
            {
                pass();
            }
            else
            {
                fail(actual,expected);
            }
        }

        protected static void assertEquals(Byte[] actual, Byte[] expected)
        {
            if (expected == null)
            {
                if (actual == null) { pass(); } else { fail(actual,expected); }
                return;
            }
            if (actual == null || actual.Length != expected.Length)
            {
                fail(actual,expected);
                return;
            }
            for (int i = 0; i < actual.Length; i++)
            {
                if (actual[i] != expected[i])
                {
                    fail(actual,expected);
                    return;
                }
            }
            pass();
        }

        protected static void assertEquals(Byte[] actual, Byte[] expected, Int32 size)
        {
            if (expected == null)
            {
                if (actual == null) { pass(); } else { fail(actual,expected); }
                return;
            }
            if (actual == null || actual.Length < size || expected.Length < size)
            {
                fail(actual,expected);
                return;
            }
            for (int i = 0; i < size; i++)
            {
                if (actual[i] != expected[i])
                {
                    fail(actual,expected);
                    return;
                }
            }
            pass();
        }

        protected static void assertEquals(Int64 actual, Int64 expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertEquals(UInt64 actual, UInt64 expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertEquals(Int32 actual, Int32 expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertEquals(UInt32 actual, UInt32 expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertEquals(Int16 actual, Int16 expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertEquals(UInt16 actual, UInt16 expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertEquals(Double actual, Double expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertEquals(Boolean actual, Boolean expected)
        {
            if (actual == expected) { pass(); } else { fail(actual,expected); }
        }

        protected static void assertTrue(Boolean actual)
        {
            if (actual) { pass(); } else { fail(actual,true); }
        }

        protected static void assertFalse(Boolean actual)
        {
            if (!actual) { pass(); } else { fail(actual,false); }
        }

        protected static void assertInResultSet(SQLRCursor cursor, String column, String value)
        {
            for (UInt64 i = 0; i < cursor.rowCount(); i++)
            {
                if (cursor.getField(i, column) == value)
                {
                    pass();
                    return;
                }
            }
            Console.WriteLine(failure);
            Console.WriteLine("\"" + value + "\" not found in column \"" + column + "\"");
            printErrors();
            status = 1;
        }

        protected static void reportTestStatus()
        {
            if (status==0)
            {
                Console.WriteLine(alltestssucceeded);
            }
            else
            {
                Console.WriteLine(sometestsfailed);
            }
            Console.Out.Flush();
        }
    }
}
