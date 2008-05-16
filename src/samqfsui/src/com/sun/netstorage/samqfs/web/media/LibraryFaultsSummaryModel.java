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

// ident	$Id: LibraryFaultsSummaryModel.java,v 1.21 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.util.NonSyncStringBuffer;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCSeverity;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import com.sun.web.ui.view.alarm.CCAlarmObject;

import java.util.ArrayList;
import java.util.Calendar;
import java.util.StringTokenizer;


public final class LibraryFaultsSummaryModel extends CCActionTableModel {

    // variables for filter.
    private String selectedFault = null;
    private ArrayList latestRows;

    // Constructor
    public LibraryFaultsSummaryModel() {

        // Construct a new instance using XML file.
        super(RequestManager.getRequestContext().getServletContext(),
            "/jsp/media/LibraryFaultsSummaryTable.xml");

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        initActionButtons();
        initFilterMenu();
        initHeaders();
        initProductName();
        initSortingCriteria();

        TraceUtil.trace3("Exiting");
    }

    // Preset sorting to Severity column
    private void initSortingCriteria() {
        TraceUtil.trace3("Entering");

        // Set primary sort name to Col1 (Most Severe Fault first)
        setPrimarySortName("Alarm");
        setPrimarySortOrder(CCActionTableModelInterface.DESCENDING);
    }

    // Initialize action buttons
    private void initActionButtons() {
        TraceUtil.trace3("Entering");
        setActionValue("AcknowledgeButton", "alarm.button.ack");
        setActionValue("DeleteButton", "alarm.button.delete");
        TraceUtil.trace3("Exiting");
    }

    // Initialize filter menu
    private void initFilterMenu() {
        TraceUtil.trace3("Entering");
        setActionValue("FilterMenu", "alarm.filterOptions.header");
        TraceUtil.trace3("Exiting");
    }

    // Initialize the action table headers
    private void initHeaders() {
        TraceUtil.trace3("Entering");
        setActionValue("IDColumn", "alarm.heading.id");
        setActionValue("SeverityColumn", "alarm.heading.severity");
        setActionValue("TimeColumn", "alarm.heading.time");
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

    public void initModelRows(String serverName, String libraryNameString)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        // first clear the model
        clear();
        latestRows = new ArrayList();

        SamUtil.doPrint(new NonSyncStringBuffer(
            "initModelRows: libraryNameString is ").append(libraryNameString).
            toString());

        String libraryName = null;
        // if String does not contain # sign
        // use the string LIB_NAME directly,
        // otherwise, parse it.  The string is assigned in Fault Summary Page
        // It is designed in this way so CommonViewBeanBase knows what to se
        // for the default TAB_NAME
        if (libraryNameString.indexOf("#") == -1) {
            libraryName = libraryNameString;
        } else {
            StringTokenizer tokens =
                new StringTokenizer(libraryNameString, "#");
            if (tokens.hasMoreTokens()) {
                libraryName = tokens.nextToken();
            }
        }

        SamUtil.doPrint(new NonSyncStringBuffer(
            "libraryName (after parsing) is ").append(libraryName).toString());
        if (libraryName == null) {
            throw new SamFSException(null, -2502);
        }

        CCAlarmObject alarm = null;
        long alarmID = -1;
        String severity = null, description = null, status = null;
        String severityText = null;
        Calendar time = null;

        Alarm allAlarms[] = SamUtil.getModel(serverName).
                                    getSamQFSSystemAlarmManager().
                                    getAllAlarmsByLibName(libraryName);
        if (allAlarms == null) {
            return;
        }

        for (int i = 0; i < allAlarms.length; i++) {
            switch (allAlarms[i].getSeverity()) {
                case Alarm.CRITICAL:
                    severity = Constants.Alarm.ALARM_CRITICAL;
                    alarm = new CCAlarmObject(CCSeverity.CRITICAL);
                    severityText = "alarm.filterOptions.critical";
                    break;

                case Alarm.MAJOR:
                    severity = Constants.Alarm.ALARM_MAJOR;
                    alarm = new CCAlarmObject(CCSeverity.MAJOR);
                    severityText = "alarm.filterOptions.major";
                    break;

                case Alarm.MINOR:
                    severity = Constants.Alarm.ALARM_MINOR;
                    alarm = new CCAlarmObject(CCSeverity.MINOR);
                    severityText = "alarm.filterOptions.minor";
                    break;

                default:
                    // should not come to this case
                    severity = "";
                    alarm = new CCAlarmObject(CCSeverity.OK);
                    severityText = "";
                    break;
            }

            // Now we know what the severity the Alarm Object is
            // Stop retrieving additional information if the Alarm Type
            // does not fit to the filtering criteria
            // Otherwise, continue the rest of the code
            if (!selectedFault.equals(severity) &&
                !selectedFault.equals(Constants.Alarm.ALARM_ALL)) {
                continue;
            }
            latestRows.add(new Integer(i));

            description = allAlarms[i].getDescription();
            time = allAlarms[i].getDateTimeGenerated();

            // Check to see which object does the alarm belongs to
            switch (allAlarms[i].getStatus()) {
                case Alarm.ACTIVE:
                    status = "alarm.status.active";
                    break;

                case Alarm.ACKNOWLEDGED:
                    status = "alarm.status.ack";
                    break;
            }

            alarmID = allAlarms[i].getAlarmID();

            // FINISH retrieving information, now start populating tableModel
            // append new row
            if (i > 0) {
                appendRow();
            }

            setValue("Alarm", alarm);
            setValue("IDText", new Long(alarmID));
            setValue("SeverityText", severityText);
            setValue("StateText", status);
            setValue("DescriptionText", description.replaceAll("\n", "<br>"));
            setValue("IDHidden", new Long(alarmID));

            if (time != null) {
                setValue("TimeText", time.getTime());
            } else {
                setValue("TimeText", "");
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
