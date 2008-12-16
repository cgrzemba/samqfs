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

// ident	$Id: TestFSgetall.java,v 1.11 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.*;

public class TestFSgetall {

    public static void main(String args[]) {
        FSInfo fs, fss[];
        String fsnames[];
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
            System.out.println("\nInitializing server connection & library...");
            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);

            System.out.println("Calling native FS.getNames()");
            fsnames = FS.getNames(ctx);
            for (int i = 0; i < TUtil.objarrLen(fsnames); i++)
            System.out.print(fsnames[i] + " ");
            System.out.println("\nCalling native FS.getAll()");
            fss = FS.getAll(ctx);
            System.out.println("Back to java ");
            for (int i = 0; i < fss.length; i++)
                System.out.println(fss[i] + "\n");
            System.out.println("\nnon samq filesystems:\n");
            String[] genfs = FS.getGenericFilesystems(ctx, "ufs");
            if (genfs != null)
                for (int i = 0; i < genfs.length; i++)
                    System.out.println("  " + genfs[i]);

            TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
