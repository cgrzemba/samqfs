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

// ident	$Id: TestAUDisco.java,v 1.12 2008/03/17 14:44:03 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.AU;

import java.util.ArrayList;

public class TestAUDisco {

    public static void main(String args[]) {
        AU aus[];
        ArrayList allslices;
        String ovrlapslices[];
        int type = -1;
        SamFSConnection conn;
        try {
            int i;
            Ctx ctx;

            TUtil.loadLib("fsmgmtjni");
            System.out.println("creating SamFSConnection w/ " + args[0]);
            conn = SamFSConnection.getNew(args[0]);
            ctx = new Ctx(conn);
            System.out.println("connection created - " + conn);
            conn.setTimeout(180);
            System.out.println("timeout set - " + conn);

            System.out.println("TEST1: AU.discoverAUs()");
            aus = AU.discoverAUs(ctx);
            allslices = new ArrayList();
            for (i = 0; i < TUtil.objarrLen(aus); i++) {
                type = aus[i].getType();
		System.out.println(aus[i]);
	    }

            System.out.println("\n\nTEST2: AU.discoverAvailAUs()\n");
            aus = AU.discoverAvailAUs(ctx);
            allslices = new ArrayList();
            for (i = 0; i < TUtil.objarrLen(aus); i++) {
                type = aus[i].getType();
		System.out.println(aus[i]);
                if (type == AU.SLICE)
                    allslices.add(aus[i].getPath());
                }
            if (i == 0)
                System.out.println("no available AU-s found!");
            else {
                System.out.println("\nTEST3: Checking overlapping slices");
                ovrlapslices = AU.checkSlicesForOverlaps(ctx,
                    (String[])allslices.toArray(new String[0]));
                for (i = 0; i < TUtil.objarrLen(ovrlapslices); i++)
                    System.out.println("\t" + ovrlapslices[i]);
                if (i == 0)
                    System.out.println("no overlapping slices found!");
            }
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
