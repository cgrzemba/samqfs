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

// ident	$Id: AdvancedNetworkConfigViewBean.java,v 1.7 2008/03/17 14:43:32 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;

import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;

import java.io.IOException;
import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Advanced Network Config page
 */

public class AdvancedNetworkConfigViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "AdvancedNetworkConfig";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/fs/AdvancedNetworkConfig.jsp";

    public static final String BREADCRUMB = "BreadCrumb";

    public static final String SHARED_FS_DETAILS_HREF = "SharedFSDetailsHref";
    public static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";

    // cc components from the corresponding jsp page(s)...
    public static final
        String CONTAINER_VIEW = "AdvancedNetworkConfigDisplayView";

    public static final String SERVER_NAME = "ServerName";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel  = null;

    // For BreadCrumb
    private CCBreadCrumbsModel breadCrumbsModel = null;

    /**
     * Constructor
     */
    public AdvancedNetworkConfigViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(SHARED_FS_DETAILS_HREF, CCHref.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(CONTAINER_VIEW, AdvancedNetworkConfigDisplayView.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
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

        View child = null;

        if (super.isChildSupported(name)) {
            child = super.createChild(name);

        // Action table Container.
        } else if (name.equals(CONTAINER_VIEW)) {
            child = new AdvancedNetworkConfigDisplayView(this, name);
        } else if (name.equals(BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("AdvancedNetworkConfig.browsertitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(SHARED_FS_DETAILS_HREF) ||
            name.equals(FS_SUMMARY_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            child = new CCHiddenField(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Create the page title model
     */
    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel =
                new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
        }
    }

    /**
     * Populate the page title model
     */
    private void loadPageTitleModel() {
        TraceUtil.trace3("Entering");
        pageTitleModel.clear();
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString(
                "AdvancedNetworkConfig.pagetitle", getFSName()));
        TraceUtil.trace3("Exiting");
    }

    /**
     * Breadcrumb link handler
     */
    public void handleSharedFSDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue("SharedFSDetailsHref");
        ViewBean targetView = getViewBean(SharedFSDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Breadcrumb link handler
     */
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue("FileSystemSummaryHref");
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        removePageSessionAttribute(Constants.PageSessionAttributes.FS_NAME);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // populate the page title
        loadPageTitleModel();

        // populate the action table model
        AdvancedNetworkConfigDisplayView view =
            (AdvancedNetworkConfigDisplayView) getChild(CONTAINER_VIEW);

        try {
            view.populateTableModels();

        } catch (SamFSMultiHostException multiEx) {
            String errMsg = SamUtil.handleMultiHostException(multiEx);
            TraceUtil.trace1("SamFSMultiHostException is thrown!");
            TraceUtil.trace1(
                "Error Code: " + multiEx.getSAMerrno() + " Reason: " + errMsg);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "AdvancedNetworkConfig.setup.error.execute",
                multiEx.getSAMerrno(),
                errMsg,
                getServerName());
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "beginDisplay()",
                "Failed to populate advanced network config settings",
                getServerName());
            if (samEx.getSAMerrno() != samEx.NOT_FOUND) {
                SamUtil.setErrorAlert(
                    this,
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "AdvancedNetworkConfig.display.error.populate",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            }
        }

        TraceUtil.trace3("Exiting");
    }

    private String getFSName() {
        return (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
    }
}
