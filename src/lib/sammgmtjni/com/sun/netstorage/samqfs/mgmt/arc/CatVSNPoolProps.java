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

// ident	$Id: CatVSNPoolProps.java,v 1.9 2008/12/16 00:08:54 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import com.sun.netstorage.samqfs.mgmt.media.CatEntry;

public class CatVSNPoolProps extends BaseVSNPoolProps {

    private CatEntry[] catEntries;

    protected CatVSNPoolProps(String poolName, int numOfVSNs, long capacity,
        long freeSpace, CatEntry[] catEntries,
        String mediaType) {
            super(poolName, numOfVSNs, capacity, freeSpace, mediaType);
            this.catEntries = catEntries;
    }


    public CatEntry[] getCatEntries() { return catEntries; }

    public String toString() {
        String s = "PoolProps[" + poolName + "]: #vsns=" + numOfVSNs +
        ",cap=" + capacity + "k,free=" + freeSpace + "k,#catentries=" +
        catEntries.length;
        for (int i = 0; i < catEntries.length; i++)
            s += "\n " + catEntries[i];
        return s;
    }
}
