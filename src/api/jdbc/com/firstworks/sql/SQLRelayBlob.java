// Copyright (c) David Muse
// See the file COPYING for more information.
package com.firstworks.sql;

import java.sql.*;
import javax.sql.rowset.serial.*;

public class SQLRelayBlob extends SerialBlob {

	public SQLRelayBlob(byte[] b) throws SQLException {
		super(b);
	}

	public byte[] getBytes(long pos, int length) throws SerialException {

		// SerialBlob doesn't like for you to call getBytes(1,0)
		// on a 0-length blob.
		//
		// Specifically, it throws the error:
		// position cannot be less than 1 or greater than the length of
		// the SerialBlob.
		//
		// We need that case to return new byte[0], so we'll just
		// return new byte[0] in any case where length==0.
		if (length==0) {
			return new byte[0];
		}
		return super.getBytes(pos,length);
	}
}
