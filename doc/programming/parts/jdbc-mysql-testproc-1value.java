PreparedStatement stmt=con.prepareStatement("select examplefunc(?,?,?)");
stmt.setInt(1,1);
stmt.setDouble(2,1.1);
stmt.setString(3,"hello");
ResultSet rs=stmt.executeQuery();
rs.next();
String result=rs.getString(1);
rs.close();
stmt.close();
