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

// ident	$Id: CurrentJobsView.java,v 1.23 2008/05/16 18:38:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import java.util.ArrayList;
import javax.servlet.ServletException;

/**
 * Creates the 'Current Jobs' Action Table and provides
 * handlers for the links within the table.
 */
public class CurrentJobsView extends CommonTableContainerView {

    private String selectedFilter;
    private CurrentJobsModel model;
    private ArrayList updatedModel = new ArrayList();
    private String serverName;

    public static final String CHILD_FILTERMENU_HREF = "FilterMenuHref";

    public static final String CHILD_TILED_VIEW = "CurrentJobsTiledView";

    public CurrentJobsView(View parent, String name, String serverName) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "CurrentJobsTable";

        this.serverName = serverName;

        // Check the system license type
        // If it is QFS only, load the table xml that's without filter
        if (isSystemQFSOnly()) {
            model = new CurrentJobsModel(
                serverName, "/jsp/jobs/CurrentJobsTableQFS.xml");
        } else {
            model = new CurrentJobsModel(
                serverName, "/jsp/jobs/CurrentJobsTable.xml");
        }

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_FILTERMENU_HREF, CCHref.class);
        registerChild(CHILD_TILED_VIEW, CurrentJobsTiledView.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {

        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Creating child " + name);
        if (name.equals(CHILD_FILTERMENU_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_TILED_VIEW)) {
            TraceUtil.trace3("Exiting");
            return new CurrentJobsTiledView(this, model, name);
        } else {
            TraceUtil.trace3("Exiting");
            return super.createChild(model, name, CHILD_TILED_VIEW);
        }
    }

    /**
     * Handler for the filter
     */
    public void handleFilterMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String selectedType = (String) getDisplayFieldValue("FilterMenu");
        TraceUtil.trace3("selectedType " + selectedType);

        ViewBean parentBean = getParentViewBean();
        if (selectedType != null) {
            parentBean.setPageSessionAttribute(
                Constants.PageSessionAttributes.CURJOBSFILTER_MENU,
                selectedType);
        } else {
            parentBean.setPageSessionAttribute(
                Constants.PageSessionAttributes.CURJOBSFILTER_MENU, "");
        }

        if (selectedType != null) {
            if (!selectedType.equals(CCActionTable.FILTER_ALL_ITEMS_OPTION)) {
                if (selectedType.equals("1")) {
                    selectedFilter = new String("Jobs.jobType1");
                } else if (selectedType.equals("2")) {
                    selectedFilter = new String("Jobs.jobType2");
                } else if (selectedType.equals("3")) {
                    selectedFilter = new String("Jobs.jobType3");
                } else if (selectedType.equals("5")) {
                    selectedFilter = new String("Jobs.jobType5");
                } else if (selectedType.equals("6")) {
                    selectedFilter = new String("Jobs.jobType6");
                } else if (selectedType.equals("7")) {
                    selectedFilter = new String("Jobs.jobType7");
                } else if (selectedType.equals("dump")) {
                    selectedFilter = new String("Jobs.jobType.dump");
                } else if (selectedType.equals("dumpEnable")) {
                    selectedFilter = new String(BaseJob.TYPE_ENABLE_DUMP_STR);
                } else if (selectedType.equals("restoreSearch")) {
                    selectedFilter =
                                new String(BaseJob.TYPE_RESTORE_SEARCH_STR);
                } else if (selectedType.equals("restore")) {
                    selectedFilter = new String(BaseJob.TYPE_RESTORE_STR);
                }
            } else {
                selectedFilter = Constants.Filter.FILTER_ALL;
            }
            model.setFilter(selectedFilter);
        }

        try {
            // remove tooltips
            ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
             getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");
            model.initModelRows();
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "handleFilterMenuHrefRequest",
                "Cannot repopulate current jobs",
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "CurrentJobs.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }

        updatedModel = model.getLatestIndex();
        parentBean.forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }


    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.SAM_CONTROL)) {

            model.setSelectionType("none");
        }

        model.setRowSelected(false);
        TraceUtil.trace3("Exiting");
    }

    public void handleButton0Request(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        CCActionTable actionTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);

        try {
            actionTable.restoreStateData();
        }
        catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "handleButton0Request",
                "Exception occurred within framework",
                serverName);

            // just throw it, let it go to onUncaughtException()
            throw mcex;
        }

        CurrentJobsModel currentModel = (CurrentJobsModel)
            actionTable.getModel();

        Integer [] selectedRows = currentModel.getSelectedRows();

        TraceUtil.trace3("selectedRows.length is " + selectedRows.length);

        // get all of the records to be deleted first
        ArrayList jobIds = new ArrayList();

        try {
            for (int i = 0; i < selectedRows.length; i++) {
                int row = selectedRows[i].intValue();
                currentModel.setRowIndex(row);
                String jobId = (String) currentModel.getValue("JobHidden");

                // add the job id to a ArrayList of jobids to be deleted
                jobIds.add(Long.valueOf(jobId));
            }

            // now delete all of the jobs
            Object [] jobIdArray = jobIds.toArray();

            for (int i = 0; i < jobIdArray.length; i++) {

            long jobId = Long.parseLong(jobIdArray[i].toString());

                LogUtil.info(this.getClass(), "handleButton0Request",
                     "Start deleting Job ID " + jobId);

                SamQFSSystemModel systemModel = SamUtil.getModel(serverName);
                BaseJob job =
                    systemModel.getSamQFSSystemJobManager().getJobById(jobId);
                systemModel.getSamQFSSystemJobManager().cancelJob(job);

                LogUtil.info(this.getClass(), "handleButton0Request",
                    "Done deleting Job ID " + jobId);
            }

            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "success.summary",
                "Jobs.cancelMsg",
                serverName);

        }
        catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "handleButton1Request",
                "Cannot delete job",
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "CurrentJobs.error.failedDelete",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
        updateModel();
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private void updateModel() {
        TraceUtil.trace3("Entering");
        String selectedType = (String)
            getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.CURJOBSFILTER_MENU);
        TraceUtil.trace3("selectedType " + selectedType);

        if (selectedType != null) {
            if (!selectedType.equals(CCActionTable.FILTER_ALL_ITEMS_OPTION)) {
                if (selectedType.equals("1")) {
                    selectedFilter = "Jobs.jobType1";
                } else if (selectedType.equals("2")) {
                    selectedFilter = "Jobs.jobType2";
                } else if (selectedType.equals("3")) {
                    selectedFilter = "Jobs.jobType3";
                } else if (selectedType.equals("5")) {
                    selectedFilter = "Jobs.jobType5";
                } else if (selectedType.equals("6")) {
                    selectedFilter = "Jobs.jobType6";
                } else if (selectedType.equals("7")) {
                    selectedFilter = "Jobs.jobType7";
                } else if (selectedType.equals("dump")) {
                    selectedFilter = new String("Jobs.jobType.dump");
                } else if (selectedType.equals("dumpEnable")) {
                    selectedFilter = new String(BaseJob.TYPE_ENABLE_DUMP_STR);
                } else if (selectedType.equals("restoreSearch")) {
                    selectedFilter =
                                new String(BaseJob.TYPE_RESTORE_SEARCH_STR);
                } else if (selectedType.equals("restore")) {
                    selectedFilter = new String(BaseJob.TYPE_RESTORE_STR);
                }
            } else {
                selectedFilter = Constants.Filter.FILTER_ALL;
            }
            ((CCDropDownMenu) getChild("FilterMenu")).setValue(selectedType);
            model.setFilter(selectedFilter);
        }

        try {
            // remove tooltips
            ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
             getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");
            model.initModelRows();
        }
        catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "updateModel",
                "Cannot re-populate current jobs",
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "CurrentJobs.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
        updatedModel = model.getLatestIndex();
        TraceUtil.trace3("Exiting");
    }

    public void populateData() throws SamFSException {
        TraceUtil.trace3("Entering");
        updateModel();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Return true/false if the system is a QFS-only system or not
     */
    private boolean isSystemQFSOnly() {
        return SamUtil.getSystemType(serverName) == SamQFSSystemModel.QFS;
    }
}
