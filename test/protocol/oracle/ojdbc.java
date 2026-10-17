// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;
import java.util.Arrays;
import java.util.Properties;
import oracle.jdbc.OracleCallableStatement;
import oracle.jdbc.OracleResultSet;
import oracle.sql.BFILE;

// ojdbc (thin driver) checks against an oracle protocol listener: the version
// call, plain dml update counts, batches and transactions, failed statements
// past the instance's maxcursors, and CLOB/BLOB selects and BFILE directory
// alias and file name lookups at the driver's default lob prefetch size and
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

	// a name of the given length, the prefix followed by the alphabet
	static String longname(String prefix, int length) {
		StringBuilder	sb=new StringBuilder(prefix);
		while (sb.length()<length) {
			sb.append((char)('a'+(sb.length()%26)));
		}
		return sb.toString();
	}

	// a long directory alias and the longest file name a BFILE takes,
	// which push the locator past 252 bytes
	static final String	longalias=longname("ALIAS",30).toUpperCase();
	static final String	longfile=longname("file",255);

	static String expectedalias(int id) {
		return (id==1)?"DMP_DIR":longalias;
	}

	static String expectedfile(int id) {
		return (id==1)?"protocoltest.txt":longfile;
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
		st.execute("begin execute immediate "+
				"'drop table protocoltestojdbcfail'; "+
				"exception when others then null; end;");
		st.execute("create table protocoltestojdbcfail "+
				"(id number primary key)");
		st.executeUpdate("insert into protocoltestojdbcfail values (1)");
		st.execute("begin execute immediate "+
				"'drop table protocoltestojdbcbf'; "+
				"exception when others then null; end;");
		st.execute("create table protocoltestojdbcbf "+
				"(id number, f bfile)");

		st.execute("begin execute immediate "+
				"'drop table protocoltestojdbcnull'; "+
				"exception when others then null; end;");
		st.execute("create table protocoltestojdbcnull "+
				"(id number, c clob, b blob, f bfile)");
		check("null lob row insert",st.executeUpdate(
				"insert into protocoltestojdbcnull values "+
				"(1,null,null,null)")==1,"");
		check("not null lob row insert",st.executeUpdate(
				"insert into protocoltestojdbcnull values "+
				"(2,to_clob('x'),to_blob(hextoraw('01')),"+
				"bfilename('"+expectedalias(1)+"','"+
				expectedfile(1)+"'))")==1,"");

		st.execute("begin execute immediate "+
				"'drop table protocoltestojdbcnclob'; "+
				"exception when others then null; end;");
		st.execute("create table protocoltestojdbcnclob "+
				"(id number, n nclob)");
		int[]	nclobpieces={2,10};
		for (int i=0; i<nclobpieces.length; i++) {
			check("nclob row "+(i+1)+" insert",st.executeUpdate(
				"insert into protocoltestojdbcnclob values ("+
				(i+1)+","+clobsql(nclobpieces[i]).
				replace("to_clob","to_nclob")+")")==1,"");
		}
		check("nclob row 3 insert",st.executeUpdate(
				"insert into protocoltestojdbcnclob values "+
				"(3,empty_clob())")==1,"");
		check("nclob row 4 insert",st.executeUpdate(
				"insert into protocoltestojdbcnclob values "+
				"(4,to_nclob('hello'))")==1,"");
		check("nclob row 5 insert",st.executeUpdate(
				"insert into protocoltestojdbcnclob values "+
				"(5,null)")==1,"");

		check("row 1 insert",insertlobs(con,1,4)==1,"");
		check("row 2 insert",insertlobs(con,2,20)==1,"");
		check("row 3 insert",st.executeUpdate(
				"insert into protocoltestojdbc values "+
				"(3,empty_clob(),empty_blob())")==1,"");
		check("row 4 insert",st.executeUpdate(
				"insert into protocoltestojdbc values "+
				"(4,to_clob('hello'),"+
				"to_blob(hextoraw('0102030405')))")==1,"");

		// bfilename() checks neither name, so the directory and
		// the file need not exist
		for (int id=1; id<=2; id++) {
			check("bfile row "+id+" insert",st.executeUpdate(
				"insert into protocoltestojdbcbf values "+
				"("+id+",bfilename('"+expectedalias(id)+"','"+
				expectedfile(id)+"'))")==1,"");
		}
		st.close();
		con.close();
	}

	static void teardown(String port) throws Exception {
		Connection	con=connect(port,null);
		Statement	st=con.createStatement();
		st.execute("drop table protocoltestojdbc");
		st.execute("drop table protocoltestojdbcdml");
		st.execute("drop table protocoltestojdbcfail");
		st.execute("drop table protocoltestojdbcbf");
		st.execute("drop table protocoltestojdbcnull");
		st.execute("drop table protocoltestojdbcnclob");
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

		// one PreparedStatement re-executed with new values, singly and
		// as a batch - ojdbc re-executes with no bind descriptors, only
		// the new row data (#10705)
		ps=con.prepareStatement(
				"insert into protocoltestojdbcdml values (?,?)");
		ps.setInt(1,60);
		ps.setString(2,"q");
		updatecount=ps.executeUpdate();
		check("reuse first insert",updatecount==1,"got "+updatecount);
		ps.setInt(1,61);
		ps.setString(2,"q");
		updatecount=ps.executeUpdate();
		check("reuse second insert",updatecount==1,"got "+updatecount);
		for (int i=62; i<65; i++) {
			ps.setInt(1,i);
			ps.setString(2,"q");
			ps.addBatch();
		}
		checkbatch("reuse batch insert",ps.executeBatch(),
						new int[] {1,1,1},major);
		ps.setInt(1,65);
		ps.setString(2,"q");
		updatecount=ps.executeUpdate();
		check("reuse insert after batch",updatecount==1,
							"got "+updatecount);
		ps.close();
		rows=count(con,"v='q'");
		check("reuse rows",rows==6,"got "+rows);
		for (int i=60; i<66; i++) {
			rows=count(con,"id="+i);
			check("reuse row "+i,rows==1,"got "+rows);
		}
		updatecount(st,"delete reuse rows",
			"delete from protocoltestojdbcdml where v='q'",6);

		// the same with a CallableStatement and an out parameter: the
		// re-executes carry the new in value and expect the out value
		// back with no io vector (#10705, #10692)
		CallableStatement	cs=con.prepareCall(
					"begin ? := ? * 2; end;");
		cs.registerOutParameter(1,Types.INTEGER);
		for (int i=1; i<=3; i++) {
			cs.setInt(2,i*10);
			cs.execute();
			check("reuse callable "+i,cs.getInt(1)==i*20,
							"got "+cs.getInt(1));
		}
		cs.close();

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

	// the suite's instances leave maxcursors at its default
	static final int	MAXCURSORS=5;
	static final int	FAILURES=MAXCURSORS+3;

	static final String	BADTABLE="select * from protocoltestojdbcnone";

	// the error codes of a run of failures, against the one expected
	static class errors {
		String	label;
		int	expected;
		int	right=0;
		String	wrong=null;

		errors(String label, int expected) {
			this.label=label;
			this.expected=expected;
		}

		void got(SQLException e) {
			if (e.getErrorCode()==expected) {
				right++;
			} else if (wrong==null) {
				wrong=e.toString();
			}
		}

		void none() {
			if (wrong==null) {
				wrong="no error";
			}
		}

		void check(int count) {
			ojdbc.check(label+" ORA-"+String.format("%05d",expected),
				right==count,
				right+" of "+count+((wrong!=null)?
					", first other: "+wrong:""));
		}
	}

	static void goodquery(Connection con, String label) {
		try {
			Statement	st=con.createStatement();
			ResultSet	rs=st.executeQuery("select 1 from dual");
			check(label,rs.next() && rs.getInt(1)==1,"no row");
			rs.close();
			st.close();
		} catch (SQLException e) {
			check(label,false,e.toString());
		}
	}

	// runs badtable on a new Statement that is left open, and returns it
	static Statement leaveopen(Connection con, errors e) {
		Statement	st=null;
		try {
			st=con.createStatement();
			st.executeQuery(BADTABLE);
			e.none();
		} catch (SQLException ex) {
			e.got(ex);
		}
		return st;
	}

	static void close(Statement[] sts) {
		for (int i=0; i<sts.length; i++) {
			try {
				if (sts[i]!=null) {
					sts[i].close();
				}
			} catch (SQLException e) {
				check("close",false,e.toString());
			}
		}
	}

	// more failures of each kind than maxcursors, with a query on the
	// same connection after each kind.  only a statement left open holds
	// a cursor, as on a real server.
	// ojdbc writes the query text with no length byte in front of it, so a
	// statement whose first character is the same as its length once had
	// that character eaten: a 98 character block (begin, 'b' is 98), a 100
	// character declare, a 115 character select, and so on (#10708)
	static void lengths(String port) throws Exception {
		System.out.println("statement lengths:");
		Connection	con=connect(port,null);
		Statement	st=con.createStatement();
		String[]	starts={"begin null;","declare x number; begin "+
					"x:=1;","select 1 from dual","  select "+
					"1 from dual","commit","update "+
					"protocoltestojdbcdml set v=v where 1=0"};
		String[]	ends={" end;"," end;",""," ","",""};
		int		bad=0;
		for (int i=0; i<starts.length; i++) {
			for (int n=60; n<=130; n++) {
				StringBuilder	sb=new StringBuilder(starts[i]);
				while (sb.length()<n-ends[i].length()) {
					sb.append(' ');
				}
				sb.append(ends[i]);
				String	sql=sb.toString();
				if (sql.length()!=n) {
					continue;
				}
				try {
					st.execute(sql);
				} catch (Exception e) {
					System.out.println("  "+n+" characters, "+
							starts[i]+": "+
							e.getMessage().trim());
					bad++;
				}
			}
		}
		check("statement lengths 60 to 130",bad==0,bad+" failed");
		con.close();
	}

	static void failed(String port) throws Exception {
		System.out.println("failed statements:");
		Connection	con=connect(port,null);

		// a new Statement per failure, closed
		errors	e=new errors("closed statements",942);
		for (int i=0; i<FAILURES; i++) {
			Statement	st=con.createStatement();
			try {
				st.executeQuery(BADTABLE);
				e.none();
			} catch (SQLException ex) {
				e.got(ex);
			}
			st.close();
		}
		e.check(FAILURES);
		goodquery(con,"query after closed statements");

		// one Statement, reused
		e=new errors("reused statement",942);
		Statement	st=con.createStatement();
		for (int i=0; i<FAILURES; i++) {
			try {
				st.executeQuery(BADTABLE);
				e.none();
			} catch (SQLException ex) {
				e.got(ex);
			}
		}
		e.check(FAILURES);
		goodquery(con,"query after reused statement");
		st.close();

		// PreparedStatements that fail to parse
		e=new errors("prepared bad table",942);
		for (int i=0; i<FAILURES; i++) {
			PreparedStatement	ps=con.prepareStatement(
						BADTABLE+" where id=?");
			try {
				ps.setInt(1,i);
				ps.executeQuery();
				e.none();
			} catch (SQLException ex) {
				e.got(ex);
			}
			ps.close();
		}
		e.check(FAILURES);
		goodquery(con,"query after prepared bad table");
		e=new errors("prepared bad statement",900);
		for (int i=0; i<FAILURES; i++) {
			PreparedStatement	ps=con.prepareStatement(
						"bogus statement");
			try {
				ps.execute();
				e.none();
			} catch (SQLException ex) {
				e.got(ex);
			}
			ps.close();
		}
		e.check(FAILURES);
		goodquery(con,"query after prepared bad statement");

		// dml that fails on execute
		e=new errors("duplicate insert",1);
		for (int i=0; i<FAILURES; i++) {
			st=con.createStatement();
			try {
				st.executeUpdate("insert into "+
					"protocoltestojdbcfail values (1)");
				e.none();
			} catch (SQLException ex) {
				e.got(ex);
			}
			st.close();
		}
		e.check(FAILURES);
		goodquery(con,"query after duplicate insert");

		// selects that fail on fetch
		e=new errors("fetch error",1476);
		for (int i=0; i<FAILURES; i++) {
			st=con.createStatement();
			try {
				ResultSet	rs=st.executeQuery(
						"select 1/0 from dual");
				while (rs.next()) {
				}
				e.none();
			} catch (SQLException ex) {
				e.got(ex);
			}
			st.close();
		}
		e.check(FAILURES);
		goodquery(con,"query after fetch error");

		// batches whose second statement fails
		e=new errors("failed batch",1);
		for (int i=0; i<FAILURES; i++) {
			PreparedStatement	ps=con.prepareStatement(
				"insert into protocoltestojdbcfail values (?)");
			try {
				ps.setInt(1,100+i);
				ps.addBatch();
				ps.setInt(1,1);
				ps.addBatch();
				ps.executeBatch();
				e.none();
			} catch (SQLException ex) {
				e.got(ex);
			}
			ps.close();
		}
		e.check(FAILURES);
		goodquery(con,"query after failed batch");

		// failed statements left open, within maxcursors, then closed
		e=new errors("open statements",942);
		Statement[]	open=new Statement[MAXCURSORS-1];
		for (int i=0; i<open.length; i++) {
			open[i]=leaveopen(con,e);
		}
		e.check(open.length);
		goodquery(con,"query with open statements");
		close(open);
		goodquery(con,"query after closing open statements");

		// failed statements left open, past maxcursors, then closed
		e=new errors("open statements within maxcursors",942);
		errors	past=new errors("open statement past maxcursors",1000);
		open=new Statement[MAXCURSORS+1];
		for (int i=0; i<open.length; i++) {
			open[i]=leaveopen(con,(i<MAXCURSORS)?e:past);
		}
		e.check(MAXCURSORS);
		past.check(1);
		close(open);
		goodquery(con,"query after closing open statements "+
							"past maxcursors");

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

			// the lobs bound to a statement stand for their data
			// (#10706)
			CallableStatement	cs=con.prepareCall(
					"begin ? := dbms_lob.getlength(?); "+
					"end;");
			cs.registerOutParameter(1,Types.INTEGER);
			cs.setClob(2,c);
			cs.execute();
			check("row "+id+" bound clob length",
				cs.getInt(1)==expectedclob(id).length(),
				"got "+cs.getInt(1));
			cs.close();
			cs=con.prepareCall("begin ? := dbms_lob.getlength(?); "+
					"end;");
			cs.registerOutParameter(1,Types.INTEGER);
			cs.setBlob(2,b);
			cs.execute();
			check("row "+id+" bound blob length",
				cs.getInt(1)==expectedblob(id).length,
				"got "+cs.getInt(1));
			cs.close();
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

	static void checknames(String label, BFILE f, int id) throws Exception {
		String	alias=f.getDirAlias();
		String	name=f.getName();
		check(label+" getDirAlias",expectedalias(id).equals(alias),
							"got \""+alias+"\"");
		check(label+" getName",expectedfile(id).equals(name),
							"got \""+name+"\"");
	}

	static void bfiles(String port, String prefetch) throws Exception {
		System.out.println("bfile lob prefetch "+
				((prefetch==null)?"default":prefetch)+":");
		Connection	con=connect(port,prefetch);
		Statement	st=con.createStatement();

		// the names of each row while it is the current one
		ResultSet	rs=st.executeQuery("select id,f from "+
					"protocoltestojdbcbf order by id");
		BFILE	first=null;
		int	rows=0;
		while (rs.next()) {
			rows++;
			int	id=rs.getInt(1);
			BFILE	f=((OracleResultSet)rs).getBFILE(2);
			if (id==1) {
				first=f;
			}
			checknames("row "+id,f,id);

			// the file isn't there, and the lob operations on it
			// say so - exists and is open are answered, and an
			// open is refused the way a real server refuses it
			// (#10699)
			check("row "+id+" fileExists",!f.fileExists(),"");
			check("row "+id+" isFileOpen",!f.isFileOpen(),"");
			try {
				f.openFile();
				check("row "+id+" openFile",false,
							"no exception");
			} catch (SQLException e) {
				check("row "+id+" openFile error",
					e.getMessage().contains("ORA-22288"),
					e.getMessage());
			}
			check("row "+id+" isFileOpen after open",
							!f.isFileOpen(),"");
		}
		check("row count",rows==2,"got "+rows);

		// row 1's names, read after fetching past it
		if (first!=null) {
			checknames("row 1 after the fetch",first,1);
		}
		rs.close();

		// fileGetName, called directly with each row's locator, in the
		// same text that getDirAlias() and getName() send
		rs=st.executeQuery("select id,f from "+
					"protocoltestojdbcbf order by id");
		while (rs.next()) {
			int	id=rs.getInt(1);
			BFILE	f=((OracleResultSet)rs).getBFILE(2);
			OracleCallableStatement	cs=
				(OracleCallableStatement)con.prepareCall(
					"begin sys.dbms_lob.fileGetName"+
					"(:1, :2, :3); end;");
			cs.setBFILE(1,f);
			cs.registerOutParameter(2,Types.VARCHAR);
			cs.registerOutParameter(3,Types.VARCHAR);
			cs.execute();
			String	alias=cs.getString(2);
			String	name=cs.getString(3);
			check("row "+id+" fileGetName directory alias",
					expectedalias(id).equals(alias),
					"got \""+alias+"\"");
			check("row "+id+" fileGetName file name",
					expectedfile(id).equals(name),
					"got \""+name+"\"");
			cs.close();
		}
		rs.close();

		// the same call in other spellings (ojdbc rewrites ? as ":1 ",
		// and a hand-written one can differ in case, whitespace, bind
		// names and the sys. prefix), each on one statement reused for
		// both rows (#10709)
		String[]	spellings={
			"begin sys.dbms_lob.fileGetName(:1, :2, :3); end;",
			"begin sys.dbms_lob.fileGetName(?,?,?); end;",
			"begin dbms_lob.filegetname(?,?,?); end;",
			"BEGIN   SYS.DBMS_LOB.FILEGETNAME( :a , :b , :c ) ;"+
								" END ;\n"
		};
		for (int i=0; i<spellings.length; i++) {
			OracleCallableStatement	cs=
				(OracleCallableStatement)con.prepareCall(
								spellings[i]);
			cs.registerOutParameter(2,Types.VARCHAR);
			cs.registerOutParameter(3,Types.VARCHAR);
			rs=st.executeQuery("select id,f from "+
					"protocoltestojdbcbf order by id");
			while (rs.next()) {
				int	id=rs.getInt(1);
				cs.setBFILE(1,((OracleResultSet)rs).getBFILE(2));
				cs.execute();
				String	alias=cs.getString(2);
				String	name=cs.getString(3);
				check("spelling "+i+" row "+id+" names",
					expectedalias(id).equals(alias) &&
					expectedfile(id).equals(name),
					"got \""+alias+"\", \""+name+"\"");
			}
			rs.close();
			cs.close();
		}

		// a query that still works after the bfiles
		rs=st.executeQuery("select 1 from dual");
		check("query after bfiles",rs.next() && rs.getInt(1)==1,"");
		rs.close();

		st.close();
		con.close();
	}

	// an nclob describes as one and its data reads back whole
	static void nclobs(String port, String prefetch) throws Exception {
		System.out.println("nclobs, lob prefetch "+
				((prefetch==null)?"default":prefetch)+":");
		Connection	con=connect(port,prefetch);
		Statement	st=con.createStatement();
		ResultSet	rs=st.executeQuery("select id,n from "+
					"protocoltestojdbcnclob order by id");
		String	type=rs.getMetaData().getColumnTypeName(2);
		check("nclob column type","NCLOB".equals(type),"got "+type);
		int	rows=0;
		while (rs.next()) {
			rows++;
			int	id=rs.getInt(1);
			String	ec=(id==5)?null:expectedclob(id);
			String	s=rs.getString(2);
			check("nclob row "+id+" getString",
				(ec==null)?(s==null):ec.equals(s),
				describe(s));
			Clob	c=rs.getClob(2);
			if (ec==null) {
				check("nclob row "+id+" getClob",c==null,"");
				continue;
			}
			long	cl=c.length();
			check("nclob row "+id+" length",cl==ec.length(),
							"got "+cl);
			if (cl==ec.length() && cl>0) {
				String	sub=c.getSubString(1,(int)cl);
				check("nclob row "+id+" getSubString",
					ec.equals(sub),describe(sub));
			}
			if (ec.length()>32768) {
				String	sub=c.getSubString(32760,20);
				check("nclob row "+id+" getSubString at 32760",
					ec.substring(32759,32779).equals(sub),
					"got \""+sub+"\"");
			}
		}
		check("nclob row count",rows==5,"got "+rows);
		rs.close();
		st.close();
		con.close();
	}

	// a null lob column reads as null, not as a lob that is empty
	static void nulllobs(String port, String prefetch) throws Exception {
		System.out.println("null lobs, lob prefetch "+
				((prefetch==null)?"default":prefetch)+":");
		Connection	con=connect(port,prefetch);
		Statement	st=con.createStatement();
		ResultSet	rs=st.executeQuery("select id,c,b,f from "+
					"protocoltestojdbcnull order by id");
		int	rows=0;
		while (rs.next()) {
			rows++;
			int	id=rs.getInt(1);
			String	c=rs.getString(2);
			boolean	cnull=rs.wasNull();
			byte[]	b=rs.getBytes(3);
			boolean	bnull=rs.wasNull();
			Clob	cl=rs.getClob(2);
			Blob	bl=rs.getBlob(3);
			if (id==1) {
				check("null row clob getString",
					c==null && cnull,describe(c));
				check("null row blob getBytes",
					b==null && bnull,describe(b));
				check("null row getClob",cl==null,"");
				check("null row getBlob",bl==null,"");
			} else {
				check("not null row clob getString",
					"x".equals(c) && !cnull,describe(c));
				check("not null row blob getBytes",
					b!=null && b.length==1 && b[0]==1,
					describe(b));
			}
		}
		check("null lob row count",rows==2,"got "+rows);
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
		try {
			lengths(port);
		} catch (Exception e) {
			check("statement lengths",false,e.toString());
		}
		try {
			failed(port);
		} catch (Exception e) {
			check("failed statements",false,e.toString());
		}
		String[]	prefetches={null,"0","-1"};
		for (int i=0; i<prefetches.length; i++) {
			try {
				lobs(port,prefetches[i]);
			} catch (Exception e) {
				check("lobs",false,e.toString());
			}
			try {
				nclobs(port,prefetches[i]);
			} catch (Exception e) {
				check("nclobs",false,e.toString());
			}
			try {
				nulllobs(port,prefetches[i]);
			} catch (Exception e) {
				check("null lobs",false,e.toString());
			}
			try {
				bfiles(port,prefetches[i]);
			} catch (Exception e) {
				check("bfiles",false,e.toString());
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
