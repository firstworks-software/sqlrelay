// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.*;

class sqlrtest {

	protected static int status=0;

	protected static String success="\u001B[32msuccess\u001B[0m";
	protected static String failure="\u001B[31mfailure\u001B[0m";
	protected static String alltestssucceeded="\n\u001B[34mAll tests succeeded\u001B[0m";
	protected static String sometestsfailed="\n\u001B[33mSome tests failed\u001B[0m";

	protected static SQLRConnection con;
	protected static SQLRCursor cur;

	protected static void printErrors() {

		if (cur!=null) {
			String err=cur.errorMessage();
			if (err!=null) {
				System.out.println(err);
				return;
			}
		}
		if (con!=null) {
			String err=con.errorMessage();
			if (err!=null) {
				System.out.println(err);
				return;
			}
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
				printErrors();
				status=1;
			}
			return;
		}

		if (actual.regionMatches(0,expected,0,length)) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			printErrors();
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
				printErrors();
				status=1;
			}
			return;
		}

		if (actual!=null && actual.equals(expected)) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			printErrors();
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
				printErrors();
				status=1;
			}
			return;
		}

		byte[]	successvalue=expected.getBytes();

		for (int index=0; index<length; index++) {
			if (actual[index]!=successvalue[index]) {
				System.out.println(failure);
				printErrors();
				status=1;
				return;
			}
		}

		System.out.print(success+" ");
	}

	protected static void assertEquals(byte[] actual,
						byte[] expected, int length) {

		if (expected==null) {
			if (actual==null) {
				System.out.print(success+" ");
				return;
			} else {
				System.out.println(failure);
				printErrors();
				status=1;
			}
			return;
		}

		for (int index=0; index<length; index++) {
			if (actual[index]!=expected[index]) {
				System.out.println(failure);
				printErrors();
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
			printErrors();
			status=1;
		}
	}

	protected static void assertEquals(double actual, double expected) {

		if (actual==expected) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			printErrors();
			status=1;
		}
	}

	protected static void assertEquals(boolean actual, int expected) {

		if (((actual)?1:0)==expected) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			printErrors();
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
	// and trailing-zero decimals are ignored, so "1.00", "$1.00", and
	// "1.0000" all match.  a real precision difference like "1.23" vs
	// "1.2345" still fails.  old freetds (0.91) renders money with 2 decimal
	// places instead of 4.
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
			printErrors();
			status=1;
		}
	}

	// compares a rendered money length tolerantly.  old freetds (0.91)
	// renders money with 2 decimal places instead of 4, so the length may
	// be 2 short.
	protected static void assertMoneyLengthEquals(long actual, int expected) {

		if (actual==expected || (expected>=2 && actual==expected-2)) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+expected);
			printErrors();
			status=1;
		}
	}

	protected static void assertTrue(boolean actual) {

		if (actual) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!=true");
			printErrors();
			status=1;
		}
	}

	protected static void assertFalse(boolean actual) {

		if (!actual) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!=false");
			printErrors();
			status=1;
		}
	}

	protected static void assertStartsWith(String actual, String prefix) {

		if (actual!=null && actual.startsWith(prefix)) {
			System.out.print(success+" ");
		} else {
			System.out.println(failure);
			System.out.println(actual+"!="+prefix);
			printErrors();
			status=1;
		}
	}

	protected static void assertInResultSet(SQLRCursor cursor,
						String column, String value) {

		for (long i=0; i<cursor.rowCount(); i++) {
			if (value.equals(cursor.getField(i,column))) {
				System.out.print(success+" ");
				return;
			}
		}
		System.out.println(failure);
		System.out.println("\""+value+"\" not found in column \""+
								column+"\"");
		printErrors();
		status=1;
	}

	protected static void reportTestStatus() {

		if (status==0) {
			System.out.println(alltestssucceeded);
		} else {
			System.out.println(sometestsfailed);
		}
	}
}
