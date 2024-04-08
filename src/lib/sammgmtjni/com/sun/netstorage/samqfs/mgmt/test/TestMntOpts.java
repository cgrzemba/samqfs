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

// ident	$Id: TestMntOpts.java,v 1.13 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.*;

public class TestMntOpts {

    public static void main(String args[]) {
        MountOptions mo;
        SamFSConnection c;
        String jnilib, hostname;
        Ctx ctx;

        if (args.length == 0) {
            System.out.println("One arg expected - filesystem name");
            System.exit(-1);
        }
        if (args.length == 1) {
            hostname = "localhost";
            jnilib = "fsmgmtjnilocal";
        } else {
            hostname = args[1];
            jnilib = "fsmgmtjni";
        }
        try {
           System.out.print("Loading native lib (" + jnilib + ")...");
           System.loadLibrary(jnilib);
           System.out.println("done.\n"
           + "Initializing server connection & library...");

           c = SamFSConnection.getNew(hostname);
           System.out.println("done\n");
           ctx = new Ctx(c);

           System.out.println("Calling native FS.getDefaultMountOpts(\"ma\","
                            + "16,F,F,F)");
           mo = FS.getDefaultMountOpts(ctx, "ma", 16, false, false, false);
           System.out.println("back to java");
           System.out.println(mo + "\n");

           System.out.println("TEST1: FS.getDefaultMountOpts(\"ms\",16,"
           + "F,F,F)");
           mo = FS.getDefaultMountOpts(ctx, "ms", 16, false, false, false);
           System.out.println("back to java");
           System.out.println(mo + "\n");

           System.out.println("TEST2: FS.getMountOpts(ctx," +
            args[0] + ")");
           mo = FS.getMountOpts(ctx, args[0]);
           System.out.println("Back to java ");
           System.out.println(mo);

           System.out.println("TEST3: FS.setMountOptions for " + args[0]);
           mo.setReadOnlyMount(true);
           mo.setTrace(false);
           mo.setStripeWidth((short)4);
           mo.setNoSetUID(false);
           mo.setSoftRAID(false);
           mo.setMisAlignedReadMin(12345);
           mo.setMisAlignedWriteMin(11111);
           mo.setDefaultMaxPartialReleaseSize(2097152);
           mo.setPartialStageSize(2097152);
           FS.setMountOpts(ctx, args[0], mo);
           System.out.println("done.check config files to verify correctness");

	   System.out.println("\nNew Option Testing 4.5\n");
	   mo = FS.getMountOpts(ctx, args[0]);
	   mo.setConsistencyChecking(!mo.isConsistencyChecking());
	   mo.setDirectIOZeroing(!mo.isDirectIOZeroing());
	   mo.setForceNFSAsync(!mo.isForceNFSAsync());
	   mo.setLeaseTimeout(
                ((mo.getLeaseTimeout() == 15) ?
                    1 : (mo.getLeaseTimeout() + 1)));
	   FS.setMountOpts(ctx, args[0], mo);
	   System.out.println("FS.getMountOpts(ctx, " + args[0] + ")" +
		   "to check changes");
           mo = FS.getMountOpts(ctx, args[0]);
           System.out.println(mo);

	   if (args.length == 3) {
               /* reset the new options */
               mo.resetConsistencyChecking();
               mo.resetDirectIOZeroing();
               mo.resetForceNFSAsync();
               mo.resetLeaseTimeout();
               System.out.println("Calling setMountOpts to reset" +
                                  " new options");
               FS.setMountOpts(ctx, args[0], mo);
               System.out.println("FS.getMountOpts(ctx, " + args[0] + ")" +
                   "to check reset");
               mo = FS.getMountOpts(ctx, args[0]);
               System.out.println(mo);
	   }
           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
