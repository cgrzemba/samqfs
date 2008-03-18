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

// ident	$Id: StripedGrp.java,v 1.7 2008/03/17 14:43:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.fs.DiskDev;

public class StripedGrp {

    private String name;    /* gxx format */
    private DiskDev[] devs;


    /* public constructor */
    public StripedGrp(String name, DiskDev devs[]) {
        this.name = name;
        this.devs = devs;
    }

    /* public methods */
    public String getName() { return name; }
    public DiskDev[] getMembers() { return devs; }
    public String toString() {
        String s = "Striped group " + name;
        for (int i = 0; i < devs.length; i++)
            s = s + "\n  " + devs[i];
        return s;
    }

}
