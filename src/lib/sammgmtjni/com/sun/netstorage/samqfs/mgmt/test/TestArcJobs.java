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

// ident	$Id: TestArcJobs.java,v 1.9 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.arc.job.ArCopyJob;
import com.sun.netstorage.samqfs.mgmt.arc.job.ArFindJob;

public class TestArcJobs {

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        ArCopyJob[] copyJobs;
        ArFindJob[] findJobs;

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

           System.out.println("TEST1: Job.getJobs(ctx, Job.TYPE_ARCOPY)");
           copyJobs = (ArCopyJob[])Job.getJobs(ctx, Job.TYPE_ARCOPY);
           System.out.println("Copy jobs:");
           for (int i = 0; i < TUtil.objarrLen(copyJobs); i++)
               System.out.println(" " + copyJobs[i]);

           System.out.println("\nTEST2: Job.getJobs(ctx, Job.TYPE_ARFIND)");
           findJobs = (ArFindJob[])Job.getJobs(ctx, Job.TYPE_ARFIND);
           System.out.println("Scan jobs:");
           for (int i = 0; i < TUtil.objarrLen(findJobs); i++)
               System.out.println(" " + findJobs[i]);

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
