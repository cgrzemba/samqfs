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

// ident	$Id: TestRel.java,v 1.9 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.rel.*;

public class TestRel {
    public static void main(String args[]) {

        ReleaserDirective rel;
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
           if (null == ctx)
               System.out.println("null ctx!!");
           System.out.println("done");

           System.out.println("\nCalling native getDefaultDirective");
           if (null == (rel = Releaser.getDefaultDirective(ctx)))
                System.out.println("Back to java (NULL)");
           else {
                System.out.println("Back to java (NOT NULL)");
                System.out.println("Defaults: log: " + rel.getLogFile() +
                    " minAge: " + rel.getMinAge());
           }

           System.out.println("\nCalling native getGlobalDirective");
           if (null == (rel = Releaser.getGlobalDirective(ctx)))
                System.out.println("Back to java (NULL)");
           else {
                System.out.println("Back to java (NOT NULL)");
                System.out.println("Global: log: " + rel.getLogFile() +
                    " minAge: " + rel.getMinAge());
           }

           rel.setLogFile("/tmp/setlogvalue");
           rel.setMinAge(45);
           System.out.println("\nCalling native setGlobalDirective(" +
            rel.getLogFile() + "," + rel.getMinAge() + ")");
           Releaser.setGlobalDirective(ctx, rel);
           System.out.println("Back to java ");

           System.out.println("\nCalling native getGlobalDirective");
           if (null == (rel = Releaser.getGlobalDirective(ctx)))
                System.out.println("Back to java (NULL)");
           else {
                System.out.println("Back to java (NOT NULL)");
                System.out.println("Global: log: " + rel.getLogFile() +
                    " minAge: " + rel.getMinAge());
           }

           rel.setLogFile("/tmp/setlogvalue");
           rel.setMinAge(-45);
           System.out.println("\nCalling native setGlobalDirective(" +
            rel.getLogFile() + "," + rel.getMinAge() + ")");
           Releaser.setGlobalDirective(ctx, rel);
           System.out.println("Back to java ");
           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
