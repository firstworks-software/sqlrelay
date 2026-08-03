// Copyright (c) David Muse
// See the file COPYING for more information.
package com.firstworks.sql;

import java.sql.Clob;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.io.Reader;
import java.io.InputStream;
import java.io.StringReader;
import java.io.ByteArrayInputStream;
import java.io.OutputStream;
import java.io.Writer;
import java.nio.charset.StandardCharsets;

// Connection.createClob() must return an empty Clob that the app then fills
// with setString(1,...), but SerialClob is fixed-size, so that throws on
// strict SerialClob (JDK 8u471+).  This Clob grows instead.
public class SQLRelayClob implements Clob {

	private StringBuffer	buf;

	public SQLRelayClob() {
		buf=new StringBuffer();
	}

	private void checkFreed() throws SQLException {
		if (buf==null) {
			throw new SQLException("clob has been freed");
		}
	}

	public long length() throws SQLException {
		checkFreed();
		return buf.length();
	}

	public String getSubString(long pos, int length) throws SQLException {
		checkFreed();
		if (pos<1 || length<0 || pos-1>buf.length()) {
			throw new SQLException("invalid position or length");
		}
		int	start=(int)(pos-1);
		int	end=start+length;
		if (end>buf.length()) {
			end=buf.length();
		}
		return buf.substring(start,end);
	}

	public Reader getCharacterStream() throws SQLException {
		checkFreed();
		return new StringReader(buf.toString());
	}

	public Reader getCharacterStream(long pos, long length)
						throws SQLException {
		return new StringReader(getSubString(pos,(int)length));
	}

	public InputStream getAsciiStream() throws SQLException {
		checkFreed();
		return new ByteArrayInputStream(
			buf.toString().getBytes(StandardCharsets.US_ASCII));
	}

	public long position(String searchstr, long start) throws SQLException {
		checkFreed();
		if (start<1) {
			throw new SQLException("invalid start");
		}
		int	index=buf.indexOf(searchstr,(int)(start-1));
		return (index<0)?-1:(index+1);
	}

	public long position(Clob searchstr, long start) throws SQLException {
		return position(searchstr.getSubString(
					1,(int)searchstr.length()),start);
	}

	public int setString(long pos, String str) throws SQLException {
		checkFreed();
		if (pos<1) {
			throw new SQLException("invalid position");
		}
		if (str==null) {
			throw new SQLException("null string");
		}
		int	start=(int)(pos-1);
		// pad with spaces if writing past the current end
		while (buf.length()<start) {
			buf.append(' ');
		}
		int	end=start+str.length();
		if (end>buf.length()) {
			end=buf.length();
		}
		buf.replace(start,end,str);
		return str.length();
	}

	public int setString(long pos, String str, int offset, int len)
						throws SQLException {
		if (str==null) {
			throw new SQLException("null string");
		}
		return setString(pos,str.substring(offset,offset+len));
	}

	public OutputStream setAsciiStream(long pos) throws SQLException {
		throw new SQLFeatureNotSupportedException();
	}

	public Writer setCharacterStream(long pos) throws SQLException {
		throw new SQLFeatureNotSupportedException();
	}

	public void truncate(long len) throws SQLException {
		checkFreed();
		if (len<0 || len>buf.length()) {
			throw new SQLException("invalid length");
		}
		buf.setLength((int)len);
	}

	public void free() throws SQLException {
		buf=null;
	}
}
