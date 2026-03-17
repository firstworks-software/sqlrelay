CallableStatement stmt=con.prepareCall("{call exampleproc(?,?,?)}");
stmt.setInt(1,1);
stmt.setDouble(2,1.1);
stmt.setString(3,"hello");
stmt.execute();
stmt.close();
