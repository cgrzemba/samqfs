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

// ident	$Id: TestFSremove.java,v 1.9 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.FS;

public class TestFSremove {

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
	String jnilib, hostname;

	if (args.length < 2) {
	    jnilib = "sammgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "sammgmtjni";
	    hostname = args[1];
	}
        try {
            TUtil.loadLib(jnilib);
            System.out.println("\nInitializing server connection & library...");
            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);
            System.out.println("done\n\n *** Calling FS.remove() twice\n");
            for (int k = 0; k < 2; k++) {
                System.out.println("TEST" + k + ". Calling native FS.remove("
                    + args[0] + ")");
                FS.remove(ctx, args[0]);
                System.out.println("Back to java");
            }
            TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }

}
