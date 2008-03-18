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

// ident	$Id: TestHost.java,v 1.10 2008/03/17 14:44:03 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.*;

import java.util.ArrayList;

public class TestHost {

    public static void main(String args[]) {

        Host host[];
        FSInfo fs[];
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

            System.out.println("\nCalling native Host.discoverIPsAndNames()");
	    String[] ips = Host.discoverIPsAndNames(ctx);

	    for (int i = 0; i < ips.length; i++)
		System.out.println("\t " + ips[i]);

            System.out.println("\nCalling native FS.getAll()");
            fs = FS.getAll(ctx);
            System.out.println("Back to java ");
            for (int i = 0; i < fs.length; i++) {
                System.out.println(fs[i] + "\n");
                if (fs[i].isShared()) {
                    System.out.println("Calling native Host.getConfig()");
                    host = Host.getConfig(ctx, fs[i].getName());

                    for (int j = 0; j < host.length; j++) {
                        System.out.println(host[j] + "\n");
                    }
                }
            }

	    if (args.length < 2) {

		String[] hosts_in =  {
		"hostname=ns-east-64, ipaddresses=10.8.11.64 192.168.0.64",
		"hostname=ns-east-16, ipaddresses=ns-east-16 192.168.0.16",
		"hostname=ns-east-33, ipaddresses= ns-east-33 192.168.0.33"};

		fsName = "testfs";
		System.out.println("\nCalling native" +
				   "Hosts.setAdvancedNetCfg()");

		Host.setAdvancedNetCfg(ctx, fsName, hosts_in);
		System.out.println("Back to java");

		System.out.println("\nCalling native" +
				   "Hosts.getAdvancedNetCfg(bbaa)");
		String[] hosts = Host.getAdvancedNetCfg(ctx, fsName);
		System.out.println("Back to java");
		for (int i = 0; i < hosts.length; i++)
		    System.out.println("\t " + hosts[i]);
	    } else {
		fsName = args[1];
		System.out.println("\nCalling native Hosts." +
				   "getAdvancedNetCfg(" + fsName + ")");
		String[] hosts = Host.getAdvancedNetCfg(ctx, fsName);
		System.out.println("Back to java");
		for (int i = 0; i < hosts.length; i++)
		    System.out.println("\t " + hosts[i]);
	    }
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
