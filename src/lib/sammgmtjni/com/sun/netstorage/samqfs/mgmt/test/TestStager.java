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

// ident	$Id: TestStager.java,v 1.8 2008/05/16 18:35:31 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.stg.*;
import com.sun.netstorage.samqfs.mgmt.arc.BufDirective;

public class TestStager {

    public static void main(String args[]) {

        StagerParams sp;
        SamFSConnection c;
        BufDirective bdir;

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

           System.out.println("\nTEST1 Calling Stager.getDefaultParams");
           sp = Stager.getDefaultParams(ctx);
           System.out.println("Defaults params: " + sp);

           System.out.println("\nTEST2 Calling Stager.getParams");
           sp = Stager.getDefaultParams(ctx);
           System.out.println("Current params: " + sp);

           System.out.println("\nTEST3 Calling Stager.setParams");
           bdir = new BufDirective("tp", "22", false);
           sp.setBufDirectives(new BufDirective[] { bdir });
           sp.setDrvDirectives(null);
           Stager.setParams(ctx, sp);

           System.out.println("\nTEST4 Calling Stager.getParams");
           sp = Stager.getParams(ctx);
           System.out.println("Current params: " + sp);

           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
