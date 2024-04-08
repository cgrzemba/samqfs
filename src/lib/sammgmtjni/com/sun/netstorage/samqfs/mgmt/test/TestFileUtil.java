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
 * or https://illumos.org/license/CDDL.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: TestFileUtil.java,v 1.8 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.FileUtil;

import java.util.ArrayList;

public class TestFileUtil {

    public static void main(String args[]) {

        SamFSConnection conn;
        Ctx ctx;
        String hostname, jnilib;
	String fsName;
        ArrayList ipAddresses;

        if (args.length == 0) {
            jnilib = "sammgmtjnilocal";
            hostname  = "localhost";
        } else {
            jnilib = "fsmgmtjni";
            hostname = args[0];
        }

        try {

            System.out.println("loading "+ jnilib);
            System.loadLibrary(jnilib);
            System.out.println("native library loaded successfuly\n");
            System.out.println("creating SamFSConnection w/ " + hostname);
            conn = SamFSConnection.getNew(hostname);
            ctx = new Ctx(conn);
            System.out.println("connection created - " + conn);
            conn.setTimeout(65);
            System.out.println("timeout set - " + conn);

	    if (args.length < 3) {

		System.out.println("Usage: hostname getinfo file_name(s)");

	    } if (args[1].equals("getinfo")) {
		String[] files = new String[args.length - 2];
		int whichDetails = FileUtil.FILE_TYPE | FileUtil.SIZE |
		    FileUtil.RELEASE_ATTS | FileUtil.STAGE_ATTS |
		    FileUtil.SAM_STATE | FileUtil.USER | FileUtil.GROUP;

		for (int i = 2; i < args.length; i++) {
		    files[i-2] = args[i];
		}
		System.out.println("\nCalling native Hosts." +
				   "getExtFileDetails(" + args[0] + "..." +
				   whichDetails);

		String[] details = FileUtil.getExtFileDetails(ctx, files,
                                                              whichDetails);

		for (int i = 0; i < details.length; i++) {
		    System.out.print(files[i]+ " ");
		    System.out.println(details[i]);
		    String[] copydets = FileUtil.getCopyDetails(ctx,
			files[i], 0);
		    for (int j = 0; j < copydets.length; j++) {
			System.out.println("\t" + copydets[j]);
		    }
		}
		System.out.println("Back to java- do it again");

		details = FileUtil.getExtFileDetails(ctx, files, whichDetails);

		for (int i = 0; i < details.length; i++) {
		    System.out.print(files[i]+ " ");
		    System.out.println(details[i]);
		    String[] copydets = FileUtil.getCopyDetails(ctx,
			files[i], 0);
		    for (int j = 0; j < copydets.length; j++) {
			System.out.println("\t" + copydets[j]);
		    }
		}

	    }
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
