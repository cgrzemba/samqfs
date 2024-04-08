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

// ident $Id: RestoreJobImpl.java,v 1.17 2008/12/16 00:12:21 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni.job;

import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.fs.RestoreFile;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSSystemModelImpl;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.EnableDumpJob;
import com.sun.netstorage.samqfs.web.model.job.FSDumpJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreSearchJob;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import java.util.GregorianCalendar;
import java.util.Properties;

public class RestoreJobImpl extends BaseJobImpl
    implements RestoreJob, RestoreSearchJob, EnableDumpJob, FSDumpJob {

    final static String KEY_ACTIVITY_ID = "activityid";
    final static String KEY_STARTTIME = "starttime";
    final static String KEY_TYPE = "type";
    final static String KEY_DETAILS = "details";
    // optional
    final static String KEY_FSNAME = "fsname";
    final static String KEY_DUMPNAME = "dumpname"; // SAMRDUMP/SEARCH/RESTORE
    final static String KEY_PID = "pid"; // SAMRDECOMPRESS only
    final static String KEY_RESTRICT = "restrictions"; // SAMRSEARCH only
    final static String KEY_STATUS = "status"; // SAMRRESTORE only
    final static String KEY_FILENAME = "filename"; // SAMRESTORE
    final static String KEY_REPLACE_TYPE = "replaceType"; // SAMRESTORE
    final static String KEY_RESTORE_TO = "destname";  // SAMRESTORE
    final static String KEY_ONLINESTATUS = "copy"; // SAMRESTORE
    final static String KEY_FILES_TODO = "filestodo";
    final static String KEY_FILES_DONE = "filesdone";

    protected String fsName, dumpName, fileName, restrictions;
    protected int replaceType = SamQFSSystemFSManager.RESTORE_REPLACE_NEVER;
    protected int onlineStatusAfterRestore = RestoreFile.STG_COPY_ASINDUMP;
    protected String restoreToPath;
    protected String filesToDo = "0";
    protected String filesDone = "0";

    // next 2 fields are needed when a job cancel is requested
    protected long activityId = -1;
    protected String activityTypeStr;

    private static long calcJobID(Properties props) throws SamFSException {
        if (props == null)
            throw new SamFSException("internal error: null properties");

        // compute jobID that will be displayed by the UI
        long id = 100 *
            ConversionUtil.strToLongVal(props.getProperty(KEY_ACTIVITY_ID));

        String typeStr = props.getProperty(KEY_TYPE);
        if (typeStr.equals("SAMRDUMP")) { id += Job.TYPE_DUMPFS; }
        if (typeStr.equals("SAMRSEARCH")) { id += Job.TYPE_SEARCHDUMP; }
        if (typeStr.equals("SAMRDECOMPRESS")) { id += Job.TYPE_ENABLEDUMP; }
        if (typeStr.equals("SAMRRESTORE")) { id += Job.TYPE_RESTORE; }
        return id;
    }

    private static int calcTypeID(String typeStr) {
        int type = -1;
        if (typeStr.equals("SAMRDUMP")) { type = BaseJob.TYPE_DUMP; }
        if (typeStr.equals("SAMRSEARCH")) {
            type = BaseJob.TYPE_RESTORE_SEARCH;
        }
        if (typeStr.equals("SAMRDECOMPRESS")) {
            type = BaseJob.TYPE_ENABLE_DUMP;
        }
        if (typeStr.equals("SAMRRESTORE")) { type = BaseJob.TYPE_RESTORE; }
        return type;
    }

    private static GregorianCalendar calcCalendar(String secs)
        throws SamFSException {
        GregorianCalendar cal = new GregorianCalendar();
        cal.setTimeInMillis(ConversionUtil.strToLongVal(secs) * 1000);
        return cal;
    }

    public RestoreJobImpl(Properties props) throws SamFSException {
        super(calcJobID(props),
              BaseJob.CONDITION_CURRENT,
              calcTypeID(props.getProperty(KEY_TYPE)),
              props.getProperty(KEY_DETAILS),
              calcCalendar(props.getProperty(KEY_STARTTIME)),
              null);
        activityId =
            ConversionUtil.strToLongVal(props.getProperty(KEY_ACTIVITY_ID));
        activityTypeStr = props.getProperty(KEY_TYPE);
        // optional properties
        fsName = props.getProperty(KEY_FSNAME);
        switch (this.getType()) {
        case BaseJob.TYPE_ENABLE_DUMP:
            dumpName = props.getProperty(KEY_DETAILS);
            break;
        case BaseJob.TYPE_RESTORE:
            filesToDo = props.getProperty(KEY_FILES_TODO);
            filesDone = props.getProperty(KEY_FILES_DONE);
            dumpName = props.getProperty(KEY_DUMPNAME);
            fileName = props.getProperty(KEY_FILENAME);
            restoreToPath = props.getProperty(KEY_RESTORE_TO);
            String replaceTypeStr = props.getProperty(KEY_REPLACE_TYPE);
            String onlineStatusStr = props.getProperty(KEY_ONLINESTATUS);
            try {
                // Need to check for null for 4.4 servers
                if (replaceTypeStr != null) {
                    replaceType = Integer.parseInt(replaceTypeStr);
                }
                if (onlineStatusStr != null) {
                    onlineStatusAfterRestore =
                        Integer.parseInt(onlineStatusStr);
                }
            } catch (NumberFormatException e) {
                // Development error
                throw new SamFSException("Invalid property value: " +
                                         "requires an int." +
                                         props);
            }
            break;
        case BaseJob.TYPE_DUMP:
            dumpName = props.getProperty(KEY_DUMPNAME);
            break;
        case BaseJob.TYPE_RESTORE_SEARCH:
            dumpName = props.getProperty(KEY_DUMPNAME);
            restrictions = props.getProperty(KEY_RESTRICT);
            break;
        }
    }

    public RestoreJobImpl(String propsStr) throws SamFSException {
        this(ConversionUtil.strToProps(propsStr));
    }

    public void cancel(Ctx c) throws SamFSException {
        Job.cancelActivity(c, Long.toString(activityId), activityTypeStr);
    }

    /**
     * @return the name filesystem currently processed
     */
    public String getFileSystemName() { return fsName; }

    public String getFileName() { return fileName; }

    public int getReplaceType() { return replaceType; }

    public String getDumpFileName() { return dumpName; }

    public String getSearchCriteria() { return restrictions; }

    /**
     * Returns the current status of the restore oepration.
     *
     * @param hostname The name of the machine hosting the restore operation.
     * If null, the method will return the most recently obtained status (status
     * is obtained during object construction and during calls to this method).
     * If not null the method will refresh its info which entails a round
     * trip to the server which is more expensive, but results in data that is
     * more current. If the job has completed and there is nothing to update,
     * the method will return the most recent status.
     */
    public String getRestoreStatus(String hostname) throws SamFSException {
        if (hostname != null) {
            // Update status data
            Ctx c = ((SamQFSSystemModelImpl)SamQFSFactory.getSamQFSAppModel().
                    getSamQFSSystemModel(hostname)).getJniContext();
            String[] jobsStrs = Job.getAllActivities(c, 2,
                                    KEY_ACTIVITY_ID + "=" + activityId);
            if (jobsStrs != null && jobsStrs.length > 0) {
                Properties props = ConversionUtil.strToProps(jobsStrs[0]);
                String toDo = props.getProperty(KEY_FILES_TODO);
                String done = props.getProperty(KEY_FILES_DONE);
                if (toDo != null && done != null) {
                    filesToDo = toDo;
                    filesDone = done;
                }
            }
        }

        // Format data into string
        int total = -1;
        try {
            total = Integer.parseInt(filesToDo);
        } catch (NumberFormatException numEx) {
            return SamUtil.getResourceStringWithoutL10NArgs(
                        "JobsDetails.restore.status.withouttodo",
                        new String [] {filesDone});
        }

        if (total <= 0) {
            return SamUtil.getResourceStringWithoutL10NArgs(
                        "JobsDetails.restore.status.withouttodo",
                        new String [] {filesDone});
        } else {
            return SamUtil.getResourceStringWithoutL10NArgs(
                        "JobsDetails.restore.status",
                        new String[] { filesDone, filesToDo });
        }
    }

    public String getRestoreToPath() { return restoreToPath; }

    public int getOnlineStatusAfterRestore() {
        return onlineStatusAfterRestore;
    }
}
