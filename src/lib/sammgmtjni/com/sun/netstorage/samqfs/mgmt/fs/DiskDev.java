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

// ident	$Id: DiskDev.java,v 1.13 2008/05/16 18:35:28 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.BaseDev;
import com.sun.netstorage.samqfs.mgmt.fs.AU;
import com.sun.netstorage.samqfs.mgmt.fs.FSInfo;

public class DiskDev extends BaseDev {

    private AU au;
    private long kbytesFree;
    private long inodesFree; // applicable from md devices only

    /* public constructor. should be used when creating new filesystems */

    public DiskDev(AU au) {
        this.au = au;
        devPath = au.getPath();
        this.eq = FSInfo.EQU_AUTO;
        kbytesFree = au.getSize();
        inodesFree = -1;
    }


    /* private constructor */
    private DiskDev(String devPath, int eq, String eqType,
        String fsetName, int eqFSet, int state, String paramPath,
        AU au,
        long kbytesFree,
        long inodesFree) {
            super(devPath, eq, eqType, fsetName, eqFSet, state, paramPath);
            this.au = au;
            this.kbytesFree = kbytesFree;
            this.inodesFree = inodesFree;
    }


    public AU getAU() { return au; }
    public long getFreeSpace() { return kbytesFree; }
    public long getFreeInodes() { return inodesFree; }
    public String toString() {
        return (super.toString() + " au(" + au.toString() + ") " + inodesFree);

    }

    /**
     * use this method with caution!
     * The only current use is in shared fs configurations, where the same
     * device has different paths on different hosts.
     */
    public void setDevicePath(String path) {
	this.devPath = path;
        this.au.setPath(path);
    }

    public void convertPathToGlobal() {
        this.devPath = devPath.replaceFirst("/did/", "/global/");
        this.au.setPath(devPath);
    }

}
