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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: TestHost.java,v 1.13 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.*;

import java.util.ArrayList;

public class TestHost {

    public static void main(String args[]) {

        String host[];
        FSInfo fs[];
        SamFSConnection conn;
        Ctx ctx;
        String hostname, jnilib;
	String fsName;
        ArrayList ipAddresses;

        if (args.length == 0) {
	    System.out.println("Usage: java -classpath ... classname hostname" +
		" discover | on <fs_name> <clients> | " +
		"off <fs_name> <clients> ");
	    System.exit(-1);
        }
	jnilib = "fsmgmtjni";
	hostname = args[0];


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

	    if (args[1].equals("discover")) {
		System.out.println("\nCalling Host.discoverIPsAndNames()");
		String[] ips = Host.discoverIPsAndNames(ctx);
		for (int i = 0; i < ips.length; i++) {
		    System.out.println("\t " + ips[i]);
		}
	    } else if (args[1].equals("gethosts")) {

		System.out.println("\nCalling native FS.getAll()");
		fs = FS.getAll(ctx);
		System.out.println("Back to java ");
		for (int i = 0; i < fs.length; i++) {
		    if (fs[i].isShared()) {
			System.out.println("Calling native Host.getConfig()");
			host = Host.getSharedFSHosts(ctx, fs[i].getName(), 0xf);

			for (int j = 0; j < host.length; j++) {
			    System.out.println(host[j] + "\n");
			}
		    }
		}
	    } else if (args[1].equals("on") && args.length >= 4) {
		System.out.println("enable clients:");
		String[] clients = new String[args.length - 3];
		for (int i = 3; i < args.length; i++) {
		    System.out.println(args[i]);
		    clients[i - 3] = args[i];
		}
		System.out.println("calling set state with clients" +
				   clients.length);
		Host.setClientState(ctx, args[2], clients, Host.CL_STATE_ON);
		System.out.println("back");
	    } else if (args[1].equals("off") && args.length >= 4) {
		System.out.println("disable clients:");
		String[] clients = new String[args.length - 3];
		for (int i = 3; i < args.length; i++) {
		    System.out.println(args[i]);
		    clients[i - 3] = args[i];
		}
		System.out.println("calling set state with clients" +
				   clients.length);
		Host.setClientState(ctx, args[2], clients, Host.CL_STATE_OFF);
		System.out.println("back");
	    } else {

		System.out.println("There was no match for args[1] " + args[1]);
	    }

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
