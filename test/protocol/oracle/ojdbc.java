// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import java.util.Arrays;
import java.util.Properties;

// ojdbc (thin driver) checks against an oracle protocol listener: the version
// call, and CLOB/BLOB selects at the driver's default lob prefetch size and
// with prefetch set to 0 (length only) and -1 (locator only)
//
// usage: java -cp ojdbc8.jar:. ojdbc port majorversion
class ojdbc {

	static int	failures=0;

	static void check(String label, boolean result, String detail) {
		if (result) {
			System.out.println("  "+label+": success");
		} else {
			System.out.println("  "+label+": failure ("+detail+")");
			failures++;
		}
	}

	static Connection connect(String port, String prefetch)
							throws Exception {
		Properties	props=new Properties();
		props.setProperty("user","testuser");
		props.setProperty("password","testpassword");

		// fail rather than hang if the listener stops answering
		props.setProperty("oracle.jdbc.ReadTimeout","60000");
		if (prefetch!=null) {
			props.setProperty("oracle.jdbc.defaultLobPrefetchSize",
								prefetch);
		}
		return DriverManager.getConnection(
				"jdbc:oracle:thin:@localhost:"+port+":ora1",props);
	}

	// 4000 character pieces, each a letter followed by the alphabet
	static String clobdata(int pieces) {
		StringBuilder	sb=new StringBuilder();
		for (int i=0; i<pieces; i++) {
			sb.append((char)('A'+i));
			for (int j=1; j<4000; j++) {
				sb.append((char)('a'+(j-1)%26));
			}
		}
		return sb.toString();
	}

	// 2000 byte pieces, the nth one all n's
	static byte[] blobdata(int pieces) {
		byte[]	b=new byte[pieces*2000];
		for (int i=0; i<b.length; i++) {
			b[i]=(byte)(i/2000);
		}
		return b;
	}

	// clobdata()'s value, built by the database
	static String clobsql(int pieces) {
		StringBuilder	sb=new StringBuilder();
		for (int i=0; i<pieces; i++) {
			if (i>0) {
				sb.append("||");
			}
			sb.append("to_clob(rpad('"+(char)('A'+i)+
				"',4000,'abcdefghijklmnopqrstuvwxyz'))");
		}
		return sb.toString();
	}

	// PL/SQL that inserts clobdata(pieces/2) and blobdata(pieces)
	static String insertsql(int id, int pieces) {
		return "insert into protocoltestojdbc values "+
				"("+id+","+clobsql(pieces/2)+",empty_blob()) "+
				"returning b into b; "+
			"for i in 0.."+(pieces-1)+" loop "+
				"dbms_lob.writeappend(b,2000,utl_raw.copies("+
				"hextoraw(lpad(to_char(i,'fmXX'),2,'0')),2000)); "+
			"end loop; ";
	}

	static void setup(String port) throws Exception {
		Connection	con=connect(port,null);
		Statement	st=con.createStatement();
		st.execute("begin execute immediate "+
				"'drop table protocoltestojdbc'; "+
				"exception when others then null; end;");
		st.execute("create table protocoltestojdbc "+
				"(id number, c clob, b blob)");

		// at serverversion 12.1 the module never answers ojdbc's plain
		// dml, but dml inside a pl/sql block works (#10701)
		st.execute("declare b blob; begin "+
				insertsql(1,4)+
				insertsql(2,20)+
				"insert into protocoltestojdbc values "+
					"(3,empty_clob(),empty_blob()); "+
				"insert into protocoltestojdbc values "+
					"(4,to_clob('hello'),"+
					"to_blob(hextoraw('0102030405'))); "+
				"commit; end;");
		st.close();
		con.close();
	}

	static void teardown(String port) throws Exception {
		Connection	con=connect(port,null);
		Statement	st=con.createStatement();
		st.execute("drop table protocoltestojdbc");
		st.close();
		con.close();
	}

	static String expectedclob(int id) {
		switch (id) {
			case 1:
				return clobdata(2);
			case 2:
				return clobdata(10);
			case 3:
				return "";
			default:
				return "hello";
		}
	}

	static byte[] expectedblob(int id) {
		switch (id) {
			case 1:
				return blobdata(4);
			case 2:
				return blobdata(20);
			case 3:
				return new byte[0];
			default:
				return new byte[] {1,2,3,4,5};
		}
	}

	static String describe(String s) {
		return (s==null)?"null":"length "+s.length();
	}

	static String describe(byte[] b) {
		return (b==null)?"null":"length "+b.length;
	}

	static void version(String port, String major) throws Exception {
		System.out.println("version:");
		Connection		con=connect(port,null);
		DatabaseMetaData	md=con.getMetaData();
		String	banner=md.getDatabaseProductVersion();
		check("banner",banner!=null &&
				banner.startsWith("Oracle Database ") &&
				banner.contains(" Release "+major+"."),
				"got \""+banner+"\"");
		int	m=md.getDatabaseMajorVersion();
		check("major version",Integer.toString(m).equals(major),
				"got "+m);
		con.close();
	}

	static void lobs(String port, String prefetch) throws Exception {
		System.out.println("lob prefetch "+
				((prefetch==null)?"default":prefetch)+":");
		Connection	con=connect(port,prefetch);
		Statement	st=con.createStatement();

		// all rows, read with getString and getBytes
		ResultSet	rs=st.executeQuery("select id,c,b from "+
					"protocoltestojdbc order by id");
		int	rows=0;
		while (rs.next()) {
			rows++;
			int	id=rs.getInt(1);
			String	c=rs.getString(2);
			byte[]	b=rs.getBytes(3);
			check("row "+id+" clob getString",
				expectedclob(id).equals(c),describe(c));
			check("row "+id+" blob getBytes",
				Arrays.equals(expectedblob(id),b),describe(b));
		}
		check("row count",rows==4,"got "+rows);
		rs.close();

		// all rows again, read through the lob interfaces
		rs=st.executeQuery("select id,c,b from "+
					"protocoltestojdbc order by id");
		rows=0;
		while (rs.next()) {
			rows++;
			int	id=rs.getInt(1);
			Clob	c=rs.getClob(2);
			Blob	b=rs.getBlob(3);
			String	ec=expectedclob(id);
			byte[]	eb=expectedblob(id);
			long	cl=c.length();
			long	bl=b.length();
			check("row "+id+" clob length",cl==ec.length(),
							"got "+cl);
			check("row "+id+" blob length",bl==eb.length,
							"got "+bl);
			if (cl==ec.length()) {
				String	s=c.getSubString(1,(int)cl);
				check("row "+id+" clob getSubString",
						ec.equals(s),describe(s));
			}
			if (bl==eb.length) {
				byte[]	s=b.getBytes(1,(int)bl);
				check("row "+id+" blob getBytes",
						Arrays.equals(eb,s),describe(s));
			}

			// a piece that spans the default 32768 prefetch size
			if (ec.length()>32768) {
				String	s=c.getSubString(32760,20);
				check("row "+id+" clob getSubString at 32760",
					ec.substring(32759,32779).equals(s),
					"got \""+s+"\"");
			}
			if (eb.length>32768) {
				byte[]	s=b.getBytes(32760,20);
				check("row "+id+" blob getBytes at 32760",
					Arrays.equals(Arrays.copyOfRange(
						eb,32759,32779),s),describe(s));
			}
		}
		check("row count",rows==4,"got "+rows);
		rs.close();

		// a query that still works after the lobs
		rs=st.executeQuery("select 1 from dual");
		check("query after lobs",rs.next() && rs.getInt(1)==1,"");
		rs.close();

		st.close();
		con.close();
	}

	public static void main(String args[]) throws Exception {

		Class.forName("oracle.jdbc.OracleDriver");

		String	port=args[0];
		String	major=args[1];

		System.out.println("ojdbc against port "+port+
					", version "+major+":");

		try {
			version(port,major);
		} catch (Exception e) {
			check("version",false,e.toString());
		}
		try {
			setup(port);
		} catch (Exception e) {
			check("setup",false,e.toString());
		}
		String[]	prefetches={null,"0","-1"};
		for (int i=0; i<prefetches.length; i++) {
			try {
				lobs(port,prefetches[i]);
			} catch (Exception e) {
				check("lobs",false,e.toString());
			}
		}
		try {
			teardown(port);
		} catch (Exception e) {
			check("teardown",false,e.toString());
		}

		System.out.println((failures==0)?"success":
					failures+" failures");
		System.exit((failures==0)?0:1);
	}
}
