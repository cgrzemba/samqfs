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

// ident	$Id: TestDVolFlags.java,v 1.7 2008/03/17 14:44:03 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;

public class TestDVolFlags {

    static void printFlags(DiskVol vol) {
        System.out.println("Volume " + vol.getVolName() + "\n" +
            " total capacity: " + vol.getCapacity() + "\n" +
            " available space: " + vol.getAvailableSpace() + "\n" +
            " Labeled: " + (vol.isLabeled() ? "T" : "F") + "\n" +
            " Read Only: " + (vol.isReadOnly() ? "T" : "F") + "\n" +
            " Unavailable: " + (vol.isUnavailable() ? "T" : "F") + "\n" +
            " Bad media: " + (vol.isBadMedia() ? "T" : "F") + "\n" +
            " Remote: " + (vol.isRemote() ? "T" : "F"));
    }

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        DiskVol vol, all[];
        if (args.length == 0) {
            System.out.println("One arg expected - disk vol name");
            System.exit(-1);
        }
        if (args.length == 1) {
            jnilib = "fsmgmtjnilocal";
            hostname = "localhost";
        } else {
            hostname = args[1];
            jnilib = "fsmgmtjni";
        }

        try {
            System.out.print("Loading native lib (" + jnilib + ")...");
            TUtil.loadLib(jnilib);
            System.out.print("\nInitializing server connection & library...");
            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);

            System.out.println("done\n");

            System.out.println("Calling native DiskVol.getAll()");
            all = DiskVol.getAll(ctx);
            for (int i = 0; i < all.length; i++)
                System.out.println(all[i]);

            System.out.println("DiskVol.get(ctx," + args[0] + ")");
            vol = DiskVol.get(ctx, args[0]);

            printFlags(vol);

            System.out.println("DiskVol.setStatusFlags(ro)");
            vol.setReadOnly(true);
            vol.setStatusFlags(ctx);

            System.out.println("DiskVol.get(ctx," + args[0] + ")");
            vol = DiskVol.get(ctx, args[0]);

            printFlags(vol);

            System.out.print("destroying connection...");
            c.destroy();
            System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
