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

// ident	$Id: TestActArcCfg.java,v 1.10 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;

public class TestActArcCfg {

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        String[] warnMsgs;

        if (args.length == 0) {
            hostname = "localhost";
            jnilib = "sammgmtjnilocal";
        } else {
            hostname = args[0];
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

        try {
           System.out.println("\nTEST1: Calling Archiver.activateCfg");
           warnMsgs = Archiver.activateCfg(ctx);
           System.out.println("Back to java ");
           if (null == warnMsgs)
               System.out.println("Archiver configuration activated");
           else {
               System.out.println("Warnings:");
               for (int i = 0; i < warnMsgs.length; i++)
                    System.out.println(" " + warnMsgs[i]);
           }
        } catch (SamFSMultiMsgException me) {
            for (int i = 0; i < me.getMessages().length; i++)
                System.out.println(" " + me.getMessages()[i]);
        }

        try {
           System.out.println("\nTEST2: Calling Archiver.activateCfgThrowWarn");
           Archiver.activateCfgThrowWarnings(ctx);
           System.out.println("Back to java ");
        } catch (SamFSMultiMsgException me) {
            for (int i = 0; i < me.getMessages().length; i++)
                System.out.println(" " + me.getMessages()[i]);
        } catch (SamFSWarnings warns) {
            for (int i = 0; i < warns.getMessages().length; i++)
                System.out.println(" " + warns.getMessages()[i]);
        }


            System.out.print("destroying connection...");
            c.destroy();
            System.out.println("done");
        } catch (SamFSException e) {
            e.printStackTrace();
        }

    }
}
