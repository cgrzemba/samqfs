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

// ident	$Id: TestMediaGet.java,v 1.11 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.*;

public class TestMediaGet {

    public static void main(String args[]) {
        Discovered discovered;
        LibDev[] libs;
        DriveDev[] drvs;
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
           String mtypes[];
           int i;
           TUtil.loadLib(jnilib);
           System.out.print("\nInitializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done");

           System.out.println("Media.getAvailMediaTypes()");
           mtypes = Media.getAvailMediaTypes(ctx);
           for (i = 0; i < TUtil.objarrLen(mtypes); i++)
               System.out.print(mtypes[i] + " ");
           System.out.println("\nCalling native Media.getLibraries()");
           libs = Media.getLibraries(ctx);
           System.out.println("Back to java ");
           for (i = 0; i < TUtil.objarrLen(libs); i++) {
               if (i == 0)
                    System.out.println("Configured Libraries:");
               System.out.println(libs[i]);
           }

           System.out.println("Calling native Media.getStdDrives()");
           drvs = Media.getStdDrives(ctx);
           System.out.println("Back to java ");

           for (i = 0; i < TUtil.objarrLen(drvs); i++) {
               if (i == 0)
                   System.out.println("Configured Drives:");
               System.out.println(drvs[i]);
           }

           TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
