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

// ident	$Id: ArCopyQueueModel.java,v 1.8 2008/04/16 16:36:59 ronaldso Exp $

/**
 * This is the model class of the archiving queue frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TimeConvertor;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.GregorianCalendar;

public final class ArCopyQueueModel extends CCActionTableModel {

    // Constructor
    public ArCopyQueueModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/ArCopyQueueTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("ArCopyQPolicyNameColumn", "Monitor.title.policyname");
        setActionValue("ArCopyQCopyNumberColumn", "Monitor.title.copynumber");
        setActionValue("ArCopyQFileSystemColumn", "Monitor.title.fsname");
        setActionValue("ArCopyQFilesColumn", "Monitor.title.count");
        setActionValue("ArCopyQSizeColumn", "Monitor.title.totalsize");
        setActionValue("ArCopyQVSNColumn", "Monitor.title.vsn");
        setActionValue("ArCopyQTypeColumn", "Monitor.title.type");
        setActionValue("ArCopyQWaitColumn", "Monitor.title.wait");
        setActionValue("ArCopyQActivityColumn", "Monitor.title.status");
    }

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();

        BaseJob [] baseJobs =
            SamUtil.getModel(serverName).getSamQFSSystemJobManager().
            getJobsByType(BaseJob.TYPE_ARCHIVE_COPY);
        if (baseJobs == null) {
            return;
        }

        for (int i = 0; i < baseJobs.length; i++) {
            if (i > 0) {
                appendRow();
            }

            ArchiveJob arcopyJob = (ArchiveJob) baseJobs[i];

            setValue("ArCopyQPolicyNameText", arcopyJob.getPolicyName());
            setValue(
                "ArCopyQCopyNumberText",
                new Integer(arcopyJob.getCopyNumber()));
            setValue("ArCopyQFileSystemText", arcopyJob.getFileSystemName());
            setValue(
                "ArCopyQFilesText",
                SamUtil.getResourceString("Monitor.utilization",
                new String [] {
                    Integer.toString(
                        arcopyJob.getTotalNoOfFilesAlreadyCopied()),
                    Integer.toString(
                        arcopyJob.getTotalNoOfFilesAlreadyCopied() +
                        arcopyJob.getTotalNoOfFilesToBeCopied())}));
            setValue(
                "ArCopyQSizeText",
                SamUtil.getResourceString("Monitor.utilization",
                new String [] {
                    arcopyJob.getDataVolumeAlreadyCopied() == 0 ?
                        "0" :
                        Capacity.newCapacity(
                            arcopyJob.getDataVolumeAlreadyCopied(),
                            SamQFSSystemModel.SIZE_KB).toString(),
                    arcopyJob.getDataVolumeToBeCopied() == 0 ?
                        "0" :
                        Capacity.newCapacity(
                            (arcopyJob.getDataVolumeAlreadyCopied() +
                            arcopyJob.getDataVolumeToBeCopied()),
                                     SamQFSSystemModel.SIZE_KB).toString()}));
            setValue("ArCopyQVSNText", arcopyJob.getVSNName());

            String mediaTypeString =
                SamUtil.getMediaTypeString(arcopyJob.getMediaType());
            setValue(
                "ArCopyQTypeText",
                mediaTypeString.equals(
                    SamUtil.getResourceString("media.type.unknown")) ?
                    "" :
                    mediaTypeString);

            long initial = arcopyJob.getStartDateTime().getTimeInMillis();
            long now = new GregorianCalendar().getTimeInMillis();
            long wait = (now - initial) / 1000;

            TraceUtil.trace3("Initial ArCopy Time: " + initial);
            TraceUtil.trace3("Current Time: " + now);
            TraceUtil.trace3("Wait Time (sec): " + wait);

            setValue("ArCopyQWaitText",
                     TimeConvertor.newTimeConvertor(
                        wait, TimeConvertor.UNIT_SEC).toString());
            setValue("ArCopyQActivityText", arcopyJob.getDescription());
        }

        return;
    }
}
