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

// ident	$Id: TestArcGlob.java,v 1.9 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.ArGlobalDirective;

public class TestArcGlob {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        ArGlobalDirective arglob;

        if (args.length == 0) {
            hostname = "localhost";
            jnilib = "sammgmtjnilocal";
        } else {
            hostname = args[0];
            jnilib = "fsmgmtjni";
        }

        try {
           System.out.print("Loading native lib (" + jnilib + ")...");
           System.loadLibrary(jnilib);
           System.out.print("done\n" +
            "Initializing server connection & library...");

           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");
           System.out.println("\nTEST1: " +
            "Calling Archiver.getDefaultArGlobalDirective");
           arglob = Archiver.getDefaultArGlobalDirective(ctx);
           System.out.println("Back to java ");
           System.out.println(arglob + "\n");

           System.out.println(
               "\nTEST2a: Calling Archiver.getArGlobalDirective");
           arglob = Archiver.getArGlobalDirective(ctx);
           System.out.println("Back to java ");
           System.out.println(arglob);

           System.out.println(
               "\nTEST3: Calling Archiver.setArGlobalDirective\n");
           arglob.resetInterval();
           Archiver.setArGlobalDirective(ctx, arglob);
           System.out.println("Back to java ");


           System.out.println("Calling Archiver.getArGlobalDirective");
           arglob = Archiver.getArGlobalDirective(ctx);
           System.out.println("Back to java ");
           System.out.println(arglob);

	   /* toggle scanlistsquash value */
           System.out.println("\nTEST4: setting scan squash to " +
		   (arglob.getScanListSquash() ? "off" : "on"));

	   arglob.setScanListSquash(!arglob.getScanListSquash());
           arglob.setInterval(300); // seconds


           Archiver.setArGlobalDirective(ctx, arglob);
           System.out.println("Back to java ");
           System.out.println("Calling Archiver.getArGlobalDirective");
           arglob = Archiver.getArGlobalDirective(ctx);
           System.out.println("Back to java");
           System.out.println(arglob);

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
		System.out.println("exception encountered");
                System.out.println(e);
        }
    }
}
