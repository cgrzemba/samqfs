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

// ident	$Id: StageJobsViewBean.java,v 1.11 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.jobs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCHref;
import java.io.IOException;
import javax.servlet.ServletException;

// TODO: This page is no longer in used.  Do we need to remove this page?

public class StageJobsViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "StageJobs";
    private static final String
        DEFAULT_DISPLAY_URL = "/jsp/jobs/StageJobs.jsp";


    private CCBreadCrumbsModel breadCrumbsModel;

    private final String CHILD_BREADCRUMB = "BreadCrumb";

    public static final String CHILD_CONTAINER_VIEW = "StageJobsView";

    public static final String CHILD_CURRENT_HREF = "CurrentJobsHref";
    public static final String CHILD_PENDING_HREF = "PendingJobsHref";
    public static final String CHILD_ALL_HREF = "AllJobsHref";

    /**
     * Constructor
     */
    public StageJobsViewBean() {
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
        registerChild(CHILD_CONTAINER_VIEW, StageJobsView.class);
        registerChild(CHILD_CURRENT_HREF, CCHref.class);
        registerChild(CHILD_PENDING_HREF, CCHref.class);
        registerChild(CHILD_ALL_HREF, CCHref.class);
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
	    // PageTitle Child
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel = new CCBreadCrumbsModel("JobsDetails.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            CCBreadCrumbs child =
                new CCBreadCrumbs(this, breadCrumbsModel, name);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            TraceUtil.trace3("Exiting");
            return new StageJobsView(this, name);
        } else if (name.equals(CHILD_CURRENT_HREF) ||
            name.equals(CHILD_PENDING_HREF) ||
            name.equals(CHILD_ALL_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {

        TraceUtil.trace3("Entering");
        StageJobsView view = (StageJobsView) getChild(CHILD_CONTAINER_VIEW);
        String serverName = (String) getPageSessionAttribute(
			Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        TraceUtil.trace3("Got serverName from page session: " + serverName);

        try {
            view.populateData();
        }
        catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "StageJobsViewBean()",
                "Cannot populate stage jobs",
                serverName);

            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "JobsDetails.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    public void handleCurrentJobsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("CurrentJobsHref");
        ViewBean targetView = getViewBean(CurrentJobsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            targetView,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handlePendingJobsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("PendingJobsHref");
        ViewBean targetView = getViewBean(PendingJobsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            targetView,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleAllJobsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("AllJobsHref");
        ViewBean targetView = getViewBean(AllJobsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            targetView,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }
}
