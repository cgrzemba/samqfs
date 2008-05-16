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

// ident	$Id: TestNotif.java,v 1.11 2008/05/16 18:35:31 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.adm.NotifSummary;

public class TestNotif {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        NotifSummary[] notifs;
        NotifSummary notif;
        boolean[] subj;

        if (args.length == 0) {
            System.out.println("One arg expected - email address");
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
           System.out.println("done\n");
           ctx = new Ctx(c);

           System.out.println("TEST1: Calling NotifSummary.get(ctx)");
           notifs = NotifSummary.get(ctx);
           System.out.println("NotifSummary objects:");
           for (int i = 0; i < TUtil.objarrLen(notifs); i++)
               System.out.println(" " + notifs[i]);
           if (TUtil.objarrLen(notifs) <= 0)
               System.out.println("no objects found.");

           System.out.println("TEST2: Adding new NotifSummary");
           notif = new NotifSummary("", args[0],
            new boolean[] { true, false, true, false, true });
           NotifSummary.add(ctx, notif);
           System.out.println(" Verify - notifSummary objects:");
           notifs = NotifSummary.get(ctx);
           for (int i = 0; i < TUtil.objarrLen(notifs); i++)
               System.out.println("  " + notifs[i]);

           System.out.println("TEST3: Modifying last object");
           notif = notifs[notifs.length - 1];
           System.out.println(" 2.1: Changing subj field (flipping ARCHINTR)");
           subj = notif.getSubj();
           subj[NotifSummary.NOTIF_SUBJ_ARCHINTR] =
               !subj[NotifSummary.NOTIF_SUBJ_ARCHINTR];
           notif.setSubj(subj);
           NotifSummary.modify(ctx, notif.getEmailAddr(), notif);
           System.out.println(" 2.2: calling NotifSummary.get");
           notifs = NotifSummary.get(ctx);
           System.out.println(" NotifSummary objects:");
           for (int i = 0; i < TUtil.objarrLen(notifs); i++)
               System.out.println("  " + notifs[i]);

          TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
