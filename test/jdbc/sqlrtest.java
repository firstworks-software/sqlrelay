// Copyright (c) David Muse
// See the file COPYING for more information.

import java.sql.*;

class sqlrtest {

	protected static int status=0;

	protected static String success="\u001B[32msuccess\u001B[0m";
	protected static String failure="\u001B[31mfailure\u001B[0m";
	protected static String alltestssucceeded=
				"\n\u001B[34mAll tests succeeded\u001B[0m";
	protected static String sometestsfailed=
				"\n\u001B[33mSome tests failed\u001B[0m";

	protected static void assertEquals(Object actual, Object expected) {

		if (expected==null) {
			if (actual==null) {
				System.out.print(success+" ");
				return;
			} else {
				System.out.println(failure);
				System.out.println(actual+"!="+expected);
				status=1;
			}
			return;
		}

		if (actual!=null && actual.equals(expected)) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			status=1;
		}
	}

	protected static void assertEquals(String actual,
						String expected, int length) {

		if (expected==null) {
			if (actual==null) {
				System.out.print(success+" ");
				return;
			} else {
				System.out.println(failure);
				System.out.println(actual+"!="+expected);
				status=1;
			}
			return;
		}

		if (actual!=null && actual.regionMatches(0,expected,0,length)) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			status=1;
		}
	}

	protected static void assertEquals(String actual, String expected) {

		if (expected==null) {
			if (actual==null) {
				System.out.print(success+" ");
				return;
			} else {
				System.out.println(failure);
				System.out.println(actual+"!="+expected);
				status=1;
			}
			return;
		}

		if (actual!=null && actual.equals(expected)) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			status=1;
		}
	}

	// strips a leading '$' and thousands separators, then trailing-zero
	// decimals, so money values that differ only in formatting compare equal
	private static String normalizeMoney(String value) {

		value=value.replace("$","").replace(",","");
		if (value.indexOf('.')>=0) {
			while (value.endsWith("0")) {
				value=value.substring(0,value.length()-1);
			}
			if (value.endsWith(".")) {
				value=value.substring(0,value.length()-1);
			}
		}
		return value;
	}

	// compares money values tolerantly - a leading '$', thousands separators,
	// and trailing-zero decimals are ignored, so "1.50", "$1.50", and
	// "1.5000" all match.  a real precision difference like "1.23" vs "1.2345"
	// still fails.  old freetds (0.91) renders money with 2 decimal places
	// instead of 4.
	protected static void assertMoneyEquals(String actual, String expected) {

		if (actual==null || expected==null) {
			assertEquals(actual,expected);
			return;
		}

		if (normalizeMoney(actual).equals(normalizeMoney(expected))) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			status=1;
		}
	}

	protected static void assertEquals(byte[] actual,
						String expected, int length) {

		if (expected==null) {
			if (actual==null) {
				System.out.print(success+" ");
				return;
			} else {
				System.out.println(failure);
				status=1;
			}
			return;
		}

		byte[]	successvalue=expected.getBytes();

		for (int index=0; index<length; index++) {
			if (actual[index]!=successvalue[index]) {
				System.out.println(failure);
				status=1;
				return;
			}
		}

		System.out.print(success+" ");
	}

	protected static void assertEquals(long actual, int expected) {

		if (actual==expected) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			status=1;
		}
	}

	protected static void assertEquals(double actual, double expected) {

		if (actual==expected) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			status=1;
		}
	}

	protected static void assertEquals(boolean actual, int expected) {

		if (((actual)?1:0)==expected) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			status=1;
		}
	}

	protected static void assertTrue(boolean actual) {

		if (actual) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!=true");
			status=1;
		}
	}

	protected static void assertFalse(boolean actual) {

		if (!actual) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!=false");
			status=1;
		}
	}

	protected static void assertContainsVersion(String value) {

		// succeeds if the value contains a version number - a digit, a
		// dot, and a digit ("#.#") anywhere (bare version or banner both
		// pass; empty/text-only fails)
		if (value!=null && value.matches(".*[0-9]\\.[0-9].*")) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println("\""+value+
					"\" does not contain a version number");
			status=1;
		}
	}

	protected static void printColumns(ResultSetMetaData rsmd)
							throws Exception {
		System.out.println();
		for (int i=1; i<rsmd.getColumnCount()+1; i++) {
			System.out.println(rsmd.getColumnName(i));
		}
	}

	protected static void printResultSet(ResultSet rs) throws Exception {
		ResultSetMetaData	rsmd=rs.getMetaData();
		System.out.println();
		while (rs.next()) {
			for (int i=1; i<rsmd.getColumnCount()+1; i++) {
                        	System.out.print(rs.getString(i)+",");
			}
                        System.out.println();
		}
	}

	// Java 7 compatible replacement for InputStream.readAllBytes()
	// (added in Java 9)
	protected static byte[] streamToBytes(java.io.InputStream is)
							throws Exception {
		java.io.ByteArrayOutputStream buf=
					new java.io.ByteArrayOutputStream();
		byte[]	b=new byte[4096];
		int	len;
		while ((len=is.read(b))!=-1) {
			buf.write(b,0,len);
		}
		return buf.toByteArray();
	}

	// Java 7 compatible replacement for Reader.transferTo(Writer)
	// (added in Java 10)
	protected static void readerToWriter(java.io.Reader r,
						java.io.Writer w)
						throws Exception {
		char[]	buf=new char[4096];
		int	len;
		while ((len=r.read(buf))!=-1) {
			w.write(buf,0,len);
		}
	}

	protected static void reportTestStatus() {

		if (status==0) {
			System.out.println(alltestssucceeded);
		} else {
			System.out.println(sometestsfailed);
		}
	}
}
