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

// ident	$Id: CatEntry.java,v 1.9 2008/03/17 14:43:59 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.media;

public class CatEntry {

    private int status; // see list of consts at the at end of this file
    private String mediaType;
    private String vsn;
    private int slot, partition;
    private long accessCount;
    private long capacity;  // Mbytes
    private long freeSpace; // Mbytes
    private long blockSz;   // bytes
    private long labelTime, modTime, mountTime;
    private String barcode;
    // archiver reserve info
    private ReservInfo resInfo;
    private int libEq;

    /**
     * private constructor
     */
    private CatEntry(int status, String mediaType, String vsn, int slot,
        int partition, long accessCount, long capacity, long freeSpace,
        long blockSz, long labelTime, long modTime, long mountTime,
        String barcode, long resTime,
        String resCopyName, String resOwner, String resFS, int libEq) {
            this.status = status;
            this.mediaType = mediaType;
            this.vsn = vsn;
            this.slot = slot;
            this.partition = partition;
            this.accessCount = accessCount;
            this.capacity  = capacity;
            this.freeSpace = freeSpace;
            this.blockSz = blockSz;
            this.labelTime = labelTime;
            this.modTime   = modTime;
            this.mountTime = mountTime;
            this.barcode = barcode;
            this.resInfo =
                new ReservInfo(resTime, resCopyName, resOwner, resFS);
            this.libEq = libEq;
    }

    // status bits. must match the values in pub/mgmt/catalog.h

    /* this entry needs to be looked at */
    public static final int CES_needs_audit = 0x80000000;
    /* slot can be unoccupied but in use */
    public static final int CES_inuse = 0x40000000;
    /* media is labeled */
    public static final int CES_labeled = 0x20000000;
    /* scanner detected bad media */
    public static final int CES_bad_media   = 0x10000000;
    /* slot occupied */
    public static final int CES_occupied    = 0x08000000;
    /* cleaning cartridge in this slot */
    public static final int CES_cleaning    = 0x04000000;
    /* bar codes in use */
    public static final int CES_bar_code    = 0x02000000;
    /* Physical write protect */
    public static final int CES_writeprotect = 0x01000000;
    /* User set read only */
    public static final int CES_read_only   = 0x00800000;
    /* media is to be re-cycled */
    public static final int CES_recycle = 0x00400000;
    /* slot is unavailable */
    public static final int CES_unavail = 0x00200000;
    /* slot is an import/export slot */
    public static final int CES_export_slot = 0x00100000;

    /* Media is not from sam */
    public static final int CES_non_sam = 0x00080000;
    /* User set capacity */
    public static final int CES_capacity_set = 0x00010000;

    /* VSN has high priority */
    public static final int CES_priority    = 0x00008000;
    /* Duplicate VSN */
    public static final int CES_dupvsn = 0x00004000;
    /* Reconcile catalog and library entries */
    public static final int CES_reconcile   = 0x00002000;
    /* This tape has been partitioned */
    public static final int CES_partitioned = 0x00001000;
    /* Archiver found volume full */
    public static final int CES_archfull    = 0x00000800;

    public int getStatus() { return status; }
    public String getMediaType() { return mediaType; }
    public String getVSN() { return vsn; }
    public int getSlot() { return slot; }
    public int getPartition() { return partition; }
    public long getAccessCount() { return accessCount; }
    public long getCapacity() { return capacity; }
    public long getFreeSpace() { return freeSpace; }
    public long getBlockSz() { return blockSz; }
    public long getLabelTime() { return labelTime; }
    public long getModTime() { return modTime; }
    public long getMountTime() { return mountTime; }
    public String getBarcode() { return barcode; }
    public boolean isReserved() { return (resInfo.getResTime() != 0); }
    public ReservInfo getReservationInfo() { return resInfo; }
    public int getLibraryEqu() { return libEq; }

    public String toString() {
        String s = mediaType + " " + vsn + " " + slot + "," + partition +
        " cap=" + capacity + "k,free=" + freeSpace + "k blk=" + blockSz +
        " bcode=" + barcode + " libEq=" + libEq + " rsrvd=" + isReserved();
        return s;
    }
}
