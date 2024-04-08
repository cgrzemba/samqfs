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

// ident	$Id: TestRelJobs.java,v 1.9 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.rel.ReleaserJob;

public class TestRelJobs {

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        ReleaserJob[] relJobs = null;

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

           System.out.println("Calling Job.getJobs(ctx, Job.TYPE_RELEASE) until"
               + " at least one job is found");
           while (TUtil.objarrLen(relJobs) <= 0)
               relJobs = (ReleaserJob[])Job.getJobs(ctx, Job.TYPE_RELEASE);
           System.out.println("Releaser jobs:");
           for (int i = 0; i < TUtil.objarrLen(relJobs); i++)
               System.out.println(" " + relJobs[i]);

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
