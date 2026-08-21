// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;
using System.Data;
using System.IO;
using System.Drawing;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace SQLRClientTest
{
    class UnicodeSQLRAdapterTest : SQLRTest
    {
        public static void Main(String[] args)
        {
            SQLRelayConnection sqlrcon =
                new SQLRelayConnection(
                "Data Source=sqlrelay:" +
                "9001:/tmp/oracle.socket;" +
                "User ID=testuser;" +
                "Password=testpassword;" +
                "Retry Time=0;" +
                "Tries=1;" +
                "Debug=false");
            sqlrcon.Open();
            SQLRelayCommand sqlrcom = (SQLRelayCommand)sqlrcon.CreateCommand();


            // unicode
            Console.WriteLine("UNICODE:");
            sqlrcom.CommandText =
                "select " +
                "unistr('abc\\00e5\\00f1\\00f6') " +
                "from dual";
            System.Data.IDataReader datareader = ExecuteReader(sqlrcom);
            assertTrue(datareader != null);
            assertTrue(datareader.Read());
            assertEquals(datareader.GetString(0), "abcåñö");
            Console.WriteLine("\n");
            sqlrcon.Close();

            Environment.Exit(status);
        }
    }
}
