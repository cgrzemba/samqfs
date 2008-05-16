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

// ident	$Id: StageJobsData.java,v 1.15 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.model.job.StageJobFileData;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

/**
 * Used to populate the 'Stage Jobs' Action Table
 */
public final class StageJobsData implements LargeDataSet {

    private String serverName;
    private long stageJobID;

    // Column headings
    public static final String[] headings = new String [] {
        "JobsDetails.stageJob.header1",
        "JobsDetails.stageJob.header2",
        "JobsDetails.stageJob.header3",
        "JobsDetails.stageJob.header4",
        "JobsDetails.stageJob.header5"
    };

    public StageJobsData(String inServerName, long jobID) {
        TraceUtil.initTrace();
        serverName = inServerName;
        stageJobID = jobID;
    }

    public Object [] getData(int start, int num, String sortName,
		String sortOrder) throws SamFSException {

        TraceUtil.trace3("Entering start = " + start + " num = " + num +
            " sortName = " + sortName + " sortOrder = " + sortOrder);

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        BaseJob baseJob =
            sysModel.getSamQFSSystemJobManager().getJobById(stageJobID);
        if (baseJob == null) {
            throw new SamFSException(null, -2011);
        }
        StageJob stageJob = (StageJob) baseJob;

        short sortby = StageJob.SORT_BY_FILE_NAME;
        if (sortName != null)  {
            if ("StageText0".compareTo(sortName) == 0) {
                sortby = StageJob.SORT_BY_FILE_NAME;
            }
        }

        boolean ascending = true;
        if (sortOrder != null && "descending".compareTo(sortOrder) == 0) {
            ascending = false;
        }
        StageJobFileData[] jobData =
            stageJob.getFileData(start, num, sortby, ascending);
        TraceUtil.trace3("jobData.length is " + jobData.length);

        if (jobData == null) {
            return new StageJobFileData[0];
        }

        TraceUtil.trace3("Exiting");
        return (Object []) jobData;
    }

    public int getTotalRecords() throws SamFSException {

        TraceUtil.trace3("Entering");
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        BaseJob baseJob =
            sysModel.getSamQFSSystemJobManager().getJobById(stageJobID);
        if (baseJob == null) {
            throw new SamFSException(null, -2011);
        }
        StageJob stageJob = (StageJob) baseJob;

        int totalJobData = (int) stageJob.getNumberOfFiles();
        TraceUtil.trace3("NUMBER OF FILES IS " + totalJobData);

        return totalJobData;
    }
}
