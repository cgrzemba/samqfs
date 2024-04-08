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

// ident	$Id: TestMediaOp.java,v 1.11 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.*;

public class TestMediaOp {

    public static void main(String args[]) {
        Discovered discovered;
        SamFSConnection c;
        Ctx ctx;
	String jnilib, hostname;

	if (args.length == 1) {
	    jnilib = "sammgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "sammgmtjni";
	    hostname = args[1];
	}
        System.out.println(args[0] + "," + args[1]);
        try {
           TUtil.loadLib(jnilib);
           System.out.print("\nInitializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done");

           try {
           System.out.println("TEST1: Media.tapeLabel() (should fail)");
           Media.tapeLabel(ctx, -1, -1, -1, "VSNnew", null, -1, false, false);
           System.out.println("Back to java ");
           } catch (SamFSException e) { System.out.println(e); }

           try {
           System.out.println("TEST2: Media.importCartridge() (should fail)");
           Media.importCartridge(ctx, -1, null);
           System.out.println("Back to java ");
           } catch (SamFSException e) { System.out.println(e); }

           try {
           DriveDev drv;
           System.out.println("TEST3: Media.isVSNLoaded(" + args[0] + ")");
           drv = Media.isVSNLoaded(ctx, args[0]);
           System.out.println((drv == null) ? "not loaded" : drv.toString());
           } catch (SamFSException e) { System.out.println(e); }
           TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
