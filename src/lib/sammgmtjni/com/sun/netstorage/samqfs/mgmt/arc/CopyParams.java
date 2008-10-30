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

// ident	$Id: CopyParams.java,v 1.17 2008/10/30 14:42:29 pg125177 Exp $

package com.sun.netstorage.samqfs.mgmt.arc;

import com.sun.netstorage.samqfs.mgmt.rec.RecyclerParams;

public class CopyParams {

    // these values must match those defined in pub/mgmt/archive.h

    private static final long AR_PARAM_archmax = 0x00000100;
    private static final long AR_PARAM_bufsize = 0x00000200;
    private static final long AR_PARAM_disk_volume  = 0x00000400;
    private static final long AR_PARAM_drivemin = 0x00000800;
    private static final long AR_PARAM_drives   = 0x00001000;
    private static final long AR_PARAM_fillvsns = 0x00002000;
    private static final long AR_PARAM_join = 0x00004000;
    private static final long AR_PARAM_buflock  = 0x00008000;
    private static final long AR_PARAM_offline_copy = 0x00010000;
    private static final long AR_PARAM_ovflmin  = 0x00020000;
    private static final long AR_PARAM_reserve  = 0x00040000;
    private static final long AR_PARAM_simdelay = 0x00080000;
    private static final long AR_PARAM_sort  = 0x00100000;
    private static final long AR_PARAM_rsort = 0x00200000;
    private static final long AR_PARAM_startage = 0x00400000;
    private static final long AR_PARAM_startcount = 0x00800000;
    private static final long AR_PARAM_startsize  = 0x01000000;
    private static final long AR_PARAM_tapenonstop = 0x02000000;
    private static final long AR_PARAM_testvfl  = 0x04000000;
    private static final long AR_PARAM_drivemax = 0x08000000;
    private static final long AR_PARAM_unarchage = 0x10000000;
    private static final long AR_PARAM_directio  = 0x20000000;
    private static final long AR_PARAM_rearch_stage_copy = 0x40000000;

    // private fields

    private String copyName; /* such as images.1 */
    private int bufSize;
    private boolean bufLocked;
    private String archMax; // bytes
    private int drives;
    private String drvMax, drvMin; // bytes
    private String dskVol;
    private boolean fillVSNs, tapeNonStop;
    private ArPriority arPrios[];
    private boolean unarchAge; // if true use access time, else use modify time
    private int join,
        rsort, // reverse sort
        sort,
        offlineCp;
    private short reserve;
    private long age;
    private int count;
    private String startSize; // bytes
    private String overflowMinSz; // bytes
    private RecyclerParams recParams;
    // following 3 copy params are preserved by JNI but not exposed
    private int simdelay;
    private boolean tstovfl, directio;
    private short rearch_stage_copy;
    private long chgFlags;
    private long queue_time_limit;
    private long fillvsnsMin;

    /**
     * private constructor
     */
    private CopyParams(String copyName, int bufSize, boolean bufLocked,
        String archMax, int drives, String drvMax, String drvMin, String dskVol,
        boolean fillVSNs, boolean tapeNonStop, ArPriority arPrios[],
        boolean unarchAge, int join, int rsort, int sort, int offlineCp,
        short reserve, long age, int count, String startSize,
        String overflowMinSz, RecyclerParams recParams,
        int simdelay, boolean tstovfl, boolean directio,
	short rearch_stage_copy, long chgFlags,
	long queue_time_limit, long fillvsnsMin) {
            this.copyName = copyName;
            this.bufSize = bufSize;
            this.bufLocked = bufLocked;
            this.archMax = archMax;
            this.drives = drives;
            this.drvMax = drvMax;
            this.drvMin = drvMin;
            this.dskVol = dskVol;
            this.fillVSNs = fillVSNs;
            this.tapeNonStop = tapeNonStop;
            this.arPrios = arPrios;
            this.unarchAge = unarchAge;
            this.reserve = reserve;
            this.join = join;
            this.rsort = rsort;
            this.sort = sort;
            this.offlineCp = offlineCp;
            this.age = age;
            this.count = count;
            this.startSize = startSize;
            this.overflowMinSz = overflowMinSz;
            this.recParams = recParams;
            this.simdelay = simdelay;
            this.tstovfl = tstovfl;
            this.directio = directio;
	    this.rearch_stage_copy = rearch_stage_copy;
            this.chgFlags = chgFlags;
            this.queue_time_limit = queue_time_limit;
	    this.fillvsnsMin = fillvsnsMin;
    }

    /**
     *  public constructor used to clone a CopyParams object
     */
    public CopyParams(String copyName, CopyParams cp) {
        this(copyName, cp.bufSize, cp.bufLocked, cp.archMax, cp.drives,
        cp.drvMax, cp.drvMin, cp.dskVol, cp.fillVSNs, cp.tapeNonStop,
        cp.arPrios, cp.unarchAge, cp.join, cp.rsort, cp.sort, cp.offlineCp,
        cp.reserve, cp.age, cp.count, cp.startSize, cp.overflowMinSz,
        cp.recParams, cp.simdelay, cp.tstovfl, cp.directio,
	cp.rearch_stage_copy, cp.chgFlags, cp.queue_time_limit, cp.fillvsnsMin);
    }

    // these values must match those defined in pub/mgmt/archive.h

    // valid values for join method
    public static final int JOIN_NOT_SET = -1;
    public static final int NO_JOIN   = 0;
    public static final int JOIN_PATH = 1;

    // valid values for offline copy method
    public static final int OC_NOT_SET = -1;
    public static final int OC_NONE = 0;
    public static final int OC_DIRECT = 1;
    public static final int OC_STAGEAHEAD = 2;
    public static final int OC_STAGEALL = 3;

    // valid values for sort method
    public static final int SM_NOT_SET = -1;
    public static final int SM_NONE = 0;
    public static final int SM_AGE  = 1;
    public static final int SM_PATH = 2;
    public static final int SM_PRIORITY = 3;
    public static final int SM_SIZE = 4;

    // valid values for reserve. RM_USER and RM_GROUP are mutually exclusive
    public static final short RM_NONE = 0;
    public static final short RM_SET = 0x0001; /* Use archive set */
    public static final short RM_OWNER = 0x0002; /* Use file owner.  */
    public static final short RM_FS = 0x0004; /* Use file system */
    public static final short RM_DIR = 0x0008; /* Owner is directory name */
    public static final short RM_USER = 0x0010; /* Owner is user id */
    public static final short RM_GROUP = 0x0020; /* Owner is group id */


    /**
     * public constructor
     */
    public CopyParams(String copyName) {
        this.copyName = copyName;
        this.chgFlags = 0;
    }

    // instance methods

    public String getName() { return copyName; }

    public int getBufSize() { return bufSize; }
    public void setBufSize(int bufSize) {
        this.bufSize = bufSize;
        chgFlags |= AR_PARAM_bufsize;
    }
    public void resetBufSize() {
        chgFlags &= ~AR_PARAM_bufsize;
    }

    public boolean isBufferLocked() { return bufLocked; }
    public void setBufferLocked(boolean bufLocked) {
        this.bufLocked = bufLocked;
        chgFlags |= AR_PARAM_buflock;
    }
    public void resetBufferLocked() {
        chgFlags &= ~AR_PARAM_buflock;
    }

    public String getArchMax() { return archMax; }
    public void setArchMax(String archMax) {
        this.archMax = archMax;
        chgFlags |= AR_PARAM_archmax;
    }
    public void resetArchMax() {
        chgFlags &= ~AR_PARAM_archmax;
    }

    public int getDrives() { return drives; }
    public void setDrives(int drives) {
        this.drives = drives;
        chgFlags |= AR_PARAM_drives;
    }
    public void resetDrives() {
        chgFlags &= ~AR_PARAM_drives;
    }

    public String getMaxDrives() { return drvMax; }
    public void setMaxDrives(String drvMax) {
        this.drvMax = drvMax;
        chgFlags |= AR_PARAM_drivemax;
    }
    public void resetMaxDrives() {
        chgFlags &= ~AR_PARAM_drivemax;
    }

    public String getMinDrives() { return drvMin; }
    public void setMinDrives(String drvMin) {
        this.drvMin = drvMin;
        chgFlags |= AR_PARAM_drivemin;
    }
    public void resetMinDrives() {
        chgFlags &= ~AR_PARAM_drivemin;
    }

    public String getDiskArchiveVol() { return dskVol; }
    public void setDiskArchiveVol(String dskVol) {
        this.dskVol = dskVol;
        chgFlags |= AR_PARAM_disk_volume;
    }
    public void resetDiskArchiveVol() {
        chgFlags &= ~AR_PARAM_disk_volume;
    }

    public boolean isFillVSNs() { return fillVSNs; }
    public void setFillVSNs(boolean fillVSNs) {
        this.fillVSNs = fillVSNs;
        chgFlags |= AR_PARAM_fillvsns;
    }
    public void resetFillVSNs() {
        chgFlags &= ~AR_PARAM_fillvsns;
    }

    public boolean getUnarchAge() { return unarchAge; }
    public void setUnarchAge(boolean unarchAge) {
        this.unarchAge = unarchAge;
        chgFlags |= AR_PARAM_unarchage;
    }
    public void resetUnarchAge() {
        chgFlags &= ~AR_PARAM_unarchage;
    }

    public String getOverflowMinSize() { return overflowMinSz; }
    public void setOverflowMinSize(String overflowMinSz) {
        this.overflowMinSz = overflowMinSz;
        chgFlags |= AR_PARAM_ovflmin;
    }
    public void resetOverflowMinSize() {
        chgFlags &= ~AR_PARAM_ovflmin;
    }

    public short getReservationMethod() { return reserve; }
    public void setReservationMethod(short reserve) {
        this.reserve = reserve;
        chgFlags |= AR_PARAM_reserve;
    }
    public void reserReservationMethod() {
        chgFlags &= ~AR_PARAM_reserve;
    }

    public int getJoinMethod() { return join; }
    public void setJoinMethod(int join) {
        this.join = join;
        chgFlags |= AR_PARAM_join;
    }
    public void resetJoinMethod() {
        chgFlags &= ~AR_PARAM_join;
    }

    public int getArchiveSortMethod() { return sort; }
    public void setArchiveSortMethod(int sort) {
        this.sort = sort;
        chgFlags |= AR_PARAM_sort;
    }
    public void resetArchiveSortMethod() {
        chgFlags &= ~AR_PARAM_sort;
    }

    public int getOfflineCopyMethod() { return offlineCp; }
    public void setOfflineCopyMethod(int offlineCp) {
        this.offlineCp = offlineCp;
        chgFlags |= AR_PARAM_offline_copy;
    }
    public void resetOfflineCopyMethod() {
        chgFlags &= ~AR_PARAM_offline_copy;
    }

    public int getReverseSortMethod() { return rsort; }
    public void setReverseSortMethod(int rsort) {
        this.rsort = rsort;
        chgFlags |= AR_PARAM_sort;
    }
    public void resetReverseSortMethod() {
        chgFlags &= ~AR_PARAM_rsort;
    }

    public RecyclerParams getRecyclerParams() { return recParams; }
    public void setRecyclerParams(RecyclerParams recParams) {
        this.recParams = recParams;
    }

    // archive workload copy parameters

    public long getStartAge() { return age; }
    public void setStartAge(long age) {
        this.age = age;
        chgFlags |= AR_PARAM_startage;
    }
    public void resetStartAge() {
        chgFlags &= ~AR_PARAM_startage;
    }

    public int getStartCount() { return count; }
    public void setStartCount(int count) {
        this.count = count;
        chgFlags |= AR_PARAM_startcount;
    }
    public void resetStartCount() {
        chgFlags &= ~AR_PARAM_startcount;
    }

    public String getStartSize() { return startSize; }
    public void setStartSize(String startSize) {
        this.startSize = startSize;
        chgFlags |= AR_PARAM_startsize;
    }
    public void resetStartSize() {
        chgFlags &= ~AR_PARAM_startsize;
    }

    public long getQueueTimeLimit() { return queue_time_limit; };

    public String toString() {
        String s = ((copyName == null) ? "[default]" : copyName) +
        ": bufsz=" + bufSize + ",bufLk=" + (bufLocked ? "T" : "F") +
        ",archmx=" + ((archMax == null) ? "-" : archMax) + ",drvs=" + drives +
        ",drvmax=" + ((drvMax == null) ? "-" : drvMax) +
        ",drvmin=" + ((drvMin == null) ? "-" : drvMin) +
        ",dskvol=" + ((dskVol == null) ? "-" : dskVol) +
        ",fillVSN=" + (fillVSNs ? "T" : "F") +
        ",unAge=" + (unarchAge ? "T" : "F") +
        ",res=" + reserve + ",join=" + join + ",rsort=" + rsort + ",sort=" +
        sort + ",offlcp=" + offlineCp + ",age=" + age + ",count=" + count +
        ",stSz=" + ((startSize == null) ? "-" : startSize) +
        ",ovrflMinSz=" + ((overflowMinSz == null) ? "-" : overflowMinSz) +
        ",recParams:" + recParams + ",directio=" + directio +
	",rearch_stage_copy=" + rearch_stage_copy +
	" [cf:" + Long.toHexString(chgFlags) + "]";
        return s;
    }
}
