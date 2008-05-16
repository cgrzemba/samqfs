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

// ident	$Id: TestReports.java,v 1.5 2008/05/16 18:35:31 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.adm.Report;

public class TestReports {

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;

	if (args.length == 0) {
	    jnilib = "fsmgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "fsmgmtjni";
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

           System.out.println("\nTEST0 Report.getAll");
           String[] reports = Report.getAll(ctx);
           System.out.println("Back to java ");
           for (int i = 0; i < reports.length; i++)
                    System.out.println(reports[i] + "\n");

           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
