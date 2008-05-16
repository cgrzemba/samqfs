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

// ident	$Id: DiskVol.java,v 1.13 2008/05/16 18:35:27 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;

public class DiskVol {

    private String volName;
    private String host; /* for remote disk archiving */
    private String path;
    private long kbytesTotal;
    private long kbytesAvail;
    private int statusFlags;


    /* status flags, these must match the definitions in diskvols.h */
    /* Volume labeled, seqnum file created */
    private static final int DV_LABELED		= 1 << 0;
    /* Volume is defined on a remote host */
    private static final int DV_REMOTE		= 1 << 1;
    /* User set volume unavailable */
    private static final int DV_UNAVAILABLE	= 1 << 2;
    /* User set volume read only */
    private static final int DV_READ_ONLY	= 1 << 3;
    /* Volume is unusable */
    private static final int DV_BAD_MEDIA	= 1 << 4;
    /* volume is a honey comb target */
    private static final int DV_STK5800_VOL	= 0x20000000;
    /* Diskvols database is not initialized */
    private static final int DV_DB_NOT_INIT	= 0x40000000;
    /* Volume is unknown */
    private static final int DV_UNKNOWN_VOLUME	= 0x80000000;

    /* private constructor */
    private DiskVol(String volName, String host, String path,
        long kbytesTotal, long kbytesAvail, int statusFlags) {
        // call public constructor first
        this(volName, host, path);

        this.kbytesTotal = kbytesTotal;
        this.kbytesAvail = kbytesAvail;
        this.statusFlags = statusFlags;
    }


    /* public constructor */
    public DiskVol(String volName, String host, String path) {
        this.volName = volName;
        this.host = host;
        this.path = path;
    }

    /**
     * create a honey comb disk volume
     * @param HCDataAddress is the hostname:port of the
     *		honeycomb data ip. The :port is optional
     */
    public DiskVol(String volName, String HCDataAddress) {
	this.volName = volName;
	this.host = HCDataAddress;
	this.statusFlags |= DV_STK5800_VOL;
    }

    // public instance methods

    public String getVolName() { return volName; }
    public String getHost() { return host; }
    public String getPath() { return path; }
    public long getCapacity() { return kbytesTotal; }
    public long getAvailableSpace() { return kbytesAvail; }

    public boolean isLabeled() {
        return ((statusFlags & DV_LABELED) == DV_LABELED);
    }
    public boolean isReadOnly() {
        return ((statusFlags & DV_READ_ONLY) == DV_READ_ONLY);
    }
    public boolean isRemote() {
        return ((statusFlags & DV_REMOTE) == DV_REMOTE);
    }
    public boolean isUnavailable() {
        return ((statusFlags & DV_UNAVAILABLE) == DV_UNAVAILABLE);
    }
    public boolean isBadMedia() {
        return ((statusFlags & DV_BAD_MEDIA) == DV_BAD_MEDIA);
    }
    public boolean isUnknown() {
        return ((statusFlags & DV_UNKNOWN_VOLUME) == DV_UNKNOWN_VOLUME);
    }
    public boolean isHoneyComb() {
        return ((statusFlags & DV_STK5800_VOL) == DV_STK5800_VOL);
    }

    public void setLabeled(boolean labeled) {
        if (labeled)
            statusFlags = statusFlags | DV_LABELED;
        else
            statusFlags = statusFlags & ~DV_LABELED;
    }
    public void setUnavailable(boolean unavail) {
        if (unavail)
            statusFlags = statusFlags | DV_UNAVAILABLE;
        else
            statusFlags = statusFlags & ~DV_UNAVAILABLE;
    }
    public void setReadOnly(boolean ro) {
        if (ro)
            statusFlags = statusFlags | DV_READ_ONLY;
        else
            statusFlags = statusFlags & ~DV_READ_ONLY;
    }
    public void setBadMedia(boolean bad) {
        if (bad)
            statusFlags = statusFlags | DV_BAD_MEDIA;
        else
            statusFlags = statusFlags & ~DV_BAD_MEDIA;
    }

    public void setStatusFlags(Ctx c) throws SamFSException {
        this.setFlags(c, this.volName, this.statusFlags);
    }

    // public native class methods

    public static native DiskVol get(Ctx c, String volName)
        throws SamFSException;
    public static native DiskVol[] getAll(Ctx c)
        throws SamFSException;
    public static native void add(Ctx c, DiskVol vol)
        throws SamFSException;
    public static native void remove(Ctx c, String volName)
        throws SamFSException;

    public static native String[] getClients(Ctx c)
        throws SamFSException;
    public static native void addClient(Ctx c, String clientHostName)
        throws SamFSException;
    public static native void removeClient(Ctx c, String clientHostName)
        throws SamFSException;

    // The flags can be any combination of user editable flags
    // (flags for which set methods exist)
    public static native void setFlags(Ctx c, String volName, int flags)
        throws SamFSException;

    public String toString() {
        String s = volName + " " + host + " " + (isHoneyComb() ? "STK5800" :
            path) + " cap=" + kbytesAvail + "k,free=" + kbytesAvail +
            "k flags=" + statusFlags;
        return s;
    }
}
