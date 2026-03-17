import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");

                PreparedStatement stmt=con.prepareStatement(
                        "select * from mytable "
                        +"where stringcol=? "
                        +"and integercol>? "
                        +"and floatcol>?");
                stmt.setString(1,"true");
                stmt.setInt(2,10);
                stmt.setDouble(3,1.1);
                ResultSet rs=stmt.executeQuery();

                ... process the result set ...

                rs.close();
                stmt.close();
                con.close();
        }
}
