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

// ident	$Id: FSDetailsViewBean.java,v 1.63 2008/05/16 18:38:53 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.DataClassDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.DataClassSummaryViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicyDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicySummaryViewBean;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the FSDetails page
 */

public class FSDetailsViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "FSDetails";
    private static final String DEFAULT_DISPLAY_URL = "/jsp/fs/FSDetails.jsp";

    public static final String CHILD_BREADCRUMB = "BreadCrumb";

    // href for breadcrumb
    public static final String CHILD_FS_ARCH_POL_HREF = "FSArchivePolicyHref";
    public static final String CHILD_FS_SUM_HREF = "FileSystemSummaryHref";
    public static final String CHILD_STATICTEXT = "StaticText";
    public static final String DATA_CLASS_SUMMARY_HREF = "DataClassSummaryHref";
    public static final String DATA_CLASS_DETAILS_HREF = "DataClassDetailsHref";
    public static final String POLICY_SUMMARY_HREF   = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF   = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";

    public static final String CHILD_ROLE = "RoleName";
    public static final String CHILD_CONTAINER_VIEW = "FSDetailsView";
    public static final String CHILD_HIDDEN_SERVERNAME = "ServerName";
    public static final String CONFIRM_MESSAGES = "ConfirmMessages";

    private static final String CLUSTER_VIEW = "FSDClusterView";
    private static final String SUNPLEX_VIEW = "SunPlexManagerView";
    private static final String DEVICES_VIEW = "FSDevicesView";

    private CCBreadCrumbsModel breadCrumbsModel;

    /**
     * Constructor
     */
    public FSDetailsViewBean() {
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
        registerChild(CHILD_CONTAINER_VIEW, FSDetailsView.class);
        registerChild(CHILD_HIDDEN_SERVERNAME, CCHiddenField.class);
        registerChild(CONFIRM_MESSAGES, CCHiddenField.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_FS_ARCH_POL_HREF, CCHref.class);
        registerChild(CHILD_FS_SUM_HREF, CCHref.class);
        registerChild(DATA_CLASS_SUMMARY_HREF, CCHref.class);
        registerChild(DATA_CLASS_DETAILS_HREF, CCHref.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(CHILD_ROLE, CCStaticTextField.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CLUSTER_VIEW, FSDClusterView.class);
        registerChild(SUNPLEX_VIEW, SunPlexManagerView.class);
        registerChild(DEVICES_VIEW, FSDevicesView.class);
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
        if (name.equals(CLUSTER_VIEW)) {
            return new FSDClusterView(this, name);
        } else if (name.equals(SUNPLEX_VIEW)) {
            return new SunPlexManagerView(this, name);
        } else if (super.isChildSupported(name)) {
            TraceUtil.trace3("Exiting");
            return super.createChild(name);
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            return new FSDetailsView(this, name);
        } else if (name.equals(DEVICES_VIEW)) {
            return new FSDevicesView(this, name);
        } else if (name.equals(CHILD_HIDDEN_SERVERNAME) ||
            name.equals(CONFIRM_MESSAGES)) {
            TraceUtil.trace3("Exiting");
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_STATICTEXT)) {
            CCStaticTextField child = new CCStaticTextField(this, name, null);
            TraceUtil.trace3("Exiting");
            return child;
            // PageTitle Child
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("FSDetails.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            CCBreadCrumbs child =
                new CCBreadCrumbs(this, breadCrumbsModel, name);
            TraceUtil.trace3("Exiting");
            return child;
        } else if (name.equals(CHILD_FS_SUM_HREF) ||
                   name.equals(CHILD_FS_ARCH_POL_HREF) ||
                   name.equals(DATA_CLASS_SUMMARY_HREF) ||
                   name.equals(DATA_CLASS_DETAILS_HREF) ||
                   name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_ROLE)) {
            CCStaticTextField child = null;

            if (SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
                child = new CCStaticTextField(this, name, "admin");
            } else {
                child = new CCStaticTextField(this, name, "oper");
            }
            TraceUtil.trace3("Exiting");
            return child;
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

        String serverName = getServerName();

        ((CCHiddenField)
            getChild(CHILD_HIDDEN_SERVERNAME)).setValue(serverName);
        ((CCHiddenField)
            getChild(CONFIRM_MESSAGES)).setValue(
                SamUtil.getResourceString(
                    "FSDetails.confirmDeleteMsg").concat(
                "###").concat(
                SamUtil.getResourceString(
                    "FSDetails.confirmUnmountAndUnshareMsg")));

        try {
            // populate the action table model data here, for error handling
            FSDetailsView view =
                (FSDetailsView) getChild(CHILD_CONTAINER_VIEW);
            view.populateData();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "beginDisplay()",
                "Unable to populate FS Summary table",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSDetails.error.populate",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    public boolean beginFSDevicesViewDisplay(ChildDisplayEvent event) {
        String psaUFS = (String) getPageSessionAttribute("psa_UFS");
        boolean showBlank =
            psaUFS == null ?
                false :
                Boolean.valueOf(psaUFS).booleanValue();
        return !showBlank;
    }

    /**
     * Handle request for backtofs link
     */
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
                throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_FS_SUM_HREF);
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for "FS Archive Policy"  link
     */
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        String s = (String) getDisplayFieldValue(CHILD_FS_ARCH_POL_HREF);

        ViewBean targetView = getViewBean(FSArchivePoliciesViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    // handle breadcrumb to the data class summary page
    public void handleDataClassSummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String) getDisplayFieldValue(DATA_CLASS_SUMMARY_HREF);

        ViewBean target = getViewBean(DataClassSummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the data class details page
    public void handleDataClassDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String) getDisplayFieldValue(DATA_CLASS_DETAILS_HREF);

        ViewBean target = getViewBean(DataClassDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
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
