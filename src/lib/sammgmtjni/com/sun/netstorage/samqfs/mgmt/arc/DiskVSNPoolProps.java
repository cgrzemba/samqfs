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

// ident	$Id: DiskVSNPoolProps.java,v 1.8 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;

public class DiskVSNPoolProps extends BaseVSNPoolProps {

    private DiskVol[] diskEntries;

    private DiskVSNPoolProps(
        String poolName, int numOfVSNs,
        long capacity, long freeSpace,
        DiskVol[] diskEntries, String mediaType) {
            super(poolName, numOfVSNs, capacity, freeSpace, mediaType);
            this.diskEntries = diskEntries;
    }


    public DiskVol[] getDiskEntries() { return diskEntries; }

    public String toString() {
        String s = "PoolProps[" + poolName + "]: #vsns=" + numOfVSNs +
        ",cap=" + capacity + "k,free=" + freeSpace + "k,#diskentries=" +
        diskEntries.length;
        for (int i = 0; i < diskEntries.length; i++)
            s += "\n " + diskEntries[i];
        return s;
    }
}
