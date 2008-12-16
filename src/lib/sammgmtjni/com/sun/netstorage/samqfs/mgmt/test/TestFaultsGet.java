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

// ident	$Id: TestFaultsGet.java,v 1.11 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.adm.Fault;
import com.sun.netstorage.samqfs.mgmt.adm.FaultAttr;
import com.sun.netstorage.samqfs.mgmt.adm.FaultSummary;

public class TestFaultsGet {

    public static void main(String args[]) {

        FaultAttr[] faults;
        SamFSConnection c;
        Ctx ctx;
	String jnilib, hostname;
        long errIDs[];

	if (args.length == 0) {
	    jnilib = "sammgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "sammgmtjni";
	    hostname = args[0];
	}
        try {
           int f; // number of faults
           FaultSummary fsum;

           TUtil.loadLib(jnilib);
           System.out.print("Initializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           if (null == ctx)
               System.out.println("null ctx!!");
           System.out.println("done");

           System.out.println("\nTEST0 Fault.getSummary");
           fsum = Fault.getSummary(ctx);
           System.out.println(fsum);

           System.out.println("\nTEST1 Get all faults (Fault.get(ctx, -1,...)");
           faults = Fault.get(ctx, -1, (byte)-1, (byte)-1, -1);
           if (null == faults) {
               System.out.println("No faults. Skipping remaining unit tests");
               TUtil.destroyConn(c);
               System.exit(-1);
           }
           System.out.println("Faults[" + faults.length + "]:");
           for (f = 0; f < faults.length; f++)
                System.out.println(faults[f]);

           System.out.println("\nTEST2 Acknowledging oldest 10 faults");
           if (f < 10)
               errIDs = new long[f];
           else
               errIDs = new long[10];
           for (int i = 0; i < errIDs.length; i++) {
               errIDs[i] = faults[i].getErrID();
               System.out.print(errIDs[i]);
           }
           Fault.ack(ctx, errIDs);
           System.out.println("\nDone. Verifying oldest 10 faults:");
           faults = Fault.get(ctx, -1, (byte)-1, (byte)-1, -1);
           for (f = 0; f < errIDs.length; f++)
               System.out.println(faults[f]);

           System.out.println("\nTEST3 Deleting oldest 10 faults");
           Fault.delete(ctx, errIDs);
           System.out.println("\nDone. Verifying oldest 10 faults:");
           faults = Fault.get(ctx, -1, (byte)-1, (byte)-1, -1);
           for (f = 0; f < errIDs.length; f++)
               System.out.println(faults[f]);

           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
