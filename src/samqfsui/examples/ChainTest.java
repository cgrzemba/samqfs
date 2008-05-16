/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: ChainTest.java,v 1.5 2008/05/16 19:39:11 am143972 Exp $


import java.io.IOException;


class SamException extends Exception {

	private String errorCode;
	
	public SamException() { }

	public SamException (String errorCode) {

		this.errorCode = errorCode;
	}

	public SamException (String errorCode, Throwable cause) {

		super(errorCode, cause);
		this.errorCode = errorCode;
	}

	public SamException (Throwable cause) {

		super(cause);
	}

	public String getMessage() { 

		if (errorCode == null) return null;
		
		// TODO: lookup error code in resource bundle and get message
		return errorCode; 
	}
}


public class ChainTest { 

	private String name;	

	public void getA() throws Exception {

		try {	
		getB();
		}
		catch (Throwable se) {
			throw new Exception(se.getMessage() == null 
								? "My Error Code" : se.getMessage(), se); 
		}
	}

	public void getB() throws SamException {

		try {	
		getC();
		}
		catch (Throwable se) {

			throw new SamException(se.getMessage() == null
								  ? "My Error Code" : se.getMessage(), se);
		}
	}
	
	public void getC() throws IOException {

		try {	
		getD();	
		}
		catch (Throwable cnfex) {
			throw new IOException(cnfex.getMessage() == null
								 ? "My Error Code" : cnfex.getMessage()); 
		}			
	}

	public void getD() throws ClassNotFoundException {

		throw new ClassNotFoundException("Exception in D");
	}

	public static void main (String[] args) {

		try {	
		new ChainTest().getA();
		}
		catch (Throwable se) {

			System.out.println(se.getMessage());
			System.out.println("-------------");
			// get the causes just to see where everything came from
			Throwable t = se.getCause();
			while (t != null) {
				System.out.println(t.getMessage());
				t = t.getCause();
			}
		}
	}
}
