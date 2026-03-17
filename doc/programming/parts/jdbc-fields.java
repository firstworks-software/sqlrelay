import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");
                Statement stmt=con.createStatement();

                ResultSet rs=stmt.executeQuery(
                        "select * from my_table");

                ResultSetMetaData rsmd=rs.getMetaData();
                int cols=rsmd.getColumnCount();

                while (rs.next()) {
                        for (int col=1; col<=cols; col++) {
                                System.out.print(rs.getString(col)+",");
                        }
                        System.out.println();
                }

                rs.close();
                stmt.close();
                con.close();
        }
}
