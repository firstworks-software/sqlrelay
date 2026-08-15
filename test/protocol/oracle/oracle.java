// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;

class oracle {

	public static void main(String args[]) throws Exception {

		Class.forName("oracle.jdbc.OracleDriver");

		// port defaults to 1522 (what the existing jdbc*test scripts
		// expect); an optional first arg overrides it, so a script
		// can point this at an oraproxy instance listening on some
		// other port
		String	port=(args.length>0)?args[0]:"1522";

		Connection	con=DriverManager.getConnection(
					"jdbc:oracle:thin:@localhost:"+port+
						":ora1",
					"testuser","testpassword");

		Statement	stmt=con.createStatement();

		ResultSet	rs=stmt.executeQuery("select 1 from dual");
		rs.next();
		rs.getInt(1);
		rs.close();

		stmt.close();

		con.close();

		System.exit(0);
	}
}
