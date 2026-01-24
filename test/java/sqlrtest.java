// Copyright (c) David Muse
// See the file COPYING for more information.

class sqlrtest {

	protected static void assertEquals(String actual, String expected, int length) {
	
		if (expected==null) {
			if (actual==null) {
				System.out.print("expected ");
				return;
			} else {
				System.out.print(actual+"!="+expected+" ");
				System.out.print("failure ");
				System.exit(1);
			}
		}
	
		if (actual.regionMatches(0,expected,0,length)) {
			System.out.print("expected ");
		} else {
			System.out.print(actual+"!="+expected+" ");
			System.out.print("failure ");
			System.exit(1);
		}
	}
	
	protected static void assertEquals(String actual, String expected) {
	
		if (expected==null) {
			if (actual==null) {
				System.out.print("expected ");
				return;
			} else {
				System.out.print(actual+"!="+expected+" ");
				System.out.print("failure ");
				System.exit(1);
			}
		}
	
		if (actual.equals(expected)) {
			System.out.print("expected ");
		} else {
			System.out.print(actual+"!="+expected+" ");
			System.out.print("failure ");
			System.exit(1);
		}
	}
	
	protected static void assertEquals(byte[] actual, String expected, int length) {
	
		if (expected==null) {
			if (actual==null) {
				System.out.print("expected ");
				return;
			} else {
				System.out.print("failure ");
				
				
				System.exit(1);
			}
		}

		byte[]	successvalue=expected.getBytes();
	
		for (int index=0; index<length; index++) {
			if (actual[index]!=successvalue[index]) {
				System.out.println("failure ");
				System.exit(1);
			}
		}

		System.out.println("expected ");
	}
	
	protected static void assertEquals(long actual, int expected) {
	
		if (actual==expected) {
			System.out.println("expected ");
		} else {
			System.out.println("failure ");
			
			
			System.exit(1);
		}
	}
	
	protected static void assertEquals(double actual, double expected) {
	
		if (actual==expected) {
			System.out.println("expected ");
		} else {
			System.out.println("failure ");
			
			
			System.exit(1);
		}
	}
	
	protected static void assertEquals(boolean actual, int expected) {

		if (((actual)?1:0)==expected) {
			System.out.println("expected ");
		} else {
			System.out.println("failure ");


			System.exit(1);
		}
	}

	protected static void assertTrue(boolean actual) {

		if (actual) {
			System.out.println("expected ");
		} else {
			System.out.println("failure ");
			System.exit(1);
		}
	}

	protected static void assertFalse(boolean actual) {

		if (!actual) {
			System.out.println("expected ");
		} else {
			System.out.println("failure ");
			System.exit(1);
		}
	}
}
