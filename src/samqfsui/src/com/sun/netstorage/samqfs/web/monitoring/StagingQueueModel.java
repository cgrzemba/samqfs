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

// ident	$Id: StagingQueueModel.java,v 1.16 2008/12/16 00:12:23 am143972 Exp $

/**
 * This is the model class of the staging queue frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.stg.job.StagerJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.model.job.StageJobFileData;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TimeConvertor;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.GregorianCalendar;

public final class StagingQueueModel extends CCActionTableModel {

    // Constructor
    public StagingQueueModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/StagingQueueTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("StagingQVSNColumn", "Monitor.title.vsn");
        setActionValue("StagingQTypeColumn", "Monitor.title.type");
        setActionValue("StagingQWaitColumn", "Monitor.title.wait");
        setActionValue("StagingQCountColumn", "Monitor.title.count");
        setActionValue("StagingQStatusColumn", "Monitor.title.status");
    }

    // TODO: Flag entries with wait time > 30 minutes!!!

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();

        BaseJob [] baseJobs =
            SamUtil.getModel(serverName).
                getSamQFSSystemJobManager().getJobsByType(BaseJob.TYPE_STAGE);
        if (baseJobs == null) {
            return;
        }

        for (int i = 0; i < baseJobs.length; i++) {
            if (i > 0) {
                appendRow();
            }

            StageJob stageJob = (StageJob) baseJobs[i];

            setValue("StagingQVSNText", stageJob.getVSNName());
            setValue("StagingQTypeText",
                SamUtil.getMediaTypeString(stageJob.getMediaType()));

            long initial = stageJob.getStartDateTime().getTimeInMillis();
            long now = new GregorianCalendar().getTimeInMillis();
            int wait = (int) ((now - initial) / 1000);

            setValue("StagingQWaitText",
                     TimeConvertor.newTimeConvertor(
                        wait, TimeConvertor.UNIT_SEC).toString());

            setValue(
                "StagingQCountText",
                Long.toString(stageJob.getNumberOfFiles()));
            setValue(
                "StagingQIDText",
                Long.toString(stageJob.getJobId()));

            setValue(
                "StagingQStatusText",
                getStageStatusString(stageJob.getStateFlag()));
        }

        return;
    }

    private String getStageStatusString(int statusFlag) {
        switch (statusFlag) {
            case StagerJob.STAGER_STATE_COPY:
                return "Monitor.stagingqueue.copy";
            case StagerJob.STAGER_STATE_DONE:
                return "Monitor.stagingqueue.done";
            case StagerJob.STAGER_STATE_LOADING:
                return "Monitor.stagingqueue.loading";
            case StagerJob.STAGER_STATE_NORESOURCES:
                return "Monitor.stagingqueue.noresource";
            case StagerJob.STAGER_STATE_POSITIONING:
                return "Monitor.stagingqueue.positioning";
            case StagerJob.STAGER_STATE_WAIT:
                return "Monitor.stagingqueue.wait";
        }

        return "";
    }
}
