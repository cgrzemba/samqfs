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

// ident	$Id: BaseJob.java,v 1.12 2008/03/17 14:43:51 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.job;

import java.util.GregorianCalendar;

public interface BaseJob {

    public static final int CONDITION_CURRENT = 0;
    public static final int CONDITION_PENDING = 1;
    public static final int CONDITION_HISTORY = 2;
    public static final int START_JOB_TYPES = 0;
    public static final int TYPE_ARCHIVE_COPY = 1;
    public static final int TYPE_ARCHIVE_SCAN = 2;
    public static final int TYPE_STAGE = 4;
    public static final int TYPE_RELEASE = 5;
    public static final int TYPE_RECYCLE = 6;
    public static final int TYPE_MOUNT = 7;

    /**
     * @deprecated
     */
    public static final int TYPE_LOG_ROTATION = -10;
    public static final int TYPE_FSCK = 8;
    public static final int TYPE_TPLABEL = 9;
    public static final int END_4_3_JOB_TYPES = 10;

    // job types introduced in 4.4
    public static final int TYPE_DUMP = 20; // FS DUMP
    public static final String TYPE_ENABLE_DUMP_STR = "Jobs.jobType.dumpEnable";
    public static final int TYPE_ENABLE_DUMP = 21;
    public static final int TYPE_RESTORE_SEARCH = 22;
    public static final String TYPE_RESTORE_SEARCH_STR =
        "Jobs.jobType.restoreSearch";
    public static final int TYPE_RESTORE = 23;
    public static final String TYPE_RESTORE_STR = "Jobs.jobType.restore";

    // Since 4.6
    public static final int TYPE_ARCHIVE_FILES = 50;
    public static final int TYPE_RELEASE_FILES = 51;
    public static final int TYPE_STAGE_FILES = 52;
    public static final int TYPE_RUN_EXPLORER = 53;
    public static final String TYPE_ARCHIVE_FILES_STR =
        "Jobs.jobType.archivefiles";
    public static final String TYPE_RELEASE_FILES_STR =
        "Jobs.jobType.releasefiles";
    public static final String TYPE_STAGE_FILES_STR =
        "Jobs.jobType.stagefiles";
    public static final String TYPE_RUN_EXPLORER_STR =
        "Jobs.jobType.runexplorer";
    public static final long INVALID_JOB_ID = -1;

    public long getJobId();
    public int getCondition();
    public int getType();

    public String getDescription();

    // Start Time and Date could be null for certain type/condition of jobs.
    public GregorianCalendar getStartDateTime();

    // End Time and Date could be null for certain type/condition of jobs.
    public GregorianCalendar getEndDateTime();

    /**
     * @deprecated
     */
    public GregorianCalendar getLastDateTime();
}
