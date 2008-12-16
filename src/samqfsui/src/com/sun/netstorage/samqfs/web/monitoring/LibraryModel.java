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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: LibraryModel.java,v 1.15 2008/12/16 00:12:23 am143972 Exp $

/**
 * This is the model class of the library frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.text.DateFormat;
import java.util.Date;

public final class LibraryModel extends CCActionTableModel {

    // Constructor
    public LibraryModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/LibraryTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("LibraryNameColumn", "Monitor.title.name");
        setActionValue("LibraryTypeColumn", "Monitor.title.type");
        setActionValue("LibraryStateColumn", "Monitor.title.state");
        setActionValue("LibraryLogColumn", "Monitor.title.log");
        setActionValue("LibraryStatusColumn", "Monitor.title.status");
        setActionValue("LibraryActivityColumn", "Monitor.title.activity");
    }

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();

        Library allLibraries [] =
            SamUtil.getModel(serverName).
                getSamQFSSystemMediaManager().getAllLibraries();

        if (allLibraries == null) {
            return;
        }

        for (int i = 0; i < allLibraries.length; i++) {
            // Skip Historian
            if (allLibraries[i].getName().
                equalsIgnoreCase(Constants.MediaAttributes.HISTORIAN_NAME)) {
                continue;
            }

            if (i > 0) {
                appendRow();
            }

            setValue("LibraryNameText", allLibraries[i].getName());
            setValue(
                "LibraryTypeText",
                SamUtil.getMediaTypeString(allLibraries[i].getMediaType()));

            DateFormat dfmt = DateFormat.getDateTimeInstance(
                                      DateFormat.SHORT,
                                      DateFormat.SHORT);
            long logTime = allLibraries[i].getLogModTime();
            setValue("LibraryLogText",
                logTime <= 0 ?
                    "" : dfmt.format(new Date(logTime * 1000)));
            setValue("LogPath", allLibraries[i].getLogPath());

            int state = allLibraries[i].getState();
            setValue("LibraryStateText", SamUtil.getStateString(state));

            // Status
            int [] statusInfoCodes = allLibraries[i].getDetailedStatus();

            if (statusInfoCodes == null || statusInfoCodes.length == 0) {
                setValue("LibraryStatusText", "");
            } else {
                StringBuffer strBuf = new StringBuffer();
                for (int j = 0; j < statusInfoCodes.length; j++) {
                    if (j > 0) {
                        strBuf.append("<br>");
                    } else {
                        if (statusInfoCodes[i] == SamQFSSystemMediaManager.
                            RM_STATUS_AUTO_LIB_ALL_SLOTS_OCCUPIED) {
                            // use 10017a (Library)
                            strBuf.append(SamUtil.getResourceString(
                                new StringBuffer(
                                Integer.toString(statusInfoCodes[j])).
                                append("a").toString()));
                        } else {
                            strBuf.append(SamUtil.getResourceString(
                                Integer.toString(statusInfoCodes[j])));
                        }
                    }
                }
                setValue("LibraryStatusText", strBuf.toString());
            }

            // Activity
            String [] messages = allLibraries[i].getMessages();

            if (messages == null || messages.length == 0) {
                setValue("LibraryActivityText", "");
            } else {
                StringBuffer strBuf = new StringBuffer();
                for (int j = 0; j < messages.length; j++) {
                    if (j > 0) {
                        strBuf.append("<br>");
                    } else {
                        strBuf.append(messages[j]);
                    }
                }
                setValue("LibraryActivityText", strBuf.toString());
            }
        }

        return;

    }
}
