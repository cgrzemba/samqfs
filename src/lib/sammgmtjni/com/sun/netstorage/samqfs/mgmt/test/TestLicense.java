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

// ident	$Id: TestLicense.java,v 1.9 2008/03/17 14:44:03 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.adm.License;


public class TestLicense {

    static final int MIN_EXPECTED_ARGS = 0;

    public static void main(String args[]) {

        SamFSConnection c;
        Ctx ctx;
        License license;
        String[] mediaTypes;

        try {
            String hostname, jnilib;

            if (args.length == MIN_EXPECTED_ARGS) {
               hostname = "localhost";
               jnilib = "sammgmtjnilocal";
            } else {
               hostname = args[MIN_EXPECTED_ARGS];
               jnilib = "sammgmtjni";
            }
            TUtil.loadLib(jnilib);
            c = SamFSConnection.getNew(hostname);
            ctx = new Ctx(c);

            System.out.println("TEST1: License.getFSType()");
            System.out.println(
                   "Calling native License.getFSType(ctx," + ")");
               System.out.println(License.getFSType(new Ctx(c)));

            System.out.println("TEST2: License.getLicense()");
               System.out.println(
                   "Calling native License.getLicense(ctx," + ")");
               license = License.getLicense(new Ctx(c));
                   System.out.println("Back to java ");
            System.out.println(license.toString());

        } catch (SamFSException e) {
            System.out.println(e);
        }
    }
}
