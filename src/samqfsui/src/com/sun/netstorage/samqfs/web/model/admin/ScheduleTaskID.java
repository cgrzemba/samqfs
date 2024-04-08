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

// ident	$Id: ScheduleTaskID.java,v 1.8 2008/12/16 00:12:16 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.admin;


/**
 * this class encapsulates a schedule task id
 */
public final class ScheduleTaskID {
    private String id;
    private String descKey;

    private ScheduleTaskID() {}
    private ScheduleTaskID(String id, String desc) {
        this.id = id;
        this.descKey = desc;
    }

    /** returns this tasklets two character code */
    public String getId() { return this.id; }

    /**
     * returns a key to the resource bundle that can be used to localize the
     * description of this tasklet.
     */
    public String getDescription() { return this.descKey; }

    /** compare two ScheduleTaskID objects */
    public boolean equals(Object o) {
        return (o instanceof ScheduleTaskID &&
                ((ScheduleTaskID)o).getId().equals(this.id));
    }

    /**
     * encoding of SAM/QFS point product schedules is handled differently that
     * those of Intellistor. This method will be used to distinguish the two
     * types of task ids
     */
    public boolean isSamQFSTaskId() {
        if (this.id.equals(RECYCLER.getId()) ||
            this.id.equals(SNAPSHOT.getId()) ||
            this.id.equals(REPORT.getId())) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * return the ScheduleTaskID object matching the give two character task
     */
    public static ScheduleTaskID getScheduleTaskID(String taskid) {
        if (taskid == null) {
            return null;
        } else if (taskid.equals(AUTO_WORM.getId())) {
            return AUTO_WORM;
        } else if (taskid.equals(HASH_COMPUTATION.getId())) {
            return HASH_COMPUTATION;
        } else if (taskid.equals(HASH_INDEXING.getId())) {
            return HASH_INDEXING;
        } else if (taskid.equals(PERIODIC_AUDIT.getId())) {
            return PERIODIC_AUDIT;
        } else if (taskid.equals(AUTO_DELETION.getId())) {
            return AUTO_DELETION;
        } else if (taskid.equals(DEDUP.getId())) {
            return DEDUP;
        } else if (taskid.equals(RECYCLER.getId())) {
            return RECYCLER;
        } else if (taskid.equals(SNAPSHOT.getId())) {
            return SNAPSHOT;
        } else if (taskid.equals(REPORT.getId())) {
            return REPORT;
        } else {
            throw new UnsupportedOperationException("Unknown task id : "
                                                    + taskid);
        }
    }

    // The supported tasklet ids
    // intellistor tasklets
    public static final ScheduleTaskID AUTO_WORM =
        new ScheduleTaskID("AW", "admin.taskletid.autoworm");
    public static final ScheduleTaskID HASH_COMPUTATION =
        new ScheduleTaskID("HC", "admin.taskletid.hashcomputation");
    public static final ScheduleTaskID HASH_INDEXING =
        new ScheduleTaskID("HI", "admin.taskletid.hashindexing");
    public static final ScheduleTaskID PERIODIC_AUDIT =
        new ScheduleTaskID("PA", "admin.taskletid.periodicaudit");
    public static final ScheduleTaskID AUTO_DELETION =
        new ScheduleTaskID("AD", "admin.taskletid.automaticdeletion");
    public static final ScheduleTaskID DEDUP =
        new ScheduleTaskID("DD", "admin.taskletid.deduplication");

    // sam/qfs tasklets
    public static final ScheduleTaskID RECYCLER =
        new ScheduleTaskID("RC", "admin.taskletid.recyclerschedule");
    public static final ScheduleTaskID SNAPSHOT =
        new ScheduleTaskID("SN", "admin.taskletid.snapshotschedule");
    public static final ScheduleTaskID REPORT =
        new ScheduleTaskID("RP", "admin.taskletid.reportschedule");
}
