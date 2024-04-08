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

// ident	$Id: Job.java,v 1.20 2008/12/16 00:08:53 am143972 Exp $

package com.sun.netstorage.samqfs.mgmt;

import com.sun.netstorage.samqfs.mgmt.fs.SamfsckJob;
import com.sun.netstorage.samqfs.mgmt.arc.job.ArCopyJob;
import com.sun.netstorage.samqfs.mgmt.arc.job.ArFindJob;
import com.sun.netstorage.samqfs.mgmt.stg.job.StagerJob;
import com.sun.netstorage.samqfs.mgmt.rel.ReleaserJob;
import com.sun.netstorage.samqfs.mgmt.media.MountJob;
import com.sun.netstorage.samqfs.mgmt.media.LabelJob;


import java.util.Vector;

public abstract class Job {

    // job state
    public static final short STATE_CURRENT = 0;
    public static final short STATE_PENDING = 1;
    public static final short STATE_HISTORY = 2; // not used

    // job types
    public static final short TYPE_ARCOPY   = 0;
    public static final short TYPE_ARFIND   = 1;
    public static final short TYPE_STAGE    = 2;
    public static final short TYPE_RELEASE  = 3;

    public static final short TYPE_MOUNT = 5; // media mount

    public static final short TYPE_SAMFSCK = 8;
    public static final short TYPE_LABEL   = 9;

    // jobs processed in the logic layer, via "activity" API:
    // cannot be retrieved via getJobs() API.
    public static final short TYPE_DUMPFS  = 10;
    public static final short TYPE_ENABLEDUMP = 11;
    public static final short TYPE_RESTORE    = 12;
    public static final short TYPE_SEARCHDUMP = 13;
    public static final short TYPE_ARCHIVE_FILES = 14;
    public static final short TYPE_RELEASE_FILES = 15;
    public static final short TYPE_STAGE_FILES   = 16;
    public static final short TYPE_RUN_EXPLORER  = 17;

    protected short type, state;
    protected long id; // job id
    protected long pid = -1; // used if this job maps to a Unix process

    // protected constructors
    protected Job(short type, short state, long id, long pid) {
        this.type = type;
        this.state = state;
        this.id = id;
        this.pid = pid;
    }
    protected Job(short type, short state, long id) {
        this(type, state, id, -1);
    }

    public short getType() {
        return type;
    }
    public short getState() {
        return state;
    }
    public long getID() {
        return id;
    }
    protected void setID(long id) {
        this.id = id;
    }
    protected void setPID(int pid) {
        this.pid = pid;
    }
    /* return -1 if processes of this type cannot be killed (history etc)  */
    public int terminate(Ctx c) throws SamFSException {
        System.out.println("terminate() type:" + type + ",state:" + state +
        ",pid:" + pid);
            if (state == STATE_HISTORY)
                throw new SamFSException("Cannot terminate jobs in this state");
            switch (type) {
                case TYPE_MOUNT:
                case TYPE_STAGE:
                    this.cancel(c);
                    break;
                default:
                    if (state == STATE_PENDING || pid < 0)
                        return -1;
                    terminateProcess(c, pid, type);
            }
            return 0;
    }
    // cancelable jobs MUST override this method
    protected void cancel(Ctx c) throws SamFSException {
        throw new SamFSException("Don't know how to cancel this type of job");
    }
    protected static native void terminateProcess(Ctx c, long pid, short type)
        throws SamFSException;


    /*
     * return current/pending jobs of specified type.
     * return null if invalid job type.
     */
    public static Job[] getJobs(Ctx c, short jobType) throws SamFSException {
        switch (jobType) {
            case Job.TYPE_ARFIND:
                return ArFindJob.getAll(c);
            case Job.TYPE_ARCOPY:
                return ArCopyJob.getAll(c);
            case Job.TYPE_STAGE:
                return StagerJob.getAll(c);
            case Job.TYPE_RELEASE:
                return ReleaserJob.getAll(c);
            case Job.TYPE_MOUNT:
                return MountJob.getAll(c);
            case Job.TYPE_SAMFSCK:
                return SamfsckJob.getAll(c);
            case Job.TYPE_LABEL:
                return LabelJob.getAll(c);
        }
        return null;
    }


    /**
     * @return an array of formatted strings
     * format is as follows:
     * activityid=<idString>,
     * starttime=<time>,
     * type=<type>, (SAMRDECOMPRESS, SAMRCRON, SAMRDUMP, SAMRSEARCH),
     * details=<details>,
     * -- the following are optional --
     * fsname=<fsname>,
     * dumpname=<dumpname>,
     * pid=<pid>,
     * restrictions=<filter>
     *
     * The following lists the name-value pairs applicable to the different type
     * SAMRCRONTAB:- updates the crontab file
     * activityid=%s,starttime=%ld,details=CrontabUpdate,type=%s,pid=%d
     *
     * SAMRDUMP:- created by takeDump() API
     * activityid=%s,starttime=%d,fsname=%s,details=%s,type=%s
     *
     * SAMRDECOMPRESS:-
     * activityid=%s,starttime=%d,fsname=%s,details=%s,type=%s,pid=%d
     *
     * SAMRSEARCH:-
     * activityid=%s,starttime=%d,fsname=%s,dumpname=%s,type=%s,restrictions=%s
     *
     * SAMRRESTORE:-
     * activityid=%s,starttime=%d,fsname=%s,dumpname=%s,type=%s
     *
     * SAMAARCHIVEFILES:-
     * activityid=%s,starttime=%ld,details=\"###\",type=%s,pid=%d
     *
     * SAMARELEASEFILES:-
     * activityid=%s,starttime=%ld,details=\"###\",type=%s,pid=%d
     *
     * SAMASTAGEFILES:-
     * activityid=%s,starttime=%ld,details=\"###\",type=%s,pid=%d
     *
     * SAMARUNEXPLORER:-
     * activityid=%s,starttime=%ld,details=\"/full/path/file\",type=%s,pid=%d
     *
     * SAMADISPATCHJOB:-
     * Multi-Host operations will share the same set of key value pairs.
     * These key-value pairs will apply for the following operation types:
     *		UnmountClients, MountClients AddClients, RemoveClients,
     *		SharedGrow, SharedShrink, SharedFSMountOptions
     *
     * Multi-Host Operation Keys:
     * activityid=%s (This matches the job id returned by the functions)
     * type=SAMADISPATCHJOB
     * operation=%s (sub type for each type of dispatch job)
     * starttime=%d
     * fsname=%s
     * host_count = int
     * hosts_responding = int
     * hosts_pending = int
     * status=success|partial_failure|failure|pending
     * error = overall_error_number (only present when failure or
     *			partial_failure set)
     * error_msg = overall_error_message
     * ok_hosts = hostf hoste hostc hostb hostn hostr
     * error_hosts = hosta hostd hostg
     * hosta = 31001 error message
     * hostd = 32200 error message
     * hostg = 34000 error message
     *
     * A note about multi-host error keys: The numbers used for the
     * error reporting keys map to error numbers. Error numbers are
     * returned to potentially allow localization of the messages in
     * the GUI even when the backend servers are running in the C
     * locale.
     *
     */
    public static native String[] getAllActivities(Ctx c, int maxEntries,
        String filter) throws SamFSException;


    public static native void cancelActivity(Ctx c, String id, String type)
        throws SamFSException;


    public String toString() {
        String s = "[" + type + "," + state + "," + id + "]";
        return s;
    }
}
