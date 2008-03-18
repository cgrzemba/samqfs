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

// ident	$Id: CurrentJobsData.java,v 1.19 2008/03/17 14:43:37 am143972 Exp $

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
 * Used to populate the 'Current Jobs' Action Table
 */
public final class CurrentJobsData extends ArrayList {

    // Column headings
    public static final String[] headings = new String [] {
        "Jobs.heading1",
        "Jobs.heading2",
        "Jobs.heading3",
        "Jobs.heading4"
    };

    // Filter menu options
    public static final String[] filterOptions = new String [] {
        "Jobs.filterOptions0",
        "Jobs.filterOptions1",
        "Jobs.filterOptions2",
        "Jobs.filterOptions3",
        "Jobs.filterOptions4"
    };

    // Buttons
    public static final String button = "Jobs.button1";

    public CurrentJobsData(String serverName) throws SamFSException {

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering CurrentJobsData");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        BaseJob [] baseJobs =
            sysModel.getSamQFSSystemJobManager().getJobsByCondition(
                BaseJob.CONDITION_CURRENT);

        if (baseJobs == null) {
            return;
        }

        for (int i = 0; i < baseJobs.length; i++) {
            String description = "";
            String jobTypeString = null;
            GenericJob genericJob = null;
            int jobType = baseJobs[i].getType();

            switch (jobType) {
                case BaseJob.TYPE_ARCHIVE_COPY:
                    jobTypeString = "Jobs.jobType1";
                    ArchiveJob archiveJob = (ArchiveJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            archiveJob.getFileSystemName(),
                            archiveJob.getPolicyName(),
                            Integer.toString(archiveJob.getCopyNumber())});
                    break;

                case BaseJob.TYPE_ARCHIVE_SCAN:
                    jobTypeString = "Jobs.jobType5";
                    ArchiveScanJob archiveScanJob =
                                (ArchiveScanJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            archiveScanJob.getFileSystemName()});
                    break;

                case BaseJob.TYPE_STAGE:
                    jobTypeString = "Jobs.jobType2";
                    StageJob stageJob = (StageJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            stageJob.getVSNName()});
                    break;

                case BaseJob.TYPE_RELEASE:
                    jobTypeString = "Jobs.jobType3";
                    ReleaseJob releaseJob = (ReleaseJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            releaseJob.getFileSystemName(),
                            releaseJob.getLWM(),
                            releaseJob.getConsumedSpacePercentage()});
                    break;

                case BaseJob.TYPE_MOUNT:
                    jobTypeString = "Jobs.jobType4";
                    MountJob mountJob = (MountJob) baseJobs[i];
                    String vsn = mountJob.getVSNName();
                    String mediaType = SamUtil.getMediaTypeString(
                        mountJob.getMediaType());
                    description = SamUtil.processJobDescription(
                        new String [] {
                            mountJob.getVSNName(),
                            SamUtil.getMediaTypeString(mountJob.getMediaType())
                        });
                    break;

                case BaseJob.TYPE_FSCK:
                    jobTypeString = "Jobs.jobType6";
                    SamfsckJob fsckJob = (SamfsckJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            fsckJob.getFileSystemName(),
                            fsckJob.getInitiatingUser(),
                            SamUtil.getResourceString(
                                fsckJob.isRepair() ?
                                    "Jobs.repair" : "Jobs.non-repair")});
                    break;

                case BaseJob.TYPE_TPLABEL:
                    jobTypeString = "Jobs.jobType7";
                    LabelJob labelJob = (LabelJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            labelJob.getLibraryName(),
                            labelJob.getVSNName()});
                    break;

                case BaseJob.TYPE_RESTORE:
                    jobTypeString = BaseJob.TYPE_RESTORE_STR;
                    RestoreJob restoreJob = (RestoreJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                                restoreJob.getFileSystemName(),
                                restoreJob.getFileName()});
                    break;

                case BaseJob.TYPE_ARCHIVE_FILES:
                    jobTypeString = BaseJob.TYPE_ARCHIVE_FILES_STR;
                    genericJob = (GenericJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            Integer.toString(genericJob.getPID())});
                    break;

                case BaseJob.TYPE_RELEASE_FILES:
                    jobTypeString = BaseJob.TYPE_RELEASE_FILES_STR;
                    genericJob = (GenericJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            Integer.toString(genericJob.getPID())});
                    break;

                case BaseJob.TYPE_STAGE_FILES:
                    jobTypeString = BaseJob.TYPE_STAGE_FILES_STR;
                    genericJob = (GenericJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            Integer.toString(genericJob.getPID())});
                    break;

                case BaseJob.TYPE_RUN_EXPLORER:
                    jobTypeString = BaseJob.TYPE_RUN_EXPLORER_STR;
                    genericJob = (GenericJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            Integer.toString(genericJob.getPID())});
                    break;

                case BaseJob.TYPE_RESTORE_SEARCH:
                    jobTypeString = BaseJob.TYPE_RESTORE_SEARCH_STR;
                    RestoreSearchJob searchJob = (RestoreSearchJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            searchJob.getFileSystemName(),
                            searchJob.getDumpFileName()});
                    break;

                case BaseJob.TYPE_DUMP:
                    jobTypeString = "Jobs.jobType.dump";
                    FSDumpJob dumpJob = (FSDumpJob) baseJobs[i];
                    description = dumpJob.getFileSystemName();
                    break;

                case BaseJob.TYPE_ENABLE_DUMP:
                    jobTypeString = BaseJob.TYPE_ENABLE_DUMP_STR;
                    EnableDumpJob dumpEnableJob = (EnableDumpJob) baseJobs[i];
                    description = SamUtil.processJobDescription(
                        new String [] {
                            dumpEnableJob.getFileSystemName(),
                            dumpEnableJob.getDumpFileName()});
                    break;

                default:
                    TraceUtil.trace1(
                        "Developer's bug: check SamQFSSystemJobManagerImpl!");
                    jobTypeString = "";
                    break;
            }


            TraceUtil.trace3(
                new StringBuffer("id: ").append(baseJobs[i].getJobId()).append(
                    ", jobTypeString: ").append(jobTypeString).append(
                    ", timeString: ").append(
                        SamUtil.getTimeString(baseJobs[i].getStartDateTime())).
                    append(", description: ").append(description).toString());

            super.add(
                new Object [] {
                    new Long(baseJobs[i].getJobId()),
                    jobTypeString,
                    new FormattedDate(baseJobs[i].getStartDateTime(),
                            SamUtil.getTimeFormat()),
                    description
            });
        }

        TraceUtil.trace3("Exiting");
    }
}
