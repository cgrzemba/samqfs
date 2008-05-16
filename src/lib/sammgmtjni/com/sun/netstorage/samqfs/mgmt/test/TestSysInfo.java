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

// ident	$Id: TestSysInfo.java,v 1.13 2008/05/16 18:35:31 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.adm.SysInfo;
import com.sun.netstorage.samqfs.mgmt.FileUtil;

public class TestSysInfo {

    public static void main(String args[]) {

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
            TUtil.loadLib(jnilib);
            System.out.println("\nInitializing server connection & library...");
            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);

            System.out.println("Calling native SysInfo.tail()");
            String[] entries = FileUtil.tailFile(ctx, "/tmp/stager.log", 50);
            System.out.println("Back to java ");
            for (int i = 0; i < entries.length; i++) {
                System.out.println(entries[i] + "\n");
            }

            System.out.println("Calling native SysInfo.getCapacities()");
            String caps = SysInfo.getCapacity(ctx);
            System.out.println("Back to java ");
            System.out.println(caps);

            System.out.println("Calling native SysInfo.getOSInfo()");
            String os = SysInfo.getOSInfo(ctx);
            System.out.println(os);

            System.out.println("Calling native SysInfo.getLogs()");
            String[] logs = SysInfo.getLogInfo(ctx);
            for (int i = 0; i < logs.length; i++) {
                System.out.println(logs[i] + "\n");
            }

            System.out.println("Calling native SysInfo.getPackageInfo()");
            String[] pkg = SysInfo.getPackageInfo(ctx, "");
            for (int i = 0; i < pkg.length; i++) {
                System.out.println(pkg[i] + "\n");
            }

            System.out.println("Calling native SysInfo.getconfigStatus()");
            String[] cfgStatus = SysInfo.getConfigStatus(ctx);
            for (int i = 0; i < cfgStatus.length; i++) {
                System.out.println(cfgStatus[i] + "\n");
            }

            System.out.println("Calling native FileUtil.getTxtFile()");
            String[] entries2 = FileUtil.getTxtFile(ctx, "/tmp/stager.log",
						    0, 10);
            System.out.println("Back to java ");
            for (int i = 0; i < entries2.length; i++) {
                System.out.println(entries2[i]);
            }

	    try {
		System.out.println("Calling FileUtil.getTxtFile(null, 0,10)");
		String[] entries3 = FileUtil.getTxtFile(ctx, null, 0, 10);
		System.out.println("Back to java ");
		for (int i = 0; i < entries2.length; i++) {
                    System.out.println(entries2[i]);
		}
	    } catch (SamFSException e) {
		System.out.println("Should have failed with no such file");
                System.out.println(e);
	    }

            System.out.println("Calling native SysInfo.listSamExplorer()");
            String[] expfiles = SysInfo.listSamExplorerOutputs(ctx);
            for (int i = 0; i < expfiles.length; i++) {
                    System.out.println(expfiles[i]);
            }

            System.out.println("Calling native SysInfo.runSamExplorer()");
	    SysInfo.runSamExplorer(ctx, null, 100);


            TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
