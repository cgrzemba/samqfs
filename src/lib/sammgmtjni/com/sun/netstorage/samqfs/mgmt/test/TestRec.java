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

// ident	$Id: TestRec.java,v 1.10 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.rec.*;

public class TestRec {

    public static void main(String args[]) {

        RecyclerParams rp;
        LibRecParams librp;
        LibRecParams[] librps;
        int actions;

        SamFSConnection c;

        Ctx ctx;
	String jnilib, hostname;

	if (args.length == 0) {
	    jnilib = "sammgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "sammgmtjni";
	    hostname = args[0];
	}
        try {
           TUtil.loadLib(jnilib);
           System.out.print("Initializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done");

           System.out.println("\nTEST1 Calling Recycler.getDefaultParams");
           rp = Recycler.getDefaultParams(ctx);
           System.out.println(rp);

           System.out.println("\nTEST2 Calling Recycler.getAllLibRecParams");
           librps = Recycler.getAllLibRecParams(ctx);
           for (int i = 0; i < librps.length; i++)
                System.out.println("Lib" + i + " params: " + librps[i]);

           System.out.println("\nTEST3 Recycler.getActions()");
           actions = Recycler.getActions(ctx);
           System.out.println("actions: 0x" + Integer.toHexString(actions));

           System.out.println("\nTEST4 Recycler.addActionLabel()");
           Recycler.addActionLabel(ctx);
           actions = Recycler.getActions(ctx);
           System.out.println("actions: 0x" + Integer.toHexString(actions));
           System.out.println("\nTEST5 Recycler.delAction()");
           Recycler.delAction(ctx);
           actions = Recycler.getActions(ctx);
           System.out.println("actions: 0x" + Integer.toHexString(actions));

           System.out.println("\nTEST6 Recycler.addActionLabel()");
           Recycler.addActionExport(ctx, "root");
           actions = Recycler.getActions(ctx);
           System.out.println("actions: 0x" + Integer.toHexString(actions));
           System.out.println("\nTEST7 Recycler.delAction()");
           Recycler.delAction(ctx);
           actions = Recycler.getActions(ctx);
           System.out.println("actions: 0x" + Integer.toHexString(actions));

           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
