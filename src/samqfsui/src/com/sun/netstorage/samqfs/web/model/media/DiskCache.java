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

// ident	$Id: DiskCache.java,v 1.14 2008/07/03 00:04:30 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.media;

public interface DiskCache extends BaseDevice {

    // disk type strings

    public static final int SLICE = 101;
    public static final int SVM_LOGICAL_VOLUME = 102;
    public static final int VXVM_LOGICAL_VOLUME = 103;

    // These four disk types are added starting 4.3

    public static final int SVM_LOGICAL_VOLUME_MIRROR  = 104;
    public static final int VXVM_LOGICAL_VOLUME_MIRROR = 105;
    public static final int SVM_LOGICAL_VOLUME_RAID_5  = 106;
    public static final int VXVM_LOGICAL_VOLUME_RAID_5 = 107;

    // ZVol support is added in 5.0
    public static final int ZFS_ZVOL = 110;


    // cache type strings

    public static final int METADATA = 1;
    public static final int MD = 2;
    public static final int MR = 3;
    public static final int NA = 0; // NA == Not Applicable
    public static final int STRIPED_GROUP = 10;


    // getters

    public int getDiskType();

    public int getDiskCacheType();

    /**
     * @since 4.5
     * @returns true if this device is highly available
     */
    public boolean isHA();

    public long getCapacity();
    public long getAvailableSpace();
    public int getConsumedSpacePercentage();

    // -1 is returned if the method is not applicable for a particular
    // "DiskCache" object
    public int getNoOfInodesRemaining();

    public String getVendor();
    public String getProductId();

    public boolean isAlloc();
    
    public String getDevicePath();

    public int getEquipOrdinal();

    // For display string with logic
    public String getDevicePathDisplayString();
}
