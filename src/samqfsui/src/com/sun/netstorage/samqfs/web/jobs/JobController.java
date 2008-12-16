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

// ident        $Id: JobController.java,v 1.6 2008/12/16 00:12:13 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.data.provider.FieldKey;
import com.sun.data.provider.RowKey;
import com.sun.data.provider.TableDataProvider;
import com.sun.data.provider.impl.ObjectArrayDataProvider;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemJobManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.GenericJob;
import com.sun.netstorage.samqfs.web.model.job.MountJob;
import com.sun.netstorage.samqfs.web.model.job.ReleaseJob;
import com.sun.netstorage.samqfs.web.model.job.LabelJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreJob;
import com.sun.netstorage.samqfs.web.model.job.RestoreSearchJob;
import com.sun.netstorage.samqfs.web.model.job.FSDumpJob;
import com.sun.netstorage.samqfs.web.model.job.EnableDumpJob;
import com.sun.netstorage.samqfs.web.model.job.SamfsckJob;
import com.sun.netstorage.samqfs.web.model.job.StageJob;
import com.sun.netstorage.samqfs.web.model.job.StageJobFileData;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.Select;
import com.sun.web.ui.component.TableRowGroup;
import javax.faces.context.FacesContext;
import javax.faces.el.ValueBinding;

public class JobController {
    private TableDataProvider dataProvider = null;
    private TableRowGroup rowGroup = null;
    private Select select = new Select("jobs");
    private long selectedJobId = -1;

    // used by job details page. cache the job object so that we don't have to
    // retrieve it again
    private BaseJob selectedJob = null;
    private TableDataProvider scanJobDataProvider = null;
    private TableDataProvider stageJobFileDataProvider = null;

    public JobController() {
    }

    private void initTableDataProvider() {
        String serverName = JSFUtil.getServerName();
        if (serverName != null) {
            try {
                SamQFSSystemModel model = SamUtil.getModel(serverName);
                if (model == null)
                    throw new SamFSException("System model is null");

                SamQFSSystemJobManager jobManager =
                    model.getSamQFSSystemJobManager();
                if (jobManager == null)
                    throw new SamFSException("job manager is null");

                BaseJob [] job = jobManager.getAllJobs();
                if (job != null) {
                    this.dataProvider = new ObjectArrayDataProvider(job);
                }
            } catch (SamFSException sfe) {
            }
        } // else we are in big trouble.
    }

    public TableDataProvider getJobList() {
        if (this.dataProvider ==  null) {
            initTableDataProvider();
        }

        return this.dataProvider;
    }

    // selection handlers
    public TableRowGroup getTableRowGroup() {
        return this.rowGroup;
    }

    public void setTableRowGroup(TableRowGroup rowGroup) {
        this.rowGroup = rowGroup;
    }

    public Select getSelect() {
        return this.select;
    }

    public void setSelect(Select sel) {
        this.select = sel;
    }

    protected Long getSelectedJobId() {
        Long jobId = null;
        TableDataProvider provider = getJobList();
        RowKey [] rows = getTableRowGroup().getSelectedRowKeys();
        if (rows != null && rows.length == 1) {
            FieldKey field = provider.getFieldKey("jobId");

            jobId = (Long)provider.getValue(field, rows[0]);
        }

        // safe to clear the selection
        getSelect().clear();
        return jobId;
    }

    // event handlers
    public String cancelJob() {
        System.out.println("Processing 'cancelJob()'");
        Long id = getSelectedJobId();

        System.out.println("cancel job : " + id);
        try {
            // First check if user has permission
            if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.SAM_CONTROL)) {
System.out.println("No permission!");
                throw new SamFSException(
                    JSFUtil.getMessage("common.nopermission"));
            }

            String serverName = JSFUtil.getServerName();
            SamQFSSystemModel model = SamUtil.getModel(serverName);
            if (model == null)
                throw new SamFSException("system model is null");

            SamQFSSystemJobManager jobManager =
                model.getSamQFSSystemJobManager();
            if (jobManager == null)
                throw new SamFSException("job manager is null");

            BaseJob theJob = jobManager.getJobById(selectedJobId);
            jobManager.cancelJob(theJob);
        } catch (SamFSException sfe) {
            System.out.println("Exception caught! " + sfe.getMessage());
            // update alert message
        }
        return "redisplay";
    }

    public String gotoJob() {
        System.out.println("Processing 'gotoJob()'");
        Long id = getSelectedJobId();

        System.out.println("go to job : " + id);
        this.selectedJobId = id.longValue();

        int type = getJob(this.selectedJobId).getType();

        System.out.println("job type = " + type);
        return getDetailsPage(type);
    }

    public String handleJobHref() {
        System.out.println("Processing 'handleJobHref()'");
        FacesContext context = FacesContext.getCurrentInstance();
        ValueBinding binding = context.getApplication()
            .createValueBinding("#{jobs.value.jobId}");

        Long id = (Long)binding.getValue(context);
        System.out.println("selected job id = " + id);

        this.selectedJobId = id.longValue();

        int type = getJob(this.selectedJobId).getType();
        System.out.println("job type = " + type);

        String pageCode = getDetailsPage(type);
        System.out.println("job details code = " + pageCode);

        return pageCode;
        // getDetailsPage(type);
    }

    // TODO: utility class candidate
    public String getDetailsPage(int jobType) {
        String code = null;
        switch (jobType) {
        case BaseJob.TYPE_ARCHIVE_COPY:
            code = "archivecopy";
            break;
        case BaseJob.TYPE_ARCHIVE_SCAN:
            code = "archivescan";
            break;
        case BaseJob.TYPE_RELEASE:
            code = "release";
            break;
        case BaseJob.TYPE_MOUNT: // TODO: this will need to change because of
                                 // mount clients
            code = "mountmedia";
            break;
        case BaseJob.TYPE_FSCK:
            code = "samfsck";
            break;
        case BaseJob.TYPE_TPLABEL:
            code = "labeltape";
            break;
        case BaseJob.TYPE_RESTORE:
            code = "restore";
            break;
        case BaseJob.TYPE_RESTORE_SEARCH:
            code = "restoresearch";
            break;
        case BaseJob.TYPE_DUMP:
            code = "recoverypoint";
            break;
        case BaseJob.TYPE_ENABLE_DUMP:
            code = "enablerecoverypoint";
            break;
        case BaseJob.TYPE_ARCHIVE_FILES:
        case BaseJob.TYPE_RELEASE_FILES:
        case BaseJob.TYPE_STAGE_FILES:
        case BaseJob.TYPE_RUN_EXPLORER:
            code = "generic";
            break;
        default:
            code = "badtype";
        }

        return code;
    }

    // job details page
    /**
     * returns the job object of the job selected in the summary page
     */
    public BaseJob getJob() {
        return getJob(this.selectedJobId);
    }

    public BaseJob getJob(long jobId) {
        if (this.selectedJob == null) {
            System.out.println("job details selected job = " + selectedJobId);
            String serverName = JSFUtil.getServerName();

            try {
                SamQFSSystemModel model = SamUtil.getModel(serverName);
                if (model == null)
                    throw new SamFSException("system model is null");

                SamQFSSystemJobManager jobManager =
                    model.getSamQFSSystemJobManager();
                if (jobManager == null)
                    throw new SamFSException("job manager is null");

                this.selectedJob = jobManager.getJobById(jobId);
                if (this.selectedJob == null)
                    throw new SamFSException("job not found");
            } catch (SamFSException sfe) {
                // TODO: // exception processing
            }
        }

        return this.selectedJob;
    }

    // return the selected job as its specific type for jsp rendering.
    // there will be one of these for every job type.
    public ArchiveJob getArchiveCopyJob() {
        return (ArchiveJob)this.selectedJob;
    }

    public ArchiveScanJob getArchiveScanJob() {
        return (ArchiveScanJob)this.selectedJob;
    }

    public GenericJob getGenericJob() {
        return (GenericJob)this.selectedJob;
    }

    public ReleaseJob getReleaseJob() {
        return (ReleaseJob)this.selectedJob;
    }

    public MountJob getMediaMountJob() {
        return (MountJob)this.selectedJob;
    }

    public SamfsckJob getSamfsckJob() {
        return (SamfsckJob)this.selectedJob;
    }

    public LabelJob getLabelTapeJob() {
        return (LabelJob)this.selectedJob;
    }

    public RestoreJob getRestoreJob() {
        return (RestoreJob)this.selectedJob;
    }

    public RestoreSearchJob getRestoreSearchJob() {
        return (RestoreSearchJob)this.selectedJob;
    }

    public FSDumpJob getRecoveryPointJob() {
        return (FSDumpJob)this.selectedJob;
    }

    public EnableDumpJob getEnableRecoveryPointJob() {
        return (EnableDumpJob)this.selectedJob;
    }

    public StageJob getStageJob() {
        return (StageJob)this.selectedJob;
    }

    // TODO: since this is a read only table, populating the data provider can
    // probably be deffered until the display response phase.
    public TableDataProvider getScanJobData() {
        if (this.scanJobDataProvider == null) {
            UIArchiveScanJobData [] data = new UIArchiveScanJobData[9];

            ArchiveScanJob scanJob = getArchiveScanJob();
            if (scanJob != null) {
                data[0] = new UIArchiveScanJobData(scanJob.getRegularFiles(),
                    JSFUtil.getMessage("job.details.scan.table.regularfiles"));

                data[1] = new UIArchiveScanJobData(scanJob.getOfflineFiles(),
                    JSFUtil.getMessage("job.details.scan.table.offlinefiles"));

                data[2] = new UIArchiveScanJobData(scanJob.getArchDoneFiles(),
                    JSFUtil.getMessage("job.details.scan.table.completed"));

                data[3] = new UIArchiveScanJobData(scanJob.getCopy1(),
                    JSFUtil.getMessage("job.details.scan.table.copy1"));

                data[4] = new UIArchiveScanJobData(scanJob.getCopy2(),
                    JSFUtil.getMessage("job.details.scan.table.copy2"));

                data[5] = new UIArchiveScanJobData(scanJob.getCopy3(),
                    JSFUtil.getMessage("job.details.scan.table.copy3"));

                data[6] = new UIArchiveScanJobData(scanJob.getCopy4(),
                    JSFUtil.getMessage("job.details.scan.table.copy4"));

                data[7] = new UIArchiveScanJobData(scanJob.getDirectories(),
                    JSFUtil.getMessage("job.details.scan.dir"));

                data[8] = new UIArchiveScanJobData(scanJob.getTotal(),
                    JSFUtil.getMessage("job.details.total"));
            }

            this.scanJobDataProvider = new ObjectArrayDataProvider(data);
        }

        return this.scanJobDataProvider;
    }


    public TableDataProvider getStageJobFileData() {
        if (this.stageJobFileDataProvider == null) {

            StageJob theJob = getStageJob();
            if (theJob != null) {

                try {
                    StageJobFileData [] data =
                        theJob.getFileData(0, 100, (short)1, true);
                    this.stageJobFileDataProvider =
                        new ObjectArrayDataProvider(data);
                } catch (SamFSException sfe) {
                    // TODO: log & alert
                }
            } // if the job
        }

        return this.stageJobFileDataProvider;
    }
}
