// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;

namespace SQLRClientTest
{
    class SQLRAdapterTest
    {
        protected static void assertEquals(Object value, Object success)
        {
            if (value == success)
            {
                Console.Write("success ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + value + "\" != \"" + success + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(String value, String success)
        {
            if (value == success)
            {
                Console.Write("success ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + value + "\" != \"" + success + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(String value, String success, Int32 size)
        {
            if (value.Substring(0,size) == success.Substring(0,size))
            {
                Console.Write("success ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + value + "\" != \"" + success + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(Int64 value, Int64 success)
        {
            if (value == success)
            {
                Console.Write("success ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + value + "\" != \"" + success + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertEquals(Boolean value, Boolean success)
        {
            if (value == success)
            {
                Console.Write("success ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + value + "\" != \"" + success + "\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertTrue(Boolean value)
        {
            if (value)
            {
                Console.Write("success ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + value + "\" != \"true\"");
                Console.Out.Flush();
                Environment.Exit(1);
            }
        }

        protected static void assertFalse(Boolean value)
        {
            if (!value)
            {
                Console.Write("success ");
                Console.Out.Flush();
            }
            else
            {
                Console.WriteLine("failure");
                Console.WriteLine("\"" + value + "\" != \"false\"");
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
