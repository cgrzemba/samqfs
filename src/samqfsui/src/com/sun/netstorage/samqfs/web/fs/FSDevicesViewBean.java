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

// ident	$Id: FSDevicesViewBean.java,v 1.27 2008/12/16 00:12:10 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicyDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicySummaryViewBean;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
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
    public static final String SHARED_FS_SUMMARY_HREF = "SharedFSSummaryHref";

    // handler for archive pages (v44)
    public static final String POLICY_SUMMARY_HREF   = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF   = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    /**
     * Constructor
     */
    public FSDevicesViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();

        pageTitleModel = createPageTitleModel();
        registerChildren();
    }

    /**
     * Register each child
     */
    protected void registerChildren() {
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        registerChild(CHILD_CONTAINER_VIEW, FSDevicesView.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_FS_SUM_HREF, CCHref.class);
        registerChild(SHARED_FS_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);

    }

    /**
     * Create child component
     * @param child name: name
     * @return child component
     */
    protected View createChild(String name) {
        if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            String serverName = (String) getPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
            if (serverName == null) {
                serverName = RequestManager.getRequest().getParameter(
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME,
                    serverName);
            }
            String fsName = (String) getPageSessionAttribute(
                Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
            if (fsName == null) {
                fsName = RequestManager.getRequest().getParameter(
                    Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
                setPageSessionAttribute(
                    Constants.PageSessionAttributes.FILE_SYSTEM_NAME,
                    fsName);
            }
            return new FSDevicesView(this, name);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            Integer [] pagePath = (Integer []) getPageSessionAttribute(
                                Constants.SessionAttributes.PAGE_PATH);
            if (pagePath == null) {
                String pathString = RequestManager.getRequest().getParameter(
                                        Constants.SessionAttributes.PAGE_PATH);
                pathString = pathString == null ? "" : pathString;
                String [] pathStringArr =
                    pathString == null ?
                        new String[0] :
                        pathString.split(",");
                Integer [] path = new Integer[pathStringArr.length];
                for (int i = 0; i < pathStringArr.length; i++) {
                    path[i] = new Integer(pathStringArr[i]);
                }

                setPageSessionAttribute(
                    Constants.SessionAttributes.PAGE_PATH,
                    path);
            }
            CCBreadCrumbsModel model =
                new CCBreadCrumbsModel("FSDevices.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, model);
            CCBreadCrumbs child = new CCBreadCrumbs(this, model, name);
            return child;
        } else if (name.equals(SHARED_FS_SUMMARY_HREF) ||
                   name.equals(CHILD_FS_SUM_HREF) ||
                   name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF)) {
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

        String fsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("FSDevices.pageTitle.fsname", fsName));
    }

    /**
     * Create PageTitle Model
     * @return PageTitleModel
     */
    private CCPageTitleModel createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel =
                PageTitleUtil.createModel("/jsp/fs/FSDevicesPageTitle.xml");
        }
        return pageTitleModel;
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

    // Handler to navigate back to Shared File System Summary Page
    public void handleSharedFSSummaryHrefRequest(
        RequestInvocationEvent evt)
        throws ServletException, IOException {

        String url = "/faces/jsp/fs/SharedFSSummary.jsp";

        TraceUtil.trace2("FSDevices: Navigate back to URL: " + url);

        String params =
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME
                 + "=" + getServerName()
                 + "&" + Constants.PageSessionAttributes.FILE_SYSTEM_NAME
                 + "=" + (String) getPageSessionAttribute(
                             Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
        JSFUtil.forwardToJSFPage(this, url, params);
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
