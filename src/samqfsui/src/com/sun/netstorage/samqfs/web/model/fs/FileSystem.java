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

// ident	$Id: FileSystem.java,v 1.26 2008/12/17 21:03:26 kilemba Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.StripedGroup;
import java.util.GregorianCalendar;

/**
 * this interface describes a SAM-FS/QFS filesystem
 */
public interface FileSystem extends GenericFileSystem {

    public static final String NOT_APPLICABLE = "--";

    // file system type
    public static final int SEPARATE_METADATA = 0;
    public static final int COMBINED_METADATA = 1;
    public static final int PROTO_FS = 5;

    // archiving type
    public static final int ARCHIVING = 2;
    public static final int NONARCHIVING = 3;

    // share type, valid only for separate metadata
    public static final int UNSHARED = 0;
    public static final int SHARED_TYPE_MDS = 1;
    public static final int SHARED_TYPE_PMDS = 2;
    public static final int SHARED_TYPE_CLIENT = 3;
    public static final int SHARED_TYPE_UNKNOWN = 4;


    public int getFSTypeByProduct();

    public int getFSType();

    public int getArchivingType();

    public int getEquipOrdinal();

    public int getShareStatus();

    /**
     * Determine if file system is a mb type file system.  It is a
     * Sun StorageTek QFS or Sun SAM-QFS disk cache family set with one or more
     * meta devices.  Metadata resides on these meta devices.  File data resides
     * on the object storage device(s) (OSDs).
     *
     * @since 5.0
     * @return true if file system is a "mb" type file system
     */
    public boolean isMbFS();

    /**
     * Determine if file system is a mat type file system.  It is a
     * Sun StorEdge QFS disk cache family set with one or more meta devices.
     * Metadata resides on these meta devices.  File data resides on the data
     * device(s). This standalone file system has no namespace and is only used
     * as the OSD target backing store of an object storage device (OSD) in an
     * mb file system.
     *
     * @since 5.0
     * @return true if file system is a "mat" type file system
     */
    public boolean isMatFS();

    /**
     * return metadata server name, in shared-QFS configs
     */
    public String getServerName();

    public int getDAUSize();

    public GregorianCalendar getTimeAboveHWM();

    public GregorianCalendar getDateCreated();

    public FileSystemMountProperties getMountProperties();

    public DiskCache[] getMetadataDevices();
    public DiskCache[] getDataDevices();
    public StripedGroup[] getStripedGroups();

    // Strictly used only in Shrink File System Wizard
    // get both metadata & data devices & wrap up striped groups
    public DiskCache[] getAllDevices();

    // action methods

    public void changeMountOptions() throws SamFSException;

    public void stopFSArchive() throws SamFSException;
    public void idleFSArchive() throws SamFSException;
    public void runFSArchive() throws SamFSException;

    public long samfsck(boolean checkAndRepair, String logFilePath)
        throws SamFSException;

    public String getFsckLogfileLocation();
    public void setFsckLogfileLocation(String logfile);

    public ArchivePolCriteria[] getArchivePolCriteriaForFS()
        throws SamFSException;

    public void addPolCriteria(ArchivePolCriteria[] newPolCriteriaList)
        throws SamFSException;

    public void removePolCriteria(ArchivePolCriteria[] newPolCriteriaList)
        throws SamFSException;

    public void reorderPolCriteria(ArchivePolCriteria[] reorderedList)
        throws SamFSException;

    public int getStatusFlag();

    public void setDeviceState(int newState, int [] eqs) throws SamFSException;

    public void grow(DiskCache[] metadata, DiskCache[] data,
        StripedGroup[] groups) throws SamFSException;

    // Added 5.0
    /*
     * Method to remove a device from a file system by releasing all of
     * the data on the device.
     *
     * options is a string of key value pairs that are based on the
     * directives in shrink.cmd. If options is non-null the options
     * will be set for this file system in the shrink.cmd file
     * prior to invoking the shrink.
     *
     * Options Keys:
     *	 block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 stage_files = TRUE | FALSE (default FALSE)
     *	 stage_partial = TRUE | FALSE (default FALSE)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkRelease(int eqToRelease, ShrinkOption options)
        throws SamFSException;

    /*
     * Method to remove a device from a file system by copying the
     * data to other devices. If replacementEq is the eq of a device
     * in the file system the data will be copied to that device.
     * If replacementEq is -1 the data will be copied to available devices
     * in the FS.
     *
     * Options Keys:
     *	 logfile = filename (default no logging)
     *	 block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkRemove(
        int eqToRemove, int replacementEq, ShrinkOption options)
	throws SamFSException;

    /*
     * Method to remove a device from a file system by copying the
     * data to a newly added device.
     *
     * Options Keys:
     *	 logfile = filename (default no logging)
     *	 block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkReplaceDev(
        int eqToRemove, DiskCache replacement, ShrinkOption options)
	throws SamFSException;

    /*
     * Method to remove a striped group from a file system by copying the
     * data to a new striped group.
     *
     * Options Keys:
     *	 logfile = filename (default no logging)
     *	 block_size = n where 1 <= n <= 16 n is in units of mb(default=1)
     *	 display_all_files = TRUE | FALSE (default FALSE)
     *	 do_not_execute = TRUE | FALSE (default FALSE)
     *	 logfile = filename (default no logging)
     *	 streams = n  where 1 <= n <= 128 default 8
     *
     * The integer return is a job id that will be meaningful only for
     * shared file systems.
     */
    public int shrinkReplaceGroup(
        int eqToRemove, StripedGroup replacement, ShrinkOption options)
	throws SamFSException;
}
