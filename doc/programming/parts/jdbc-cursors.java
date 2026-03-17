import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");

                Statement stmt1=con.createStatement();
                PreparedStatement stmt2=con.prepareStatement(
                        "insert into my_other_table values (?,?,?)");

                ResultSet rs=stmt1.executeQuery(
                        "select * from my_huge_table");

                while (rs.next()) {
                        stmt2.setString(1,rs.getString(1));
                        stmt2.setString(2,rs.getString(2));
                        stmt2.setString(3,rs.getString(3));
                        stmt2.executeUpdate();
                }

                rs.close();
                stmt2.close();
                stmt1.close();
                con.close();
        }
}
