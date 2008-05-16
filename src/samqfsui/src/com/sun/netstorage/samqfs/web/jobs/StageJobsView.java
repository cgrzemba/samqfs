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

// ident	$Id: StageJobsView.java,v 1.10 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.table.CCActionTable;

/**
 * Creates the ArchivePolSummary Action Table and provides
 * handlers for the links within the table.
 */
public class StageJobsView extends CommonTableContainerView {

    private CCPageTitleModel pageTitleModel;
    private CCPropertySheetModel propertySheetModel;

    private StageJobsModel model;

    private BaseJob baseJob;
    private StageJob stageJob;

    private long jobId;

    private String serverName;

    public StageJobsView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "StageJobsTable";

        serverName = (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        TraceUtil.trace3("Got serverName from page session: " + serverName);
        String stageJobIDStr =
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.STAGE_JOB_ID);
        TraceUtil.trace3("Got stageJobID from page session: " + stageJobIDStr);

        LargeDataSet dataSet =
            new StageJobsData(
                serverName, Long.parseLong(stageJobIDStr));
        model = new StageJobsModel(dataSet);

        pageTitleModel = createPageTitleModel();
        createPropertySheetModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren(model);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
	    // Property Sheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            return PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else {
            return super.createChild(model, name);
        }
    }

    public void populateData() throws SamFSException {
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
                "populateData",
                "ModelControlException occurred within framework",
                serverName);
            throw new SamFSException("Exception occurred with framework");
        }
        model.initModelRows();
        TraceUtil.trace3("Exiting");
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel =
                PageTitleUtil.createModel("/jsp/jobs/StageJobsPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private void createPropertySheetModel() {
        TraceUtil.trace3("Entering");

        ViewBean vb = getParentViewBean();
        String jobIdString = (String) vb.getPageSessionAttribute(
            Constants.PageSessionAttributes.JOB_ID);
        TraceUtil.trace3("Got jobIdString from page session: " + jobIdString);

        try {
            String [] jobIdSplit = jobIdString.split(",");
            jobId = Long.parseLong(jobIdSplit[0]);
            String jobCondition = jobIdSplit[2];

            // the load.. methods will load the correct propertysheet
            // and populate the propertySheetModel
            if (jobCondition.equals("Current")) {
                loadStageCurrentPropertySheet();
            } else if (jobCondition.equals("Pending")) {
                loadStagePendingPropertySheet();
            }

             // load standard values into the property sheet
             propertySheetModel.setValue("jobIdText", jobIdSplit[0]);
             propertySheetModel.setValue("jobTypeText", "Jobs.jobType2");
        }
        catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "createPropertySheetModel",
                "Cannot populate job details",
                serverName);

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "JobsDetails.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
        TraceUtil.trace3("Exiting");
    }

    private void loadStageCurrentPropertySheet() throws SamFSException {
        propertySheetModel = PropertySheetUtil.createModel(
            "/jsp/jobs/StageJobsCurrentPropSheet.xml");
        getModelInfo();
        loadGenericStageValues();
        propertySheetModel.setValue("positionText", stageJob.getPosition());
        propertySheetModel.setValue("offSetText", stageJob.getOffset());
        propertySheetModel.setValue("sizeFileText", stageJob.getFileSize());
        propertySheetModel.setValue("fileNameText", stageJob.getFileName());
    }

    private void loadStagePendingPropertySheet() throws SamFSException {
        propertySheetModel = PropertySheetUtil.createModel(
            "/jsp/jobs/StageJobsPendingPropSheet.xml");
        getModelInfo();
        loadGenericStageValues();
    }


    private void loadGenericStageValues() {
        propertySheetModel.setValue("vsnText", stageJob.getVSNName());
        propertySheetModel.setValue(
            "medTypeText", SamUtil.getMediaTypeString(stageJob.getMediaType()));
        propertySheetModel.setValue(
            "initTimeText", SamUtil.getTimeString(stageJob.getStartDateTime()));
    }

    private void getModelInfo() throws SamFSException {
        TraceUtil.trace3("Entering");
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        // get the job that will be displayed
        baseJob = sysModel.getSamQFSSystemJobManager().getJobById(jobId);
        if (baseJob == null) {
            throw new SamFSException(null, -2011);
        }

        stageJob = (StageJob) baseJob;
        TraceUtil.trace3("Exiting");
    }
}
