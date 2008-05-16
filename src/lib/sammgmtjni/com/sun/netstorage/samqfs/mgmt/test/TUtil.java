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

// ident	$Id: TUtil.java,v 1.9 2008/05/16 18:35:30 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.test;

import com.sun.netstorage.samqfs.mgmt.SamFSConnection;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Util;

public class TUtil {

    public static String str(Object o) {
        if (o == null)
            return ("NULL");
        else
            return o.toString();
    }

    public static int objarrLen(Object arr[]) {
        if (null == arr)
            return (-1);
        else
            return arr.length;
    }

    public static void loadLib(String jnilibname) {
           System.loadLibrary(jnilibname);
           System.out.println("Native lib (" +
               jnilibname + ") loaded successfuly.");
           Util.setNativeTraceLevel(3); // most verbose
           System.out.println("Trace level set to 3 (max)");
    }

    public static void destroyConn(SamFSConnection c) throws SamFSException {
           System.out.print("destroying connection...");
           c.destroy();
           System.out.println("done.");
    }
}
