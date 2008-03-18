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

// ident	$Id: FSDevicesViewBean.java,v 1.22 2008/03/17 14:43:33 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicyDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicySummaryViewBean;
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

/**
 * ViewBean used to display the 'File System Archive Policy' page
 */

public class FSDevicesViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "FSDevices";
    private static final String DEFAULT_DISPLAY_URL = "/jsp/fs/FSDevices.jsp";

    // Used for constructing the Action Table
    private static final String CHILD_CONTAINER_VIEW = "FSDevicesView";

    public static final String CHILD_BREADCRUMB  = "BreadCrumb";
    public static final String CHILD_FS_SUM_HREF = "FileSystemSummaryHref";
    public static final String CHILD_FS_DET_HREF = "FileSystemDetailsHref";
    public static final String CHILD_SHARED_FS_HREF   = "SharedFSDetailsHref";

    // handler for archive pages (v44)
    public static final String POLICY_SUMMARY_HREF   = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF   = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";

    /**
     * Constructor
     */
    public FSDevicesViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_CONTAINER_VIEW, FSDevicesView.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_FS_SUM_HREF, CCHref.class);
        registerChild(CHILD_FS_DET_HREF, CCHref.class);
        registerChild(CHILD_SHARED_FS_HREF, CCHref.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create child component
     * @param child name: name
     * @return child component
     */
    protected View createChild(String name) {

        TraceUtil.trace3("Entering");
        if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            TraceUtil.trace3("Exiting");
            return new FSDevicesView(this, name);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            CCBreadCrumbsModel model =
                new CCBreadCrumbsModel("FSDevices.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, model);
            CCBreadCrumbs child = new CCBreadCrumbs(this, model, name);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_FS_SUM_HREF) ||
                   name.equals(CHILD_FS_DET_HREF) ||
                   name.equals(CHILD_SHARED_FS_HREF) ||
                   name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        FSDevicesView view = (FSDevicesView) getChild(CHILD_CONTAINER_VIEW);

        try {
            view.populateData();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "FSDevicesViewBean()",
                "Unable to populate device table",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSDevices.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for backto FS href
     * @param event RequestInvocationEvent event
     */
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        String s = (String) getDisplayFieldValue(CHILD_FS_SUM_HREF);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
        removePageSessionAttribute(
            Constants.SessionAttributes.POLICY_NAME);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for backto detail href
     * @param event RequestInvocationEvent event
     */
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_FS_DET_HREF);
        ViewBean targetView = getViewBean(FSDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for backto detail href
     * @param event RequestInvocationEvent event
     */
    public void handleSharedFSDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_SHARED_FS_HREF);
        ViewBean targetView = getViewBean(SharedFSDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    // handle breadcrumb to the policy summary page
    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_SUMMARY_HREF);
        ViewBean target = getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the policy details summary page
    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_DETAILS_HREF);
        ViewBean target = getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the criteria details summary page
    public void handleCriteriaDetailsHrefRequest(
        RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(CRITERIA_DETAILS_HREF);
        ViewBean target = getViewBean(CriteriaDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }
}
