/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: TestStageFiles.java,v 1.6 2008/03/17 14:44:04 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.stg.*;


public class TestStageFiles {

    public static void main(String args[]) {

        StagerParams sp;
        SamFSConnection c;

        Ctx ctx;
	String jnilib, hostname;

	if (args.length == 0) {
	    jnilib = "fsmgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "fsmgmtjni";
	    hostname = args[0];
	}
        try {
	    if (args.length < 3) {
		System.out.println("Usage: <copynum> <filename>" +
				 " <options>");
	    }
            TUtil.loadLib(jnilib);
            System.out.print("Initializing server connection & library...");
            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);
            if (null == ctx)
                System.out.println("null ctx!!");
            System.out.println("done loading lib and connecting");


	    System.out.println("Testing Staging File:" + args[1]);

	    String[] files = new String[1];

	    files[0] = args[2];
	    int copy = Integer.parseInt(args[1], 10);
	    int options = Integer.parseInt(args[3], 10);
	    System.out.println("\nCalling native Stager." +
                               "stageFiles " + files[0] +
                               " from copy " + copy + " with options " +
                               options + ")");

            if (copy == 1) {
                options |= Stager.COPY_1;
            } else if (copy == 2) {
                options |= Stager.COPY_2;
            } else if (copy == 3) {
                options |= Stager.COPY_3;
            } else if (copy == 4) {
                options |= Stager.COPY_4;
	    }

            Stager.stageFiles(ctx, files, options);
	    System.out.println("Back to java");

            TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
