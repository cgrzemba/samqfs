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

// ident	$Id: AU.java,v 1.20 2008/12/16 00:08:55 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

/* Allocatable Unit = disk slice or volume */
public class AU {

    public static final int SLICE = 0;
    public static final int SVM_LOGICAL_VOLUME  = 1;
    public static final int VXVM_LOGICAL_VOLUME = 2;
    public static final int ZFS_ZVOL = 3;
    public static final int OSD = 4;

    private String path;
    private int type;
    private long size; /* kbytes */
    /* following fields available since 4.3 */
    private String usedBy; /* non-null if this is in use */
    private String raid;
    private SCSIDevInfo scsi;

    private AU(int type, long size, String path, String usedBy,
        String raid, SCSIDevInfo scsi) {
            this.type = type;
            this.size = size;
            this.path = path;
	    this.usedBy = usedBy;
	    if (usedBy != null)
		if (usedBy.length() == 0)
		    this.usedBy = null; // null = not used
            this.raid = raid;
            this.scsi = scsi;
    }


    public int getType() { return type; }
    public long getSize() { return size; }
    public String getPath() { return path; }
    public String getUsedBy() { return usedBy; }


    /*
     * Caution: Use this only for configuring Shared FS.
     */
    public void setPath(String path) { this.path = path; }

    /**
     * RAID level for volumes.
     * this is either NULL or it must start with one of the following:
     * '1' - mirrored
     * '5' - RAID5
     */
    public String getRAID() { return (raid); }

    /**
     *  always return null if au type is SVM/VxVM
     */
    public SCSIDevInfo getSCSIDevInfo() { return (scsi); }

    public String toString() {
            return (Integer.toString(type) + " " + path + "\t" +
                Long.toString(size)+ "K\t" +
		((usedBy != null) ? usedBy : "~avail~") + "\t" +
		raid + " " + scsi);
    }

    public static native AU[] discoverAvailAUs(Ctx c) throws SamFSException;

    public static native AU[] discoverAUs(Ctx c) throws SamFSException;

    /**
     * @since 4.5
     * @param hosts lists of hosts that must be able to access the AU
     * @param availOnly if true, then return the available AU-s
     * @return list of HA AU-s only
     */
    public static native AU[] discoverHAAUs(Ctx c, String[] hosts,
                                            boolean availOnly)
        throws SamFSException;


    /**
     * @return a subset of the input - only those slices for which overlaps
     * are detected.
     * slices that span across the whole disk (such as s2 under the default
     * Solaris formatting) are not taken into account when reporting overlaps.
     *
     * since 4.2 (API ver 1.1)
     */
    public static native String[] checkSlicesForOverlaps(Ctx ctx,
        String slices[])
        throws SamFSException;
}
