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

// ident	$Id: TestVSNPool.java,v 1.9 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.VSNOp;
import com.sun.netstorage.samqfs.mgmt.arc.VSNPool;

public class TestVSNPool {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        VSNPool pool;
        VSNPool[] pools; // all


        if (args.length == 0) {
            System.out.println("One arg expected by TEST2 - pool name");
            System.exit(-1);
        }
        if (args.length == 1) {
            hostname = "localhost";
            jnilib = "sammgmtjnilocal";
        } else {
            hostname = args[1];
            jnilib = "sammgmtjni";
        }

        try {
           System.out.print("Loading native lib (" + jnilib + ")...");
           System.loadLibrary(jnilib);
           System.out.print("done\n" +
            "Initializing server connection & library...");

           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");

           System.out.println("TEST1: Calling VSNOp.getPools");
           pools = VSNOp.getPools(ctx);
           System.out.println("Back to java ");
           for (int i = 0; i < TUtil.objarrLen(pools); i++)
               System.out.println(pools[i]);

           System.out.println("TEST2: Calling VSNOp.addPool(ctx," +
            args[0] + ")");
           pool = new VSNPool(args[0], "lt",
            new String[] { "myvsn1", "myvsn2" });
           VSNOp.addPool(ctx, pool);
           System.out.println("Back to java ");

           System.out.println("TEST3: Calling VSNOp.getPool(ctx," +
            args[0] + ")");
           VSNOp.getPool(ctx, args[0]);
           System.out.println("Back to java ");
           System.out.println(pool);

           System.out.println("TEST4: Modifying VSNPool " + args[0]);
           pool.setVSNs(new String[] {"myvsn3", "myvsn4"});
           VSNOp.modifyPool(ctx, pool);
           System.out.println("Back to java.");
           System.out.println("Modified VSNPool (calling getPool()): ");
           System.out.println(VSNOp.getPool(ctx, args[0]).toString());

           System.out.println("TEST5: Removing VSNPool " + args[0]);
           VSNOp.removePool(ctx, args[0]);
           System.out.println("Back to java.");
           System.out.println("TEST6: Getting removed pool (error expected)");
           pool = VSNOp.getPool(ctx, args[0]);

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
