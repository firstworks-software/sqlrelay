Statement stmt=con.createStatement();
ResultSet rs=stmt.executeQuery("{call exampleproc}");
rs.next();
String result=rs.getString(1);
rs.close();
stmt.close();
