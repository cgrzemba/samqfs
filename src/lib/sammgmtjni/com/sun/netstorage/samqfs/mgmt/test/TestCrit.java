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

// ident	$Id: TestCrit.java,v 1.10 2008/12/16 00:08:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.arc.Copy;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;

public class TestCrit {

    public static void main(String args[]) {
        SamFSConnection c;
        Ctx ctx;
        String jnilib, hostname;
        Criteria crit;
        Copy copy;
        String[] names;
        Criteria[] crits, crits2;
        ArFSDirective arfsd;
        int i;

        if (args.length == 0) {
            System.out.println("One arg expected by TEST3 - filesystem name");
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
           System.out.print("Loading native lib (" + jnilib + ")...");
           System.loadLibrary(jnilib);
           System.out.print("done\n" +
            "Initializing server connection & library...");

           c = SamFSConnection.getNew(hostname);
           ctx = new Ctx(c);
           System.out.println("done\n");

           System.out.println("\nTEST1: " +
            "Calling Archiver.getCriteriaNames");
           names = Archiver.getCriteriaNames(ctx);
           System.out.println("Back to java ");
           for (i = 0; i < TUtil.objarrLen(names); i++)
               System.out.println(names[i]);

           System.out.println("\nTEST2: Calling Archiver.getCriteria");
           crits = Archiver.getCriteria(ctx);
           System.out.println("Back to java ");
           for (i = 0; i < TUtil.objarrLen(crits); i++)
               System.out.println(crits[i]);

           System.out.println("\nTEST3: Calling Archiver.getCriteriaForFS(ctx,"
                        + args[0] + ")");
           crits = Archiver.getCriteria(ctx);
           System.out.println("Back to java ");
           for (i = 0; i < TUtil.objarrLen(crits); i++)
               System.out.println(crits[i]);

           System.out.println("\nTEST4: Modifying last criteria in "
           + args[0]);
           System.out.print(" 4.1: getArFSDirective(ctx," + args[0] + ")");
           arfsd = Archiver.getArFSDirective(ctx, args[0]);
           System.out.println("done: \n" + arfsd);
           crit = arfsd.getCriteria()[arfsd.getCriteria().length - 1];
           crit.setRootDir("myrootdir");
           crit.setReleaseAttr(Criteria.NEVER_RELEASE);
           crit.setStageAttr(Criteria.ASSOCIATIVE_STAGE);
           System.out.println(" 4.2: modifyArFSDirective() (modifying " +
           crit.getSetName() + ")");
           System.out.println(" criteria to be written:" + crit);
           Archiver.setArFSDirective(ctx, arfsd);
           System.out.println("done");

           System.out.print(" 4.3 re-retrieving the ArFSDirective");
           arfsd = Archiver.getArFSDirective(ctx, args[0]);
           System.out.println("done\n" + arfsd);

           System.out.println("\nTEST5: Creating new criteria 'newcrit' in " +
            args[0]);
           crit = new Criteria(arfsd.getFSName(), "newcrit");
           crit.setMaxSize("1234");
           crit.setRootDir("andrei");
           copy = new Copy();
           copy.setCopyNumber(1);
           copy.setArchiveAge(600);
           crit.setCopies(new Copy[] { copy, null, null, null });
           crits2 = new Criteria[crits.length + 1];
           for (i = 0; i < crits.length; i++)
               crits2[i] = crits[i];
           crits2[i] = crit;
           arfsd.setCriteria(crits2);
           System.out.println("New arFSDirective:" + arfsd);
           Archiver.setArFSDirective(ctx, arfsd);
           System.out.println("Back to java.");

           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done");
        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
