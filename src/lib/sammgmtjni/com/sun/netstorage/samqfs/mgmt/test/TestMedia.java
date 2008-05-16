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

// ident	$Id: TestMedia.java,v 1.10 2008/05/16 18:35:31 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.*;

public class TestMedia {


    static void displayMedia(Discovered discovered) {
        LibDev[] libs;
        DriveDev[] drvs;
        libs = discovered.libraries();
        for (int i = 0; i < TUtil.objarrLen(libs); i++)
            System.out.println("Library " + i + ": " + libs[i]);
        drvs = discovered.drives();
        for (int i = 0; i < TUtil.objarrLen(drvs); i++)
            System.out.println("StdDrive " + i + ": " + drvs[i]);
    }

    public static void main(String args[]) {
        Discovered discovered;
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
           System.out.print("\nInitializing server connection & library...");
           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done");

           System.out.println("TEST1: Media.discover(ctx, null)");
           discovered = Media.discover(ctx, null);
           System.out.println("Back to java ");
           System.out.println("Discovered media:");
           displayMedia(discovered);

           System.out.println("\nTEST2: Media.discoverUnused(ctx, null)");
           discovered = Media.discoverUnused(ctx, null);
           System.out.println("Media NOT configured for SAM-FS use:");
           displayMedia(discovered);

           if (discovered.libraries().length == 0)
               System.out.println("\nTESTS3,4,5 skipped (no lib. available)");
           else {
               try {
               LibDev lib = discovered.libraries()[0];
               lib.setPath(lib.getAlternatePaths()[0]);
               lib.setFamilySetName("mylib");
               System.out.println("\nTEST3: adding the first unused library (" +
                   lib.getDevicePath() + "); using first path in altPaths");
               System.out.print(" 3.1 calling Media.addLibrary()...");
               for (int d = 0; d < lib.getDrives().length; d++) {
                   DriveDev drv = lib.getDrives()[d];
                   if (drv.getAlternatePaths() == null) {
                       System.out.println("Drive busy. Using hardcoded paths");
                       if (d == 0) drv.setPath("/dev/rmt/2cbn");
                       if (d == 1) drv.setPath("/dev/rmt/3cbn");
                       drv.setEquipType("lt");
                       drv.setFamilySetName("mylib");
                   } else
                       drv.setPath(drv.getAlternatePaths()[0]);
               }
               System.out.println("Library to be added:" + lib);
               Media.addLibrary(ctx, lib);
               System.out.println("done\n 3.2 calling Media.discoverUnused()");
               displayMedia(Media.discoverUnused(ctx, null));

               System.out.println("TEST4: getLibraryByFSet(ctx, mylib");
               lib = Media.getLibraryByFSet(ctx, "mylib");
               System.out.println("TEST5: Media.removeLibrary (eq " +
                lib.getEquipOrdinal() + ")");
               Media.removeLibrary(ctx, lib.getEquipOrdinal(), false);
               System.out.println(" 5.2 calling Media.discoverUnused()");
               displayMedia(Media.discoverUnused(ctx, null));
               } catch (SamFSException e) {
                   System.out.println(e);
               }
           }
           if (discovered.drives().length == 0)
               System.out.println("\nTESTS6,7,8 skipped (no drives available)");
           else {
               try {
               DriveDev drv = discovered.drives()[0];
               drv.setPath(drv.getAlternatePaths()[0]);
               drv.setFamilySetName("mydrv");
               System.out.println("\nTEST6: adding the first unused drive (" +
                   drv.getDevicePath() + "); using first path in altPaths");
               System.out.print(" 6.1 calling Media.addLibrary()...");
               if (drv.getAlternatePaths() == null) {
                       System.out.println("Drive busy. Please try later");
               } else
                       drv.setPath(drv.getAlternatePaths()[0]);
               System.out.println("Drive to be added:" + drv);
               Media.addStdDrive(ctx, drv);
               System.out.println("done\n 6.2 calling Media.discoverUnused()");
               displayMedia(Media.discoverUnused(ctx, null));

               System.out.println("TEST7: getStdDriveByPath(ctx, mydrv");
               drv = Media.getStdDriveByPath(ctx, drv.getDevicePath());
               System.out.println("TEST8: Media.removeStdDrive (eq " +
                drv.getEquipOrdinal() + ")");
               Media.removeStdDrive(ctx, drv.getEquipOrdinal());
               System.out.println(" 5.2 calling Media.discoverUnused()");
               displayMedia(Media.discoverUnused(ctx, null));
               } catch (SamFSException e) {
                   System.out.println(e);
               }
           }
           TUtil.destroyConn(c);
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
