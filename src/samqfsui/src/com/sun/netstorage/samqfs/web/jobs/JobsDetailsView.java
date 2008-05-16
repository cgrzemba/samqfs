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

// ident	$Id: JobsDetailsView.java,v 1.25 2008/05/16 18:38:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.RestoreFile;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.EnableDumpJob;
import com.sun.netstorage.samqfs.web.model.job.FSDumpJob;
import com.sun.netstorage.samqfs.web.model.job.GenericJob;
import com.sun.netstorage.samqfs.web.model.job.LabelJob;
import com.sun.netstorage.samqfs.web.model.job.MountJob;
import com.sun.netstorage.samqfs.web.model.job.ReleaseJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreSearchJob;
import com.sun.netstorage.samqfs.web.model.job.SamfsckJob;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.FsmVersion;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;

/**
 * Creates the 'Job Details Page'. This ViewBean is dynamic in nature in
 * that it creates multiple types of property sheets depending on the
 * type and time of the Job.
 */
public class JobsDetailsView extends CommonTableContainerView {

    private CCPageTitleModel pageTitleModel;
    private CCPropertySheetModel propertySheetModel;

    // jobType can be Archive, Release, Media Mount Request,
    // Recycle, SAM-FS Dump, Roll over of Log File
    private String jobType;
    private String jobCondition;
    private String jobIDString;
    // unique identification number for the job
    private long jobId;

    // the job being displayed
    private BaseJob jobDisplay;

    // Archive specific instance variables
    private ArchiveJob archiveJob;

    private ArchiveCopy archiveCopy;

    private FileSystem fileSystem;

    private ArchivePolicy archivePolicy;

    private String serverName;


    public JobsDetailsView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        serverName = ((CommonViewBeanBase) getParentViewBean()).getServerName();

        TraceUtil.trace3("Got serverName from page session: " + serverName);

        pageTitleModel = createPageTitleModel();
        createPropertySheetModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Creating child " + name);
        if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(
                propertySheetModel, name)) {
            return PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else {
            return super.createChild(name);
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                    "/jsp/jobs/JobsDetailsPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    /**
     * Depending on the type of Job, and the time of the job,
     * different property sheet models are created.
     */
    private void createPropertySheetModel() {

        TraceUtil.trace3("Entering");
        String propFile = null;

        ViewBean vb = getParentViewBean();
        String jobIdString = (String)
        vb.getPageSessionAttribute(Constants.PageSessionAttributes.JOB_ID);

        TraceUtil.trace3("jobIdString is " + jobIdString);

        // break up the Job-id into three parts: jobid, type, condition
        String [] jobIdSplit = jobIdString.split(",");

        jobIDString = jobIdSplit[0];
        jobId = Long.parseLong(jobIDString);
        jobType = jobIdSplit[1];
        jobCondition = jobIdSplit[2];

        String propertySheetXML;
        // different types of Property Sheets
        if (jobType.equals("Jobs.jobType1")) {
            if (jobCondition.equals("Current")) {
                propertySheetModel = PropertySheetUtil.createModel(
                        "/jsp/jobs/ArchiveJobsCurrentPropSheet.xml");
            } else if (jobCondition.equals("Pending")) {
                propertySheetModel = PropertySheetUtil.createModel(
                        "/jsp/jobs/ArchiveJobsPendingPropSheet.xml");
            } else if (jobCondition.equals("History")) {
                propertySheetModel = PropertySheetUtil.createModel(
                        "/jsp/jobs/ArchiveJobsHistoricalPropSheet.xml");
            }
        } else if (jobType.equals("Jobs.jobType5")) {
            if (jobCondition.equals("Current")) {
                propertySheetModel = PropertySheetUtil.createModel(
                        "/jsp/jobs/ArchiveJobsCurrentScanningPropSheet.xml");
                CurrentScanningModel currentScanningInfo =
                        new CurrentScanningModel(serverName, jobId);
                // set the model
                propertySheetModel.setModel(
                        "currentScanningInfo", currentScanningInfo);
            } else if (jobCondition.equals("Pending")) {
                propertySheetModel = PropertySheetUtil.createModel(
                        "/jsp/jobs/ArchiveJobsPendingScanningPropSheet.xml");
                PendingScanningModel pendingScanningInfo =
                        new PendingScanningModel(serverName, jobId);
                // set the model
                propertySheetModel.setModel(
                        "pendingScanningInfo", pendingScanningInfo);
            }
        } else if (jobType.equals("Jobs.jobType3")) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/ReleaseJobsPropSheet.xml");
        } else if (jobType.equals("Jobs.jobType4")) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/MediaMountJobsPropSheet.xml");
        } else if (jobType.equals("Jobs.jobType6")) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/SamfsckJobsPropSheet.xml");
        } else if (jobType.equals("Jobs.jobType7")) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/TapeLabelJobsPropSheet.xml");
        } else if (jobType.equals(BaseJob.TYPE_RESTORE_STR)) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/RestoreJobsPropSheet.xml");
        } else if (jobType.equals(BaseJob.TYPE_RESTORE_SEARCH_STR)) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/RestoreSearchJobsPropSheet.xml");
        } else if (jobType.equals("Jobs.jobType.dump")) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/DumpJobsPropSheet.xml");
        } else if (jobType.equals(BaseJob.TYPE_ENABLE_DUMP_STR)) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/EnableDumpJobsPropSheet.xml");
        } else if (jobType.equals(BaseJob.TYPE_ARCHIVE_FILES_STR) ||
                   jobType.equals(BaseJob.TYPE_RELEASE_FILES_STR) ||
                   jobType.equals(BaseJob.TYPE_STAGE_FILES_STR) ||
                   jobType.equals(BaseJob.TYPE_RUN_EXPLORER_STR)) {
            propertySheetModel = PropertySheetUtil.createModel(
                    "/jsp/jobs/GenericJobsPropSheet.xml");
        }

        TraceUtil.trace3("Exiting");
    }


    public void loadArchiveCurrentPropSheet() throws SamFSException {

        TraceUtil.trace3("Entering");

        getModelInfo();
        loadGenericArchiveValues();

        propertySheetModel.setValue("initTimeText",
                SamUtil.getTimeString(archiveJob.getStartDateTime()));

        propertySheetModel.setValue("totFilesCopiedText",
                Integer.toString(archiveJob.getTotalNoOfFilesToBeCopied()));

        propertySheetModel.setValue("volumeDataText",
                Long.toString(archiveJob.getDataVolumeToBeCopied()));

        TraceUtil.trace3("Exiting");
    }

    public void loadArchiveCurrentScanningPropSheet(long jobId)
            throws SamFSException {

        TraceUtil.trace3("Entering");
        CurrentScanningModel currentScanningInfo = (CurrentScanningModel)
        propertySheetModel.getModel("currentScanningInfo");
        currentScanningInfo.initModelRows();
        TraceUtil.trace3("Exiting");
    }

    public void loadArchivePendingScanningPropSheet(long jobId)
            throws SamFSException {

        TraceUtil.trace3("Entering");
        PendingScanningModel pendingScanningInfo = (PendingScanningModel)
        propertySheetModel.getModel("pendingScanningInfo");
        pendingScanningInfo.initModelRows();
        TraceUtil.trace3("Exiting");
    }

    public void loadArchivePendingPropSheet() throws SamFSException {

        TraceUtil.trace3("Entering");

        getModelInfo();
        loadGenericArchiveValues();
        propertySheetModel.setValue("initTimeText",
                SamUtil.getTimeString(archiveJob.getStartDateTime()));

        propertySheetModel.setValue("totFilesCopiedText",
                Integer.toString(archiveJob.getTotalNoOfFilesToBeCopied()));

        propertySheetModel.setValue("volumeDataText",
                Long.toString(archiveJob.getDataVolumeToBeCopied()));

        TraceUtil.trace3("Exiting");
    }

    public void loadGenericArchiveValues() throws SamFSException {
        TraceUtil.trace3("Entering");
        archiveJob = (ArchiveJob) jobDisplay;

        propertySheetModel.setValue(
                "fsNameText", archiveJob.getFileSystemName());
        propertySheetModel.setValue(
                "policyNameText", archiveJob.getPolicyName());
        propertySheetModel.setValue("copyNumText",
                Integer.toString(archiveJob.getCopyNumber()));
        propertySheetModel.setValue("vsnText", archiveJob.getVSNName());
        propertySheetModel.setValue("medTypeText",
                SamUtil.getMediaTypeString(archiveJob.getMediaType()));
        TraceUtil.trace3("Exiting");
    }

    public void loadReleasePropSheet() throws SamFSException {
        TraceUtil.trace3("Entering");

        getModelInfo();
        ReleaseJob releaseJob = (ReleaseJob) jobDisplay;

        propertySheetModel.setValue(
                "fsNameText", releaseJob.getFileSystemName());
        propertySheetModel.setValue("releaseInitTimeText",
                SamUtil.getTimeString(releaseJob.getStartDateTime()));
        propertySheetModel.setValue("lwmText", releaseJob.getLWM());
        propertySheetModel.setValue(
                "diskConsumedText", releaseJob.getConsumedSpacePercentage());
        TraceUtil.trace3("Exiting");
    }

    public void loadMediaMountPropSheet() throws SamFSException {
        TraceUtil.trace3("Entering");

        getModelInfo();
        MountJob mountJob = (MountJob) jobDisplay;

        propertySheetModel.setValue(
            "vsnNameText", mountJob.getVSNName());
        propertySheetModel.setValue(
            "medTypeText", SamUtil.getMediaTypeString(mountJob.getMediaType()));
        propertySheetModel.setValue(
            "libNameText", mountJob.getLibraryName());
        propertySheetModel.setValue("archiveStageRelatedText",
            mountJob.isArchiveMount() ? "Jobs.jobType1" : "Jobs.jobType2");
        propertySheetModel.setValue(
            "processIdText", Long.toString(mountJob.getProcessId()));
        propertySheetModel.setValue(
            "initUserText", mountJob.getInitiatingUsername());
        propertySheetModel.setValue(
            "initTimeText", SamUtil.getTimeString(mountJob.getStartDateTime()));
        propertySheetModel.setValue(
            "timeQueueText", SamUtil.getTimeString(mountJob.getTimeInQueue()));
        TraceUtil.trace3("Exiting");
    }

    public void loadDumpPropSheet() throws SamFSException {
        TraceUtil.trace3("Entering");

        getModelInfo();
        FSDumpJob dumpJob = (FSDumpJob) jobDisplay;

        propertySheetModel.setValue(
            "startTimeText", SamUtil.getTimeString(dumpJob.getStartDateTime()));
        propertySheetModel.setValue(
            "fsNameText", dumpJob.getFileSystemName());
        propertySheetModel.setValue(
            "dumpFileText", dumpJob.getDumpFileName());

        TraceUtil.trace3("Exiting");
    }

    public void loadDumpEnablePropSheet() throws SamFSException {
        TraceUtil.trace3("Entering");

        getModelInfo();
        EnableDumpJob dumpEnableJob = (EnableDumpJob) jobDisplay;

        propertySheetModel.setValue(
                "startTimeText", SamUtil.getTimeString(
                dumpEnableJob.getStartDateTime()));
        propertySheetModel.setValue(
                "fsNameText", dumpEnableJob.getFileSystemName());
        propertySheetModel.setValue(
                "dumpFileNameText", dumpEnableJob.getDumpFileName());

        TraceUtil.trace3("Exiting");
    }

    public void loadSamfsckPropSheet() throws SamFSException {
        TraceUtil.trace3("Entering");

        getModelInfo();
        SamfsckJob job = (SamfsckJob) jobDisplay;

        propertySheetModel.setValue(
                "fsNameText", job.getFileSystemName());
        propertySheetModel.setValue(
                "initUserText", job.getInitiatingUser());
        propertySheetModel.setValue(
                "initTimeText", SamUtil.getTimeString(job.getStartDateTime()));
        TraceUtil.trace3("Exiting");
    }

    public void loadTapeLabelPropSheet() throws SamFSException {
        TraceUtil.trace3("Entering");

        getModelInfo();
        LabelJob job = (LabelJob) jobDisplay;

        propertySheetModel.setValue(
                "vsnText", job.getVSNName());
        propertySheetModel.setValue(
                "driveText", job.getDriveName());
        propertySheetModel.setValue(
                "libText", job.getLibraryName());
        TraceUtil.trace3("Exiting");
    }

    public void loadRestorePropSheet() throws SamFSException {

        TraceUtil.trace3("Entering");

        getModelInfo();
        RestoreJob job = (RestoreJob) jobDisplay;

        propertySheetModel.setValue("initTimeText",
                SamUtil.getTimeString(job.getStartDateTime()));
        propertySheetModel.setValue("fsNameText", job.getFileSystemName());
        propertySheetModel.setValue("snapshotNameText", job.getDumpFileName());
        // If the entire file system is being restored, the file will be "."
        // Instead of displaying ".", display a more informative string.
        if (job.getFileName().equals(".")) {
            propertySheetModel.setValue("fileNameText",
                    SamUtil.getResourceString(
                    "JobsDetails.restore.restoreFS"));
        } else {
            propertySheetModel.setValue("fileNameText", job.getFileName());
        }

        FsmVersion version45 = new FsmVersion("4.5", this.serverName);

        if (!version45.isAPICompatibleWithUI()) {
            propertySheetModel.setVisible("restoreToProp", false);
            propertySheetModel.setVisible("replaceTypeProp", false);
            propertySheetModel.setVisible("onlineStatusProp", false);
        } else {
            propertySheetModel.setValue(
                "restoreToText", job.getRestoreToPath());

            String restoreTypeStr;
            switch (job.getReplaceType()) {
                case SamQFSSystemFSManager.RESTORE_REPLACE_ALWAYS:
                    restoreTypeStr =
                        "FSRestore.restore.replaceType.replaceAlways";
                    break;

                case SamQFSSystemFSManager.RESTORE_REPLACE_WITH_NEWER:
                    restoreTypeStr =
                        "FSRestore.restore.replaceType.replaceWithNewer";
                    break;

                case SamQFSSystemFSManager.RESTORE_REPLACE_NEVER:
                default:
                    restoreTypeStr =
                        "FSRestore.restore.replaceType.replaceNever";
                    break;
            }
            propertySheetModel.setValue(
                "replaceTypeText",
                SamUtil.getResourceString(restoreTypeStr));

            String onlineStatusStr;
            int onlineStatus = job.getOnlineStatusAfterRestore();
            if (onlineStatus == RestoreFile.NO_STG_COPY) {
                onlineStatusStr =
                    "FSRestore.restore.stageOptionOffLine";
            } else if (onlineStatus == RestoreFile.SYS_STG_COPY) {
                onlineStatusStr =
                    "FSRestore.restore.stageOptionSystemPick";
            } else if (onlineStatus == RestoreFile.STG_COPY_ASINDUMP) {
                onlineStatusStr =
                    "FSRestore.restore.stageOptionAsInDump";
            } else if (onlineStatus >= 0 && onlineStatus <= 3) {
                // Archive copies
                onlineStatusStr =
                    "FSRestore.restore.stageOptionArchiveCopy.copy";
            } else {
                onlineStatusStr =
                    SamUtil.getResourceString(
                        "FSRestore.restore.error.invalidStageOption",
                        String.valueOf(onlineStatus));
            }
            propertySheetModel.setValue(
                "onlineStatusAfterRestoreText",
                SamUtil.getResourceString(onlineStatusStr));
        } // version45

        propertySheetModel.setValue(
            "statusText",
            job.getRestoreStatus(null));
        TraceUtil.trace3("Exiting");
    }

    public void loadRestoreSearchPropSheet() throws SamFSException {

        TraceUtil.trace3("Entering");

        getModelInfo();
        RestoreSearchJob job = (RestoreSearchJob) jobDisplay;

        propertySheetModel.setValue("initTimeText",
                SamUtil.getTimeString(job.getStartDateTime()));
        propertySheetModel.setValue("fsNameText", job.getFileSystemName());
        propertySheetModel.setValue("snapshotFileNameText",
                job.getDumpFileName());
        propertySheetModel.setValue("searchCriteriaText",
                job.getSearchCriteria());
        TraceUtil.trace3("Exiting");
    }

    public void loadGenericPropSheet() throws SamFSException {
        TraceUtil.trace3("Entering");

        getModelInfo();
        GenericJob genericJob = (GenericJob) jobDisplay;

        propertySheetModel.setValue(
            "startTimeText",
            SamUtil.getTimeString(genericJob.getStartDateTime()));
        propertySheetModel.setValue(
            "activityIDText", genericJob.getDescription());
        propertySheetModel.setValue(
            "pidText", Integer.toString(genericJob.getPID()));

        TraceUtil.trace3("Exiting");
    }

    private void getModelInfo() throws SamFSException {
        TraceUtil.trace3("Entering");
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        if (sysModel == null) {
            throw new SamFSException(null, -2001);
        }

        // get the job that will be displayed
        jobDisplay = sysModel.getSamQFSSystemJobManager().getJobById(jobId);
        if (jobDisplay == null) {
            throw new SamFSException(null, -2011);
        }
    }


    public void populateData() throws SamFSException {
        // different types of Property Sheets
        if (jobType.equals("Jobs.jobType1")) {
            if (jobCondition.equals("Current")) {
                loadArchiveCurrentPropSheet();
            } else if (jobCondition.equals("Pending")) {
                loadArchivePendingPropSheet();
            }
        } else if (jobType.equals("Jobs.jobType5")) {
            if (jobCondition.equals("Current")) {
                loadArchiveCurrentScanningPropSheet(jobId);
            } else if (jobCondition.equals("Pending")) {
                loadArchivePendingScanningPropSheet(jobId);
            }
        } else if (jobType.equals("Jobs.jobType3")) {
            loadReleasePropSheet();
        } else if (jobType.equals("Jobs.jobType4")) {
            loadMediaMountPropSheet();
        } else if (jobType.equals("Jobs.jobType6")) {
            loadSamfsckPropSheet();
        } else if (jobType.equals("Jobs.jobType7")) {
            loadTapeLabelPropSheet();
        } else if (jobType.equals(BaseJob.TYPE_RESTORE_STR)) {
            loadRestorePropSheet();
        } else if (jobType.equals(BaseJob.TYPE_RESTORE_SEARCH_STR)) {
            loadRestoreSearchPropSheet();
        } else if (jobType.equals("Jobs.jobType.dump")) {
            loadDumpPropSheet();
        } else if (jobType.equals(BaseJob.TYPE_ENABLE_DUMP_STR)) {
            loadDumpEnablePropSheet();
        } else if (jobType.equals(BaseJob.TYPE_ARCHIVE_FILES_STR) ||
                   jobType.equals(BaseJob.TYPE_RELEASE_FILES_STR) ||
                   jobType.equals(BaseJob.TYPE_STAGE_FILES_STR) ||
                   jobType.equals(BaseJob.TYPE_RUN_EXPLORER_STR)) {
            loadGenericPropSheet();
        }

        propertySheetModel.setValue("jobTypeText", jobType);
        // set the values in the correct property sheet
        // there's always going to be a Job-Id
        propertySheetModel.setValue("jobIdText", jobIDString);
    }

}
