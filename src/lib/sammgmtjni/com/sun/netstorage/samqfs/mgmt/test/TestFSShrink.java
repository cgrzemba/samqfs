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

// ident	$Id: TestFSShrink.java,v 1.2 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.mgmt.fs.AU;
import com.sun.netstorage.samqfs.mgmt.BaseDev;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.StripedGrp;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import java.io.InputStreamReader;
import java.io.IOException;

public class TestFSShrink {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        FSInfo fs;
        AU[] aus;
	boolean stripedGroup = false;

        DiskDev[] data = new DiskDev[2]; // add 2 data devices

        if (args.length == 0) {
            System.out.println("expecting arguments: fs_name host ");
            System.exit(-1);
        }
	hostname = args[1];
	jnilib = "fsmgmtjni";


        try {

	    System.out.print("Loading native lib (" + jnilib + ")...");
	    System.loadLibrary(jnilib);
	    System.out.print("done\n" +
		"Initializing server connection & library...");

	    c = SamFSConnection.getNew(hostname);
	    ctx = new Ctx(c);
	    System.out.println("done\n");

	    System.out.print("getting FSInfo for " + args[0] + "...");
	    fs = FS.get(ctx, args[0]);
	    System.out.println("done\n");

	    /*
	     * Determine if we should remove a standard data dev or
	     * a striped group.
	     */
	    DiskDev[] ddev = fs.getDataDevices();
	    if (ddev == null || ddev.length == 0) {

		StripedGrp[] grps = fs.getStripedGroups();
		System.out.println("got striped groups:" + grps.length);
		if (grps != null && grps.length != 0) {

		    for (int i = 0; i < grps.length; i++) {
			ddev = grps[i].getMembers();
			if (ddev[0].getState() == BaseDev.DEV_OFF) {
			    continue;
			}
			System.out.println("got members:" + ddev.length);
			stripedGroup = true;
			break;
		    }
		}
	    }


	    /* Pick the eq to disable */
	    int eqToReplace = -1;
	    if (ddev != null && ddev.length != 0) {
		System.out.println("picking dev to remove");
		for (int i = 0; i < ddev.length; i++) {
		    System.out.println("picking dev to remove considering:" +
					i);
		    if (ddev[i].getState() != BaseDev.DEV_OFF) {
			eqToReplace = ddev[i].getEquipOrdinal();
			System.out.println("dev is off:" + i);
			break;
		    }
		}
	    }
	    if (eqToReplace == -1) {
		System.out.println("Failed to find a device to remove");
		System.exit(-1);
	    }


	    /* Find some disks to potentially use as replacments */
	    System.out.print("discovering aus...");
	    aus = AU.discoverAvailAUs(ctx);
	    System.out.println("done\n\nChoosing some AU-s ");
	    DiskDev[] dsks = new DiskDev[4];
	    for (int i = aus.length - 1, j = 0; (i >= 0) && (j < 4); i--) {
		System.out.println("i:j->" + i +":"+j);
		if (!aus[i].getPath().endsWith("s2") &&
		    aus[i].getPath().startsWith("/dev/dsk")) {
		    dsks[j++] = new DiskDev(aus[i]);
		}
	    }

	    if (!stripedGroup) {
		System.out.println("Potential replacement disk" +
				   dsks[0]);
	    } else {
		System.out.println("Potential replacements for striped group:");
		System.out.println(dsks[0]);
		System.out.println(dsks[1]);
	    }

	    try {
		System.out.println("Do you want to pass a replacement?");
		InputStreamReader isr = new InputStreamReader(System.in);
		String kvOptions =
		    new String("block_size=6,logfile=/tmp/logit");

		if ('n' == isr.read()) {
		    if (fs.isArchiving()) {
			System.out.println("Calling native FS.shrinkRelease()");
			FS.shrinkRelease(ctx, fs.getName(),
					 eqToReplace, kvOptions);

			System.out.println("Back to java ");
		    } else {
			System.out.println("Calling native FS.shrinkRemove()");
			FS.shrinkRemove(ctx, fs.getName(),
					 eqToReplace, -1, kvOptions);
			System.out.println("back from FS.shrinkRemove()");
		    }
		} else {
		    if (!stripedGroup) {
 			System.out.println("Calling FS.shrinkReplaceDev("
					   + fs.getName() + ", " + eqToReplace
					   + dsks[0].getDevicePath());
			FS.shrinkReplaceDev(ctx, fs.getName(),
					 eqToReplace, dsks[0], kvOptions);
			System.out.println("back from FS.shrinkReplaceDev()");
		    } else {

			data[0] = dsks[0];
			data[1] = dsks[1];

			StripedGrp grp = new StripedGrp("g6", data);
			FS.shrinkReplaceGroup(ctx, fs.getName(),
					 eqToReplace, grp, kvOptions);

 			System.out.println("Calling FS.shrinkReplaceDev()");

		    }

		}
	    } catch (IOException e) { e.printStackTrace(); }

	    System.out.print("destroying connection...");
	    c.destroy();
	    System.out.println("done");
        } catch (SamFSException e) {
	    System.out.println(e);
        }
    }
}
