CallableStatement stmt=con.prepareCall("{call exampleproc(?,?,?)}");
stmt.registerOutParameter(1,Types.INTEGER);
stmt.registerOutParameter(2,Types.DOUBLE);
stmt.registerOutParameter(3,Types.VARCHAR);
stmt.execute();
int out1=stmt.getInt(1);
double out2=stmt.getDouble(2);
String out3=stmt.getString(3);
stmt.close();
