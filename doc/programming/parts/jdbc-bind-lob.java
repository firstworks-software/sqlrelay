import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");

                Statement stmt=con.createStatement();
                stmt.executeUpdate("create table images "
                        +"(image blob, description clob)");
                stmt.close();

                byte[] imagedata;
                long imagelength;

                ... read an image from a file into imagedata and the length of the
                        file into imagelength ...

                String description;
                long desclength;

                ... read a description from a file into description and the length of
                        the file into desclength ...

                PreparedStatement pstmt=con.prepareStatement(
                        "insert into images values (?,?)");
                pstmt.setBytes(1,imagedata);
                pstmt.setString(2,description);
                pstmt.executeUpdate();

                pstmt.close();
                con.close();
        }
}
