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

// ident $Id: SystemCapacity.java,v 1.7 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model;

import java.util.Properties;

import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/**
 * this class encapsulates information about media related capacity for
 * a SAM-FS/QFS server
 */
public class SystemCapacity {

    // constant below must match those in mgmt.h
    final String KEY_NUMOF_MNT_FS   = "MountedFS";
    final String KEY_DSKCACHE_TOTAL = "DiskCache";
    final String KEY_DSKCACHE_AVAIL = "AvailableDiskCache";
    final String KEY_NUMOF_LIBS = "LibCount";
    final String KEY_MEDIA_TOTAL = "MediaCapacity";
    final String KEY_MEDIA_AVAIL = "AvailableMediaCapacity";
    final String KEY_NUMOF_SLOTS = "SlotCount";

    protected int mountedFS;
    protected long diskCache, availDiskCache;
    protected int libCount;
    protected long mediaCapacity, availMedia;
    protected int slotCount;

    public SystemCapacity(Properties props) throws SamFSException {
        if (props == null)
            return;
        mountedFS =
            ConversionUtil.strToIntVal(props.getProperty(KEY_NUMOF_MNT_FS));
        diskCache =
            ConversionUtil.strToLongVal(props.getProperty(KEY_DSKCACHE_TOTAL));
        availDiskCache =
            ConversionUtil.strToLongVal(props.getProperty(KEY_DSKCACHE_AVAIL));
        libCount =
            ConversionUtil.strToIntVal(props.getProperty(KEY_NUMOF_LIBS));
        mediaCapacity =
            ConversionUtil.strToLongVal(props.getProperty(KEY_MEDIA_TOTAL));
        availMedia =
            ConversionUtil.strToLongVal(props.getProperty(KEY_MEDIA_AVAIL));
        slotCount =
            ConversionUtil.strToIntVal(props.getProperty(KEY_NUMOF_SLOTS));
    }

    // The following constructor is explicitly used by the SIMULATOR ONLY.
    public SystemCapacity(
        int mountedFS,
        long diskCache,
        long availDiskCache,
        int libCount,
        long mediaCapacity,
        long availMedia,
        int slotCount) {

        this.mountedFS = mountedFS;
        this.diskCache = diskCache;
        this.availDiskCache = availDiskCache;
        this.libCount  = libCount;
        this.mediaCapacity  = mediaCapacity;
        this.availMedia = availMedia;
        this.slotCount = slotCount;
    }

    public int getNumOfMountedFS() { return mountedFS; }

    public long getDiskCacheKB() { return diskCache; }
    public long getAvailDiskCacheKB() { return availDiskCache; }

    public int getNumOfLibs() { return libCount; }

    public long getMediaKB() { return mediaCapacity; }
    public long getAvailMediaKB() { return availMedia; }

    public int getNumOfSlots() { return slotCount; }

    public String toString() {
        String E = "=", C = ",";
        String s = KEY_NUMOF_MNT_FS + E + mountedFS + C +
                   KEY_DSKCACHE_TOTAL + E + diskCache + C +
                   KEY_DSKCACHE_AVAIL + E + availDiskCache + C +
                   KEY_NUMOF_LIBS + E + libCount + C +
                   KEY_MEDIA_TOTAL + E + mediaCapacity + C +
                   KEY_MEDIA_AVAIL + E + availMedia + C +
                   KEY_NUMOF_SLOTS + E + slotCount;
        return s;
    }
}
