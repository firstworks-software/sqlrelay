import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");
                Statement stmt=con.createStatement();

                stmt.executeQuery("select * from my_table");

                ... do some stuff that takes a short time ...

                stmt.executeQuery("select * from my_other_table");

                ... process the result set ...

                stmt.close();
                con.close();
        }
}
