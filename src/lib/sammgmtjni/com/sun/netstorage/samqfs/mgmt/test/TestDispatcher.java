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

// ident	$Id: TestDispatcher.java,v 1.4 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.*;
import com.sun.netstorage.samqfs.mgmt.Job;

public class TestDispatcher {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        FSInfo fs;
	boolean stripedGroup = false;
	int jobId;
        DiskDev[] data = new DiskDev[2]; // add 2 data devices

        if (args.length < 2) {
            System.out.println("usage: host fsname " +
		"<mount|umount|options|deletejob jobids");
            System.exit(-1);
        }
	hostname = args[0];
	jnilib = "fsmgmtjni";

	String op = args[1];
	String fsname = null;
	if (args.length > 2) {
	    fsname = args[2]; /* will be ignored by deletejob */
	}
        try {

	    System.out.print("Loading native lib (" + jnilib + ")...");
	    System.loadLibrary(jnilib);
	    System.out.print("done\n" +
		"Initializing server connection & library...");

	    c = SamFSConnection.getNewSetTimeout(hostname, 300);
	    ctx = new Ctx(c);
	    System.out.println("done\n");


	    if (op.equals("remove")) {
		/* mds remove fsname hostname hostname hostname */

		String[] hostNames = new String[args.length - 3];
		for (int i = 3; i < args.length; i++) {
		    hostNames[i - 3] = args[i];
		}
		System.out.println("Calling remove host " + args[3] + "...");
		jobId = Host.removeHosts(ctx, fsname, hostNames);
		System.out.println("removed host " + args[3] + "..."
				   + " returned: " + jobId);

	    } else if (op.equals("add")) {
		/* 67 add fsname hostname ipaddr hostname ipaddr... */

		if (args.length < 5) {
		    System.out.println("usage: mds add fsname " +
			"host ipaddr host ipaddr");
		}

		Host[] hosts = new Host[(args.length - 3)/2];

		for (int i = 3; i < args.length; i += 2) {
		    String[] ipaddrs = new String[1];
		    ipaddrs[0] = args[i+1];
		    hosts[(i -3)/2] = new Host(args[i], ipaddrs, 0, false);
		}
		fs = FS.get(ctx, fsname);

		System.out.println("Calling add host " + args[3]);
		jobId = Host.addHosts(ctx, fsname, hosts,
			"mount_point=/fsn,read_only=yes,mount_fs=yes," +
			"mount_at_boot=yes,potential_mds=yes");

		System.out.println("added host " + args[3] +
			" returned: " + jobId);


	    } else if (op.equals("deletejob")) {


		for (int i = 2; i < args.length; i++) {
		    try {
			System.out.println("Calling cancel for " + args[i]);
			Job.cancelActivity(ctx, args[i], "SAMADISPATCHJOB");
		    } catch (SamFSException e) {
			System.out.println("Canceling " + args[i] + " failed:");
			System.out.println(e);
			e.printStackTrace();
		    }
		    System.out.println("Canceled activity");
		}
		System.exit(0);
	    } else if (op.equals("mount")) {

		String[] hosts = new String[args.length - 3];
		for (int i = 3; i < args.length; i++) {
		    hosts[i - 3] = args[i];
		}

		try {
		    int ret = FS.mountClients(ctx, fsname, hosts);
		    System.out.println("mount clients returned: " + ret + "\n");

		} catch (SamFSException e) {
		    System.out.println(e);
		    e.printStackTrace();
		}


	    } else if (op.equals("unmount")) {

		String[] hosts = new String[args.length - 3];
		for (int i = 3; i < args.length; i++) {
		    hosts[i - 3] = args[i];
		}

		try {
		    int ret = FS.unmountClients(ctx, fsname, hosts);
		    System.out.println("unmount returned:" + ret +"\n");

		} catch (SamFSException e) {
		    System.out.println(e);
		    e.printStackTrace();
		}
	    } else if (op.equals("grow") && args.length >= 4)  {

		/* mdshost grow fsname disk disk disk */
		AU[] aus = AU.discoverAvailAUs(ctx);
		DiskDev[] ddev = new DiskDev[args.length - 3];
		for (int i = 3; i < args.length; i++) {
		    for (int j = 0; j < aus.length; j++) {
			if (args[i].equals(aus[j].getPath())) {
			    ddev[i - 3] = new DiskDev(aus[j]);
			}
		    }
		    if (ddev[i-3] == null) {
			System.out.println("Did not find the disk: " + args[i]);
			System.exit(-1);
		    }
		}
		System.out.println("found " + (args.length - 3) +
			" disks. \nCalling Grow");

		fs = FS.get(ctx, fsname);
		int ret = FS.grow(ctx, fs, null, ddev, null);

		System.out.println("Grow returned: " + ret);


	    } else if (op.equals("growmeta") && args.length >= 4)  {


		AU[] aus = AU.discoverAvailAUs(ctx);
		DiskDev[] ddev = new DiskDev[args.length - 3];
		for (int i = 3; i < args.length; i++) {
		    for (int j = 0; j < aus.length; j++) {
			if (args[i].equals(aus[j].getPath())) {
			    ddev[i - 3] = new DiskDev(aus[j]);
			}
		    }
		    if (ddev[i-3] == null) {
			System.out.println("Did not find the disk: " + args[i]);
			System.exit(-1);
		    }
		}
		System.out.println("found " + (args.length - 3) +
			" disks. \nCalling Grow");

		fs = FS.get(ctx, fsname);
		int ret = FS.grow(ctx, fs, ddev, null, null);

		System.out.println("Grow meta returned: " + ret);


	    } else if (op.equals("options") && args.length >= 5) {

		try {
		    System.out.print("getting FSInfo for " + fsname + "...");
		    fs = FS.get(ctx, fsname);
		    System.out.println("done\n");

		    /* last arg is the new value of read lease */
		    int newreadlease = Integer.valueOf(args[args.length -1]);

		    /*
		     * Take into account that the last arg
		     * is the readlease value
		     */
		    String[] hosts = new String[args.length - 4];
		    for (int i = 3; i < args.length - 1; i++) {
			hosts[i - 3] = args[i];
		    }


		    /* currently hacked through standard change mount options */
		    MountOptions mo = fs.getMountOptions();
		    mo.setReadLeaseDuration(newreadlease);

		    int ret = FS.setSharedFSMountOptions(ctx,
				fsname, hosts, mo);
		    System.out.println("Change shared fs mount" +
				"options returned" + ret);

		} catch (SamFSException e) {
		    System.out.println(e);
		    e.printStackTrace();
		}

	    } else {
		System.out.println("Operation not recognized. " + op +
			" Getting activities ");
	    }


	    String filter = "type=SAMADISPATCHJOB";

	    String[] activities = Job.getAllActivities(ctx, 10, filter);
	    System.out.println("Back to java with " + activities.length +
			"activities");
	    for (int i = 0; i < activities.length; i++) {
		System.out.println(activities[i] + "\n");
	    }


	    System.out.print("destroying connection...");
	    c.destroy();
	    System.out.println("done");


	    } catch (SamFSException e) {
		System.out.println(e);
		e.printStackTrace();
	    }

    }
}
