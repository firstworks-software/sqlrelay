// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class SQLRAdapterTest
    {
        protected static void assertEquals(Object actual, Object expected)
        {
            if (actual == expected)
            {
                Console.Write("expected ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + actual + "\" != \"" + expected + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(String actual, String expected)
        {
            if (actual == expected)
            {
                Console.Write("expected ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + actual + "\" != \"" + expected + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(String actual, String expected, Int32 size)
        {
            if (actual.Substring(0,size) == expected.Substring(0,size))
            {
                Console.Write("expected ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + actual + "\" != \"" + expected + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(Int64 actual, Int64 expected)
        {
            if (actual == expected)
            {
                Console.Write("expected ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + actual + "\" != \"" + expected + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(Boolean actual, Boolean expected)
        {
            if (actual == expected)
            {
                Console.Write("expected ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + actual + "\" != \"" + expected + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertTrue(Boolean actual)
        {
            if (actual)
            {
                Console.Write("expected ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + actual + "\" != \"true\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertFalse(Boolean actual)
        {
            if (!actual)
            {
                Console.Write("expected ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + actual + "\" != \"false\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static Int64 ExecuteScalar(SQLRelayCommand cmd)
        {
            try
            {
                return Convert.ToInt64(cmd.ExecuteScalar());
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                Console.Out.Flush();
                return -1;
            }
        }

        protected static Int64 ExecuteNonQuery(SQLRelayCommand cmd)
        {
            try
            {
                return Convert.ToInt64(cmd.ExecuteNonQuery());
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                Console.Out.Flush();
                return -1;
            }
        }

        protected static SQLRelayDataReader ExecuteReader(SQLRelayCommand cmd)
        {
            try
            {
                return (SQLRelayDataReader)cmd.ExecuteReader();
            }
            catch (Exception ex)
            {
                Console.WriteLine(ex.Message);
                Console.Out.Flush();
                return null;
            }
        }
    }
}
