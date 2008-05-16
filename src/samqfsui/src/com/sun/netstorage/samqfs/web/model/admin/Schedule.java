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

// ident	$Id: Schedule.java,v 1.17 2008/05/16 18:38:58 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.admin;

import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.text.FieldPosition;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;

/**
 * This class defines a task schedule define by
 * @see com.sun.netstorage.samqfs.mgmt.admin.TaskSchedule
 */
public abstract class Schedule {
    // symbolic constants used as keys to the comma delimited schedule string
    // used by JNI & C-API
    public static final String TASK_ID = "task";
    public static final String TASK_NAME = "id";
    public static final String START_TIME = "starttime";
    public static final String PERIODICITY = "periodicity";
    public static final String DURATION = "duration";

    // members
    protected ScheduleTaskID taskId; // NOTE: task type, maps to the 'task' key
    protected String taskName; // NOTE: unique task identifier, maps to 'id'
    protected Date startTime;
    protected long periodicity;
    protected int periodicityUnit;
    protected long duration;
    protected int durationUnit;

    protected final String c = ",", e = "=", colon = ":";

    /** constructors */
    protected Schedule() {}

    protected Schedule(Properties props) {
        decode(props);
    }

    /**
     * Sub-classes of this class should implement this method to decode the
     * values that are specific to them.
     */
    protected abstract Properties decodeSchedule(Properties props);

    /**
     * Sub-classes of this class should implement this method to encode the
     * values that are specific to them.
     */
    public abstract String encodeSchedule();

    /**
     * parses the comma delimited name-value pair used by the JNI & C-API to an
     * instance of this class <code>Schedule</code> used by the Logic and
     * Presentation Layers.
     */
    protected Properties decode(Properties props) {
        // decode general schedule attributes

        // task id
        this.taskId =
            ScheduleTaskID.getScheduleTaskID(props.getProperty(TASK_ID));

        // task name
        this.taskName = props.getProperty(TASK_NAME);

        // start date
        String temp = props.getProperty(START_TIME);
        try {
            this.startTime =
                new SimpleDateFormat("yyyyMMddHHmm").parse(temp.trim());
        } catch (ParseException pe) {
            TraceUtil.trace1("ParseException caught in decode()!");
            TraceUtil.trace1("Reason: " + pe.getMessage());
        }

        // periodicity
        temp = props.getProperty(PERIODICITY);
        this.periodicity = SamQFSUtil.getLongValSecond(temp);
        this.periodicityUnit = SamQFSUtil.getTimeUnitInteger(temp);

        // duration
        temp = props.getProperty(DURATION);
        this.duration = SamQFSUtil.getLongValSecond(temp);
        this.durationUnit = SamQFSUtil.getTimeUnitInteger(temp);

        // decode the child class schedule attributes
        return decodeSchedule(props);
    }

    /**
     * converts an instance of this class <code>Schedule</code> to the comma
     * delimited name-value pair used by JNI & C-API.
     *
     * @return String - encoded string
     */
    protected String encode() {
        StringBuffer buffer = new StringBuffer();

        // task id
        String temp = this.taskId == null ? "" : this.taskId.getId();
        buffer.append(TASK_ID).append(e).append(temp);

        // task name
        temp = this.taskName == null ? "" : this.taskName.trim();
        buffer.append(c).append(TASK_NAME).append(e).append(this.taskName);

        // start time
        if (this.startTime != null) {
            SimpleDateFormat format = new SimpleDateFormat("yyyyMMddHHmm");
            StringBuffer buf = new StringBuffer();
            buf = format.format(this.startTime,
                                buf,
                                new FieldPosition(0));
            buffer.append(c).append(START_TIME).append(e).append(buf);
        }

        // periodicity
        if (this.periodicity != -1) {
            buffer.append(c).append(PERIODICITY).append(e)
                .append(Long.toString(this.periodicity))
                .append(SamQFSUtil.getTimeUnitString(this.periodicityUnit));
        }

        // duration
        if (this.duration != -1) {
            buffer.append(c).append(DURATION).append(e)
                .append(Long.toString(this.duration))
                .append(SamQFSUtil.getTimeUnitString(this.durationUnit));
        }

        // encode the specific schedule attributes
        String subclassSchedule = encodeSchedule();
        if (subclassSchedule != null && subclassSchedule.length() != 0) {
            buffer.append(subclassSchedule);
        }

        return buffer.toString();
    }

    /**
     * override Object.toString
     */
    public String toString() {
        return encode();
    }

    // setters
    public void setTaskId(ScheduleTaskID id) { this.taskId = id; }
    public void setTaskName(String task) { this.taskName = task; }
    public void setStartTime(Date date) { this.startTime = date; }
    public void setPeriodicity(long periodicity) {
        this.periodicity = periodicity;
    }
    public void setPeriodicityUnit(int unit) { this.periodicityUnit = unit; }
    public void setDuration(long duration) { this.duration = duration; }
    public void setDurationUnit(int unit) { this.durationUnit = unit; }

    // getters
    public ScheduleTaskID getTaskId() { return this.taskId; }
    public String getTaskName() { return this.taskName; }
    public Date getStartTime() { return this.startTime; }
    public long getPeriodicity() { return this.periodicity; }
    public int getPeriodicityUnit() { return this.periodicityUnit; }
    public long getDuration() { return this.duration; }
    public int getDurationUnit() { return this.durationUnit; }
}
