import java.sql.*;

public class myclass {
        public static void main(String[] args) {

                try {
                        Connection con=DriverManager.getConnection(
                                "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");
                        Statement stmt=con.createStatement();

                        stmt.executeQuery(
                                "select * from my_nonexistant_table");

                        stmt.close();
                        con.close();
                } catch (SQLException ex) {
                        System.out.println("Error: "+ex.getMessage());
                }
        }
}
