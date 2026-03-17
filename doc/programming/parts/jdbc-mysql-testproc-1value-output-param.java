CallableStatement stmt=con.prepareCall("{call exampleproc(?)}");
stmt.registerOutParameter(1,Types.INTEGER);
stmt.execute();
int result=stmt.getInt(1);
stmt.close();
