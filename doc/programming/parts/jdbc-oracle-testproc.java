PreparedStatement stmt=con.prepareStatement("begin exampleproc(?,?,?); end;");
stmt.setInt(1,1);
stmt.setDouble(2,1.1);
stmt.setString(3,"hello");
stmt.executeUpdate();
stmt.close();
