CallableStatement stmt=con.prepareCall("begin exampleproc(?,?,?,?); end;");
stmt.setInt(1,1);
stmt.setDouble(2,1.1);
stmt.setString(3,"hello");
stmt.registerOutParameter(4,Types.INTEGER);
stmt.execute();
int result=stmt.getInt(4);
stmt.close();
