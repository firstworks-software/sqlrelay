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

                for (int i=1; i<=cols; i++) {
                        System.out.println("Name:           "+rsmd.getColumnName(i));
                        System.out.println("Type:           "+rsmd.getColumnTypeName(i));
                        System.out.println("Display Size:   "+rsmd.getColumnDisplaySize(i));
                        System.out.println("Precision:      "+rsmd.getPrecision(i));
                        System.out.println("Scale:          "+rsmd.getScale(i));
                        System.out.println("Nullable:       "+rsmd.isNullable(i));
                        System.out.println("Auto Increment: "+rsmd.isAutoIncrement(i));
                        System.out.println();
                }

                rs.close();
                stmt.close();
                con.close();
        }
}
