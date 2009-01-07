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

// ident	$Id: RecoveryPointScheduleView.java,v 1.20 2009/01/07 21:27:25 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.adm.NotifSummary;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemAdminManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.fs.Compression;
import com.sun.netstorage.samqfs.web.model.fs.RecoveryPointSchedule;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserControl;
import com.sun.netstorage.samqfs.web.remotefilechooser.RemoteFileChooserModel;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCDateTimeModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.datetime.CCDateTimeWindow;
import com.sun.web.ui.view.filechooser.CCFileChooserWindow;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import java.io.File;
import java.io.IOException;
import java.text.NumberFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.List;
import javax.servlet.ServletException;

public class RecoveryPointScheduleView extends RequestHandlingViewBase {
    // child views
    public static final String FS_NAME = "fsName";
    public static final String FILE_PATH = "recoveryPointFile";
    public static final String FILE_NAME = "namePrefixValue";
    public static final String RETENTION_TYPE = "retentionType";
    public static final String RETENTION_PERIOD = "retentionValue";
    public static final String RETENTION_PERIOD_UNIT = "retentionUnit";
    public static final String EXCLUDE_DIRS = "contentsExcludeList";
    public static final String ADD_EXCLUDE_DIRS = "excludeDirs";
    public static final String ENABLE_SCHEDULE = "enableScheduleValue";
    public static final String CALENDAR = "scheduleCalendar";
    public static final String COMPRESSION = "compressionValue";
    public static final String AUTO_INDEX = "autoIndexValue";
    public static final String PRE_SCRIPT = "preScript";
    public static final String QUIT_ON_FAILURE = "preScriptSkipOnFatalError";
    public static final String POST_SCRIPT = "postScript";
    public static final String LOG_FILE = "logFile";
    public static final String NOTIFICATION = "notificationValue";
    public static final String CONTENTS_ALERT = "contentsAlert";
    public static final String START_HOUR = "startTimeHour";
    public static final String START_MINUTE = "startTimeMinute";
    public static final String REPEAT_INTERVAL = "repeatInterval";

    // hidden field to hold he user selected directories to exclude.
    public static final String SELECTED_DIRS = "excludeDirList";

    // to be appended to the name prefix to generate unique names
    public static final String NAME_SUFFIX = "-%Y-%m-%dT%H:%M";
    public static final String MOUNT_POINT = "mountPoint";

    // file choosers names
    private static final String [] fcName = {
        FILE_PATH,
        ADD_EXCLUDE_DIRS,
        PRE_SCRIPT,
        POST_SCRIPT,
        LOG_FILE};

    // repeat intervals
    public static final String EVERY_FOUR_HOURS = "every4hours";
    public static final String EVERY_EIGHT_HOURS = "every8hours";

    // models
    private RemoteFileChooserModel fcModel;
    private CCPropertySheetModel ptModel;

    public RecoveryPointScheduleView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ptModel = PropertySheetUtil
            .createModel("/jsp/fs/RecoveryPointSchedulePropertySheet.xml");
        String serverName = ((CommonViewBeanBase)parent).getServerName();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        // register file choosers
        for (int i = 0; i < fcName.length; i++) {
            registerChild(fcName[i], RemoteFileChooserControl.class);
        }

        PropertySheetUtil.registerChildren(this, ptModel);
        registerChild(CALENDAR, CCDateTimeWindow.class);
        registerChild(MOUNT_POINT, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(MOUNT_POINT)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CONTENTS_ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(FILE_PATH) ||
            name.equals(ADD_EXCLUDE_DIRS) ||
            name.equals(PRE_SCRIPT) ||
            name.equals(POST_SCRIPT) ||
            name.equals(LOG_FILE)) {
            return new RemoteFileChooserControl(this,
                new RemoteFileChooserModel(null), name);
        } else if (name.equals(CONTENTS_ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(CALENDAR)) {
            CCDateTimeModel model = new CCDateTimeModel();
            model.setType(CCDateTimeModel.FOR_DATE_SELECTION);
            return new CCDateTimeWindow(this, model, name);
        } else if (PropertySheetUtil.isChildSupported(ptModel, name)) {
            return PropertySheetUtil.createChild(this, ptModel, name);
        } else {
            throw new IllegalArgumentException("Invalid child '" + name + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        RecoveryPointScheduleViewBean parent =
            (RecoveryPointScheduleViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        // initialize calendar
        CCDateTimeModel dtModel =
            (CCDateTimeModel) ((CCDateTimeWindow)getChild(CALENDAR)).getModel();

        // initialize file chooser models
        for (int i = 0; i < fcName.length; i++) {
            RemoteFileChooserModel fcModel = (RemoteFileChooserModel)
            ((RemoteFileChooserControl)getChild(fcName[i])).getModel();
            fcModel.setServerName(serverName);
        }

        // initialize retention period unit
        CCDropDownMenu dropDown =
            (CCDropDownMenu)getChild(RETENTION_PERIOD_UNIT);
        dropDown.setOptions(
            new OptionList(
                new String [] {
                    "common.unit.time.days",
                    "common.unit.time.weeks",
                    "common.unit.time.months",
                    "common.unit.time.years"},
                new String [] {
                    Integer.toString(SamQFSSystemModel.TIME_DAY),
                    Integer.toString(SamQFSSystemModel.TIME_WEEK),
                    Integer.toString(SamQFSSystemModel.TIME_MONTHS),
                    Integer.toString(SamQFSSystemModel.TIME_YEAR)}));

        // initialize compress type
        dropDown = (CCDropDownMenu)getChild(COMPRESSION);
        dropDown.setOptions(
            new OptionList(
                new String [] {
                    Compression.GZIP.getKey(),
                    Compression.NONE.getKey()},
                new String [] {
                    Integer.toString(Compression.GZIP.getType()),
                    Integer.toString(Compression.NONE.getType())}));

        // initialize calendar scheduler fields.
        // start hour
        NumberFormat nf = NumberFormat.getIntegerInstance();
        nf.setMinimumIntegerDigits(2);
        String [] hours = new String[24];
        for (int i = 0; i < 24; i++) { // 0-23 step 1
            hours[i] = nf.format(i);
        }
        ((CCDropDownMenu) getChild(START_HOUR)).
                    setOptions(new OptionList(hours, hours));

        // start start minute
        String [] minutes = new String[12];
        for (int i = 0, j = 0; i < 60; i += 5, j++) { // increments of 5 minutes
            minutes[j] = nf.format(i);
        }
        ((CCDropDownMenu)getChild(START_MINUTE))
        .setOptions(new OptionList(minutes, minutes));

        // repeat interval
        String [] labels = {
            "fs.recoverypointschedule.repeatInterval.2hours",
            "fs.recoverypointschedule.repeatInterval.4hours",
            "fs.recoverypointschedule.repeatInterval.8hours",
            "fs.recoverypointschedule.repeatInterval.12hours",
            "fs.recoverypointschedule.repeatInterval.1day",
            "fs.recoverypointschedule.repeatInterval.1week",
            "fs.recoverypointschedule.repeatInterval.1month"};
        String [] values = {"2h", "4h", "8h", "12h", "1d", "1w", "1M"};

        ((CCDropDownMenu) getChild(REPEAT_INTERVAL))
                    .setOptions(new OptionList(labels, values));

        TraceUtil.trace3("Exiting");
    }

    private String getRepeatIntervalName(long period, int unit) {
        TraceUtil.trace3("Entering");
        StringBuffer buf = new StringBuffer();
        buf.append(Long.toString(period))
        .append(SamQFSUtil.getTimeUnitString(unit));

        TraceUtil.trace3("Exiting");
        return buf.toString();
    }

    /** retrieve the schedule being edited in this page */
    private RecoveryPointSchedule getRecoveryPointSchedule()
    throws SamFSException {
        RecoveryPointScheduleViewBean parent =
            (RecoveryPointScheduleViewBean)getParentViewBean();
        String taskId = (String)
        parent.getPageSessionAttribute(Constants.admin.TASK_ID);
        String taskName = (String)
        parent.getPageSessionAttribute(Constants.admin.TASK_NAME);
        Schedule [] schedules =
            SamUtil.getModel(
                parent.getServerName())
                    .getSamQFSSystemAdminManager()
                    .getSpecificTasks(
                        ScheduleTaskID.getScheduleTaskID(taskId), taskName);

        RecoveryPointSchedule theSchedule = null;
        if (schedules != null && schedules.length > 0) {
            theSchedule = (RecoveryPointSchedule)schedules[0];
        }

        return theSchedule;
    }

    /** load the recovery point schedule from the server */
    public void loadSchedule() throws SamFSException { // begin load schedule
        TraceUtil.trace3("Entering");

        RecoveryPointScheduleViewBean parent =
            (RecoveryPointScheduleViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        SamQFSSystemAdminManager manager =
            SamUtil.getModel(serverName).getSamQFSSystemAdminManager();

        String taskId = (String)
        parent.getPageSessionAttribute(Constants.admin.TASK_ID);
        String taskName = (String)
        parent.getPageSessionAttribute(Constants.admin.TASK_NAME);

        // incase we came from the first time config page
        if (taskId == null) {
            taskId = ScheduleTaskID.SNAPSHOT.getId();
        }
        if (taskName == null) {
            taskName = getFileSystemName();
        }
        ScheduleTaskID id = ScheduleTaskID.getScheduleTaskID(taskId);

        Schedule [] schedules = manager.getSpecificTasks(id, taskName);
        RecoveryPointSchedule schedule = null;

        // retrieve the mount point
        String mountPoint = SamUtil.getModel(serverName)
        .getSamQFSSystemFSManager()
        .getFileSystem(taskName)
        .getMountPoint();

        if (schedules != null && schedules.length !=  0) { // we've a schedule
            schedule = (RecoveryPointSchedule)schedules[0];

            // recovery point file path
            RemoteFileChooserControl chooser =
                (RemoteFileChooserControl)getChild(FILE_PATH);
            ((CCTextField) chooser.getChild(
                CCFileChooserWindow.BROWSED_FILE_NAME)).
                setValue(schedule.getLocation());

            // prefix retrieve the actual prefix from the name
            String prefix = schedule.getNames();
            if (prefix != null) {
                String [] prefixTokens = prefix.split(NAME_SUFFIX);
                if (prefixTokens.length > 0) {
                    prefix = prefixTokens[0];
                } else {
                    prefix = "";
                }
            } else {
                prefix = "";
            }
            ((CCTextField)getChild(FILE_NAME)).setValue(prefix);

            // retention period
            CCRadioButton retentionType =
                (CCRadioButton)getChild(RETENTION_TYPE);
            CCTextField retentionValue =
                (CCTextField)getChild(RETENTION_PERIOD);
            CCDropDownMenu retentionUnit =
                (CCDropDownMenu)getChild(RETENTION_PERIOD_UNIT);

            if (schedule.getDuration() == -1) { // retain forever
                retentionType.setValue("always");
                retentionValue.setValue("1");
                retentionValue.setDisabled(true);
                retentionUnit
                    .setValue(Integer.toString(SamQFSSystemModel.TIME_YEAR));
                retentionUnit.setDisabled(true);
            } else { // retain for the specified period
                retentionType.setValue("until");
                retentionValue.setDisabled(false);
                retentionUnit.setDisabled(false);

                retentionValue.setValue(Long.toString(schedule.getDuration()));
                retentionUnit.setValue(
                    Integer.toString(schedule.getDurationUnit()));

            }

            // exclude directories
            String [] dirs = schedule.getExcludedDirs();
            dirs = dirs == null ? new String[0] : dirs;

            // Append mount point of the file system to the entries in the
            // dirList.  RemoteFileChooser always adds the selected directory
            // with its absolute path.  Thus we need to be consistent and
            // populate the existing paths with absolute paths as well.
            for (int i = 0; i < dirs.length; i++) {
                dirs[i] = mountPoint.concat(File.separator).concat(dirs[i]);
            }

            OptionList dirList = null;
            if (dirs.length > 0) {
                dirList = new OptionList(dirs, dirs);
                dirList.add(0,
                    "fs.recoverypointschedule.contents.exclude.dummy",
                    "");
            } else {
                dirList = new OptionList(new String []
                {"fs.recoverypointschedule.contents.exclude.dummy"},
                    new String [] { ""});
            }

            ((CCSelectableList)getChild(EXCLUDE_DIRS)).setOptions(dirList);

            // schedule calendar
            CCDateTimeWindow dtw = (CCDateTimeWindow)getChild(CALENDAR);
            CCDateTimeModel dtModel = (CCDateTimeModel)dtw.getModel();

            Date startTime = schedule.getStartTime();

            String dateString =
                new SimpleDateFormat(
                    dtw.getDateFormatPattern()).format(startTime);

            dtModel.setStartDateTime(startTime);
            ((CCTextField)dtw.getChild("textField")).setValue(dateString);

            // initialize the date time window start time & repeat interval
            // parameters to work around a lockhart problem.
            GregorianCalendar cal =
                new GregorianCalendar(
                    getRequestContext().getRequest().getLocale());
            cal.setTime(startTime);
            int startHour = cal.get(Calendar.HOUR);
            if (cal.get(Calendar.AM_PM) == Calendar.PM) {
                startHour += 12;
            }
            int startMinute = ((int)cal.get(Calendar.MINUTE)/5) * 5;
            NumberFormat nf = NumberFormat.getIntegerInstance();
            nf.setMinimumIntegerDigits(2);
            ((CCDropDownMenu) getChild(START_HOUR)).
                            setValue(nf.format(startHour));
            ((CCDropDownMenu) getChild(START_MINUTE)).
                            setValue(nf.format(startMinute));

            // repeat interval
            String repeatInterval =
                getRepeatIntervalName(schedule.getPeriodicity(),
                schedule.getPeriodicityUnit());
            ((CCDropDownMenu)
                getChild(REPEAT_INTERVAL)).setValue(repeatInterval);

            // enable schedule
            String enableSchedule = schedule.isDisabled() ? "false" : "true";
            ((CCCheckBox)getChild(ENABLE_SCHEDULE)).setValue(enableSchedule);

            // compression type
            Compression compression = schedule.getCompression();
            if (compression != null) {
                ((CCDropDownMenu)getChild(COMPRESSION))
                .setValue(Integer.toString(compression.getType()));

                // if the compression type is 'compress' add it as an option
                if (compression == Compression.COMPRESS) {
                    ((CCDropDownMenu)getChild(COMPRESSION)).getOptions().add(
                        0,
                        Compression.COMPRESS.getKey(),
                        Integer.toString(Compression.COMPRESS.getType()));
                }
            }

            // index after creation
            String autoIndex = schedule.isAutoIndex() ? "true" : "false";
            ((CCCheckBox)getChild(AUTO_INDEX)).setValue(autoIndex);

            // prescript
            ((CCTextField)((RemoteFileChooserControl)getChild(PRE_SCRIPT))
            .getChild(CCFileChooserWindow.BROWSED_FILE_NAME))
            .setValue(schedule.getPrescript());

            // proceed even if prescript fatally fails
            String quitOnPreFailure =
                schedule.isQuitOnPrescriptFailure() ? "true" : "false";
            ((CCCheckBox)getChild(QUIT_ON_FAILURE)).setValue(quitOnPreFailure);

            // post script
            ((CCTextField)((RemoteFileChooserControl)getChild(POST_SCRIPT))
            .getChild(CCFileChooserWindow.BROWSED_FILE_NAME))
            .setValue(schedule.getPostscript());

            // log file
            ((CCTextField)((RemoteFileChooserControl)getChild(LOG_FILE))
            .getChild(CCFileChooserWindow.BROWSED_FILE_NAME))
            .setValue(schedule.getLogFile());


        } else { // no schedule, must be creating a new schedule
            Date startTime = new Date();
            CCDateTimeWindow dtw = (CCDateTimeWindow)getChild(CALENDAR);
            CCDateTimeModel dtModel = (CCDateTimeModel) dtw.getModel();

            String dateString =
                new SimpleDateFormat(dtw.getDateFormatPattern())
                .format(startTime);

            dtModel.setStartDateTime(startTime);
            ((CCTextField)dtw.getChild("textField")).setValue(dateString);
            // initialize starte date/time
            GregorianCalendar cal = new GregorianCalendar(
                getRequestContext().getRequest().getLocale());

            cal.setTime(startTime);
            int startHour = cal.get(Calendar.HOUR);
            if (cal.get(Calendar.AM_PM) == Calendar.PM) {
                startHour += 12;
            }
            int startMinute = ((int)cal.get(Calendar.MINUTE)/5) * 5;
            NumberFormat nf = NumberFormat.getIntegerInstance();
            nf.setMinimumIntegerDigits(2);
            ((CCDropDownMenu)getChild(START_HOUR))
            .setValue(nf.format(startHour));
            ((CCDropDownMenu)getChild(START_MINUTE))
            .setValue(nf.format(startMinute));

            // default to a repeat interval of daily
            ((CCDropDownMenu)getChild(REPEAT_INTERVAL)).setValue("1d");

            // enable schedule
            ((CCCheckBox)getChild(ENABLE_SCHEDULE)).setValue("true");

            // enable auto-indexing
            ((CCCheckBox)getChild(AUTO_INDEX)).setValue("true");

            // insert the label options for exclude dirs & notification emails
            ((CCSelectableList)getChild(EXCLUDE_DIRS))
            .setOptions(new OptionList(
                new String [] {
                "fs.recoverypointschedule.contents.exclude.dummy"},
                new String [] {""}));

            // set the fs name as the default prefix
            ((CCTextField)getChild(FILE_NAME)).setValue(taskName);

            // default location to mount point
            RemoteFileChooserControl chooser =
                (RemoteFileChooserControl)getChild(FILE_PATH);
            ((CCTextField)chooser
                .getChild(CCFileChooserWindow.BROWSED_FILE_NAME))
                .setValue(mountPoint);

            // default retention schedule - forever
            ((CCRadioButton)getChild(RETENTION_TYPE)).setValue("always");

            // disable retention period fields
            ((CCTextField)getChild(RETENTION_PERIOD)).setDisabled(true);
            ((CCDropDownMenu)getChild(RETENTION_PERIOD_UNIT))
            .setDisabled(true);
        }

        // set the file system name
        ((CCStaticTextField)getChild(FS_NAME)).setValue(taskName);

        // notifications
        String rawEmail = SamUtil.getModel(serverName)
        .getSamQFSSystemAdminManager()
        .getNotificationEmailsForSubject(NotifSummary.NOTIF_SUBJ_DUMPINTR);

        OptionList notificationList = new OptionList();
        if (rawEmail != null) {
            String [] email = rawEmail.split(",");
            notificationList.setOptions(email, email);
        }
        notificationList.add(0,
            "fs.recoverypointschedule.psheet.notification.dummy",
            "");
        ((CCSelectableList)getChild(NOTIFICATION))
        .setOptions(notificationList);

        // send the mount point to the broswe for initializing the 'add'
        // excluded dirs button
        ((CCHiddenField)getChild(MOUNT_POINT)).setValue(mountPoint);
        TraceUtil.trace3("Exiting");
    } // end begin load schedule


    /** save the recovery point schedule */
    public List saveSchedule() throws SamFSException {
        TraceUtil.trace3("Entering");

        List errors = new ArrayList();
        RecoveryPointScheduleViewBean parent =
            (RecoveryPointScheduleViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        SamQFSSystemAdminManager manager =
            SamUtil.getModel(serverName).getSamQFSSystemAdminManager();

        RecoveryPointSchedule schedule = getRecoveryPointSchedule();
        if (schedule == null) { // we must be creating a new schedule
            schedule = new RecoveryPointSchedule();

            // set the task name & task id
            String taskName = (String)
            parent.getPageSessionAttribute(Constants.admin.TASK_NAME);
            String taskIdString = (String)
            parent.getPageSessionAttribute(Constants.admin.TASK_ID);
            if (taskName == null) {
                taskName = (String)parent.getPageSessionAttribute(
                    Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
            }
            ScheduleTaskID taskId =
                ScheduleTaskID.getScheduleTaskID(taskIdString);

            schedule.setTaskName(taskName);
            schedule.setTaskId(taskId);
        }

        // retrieve the schedule object

        // file path
        String filePath = ((RemoteFileChooserControl)getChild(FILE_PATH))
        .getDisplayFieldStringValue(CCFileChooserWindow.BROWSED_FILE_NAME);
        schedule.setLocation(filePath);

        // file name
        String fileName = getDisplayFieldStringValue(FILE_NAME);
        schedule.setNames(fileName.concat(NAME_SUFFIX));

        // retention schedule
        if ("always".equals(getDisplayFieldStringValue(RETENTION_TYPE))) {
            // retain forever
            schedule.setDuration(-1);
            schedule.setDurationUnit(-1);
        } else {
            try {
                long retentionPeriod =
                    Long.parseLong(
                        getDisplayFieldStringValue(RETENTION_PERIOD));
                int retentionPeriodUnit =
                    Integer.parseInt(
                        getDisplayFieldStringValue(RETENTION_PERIOD_UNIT));
                if (retentionPeriod < 1) {
                    errors.add("fs.recoverypointschedule.retention.negative");
                } else {
                    schedule.setDuration(retentionPeriod);
                    schedule.setDurationUnit(retentionPeriodUnit);
                }
            } catch (NumberFormatException nfe) {
                errors.add("fs.recoverypointschedule.retention.nan");
            }
        }

        // excluded dirs
        String mountPoint =
            (String) ((CCHiddenField)getChild(MOUNT_POINT)).getValue();
        String rawDirs = getDisplayFieldStringValue(SELECTED_DIRS);
        if (rawDirs != null && rawDirs.length() > 0) {
            String [] dirs = rawDirs.split(":");
            for (int i = 0; i < dirs.length; i++) {
                if (dirs[i].startsWith(mountPoint)) {
                    dirs[i] = dirs[i].substring(mountPoint.length() + 1);
                }
            }
            schedule.setExcludeDirs(dirs);
        }

        // enable schedule
        boolean disabled =
            "false".equals(getDisplayFieldStringValue(ENABLE_SCHEDULE));
        schedule.setDisabled(disabled);

        // date time
        CCDateTimeWindow dt = (CCDateTimeWindow)getChild(CALENDAR);

        // validate datetime input
        if (dt.validateDataInput()) {
            // get the start date and time
            CCDateTimeModel dtModel = (CCDateTimeModel)dt.getModel();
            Date startDateTime = dtModel.getStartDateTime();
            GregorianCalendar orig = new GregorianCalendar();
            orig.setTime(startDateTime);

            int hour =
                Integer.parseInt(getDisplayFieldStringValue(START_HOUR));
            int minute =
                Integer.parseInt(getDisplayFieldStringValue(START_MINUTE));
            GregorianCalendar cal =
                new GregorianCalendar(orig.get(Calendar.YEAR),
                orig.get(Calendar.MONTH),
                orig.get(Calendar.DAY_OF_MONTH),
                hour,
                minute);

            startDateTime = cal.getTime();

            // repeat interval
            String repeatInterval =
                getDisplayFieldStringValue(REPEAT_INTERVAL);
            long interval = SamQFSUtil.getLongValSecond(repeatInterval);
            int intervalUnit = SamQFSUtil.getTimeUnitInteger(repeatInterval);

            schedule.setStartTime(startDateTime);
            schedule.setPeriodicity(interval);
            schedule.setPeriodicityUnit(intervalUnit);
        }

        // compression type
        int c = Integer.parseInt(getDisplayFieldStringValue(COMPRESSION));
        Compression compression = Compression.getCompression(c);
        schedule.setCompression(compression);

        // auto-index
        boolean b = "true".equals(getDisplayFieldStringValue(AUTO_INDEX));
        schedule.setAutoIndex(b);

        // pre-script
        String prescript = ((CCFileChooserWindow)getChild(PRE_SCRIPT))
        .getDisplayFieldStringValue(CCFileChooserWindow.BROWSED_FILE_NAME);
        schedule.setPrescript(prescript);

        // quit on prescript failure
        b = "true".equals(getDisplayFieldStringValue(QUIT_ON_FAILURE));
        schedule.setQuitOnPrescriptFailure(b);

        // post-script
        String postscript = ((CCFileChooserWindow)getChild(POST_SCRIPT))
        .getDisplayFieldStringValue(CCFileChooserWindow.BROWSED_FILE_NAME);
        schedule.setPostscript(postscript);

        // log file
        String logFile = ((CCFileChooserWindow)getChild(LOG_FILE))
        .getDisplayFieldStringValue(CCFileChooserWindow.BROWSED_FILE_NAME);
        schedule.setLogFile(logFile);

        // make sure we have all the fields requried for a recovery point
        // schedule : start date & time, location, prefix, periodicity
        if (schedule.getStartTime() == null ||
            schedule.getPeriodicity() == -1 ||
            schedule.getLocation() == null ||
            schedule.getLocation().length() == 0 ||
            schedule.getNames() == null ||
            schedule.getNames().length() == 0) {
            errors.add("fs.recoverypointschedule.error.incomplete");
        }

        if (errors.size() == 0) { // no errors found
            // save the schedule
            manager.setTaskSchedule(schedule);
        }

        TraceUtil.trace3("Exiting");
        return errors;
    }

    // safety handler for the file chooser browser buttons
    public void handleBrowseServerRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    private String getFileSystemName() {
        String fsName = RequestManager
            .getRequestContext().getRequest().getParameter("FS_NAME");

        // we must have come from the common tasks page.
        RecoveryPointScheduleViewBean parent =
            (RecoveryPointScheduleViewBean)getParentViewBean();
        parent.setFromFirstTimeConfig(true);
        return fsName;
    }
}
