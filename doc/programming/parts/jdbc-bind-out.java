import java.sql.*;

public class myclass {
        public static void main(String[] args) throws Exception {

                Connection con=DriverManager.getConnection(
                        "jdbc:sqlrelay://user:password@sqlrserver:9000:/tmp/example.socket");

                CallableStatement stmt=con.prepareCall(
                        "{call addAndConvert(?,?,?,?,?,?,?,?)}");
                stmt.setInt(1,10);
                stmt.setInt(2,20);
                stmt.setDouble(3,1.1);
                stmt.setDouble(4,2.2);
                stmt.setInt(5,30);
                stmt.registerOutParameter(6,Types.INTEGER);
                stmt.registerOutParameter(7,Types.DOUBLE);
                stmt.registerOutParameter(8,Types.VARCHAR);
                stmt.execute();

                int result1=stmt.getInt(6);
                double result2=stmt.getDouble(7);
                String result3=stmt.getString(8);

                ... do something with result1, result2, result3 ...

                stmt.close();
                con.close();
        }
}
