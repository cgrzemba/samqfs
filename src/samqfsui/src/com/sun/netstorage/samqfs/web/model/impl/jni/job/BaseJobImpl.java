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

// ident	$Id: BaseJobImpl.java,v 1.13 2008/05/15 04:34:10 kilemba Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import java.util.GregorianCalendar;
import java.util.Date;

public class BaseJobImpl implements BaseJob {

    private Job jniJob = null;
    private long jobID = -1;
    private int condition = -1;
    private int type = -1;
    private String desc = null;
    private GregorianCalendar startDateTime = null;
    private GregorianCalendar endDateTime = null;

    public BaseJobImpl() {
    }

    public BaseJobImpl(long jobID,
                       int condition,
                       int type,
                       String desc,
                       GregorianCalendar startDateTime,
                       GregorianCalendar endDateTime) {

        this.jobID = jobID;
        this.condition = condition;
        this.type = type;
        this.desc = desc;
        this.startDateTime = startDateTime;
        this.endDateTime = endDateTime;
    }

    public BaseJobImpl(Job job) {
        this.jniJob = job;
        update();
    }

    public Job getJniJob() { return jniJob; }

    public long getJobId() { return jobID; }

    public int getCondition() { return condition; }

    public void setCondition(int condition) {
        this.condition = condition;
    }

    public int getType() { return type; }

    public String getDescription() { return desc; }
    public void setDescription(String desc) {
        if (SamQFSUtil.isValidString(desc))
            this.desc = desc;
    }

    // Start Time and Date could be null for certain type/condition of jobs.
    public GregorianCalendar getStartDateTime() { return startDateTime; }
    public void  setstartDateTime(GregorianCalendar startDateTime) {
        this.startDateTime = startDateTime;
    }

    public Date getStartTime() {
        return this.startDateTime != null
            ? new Date(this.startDateTime.getTimeInMillis()) : null;
    }

    // End Time and Date could be null for certain type/condition of jobs.
    public GregorianCalendar getEndDateTime() {	return endDateTime; }
    public void  setEndDateTime(GregorianCalendar endDateTime) {
        this.endDateTime = endDateTime;
    }

    public GregorianCalendar getLastDateTime() { return null; }

    public String toString() {
        StringBuffer buf = new StringBuffer();

        buf.append("Job ID: ")
            .append(jobID)
            .append("\n")
            .append("Condition: ")
            .append(condition)
            .append("\n")
            .append("Type: ")
            .append(type)
            .append("\n")
            .append("Description: ")
            .append(desc)
            .append("\n")
            .append("Start Time: ")
            .append(SamQFSUtil.dateTime(startDateTime))
            .append("\n")
            .append("End Time: ")
            .append(SamQFSUtil.dateTime(endDateTime));

        return buf.toString();
    }

    private void update() {

        if (jniJob != null) {
            this.jobID = jniJob.getID();

            switch (jniJob.getState()) {
            case Job.STATE_CURRENT:
                condition = BaseJob.CONDITION_CURRENT;
                break;
            case Job.STATE_PENDING:
                condition = BaseJob.CONDITION_PENDING;
                break;
            case Job.STATE_HISTORY:
                condition = BaseJob.CONDITION_HISTORY;
                break;
            }

            switch (jniJob.getType()) {
            case Job.TYPE_ARCOPY:
                type = BaseJob.TYPE_ARCHIVE_COPY;
                break;
            case Job.TYPE_ARFIND:
                type = BaseJob.TYPE_ARCHIVE_SCAN;
                break;
            case Job.TYPE_STAGE:
                type = BaseJob.TYPE_STAGE;
                break;
            case Job.TYPE_RELEASE:
                type = BaseJob.TYPE_RELEASE;
                break;
            case Job.TYPE_MOUNT:
                type = BaseJob.TYPE_MOUNT;
                break;
            case Job.TYPE_SAMFSCK:
                type = BaseJob.TYPE_FSCK;
                break;
            case Job.TYPE_LABEL:
                type = BaseJob.TYPE_TPLABEL;
                break;
            }
        }
    }
}
