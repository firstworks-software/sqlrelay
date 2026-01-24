// Copyright (c) David Muse
// See the file COPYING for more information.

import com.firstworks.sqlrelay.SQLRConnection;
import com.firstworks.sqlrelay.SQLRCursor;


class sqlrtest {

	protected static void assertEquals(String value, String success, int length) {
	
		if (success==null) {
			if (value==null) {
				System.out.print("success ");
				return;
			} else {
				System.out.print(value+"!="+success+" ");
				System.out.print("failure ");
				System.exit(1);
			}
		}
	
		if (value.regionMatches(0,success,0,length)) {
			System.out.print("success ");
		} else {
			System.out.print(value+"!="+success+" ");
			System.out.print("failure ");
			System.exit(1);
		}
	}
	
	protected static void assertEquals(String value, String success) {
	
		if (success==null) {
			if (value==null) {
				System.out.print("success ");
				return;
			} else {
				System.out.print(value+"!="+success+" ");
				System.out.print("failure ");
				System.exit(1);
			}
		}
	
		if (value.equals(success)) {
			System.out.print("success ");
		} else {
			System.out.print(value+"!="+success+" ");
			System.out.print("failure ");
			System.exit(1);
		}
	}
	
	protected static void assertEquals(byte[] value, String success, int length) {
	
		if (success==null) {
			if (value==null) {
				System.out.print("success ");
				return;
			} else {
				System.out.print("failure ");
				
				
				System.exit(1);
			}
		}

		byte[]	successvalue=success.getBytes();
	
		for (int index=0; index<length; index++) {
			if (value[index]!=successvalue[index]) {
				System.out.println("failure ");
				System.exit(1);
			}
		}

		System.out.println("success ");
	}
	
	protected static void assertEquals(long value, int success) {
	
		if (value==success) {
			System.out.println("success ");
		} else {
			System.out.println("failure ");
			
			
			System.exit(1);
		}
	}
	
	protected static void assertEquals(double value, double success) {
	
		if (value==success) {
			System.out.println("success ");
		} else {
			System.out.println("failure ");
			
			
			System.exit(1);
		}
	}
	
	protected static void assertEquals(boolean value, int success) {

		if (((value)?1:0)==success) {
			System.out.println("success ");
		} else {
			System.out.println("failure ");


			System.exit(1);
		}
	}

	protected static void assertTrue(boolean value) {

		if (value) {
			System.out.println("success ");
		} else {
			System.out.println("failure ");
			System.exit(1);
		}
	}

	protected static void assertFalse(boolean value) {

		if (!value) {
			System.out.println("success ");
		} else {
			System.out.println("failure ");
			System.exit(1);
		}
	}
}
