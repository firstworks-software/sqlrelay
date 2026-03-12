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

	protected static void reportTestStatus() {

		if (status==0) {
			System.out.println(alltestssucceeded);
		} else {
			System.out.println(sometestsfailed);
		}
	}
}
