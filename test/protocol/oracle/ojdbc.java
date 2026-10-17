// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import java.util.Arrays;
import java.util.Properties;

// ojdbc (thin driver) checks against an oracle protocol listener: the version
// call, plain dml update counts, batches and transactions, and CLOB/BLOB
// selects at the driver's default lob prefetch size and with prefetch set to
// 0 (length only) and -1 (locator only)
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

	// inserts clobdata(pieces/2), built by the database, and
	// blobdata(pieces), bound, and returns the update count
	static int insertlobs(Connection con, int id, int pieces)
							throws Exception {
		PreparedStatement	ps=con.prepareStatement(
				"insert into protocoltestojdbc values "+
				"(?,"+clobsql(pieces/2)+",?)");
		ps.setInt(1,id);
		ps.setBytes(2,blobdata(pieces));
		int	result=ps.executeUpdate();
		ps.close();
		return result;
	}

	static void setup(String port) throws Exception {
		System.out.println("setup:");
		Connection	con=connect(port,null);
		Statement	st=con.createStatement();
		st.execute("begin execute immediate "+
				"'drop table protocoltestojdbc'; "+
				"exception when others then null; end;");
		st.execute("create table protocoltestojdbc "+
				"(id number, c clob, b blob)");
		st.execute("begin execute immediate "+
				"'drop table protocoltestojdbcdml'; "+
				"exception when others then null; end;");
		st.execute("create table protocoltestojdbcdml "+
				"(id number, v varchar2(20))");

		check("row 1 insert",insertlobs(con,1,4)==1,"");
		check("row 2 insert",insertlobs(con,2,20)==1,"");
		check("row 3 insert",st.executeUpdate(
				"insert into protocoltestojdbc values "+
				"(3,empty_clob(),empty_blob())")==1,"");
		check("row 4 insert",st.executeUpdate(
				"insert into protocoltestojdbc values "+
				"(4,to_clob('hello'),"+
				"to_blob(hextoraw('0102030405')))")==1,"");
		st.close();
		con.close();
	}

	static void teardown(String port) throws Exception {
		Connection	con=connect(port,null);
		Statement	st=con.createStatement();
		st.execute("drop table protocoltestojdbc");
		st.execute("drop table protocoltestojdbcdml");
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

	static void updatecount(Statement st, String label, String sql,
							int expected) throws Exception {
		int	result=st.executeUpdate(sql);
		check(label,result==expected,"got "+result);
	}

	static int count(Connection con, String where) throws Exception {
		Statement	st=con.createStatement();
		ResultSet	rs=st.executeQuery("select count(*) from "+
					"protocoltestojdbcdml where "+where);
		rs.next();
		int	result=rs.getInt(1);
		rs.close();
		st.close();
		return result;
	}

	// executeBatch() gets a row count per statement only from 12.1 on,
	// an older server gives SUCCESS_NO_INFO (-2) for each
	static void checkbatch(String label, int[] result,
					int[] expected, String major) {
		if (Integer.parseInt(major)<12) {
			expected=new int[expected.length];
			Arrays.fill(expected,Statement.SUCCESS_NO_INFO);
		}
		check(label,Arrays.equals(expected,result),
					"got "+Arrays.toString(result));
	}

	static void dml(String port, String major) throws Exception {
		System.out.println("dml:");
		Connection	con=connect(port,null);
		Statement	st=con.createStatement();

		// executeUpdate
		updatecount(st,"insert",
			"insert into protocoltestojdbcdml values (1,'a')",1);
		updatecount(st,"insert select",
			"insert into protocoltestojdbcdml "+
			"select level+1,'a' from dual connect by level<=4",4);
		updatecount(st,"update",
			"update protocoltestojdbcdml set v='b' where id<=3",3);
		updatecount(st,"delete 0 rows",
			"delete from protocoltestojdbcdml where id=99",0);
		updatecount(st,"delete 2 rows",
			"delete from protocoltestojdbcdml where id>=4",2);

		// execute and getUpdateCount
		boolean	result=st.execute(
			"update protocoltestojdbcdml set v='c' where id>=2");
		int	updatecount=st.getUpdateCount();
		check("execute update",!result && updatecount==2,
				"got "+result+", update count "+updatecount);

		// prepared executeUpdate
		PreparedStatement	ps=con.prepareStatement(
				"insert into protocoltestojdbcdml values (?,?)");
		ps.setInt(1,10);
		ps.setString(2,"p");
		updatecount=ps.executeUpdate();
		check("prepared insert",updatecount==1,"got "+updatecount);
		ps.close();

		// executeBatch
		ps=con.prepareStatement(
				"insert into protocoltestojdbcdml values (?,?)");
		for (int i=20; i<23; i++) {
			ps.setInt(1,i);
			ps.setString(2,"x");
			ps.addBatch();
		}
		checkbatch("batch insert",ps.executeBatch(),
						new int[] {1,1,1},major);
		ps.close();
		ps=con.prepareStatement(
			"update protocoltestojdbcdml set v=? where id>=?");
		ps.setString(1,"x");
		ps.setInt(2,10);
		ps.addBatch();
		ps.setString(1,"y");
		ps.setInt(2,22);
		ps.addBatch();
		ps.setString(1,"z");
		ps.setInt(2,99);
		ps.addBatch();
		checkbatch("batch update",ps.executeBatch(),
						new int[] {4,1,0},major);
		ps.close();
		int	rows=count(con,"v='x'");
		check("batch update rows x",rows==3,"got "+rows);
		rows=count(con,"v='y'");
		check("batch update rows y",rows==1,"got "+rows);

		// rollback
		con.setAutoCommit(false);
		updatecount(st,"insert before rollback",
			"insert into protocoltestojdbcdml values (30,'r')",1);
		con.rollback();
		rows=count(con,"id=30");
		check("rolled back",rows==0,"got "+rows);

		// commit, with a second session that sees the row only after
		updatecount(st,"insert before commit",
			"insert into protocoltestojdbcdml values (31,'c')",1);
		Connection	other=connect(port,null);
		rows=count(other,"id=31");
		check("not visible before commit",rows==0,"got "+rows);
		con.commit();
		rows=count(other,"id=31");
		check("visible after commit",rows==1,"got "+rows);
		other.close();
		con.setAutoCommit(true);

		rows=count(con,"1=1");
		check("row count",rows==8,"got "+rows);

		st.close();
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
		try {
			dml(port,major);
		} catch (Exception e) {
			check("dml",false,e.toString());
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
