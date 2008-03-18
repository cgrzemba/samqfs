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

// ident	$Id: StageFile.java,v 1.12 2008/03/17 14:43:45 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.sun.netstorage.samqfs.mgmt.FileUtil;
import java.util.Properties;

public class StageFile extends RemoteFile {

    // keys for the name=value pairs returned by the jni layer
    public static final String NAME = "file_name";
    public static final String TYPE = "file_type";
    public static final String SIZE = "size";
    public static final String USER = "user";
    public static final String GROUP = "group";
    public static final String CREATE_DATE = "created";
    public static final String MODIFIED_DATE = "modified";
    public static final String ACCESSED_DATE = "accessed";
    public static final String PROTECTION = "protection";
    public static final String CPROTECTION = "cprotection";
    public static final String ARCHIVE_ATTS = "archive_atts";
    public static final String STAGE_ATTS = "stage_atts";
    public static final String RELEASE_ATTS = "release_atts";
    public static final String PARTIAL_RELEASE = "partial_release_sz";
    public static final String STATE_ONLINE = "online";
    public static final String STATE_OFFLINE = "offline";
    public static final String STATE_PARTIAL_ONLINE = "partial_online";
    public static final String SEGMENT_SIZE = "seg_size_mb";
    public static final String SEGMENT_STAGE_AHEAD  = "seg_stage_ahead";
    public static final String SEGMENT_COUNT = "seg_count";
    public static final String ARCH_DONE = "archdone";
    public static final String STAGE_PENDING = "stage_pending";

    // sam state
    public static final int ONLINE = 0x01;
    public static final int OFFLINE = 0x02;
    public static final int PARTIAL_ONLINE = 0x04;

    // delimitor used to separate each copy information
    public static final String COPY_DELIMITOR = "@@@";
    public static final String COPY_CONTENT_DELIMITOR = "###";


    private int type, onlineStatus;
    private int archiveAttributes, stageAttributes, releaseAttributes;
    private int onlineCount, partialOnlineCount, offlineCount;

    // protection in number
    private int protection;

    // protection in string form e.g. -rw-r--r--
    private String cprotection;

    private Properties properties;
    private String rawDetails, user, group;
    private long createdTime, accessedTime, partialReleaseSize;
    private long segmentSize, segmentStageAhead;
    private int archDone, stagePending, segCount;

    // variable to keep track if entry contains copy information
    private FileCopyDetails [] copyDetails;

    public StageFile(String name) {
        super(name);
    }

    public StageFile(String name, long size, long modifiedTime, int type) {
        super(name, size, modifiedTime, (type == FileUtil.FTYPE_DIR));

        this.type = type;
    }

    public int getType() {
        return this.type;
    }

    public boolean isDirectory() {
        return this.type == FileUtil.FTYPE_DIR;
    }

    public void setFileCopyDetails(FileCopyDetails [] copyDetails) {
        this.copyDetails = copyDetails;
    }

    public FileCopyDetails [] getFileCopyDetails() {
        return copyDetails;
    }

    public long getCreatedTime() {
        return this.createdTime;
    }

    public void setCreatedTime(long createdTime) {
        this.createdTime = createdTime;
    }

    public void setAccessedTime(long accessedTime) {
        this.accessedTime = accessedTime;
    }

    public long getAccessedTime() {
        return this.accessedTime;
    }

    public int getProtection() {
        return this.protection;
    }

    public void setProtection(int protection) {
        this.protection = protection;
    }

    public String getCProtection() {
        return this.cprotection;
    }

    public void setCProtection(String cprotection) {
        this.cprotection = cprotection;
    }

    public void setProperties(Properties properties) {
        this.properties = properties;
    }

    public void setRawDetails(String details) {
        rawDetails = details;
    }

    public String getRawDetails() {
        return rawDetails;
    }

    /**
     * Get/Set Stage Attributes
     *
     * FileUtil.AR_ATT_NEVER = 1
     * <ADD MORE>
     */
    public void setArchiveAttributes(int attributes) {
        this.archiveAttributes = attributes;
    }

    public int getArchiveAttributes() {
        return this.archiveAttributes;
    }

    /**
     * Get/Set Stage Attributes
     *
     * FileUtil.ST_ATT_NEVER = 1
     * FileUtil.ST_ATT_ASSOCIATIVE = 2
     */
    public void setStageAttributes(int attributes) {
        this.stageAttributes = attributes;
    }

    public int getStageAttributes() {
        return this.stageAttributes;
    }

    /**
     * Get/Set Release Attributes
     *
     * FileUtil.RL_ATT_NEVER = 1
     * FileUtil.RL_ATT_WHEN1COPY = 2
     */
    public void setReleaseAttributes(int attributes) {
        this.releaseAttributes = attributes;
    }

    public int getReleaseAttributes() {
        return this.releaseAttributes;
    }

    public void setPartialReleaseSize(long partialReleaseSize) {
        this.partialReleaseSize = partialReleaseSize;
    }

    public long getPartialReleaseSize() {
        return this.partialReleaseSize;
    }

    public void setSegmentCount(int segCount) {
        this.segCount = segCount;
    }

    public int getSegmentCount() {
        return this.segCount;
    }

    public void setSegmentSize(long segmentSize) {
        this.segmentSize = segmentSize;
    }

    public long getSegmentSize() {
        return this.segmentSize;
    }

    public void setSegmentStageAhead(long segmentStageAhead) {
        this.segmentStageAhead = segmentStageAhead;
    }

    public long getSegmentStageAhead() {
        return this.segmentStageAhead;
    }

    public void setArchDone(int archDone) {
        this.archDone = archDone;
    }

    public int getArchDone() {
        return this.archDone;
    }

    public void setStagePending(int stagePending) {
        this.stagePending = stagePending;
    }

    public int getStagePending() {
        return this.stagePending;
    }


    public void setUser(String user) {
        this.user = user;
    }

    public String getUser() {
        return user;
    }

    public void setGroup(String group) {
        this.group = group;
    }

    public String getGroup() {
        return group;
    }


    /**
     * Online/Offline/Partial Online is determined differently for files that
     * are segmented.
     *
     * e.g. For non-segmented files, online/offline/partial online is
     * mutually exclusive.
     *
     * e.g. For segmented files, online, offline, and partial online may
     * co-exists.  The value of each key represents the number of segments
     * that are in that state.
     *
     * online=2 offline=1 partial_online=2 means there are two online
     * segments, 1 offline segment and 2 partial_online segments.
     *
     * To determine what we are showing in the summary page per file entry,
     * online is used if all segments are online.  Offline is used if all
     * segments are offline.  Partial online will be used otherwise.
     *
     * onlineCount, offlineCount, partialOnlineCount carry the number of
     * segments that are in various sam state.  onlineStatus applies the
     * logic above and determine if a FILE is shown as
     * online/offline/partial online.
     */

    public String getLocalizedOnlineStatus() {
        switch (onlineStatus) {
            case ONLINE:
                return "fs.stage.status.online";
            case OFFLINE:
                return "fs.stage.status.offline";
            case PARTIAL_ONLINE:
                return "fs.stage.status.partial_online";
            default:
                return "fs.stage.status.unknown";
        }
    }

    public int getOnlineStatus() {
        if (segCount <= 1) {
            // Non-segment file
            if (partialOnlineCount > 0) {
                onlineStatus = PARTIAL_ONLINE;
            } else if (offlineCount > 0) {
                onlineStatus = OFFLINE;
            } else {
                onlineStatus = ONLINE;
            }
        } else {
            // segment file
            if (segCount == onlineCount) {
                onlineStatus = ONLINE;
            } else if (segCount == offlineCount) {
                onlineStatus = OFFLINE;
            } else {
                onlineStatus = PARTIAL_ONLINE;
            }
        }

        return onlineStatus;
    }

    public void setOnlineCount(int onlineCount) {
        this.onlineCount = onlineCount;
    }

    public int getOnlineCount() {
        return this.onlineCount;
    }

    public void setOfflineCount(int offlineCount) {
        this.offlineCount = offlineCount;
    }

    public int getOfflineCount() {
        return this.offlineCount;
    }

    public void setPartialOnlineCount(int partialOnlineCount) {
        this.partialOnlineCount = partialOnlineCount;
    }

    public int getPartialOnlineCount() {
        return this.partialOnlineCount;
    }

}
