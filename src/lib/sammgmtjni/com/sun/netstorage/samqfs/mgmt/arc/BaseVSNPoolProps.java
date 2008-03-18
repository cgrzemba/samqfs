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

// ident	$Id: BaseVSNPoolProps.java,v 1.6 2008/03/17 14:43:57 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

public class BaseVSNPoolProps {

    protected String poolName;
    protected int numOfVSNs;
    protected long capacity;  // kbytes
    protected long freeSpace; // kbytes
    protected String mediaType;

    protected BaseVSNPoolProps(
        String poolName, int numOfVSNs,
        long capacity, long freeSpace,
        String mediaType) {
            this.poolName   = poolName;
            this.numOfVSNs  = numOfVSNs;
            this.capacity   = capacity;
            this.freeSpace  = freeSpace;
            this.mediaType  = mediaType;
    }

    public int getNumOfVSNs() { return numOfVSNs; }
    public long getCapacity() { return capacity; }   // kbytes
    public long getFreeSpace() { return freeSpace; } // kbytes
    public String getMediaType() { return mediaType; }


    public String toString() {
        String s = "PoolProps[" + poolName + "]: #vsns=" + numOfVSNs +
        ",cap=" + capacity + "k,free=" + freeSpace + "k" +
        ", type=" + mediaType;
        return s;
    }
}
