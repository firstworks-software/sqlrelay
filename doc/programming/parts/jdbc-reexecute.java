import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");

                PreparedStatement stmt=con.prepareStatement(
                        "select * from mytable where mycolumn>?");

                stmt.setInt(1,1);
                ResultSet rs=stmt.executeQuery();

                ... process the result set ...

                rs.close();
                stmt.setInt(1,5);
                rs=stmt.executeQuery();

                ... process the result set ...

                rs.close();
                stmt.setInt(1,10);
                rs=stmt.executeQuery();

                ... process the result set ...

                rs.close();
                stmt.close();
                con.close();
        }
}
