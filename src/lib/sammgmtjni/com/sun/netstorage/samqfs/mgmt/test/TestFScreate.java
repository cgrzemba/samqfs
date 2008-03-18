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

// ident	$Id: TestFScreate.java,v 1.9 2008/03/17 14:44:03 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.*;

public class TestFScreate {

    public static void main(String args[]) {
        FSInfo fs;
        SamFSConnection c = null;
        String jnilib, hostname;
	Ctx ctx = null;
        AU[] aus;
        DiskDev[] meta;
        DiskDev[] data;

        if (args.length == 0) {
            System.out.println("One arg expected - filesystem name");
            System.exit(-1);
        }
        if (args.length == 1) {
            hostname = "localhost";
            jnilib = "sammgmtjnilocal";
        } else {
            hostname = args[1];
            jnilib = "sammgmtjni";
        }

        try {
           System.out.print("Loading native lib (" + jnilib + ")...");
           System.loadLibrary(jnilib);
           System.out.print("done.\n" +
           "Initializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           System.out.println("done");
	   ctx = new Ctx(c);

           fs = new FSInfo(args[0], 44, 4096, FSInfo.COMBINED_METADATA,
            null, null, null, null, "/mntpoint");
           System.out.println("TEST1. Calling native FS.create(..., F)");
           fs.create(ctx, false);
           System.out.println("Back to java ");
        } catch (SamFSException e) {
                System.out.println(e);
        }
        try {
	   System.out.println("TEST2. Now use some real slices...");
           aus = AU.discoverAvailAUs(ctx);
           System.out.println("Back to java. Constructing FSInfo ");

           meta = new DiskDev[2];
           data = new DiskDev[5];

           for (int i = 0; i < aus.length; i++) {
                    if (aus[i].getPath().endsWith("c2t26d0s4"))
                        meta[0] = new DiskDev(aus[i]);
                    else if (aus[i].getPath().endsWith("c2t23d0s7"))
                        meta[1] = new DiskDev(aus[i]);
                    else if (aus[i].getPath().endsWith("c2t20d0s7"))
                        data[0] = new DiskDev(aus[i]);
                    else if (aus[i].getPath().endsWith("c2t19d0s7"))
                        data[1] = new DiskDev(aus[i]);
                    else if (aus[i].getPath().endsWith("c2t17d0s7"))
                        data[2] = new DiskDev(aus[i]);
                    else if (aus[i].getPath().endsWith("c2t16d0s7"))
                        data[3] = new DiskDev(aus[i]);
                    else if (aus[i].getPath().endsWith("c2t10d0s7"))
                        data[4] = new DiskDev(aus[i]);
                }
	   if (meta[0] == null) System.out.println("meta[0] is null");
	   if (meta[1] == null) System.out.println("meta[1] is null");
	   if (data[0] == null) System.out.println("data[0] is null");
	   if (data[1] == null) System.out.println("data[1] is null");
	   if (data[2] == null) System.out.println("data[2] is null");
	   if (data[3] == null) System.out.println("data[3] is null");
	   if (data[4] == null) System.out.println("data[4] is null");

	   fs = new FSInfo(args[0], 44, 16, FSInfo.SEPARATE_METADATA,
            meta, data, null, null, "/mntpoint2");
           System.out.println("Calling native FS.create(..., T)");
           fs.create(ctx, true);
           System.out.println("Back to java ");

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done.");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
