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

// ident	$Id: TestCParams.java,v 1.11 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.CopyParams;
import com.sun.netstorage.samqfs.mgmt.rec.RecyclerParams;

public class TestCParams {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        CopyParams cp;
        CopyParams[] cps; // all
        RecyclerParams rp;


        if (args.length == 0) {
            System.out.println("One arg expected by TEST3 - copy name");
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
           TUtil.loadLib(jnilib);
           System.out.print("Initializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");

           System.out.println("TEST1: Calling Archiver.getDefaultCopyParams");
           cp = Archiver.getDefaultCopyParams(ctx);
           System.out.println("Back to java ");
           System.out.println(cp + "\n");

           System.out.println("TEST2: Calling Archiver.getCopyParams");
           cps = Archiver.getCopyParams(ctx);
           System.out.println("Back to java ");
           for (int i = 0; i < TUtil.objarrLen(cps); i++)
               System.out.println(cps[i]);

           System.out.println("TEST3: Calling Archiver.getCopyParamsForCopy"
           + "(ctx," + args[0] + ")");
           cp = Archiver.getCopyParamsForCopy(ctx, args[0]);
           System.out.println("Back to java ");
           System.out.println(cp);

           System.out.println("TEST4: changing CopyParams for set " + args[0]);
           cp.setBufSize(16);
           cp.setDrives(2);
           cp.setUnarchAge(true);
           rp = cp.getRecyclerParams();
           rp.setHWM(95);
           rp.setDatasize("102400");
           rp.setEmailAddr("andrei");
           cp.setRecyclerParams(rp);
           System.out.println(cp);
           Archiver.setCopyParams(ctx, cp);
           System.exit(0);
           System.out.println("TEST5: rereading CopyParams for " + args[0]);
           cp = Archiver.getCopyParamsForCopy(ctx, args[0]);
           System.out.println(cp);

           System.out.println("TEST6: resetting modified params to default");
           cp.resetBufSize();
           cp.resetDrives();
           System.out.println("CP after resets, before calling set:\n" + cp);
           Archiver.setCopyParams(ctx, cp);
           System.out.println("Reread cp" +
            Archiver.getCopyParamsForCopy(ctx, args[0]));

           TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
