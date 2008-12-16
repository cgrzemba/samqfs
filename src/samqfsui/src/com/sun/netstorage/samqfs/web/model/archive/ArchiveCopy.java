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

// ident	$Id: ArchiveCopy.java,v 1.15 2008/12/16 00:12:17 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.archive;

/*
 * The getters of this interface can be mapped from ar_set_copy_cfg,
 * ar_set_copy_params, and vsn_pools structures in archive.h
 */
public interface ArchiveCopy {

    public ArchivePolicy getArchivePolicy();

    public void setArchivePolicy(ArchivePolicy policy);

    public int getCopyNumber();

    public void setCopyNumber(int number);

    // TODO: REMOVE
    /**
     * return the VSN Map settings for this copy as seen by the archiver,
     * putting into consideration the 'allsets' settings. This method uses the
     * following algorithm to determine the effective copy vsn map.
     *
     * 1. If the copy has an explicit vsn setting, use that
     * 2. If not, use the copy number's allsets policy setting
     * 3. If none exists, use the allsets policy's allsets copy
     * 4. Lastly, return an empty map.
     *
     * @return - ArchiveVSNMap
     */

    public ArchiveVSNMap getArchiveVSNMap();

    public void setArchiveVSNMap(ArchiveVSNMap map);

    public boolean isRearchive();

    public String getDiskArchiveVSN();

    public void setDiskArchiveVSN(String vsn);

    public String getDiskArchiveVSNPath();

    public void setDiskArchiveVSNPath(String path);

    public String getDiskArchiveVSNHost();

    public void setDiskArchiveVSNHost(String hostname);

    public int getReservationMethod();

    public void setReservationMethod(int method);

    public int getArchiveSortMethod();

    public void setArchiveSortMethod(int method);

    public int getOfflineCopyMethod();

    public void setOfflineCopyMethod(int method);

    public int getDrives();

    public void setDrives(int drives);

    public long getMinDrives();

    public void setMinDrives(long drives);

    public int getMinDrivesUnit();

    public void setMinDrivesUnit(int drivesUnit);

    public long getMaxDrives();

    public void setMaxDrives(long drives);

    public int getMaxDrivesUnit();

    public void setMaxDrivesUnit(int drivesUnit);

    public int getJoinMethod();

    public void setJoinMethod(int method);

    public int getUnarchiveTimeReference();

    public void setUnarchiveTimeReference(int ref);

    public boolean isFillVSNs();

    public void setFillVSNs(boolean fillVSN);

    public long getOverflowMinSize();

    public void setOverflowMinSize(long size);

    public int getOverflowMinSizeUnit();

    public void setOverflowMinSizeUnit(int sizeUnit);

    public long getArchiveMaxSize();

    public void setArchiveMaxSize(long size);

    public int getArchiveMaxSizeUnit();

    public void setArchiveMaxSizeUnit(int sizeUnit);

    public int getBufferSize();

    public void setBufferSize(int size);

    public boolean isBufferLocked();

    public void setBufferLocked(boolean lock);

    public long getStartAge();

    public void setStartAge(long age);

    public int getStartAgeUnit();

    public void setStartAgeUnit(int unit);

    public int getStartCount();

    public void setStartCount(int count);

    public long getStartSize();

    public void setStartSize(long size);

    public int getStartSizeUnit();

    public void setStartSizeUnit(int unit);

    public long getRecycleDataSize();

    public void setRecycleDataSize(long size);

    public int getRecycleDataSizeUnit();

    public void setRecycleDataSizeUnit(int unit);

    public int getRecycleHWM();

    public void setRecycleHWM(int hwm);

    public boolean isIgnoreRecycle();

    public void setIgnoreRecycle(boolean ignore);

    public String getNotificationAddress();

    public void setNotificationAddress(String addr);

    public int getMinGain();

    public void setMinGain(int gain);

    public int getMaxVSNCount();

    public void setMaxVSNCount(int count);
}
