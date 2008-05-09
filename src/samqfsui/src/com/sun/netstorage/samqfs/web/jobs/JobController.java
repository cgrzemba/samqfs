/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident        $Id: JobController.java,v 1.1 2008/05/09 21:08:57 kilemba Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.sun.data.provider.FieldKey;
import com.sun.data.provider.RowKey;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.data.provider.TableDataProvider;
import com.sun.data.provider.impl.ObjectArrayDataProvider;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemJobManager;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJob;
import com.sun.netstorage.samqfs.web.model.job.ArchiveScanJobData;
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


    public JobController() {
    }

    private void initTableDataProvider() {
        String serverName = JSFUtil.getServerName();
        if (serverName != null){
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
            // update alert message
        }
        return "redisplay";
    }
    
    public String gotoJob() {
        System.out.println("Processing 'gotoJob()'");
        Long id = getSelectedJobId();

        System.out.println("go to job : " + id);
        this.selectedJobId = id.longValue();
        return "jobdetails";
    }
    
    public String handleJobHref() {
        System.out.println("Processing 'handleJobHref()'");
        FacesContext context = FacesContext.getCurrentInstance();
        ValueBinding binding = context.getApplication()
            .createValueBinding("#{jobs.value.jobId}");
        
        Long id = (Long)binding.getValue(context);
        System.out.println("selected job id = " + id);
        
        this.selectedJobId = id.longValue();
        return "jobdetails";
    }
    
    // job details page
    /**returns the job object of the job selected in the summary page */
    public BaseJob getJob() {
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
            
                this.selectedJob = jobManager.getJobById(selectedJobId);
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

    // TODO: since this is a read only table, populating the data provider can
    // probably be deffered until the display response phase.
    public TableDataProvider getScanJobData() {
        if (this.scanJobDataProvider == null) {
            ArchiveScanJobData [] data = new ArchiveScanJobData[9];

            ArchiveScanJob scanJob = getArchiveScanJob();
            if (scanJob != null) {
                data[0] = scanJob.getRegularFiles();
                data[1] = scanJob.getOfflineFiles();
                data[2] = scanJob.getArchDoneFiles();
                data[3] = scanJob.getCopy1();
                data[4] = scanJob.getCopy2();
                data[5] = scanJob.getCopy3();
                data[6] = scanJob.getCopy4();
                data[7] = scanJob.getDirectories();
                data[8] = scanJob.getTotal();
            }

            this.scanJobDataProvider = new ObjectArrayDataProvider(data);
        }

        return this.scanJobDataProvider;
    }

    public String getJobDetailsPageFragment() {
        System.out.println("Entered getJobDetailsPageFragment()");
        // String page = "ArchiveScanJobDetails.jsp";
        String page = null;

        switch (this.selectedJob.getType()) {
        case BaseJob.TYPE_ARCHIVE_SCAN:
            page = "ArchiveScanJobDetails.jsp";
            break;
        case BaseJob.TYPE_ARCHIVE_COPY:
            page = "ArchiveCopyJobDetails.jsp";
            break;
        default:
            // do nothing
        }

        System.out.println("getJobDetailsFragment() : " + page);
        return page;
    }
}
