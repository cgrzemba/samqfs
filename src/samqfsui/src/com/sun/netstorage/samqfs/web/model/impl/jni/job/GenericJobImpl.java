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

// ident    $Id: GenericJobImpl.java,v 1.6 2008/12/16 00:12:21 am143972 Exp $


package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.GenericJob;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import java.util.GregorianCalendar;
import java.util.Properties;


/**
 * This file is used for generic job types that are introduced in 4.6.  Use
 * this class for the following new job types that are retrieved from
 * listActivities (C-layer), getAllActivities (JNI layer)
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
 */
public class GenericJobImpl extends BaseJobImpl implements GenericJob {

    public final static String KEY_ACTIVITY_ID = "activityid";
    public final static String KEY_STARTTIME = "starttime";
    public final static String KEY_TYPE = "type";
    public final static String KEY_DETAILS = "details";
    public final static String KEY_PID = "pid";
    protected int pid = -1;

    // next 2 fields are needed when a job cancel is requested
    protected long activityId = -1;
    protected String activityTypeStr;

    public GenericJobImpl(Properties props) throws SamFSException {
        super(calcJobID(props),
              BaseJob.CONDITION_CURRENT,
              calcTypeID(props.getProperty(KEY_TYPE)),
              props.getProperty(KEY_DETAILS),
              calcCalendar(props.getProperty(KEY_STARTTIME)),
              null);

        activityId =
            ConversionUtil.strToLongVal(props.getProperty(KEY_ACTIVITY_ID));
        activityTypeStr = props.getProperty(KEY_TYPE);
        String pidStr = props.getProperty(KEY_PID);
        try {
            pid = Integer.parseInt(pidStr);
        } catch (NumberFormatException numEX) {
            throw new SamFSException("Logic: PID is not an integer!");
        }
    }

    public GenericJobImpl(String propsStr) throws SamFSException {
        this(ConversionUtil.strToProps(propsStr));
    }

    private static long calcJobID(Properties props)
        throws SamFSException {
        if (props == null) {
            throw new SamFSException("Internal error: null properties");
        }

        // compute jobID that will be displayed by the UI
        long id = 100 *
            ConversionUtil.strToLongVal(props.getProperty(KEY_ACTIVITY_ID));

        String typeStr = props.getProperty(KEY_TYPE);
        if (typeStr.equals("SAMAARCHIVEFILES")) {
            id += Job.TYPE_ARCHIVE_FILES;
        } else if (typeStr.equals("SAMARELEASEFILES")) {
            id += Job.TYPE_RELEASE_FILES;
        } else if (typeStr.equals("SAMASTAGEFILES")) {
            id += Job.TYPE_STAGE_FILES;
        } else if (typeStr.equals("SAMARUNEXPLORER")) {
            id += Job.TYPE_RUN_EXPLORER;
        }
        return id;
    }

    private static int calcTypeID(String typeStr) {
        int type = -1;
        if (typeStr.equals("SAMAARCHIVEFILES")) {
            type = BaseJob.TYPE_ARCHIVE_FILES;
        } else if (typeStr.equals("SAMARELEASEFILES")) {
            type = BaseJob.TYPE_RELEASE_FILES;
        } else if (typeStr.equals("SAMASTAGEFILES")) {
            type = BaseJob.TYPE_STAGE_FILES;
        } else if (typeStr.equals("SAMARUNEXPLORER")) {
            type = BaseJob.TYPE_RUN_EXPLORER;
        }
        return type;
    }

    private static GregorianCalendar calcCalendar(String secs)
        throws SamFSException {
        GregorianCalendar cal = new GregorianCalendar();
        cal.setTimeInMillis(ConversionUtil.strToLongVal(secs) * 1000);
        return cal;
    }

    public void cancel(Ctx c) throws SamFSException {
        Job.cancelActivity(c, Long.toString(activityId), activityTypeStr);
    }

    public int getPID() throws SamFSException {
        return pid;
    }
}
