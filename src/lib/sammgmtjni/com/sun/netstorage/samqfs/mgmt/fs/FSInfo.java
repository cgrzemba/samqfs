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

// ident	$Id: FSInfo.java,v 1.18 2008/03/17 14:43:58 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.Ctx;

public class FSInfo {

    // fs equipment type
    public static final String SEPARATE_METADATA = "ma";
    public static final String COMBINED_METADATA = "ms";
    // since 4.4
    public static final String UFS_DATA = "ufs";

    /**
     * this value should be used if the user wants a new valid EQU to be
     * automatically chosen for the FS when the FS is created.
     */
    public static final int EQU_AUTO = 0;

    // instance fields
    // Group1: fields needed for filesystem creation
    private String name;
    private int equ;
    private int dau; // DAU size (bytes)
    private String eqType; // ma/ms/ufs
    private DiskDev metadataDevs[];
    private DiskDev dataDevs[];
    private StripedGrp stripedGrps[];
    private MountOptions opts;
    private String mntPoint;
    // shared-specific
    private String sharedFsServerName;
    private Host hosts[];
    // Group2: fields that cannot be set by the public constructor
    private long time; // fs creation time. set by user only for HA-QFS
    private int smallDau; // only used in dual-dau cfgs
    private boolean archiving, shared;
    private long kbytesTotal;
    private long kbytesAvail;
    private int statusFlags; // see bottom for a list of valid flags
    private String nfsShareState; // NFS_SHARED | NFS_NOTSHARED | NFS_CONFIGURED


    /* private constructor */
    private FSInfo(String name, int equ, int smallDau, int dau, String eqType,
        long time,
        boolean archiving, boolean shared,
        DiskDev metadataDevs[], DiskDev dataDevs[], StripedGrp stripedGrps[],
        MountOptions opts, String mntPoint,
        long kbytesTotal, long kbytesAvail,
        String sharedFsServerName, Host hosts[],
        int statusFlags, String nfsShareState) {

            // call public constructor first
            this(name, equ, dau, eqType, metadataDevs, dataDevs, stripedGrps,
                opts, mntPoint);
            this.time  = time;
            this.smallDau  = smallDau;
            this.archiving = archiving;
            this.shared    = shared;
            this.kbytesAvail = kbytesAvail;
            this.kbytesTotal = kbytesTotal;
            this.sharedFsServerName = sharedFsServerName;
            this.hosts = hosts;
            this.statusFlags = statusFlags;
            this.nfsShareState = nfsShareState;
    }

    /**
     * public constructor for non-shared FS
     */
    public FSInfo(String name, int equ, int dau, String eqType,
        DiskDev metadataDevs[], DiskDev dataDevs[], StripedGrp stripedGrps[],
        MountOptions opts, String mntPoint) {
            this.name = name;
            this.equ = equ;
            this.dau = dau;
            this.eqType = eqType;
            this.metadataDevs = metadataDevs;
            this.dataDevs = dataDevs;
            this.stripedGrps = stripedGrps;
            this.opts = opts;
            this.mntPoint = mntPoint;
	    this.shared = false;
	    this.sharedFsServerName = null;
	    this.hosts = null;
	    this.time = 0;
    }

    /**
     * public constructor for shared FS
     */
    public FSInfo(String name, int equ, int dau, String eqType,
        DiskDev metadataDevs[], DiskDev dataDevs[], StripedGrp stripedGrps[],
        MountOptions opts, String mntPoint,
	/* following arguments are shared-specific arguments */
	String sharedFsServerName,
	boolean mdServer, /* metadata server ? */
	boolean potential /* if mdServer, then potential or active? */,
	Host hosts[]) {

            this(name, equ, dau, eqType, metadataDevs, dataDevs, stripedGrps,
		 opts, mntPoint);
	    this.shared = true;
            this.sharedFsServerName = sharedFsServerName;
	    if (!mdServer) {
		/* client */
		this.statusFlags = FS_CLIENT | FS_NODEVS;
	    } else if (potential) {
		/* potential metadata server */
		this.statusFlags = FS_CLIENT;
	    } else {
		/* metadata server */
		this.statusFlags = FS_SERVER;
		this.time = 0;
	    }
            this.hosts = hosts;
    }

    /* instance methods */

    public String getName() { return name; }
    public int getEqu() { return equ; }
    public int getDAUSize() { return dau; }
    public long getCreationTime() { return time; }
    public boolean isArchiving() { return archiving; }
    public boolean isShared() { return shared; }
    public MountOptions getMountOptions() { return opts; }
    public String getMountPoint() { return mntPoint; }
    public DiskDev[] getMetadataDevices() { return metadataDevs; }
    public DiskDev[] getDataDevices() { return dataDevs; }
    public StripedGrp[] getStripedGroups() { return stripedGrps; }
    public long getCapacity() { return kbytesTotal; }
    public long getAvailableSpace() { return kbytesAvail; }
    public String getServerName() { return sharedFsServerName; }
    public Host[] getHosts() { return hosts; }
    public String getNFSShareState() { return nfsShareState; }

    /* set this when creating HA-QFS on secondary hosts */
    public void doNotMkfs() { time = 1; }

    /* status information */
    public boolean isMounted() {
	return ((statusFlags & FS_MOUNTED) == FS_MOUNTED); }
    public boolean isMdServer() {
	return ((statusFlags & FS_SERVER) == FS_SERVER); }
    public boolean isPotentialMdServer() {
	return ((statusFlags & FS_CLIENT) == FS_CLIENT &&
		(statusFlags & FS_NODEVS) == 0); }
    public boolean isClient() {
	return ((statusFlags & (FS_CLIENT | FS_NODEVS))
		== (FS_CLIENT | FS_NODEVS)); }

    /**
     * Added in 4.6
     * Simply return the statusFlag to the logic layer.
     * This is used by the Monitoring console.
     */
    public int getStatusFlags() { return statusFlags; }

    /**
     * failover status - 0 or:
     *
     * FS_FREEZING  - host is failing over
     * FS_FROZEN    - host is frozen
     * FS_THAWING   - host is thawing
     * FS_RESYNCING - server is resyncing
     */
    public int failoverStatus() {
	return (statusFlags & 0x0f000000);
    }


    public String toString() {
        String s = "name=" + name + ",dau=" + dau + ",equ=" + equ +
                    ",isArc=" + (archiving ? "T" : "F") +
                    ",shared=" + (shared ? "T" : "F") + ",time=" + time +
                   ",cap=" + kbytesTotal + ",space=" + kbytesAvail +
                   Integer.toHexString(statusFlags) + "\n";
        s += "metadata devices:\n";
        if (metadataDevs != null)
          for (int i = 0; i < metadataDevs.length; i++)
              s += "   " + metadataDevs[i] + "\n";
         else
              s += "  none\n";

         s += "data devices:\n";
         if (dataDevs != null)
           for (int i = 0; i < dataDevs.length; i++)
               s += "   " + dataDevs[i] + "\n";
         else
               s += "  none\n";

         s += "striped groups devices:\n";
         if (stripedGrps != null)
           for (int i = 0; i < stripedGrps.length; i++)
               s += stripedGrps[i] + "\n";
         if (shared)
               s += "MDS server:" + sharedFsServerName + "\n";
	 if (null != hosts) {
		 s += "hosts config:\n";
		 for (int i = 0; i < hosts.length; i++)
               s += hosts[i] + "\n";
	 }
         return s;
    }


    /* intance methods that call native class methods */
    public void create(Ctx c, boolean chgVFSTab) throws SamFSException {
            FS.create(c, this, chgVFSTab);
    }
    public void remove(Ctx c) throws SamFSException { FS.remove(c, name); }
    public void mount(Ctx c) throws SamFSException { FS.mount(c, name); }
    public void umount(Ctx c) throws SamFSException { FS.umount(c, name); }
    public void grow(Ctx c, DiskDev[] addMdataDisks, DiskDev[] addDataDisks,
        StripedGrp[] addGrps) throws SamFSException {
            FS.grow(c, this, addMdataDisks, addDataDisks, addGrps);
    }
    public void fsck(Ctx c, String logFile, boolean repair)
        throws SamFSException {
            FS.fsck(c, name, logFile, repair);
    }


    /*
     * status bits.
     * these values MUST match those defined in sam/mount.h
     */

    public static final int FS_MOUNTED = 0x00000001;
    public static final int FS_MOUNTING = 0x00000002;
    public static final int FS_UMOUNT_IN_PROGRESS = 0x00000004;
    /* host is now metadata server */
    public static final int FS_SERVER = 0x00000010;
    /* host is not metadata server */
    public static final int FS_CLIENT = 0x00000020;
    /* host can't be metadata server */
    public static final int FS_NODEVS = 0x00000040;
    /* metadata server running SAM */
    public static final int FS_SAM = 0x00000080;
    /* lock write operations */
    public static final int FS_LOCK_WRITE = 0x00000100;
    /* lock name operations */
    public static final int FS_LOCK_NAME = 0x00000200;
    /* lock remove name operations */
    public static final int FS_LOCK_RM_NAME = 0x00000400;
    /* lock all operations */
    public static final int FS_LOCK_HARD = 0x00000800;
    /* host is failing over */
    public static final int FS_FREEZING = 0x01000000;
    /* host is frozen */
    public static final int FS_FROZEN = 0x02000000;
    /* host is thawing */
    public static final int FS_THAWING = 0x04000000;
    /* server is resyncing */
    public static final int FS_RESYNCING = 0x08000000;
    /* releasing is active on fs */
    public static final int FS_RELEASING = 0x20000000;
    /* staging is active on fs */
    public static final int FS_STAGING = 0x40000000;
    /* archiving is active on fs */
    public static final int FS_ARCHIVING = 0x80000000;

    // Indicate whether this filesystem is NFS shared or not
    public static final String NFS_SHARED = "yes";
    public static final String NFS_NOTSHARED = "no";
    public static final String NFS_CONFIGURED = "config";
}
