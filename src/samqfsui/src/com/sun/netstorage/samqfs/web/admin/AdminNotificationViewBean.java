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

// ident	$Id: AdminNotificationViewBean.java,v 1.25 2008/05/16 19:39:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.fs.SharedFSDetailsViewBean;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;

/**
 *  This class is the view bean for the Admin Notification Summary page
 */

public class AdminNotificationViewBean extends CommonViewBeanBase {

    // Page information...
    private static final
        String DISPLAY_URL = "/jsp/admin/AdminNotification.jsp";
    private static final String PAGE_NAME = "AdminNotification";
    private static final String CHILD_BREADCRUMB  = "BreadCrumb";
    public static final String CHILD_FS_SUM_HREF = "FileSystemSummaryHref";
    public static final String CHILD_FS_DET_HREF = "FileSystemDetailsHref";
    public static final String CHILD_SHARED_FS_HREF = "SharedFSDetailsHref";
    // child that used in popup to pass server name
    public static final String SERVER_NAME = "ServerName";
    // child that used in javascript confirm messages
    public static final String CONFIRM_MESSAGE = "Messages";

    public static final String CONTAINER_VIEW = "AdminNotificationView";

    /**
     * Constructor
     * The view bean calls the RequestDispatcher's forward method passing in
     * the view beans's DISPLAY_URL. This passes control over to the
     * jsp to render its contents. the page title model is created and the
     * children are registered
     */
    public AdminNotificationViewBean() {
        super(PAGE_NAME, DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child with the parent view. This won't create the
     * child views, only metadata, containerView will create the child
     * views 'just in time' via call to createChild()
     */
    protected void registerChildren() {
        super.registerChildren();
        TraceUtil.trace3("Entering");
        registerChild(CONTAINER_VIEW, AdminNotificationView.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_FS_SUM_HREF, CCHref.class);
        registerChild(CHILD_FS_DET_HREF, CCHref.class);
        registerChild(CHILD_SHARED_FS_HREF, CCHref.class);
        registerChild(CONFIRM_MESSAGE, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Simplest implementation of createChild
     * Call constructor for each child view in multi-conditional statement block
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entering: child view name is ").append(name).toString());

        View child = null;

        if (super.isChildSupported(name)) {
            child = super.createChild(name);

        } else if (name.equals(CHILD_BREADCRUMB)) {
            CCBreadCrumbsModel model =
                new CCBreadCrumbsModel("emailAlert.title");
            BreadCrumbUtil.createBreadCrumbs(this, name, model);
            child = new CCBreadCrumbs(this, model, name);

        // Javascript used Hidden Field
        } else if (name.equals(CONFIRM_MESSAGE)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString(
                        "AdminNotification.confirmMsg1")).toString());

        } else if (name.equals(SERVER_NAME)) {
            child = new CCHiddenField(this, name, getServerName());

        } else if (name.equals(CONTAINER_VIEW)) {
            child = new AdminNotificationView(this, name);

        // Breadcrumb HREFs
        } else if (name.equals(CHILD_FS_SUM_HREF) ||
                   name.equals(CHILD_FS_DET_HREF) ||
                   name.equals(CHILD_SHARED_FS_HREF)) {
            child = new CCHref(this, name, null);

        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        try {
            AdminNotificationView view =
                (AdminNotificationView) getChild(CONTAINER_VIEW);
            view.populateDisplay();

        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "AdminNotificationViewBean()",
                "Failed to populate notifications",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "AdminNotification.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }

    public void handleFileSystemSummaryHrefRequest(
        RequestInvocationEvent event) {

        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        String s = (String) getDisplayFieldValue(CHILD_FS_SUM_HREF);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for back to FS detail (4.3) href
     * @param event RequestInvocationEvent event
     */
    public void handleSharedFSDetailsHrefRequest(RequestInvocationEvent event) {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_SHARED_FS_HREF);
        ViewBean targetView = getViewBean(SharedFSDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for backto detail page link
     * @param event RequestInvocationEvent event
     */
    public void handleFileSystemDetailsHrefRequest(
        RequestInvocationEvent event) {

        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(FSDetailsViewBean.class);
        String s = (String) getDisplayFieldValue(CHILD_FS_DET_HREF);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }
}
