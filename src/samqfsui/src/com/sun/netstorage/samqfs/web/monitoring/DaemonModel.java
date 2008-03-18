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

// ident	$Id: DaemonModel.java,v 1.14 2008/03/17 14:43:52 am143972 Exp $

/**
 * This is the model class of the daemon frame
 */

package com.sun.netstorage.samqfs.web.monitoring;

import com.iplanet.jato.RequestManager;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.ConversionUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import java.text.DateFormat;
import java.util.Date;
import java.util.Properties;

public final class DaemonModel extends CCActionTableModel {

    // The keys should match the definition in monitor.h
    /**
     * activityid=pid (if absent, then it is not running)
     * details=daemon or process name
     * type=SAMDXXXX
     * description=long user friendly name
     * starttime=secs
     * parentid=ppid
     * modtime=secs
     * path=/var/opt/SUNWsamfs/devlog/21
     */
    private static final String NAME = "details";
    private static final String PID  = "activityid";
    private static final String DESC = "description";
    private static final String START_TIME = "starttime";
    private static final String MOD_TIME   = "modtime";
    private static final String LOG_FILE   = "path";


    // Constructor
    public DaemonModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/monitoring/DaemonTable.xml");

        initHeaders();
    }


    // Initialize the action table headers
    private void initHeaders() {
        setActionValue("DaemonNameColumn", "Monitor.title.name");
        setActionValue("DaemonLogColumn", "Monitor.title.log");
        setActionValue("DaemonDescColumn", "Monitor.title.desc");
        setActionValue("DaemonStartTimeColumn", "Monitor.title.starttime");
    }

    public void initModelRows(String serverName) throws SamFSException {
        // first clear the model
        clear();


        String [] details =
            SamUtil.getModel(serverName).
                getSamQFSSystemAdminManager().getProcessStatus();
        if (details == null) {
            return;
        }

        // activityid=2330,starttime=1158773932,details=sam-arfind samfs8,
        // type=SAMDARFIND,parentid=2326,description=File system archive monitor
        for (int i = 0; i < details.length; i++) {
            TraceUtil.trace3("Daemon: ".concat(details[i]));

            boolean flagRed = false;
            Properties props = ConversionUtil.strToProps(details[i]);

            if (i > 0) {
                appendRow();
            }

            // Determine if we need to flag this entry
            flagRed = props.getProperty(PID) == null;

            long logTime = -1;
            DateFormat dfmt = DateFormat.getDateTimeInstance(
                                      DateFormat.SHORT,
                                      DateFormat.SHORT);
            try {
                String logTimeStr = props.getProperty(MOD_TIME);
                logTime = logTimeStr == null ? -1 : Long.parseLong(logTimeStr);
            } catch (NumberFormatException numEx) {
                TraceUtil.trace1(
                    "Number Format Exception in log time!");
            }

            String name = props.getProperty(NAME, "");
            String desc = props.getProperty(DESC, "");
            String startTime = props.getProperty(START_TIME, "");
            String logPath   = props.getProperty(LOG_FILE, "");

            setValue(
                "DaemonNameText",
                flagRed ?
                    SamUtil.makeRed(name) : name);
            setValue(
                "DaemonDescText",
                flagRed ?
                    SamUtil.makeRed(desc) : desc);

            setValue(
                "DaemonLogText",
                logTime == -1 ?
                    "" : dfmt.format(new Date(logTime * 1000)));

            setValue("LogPath", logPath);

            if (startTime.length() != 0) {
                long startTimeLong = -1;
                try {
                    startTimeLong = Long.parseLong(startTime);
                } catch (NumberFormatException numEx) {
                    setValue("DaemonStartTimeText", "");
                }
                if (startTimeLong != -1) {
                    setValue(
                        "DaemonStartTimeText",
                        flagRed ?
                            SamUtil.makeRed(
                                SamUtil.getTimeString(startTimeLong)) :
                            SamUtil.getTimeString(startTimeLong));
                }
            } else {
                setValue("DaemonStartTimeText", "");
            }
        }

        return;
    }
}
