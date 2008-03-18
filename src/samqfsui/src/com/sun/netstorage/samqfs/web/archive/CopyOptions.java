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

// ident	$Id: CopyOptions.java,v 1.6 2008/03/17 14:40:43 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

public interface CopyOptions {
    // fields common between disk and tape copies

    // Queue management
    public static final String START_AGE = "startAge";
    public static final String START_AGE_UNIT = "startAgeUnit";
    public static final String START_COUNT = "startCount";
    public static final String START_SIZE = "startSize";
    public static final String START_SIZE_UNIT = "startSizeUnit";

    // recycling
    public static final String DISABLE_RECYCLER = "disableRecycler";
    public static final String HIGH_WATER_MARK = "highWaterMark";
    public static final String HIGH_WATER_MARK_UNIT = "highWaterMarkUnit";
    public static final String MIN_GAIN = "minGain";
    public static final String EMAIL_ADDRESS = "emailAddress";

    // tuning parameters
    public static final String OFFLINE_METHOD = "offlineMethod";
    public static final String BUFFER_SIZE = "bufferSize";
    public static final String LOCK_BUFFER = "lockBuffer";
    public static final String MAX_SIZE_ARCHIVE = "maxSizeForArchive";
    public static final String MAX_SIZE_ARCHIVE_UNIT = "maxSizeForArchiveUnit";

    // archive organization
    public static final String UNARCHIVE_TIME_REF = "unarchiveTimeRef";

    // fields only applicable to tape copies only

    // archive organization
    public static final String MEDIA_TYPE = "mediaType";

    // reservation method
    public static final String RM_ATTRIBUTE = "rmAttribute";
    public static final String RM_POLICY = "rmPolicy";
    public static final String RM_FILESYSTEM = "rmFileSystem";
    public static final String SORT_METHOD = "sortMethod";
    public static final String JOIN_METHOD = "joinMethod";
    public static final String FILL_VSNS = "fillVSNs";

    // tuning parameters
    public static final String DRIVES = "drivesToUse";
    public static final String MAX_PER_DRIVE = "maxSizePerDrive";
    public static final String MAX_PER_DRIVE_UNIT = "maxSizePerDriveUnit";
    public static final String DRIVE_TRIGGER = "multiDriveTrigger";
    public static final String DRIVE_TRIGGER_UNIT = "multiDriveTriggerUnit";
    public static final String MIN_OVERFLOW = "minOverflow";
    public static final String MIN_OVERFLOW_UNIT = "minOverflowUnit";

    // recycling
    public static final String RECYCLE_SIZE = "recycleSize";
    public static final String RECYCLE_SIZE_UNIT = "recycleSizeUnit";
    public static final String MAX_VSN_COUNT = "maxVSNCount";
}
