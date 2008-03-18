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

// ident	$Id: StageJobsModel.java,v 1.11 2008/03/17 14:43:38 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.job.StageJobFileData;
import com.sun.netstorage.samqfs.web.util.LDSTableModel;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

/**
 * A CCActionTableModel for the staging job Action table.
 */
public final class StageJobsModel extends LDSTableModel {

    public StageJobsModel(LargeDataSet data) {

        super(RequestManager.getRequestContext().getServletContext(),
              "/jsp/jobs/StageJobsTable.xml");

        dataSet = data;
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        TraceUtil.initTrace();
        initHeaders();
        TraceUtil.trace3("Exiting");
    }

    private void initHeaders() {

        TraceUtil.trace3("Entering");
        for (int i = 0; i < StageJobsData.headings.length; i++) {
            setActionValue("StageCol" + i, StageJobsData.headings[i]);
        }
        TraceUtil.trace3("Exiting");
    }

    public void initModelRows() throws SamFSException {

        TraceUtil.trace3("Entering");

        super.getModelRows();

        for (int i = 0; i < currentDataSet.length; i++) {
            if (i > 0) {
                appendRow();
            }

            StageJobFileData stageJob = (StageJobFileData) currentDataSet[i];

            String fileName = stageJob.getFileName();
            String size = stageJob.getFileSizeInBytes();
            String position = stageJob.getPosition();
            String offset = stageJob.getOffset();
            String user = stageJob.getUser();

            setValue("StageText0", fileName);
            setValue("StageText1", size);
            setValue("StageText2", position);
            setValue("StageText3", offset);
            setValue("StageText4", user);
        }
        TraceUtil.trace3("Exiting");
    }
}
