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

// ident	$Id: TestConn.java,v 1.11 2008/05/16 18:35:31 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSConnection;

public class TestConn {

    public static void main(String arg[]) {

        SamFSConnection c, c2;

        try {
           long tout;
           TUtil.loadLib("fsmgmtjni");

           System.out.print("Calling native getDefaultTimeout()");
           tout = SamFSConnection.getDefaultTimeout();

           System.out.println(" -> " + tout + "sec");
           System.out.println("Calling native getNew()");
           c = SamFSConnection.getNew(arg[0]);
           System.out.print(c);
           System.out.print("Calling native getTimeout()");
           tout = c.getTimeout();
           System.out.println(" -> " + tout + "sec");
           System.out.print("Hostname is " + c.getServerHostname());
           System.out.println();
           System.out.print("Architecture is " + c.getServerArch());
           System.out.println();
           System.out.println("Creating second connection - getNew(15)");
           c2 = SamFSConnection.getNewSetTimeout(arg[0], 15);
           System.out.print("Calling native c2.getTimeout()");
           tout = c2.getTimeout();
           System.out.println(" -> " + tout + "sec.\n" +
               "Calling native c2.setTimeout(33)");
           c.setTimeout(33);
           System.out.print("Rereading timeout");
           tout = c.getTimeout();
           System.out.println(" -> " + tout + "sec.");

           System.out.println("Calling native destroy for both connections");
           c.destroy();
           c2.destroy();
           c.destroy(); /* this should have no effect */

        } catch (SamFSException e) {
                System.out.println(e);
        }
    }
}
