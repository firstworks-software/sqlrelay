import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");
                Statement stmt=con.createStatement();

                ResultSet rs=stmt.executeQuery(
                        "select image, description from images");

                while (rs.next()) {
                        byte[] image=rs.getBytes(1);
                        String desc=rs.getString(2);

                        ... do something with image and desc ...
                }

                rs.close();
                stmt.close();
                con.close();
        }
}
