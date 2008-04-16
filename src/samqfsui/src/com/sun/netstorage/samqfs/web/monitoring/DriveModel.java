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

// ident	$Id: DriveModel.java,v 1.15 2008/04/16 16:36:59 ronaldso Exp $

/**
 * This is the model class of the library drive frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.media.DriveDev;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TimeConvertor;
import com.sun.web.ui.model.CCActionTableModel;
import java.text.DateFormat;
import java.util.Date;

public final class DriveModel extends CCActionTableModel {

    // Constructor
    public DriveModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/DriveTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("DriveLogColumn", "Monitor.title.log");
        setActionValue("DriveLibraryColumn", "Monitor.title.library");
        setActionValue("DriveVSNColumn", "Monitor.title.vsn");
        setActionValue("DriveTypeColumn", "Monitor.title.type");
        setActionValue("DriveIdleTimeColumn", "Monitor.title.idlefor");
        setActionValue("DriveStateColumn", "Monitor.title.state");
        setActionValue("DriveStatusColumn", "Monitor.title.status");
        setActionValue("DriveActivityColumn", "Monitor.title.activity");
    }

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();

        Library [] allLibraries =
            SamUtil.getModel(serverName).
                getSamQFSSystemMediaManager().getAllLibraries();

        if (allLibraries == null) {
            return;
        }

        int count = 0;
        for (int i = 0; i < allLibraries.length; i++) {
            Drive [] allDrives = allLibraries[i].getDrives();
            if (allDrives == null) {
                continue;
            }

            for (int j = 0; j < allDrives.length; j++) {
                if (count > 0) {
                    appendRow();
                }

                DateFormat dfmt = DateFormat.getDateTimeInstance(
                                          DateFormat.SHORT,
                                          DateFormat.SHORT);
                long logTime = allDrives[j].getLogModTime();
                setValue("DriveLogText",
                    logTime <= 0 ?
                        "" : dfmt.format(new Date(logTime * 1000)));
                setValue("LogPath", allDrives[j].getLogPath());

                setValue("DriveLibraryText", allDrives[j].getLibraryName());
                setValue("DriveVSNText", allDrives[j].getVSNName());
                setValue("DriveTypeText",
                    SamUtil.getMediaTypeString(allDrives[j].getEquipType()));

                long idleTime = allDrives[j].getLoadIdleTime();
                setValue(
                    "DriveIdleTimeText",
                    idleTime <= 0 ?
                        "" :
                        TimeConvertor.newTimeConvertor(
                            idleTime, TimeConvertor.UNIT_SEC).toString());
                setValue(
                    "DriveStateText",
                    SamUtil.getStateString(allDrives[j].getState()));

                long tapeAlertFlag = allDrives[j].getTapeAlertFlags();

                // Messages
                setValue(
                    "DriveActivityText",
                    getActivityString(allDrives[j].getMessages()));

                // Status Information
                setValue(
                    "DriveStatusText",
                    getStatusInfoString(
                        allDrives[j].getDetailedStatus(), tapeAlertFlag));

                count++;
            }
        }

        return;
    }

    private String getActivityString(String [] messages) {
        if (messages == null || messages.length == 0) {
            return "";
        } else {
            StringBuffer strBuf = new StringBuffer();
            for (int k = 0; k < messages.length; k++) {
                if (k > 0) {
                    strBuf.append("<br>");
                }
                strBuf.append(messages[k]);
            }
            return strBuf.toString();
        }
    }

    /**
     * from tapealert.c
     * public static final int CLEAN_NOW = 0x80000;
     * public static final int STK_CLEAN_REQUESTED = 0x800000;
     */
    private String getStatusInfoString(
        int [] statusInfoCodes, long tapeAlertFlag) {

        StringBuffer strBuf = new StringBuffer();

        // Show drive clean status if applicable
        if ((tapeAlertFlag & DriveDev.CLEAN_NOW) == DriveDev.CLEAN_NOW) {
           strBuf.append(
               SamUtil.getResourceString("Monitor.drive.cleannow"));
        } else if ((tapeAlertFlag & DriveDev.CLEAN_NOW) == DriveDev.CLEAN_NOW) {
           strBuf.append(
               SamUtil.getResourceString("Monitor.drive.cleanrequested"));
        }

        if (statusInfoCodes == null || statusInfoCodes.length == 0) {
            return strBuf.toString();
        } else {
            for (int k = 0; k < statusInfoCodes.length; k++) {
                if (strBuf.length() > 0) {
                    strBuf.append("<br>");
                }

                if (statusInfoCodes[k] == SamQFSSystemMediaManager.
                    RM_STATUS_AUTO_LIB_ALL_SLOTS_OCCUPIED) {
                    // use 10017b (Drive)
                    strBuf.append(SamUtil.getResourceString(
                        new StringBuffer(
                        Integer.toString(statusInfoCodes[k])).
                        append("b").toString()));
                } else {
                    strBuf.append(SamUtil.getResourceString(
                        Integer.toString(statusInfoCodes[k])));
                }
            }
            return strBuf.toString();
        }
    }
}
