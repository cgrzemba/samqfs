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

// ident	$Id: CurrentScanningData.java,v 1.12 2008/03/17 14:43:37 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJob;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJobData;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import java.util.ArrayList;

/**
 * Retrieves 'current' archive scanning data from the Model to populate the
 * Action Table on the 'Job Details' page.
 */
public final class CurrentScanningData extends ArrayList {

    // Column headings
    public static final String[] headings = new String [] {
        "JobsDetails.scanningData.heading1",
        "JobsDetails.scanningData.heading2",
        "JobsDetails.scanningData.heading3",
        "JobsDetails.scanningData.heading4",
        "JobsDetails.scanningData.heading5"
    };

    public CurrentScanningData(String serverName, long id)
        throws SamFSException {

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        BaseJob baseJob = sysModel.getSamQFSSystemJobManager().getJobById(id);
        if (baseJob == null) {
            throw new SamFSException(null, -2011);
        }
        ArchiveScanJob scanJob = (ArchiveScanJob) baseJob;

        for (int i = 0; i < 9; i++) {
            String rowHeader = null, numFile = "", consume = null,
            curNumFile = "", curConsume = null;
            ArchiveScanJobData data = null;
            switch (i) {
            case 0:
                rowHeader = "JobsDetails.scanningData.row1";
                data = scanJob.getRegularFiles();
                break;
            case 1:
                rowHeader = "JobsDetails.scanningData.row2";
                data = scanJob.getOfflieFiles();
                break;
            case 2:
                rowHeader = "JobsDetails.scanningData.row3";
                data = scanJob.getArchDoneFiles();
                break;
            case 3:
                rowHeader = "JobsDetails.scanningData.row4";
                data = scanJob.getCopy1();
                break;
            case 4:
                rowHeader = "JobsDetails.scanningData.row5";
                data = scanJob.getCopy2();
                break;
            case 5:
                rowHeader = "JobsDetails.scanningData.row6";
                data = scanJob.getCopy3();
                break;
            case 6:
                rowHeader = "JobsDetails.scanningData.row7";
                data = scanJob.getCopy4();
                break;
            case 7:
                rowHeader = "JobsDetails.scanningData.row8";
                data = scanJob.getDirectories();
                break;
            case 8:
                rowHeader = "JobsDetails.scanningData.row9";
                data = scanJob.getTotal();
                break;
            default:
                break;
            }

            if (data == null) {
                continue;
            }

            int file = data.getTotalNoOfFiles();
            if (file != -1) {
                numFile = Integer.toString(file);
            }
            consume = data.getTotalConsumedSpace();
            int completeFile = data.getNoOfCompletedFiles();
            if (completeFile != -1) {
                curNumFile = Integer.toString(completeFile);
            }
            curConsume = data.getCurrentConsumedSpace();

            super.add(
                new Object [] {
                    rowHeader,
                    numFile,
                    consume,
                    curNumFile,
                    curConsume
            });
        }
        TraceUtil.trace3("Exiting");
    }
}
