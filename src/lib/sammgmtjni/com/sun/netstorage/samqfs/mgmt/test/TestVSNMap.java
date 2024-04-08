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

// ident	$Id: TestVSNMap.java,v 1.10 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.VSNOp;
import com.sun.netstorage.samqfs.mgmt.arc.VSNMap;


public class TestVSNMap {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        VSNMap vmap;
        VSNMap[] vmaps; // all

        if (args.length == 0) {
            System.out.println("One arg expected by TEST2 - copy name");
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
           TUtil.loadLib(jnilib);
           System.out.print("Initializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");

           System.out.println("TEST1: Calling VSNOp.getMaps");
           vmaps = VSNOp.getMaps(ctx);
           System.out.println("Back to java ");
           for (int i = 0; i < TUtil.objarrLen(vmaps); i++)
               System.out.println(vmaps[i]);
           System.out.println("TEST3: Calling VSNOp.getMap(ctx," +
            args[0] + ")");
           vmap = VSNOp.getMap(ctx, args[0]);
           System.out.println("Back to java ");
           System.out.println(vmap);

           System.out.println("TEST4: Modifying VSNMap " + args[0]);
           vmap.setVSNNames(new String[] {"myvsn3", "myvsn4"});
           VSNOp.modifyMap(ctx, vmap);
           System.out.println("Back to java.");
           System.out.println("Modified VSNMap (calling getMap()): ");
           System.out.println(VSNOp.getMap(ctx,
					   vmap.getCopyName()).toString());
           System.out.println("TEST5: Removing VSNMap[" + args[0] + "]");
           vmap.setVSNNames(new String[] {"myvsn3", "myvsn4"});
           VSNOp.removeMap(ctx, args[0]);
           System.out.println("Back to java.");

           System.out.println("TEST6: Getting removed map (error expected)");
           VSNOp.removeMap(ctx, args[0]);

           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
