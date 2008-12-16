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

// ident	$Id: TestFSgrow.java,v 1.10 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.fs.FS;
import com.sun.netstorage.samqfs.mgmt.fs.AU;
import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;
import java.io.InputStreamReader;
import java.io.IOException;

public class TestFSgrow {
        public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        FSInfo fs;
        AU[] aus;
        DiskDev[] meta = new DiskDev[2]; // add 2 metadata devices
        DiskDev[] data = new DiskDev[2]; // add 2 data devices

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
            DiskDev[] dsks = new DiskDev[4];
           System.out.print("Loading native lib (" + jnilib + ")...");
           System.loadLibrary(jnilib);
           System.out.print("done\n" +
            "Initializing server connection & library...");

           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");

           System.out.print("getting FSInfo for " + args[0] + "...");
           fs = FS.get(ctx, args[0]);
           System.out.println("done\n");

           System.out.print("discovering aus...");
           aus = AU.discoverAvailAUs(ctx);
           System.out.println("done\n\nChoosing some AU-s ");

           for (int i = aus.length - 1, j = 0; (i >= 0) && (j < 4); i--)
               if (!aus[i].getPath().endsWith("s2")) {
                   dsks[j++] = new DiskDev(aus[i]);
                   System.out.println(aus[i]);
               }
           meta[0] = dsks[0];
           meta[1] = dsks[1];
           data[0] = dsks[2];
           data[1] = dsks[3];
           System.out.println("adding the following slices:"
           + "\n meta: " + meta[0] + "\n       " + meta[1]
           + "\n data: " + data[0] + "\n       " + data[1] + "\n continue?");
           try {
           if ('n' != (new InputStreamReader(System.in)).read()) {

              System.out.println("Calling native FS.grow()");
              FS.grow(ctx, fs, meta, data, null);
              System.out.println("Back to java ");
           }} catch (IOException e) { e.printStackTrace(); }

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
