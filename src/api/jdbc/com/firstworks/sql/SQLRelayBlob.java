// Copyright (c) David Muse
// See the file COPYING for more information.
package com.firstworks.sql;

import java.sql.Blob;
import java.sql.SQLException;
import java.sql.SQLFeatureNotSupportedException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.ByteArrayInputStream;

// A writable, growable Blob.
//
// Connection.createBlob() must return an empty Blob that the app then fills
// with setBytes(1,...).  SerialBlob is fixed-size and can't grow, so setBytes
// into an empty one throws on strict SerialBlob (JDK 8u471+).  Constructed
// from a byte[], it also serves the result-set getBlob() path, where creating
// a zero-length blob and setBytes(1,...) into it would hit the same problem.
public class SQLRelayBlob implements Blob {

	private byte[]	data;

	public SQLRelayBlob(byte[] b) {
		data=(b==null)?new byte[0]:(byte[])b.clone();
	}

	private void checkFreed() throws SQLException {
		if (data==null) {
			throw new SQLException("blob has been freed");
		}
	}

	public long length() throws SQLException {
		checkFreed();
		return data.length;
	}

	public byte[] getBytes(long pos, int length) throws SQLException {
		checkFreed();
		if (length==0) {
			return new byte[0];
		}
		if (pos<1 || length<0 || pos-1>data.length) {
			throw new SQLException("invalid position or length");
		}
		int	start=(int)(pos-1);
		int	len=length;
		if (start+len>data.length) {
			len=data.length-start;
		}
		byte[]	out=new byte[len];
		System.arraycopy(data,start,out,0,len);
		return out;
	}

	public InputStream getBinaryStream() throws SQLException {
		checkFreed();
		return new ByteArrayInputStream(data);
	}

	public InputStream getBinaryStream(long pos, long length)
						throws SQLException {
		return new ByteArrayInputStream(getBytes(pos,(int)length));
	}

	public long position(byte[] pattern, long start) throws SQLException {
		checkFreed();
		if (start<1) {
			throw new SQLException("invalid start");
		}
		for (int i=(int)(start-1); i+pattern.length<=data.length; i++) {
			boolean	match=true;
			for (int j=0; j<pattern.length; j++) {
				if (data[i+j]!=pattern[j]) {
					match=false;
					break;
				}
			}
			if (match) {
				return i+1;
			}
		}
		return -1;
	}

	public long position(Blob pattern, long start) throws SQLException {
		return position(pattern.getBytes(1,(int)pattern.length()),start);
	}

	public int setBytes(long pos, byte[] bytes) throws SQLException {
		return setBytes(pos,bytes,0,bytes.length);
	}

	public int setBytes(long pos, byte[] bytes, int offset, int len)
						throws SQLException {
		checkFreed();
		if (pos<1) {
			throw new SQLException("invalid position");
		}
		if (bytes==null) {
			throw new SQLException("null bytes");
		}
		int	start=(int)(pos-1);
		// grow if writing past the current end
		if (start+len>data.length) {
			byte[]	grown=new byte[start+len];
			System.arraycopy(data,0,grown,0,data.length);
			data=grown;
		}
		System.arraycopy(bytes,offset,data,start,len);
		return len;
	}

	public OutputStream setBinaryStream(long pos) throws SQLException {
		throw new SQLFeatureNotSupportedException();
	}

	public void truncate(long len) throws SQLException {
		checkFreed();
		if (len<0 || len>data.length) {
			throw new SQLException("invalid length");
		}
		byte[]	truncated=new byte[(int)len];
		System.arraycopy(data,0,truncated,0,(int)len);
		data=truncated;
	}

	public void free() throws SQLException {
		data=null;
	}
}
