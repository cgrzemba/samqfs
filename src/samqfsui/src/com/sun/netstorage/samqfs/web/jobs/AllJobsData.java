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

// ident	$Id: AllJobsData.java,v 1.22 2008/03/17 14:43:37 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.EnableDumpJob;
import com.sun.netstorage.samqfs.web.model.job.FSDumpJob;
import com.sun.netstorage.samqfs.web.model.job.GenericJob;
import com.sun.netstorage.samqfs.web.model.job.LabelJob;
import com.sun.netstorage.samqfs.web.model.job.MountJob;
import com.sun.netstorage.samqfs.web.model.job.ReleaseJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreSearchJob;
import com.sun.netstorage.samqfs.web.model.job.SamfsckJob;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.util.FormattedDate;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;


/**
 * Used to populate the 'All Jobs' action table.
 */
public final class AllJobsData extends ArrayList {

    // Column headings
    public static final String[] headings = new String [] {
        "Jobs.heading1",
        "Jobs.heading2",
        "Jobs.heading3",
        "Jobs.heading4",
        "AllJobs.heading5"
    };

    // Buttons
    public static final String button = new String("Jobs.button1");

    // Filter menu options
    public static final String[] filterOptions = new String [] {
        "Jobs.filterOptions0",
        "Jobs.filterOptions1",
        "Jobs.filterOptions2",
        "Jobs.filterOptions3",
        "Jobs.filterOptions4"
    };

    public AllJobsData(String serverName) throws SamFSException {

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        BaseJob [] baseJobs = null;
        BaseJob [] currentJobs =
            sysModel.getSamQFSSystemJobManager().getJobsByCondition(
                BaseJob.CONDITION_CURRENT);
        BaseJob [] pendingJobs =
            sysModel.getSamQFSSystemJobManager().getJobsByCondition(
                BaseJob.CONDITION_PENDING);

        int currentJobLength = 0;
        int pendingJobLength = 0;

        if (currentJobs != null) {
            currentJobLength = currentJobs.length;
        }

        if (pendingJobs != null) {
            pendingJobLength = pendingJobs.length;
        }

        baseJobs = new BaseJob[currentJobLength + pendingJobLength];
        int i;

        for (i = 0; i < currentJobLength; i++) {
            baseJobs[i] = currentJobs[i];
        }

        for (int j = 0; j < pendingJobLength; j++) {
            baseJobs[i++] = pendingJobs[j];
        }

        TraceUtil.trace3("baseJobs.length is " + baseJobs.length);
        for (int j = 0; j < baseJobs.length; j++) {

            // get the Job Type
            // NOTE: May not need to include all of
            // the Job Types
            // here for Current and All
            String jobTypeString = "";

            // make the description as well
            String description = new String();
            String comma = ",";
            int jobType = baseJobs[j].getType();
            if (jobType == BaseJob.TYPE_ARCHIVE_COPY) {
                jobTypeString = "Jobs.jobType1";
                ArchiveJob archiveJob = (ArchiveJob) baseJobs[j];
                String fileSystem = archiveJob.getFileSystemName();
                String policyName = archiveJob.getPolicyName();
                String copyNumber =
                    Integer.toString(archiveJob.getCopyNumber());
                description = SamUtil.processJobDescription(
                    new String [] {fileSystem, policyName, copyNumber});
            } else if (jobType == BaseJob.TYPE_ARCHIVE_SCAN) {
                jobTypeString = "Jobs.jobType5";
                ArchiveScanJob archiveScanJob = (ArchiveScanJob) baseJobs[j];
                String fileSystem = archiveScanJob.getFileSystemName();
                description = fileSystem;
                description = SamUtil.processJobDescription(
                    new String [] {fileSystem});
            } else if (jobType == BaseJob.TYPE_STAGE) {
                jobTypeString = "Jobs.jobType2";
                StageJob stageJob = (StageJob) baseJobs[j];
                String vsn = stageJob.getVSNName();
                description = SamUtil.processJobDescription(
                    new String [] { vsn});
            } else if (jobType == BaseJob.TYPE_RELEASE) {
                jobTypeString = "Jobs.jobType3";
                ReleaseJob releaseJob = (ReleaseJob) baseJobs[j];
                String fileSystem = releaseJob.getFileSystemName();
                String lwm = releaseJob.getLWM();
                String spaceConsumed = releaseJob.getConsumedSpacePercentage();
                description = SamUtil.processJobDescription(
                    new String [] { fileSystem, lwm, spaceConsumed});
            } else if (jobType == BaseJob.TYPE_MOUNT) {
                jobTypeString = "Jobs.jobType4";
                MountJob mountJob = (MountJob) baseJobs[j];
                String vsn = mountJob.getVSNName();
                String mediaType =
                    SamUtil.getMediaTypeString(mountJob.getMediaType());
                description = SamUtil.processJobDescription(
                    new String [] { vsn, mediaType});
            } else if (jobType == BaseJob.TYPE_FSCK) {
                jobTypeString = "Jobs.jobType6";
                SamfsckJob fsckJob = (SamfsckJob) baseJobs[j];
                String fsName = fsckJob.getFileSystemName();
                String initUser = fsckJob.getInitiatingUser();
                String repair = "";
                if (fsckJob.isRepair()) {
                    repair = "Jobs.repair";
                } else {
                    repair = "Jobs.non-repair";
                }
                description = SamUtil.processJobDescription(
                    new String [] { fsName, initUser,
                        SamUtil.getResourceString(repair)});
            } else if (jobType == BaseJob.TYPE_TPLABEL) {
                jobTypeString = "Jobs.jobType7";
                LabelJob labelJob = (LabelJob) baseJobs[j];
                String lib = labelJob.getLibraryName();
                String vsn = labelJob.getVSNName();
                description = SamUtil.processJobDescription(
                    new String [] { lib, vsn });
            } else if (jobType == BaseJob.TYPE_RESTORE) {
                jobTypeString = BaseJob.TYPE_RESTORE_STR;
                RestoreJob restoreJob = (RestoreJob) baseJobs[j];
                description = SamUtil.processJobDescription(
                    new String [] { restoreJob.getFileSystemName(),
                                    restoreJob.getFileName()});
            } else if (jobType == BaseJob.TYPE_RESTORE_SEARCH) {
                jobTypeString = BaseJob.TYPE_RESTORE_SEARCH_STR;
                RestoreSearchJob searchJob = (RestoreSearchJob) baseJobs[j];
                description = SamUtil.processJobDescription(
                    new String [] { searchJob.getFileSystemName(),
                                    searchJob.getDumpFileName()});
            } else if (jobType == BaseJob.TYPE_DUMP) {
                jobTypeString = "Jobs.jobType.dump";
                FSDumpJob dumpJob = (FSDumpJob) baseJobs[j];
                description = dumpJob.getFileSystemName();
            } else if (jobType == BaseJob.TYPE_ENABLE_DUMP) {
                jobTypeString = BaseJob.TYPE_ENABLE_DUMP_STR;
                EnableDumpJob enableDumpJob = (EnableDumpJob) baseJobs[j];
                description = SamUtil.processJobDescription(
                    new String [] { enableDumpJob.getFileSystemName(),
                                    enableDumpJob.getDumpFileName()});
            } else if (jobType == BaseJob.TYPE_ARCHIVE_FILES) {
                jobTypeString = BaseJob.TYPE_ARCHIVE_FILES_STR;
                GenericJob genericJob = (GenericJob) baseJobs[j];
                description = SamUtil.processJobDescription(
                    new String [] { Integer.toString(genericJob.getPID())});
            } else if (jobType == BaseJob.TYPE_RELEASE_FILES) {
                jobTypeString = BaseJob.TYPE_RELEASE_FILES_STR;
                GenericJob genericJob = (GenericJob) baseJobs[j];
                description = SamUtil.processJobDescription(
                    new String [] { Integer.toString(genericJob.getPID())});
            } else if (jobType == BaseJob.TYPE_STAGE_FILES) {
                jobTypeString = BaseJob.TYPE_STAGE_FILES_STR;
                GenericJob genericJob = (GenericJob) baseJobs[j];
                description = SamUtil.processJobDescription(
                    new String [] { Integer.toString(genericJob.getPID())});
            } else if (jobType == BaseJob.TYPE_RUN_EXPLORER) {
                jobTypeString = BaseJob.TYPE_RUN_EXPLORER_STR;
                GenericJob genericJob = (GenericJob) baseJobs[j];
                description = SamUtil.processJobDescription(
                    new String [] { Integer.toString(genericJob.getPID())});
            }


            TraceUtil.trace3("Job id is " +
                Long.toString(baseJobs[j].getJobId()));

            int condition = baseJobs[j].getCondition();
            String conditionString = "";
            if (condition == BaseJob.CONDITION_CURRENT) {
                conditionString = "Jobs.conditionCurrent";
            } else if (condition == BaseJob.CONDITION_PENDING) {
                conditionString = "Jobs.conditionPending";
            }

            super.add(
                new Object [] {
                    new Long(baseJobs[j].getJobId()),
                    jobTypeString,
                    new FormattedDate(baseJobs[j].getStartDateTime(),
                            SamUtil.getTimeFormat()),
                    description,
                    conditionString
                });
        }
        TraceUtil.trace3("Exiting");
    }
}
