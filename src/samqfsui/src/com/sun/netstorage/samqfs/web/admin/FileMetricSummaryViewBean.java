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

// ident	$Id: FileMetricSummaryViewBean.java,v 1.25 2008/12/16 00:10:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.Metric;
import com.sun.netstorage.samqfs.web.model.fs.RecoveryPointSchedule;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCDateTimeModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.datetime.CCDateTimeWindow;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import java.text.DateFormat;
import java.text.NumberFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import javax.servlet.ServletException;

public class FileMetricSummaryViewBean extends CommonViewBeanBase {
    private static String PAGE_NAME = "FileMetricSummary";
    private static String DEFAULT_URL = "/jsp/admin/FileMetricSummary.jsp";

    // children
    private static final String PAGE_TITLE = "PageTitle";
    private static final String CHILD_STATICTEXT = "StaticText";
    private static final String SERVER_NAME = "ServerName";
    private static final String CHILD_FSNAME_LABEL = "fsnameLabel";
    // Dropdown of file system names
    private static final String CHILD_FSNAME_OPTIONS = "fsnameOptions";
    private static final String CHILD_FSNAME_MENU_HREF = "fsnameHref";
    // Href and NoHref
    private static final String CHILD_STORAGETIER_HREF = "StorageTierHref";
    private static final String CHILD_FILEBYLIFE_HREF = "FileByLifeHref";
    private static final String CHILD_FILEBYAGE_HREF = "FileByAgeHref";
    private static final String CHILD_FILEBYOWNER_HREF = "FileByOwnerHref";
    private static final String CHILD_FILEBYGROUP_HREF = "FileByGroupHref";
    private static final String CHILD_STORAGETIER_NOHREF = "StorageTierNoHref";
    private static final String CHILD_FILEBYLIFE_NOHREF = "FileByLifeNoHref";
    private static final String CHILD_FILEBYAGE_NOHREF = "FileByAgeNoHref";
    private static final String CHILD_FILEBYOWNER_NOHREF = "FileByOwnerNoHref";
    private static final String CHILD_FILEBYGROUP_NOHREF = "FileByGroupNoHref";
    // button
    private static final String CHILD_SCHEDULE_BUTTON = "ScheduleMetricButton";
    private static final String CHILD_SCHEDULE_TEXT = "ScheduleText";
    // time range
    private static final String CHILD_STARTDATE_LABEL = "startDateLabel";
    private static final String CHILD_ENDDATE_LABEL = "endDateLabel";
    private static final String CHILD_STARTDATE_POPUP = "startDate";
    private static final String CHILD_ENDDATE_POPUP = "endDate";
    private static final String CHILD_TIMERANGE_BUTTON = "TimeRangeButton";


    public FileMetricSummaryViewBean() {
        super(PAGE_NAME, DEFAULT_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public String getJpgFileName() { return jpgFileName; } // used by JSP
    public String getMapString() { return imageMapString; } // used by JSP

    public void registerChildren() {
        super.registerChildren();
        TraceUtil.trace3("Entering");
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_SCHEDULE_TEXT, CCStaticTextField.class);
        registerChild(CHILD_SCHEDULE_BUTTON, CCButton.class);
        registerChild(CHILD_FSNAME_LABEL, CCLabel.class);
        registerChild(CHILD_FSNAME_OPTIONS, CCDropDownMenu.class);
        registerChild(CHILD_FSNAME_MENU_HREF, CCHref.class);
        registerChild(CHILD_STORAGETIER_HREF, CCHref.class);
        registerChild(CHILD_STORAGETIER_NOHREF, CCStaticTextField.class);
        registerChild(CHILD_FILEBYLIFE_HREF, CCHref.class);
        registerChild(CHILD_FILEBYLIFE_NOHREF, CCStaticTextField.class);
        registerChild(CHILD_FILEBYAGE_HREF, CCHref.class);
        registerChild(CHILD_FILEBYAGE_NOHREF, CCStaticTextField.class);
        registerChild(CHILD_FILEBYOWNER_HREF, CCHref.class);
        registerChild(CHILD_FILEBYOWNER_NOHREF, CCStaticTextField.class);
        registerChild(CHILD_FILEBYGROUP_HREF, CCHref.class);
        registerChild(CHILD_FILEBYGROUP_NOHREF, CCStaticTextField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(CHILD_STARTDATE_LABEL, CCLabel.class);
        registerChild(CHILD_STARTDATE_POPUP, CCDateTimeWindow.class);
        registerChild(CHILD_ENDDATE_LABEL, CCLabel.class);
        registerChild(CHILD_ENDDATE_POPUP, CCDateTimeWindow.class);
        registerChild(CHILD_TIMERANGE_BUTTON, CCButton.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "Entering: name is ").append(name).toString());
        if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else if (name.equals(CHILD_STATICTEXT) ||
            name.equals(CHILD_SCHEDULE_TEXT) ||
            name.equals(CHILD_STORAGETIER_NOHREF) ||
            name.equals(CHILD_FILEBYLIFE_NOHREF) ||
            name.equals(CHILD_FILEBYAGE_NOHREF) ||
            name.equals(CHILD_FILEBYOWNER_NOHREF) ||
            name.equals(CHILD_FILEBYGROUP_NOHREF)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_FSNAME_LABEL) ||
            name.equals(CHILD_STARTDATE_LABEL) ||
            name.equals(CHILD_ENDDATE_LABEL)) {
            return new CCLabel(this, name, null);

        } else if (name.equals(CHILD_FSNAME_OPTIONS)) {
            return new CCDropDownMenu(this, name, null);

        } else if (name.equals(CHILD_FSNAME_MENU_HREF) ||
            name.equals(CHILD_STORAGETIER_HREF) ||
            name.equals(CHILD_FILEBYLIFE_HREF) ||
            name.equals(CHILD_FILEBYAGE_HREF) ||
            name.equals(CHILD_FILEBYOWNER_HREF) ||
            name.equals(CHILD_FILEBYGROUP_HREF)) {
            return new CCHref(this, name, null);

        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, getServerName());
        } else if (name.equals(CHILD_SCHEDULE_BUTTON) ||
            name.equals(CHILD_TIMERANGE_BUTTON)) {
            return new CCButton(this, name, null);
        } else if (name.equals(CHILD_STARTDATE_POPUP) ||
            name.equals(CHILD_ENDDATE_POPUP)) {
            CCDateTimeModel model = (CCDateTimeModel) new CCDateTimeModel();
            model.setType(CCDateTimeModel.FOR_DATE_SELECTION);
            return new CCDateTimeWindow(this, model, name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }

    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");


        String root = RequestManager.getRequestContext().
            getServletContext().getRealPath("/");

        // fs dropdown
        String selFsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME);

        // metric type
        String metricType = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.METRIC_TYPE);
        metricType = (metricType != null) ? metricType : "storagetier";

        // time ranges
        // Get the time ranges from page session (if refresh on user selection)
        // Else start and end date is chosen based on recovery point schedule
        // By default, if schedule not available, the range is 4 weeks
        Date endDate = (Date) getPageSessionAttribute(
            Constants.PageSessionAttributes.END_DATE);
        Date startDate = (Date) getPageSessionAttribute(
            Constants.PageSessionAttributes.START_DATE);

        try {

            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            OptionList fsnameOptions = new OptionList();
            // first entry is always "Sample Metrics"
            fsnameOptions.add(
                SamUtil.getResourceString("reports.sample"), SAMPLE_METRICS);
            FileSystem [] fss =
                sysModel.getSamQFSSystemFSManager().getAllFileSystems();
            // metrics are available for all filesystems
            for (int i = 0; i < fss.length; i++) {
                // prune out MAT file systems
                if (!fss[i].isMatFS()) {
                    fsnameOptions.add(fss[i].getName(), fss[i].getName());
                }
            }
            if (selFsName ==  null) {
                // page displayed for the first time, try from the session
                selFsName = SamUtil.getLastUsedFSName(getServerName());
                if (selFsName != null) {
                    setPageSessionAttribute(
                        Constants.PageSessionAttributes.FS_NAME, selFsName);
                }
            }
            TraceUtil.trace3("Selected file system " +
                selFsName != null ? selFsName : "null");

            ((CCDropDownMenu) getChild(CHILD_FSNAME_OPTIONS)).
                setOptions(fsnameOptions);
            // first time display, set display field value to SAMPLE_METRICS
            selFsName = selFsName == null ? SAMPLE_METRICS : selFsName;
            ((CCDropDownMenu) getChild(
                CHILD_FSNAME_OPTIONS)).setValue(selFsName);

            if (endDate == null) {
                // endDate is now
                Calendar c = new GregorianCalendar();
                c.setTime(new Date());
                c.set(Calendar.HOUR_OF_DAY, 23);
                c.set(Calendar.MINUTE, 00);
                endDate = c.getTime();

            } // else user selected end time
            if (startDate == null) {
                // get the snapshot schedule for fsName to calculate date range
                RecoveryPointSchedule recoveryPntSchedule =
                    SamUtil.getRecoveryPointSchedule(sysModel, selFsName);


                long periodicity = 2592000000L; // default date range ~ 1 month

                if (recoveryPntSchedule != null &&
                    !recoveryPntSchedule.isDisabled()) {
                    int periodicityUnit =
                        recoveryPntSchedule.getPeriodicityUnit();
                    switch (periodicityUnit) {
                        case SamQFSSystemModel.TIME_HOUR:
                        case SamQFSSystemModel.TIME_DAY:
                            periodicity = 259200000L; // 3 days in msec
                            break;
                        case SamQFSSystemModel.TIME_WEEK:
                        case SamQFSSystemModel.TIME_DAY_OF_WEEK:
                            periodicity = 604800000L; // 1 week in msec
                            break;
                        default:
                            periodicity = 2592000000L; // 1 month
                    }
                }
                long end = endDate.getTime();

                Calendar s = new GregorianCalendar();
                s.setTime(new Date(endDate.getTime() - periodicity));
                s.set(Calendar.HOUR_OF_DAY, 00);
                s.set(Calendar.MINUTE, 00);
                startDate = s.getTime();
            }
            // Input Validation
            if (endDate.before(startDate)) {
                throw new SamFSException(
                    SamUtil.getResourceString(
                        "reports.error.invalidTimeRange"));
            }

            String xslFileName = new StringBuffer()
                .append("/xsl/svg/")
                .append(metricType)
                .append(".xsl")
                .toString();

            if (!SAMPLE_METRICS.equals(selFsName)) {

                // Get the xml from the server, pass fsName and type as params
                try {
                    Metric metric = sysModel.getSamQFSSystemFSManager().
                        getMetric(
                            selFsName,
                            Metric.convType2Int(metricType),
                            startDate.getTime()/1000L, // api requires time(sec)
                            endDate.getTime()/1000L);

                    String jpgName = new StringBuffer()
                        .append("/tmp/")
                        .append(getServerName())
                        .append(Constants.UNDERBAR)
                        .append(selFsName)
                        .append(Constants.UNDERBAR)
                        .append(metricType)
                        .append(".jpg")
                        .toString();

                    metric.createJpg(jpgName, xslFileName);

                    this.jpgFileName = jpgName;
                    this.imageMapString = metric.getImageMapString();

                } catch (SamFSException ex) {
                    /* If SE_NO_METRICS_AVAILABLE, use empty jpg */
                    if (ex.getSAMerrno() == 32020) {
                        this.jpgFileName = new StringBuffer()
                                            .append("/samqfsui/xsl/svg/empty_")
                                            .append(metricType)
                                            .append(".jpg").toString();
                        this.imageMapString = "";
                    } else {
                        throw ex;
                    }
                }
                ((CCButton)getChild(CHILD_SCHEDULE_BUTTON)).setDisabled(false);
            } else { // display the sample image
                this.jpgFileName = new StringBuffer()
                                        .append("/samqfsui/xsl/svg/sample_")
                                        .append(metricType)
                                        .append(".jpg").toString();
                // disable the dates dropdown
                ((CCButton) getChild(CHILD_TIMERANGE_BUTTON)).setVisible(false);
                ((CCDateTimeWindow)
                    getChild(CHILD_STARTDATE_POPUP)).setVisible(false);
                ((CCStaticTextField)
                    getChild(CHILD_STARTDATE_LABEL)).setVisible(false);
                ((CCDateTimeWindow)
                    getChild(CHILD_ENDDATE_POPUP)).setVisible(false);
                ((CCStaticTextField)
                    getChild(CHILD_ENDDATE_LABEL)).setVisible(false);

            }

            ((CCStaticTextField)
                getChild(CHILD_SCHEDULE_TEXT)).
                    setValue(getDataCollectionScheduleStr(selFsName));

            updateLinks(metricType);

            // set the dates in the text field
            CCDateTimeWindow startDateTag =
                (CCDateTimeWindow)getChild(CHILD_STARTDATE_POPUP);
            if (startDateTag != null) {
                String str =
                    new SimpleDateFormat(
                        startDateTag.getDateFormatPattern()).format(startDate);
                // set the date string on the calendar text field directly
                ((CCTextField)startDateTag.getChild("textField")).setValue(str);
                // If metric type is filebyowner or filebygroup, disable
                // startDate because we cannot fit data (for more than 4 dates)
                // in the graph
                if (("filebyowner".equals(metricType)) ||
                    ("filebygroup".equals(metricType))) {
                    ((CCStaticTextField)
                        getChild(CHILD_STARTDATE_LABEL)).setVisible(false);
                    startDateTag.setVisible(false);
                }
            }
            // set the dates in the text field
            CCDateTimeWindow endDateTag =
                (CCDateTimeWindow)getChild(CHILD_ENDDATE_POPUP);
            if (endDateTag != null) {
                String str =
                    new SimpleDateFormat(
                        endDateTag.getDateFormatPattern()).format(endDate);
                // set the date string on the calendar text field directly
                ((CCTextField)endDateTag.getChild("textField")).setValue(str);
            }

        } catch (SamFSException samEx) {
            TraceUtil.trace1("Exception: " + samEx.getMessage());
            SamUtil.processException(
                samEx,
                this.getClass(),
                "FileMetricSummaryView",
                samEx.getMessage(),
                getServerName());

            SamUtil.setErrorAlert(
                this,
                this.CHILD_COMMON_ALERT,
                SamUtil.getResourceString("reports.error.populate"),
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());

        }

        TraceUtil.trace3("Exiting");
    }

    public void handleFsnameHrefRequest(RequestInvocationEvent event)
    {
        TraceUtil.trace3("Entering");
        String selFsName = (String) getDisplayFieldValue(CHILD_FSNAME_OPTIONS);
        setPageSessionAttribute(
            Constants.PageSessionAttributes.FS_NAME, selFsName);
        // preserve the fsname so that it is available between pages, i.e. if a
        // user chooses a fs 'X' in metrics page and goes to the Recovery Points
        // page, the fs 'X' should be chosen in the page
        if (!SAMPLE_METRICS.equals(selFsName)) {
            SamUtil.setLastUsedFSName(
                ((CommonViewBeanBase)getParentViewBean()).getServerName(),
                selFsName);
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleTimeRangeButtonRequest(RequestInvocationEvent event)
    {
        TraceUtil.trace3("Entering");
        // Get the start CCDateTimeWindow view
        CCDateTimeWindow startDate =
            (CCDateTimeWindow) getChild(CHILD_STARTDATE_POPUP);
        // validate Input
        if (startDate.validateDataInput()) {

            Calendar s = new GregorianCalendar();
            s.setTime(startDate.getModel().getStartDateTime());
            s.set(Calendar.HOUR_OF_DAY, 00);
            s.set(Calendar.MINUTE, 00);

            setPageSessionAttribute(
                Constants.PageSessionAttributes.START_DATE,
                (Date) s.getTime());
        }

        // Get the end CCDateTimeWindow view
        CCDateTimeWindow endDate =
            (CCDateTimeWindow) getChild(CHILD_ENDDATE_POPUP);
        // validate Input
        if (endDate.validateDataInput()) {

            Calendar e = new GregorianCalendar();
            e.setTime(endDate.getModel().getStartDateTime());
            e.set(Calendar.HOUR_OF_DAY, 23);
            e.set(Calendar.MINUTE, 00);

            setPageSessionAttribute(
                Constants.PageSessionAttributes.END_DATE,
                (Date) e.getTime());
        }

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleStorageTierHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        setPageSessionAttribute(
            Constants.PageSessionAttributes.METRIC_TYPE, "storagetier");

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleFileByLifeHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        setPageSessionAttribute(
            Constants.PageSessionAttributes.METRIC_TYPE, "filebylife");

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleFileByAgeHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        setPageSessionAttribute(
            Constants.PageSessionAttributes.METRIC_TYPE, "filebyage");

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleFileByOwnerHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        setPageSessionAttribute(
            Constants.PageSessionAttributes.METRIC_TYPE, "filebyowner");

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleFileByGroupHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        setPageSessionAttribute(
            Constants.PageSessionAttributes.METRIC_TYPE, "filebygroup");

        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private String getDataCollectionScheduleStr(String fsName)
        throws SamFSException {

        boolean existsRecoveryPntSchedule = false;
        boolean existsLiveSchedule = false;

        RecoveryPointSchedule recoveryPntSchedule = null;
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        // get the snapshot schedule for fsName
        recoveryPntSchedule =
            SamUtil.getRecoveryPointSchedule(sysModel, fsName);
        if (recoveryPntSchedule != null && !recoveryPntSchedule.isDisabled()) {
            existsRecoveryPntSchedule = true;
        }
        // get schedule to collect from live system
        Schedule [] collectFromFsSchedule =
            sysModel.getSamQFSSystemAdminManager().
                getSpecificTasks(ScheduleTaskID.REPORT, fsName);
        if (collectFromFsSchedule.length > 0
            && collectFromFsSchedule[0] != null) {
            existsLiveSchedule = true;
        }

        StringBuffer dataCollectionMsgBuf = new StringBuffer();
        dataCollectionMsgBuf.append(
            SamUtil.getResourceString(
                "reports.msg.filedistribution.datasource"));

        if (!existsRecoveryPntSchedule && !existsLiveSchedule) {
            dataCollectionMsgBuf.append("<br />").append(
                SamUtil.getResourceString(
                    "reports.msg.filedistribution.nodataavailable"));
        }
        if (existsRecoveryPntSchedule) {
            dataCollectionMsgBuf.append("<br />").append(
                SamUtil.getResourceString(
                   "reports.msg.filedistribution.collectfromrecoverypnts",
                    SamUtil.getSchedulePeriodicString(
                        recoveryPntSchedule.getPeriodicity(),
                        recoveryPntSchedule.getPeriodicityUnit())));
        }
        if (existsLiveSchedule) {
            dataCollectionMsgBuf.append("<br />").append(
                SamUtil.getResourceString(
                   "reports.msg.filedistribution.collectfromlivefs",
                    SamUtil.getSchedulePeriodicString(
                        collectFromFsSchedule[0].getPeriodicity(),
                        collectFromFsSchedule[0].getPeriodicityUnit())));
        }
        return dataCollectionMsgBuf.toString();
    }

    private void updateLinks(String noLink) {
        if (noLink == null) {
            return;
        }
        if ("storagetier".equals(noLink)) {
            ((CCHref)getChild(CHILD_STORAGETIER_HREF)).setVisible(false);
            ((CCStaticTextField)getChild(CHILD_STORAGETIER_NOHREF)).
                setValue("reports.type.storagetier");
        } else if ("filebygroup".equals(noLink)) {
            ((CCHref)getChild(CHILD_FILEBYGROUP_HREF)).setVisible(false);
            ((CCStaticTextField)getChild(CHILD_FILEBYGROUP_NOHREF)).
                setValue("reports.type.filebygroup");
        } else if ("filebyowner".equals(noLink)) {
            ((CCHref)getChild(CHILD_FILEBYOWNER_HREF)).setVisible(false);
            ((CCStaticTextField)getChild(CHILD_FILEBYOWNER_NOHREF)).
                setValue("reports.type.filebyowner");
        } else if ("filebyage".equals(noLink)) {
            ((CCHref)getChild(CHILD_FILEBYAGE_HREF)).setVisible(false);
            ((CCStaticTextField)getChild(CHILD_FILEBYAGE_NOHREF)).
                setValue("reports.type.filebyage");
        } else if ("filebylife".equals(noLink)) {
            ((CCHref)getChild(CHILD_FILEBYLIFE_HREF)).setVisible(false);
            ((CCStaticTextField)getChild(CHILD_FILEBYLIFE_NOHREF)).
                setValue("reports.type.filebylife");
        }
    }

    // display-date in xsl calls this function
    // this is merely for convenience
    public static String getDateTimeString(long timeInSecs) {
        GregorianCalendar calendar = new GregorianCalendar();
        calendar.setTimeInMillis(timeInSecs * 1000);

        SimpleDateFormat fmt =
                        (SimpleDateFormat) DateFormat.getDateTimeInstance(
                            DateFormat.SHORT, DateFormat.SHORT,
                            RequestManager.
                                getRequestContext().getRequest().getLocale());
        NumberFormat nf = NumberFormat.getIntegerInstance();
        nf.setMinimumIntegerDigits(2);
        fmt.setNumberFormat(nf);

        return fmt.format(calendar.getTime());
    }

    private static final String SAMPLE_METRICS = "-- Sample --";

    private String jpgFileName = "/samqfsui/xsl/svg/error.jpg";
    private String imageMapString = "";
}
