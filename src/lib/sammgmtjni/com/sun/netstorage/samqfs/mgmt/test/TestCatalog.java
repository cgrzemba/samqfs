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

// ident	$Id: TestCatalog.java,v 1.9 2008/03/17 14:44:03 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;


import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.media.Media;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;

public class TestCatalog {

    public static void displayCatEntries(CatEntry[] catents) {
        if (null == catents) {
            System.out.println("null CatEntry array ");
        } else {
            System.out.println(catents.length + " entries:");
            for (int i = 0; i < catents.length; i++)
                System.out.println(catents[i]);
        }
    }

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
	String jnilib, hostname;
        CatEntry[] catents;
        CatEntry catent;

	if (args.length == 1) {
	    jnilib = "sammgmtjnilocal";
	    hostname = "localhost";
	} else {
	    jnilib = "sammgmtjni";
	    hostname = args[1];
	}
        try {
           int libEq;
           TUtil.loadLib(jnilib);
           System.out.print("Initializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done");

           System.out.println("\nTEST1 Media.getCatEntries for regexp "
            + args[0]);
           catents = Media.getCatEntriesForRegexp(ctx, args[0], -1, -1,
               Media.VSN_NO_SORT, false);
           displayCatEntries(catents);
           if (null == catents[0]) {
               System.out.println("Skipping remaining tests");
               TUtil.destroyConn(c);
           }
           libEq = catents[0].getLibraryEqu();

           System.out.println("\nTEST2 Media.getAllCatEntries for libEq " +
               libEq);
           catents = Media.getAllCatEntriesForLib(ctx,
               libEq, -1, -1, Media.VSN_NO_SORT, false);
           displayCatEntries(catents);

           System.out.println("\nTEST3 Media.getCatEntries for libEq " +
               libEq + "(slots: 5 - 10)");
           catents = Media.getCatEntriesForLib(ctx,
               libEq, 5, 10, Media.VSN_NO_SORT, false);
           displayCatEntries(catents);

           System.out.println("\nTEST4 Media.getCatEntries for VSN " +
               catents[0].getVSN());
           catents = Media.getCatEntriesForVSN(ctx, catents[0].getVSN());
           displayCatEntries(catents);

           System.out.println("\nTEST5 Media.getCatEntry for slot " +
                catents[0].getSlot() + " part " + catents[0].getPartition());
           catent = Media.getCatEntryForSlot(ctx, libEq, catents[0].getSlot(),
            catents[0].getPartition());
           System.out.println(catent);

           TUtil.destroyConn(c);

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
