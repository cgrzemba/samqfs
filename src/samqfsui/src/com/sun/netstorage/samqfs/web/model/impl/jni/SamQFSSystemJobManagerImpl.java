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

// ident	$Id: SamQFSSystemJobManagerImpl.java,v 1.16 2008/07/15 17:19:46 kilemba Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.sun.netstorage.samqfs.mgmt.Job;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.job.ArCopyJob;
import com.sun.netstorage.samqfs.mgmt.arc.job.ArFindJob;
import com.sun.netstorage.samqfs.mgmt.media.LabelJob;
import com.sun.netstorage.samqfs.mgmt.rel.ReleaserJob;
import com.sun.netstorage.samqfs.mgmt.stg.job.StagerJob;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemJobManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.MultiHostStatus;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.ArchiveJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.ArchiveScanJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.BaseJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.GenericJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.LabelJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.MountJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.ReleaseJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.RestoreJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.SamfsckJobImpl;
import com.sun.netstorage.samqfs.web.model.impl.jni.job.StageJobImpl;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import java.util.ArrayList;
import java.util.Properties;

public class SamQFSSystemJobManagerImpl implements SamQFSSystemJobManager {

    private SamQFSSystemModelImpl theModel;
    private ArrayList jobs = new ArrayList();

    public SamQFSSystemJobManagerImpl(SamQFSSystemModel model) {
        theModel = (SamQFSSystemModelImpl) model;
    }

    public BaseJob[] getAllJobs() throws SamFSException {
        jobs.clear();

        Job[] jniJobs = null;
        try {
            jniJobs = Job.getJobs(theModel.getJniContext(), Job.TYPE_ARFIND);
        } catch (Exception e) {
            theModel.processException(e);
        }

        if (jniJobs != null) {
            for (int i = 0; i < jniJobs.length; i++)
                jobs.add(new ArchiveScanJobImpl((ArFindJob) jniJobs[i]));
            jniJobs = null;
        }

        try {
            jniJobs = Job.getJobs(theModel.getJniContext(), Job.TYPE_ARCOPY);
        } catch (Exception e) {
            theModel.processException(e);
        }

        if (jniJobs != null) {
            for (int i = 0; i < jniJobs.length; i++)
                jobs.add(new ArchiveJobImpl((ArCopyJob) jniJobs[i]));
            jniJobs = null;
        }

        try {
            jniJobs = Job.getJobs(theModel.getJniContext(), Job.TYPE_STAGE);
        } catch (Exception e) {
            theModel.processException(e);
        }

        if (jniJobs != null) {
            for (int i = 0; i < jniJobs.length; i++)
                jobs.add(new StageJobImpl(theModel, (StagerJob) jniJobs[i]));
            jniJobs = null;
        }

        try {
            jniJobs = Job.getJobs(theModel.getJniContext(), Job.TYPE_RELEASE);
        } catch (Exception e) {
            theModel.processException(e);
        }

        if (jniJobs != null) {
            for (int i = 0; i < jniJobs.length; i++)
                jobs.add(new ReleaseJobImpl((ReleaserJob) jniJobs[i]));
            jniJobs = null;
        }

        try {
            jniJobs = Job.getJobs(theModel.getJniContext(), Job.TYPE_MOUNT);
        } catch (Exception e) {
            theModel.processException(e);
        }

        if (jniJobs != null) {
            for (int i = 0; i < jniJobs.length; i++)
                jobs.add(new MountJobImpl(theModel,
                (com.sun.netstorage.samqfs.mgmt.media.MountJob) jniJobs[i]));
            jniJobs = null;
        }

        try {
            jniJobs = Job.getJobs(theModel.getJniContext(), Job.TYPE_SAMFSCK);
        } catch (Exception e) {
            theModel.processException(e);
        }

        if (jniJobs != null) {
            for (int i = 0; i < jniJobs.length; i++)
                jobs.add(new SamfsckJobImpl((com.sun.netstorage.samqfs.mgmt.
                                             fs.SamfsckJob) jniJobs[i]));
            jniJobs = null;
        }

        try {
            jniJobs = Job.getJobs(theModel.getJniContext(), Job.TYPE_LABEL);
        } catch (Exception e) {
            theModel.processException(e);
        }

        if (jniJobs != null) {
            for (int i = 0; i < jniJobs.length; i++)
                jobs.add(new LabelJobImpl((LabelJob) jniJobs[i]));
            jniJobs = null;
        }

        // now get 4.4 new (restore-related) jobs
        try {
            String[] jobsStrs = Job.getAllActivities(theModel.getJniContext(),
                                                     100,
                                                     "type=SAM[AR]");
            if (jobsStrs != null) {
                for (int i = 0; i < jobsStrs.length; i++) {
                    if (isRestoreJob(jobsStrs[i])) {
                        jobs.add(new RestoreJobImpl(jobsStrs[i]));
                    } else {
                        jobs.add(new GenericJobImpl(jobsStrs[i]));
                    }
                }
            }
        } catch (Exception e) {
            theModel.processException(e);
        }

        return (BaseJob[]) jobs.toArray(new BaseJob[0]);
    }

    private boolean isRestoreJob(String details) {
        Properties myProp = ConversionUtil.strToProps(details);
        if (myProp != null) {
            String actID = myProp.getProperty(GenericJobImpl.KEY_TYPE);

            // Restore job has multiple definitions
            // SAMRCRONTAB:- updates the crontab file
            // SAMRDUMP:- created by takeDump() API
            // SAMRDECOMPRESS:-
            // SAMRSEARCH:-
            // SAMRRESTORE:-

            if (actID != null && actID.startsWith("SAMR")) {
                return true;
            }
        }

        return false;
    }

    public BaseJob[] getJobsByType(int type) throws SamFSException {

        ArrayList specificJobs = new ArrayList();
        String activityType = null;
        switch (type) {
            case BaseJob.TYPE_DUMP:
                activityType = "SAMRDUMP";
                break;
	    case BaseJob.TYPE_ENABLE_DUMP:
                activityType = "SAMRDECOMPRESS";
                break;
            case BaseJob.TYPE_RESTORE:
                activityType = "SAMRRESTORE";
                break;
            case BaseJob.TYPE_RESTORE_SEARCH:
                activityType = "SAMRSEARCH";
                break;
            default:
                // old type of job
                getAllJobs();

                if (jobs != null) {
                    for (int i = 0; i < jobs.size(); i++) {
                        if (((BaseJob) jobs.get(i)).getType() == type) {
                            specificJobs.add(jobs.get(i));
                        }
                    }
                }
        }
        if (activityType != null) { // activity-type job
            String[] jobsStrs = Job.getAllActivities(theModel.getJniContext(),
                100, "type=" + activityType);
            if (jobsStrs != null)
                for (int i = 0; i < jobsStrs.length; i++) {
                    if (isRestoreJob(jobsStrs[i])) {
                        specificJobs.add(new RestoreJobImpl(jobsStrs[i]));
                    } else {
                        specificJobs.add(new GenericJobImpl(jobsStrs[i]));
                    }
                }
        }
        return (BaseJob[]) specificJobs.toArray(new BaseJob[0]);
    }

    public BaseJob[] getJobsByCondition(int condition) throws SamFSException {

        getAllJobs();
        ArrayList specificJobs = new ArrayList();
        if (jobs != null) {
            for (int i = 0; i < jobs.size(); i++) {
                if (((BaseJob) jobs.get(i)).getCondition() == condition)
                    specificJobs.add(jobs.get(i));
            }
        }

        return (BaseJob[]) specificJobs.toArray(new BaseJob[0]);
    }

    public BaseJob getJobById(long id) throws SamFSException {

        getAllJobs();
        BaseJob job = null;
        if (jobs != null)
            for (int i = 0; i < jobs.size(); i++)
                if (((BaseJob) jobs.get(i)).getJobId() == id) {
                    job = (BaseJob) jobs.get(i);
                    break;
                }

        return job;
    }

    public void cancelJob(BaseJob job) throws SamFSException {

        if (job == null)
            throw new SamFSException("logic.invalidJob");

        Job jniJob = ((BaseJobImpl) job).getJniJob();
        if (jniJob == null) {
            if (job.getType() >= BaseJob.START_JOB_TYPES &&
                job.getType() <= BaseJob.END_4_3_JOB_TYPES)
                throw new SamFSException("logic.noBackEndJob");
            else { // should be a 4.4 "activity-type" job
                try {
                    RestoreJobImpl rJob = (RestoreJobImpl) job;
                    rJob.cancel(theModel.getJniContext());
                } catch (ClassCastException cce) {
                   throw new SamFSException("logic.noBackEndJob");
                }
            }
        } else // old job type
             jniJob.terminate(theModel.getJniContext());

        int index = jobs.indexOf(job);
        if (index != -1)
            jobs.remove(index);
    }

    public MultiHostStatus getMultiHostStatus(long jobId)
        throws  SamFSException {
        // retrieve the specific job whose status we are interested in
        String filter = "activity_id=" + jobId;

        String [] status = Job.getAllActivities(theModel.getJniContext(),
                                                5,
                                                filter);
        // there should be only one item in the list
        if (status != null && status.length > 0) {
            return new MultiHostStatus(status[0]);
        }

        // we shouldn't get here
        return null;
    }
}
