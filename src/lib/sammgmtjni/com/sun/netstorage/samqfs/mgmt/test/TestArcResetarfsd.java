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

// ident	$Id: TestArcResetarfsd.java,v 1.8 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;


public class TestArcResetarfsd {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        ArFSDirective arfsd;

        if (args.length == 0) {
            System.out.println("At least one arg expected - filesystem name");
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

           System.out.println("TEST1: Calling Archiver.resetArFSDirective" +
             "(ctx," + args[0] + ")");
           Archiver.resetArFSDirective(ctx, args[0]);
           System.out.println("Back to java ");

           System.out.println("TEST2: Calling Archiver.getArFSDirective"
           + "(ctx," + args[0] + ")");
           arfsd = Archiver.getArFSDirective(ctx, args[0]);
           System.out.println("Back to java ");
           System.out.println(arfsd);

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
