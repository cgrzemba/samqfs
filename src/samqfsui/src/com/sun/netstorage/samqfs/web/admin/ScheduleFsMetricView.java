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

// ident	$Id: ScheduleFsMetricView.java,v 1.11 2008/05/16 19:39:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.FileMetricSchedule;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.fs.RecoveryPointSchedule;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.web.ui.model.CCDateTimeModel;
import com.sun.web.ui.view.datetime.CCDateTimeWindow;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import java.text.NumberFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;

/**
 *  This class is the view bean for create media report
 */

public class ScheduleFsMetricView extends RequestHandlingViewBase {

    private static final String CHILD_PROPERTY_SHEET = "PropertySheet";
    private static final String CHILD_DATASOURCE_NOTE = "dataSourceNote";
    public static final String CHILD_SCHEDULE_LABEL = "scheduleLabel";
    public static final String CHILD_STARTDATE_POPUP = "startDate";
    public static final String INTERVAL_OPTIONS = "intervalOptions";
    public static final String START_HOUR = "startHour";
    public static final String START_MINUTE = "startMinute";

    // Page Title Attributes and Components.
    private CCPropertySheetModel psModel = null;

    private OptionList intervalOptions = new OptionList(
        new String[] {"common.repeatInterval.hourly",
                      "common.repeatInterval.daily",
                      "common.repeatInterval.weekly",
                      "common.repeatInterval.monthly"
                     },
        new String[] {"common.repeatInterval.hourly",
                      "common.repeatInterval.daily",
                      "common.repeatInterval.weekly",
                      "common.repeatInterval.monthly"
                    });


    public ScheduleFsMetricView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // for the fs metric options
        psModel = PropertySheetUtil.createModel(
            "/jsp/admin/ScheduleFsMetricPropertySheet.xml");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_SCHEDULE_LABEL, CCLabel.class);
        registerChild(CHILD_STARTDATE_POPUP, CCDateTimeWindow.class);
        PropertySheetUtil.registerChildren(this, psModel);
        TraceUtil.trace3("Exiting");
    }

    protected View createChild(String name) {
        TraceUtil.trace3(new StringBuffer().append("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_SCHEDULE_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_STARTDATE_POPUP)) {
            CCDateTimeModel model = (CCDateTimeModel) new CCDateTimeModel();

            // Set the start date time, to current date
            // model.setStartDateTime(new Date());
            model.setType(CCDateTimeModel.FOR_DATE_SELECTION);
            child = new CCDateTimeWindow(this, model, name);

        } else if (PropertySheetUtil.isChildSupported(psModel, name)) {
            return PropertySheetUtil.createChild(this, psModel, name);
        } else {
            throw new IllegalArgumentException(new StringBuffer().append(
                "Invalid child name [").append(name).append("]").toString());
        }

        TraceUtil.trace3("Exiting");
        return (View) child;

    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // retrive the assigned psModel to maintain state information
        // CCPropertySheet ps = (CCPropertySheet)getChild(CHILD_PROPERTY_SHEET);
        // CCPropertySheetModel psModel = (CCPropertySheetModel)ps.getModel();
        // psModel.clear();

        ScheduleFsMetricViewBean vb =
            (ScheduleFsMetricViewBean)getParentViewBean();
        // get the file system name from page session
        // parent has saved it in page session
        String fsName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);

        if (psModel == null) {
            TraceUtil.trace3("psModel is null");
            throw new ModelControlException();
        }

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(vb.getServerName());
            // get the snapshot schedule for fsName

            RecoveryPointSchedule snapshotSchedule =
                SamUtil.getRecoveryPointSchedule(sysModel, fsName);
            if (snapshotSchedule != null &&
                !snapshotSchedule.isDisabled()) {
                ((CCLabel)getChild(CHILD_DATASOURCE_NOTE)).setValue(
                    SamUtil.getResourceString(
                        "reports.msg.filedistribution.collectfromrecoverypnts",
                        SamUtil.getSchedulePeriodicString(
                            snapshotSchedule.getPeriodicity(),
                            snapshotSchedule.getPeriodicityUnit())));
            }
            // get schedules of type "Reports" and by fsName
            Schedule [] schedule =
                sysModel.getSamQFSSystemAdminManager().
                    getSpecificTasks(ScheduleTaskID.REPORT, fsName);

            CCDateTimeWindow startDate =
                (CCDateTimeWindow)getChild(CHILD_STARTDATE_POPUP);
            Calendar c = startDate.getCalendar();
            CCDateTimeModel sdModel = (CCDateTimeModel) startDate.getModel();

            // only one schedule is supported in addition to snapshot schedule
            if (schedule.length > 0 && schedule[0] != null) {
                // Set start time.
                c.setTime(schedule[0].getStartTime());

                // convert the period for appropriate display
                ((CCDropDownMenu)getChild(INTERVAL_OPTIONS)).
                    setValue(String.valueOf(
                        schedule[0].getPeriodicityUnit()));

            } else {
                ((CCDropDownMenu)getChild(INTERVAL_OPTIONS)).
                    setValue(String.valueOf(8)); // daily
            }
            sdModel.setStartDateTime(c.getTime());

            String dateStr = new SimpleDateFormat(
                startDate.getDateFormatPattern()).format(c.getTime());

            // set the date string on the calendar text field directly
            ((CCTextField)startDate.getChild("textField")).setValue(dateStr);

            NumberFormat nf = NumberFormat.getIntegerInstance();
            nf.setMinimumIntegerDigits(2);

            ((CCDropDownMenu)getChild(START_HOUR)).setValue(
                String.valueOf(nf.format(c.get(Calendar.HOUR_OF_DAY))));
            ((CCDropDownMenu)getChild(START_MINUTE)).setValue(
                String.valueOf(nf.format(c.get(Calendar.MINUTE))));

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay",
                "Failed to get the populate the new fs metric page",
                vb.getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "reports.fsmetric.msg.unabletodisplayoptions"),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                vb.getServerName());
        }
        TraceUtil.trace3("Exiting");
    }

    public void submitRequest() {
        TraceUtil.trace3("Entering");

        ScheduleFsMetricViewBean vb =
            (ScheduleFsMetricViewBean)getParentViewBean();

        // get the file system name from page session
        // parent has saved it in page session
        String fsName = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);
        // TBD: Check that the user input is valid
        GregorianCalendar calendar = new GregorianCalendar();
        // Get the CCDateTimeWindow view
        CCDateTimeWindow startDate =
            (CCDateTimeWindow) getChild(CHILD_STARTDATE_POPUP);
        // validateDataInput() before retrieving model values
        if (startDate.validateDataInput()) {
            calendar.setTime(startDate.getModel().getStartDateTime());
        } else { // TBD

        }
        int startHour = Integer.parseInt(
                            (String) getDisplayFieldValue(START_HOUR));
        calendar.set(Calendar.HOUR_OF_DAY, startHour);
        int startMinute = Integer.parseInt(
                            (String) getDisplayFieldValue(START_MINUTE));
        calendar.set(Calendar.MINUTE, startMinute);

        int repeatVal = Integer.parseInt(
            (String)((CCDropDownMenu)getChild(INTERVAL_OPTIONS)).getValue());

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(vb.getServerName());
            FileMetricSchedule schedule =
                new FileMetricSchedule(fsName, repeatVal, calendar.getTime());

            sysModel.getSamQFSSystemAdminManager().setTaskSchedule(schedule);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay",
                "Failed to get the populate the new fs metric page",
                vb.getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "reports.fsmetric.msg.unabletodisplayoptions"),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                vb.getServerName());
        }
        SamUtil.setInfoAlert(
            vb.getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString("reports.success.text.createSchedule"),
            vb.getServerName());
        TraceUtil.trace3("Exiting");
    }
}
