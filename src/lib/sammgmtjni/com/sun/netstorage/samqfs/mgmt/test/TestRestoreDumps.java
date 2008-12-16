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

// ident	$Id: TestRestoreDumps.java,v 1.13 2008/12/16 00:08:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import java.util.Calendar;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.*;
import com.sun.netstorage.samqfs.mgmt.FileUtil;

public class TestRestoreDumps {

    public static void main(String args[]) {

        SamFSConnection c;
        String jnilib, hostname;
        String restrictions = "";

        if (args.length == 0) {
            System.out.println("One arg expected - filesystem name");
            System.exit(-1);
        }
        if (args.length == 1) {
            hostname = "localhost";
            jnilib = "sammgmtjnilocal";
        } else {
            hostname = args[1];
            jnilib = "sammgmtjni";
        }

        try {
            TUtil.loadLib(jnilib);
            System.out.println("\nInitializing server connection & library...");

            c = SamFSConnection.getNew(hostname);
            System.out.println("done\n");

            // System.out.println("\nGetting restore status information...");
            System.out.println("Restore status is " +
                Restore.getStatus(new Ctx(c)));
            System.out.println();

            System.out.println("\nGetting dump parameters...");
            String defaultParams = Restore.getParams(new Ctx(c), args[0]);
            System.out.println("Dump parameters[ " + defaultParams + " ]");
            System.out.println();

            System.out.println("\nSetting dump parameters...");
            String params =
                "location=.csd, names=samfs1-%Y-%m-%D-%R, frequency=" +
                Calendar.getInstance().getTimeInMillis() +
                "P86400, retention=0P0, logfile=/tmp/dump_samfs1";
            Restore.setParams(new Ctx(c), "samfs1", params);
            System.out.println();
            System.out.println("\nGetting dump parameters...");
            System.out.println("Dump parameters[ " +
                Restore.getParams(new Ctx(c), args[0]) + " ]");

            System.out.println("\nRestoring default parameters");
            Restore.setParams(new Ctx(c), "samfs1", defaultParams);
            System.out.println();

            System.out.println("\nListing dumps...");
            String [] dump = Restore.getDumps(new Ctx(c), args[0]);
            for (int i = 0; i < TUtil.objarrLen(dump); i++) {
                System.out.print(dump[i] + "\n");
            }
            System.out.println();
            try {
            System.out.println("\nGetting dump status...");
            String[] dumpStatus = Restore.getDumpStatus(
                new Ctx(c), args[0], dump);
            for (int n = 0; n < TUtil.objarrLen(dumpStatus); n++) {
                System.out.println(dump[n] + " : " + dumpStatus[n]);
                // TBD
                // If status is compressed, decompress the dump

            }
            // TBD - Take dump

            } catch (SamFSException e) {
                System.out.print("Error " + e.getMessage() + "\n\n");
            }
            for (int i = 0; i < TUtil.objarrLen(dump); i++) {

                System.out.println(
                    "\nVersions of filesystem " + args[0] +
                    " from dump " + dump[i] + " ...");
                try {
                    String [] files = Restore.listVersions(
                        new Ctx(c), args[0], dump[i], 100, ".", restrictions);
                    for (int j = 0; j < files.length; j++) {
                        System.out.print(files[j] + "\n");
                    }
                } catch (SamFSException e) {
                    System.out.print("Error " + e.getMessage() + "\n\n");
                }

                try {
                    System.out.println("\nVersion details from " +
                        dump[i] +
                        " for Rshared/d2/d12/file96 in filesystem " +
                        args[0] + "...");
                    String [] versionDetails =
                        Restore.getVersionDetails(
                            new Ctx(c), args[0],
                            dump[i], "./Rshared/d2/d12/file96");
                    for (int k = 0; k < versionDetails.length; k++) {
                        System.out.println(versionDetails[k]);
                    }
                } catch (SamFSException e) {
                    System.out.println("Error " + e.getMessage() + "\n\n");
                }

                try {
                    System.out.println("\nSearch versions...");
                    String searchId =
                        Restore.searchFiles(
                            new Ctx(c), args[0], dump[i],
                            100, "./Rshared/d2/d12/file96", restrictions);
                    System.out.println("Search id: " + searchId);

                } catch (SamFSException e) {
                    System.out.println("Error " + e.getMessage() + "\n\n");
                }

            }

            System.out.println("\nList directory...");
            String [] dir = FileUtil.getDirEntries(
                new Ctx(c), 100, args[0], restrictions);
            for (int i = 0; i < TUtil.objarrLen(dir); i++) {
                System.out.print(dir[i] + "\n");
            }
            String[] fileDetails = FileUtil.getFileDetails(
                new Ctx(c), args[0], dir);
            for (int d = 0; d < TUtil.objarrLen(fileDetails); d++) {
                System.out.println("File Details: " + fileDetails[d]);
            }
            System.out.println("\nGetting file status...");
            int [] fileStat = FileUtil.getFileStatus(new Ctx(c), dir);
            for (int i = 0; i < fileStat.length; i++) {
                System.out.print(fileStat[i] + "\n");
            }

            System.out.println("\nGetting search results");
            String [] searchResults = Restore.getSearchResults(new Ctx(c),
                args[0]);
            for (int p = 0; p < TUtil.objarrLen(searchResults); p++) {
                System.out.print(searchResults[p] + "\n");
            }


            String dumpName = "samfs1-2005-01-07-12:30";
            String [] filePath = new String[1];
            filePath[0] = "./Rshared/d2/d12/file98";
            String [] destination = new String[1];
            destination[0] = "/samfs1/Rshared/d2/d12/file98";
            int [] copies = new int[1];
            for (int j = 0; j < copies.length; j++) {
                 copies[j] = 2000; // Dont stage
            }
            System.out.println("Restoring inode for file " + filePath[0] +
                "from " + dumpName + " to " + destination[0] + "...");
            String restoreId = Restore.restoreInodes(
                new Ctx(c), args[0], dumpName, filePath, destination,
                copies, 0);
            System.out.println("Restore inode id: " + restoreId);

            System.out.println("Taking a dump of " + args[0] + " now");
            String dumpId = Restore.takeDump(
                new Ctx(c), args[0], "/tmp/dump");
            System.out.println("Started taking a dump of " + args[0] +
                " to /tmp/dump. Dump ID is "  + dumpId);

            System.out.print("destroying connection...");
            c.destroy();
            System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
