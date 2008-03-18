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

// ident	$Id: TestStkMedia.java,v 1.8 2008/03/17 14:44:04 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.*;

public class TestStkMedia {

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
           System.out.print("\nInitializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done");

           StkClntConn stkConns [] = new StkClntConn[1];
           stkConns[0] = new StkClntConn("ns-east-63", "50014");

           System.out.println("TEST1: Media.discoverStk(ctx, stkClntConn)");
           LibDev[] libs = Media.discoverStk(ctx, stkConns);
           System.out.println("Back to java ");
           System.out.println("Discovered stk media:");
           for (int i = 0; i < TUtil.objarrLen(libs); i++)
               System.out.println("Library " + i + ": " + libs[i]);

           System.out.println(
               "TEST2: Media.getPhyConfForStkLib(ctx, stkClntConn)");
           StkPhyConf phyConf =
               Media.getPhyConfForStkLib(ctx, "sg", stkConns[0]);
           System.out.println("Back to java ");

           System.out.println("Got lsms:");
           int[] lsms = phyConf.getLsms();
           for (int i = 0; i < lsms.length; i++)
               System.out.println("LSM " + i + ": " + lsms[i]);

           System.out.println("Got panels:");
           int[] panels = phyConf.getPanels();
           for (int i = 0; i < panels.length; i++)
               System.out.println("Panel " + i + ": " + panels[i]);

           System.out.println("minRow = " + phyConf.getMinRow() +
                ", maxRow = " + phyConf.getMaxRow() +
                ", minCol = " + phyConf.getMinCol() +
                ", maxCol = " + phyConf.getMaxCol());

           System.out.println("Got Pools:");
           StkPool[] pools = phyConf.getPools();
           for (int i = 0; i < TUtil.objarrLen(pools); i++)
               System.out.println("Pool " + i + ": " + pools[i]);

           System.out.println(
               "TEST3: Media.getVSNsForStkLib(ctx, stkClntConn)");
           String filter = "access_date=10-28-05, equ_type=li, filter_type=4";
           StkVSN[] vsns = Media.getVSNsForStkLib(ctx, stkConns[0], filter);
           System.out.println("Back to java ");
           System.out.println("Got VSNs:");
           for (int i = 0; i < TUtil.objarrLen(vsns); i++)
               System.out.println("VSN " + i + ": " + vsns[i]);

           System.out.println("TEST4: Media.getVSNNamesForStkLib(ctx, li)");
           String[] vsnNames = Media.getVSNNamesForStkLib(ctx, "li");
           System.out.println("Back to java ");
           System.out.println("Got VSN Names:");
           for (int i = 0; i < TUtil.objarrLen(vsnNames); i++)
               System.out.println("VSN " + i + ": " + vsnNames[i]);


           TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
