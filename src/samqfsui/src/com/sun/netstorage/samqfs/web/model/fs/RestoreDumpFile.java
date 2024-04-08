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
 * or https://illumos.org/license/CDDL.
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

// ident $Id: RestoreDumpFile.java,v 1.15 2008/12/16 00:12:18 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.fs;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemJobManager;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;

import java.util.Properties;
import java.util.GregorianCalendar;

public class RestoreDumpFile {

    /* valid keys are defined below; keys are case-insensitive */
    protected static final String KEY_STATE = "status";
    protected static final String KEY_SIZE = "size";
    protected static final String KEY_CREATED  = "created";
    protected static final String KEY_MODIFIED = "modified";
    protected static final String KEY_LOCKED = "locked";
    protected static final String KEY_COMPRESSED = "compressed";
    protected static final String KEY_INDEXED = "indexed";
    protected static final String KEY_NUMENTRIES = "numnodes";

    // must match strings defined in pub/mgmt/restore.h
    public static final String STATE_OFFLINE = "offline";
    public static final String STATE_COMPRESSED = "compressed";
    public static final String STATE_DECOMPRESSING = "decompressing";
    public static final String STATE_INDEXING = "indexing";
    public static final String STATE_UNINDEXED = "unindexed";
    public static final String STATE_AVAIL = "available";
    public static final String STATE_BROKEN = "broken";

    protected String stateStr;
    protected String size; // in bytes

    protected boolean isSelected;
    protected String fileName;
    protected GregorianCalendar createdTime;
    protected GregorianCalendar modTime;

    protected Boolean isLocked = null;
    protected Boolean isCompressed = null;
    protected Boolean isIndexed = null;
    protected Integer numEntries = null;

    // Simulator support
    private boolean isIndexing = false;
    private int indexingCnt = 0;


    public RestoreDumpFile(String fileName, Properties fileProps)
        throws SamFSException {

        String sizeStr, crTimeStr, modTimeStr;

        this.fileName = fileName;

        stateStr   = fileProps.getProperty(KEY_STATE);
        size = fileProps.getProperty(KEY_SIZE);

        crTimeStr  = fileProps.getProperty(KEY_CREATED);
        if (crTimeStr == null)
            createdTime = null;
        else {
            createdTime = new GregorianCalendar();
            createdTime.setTimeInMillis(1000 *
                ConversionUtil.strToLongVal(crTimeStr));
	}

        modTimeStr = fileProps.getProperty(KEY_MODIFIED);
        if (modTimeStr == null)
            modTime = null;
        else {
            modTime = new GregorianCalendar();
            modTime.setTimeInMillis(1000 *
                ConversionUtil.strToLongVal(modTimeStr));
        }

        String compressedStr = fileProps.getProperty(KEY_COMPRESSED);
        if (compressedStr == null) {
            isCompressed = null;
        } else {
            int compressedInt = ConversionUtil.strToIntVal(compressedStr);
            if (compressedInt > 0) {
                isCompressed = Boolean.TRUE;
            } else {
                isCompressed = Boolean.FALSE;
            }
        }
        isLocked = propertyValueToBoolean(fileProps.getProperty(KEY_LOCKED));
        isIndexed = propertyValueToBoolean(fileProps.getProperty(KEY_INDEXED));

        String numEntriesStr = fileProps.getProperty(KEY_NUMENTRIES);
        if (numEntriesStr == null) {
            numEntries = null;
        } else {
            numEntries = new Integer(ConversionUtil.strToIntVal(numEntriesStr));
        }
    }

    // Simulator support
    public RestoreDumpFile(String fileName, String state, String size,
        GregorianCalendar createdTime, GregorianCalendar modTime,
        Boolean isLocked, Boolean isCompressed, Boolean isIndexed,
        Integer numEntries) {

        this.isSelected  = false;
        this.fileName = fileName;
        this.stateStr = state;
        this.size = size;
        this.createdTime = (GregorianCalendar) createdTime.clone();
        this.modTime = (GregorianCalendar) modTime.clone();
        this.isLocked = isLocked;
        this.isCompressed = isCompressed;
        this.isIndexed = isIndexed;
        this.numEntries = numEntries;
    }

    private Boolean propertyValueToBoolean(String propertyVal) {
        if (propertyVal == null) {
            return null;
        }

        if (propertyVal.equals("1")) {
            return Boolean.TRUE;
        } else {
            return Boolean.FALSE;
        }
    }

    // Use to manage "Create index" and "Delete index" buttons.
    // Takes into account compressed state too and type of compression.
    public boolean isAvailable() {
        return stateStr.equals(STATE_AVAIL);
    }

    // Use to display if the recovery point is broken or not
    public boolean isBroken() {
        return stateStr.equals(STATE_BROKEN);
    }

    /**
     * Use only for status column dispaly.
     */
    public boolean isIndexed() {
        if (isIndexed == null) {
            // 4.4 servers
            if (stateStr == null) {
                return false;
            } else {
                return (stateStr.compareTo(STATE_AVAIL) == 0);
            }
        } else {
            return isIndexed.booleanValue();
        }
    }

    /**
     * Use only for status column dispaly.
     */
    public boolean isCompressed() {
        if (isCompressed == null) {
            if (stateStr == null) {
                return true;
            } else {
                return stateStr.equals(STATE_COMPRESSED);
            }
        } else {
            return isCompressed.booleanValue();
        }
    }

    public boolean isLocked() {
        if (isLocked == null) {
            return false;
        } else {
            return isLocked.booleanValue();
        }
    }

    public void setIsLocked(boolean isLocked) {
        this.isLocked = Boolean.valueOf(isLocked);
    }

    public boolean isProcessing() {
        if (stateStr == null) {
            return false;
        } else {
            if (stateStr.equals(STATE_DECOMPRESSING) ||
                stateStr.equals(STATE_INDEXING)) {
                return true;
            } else {
                return false;
            }
        }
    }

    public String getSize() {
        return size;
    }

    public boolean getIsSelected() {
        return this.isSelected;
    }
    public void setIsSelected(boolean isSelected) {
        this.isSelected = isSelected;
    }

    public String getFileName() {
        return this.fileName;
    }

    public GregorianCalendar getCreatedTime() {
        return this.createdTime;
    }
    public GregorianCalendar getModTime() {
        return this.modTime;
    }

    public Integer getNumEntries() {
        return this.numEntries;
    }

    public String toString() {
        long created = (createdTime == null) ? -1 :
                       (createdTime.getTimeInMillis() / 1000);
        long modif = (modTime == null) ? -1 :
                       (modTime.getTimeInMillis() / 1000);
        NonSyncStringBuffer s = new NonSyncStringBuffer()
            .append(fileName).append(": ")
            .append(KEY_STATE).append("=").append(stateStr).append(",")
            .append(KEY_SIZE).append("=").append(size).append(",")
            .append(KEY_CREATED).append("=").append(created).append(",")
            .append(KEY_MODIFIED).append("=").append(modif);
        if (isLocked != null) {
            s.append(",").append(KEY_LOCKED).append("=")
             .append(isLocked.toString());
        }
        if (isCompressed != null) {
            s.append(",").append(KEY_COMPRESSED).append("=")
             .append(isCompressed.toString());
        }
        if (isIndexed != null) {
            s.append(",").append(KEY_INDEXED).append("=")
             .append(isIndexed.toString());
        }
        if (numEntries != null) {
            s.append(",").append(KEY_NUMENTRIES).append("=")
             .append(numEntries.toString());
        }

        return s.toString();
    }

    private long enableDumpJobId = BaseJob.INVALID_JOB_ID;

    public boolean getIsIndexing()  {
        return this.isIndexing;
    }

    public void startMakingAvailable(long jobId) {
        this.enableDumpJobId = jobId;
        this.isIndexing = true;
        this.indexingCnt = 2;  // Job lasts 2 refreshes.
        this.stateStr = STATE_INDEXING;
    }

    public long getEnableDumpJobId() {
        return this.enableDumpJobId;
    }

    public void changeNextState(String serverName) throws SamFSException {
        if (!this.isIndexing) {
            return;
        }

        if (this.indexingCnt > 0) {
            this.indexingCnt--;
        } else {
            // All done
            this.stateStr = STATE_AVAIL;
            isIndexing = false;
            isIndexed = Boolean.TRUE;
            isCompressed = Boolean.FALSE;
            // Stop the job
            SamQFSSystemJobManager jobMgr = SamUtil.getModel(serverName)
                                                .getSamQFSSystemJobManager();
            BaseJob theJob = jobMgr.getJobById(this.enableDumpJobId);
            if (theJob != null) {
                jobMgr.cancelJob(theJob);
            }
            this.enableDumpJobId = BaseJob.INVALID_JOB_ID;
        }
    }

    public void makeUnindexed() {
        this.stateStr = STATE_UNINDEXED;
        isIndexed = Boolean.FALSE;
    }
}
