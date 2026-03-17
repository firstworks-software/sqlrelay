import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");

                // turn off autocommit
                con.setAutoCommit(false);

                Statement stmt=con.createStatement();

                stmt.executeUpdate("insert into my_table values (1,2,3)");

                con.commit();

                stmt.executeUpdate("insert into my_table values (4,5,6)");

                con.rollback();

                // turn autocommit back on
                con.setAutoCommit(true);

                stmt.close();
                con.close();
        }
}
