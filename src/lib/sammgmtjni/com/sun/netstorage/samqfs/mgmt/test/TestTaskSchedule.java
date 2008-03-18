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

// ident	$Id: TestTaskSchedule.java,v 1.4 2008/03/17 14:44:04 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.adm.TaskSchedule;

public class TestTaskSchedule {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib = "fsmgmtjni", hostname = "localhost";
        int i;

        if (args.length == 0) {
            jnilib += "local";
        } else if (args.length == 1) {
            hostname = args[0];
        }

        try {
            System.out.print("Loading native lib (" + jnilib + ")...");
            System.loadLibrary(jnilib);
            System.out.print("done\n" +
            "Initializing server connection to "
                + hostname + " & library" + jnilib);

            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);
            System.out.println("done\n");

            /* ********* Test code begins here **************** */

            String name = null;
            System.out.print("Calling TaskSchedule.getSchedules()...");
            String[] scheds = TaskSchedule.getTaskSchedules(ctx);
            System.out.println("done");
            if (scheds != null) {
               System.out.println("Got some data...");
               for (i = 0; i < scheds.length; i++) {
                   System.out.println(scheds[i]);
               }
            } else {
               System.out.println("Got null");
            }
            String[] schs = TaskSchedule.getTaskSchedules(ctx);
            System.out.println("done");
            if (schs != null) {
               System.out.println("Got some data...");
               for (i = 0; i < schs.length; i++) {
                   System.out.println(schs[i]);
               }
            } else {
               System.out.println("Got null");
            }


            System.out.println("\nSetting the schedule for DD");
            TaskSchedule.setTaskSchedule(
                ctx, "task=DD, starttime = 200609221200, " +
                     "periodicity=40h, duration=10h");
            System.out.println("\nSet the schedule for DD");

            TaskSchedule.removeTaskSchedule(ctx, "task=RC");
            System.out.println("\nremoved the schedule for RC");
            for (i = 0; i < schs.length; i++) {
               if (schs[i].indexOf("task=RC") != -1) {
                   TaskSchedule.setTaskSchedule(ctx, schs[i]);
                   System.out.println("\nadded back the schedule for RC");
               }
            }

            System.out.print("destroying connection...");
            c.destroy();
            System.out.println("done");
        } catch (SamFSException e) {
            tem.out.println("SAM ErrMsg = " + e.getMessage());
        }
    }System.out.println(e);
            System.out.println("SAM errno = " + e.getSAMerrno());
            Sys
}
