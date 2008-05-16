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

// ident	$Id: TapeMountQueueModel.java,v 1.12 2008/05/16 18:39:04 am143972 Exp $

/**
 * This is the model class of the Tape Mount Queue frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.MountJob;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.util.GregorianCalendar;

public final class TapeMountQueueModel extends CCActionTableModel {

    // Constructor
    public TapeMountQueueModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/TapeMountQueueTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("TMQVSNColumn", "Monitor.title.vsn");
        setActionValue("TMQTypeColumn", "Monitor.title.type");
        setActionValue("TMQLibraryColumn", "Monitor.title.library");
        setActionValue("TMQWaitColumn", "Monitor.title.wait");
        setActionValue("TMQUserColumn", "Monitor.title.user");
        setActionValue("TMQStatusColumn", "Monitor.title.status");
        setActionValue("TMQActivityColumn", "Monitor.title.activity");
    }

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();

        BaseJob [] baseJobs =
            SamUtil.getModel(serverName).
                getSamQFSSystemJobManager().getJobsByType(BaseJob.TYPE_MOUNT);
        if (baseJobs == null) {
            return;
        }

        for (int i = 0; i < baseJobs.length; i++) {
            if (i > 0) {
                appendRow();
            }

            MountJob mountJob = (MountJob) baseJobs[i];
            GregorianCalendar calendar = mountJob.getTimeInQueue();
            long waitTime =
                calendar == null ?
                    0 : calendar.getTimeInMillis() / 1000;

            // Flag it if wait time is longer than 30 minutes
            boolean flagit = waitTime > 30 * 60;

            setValue(
                "TMQVSNText",
                flagit?
                    SamUtil.makeRed(mountJob.getVSNName()) :
                    mountJob.getVSNName());
            setValue(
                "TMQTypeText",
                flagit?
                    SamUtil.makeRed(
                        SamUtil.getMediaTypeString(mountJob.getMediaType())) :
                    SamUtil.getMediaTypeString(mountJob.getMediaType()));
            setValue(
                "TMQLibraryText",
                flagit?
                    SamUtil.makeRed(mountJob.getLibraryName()) :
                    mountJob.getLibraryName());

            setValue(
                "TMQWaitText",
                flagit?
                    SamUtil.makeRed(Long.toString(waitTime)) :
                    Long.toString(waitTime));

            setValue(
                "TMQUserText",
                flagit?
                    SamUtil.makeRed(mountJob.getInitiatingUsername()) :
                    mountJob.getInitiatingUsername());

            setValue(
                "TMQActivityText",
                getActivityString(mountJob.getStatusFlag()).
                        concat(" (").
                        concat(Long.toString(mountJob.getProcessId())).
                        concat(") "));


        }

        return;
    }

    /**
     * Construct Activity String for this table
     *
     * Definitions are from MountJob.java
     *
     * short LD_BUSY    = 0x000001; // entry is busy don't touch
     * short LD_IN_USE  = 0x000002; // entry in use (occupied)
     * short LD_P_ERROR  = 0x000004; // clear vsn requested
     * short LD_WRITE   = 0x000008; // write access request flag
     * short LD_FS_REQ  = 0x000010; // file system requested
     * short LD_BLOCK_IO = 0x000020; // block IO for opt/tp IO
     * short LD_STAGE   = 0x000040; // stage request flag
     */
    private String getActivityString(int statusFlag) {
        if ((statusFlag &
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_BUSY) ==
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_BUSY) {
            return SamUtil.getResourceString("Monitor.mountqueue.ld_busy");
        } else if ((statusFlag &
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_IN_USE) ==
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_IN_USE) {
            return SamUtil.getResourceString("Monitor.mountqueue.ld_in_use");
        } else if ((statusFlag &
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_P_ERROR) ==
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_P_ERROR) {
            return SamUtil.getResourceString("Monitor.mountqueue.ld_p_error");
        } else if ((statusFlag &
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_WRITE) ==
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_WRITE) {
            return SamUtil.getResourceString("Monitor.mountqueue.ld_write");
        } else if ((statusFlag &
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_FS_REQ) ==
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_FS_REQ) {
            return SamUtil.getResourceString("Monitor.mountqueue.ld_fs_req");
        } else if ((statusFlag &
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_BLOCK_IO) ==
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_BLOCK_IO) {
            return SamUtil.getResourceString("Monitor.mountqueue.ld_block_io");
        } else if ((statusFlag &
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_STAGE) ==
            com.sun.netstorage.samqfs.mgmt.media.MountJob.LD_STAGE) {
            return SamUtil.getResourceString("Monitor.mountqueue.ld_stage");
        } else {
            return "";
        }
    }
}
