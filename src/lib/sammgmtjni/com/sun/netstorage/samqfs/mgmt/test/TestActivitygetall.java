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

// ident	$Id: TestActivitygetall.java,v 1.6 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.fs.Restore;

public class TestActivitygetall {

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
	String jnilib, hostname;

	if (args.length == 0) {
	    jnilib = "sammgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "sammgmtjni";
	    hostname = args[0];
	}
        try {
            TUtil.loadLib(jnilib);
            System.out.println("\nInitializing server connection & library...");
            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);

            System.out.println("\n\n *** Calling 100 times\n");
            int maxEntries = 50;
            String filter = " ";

            try {
                System.out.println("\nMake the snapshot available...");
                String id = Restore.decompressDump(
                    ctx,
                    "samfs1",
                    "csd-Dec162004-samfs3");

                System.out.println("Search id: " + id);

            } catch (SamFSException e) {
                System.out.println("Error " + e.getMessage() + "\n\n");
            }
            for (int k = 0; k < 100; k++) {
                System.out.println("\nCalling native Jobs.getAllActivity()");
                String[] activities =
                    Job.getAllActivities(ctx, maxEntries, filter);
                System.out.println("Back to java ");
                for (int i = 0; i < activities.length; i++)
                    System.out.println(activities[i] + "\n");
            }

            TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
