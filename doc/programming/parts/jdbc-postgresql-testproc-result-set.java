Statement stmt=con.createStatement();
ResultSet rs=stmt.executeQuery("select * from examplefunc() as (col1 int, col2 float, col3 char(40))");
while (rs.next()) {
        String col1=rs.getString(1);
        String col2=rs.getString(2);
        String col3=rs.getString(3);
        ... process col1, col2, col3 ...
}
rs.close();
stmt.close();
