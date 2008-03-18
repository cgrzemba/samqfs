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

// ident	$Id: JobsDetailsViewBean.java,v 1.29 2008/03/17 14:43:37 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.fs.RecoveryPointsViewBean;
import com.sun.netstorage.samqfs.web.media.LibraryDriveSummaryViewBean;
import com.sun.netstorage.samqfs.web.media.LibrarySummaryViewBean;
import com.sun.netstorage.samqfs.web.media.VSNDetailsViewBean;
import com.sun.netstorage.samqfs.web.media.VSNSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.job.ArchiveJob;
import com.sun.netstorage.samqfs.web.model.job.BaseJob;
import com.sun.netstorage.samqfs.web.model.job.MountJob;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCOption;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Creates the 'Job Details Page'. This ViewBean is dynamic in nature in
 * that it creates multiple types of property sheets depending on the
 * type and time of the Job.
 */
public class JobsDetailsViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "JobsDetails";
    private static final String
        DEFAULT_DISPLAY_URL = "/jsp/jobs/JobsDetails.jsp";

    // jobType can be Archive, Release, Media Mount Request,
    // Recycle, SAM-FS Dump, Roll over of Log File
    private String jobType;

    // jobTime can be Current, Pending, History, or All
    private String jobTime;

    // unique identification number for the job
    private long jobId;

    // the job being displayed
    private BaseJob jobDisplay;

    // Archive specific instance variables
    private ArchiveJob archiveJob;

    private ArchiveCopy archiveCopy;

    private FileSystem fileSystem;

    private ArchivePolicy archivePolicy;

    private CCBreadCrumbsModel breadCrumbsModel;

    // Component variables
    public static final String CHILD_BREADCRUMB   = "BreadCrumb";
    public static final String CHILD_CURRENT_HREF = "CurrentJobsHref";
    public static final String CHILD_PENDING_HREF = "PendingJobsHref";
    public static final String CHILD_ALL_HREF = "AllJobsHref";

    // These are used for breadcrumbs when arriving via alert links
    public static final String CHILD_LIBSUMMARY_HREF = "LibrarySummaryHref";
    public static final
        String LIBRARY_DRIVE_SUMMARY_HREF = "LibraryDriveSummaryHref";
    public static final String CHILD_VSNSUMMARY_HREF = "VSNSummaryHref";
    public static final String CHILD_VSNDETAILS_HREF = "VSNDetailsHref";
    public static final String CHILD_FSSUMMARY_HREF  = "FileSystemSummaryHref";
    public static final String CHILD_FSDETAILS_HREF  = "FileSystemDetailsHref";
    public static final String RECOVERY_POINTS_HREF = "RecoveryPointsHref";

    public static final String CHILD_CONTAINER_VIEW = "JobsDetailsView";

    // For auto-refresh
    public static final String CHILD_DROPDOWN = "RefreshMenu";
    public static final String CHILD_REFRESH_RATE = "RefreshRateHiddenField";
    public static final String CHILD_REFRESH_LABEL = "RefreshLabel";
    public static final String CHILD_REFRESH_MENU_HREF = "RefreshMenuHref";
    public static final String CHILD_REFRESH_SUBMIT_HREF = "RefreshSubmitHref";


    public JobsDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
    	TraceUtil.trace3("Entering");
    	super.registerChildren();
    	registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_CONTAINER_VIEW, JobsDetailsView.class);
        registerChild(CHILD_CURRENT_HREF, CCHref.class);
        registerChild(CHILD_PENDING_HREF, CCHref.class);
        registerChild(CHILD_ALL_HREF, CCHref.class);
        registerChild(CHILD_LIBSUMMARY_HREF, CCHref.class);
        registerChild(LIBRARY_DRIVE_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_VSNSUMMARY_HREF, CCHref.class);
        registerChild(CHILD_VSNDETAILS_HREF, CCHref.class);
        registerChild(CHILD_FSSUMMARY_HREF, CCHref.class);
        registerChild(CHILD_FSDETAILS_HREF, CCHref.class);
        registerChild(RECOVERY_POINTS_HREF, CCHref.class);
        registerChild(CHILD_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_REFRESH_RATE, CCHiddenField.class);
        registerChild(CHILD_REFRESH_LABEL, CCLabel.class);
        registerChild(CHILD_REFRESH_MENU_HREF, CCHref.class);
        registerChild(CHILD_REFRESH_SUBMIT_HREF, CCHref.class);
    	TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
    	TraceUtil.trace3("Entering");
    	TraceUtil.trace3("Creating child " + name);
    	if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel = new CCBreadCrumbsModel("JobsDetails.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            CCBreadCrumbs child =
                new CCBreadCrumbs(this, breadCrumbsModel, name);
            TraceUtil.trace3("Exiting");
            return child;
    	} else if (name.equals(CHILD_CONTAINER_VIEW)) {
            TraceUtil.trace3("Exiting");
            return new JobsDetailsView(this, name);
        } else if (name.equals(CHILD_CURRENT_HREF) ||
                   name.equals(CHILD_PENDING_HREF) ||
                   name.equals(CHILD_ALL_HREF) ||
                   name.equals(CHILD_FSSUMMARY_HREF) ||
                   name.equals(CHILD_FSDETAILS_HREF) ||
                   name.equals(CHILD_VSNSUMMARY_HREF) ||
                   name.equals(CHILD_VSNDETAILS_HREF) ||
                   name.equals(CHILD_LIBSUMMARY_HREF) ||
                   name.equals(LIBRARY_DRIVE_SUMMARY_HREF) ||
                   name.equals(CHILD_REFRESH_MENU_HREF) ||
                   name.equals(CHILD_REFRESH_SUBMIT_HREF) ||
                   name.equals(RECOVERY_POINTS_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            // Set child options and "none selected" label.
            myChild.setOptions(createOptionList());
            return (View) myChild;
        // Refresh Rate Hidden Field
	} else if (name.equals(CHILD_REFRESH_RATE)) {
            return new CCHiddenField(this, name, null);
	// Refresh Label
	} else if (name.equals(CHILD_REFRESH_LABEL)) {
            return new CCLabel(this, name, null);

    	} else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    public void handleLibNameHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        MountJob mountJob = (MountJob) jobDisplay;
        TraceUtil.trace3("mountJob is " + mountJob);
        String libName = "";
        try {
            libName = mountJob.getLibraryName();
        }
        catch (SamFSException smfex) {
            // should not get here!
        }

        ViewBean targetView = getViewBean(LibraryDriveSummaryViewBean.class);
        targetView.setPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME, libName);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleCurrentJobsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("CurrentJobsHref");
        ViewBean targetView = getViewBean(CurrentJobsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handlePendingJobsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("PendingJobsHref");
        ViewBean targetView = getViewBean(PendingJobsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleAllJobsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("AllJobsHref");
        ViewBean targetView = getViewBean(AllJobsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_LIBSUMMARY_HREF);
        ViewBean targetView = getViewBean(LibrarySummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleLibraryDriveSummaryHrefRequest(
        RequestInvocationEvent event) throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(LIBRARY_DRIVE_SUMMARY_HREF);
        ViewBean targetView = getViewBean(LibraryDriveSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_VSNSUMMARY_HREF);
        ViewBean targetView = getViewBean(VSNSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleVSNDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_VSNDETAILS_HREF);
        ViewBean targetView = getViewBean(VSNDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_FSSUMMARY_HREF);
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_FSDETAILS_HREF);
        ViewBean targetView = getViewBean(FSDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleRecoveryPointsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(RECOVERY_POINTS_HREF);
        CommonViewBeanBase targetView =
                (CommonViewBeanBase) getViewBean(RecoveryPointsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);

        removePageSessionAttribute(Constants.PageSessionAttributes.JOB_ID);

        // Forward with all page session attributes because the were passed in
        // when forwarding to this page and we want to restore page status.
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        String serverName = getServerName();

        try {
            // populate the AT's model data here, for error handling
            JobsDetailsView view =
                (JobsDetailsView) getChild(CHILD_CONTAINER_VIEW);
            view.populateData();
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginDisplay",
                "Cannot populate job details",
                serverName);

            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "JobsDetails.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }
    }

    private OptionList createOptionList() {
	TraceUtil.trace3("Entering");

        // Options.
	CCOption[] options = new CCOption[] {
	// Params: label, value, enabled title, disabled title.
            new CCOption(
		"Off", "10000",
		"Turn auto-refresh to Off", "Off"),
            new CCOption(
		"15 seconds", "15",
		"Set auto-fresh to 15 seconds", "15"),
            new CCOption(
		"30 seconds", "30",
		"Set auto-refresh to 30 seconds", "30"),
            new CCOption(
		"1 minute", "60",
		"Set auto-refresh to 1 minute", "60"),
            new CCOption(
		"2 minutes", "120",
		"Set auto-refresh to 2 minutes", "120"),
            new CCOption(
		"5 minutes", "300",
		"Set auto-refresh to 5 minutes", "300")};

	OptionList optionList = new OptionList(options);

	TraceUtil.trace3("Exiting");
	return optionList;
    }

    public void handleRefreshMenuHrefRequest(RequestInvocationEvent event)
	throws ServletException, IOException {
	handleForward(event);
    }

    public void handleRefreshSubmitHrefRequest(RequestInvocationEvent event)
	throws ServletException, IOException {
	handleForward(event);
    }

    private void handleForward(RequestInvocationEvent event)
	throws ServletException, IOException {
	String refreshRate = (String) getDisplayFieldValue(CHILD_DROPDOWN);

	((CCHiddenField)
            getChild(CHILD_REFRESH_RATE)).setValue(refreshRate);
	this.forwardTo(getRequestContext());
    }
}
