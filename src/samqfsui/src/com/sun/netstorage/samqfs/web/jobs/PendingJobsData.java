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

// ident	$Id: PendingJobsData.java,v 1.15 2008/03/17 14:43:38 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.MountJob;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.util.FormattedDate;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;

/**
 * Used to populate the 'Pending Jobs' Action Table
 */
public final class PendingJobsData extends ArrayList {

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
        "Jobs.filterOptions4"
    };

    // Buttons
    public static final String button = "Jobs.button1";

    public PendingJobsData(String serverName) throws SamFSException {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        BaseJob [] baseJobs =
            sysModel.getSamQFSSystemJobManager().getJobsByCondition(
                BaseJob.CONDITION_PENDING);

        if (baseJobs == null) {
            return;
        }

        for (int i = 0; i < baseJobs.length; i++) {
            String description = "";
            String comma = ",";
            String jobTypeString = null;
            int jobType = baseJobs[i].getType();

            if (jobType == BaseJob.TYPE_ARCHIVE_COPY) {
                jobTypeString = "Jobs.jobType1";
                ArchiveJob archiveJob = (ArchiveJob) baseJobs[i];
                String fileSystem = archiveJob.getFileSystemName();
                String policyName = archiveJob.getPolicyName();
                String copyNumber =
                    Integer.toString(archiveJob.getCopyNumber());
                description = SamUtil.processJobDescription(
                    new String [] {fileSystem, policyName, copyNumber});
            } else if (jobType == BaseJob.TYPE_ARCHIVE_SCAN) {
                jobTypeString = "Jobs.jobType5";
                ArchiveScanJob archiveScanJob = (ArchiveScanJob) baseJobs[i];
                String fileSystem = archiveScanJob.getFileSystemName();
                description = SamUtil.processJobDescription(
                    new String [] { fileSystem });
            } else if (jobType == BaseJob.TYPE_STAGE) {
                jobTypeString = "Jobs.jobType2";
                StageJob stageJob = (StageJob) baseJobs[i];
                String vsn = stageJob.getVSNName();
                description = SamUtil.processJobDescription(
                    new String [] { vsn });
            } else if (jobType == BaseJob.TYPE_MOUNT) {
                jobTypeString = "Jobs.jobType4";
                MountJob mountJob = (MountJob) baseJobs[i];
                String vsn = mountJob.getVSNName();
                String mediaType = SamUtil.getMediaTypeString(
                    mountJob.getMediaType());
                description = SamUtil.processJobDescription(
                    new String [] {vsn, mediaType});
            }

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
