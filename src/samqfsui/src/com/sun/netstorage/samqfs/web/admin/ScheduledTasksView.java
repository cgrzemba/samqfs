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

//	ident	$Id: ScheduledTasksView.java,v 1.12 2008/12/16 00:10:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.fs.RecoveryPointScheduleViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemAdminManager;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletException;
import com.sun.web.ui.view.html.CCHiddenField;

/**
 * Scheduled Tasks Page
 */
public class ScheduledTasksView extends CommonTableContainerView {
    public static final String TILED_VIEW = "ScheduledTasksTiledView";

    public static final String SCHEDULE_ID = "selectedId";
    public static final String SCHEDULE_NAME = "selectedName";

    private CCActionTableModel tableModel;

    public ScheduledTasksView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "ScheduledTasksTable";
        createTableModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(TILED_VIEW, ScheduledTasksTiledView.class);
        registerChild(SCHEDULE_ID, CCHiddenField.class);
        registerChild(SCHEDULE_NAME, CCHiddenField.class);
        super.registerChildren(tableModel);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new StringBuffer(
            "Entering: child: ").append(name).toString());
        if (name.equals(SCHEDULE_ID) ||
            name.equals(SCHEDULE_NAME)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(TILED_VIEW)) {
            return new ScheduledTasksTiledView(this, tableModel, name);
        } else {
            return super.createChild(tableModel, name, TILED_VIEW);
        }
    }

    private void createTableModel() {
        TraceUtil.trace3("Entering");

        this.tableModel = new CCActionTableModel(
             RequestManager.getRequestContext().getServletContext(),
             "/jsp/admin/ScheduledTasksTable.xml");

        TraceUtil.trace3("Exiting");
    }

    /** begin display of the table container view */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // set button labels
        this.tableModel.setActionValue("Remove", "common.button.remove");
        this.tableModel.setActionValue("Edit", "common.button.edit");

        // set column titles
        this.tableModel.setActionValue("Id", "admin.scheduledtasks.table.id");
        this.tableModel.setActionValue("Name",
                                       "admin.scheduledtasks.table.name");
        this.tableModel.setActionValue("StartTime",
                                       "admin.scheduledtasks.table.starttime");
        this.tableModel.setActionValue("RepeatInterval",
                                  "admin.scheduledtasks.table.repeatinterval");

        // disable tool tips
        this.tableModel.setSelectionType(CCActionTableModel.SINGLE);
        CCActionTable theTable = (CCActionTable)getChild(CHILD_ACTION_TABLE);
        ((CCRadioButton)theTable
         .getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

        // make sure users are authorized to do this
        if (!SecurityManagerFactory.getSecurityManager()
            .hasAuthorization(Authorization.CONFIG)) {
            tableModel.setSelectionType(CCActionTableModel.NONE);
        }
    }

    /** populate the scheduled tasks table */
    public void populateTableModel() {
        ScheduledTasksViewBean parent =
            (ScheduledTasksViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        try {
            SamQFSSystemAdminManager manager =
                SamUtil.getModel(serverName).getSamQFSSystemAdminManager();
            Schedule [] schedule = manager.getSpecificTasks(null, null);

            StringBuffer ids = new StringBuffer(), names = new StringBuffer();

            tableModel.clear();

            for (int i = 0; i < schedule.length; i++) { // for each schedule
                if (i > 0)
                    this.tableModel.appendRow();

                // id
                this.tableModel.setValue("IdText",
                                     schedule[i].getTaskId().getDescription());
                this.tableModel.setValue("TaskId",
                                         schedule[i].getTaskId().getId());

                // name
                String name = schedule[i].getTaskName();
                name = name == null ? "" : name;
                this.tableModel.setValue("NameText", name);
                this.tableModel.setValue("TaskName", name);

                // start time
                this.tableModel.setValue("StartTimeText",
                          SamUtil.getTimeString(schedule[i].getStartTime()));

                String s = "";
                // periodicity
                long period = schedule[i].getPeriodicity();
                if (period != -1) {
                    s = s.concat(Long.toString(period)).concat(" ")
                        .concat(SamUtil.getTimeUnitL10NString(
                                schedule[i].getPeriodicityUnit()));
                }
                this.tableModel.setValue("RepeatIntervalText", s);

                ids.append(schedule[i].getTaskId().getId()).append(";");
                names.append(name).append(";");

                this.tableModel.setRowSelected(false);
            }

            // save the list of schedule names & ids for the browser-side
            ((CCHiddenField)parent.getChild(parent.ALL_IDS))
                .setValue(ids.toString());
            ((CCHiddenField)parent.getChild(parent.ALL_NAMES))
                .setValue(names.toString());
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "populateTableModel",
                                     "Unable to retrieve scheduled tasks",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  sfe.getMessage(),
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
    }

    public void handleNewRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        CommonViewBeanBase target = (CommonViewBeanBase)
            getViewBean(RecoveryPointScheduleViewBean.class);
        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();

        source.forwardTo(target);
    }


    public void handleRemoveRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        ScheduledTasksViewBean parent =
            (ScheduledTasksViewBean)getParentViewBean();
        String serverName = parent.getServerName();

        try {
            String id = getDisplayFieldStringValue(SCHEDULE_ID);
            String name = getDisplayFieldStringValue(SCHEDULE_NAME);

            // try to retrieve the named schedule
            SamQFSSystemAdminManager adminManager =
                SamUtil.getModel(serverName).getSamQFSSystemAdminManager();
            ScheduleTaskID taskId = ScheduleTaskID.getScheduleTaskID(id);

            Schedule [] schedule = adminManager.getSpecificTasks(taskId, name);
            if (schedule != null && schedule.length > 0) {
                // remove the schedule.
                // only one schedule should match
                adminManager.removeTaskSchedule(schedule[0]);

                // success alert
                SamUtil.setInfoAlert(parent,
                                     parent.CHILD_COMMON_ALERT,
                                     "success.summary",
                                     "admin.scheduledtasks.remove.success",
                                     serverName);
            }
        } catch (SamFSException sfe) {
            // process exception
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleRemoveRequest",
                                     "admin.scheduledtasks.remove.failure",
                                     serverName);

            // error alert
            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "failure.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // disable the add remove button
        ((CCButton)getChild("Edit")).setDisabled(true);
        ((CCButton)getChild("Remove")).setDisabled(true);
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleEditRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {

        // retrieve the schedule name and id, set them in the page session
        // attribute and forward to the right schedule details page
        String taskId = getDisplayFieldStringValue(SCHEDULE_ID);
        String taskName = getDisplayFieldStringValue(SCHEDULE_NAME);

        CommonViewBeanBase source = (CommonViewBeanBase)getParentViewBean();
        CommonViewBeanBase target = null;

        ScheduleTaskID id = ScheduleTaskID.getScheduleTaskID(taskId);
		if (id.equals(ScheduleTaskID.SNAPSHOT)) {
                    target = (CommonViewBeanBase)
                            getViewBean(RecoveryPointScheduleViewBean.class);
		} else if (id.equals(ScheduleTaskID.REPORT)) {
                    target = (CommonViewBeanBase)
                            getViewBean(FileMetricSummaryViewBean.class);
		} else {
                    source.forwardTo(getRequestContext());
		}

        source.setPageSessionAttribute(Constants.admin.TASK_ID, taskId);
        source.setPageSessionAttribute(Constants.admin.TASK_NAME, taskName);

        BreadCrumbUtil.breadCrumbPathForward(source,
            PageInfo.getPageInfo().getPageNumber(source.getName()));

        // finally forward to the target view bean
        source.forwardTo(target);
    }

    public void handleIdHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    public void handleNameHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }
}
