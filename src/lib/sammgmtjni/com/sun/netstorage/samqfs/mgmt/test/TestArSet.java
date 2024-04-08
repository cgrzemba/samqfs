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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: TestArSet.java,v 1.10 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.arc.Copy;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;

public class TestArSet {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib = "fsmgmtjni", hostname = "localhost";
        Criteria crit;
        Copy copy;
        String[] names;
        Criteria[] crits, crits2;
        ArFSDirective arfsd;
        int i;

        if (args.length == 0) {
            jnilib += "local";
        } else if (args.length == 1) {
            hostname = args[0];
        }

        try {
           System.out.print("Loading native lib (" + jnilib + ")...");
           System.loadLibrary(jnilib);
           System.out.print("done\n" +
            "Initializing server connection & library...");

           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");

           /* ********* Test code begins here **************** */

           String name = null;
           System.out.print("Calling Archiver.getArSets()...");
           ArSet[] set = Archiver.getArSets(ctx);
           System.out.println("done");
           if (set != null) {
               System.out.println("Got some data...");
               for (i = 0; i < set.length; i++) {
                   if (i == 1) {
                       // Save the second name for future use
                       name = set[i].getArSetName();
                       System.out.println("Saving set name: " + name);
                   }
                   System.out.println(set[i]);
               }
           } else {
               System.out.println("Got null");
           }

           if (name != null) {
               System.out.print("Calling Archiver.getArSet()...");
               ArSet s = Archiver.getArSet(ctx, name);
               System.out.println("done");
               if (s != null) {
                   System.out.println("Got: " + s);
               } else {
                   System.out.println("Got null");
               }

               try {
                  System.out.print("Calling Archiver.createArSet()...");
                  Archiver.createArSet(ctx, s);
                  System.out.println("done");
               } catch (SamFSException e) {
                    System.out.println(e);
               }

               try {
                   System.out.print("Calling Archiver.modifyArSet()...");
                   Archiver.modifyArSet(ctx, s);
                   System.out.println("done");
               } catch (SamFSException e) {
                    System.out.println(e);
               }

               System.out.print("Calling Archiver.deleteArSet()...");
               Archiver.deleteArSet(ctx, name);
               System.out.println("done");
           }

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
                System.out.println("SAM errno = " + e.getSAMerrno());
                System.out.println("SAM ErrMsg = " + e.getMessage());
        }
    }
}
