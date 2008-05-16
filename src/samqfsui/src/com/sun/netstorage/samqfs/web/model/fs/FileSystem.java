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

// ident	$Id: FileSystem.java,v 1.20 2008/05/16 18:39:00 am143972 Exp $

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
}
