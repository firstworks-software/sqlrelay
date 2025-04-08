package com.firstworks.sql;

import java.sql.*;

import java.util.Properties;
import java.util.logging.Logger;
import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;

public class SQLRelayDriver implements Driver {

	private static final int	MAJOR_VERSION=1;
	private static final int	MINOR_VERSION=2;

	public static final boolean	debug=true;

	private int	indent=0;

	static {
		try {
			DriverManager.registerDriver(new SQLRelayDriver());
		} catch (SQLException e) {
			e.printStackTrace();
		}
	}

	public SQLRelayDriver() throws SQLException {
		debugFunction();
		debugEnd();
	}

	/**
	 *   Try to make a database connection to the given URL.
	 */
	public Connection connect(String url,
					Properties info)
					throws SQLException {
		debugFunction();
		SQLRelayConnectInfo	ci=parseConnectInfo(url,info);
		Connection	conn=null;
		if (validConnectInfo(ci)) {
			conn=new SQLRelayConnection(ci.host,ci.port,ci.socket,
							ci.user,ci.password,
							ci.retrytime,ci.tries,
							this);
		}
		debugEnd();
		return conn;
	}

	public SQLRelayConnectInfo parseConnectInfo(String url,
						Properties info) {
		debugFunction();

		String	host=null;
		String	portstr=null;
		String	socket=null;
		String	user=null;
		String	password=null;
		String	retrytimestr=null;
		String	triesstr=null;

		// all parameters could be passed in as properties
		if (info!=null) {
			host=info.getProperty("Host");
			portstr=info.getProperty("Port");
			socket=info.getProperty("Socket");
			user=info.getProperty("User");
			if (user==null) {
				user=info.getProperty("user");
			}
			password=info.getProperty("Password");
			if (password==null) {
				password=info.getProperty("password");
			}
			retrytimestr=info.getProperty("Retry Time");
			triesstr=info.getProperty("Tries");
		}

		// override them if they were passed in in the url
		// url format:
		// jdbc:sqlrelay://[user:password@]host[:port][:socket]

		// check for jdbc:sqlrelay://
		if (url.substring(0,16).equals("jdbc:sqlrelay://")) {

			// split the rest of the string on @ and get the parts
			String[]	parts=url.substring(16).split("@");
			String		cred=null;
			String		conn=null;
			if (parts.length==1) {
				conn=parts[0];
			} else if (parts.length>=2) {
				cred=parts[0];
				conn=parts[1];
			}

			// split the cred on : and get the parts
			if (cred!=null) {
				parts=cred.split(":");
				user=parts[0];
				if (parts.length>=2) {
					password=parts[1];
				}
			}

			// split the conn on : and get the parts
			if (conn!=null) {
				parts=conn.split(":");
				if (parts.length>=1) {
					host=parts[0];
				}
				if (parts.length>=2) {
					portstr=parts[1];
				}
				if (parts.length>=3) {
					socket=parts[2];
				}
			}
		}

		// convert port string to integer, default to 9000
		short	port;
		try {
			port=Short.parseShort(portstr);
		} catch (NumberFormatException ex) {
			port=9000;
		}
		if (port<0) {
			port=9000;
		}

		// convert retrytime string to integer, default to 0
		int	retrytime;
		try {
			retrytime=Integer.parseInt(retrytimestr);
		} catch (NumberFormatException ex) {
			retrytime=0;
		}
		if (retrytime<=0) {
			retrytime=0;
		}

		// convert tries string to integer, default to 1
		int	tries=1;
		try {
			tries=Integer.parseInt(triesstr);
		} catch (NumberFormatException ex) {
			tries=1;
		}
		if (tries<=1) {
			tries=1;
		}

		// create, populate, return a SQLRelayConnectInfo
		SQLRelayConnectInfo	ci=new SQLRelayConnectInfo();
		ci.host = host;
		ci.portstr = portstr;
		ci.port = port;
		ci.socket = socket;
		ci.user = user;
		ci.password = password;
		ci.retrytimestr = retrytimestr;
		ci.retrytime = retrytime;
		ci.triesstr = triesstr;
		ci.tries = tries;

		debugPrintln("host: ",host);
		debugPrintln("portstr: ",portstr);
		debugPrintln("port: ",port);
		debugPrintln("socket: ",socket);
		debugPrintln("user: ",user);
		debugPrintln("password: ",password);
		debugPrintln("retrytimestr: ",retrytimestr);
		debugPrintln("retrytime: ",retrytime);
		debugPrintln("triesstr: ",triesstr);
		debugPrintln("tries: ",tries);

		debugEnd();
		return ci;
	}

	private boolean validConnectInfo(SQLRelayConnectInfo ci) {
		debugFunction();
		debugPrintln("host not null: ",ci.host!=null);
		debugPrintln("port > 0: ",ci.port>0);
		debugPrintln("socket not null: ",ci.socket!=null);
		boolean valid=
			((ci.host!=null && ci.port>0) || ci.socket!=null);
		debugPrintln("valid: ",valid);
		debugEnd();
		return valid;
	}

	/**
	 *  Returns true if the driver thinks that it can open a connection to
	 *  the given URL.
	 */
	public boolean acceptsURL(String url) throws SQLException {
		debugFunction();
		boolean	valid=validConnectInfo(parseConnectInfo(url,null));
		debugEnd();
		return valid;
	}

	/**
	 *  The getPropertyInfo method is intended to allow a generic GUI tool
	 *  to discover what properties it should prompt a human for in order
	 *  to get enough information to connect to a database. 
	 */
	public DriverPropertyInfo[] getPropertyInfo(String url,
							Properties info)
							throws SQLException {
		debugFunction();

		SQLRelayConnectInfo	ci=parseConnectInfo(url,info);

		List<DriverPropertyInfo>	dpilist=
					new ArrayList<DriverPropertyInfo>();

		DriverPropertyInfo	dpi;

		if (ci.host==null) {
			dpi=new DriverPropertyInfo("Host","");
			dpi.description="Name of host running SQL Relay.";
			dpi.required=true;
			dpilist.add(dpi);
		}
		if (ci.portstr==null) {
			dpi=new DriverPropertyInfo("Port","");
			dpi.description="Port SQL Relay is listening on.";
			dpi.required=true;
			dpilist.add(dpi);
		}
		if (ci.socket==null) {
			dpi=new DriverPropertyInfo("Socket","");
			dpi.description="Filename of unix socket SQL Relay is "+
						"listening on.";
			dpi.required=false;
			dpilist.add(dpi);
		}
		if (ci.user==null) {
			dpi=new DriverPropertyInfo("User","");
			dpi.description="User name for authentication.";
			dpi.required=true;
			dpilist.add(dpi);
		}
		if (ci.password==null) {
			dpi=new DriverPropertyInfo("Password","");
			dpi.description="Password for authentication.";
			dpi.required=true;
			dpilist.add(dpi);
		}
		if (ci.retrytimestr==null) {
			dpi=new DriverPropertyInfo("Retry Time","0");
			dpi.description=
				"If connection fails, wait this number "+
				"of seconds before trying to connect "+
				"again.";
			dpi.required=false;
			dpilist.add(dpi);
		}
		if (ci.triesstr==null) {
			dpi=new DriverPropertyInfo("Tries","1");
			dpi.description="If connection fails, retry this "+
					"number of times.";
			dpi.required=false;
			dpilist.add(dpi);
		}

		DriverPropertyInfo[]	dpiarray=
			dpilist.toArray(new DriverPropertyInfo[dpilist.size()]);

		debugEnd();
		return dpiarray;
	}

	/**
	 *   Get the driver's major version number.
	 */
	public int getMajorVersion() {
		debugFunction();
		debugEnd();
		return MAJOR_VERSION;
	}

	/**
	 *   Get the driver's minor version number.
	 */
	public int getMinorVersion() {
		debugFunction();
		debugEnd();
		return MINOR_VERSION;
	}

	public Logger getParentLogger() throws SQLFeatureNotSupportedException {
		debugFunction();
		throw new SQLFeatureNotSupportedException();
	}

	/**
	 *  Report whether the Driver is a genuine JDBC COMPLIANT (tm) driver. 
	 */
	public boolean jdbcCompliant() {
		debugFunction();
		debugEnd();
		return false;
	}

	public void debugStart(String str) {
		if (!debug) {
			return;
		}
		debugPrintIndent();
		System.out.println(str + " {");
		indent++;
	}

	public void debugEnd() {
		if (!debug) {
			return;
		}
		indent--;
		debugPrintIndent();
		System.out.println("}");
	}

	public void debugZeroIndent() {
		indent=0;
	}

	private void debugPrintIndent() {
		for (int i=0; i<indent; i++) {
			System.out.print("	");
		}
	}

	public void debugFunction() {
		if (!debug) {
			return;
		}
		StackTraceElement	ste=new Throwable().getStackTrace()[1];
		debugStart(Thread.currentThread().getId()+"|"+
				System.identityHashCode(this)+": "+
				ste.getClassName().substring(
					ste.getClassName().lastIndexOf(".")+1)+
				"."+ste.getMethodName());
	}

	public <T> void debugPrintln(T... elements) {
		if (!debug) {
			return;
		}
		debugPrintIndent();
		for (T e: elements) {
			System.out.print(e);
		}
		System.out.print("\n");
	}

	private static final char[] hexarray="0123456789ABCDEF".toCharArray();
	public void debugPrint(byte[] bytes) {
		if (!debug) {
			return;
		}
		for (int i=0; i<bytes.length; i++) {
			if (bytes[i]>=' ' && bytes[i]<='~') {
				System.out.print((char)bytes[i]);
			} else {
				int	b=bytes[i]&0xff;
				System.out.print(hexarray[b>>>4]);
				System.out.print(hexarray[b&0x0f]);
			}
		}
	}
}
