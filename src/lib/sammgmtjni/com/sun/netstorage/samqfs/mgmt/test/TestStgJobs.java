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

// ident	$Id: TestStgJobs.java,v 1.12 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.stg.job.StagerJob;
import com.sun.netstorage.samqfs.mgmt.stg.job.StgFileInfo;

public class TestStgJobs {

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        StagerJob[] stgJobs;

        if (args.length == 0) {
            hostname = "localhost";
            jnilib = "fsmgmtjnilocal";
        } else {
            hostname = args[0];
            jnilib = "fsmgmtjni";
        }

        try {
           TUtil.loadLib(jnilib);
           System.out.print("Initializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");

           System.out.println("TEST1: Job.getJobs(ctx, Job.TYPE_STAGE)");
           stgJobs = (StagerJob[])Job.getJobs(ctx, Job.TYPE_STAGE);
           System.out.println("Stager jobs:");

           for (int i = 0; i < TUtil.objarrLen(stgJobs); i++) {

               System.out.println(
                   "Stage State flags: " + stgJobs[i].getStateFlag());
               StgFileInfo files[]
                = stgJobs[i].getFilesInfo(ctx, -1, -1, (short)0, false);
               long nfiles;
               System.out.println(" " + stgJobs[i]);
               nfiles = stgJobs[i].getNumberOfFiles(ctx);
               System.out.println(" " + nfiles + " files:");
               for (int j = 0; j < TUtil.objarrLen(files); j++)
                    System.out.println(" " + files[j]);
           }

           TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
