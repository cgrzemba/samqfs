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

// ident	$Id: CurrentAlarmSummaryModel.java,v 1.29 2008/12/16 00:10:53 am143972 Exp $

/**
 * This is the model class of current fault summary.
 */

package com.sun.netstorage.samqfs.web.alarms;

import com.iplanet.jato.RequestManager;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCSeverity;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import com.sun.web.ui.view.alarm.CCAlarmObject;

import java.util.ArrayList;
import java.util.Calendar;

/**
 * This is the Model Class of Fault Summary page
 */

public final class CurrentAlarmSummaryModel extends CCActionTableModel {

    // variables for filter.
    private String selectedFault = null;
    private ArrayList latestRows;

    // Constructor
    public CurrentAlarmSummaryModel() {
        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/alarms/CurrentAlarmSummaryTable.xml");

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        initActionButtons();
        initHeaders();
        initProductName();
        initFilterMenu();
        initSortingCriteria();

        TraceUtil.trace3("Exiting");
    }

    // Preset sorting to Severity column
    private void initSortingCriteria() {
        TraceUtil.trace3("Entering");

        // Set primary sort name to TimeHidden (Latest Fault Time)
        // Set secondary sort name to Alarm (Most Severe Fault)
        setPrimarySortName("TimeHidden");
        setPrimarySortOrder(CCActionTableModelInterface.DESCENDING);
        setSecondarySortName("Alarm");
        setSecondarySortOrder(CCActionTableModelInterface.DESCENDING);
    }

    // Initialize action buttons
    private void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue("AcknowledgeButton", "alarm.button.ack");
        setActionValue("DeleteButton", "alarm.button.delete");
        TraceUtil.trace3("Exiting");
    }

    // Initialize the action table headers
    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("IDColumn", "alarm.heading.id");
        setActionValue("SeverityColumn", "alarm.heading.severity");
        setActionValue("TimeColumn", "alarm.heading.time");
        setActionValue("DeviceColumn", "alarm.heading.device");
        setActionValue("StateColumn", "alarm.heading.state");
        setActionValue("DescriptionColumn", "alarm.heading.desc");
        TraceUtil.trace3("Exiting");
    }

    // Initialize product name for secondary page
    private void initProductName() {
        TraceUtil.trace3("Entering");
        setProductNameAlt("secondaryMasthead.productNameAlt");
        setProductNameSrc("secondaryMasthead.productNameSrc");
        setProductNameHeight(Constants.ProductNameDim.HEIGHT);
        setProductNameWidth(Constants.ProductNameDim.WIDTH);
        TraceUtil.trace3("Exiting");
    }

    // Initialize filter menu
    private void initFilterMenu() {
        TraceUtil.trace3("Entering");
        setActionValue("FilterMenu", "alarm.filterOptions.header");
        TraceUtil.trace3("Exiting");
    }

    public void initModelRows(String serverName) throws SamFSException {
        TraceUtil.trace3("Entering");

        // first clear the model
        clear();

        // latestRows - keep indexes due to filtering
        latestRows = new ArrayList();

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        Alarm allAlarms[] =
            sysModel.getSamQFSSystemAlarmManager().getAllAlarms();

        if (allAlarms == null) {
            return;
        }

        String severity = null, description = null, state = null;
        String device = null, timeString = null;
        Calendar time = null;
        Library myLibrary = null;
        long alarmID = -1;

        for (int i = 0; i < allAlarms.length; i++) {
            switch (allAlarms[i].getSeverity()) {
                case Alarm.CRITICAL:
                    severity = Constants.Alarm.ALARM_CRITICAL;
                    break;

                case Alarm.MAJOR:
                    severity = Constants.Alarm.ALARM_MAJOR;
                    break;

                case Alarm.MINOR:
                    severity = Constants.Alarm.ALARM_MINOR;
                    break;

                default:
                    // should not come to this case
                    severity = "";
                    break;
            }

            // After retrieving the severity of the fault, check if this record
            // is needed based on the filter settings
            // Skip the rest of the code if the fault does not match the
            // filtering criteria
            if (!selectedFault.equals(severity) &&
                !selectedFault.equals(Constants.Alarm.ALARM_ALL)) {
                continue;
            }

            description = allAlarms[i].getDescription();
            time = allAlarms[i].getDateTimeGenerated();

            // Check to see which object does the alarm belongs to
            device = allAlarms[i].getAssociatedLibraryName();

            switch (allAlarms[i].getStatus()) {
                case Alarm.ACTIVE:
                    state = "alarm.status.active";
                    break;

                case Alarm.ACKNOWLEDGED:
                    state = "alarm.status.ack";
                    break;
            }

            alarmID = allAlarms[i].getAlarmID();

            // Done retrieving info, start populating the TableModel

            latestRows.add(new Integer(i));

            // append new row
            if (i > 0) {
                appendRow();
            }

            setValue("IDText", Long.toString(alarmID));

            // set the time formatted to the locale
            if (time != null) {
                setValue("TimeHidden", time); // time is GC
                // for display of the time string, use SamUtil.getTimeString()
                setValue("TimeText", SamUtil.getTimeString(time));
            } else {
                setValue("TimeText", "");
            }

            setValue("DeviceText", device);
            setValue("StateText", state);
            setValue("DescriptionText", description.replaceAll("\n", "<br>"));
            setValue("IDHidden", Long.toString(alarmID));
            setValue(CurrentAlarmSummaryTiledView.CHILD_DEV_HREF, device);

            if (severity.equals(Constants.Alarm.ALARM_CRITICAL)) {
                setValue("SeverityText", "alarm.filterOptions.critical");
                setValue("Alarm", new CCAlarmObject(CCSeverity.CRITICAL));
            } else if (severity.equals(Constants.Alarm.ALARM_MAJOR)) {
                setValue("SeverityText", "alarm.filterOptions.major");
                setValue("Alarm", new CCAlarmObject(CCSeverity.MAJOR));
            } else if (severity.equals(Constants.Alarm.ALARM_MINOR)) {
                setValue("SeverityText", "alarm.filterOptions.minor");
                setValue("Alarm", new CCAlarmObject(CCSeverity.MINOR));
            } else {
                // Should not come to this case
                setValue("SeverityText", "");
                setValue("Alarm", new CCAlarmObject(CCSeverity.OK));
            }
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Set filter to only show rows matching given alarm severity.
     *
     * @param severity The alarm severity.
     */
    public void setFilter(String selectedType) {
        TraceUtil.trace3("Entering");
        this.selectedFault = selectedType;
        TraceUtil.trace3("Exiting");

    }

    // Return an ArrayList to hold the latest table index
    public ArrayList getLatestIndex() {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Exiting");
        return latestRows;
    }
}
