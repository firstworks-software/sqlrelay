// Copyright (c) David Muse
// See the file COPYING for more information.

using System;
using SQLRClient;
using System.Data;

namespace SQLRClientTest
{
    class PostgresqlSQLRAdapterTest : SQLRTest
    {
        public static void Main(String[] args)
        {

            // open connection and command
            SQLRelayConnection sqlrcon =
                new SQLRelayConnection(
                "Data Source=sqlrelay:" +
                "9003:/tmp/postgresql.socket;" +
                "User ID=testuser;" +
                "Password=testpassword;" +
                "Retry Time=0;" +
                "Tries=1;" +
                "Debug=false");
            sqlrcon.Open();

            SQLRelayCommand sqlrcom = (SQLRelayCommand)sqlrcon.CreateCommand();


            // drop existing table
            sqlrcom.CommandText = "drop table testtable";
            ExecuteNonQuery(sqlrcom);


            // error sqlstate
            Console.WriteLine("ERROR SQLSTATE:");
            sqlrcom.CommandText = "create table testtable (col1 int)";
            assertEquals(ExecuteNonQuery(sqlrcom), 0);

            // postgresql reports a duplicate table as 42P07, in every locale
            sqlrcom.CommandText = "create table testtable (col1 int)";
            try
            {
                sqlrcom.ExecuteNonQuery();
                assertTrue(false);
            }
            catch (SQLRelayException ex)
            {
                assertEquals(ex.SQLState, "42P07");
            }

            // and an undefined table as 42P01
            sqlrcom.CommandText = "select * from nonexistenttable";
            try
            {
                sqlrcom.ExecuteReader();
                assertTrue(false);
            }
            catch (SQLRelayException ex)
            {
                assertEquals(ex.SQLState, "42P01");
            }

            sqlrcom.CommandText = "drop table testtable";
            assertEquals(ExecuteNonQuery(sqlrcom), 0);
            Console.WriteLine("\n");


            sqlrcon.Close();

            reportTestStatus();

            Environment.Exit(status);
        }
    }
}
