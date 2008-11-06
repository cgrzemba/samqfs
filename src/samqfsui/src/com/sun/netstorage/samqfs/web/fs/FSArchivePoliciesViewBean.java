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

// ident	$Id: FSArchivePoliciesViewBean.java,v 1.37 2008/11/06 00:47:07 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicyDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.PolicySummaryViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.FSArchiveDirective;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCImageField;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * ViewBean used to display the File System Archive Policy page
 */

public class FSArchivePoliciesViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "FSArchivePolicies";
    private static final String
        DEFAULT_DISPLAY_URL = "/jsp/fs/FSArchivePolicies.jsp";

    // Used for constructing the Action Table
    public static final String CHILD_CONTAINER_VIEW = "FSArchivePoliciesView";

    // Page Title Attributes and Components.
    protected CCPageTitleModel pageTitleModel = null;
    protected CCPropertySheetModel propertySheetModel = null;

    private CCBreadCrumbsModel breadCrumbsModel;

    public static final String CHILD_BREADCRUMB    = "BreadCrumb";

    // href for breadcrumb
    public static final String CHILD_FS_SUMMARY_HREF = "FileSystemSummaryHref";
    public static final String CHILD_FS_DETAILS_HREF = "FileSystemDetailsHref";
    public static final String CHILD_SHARED_FS_HREF = "SharedFSDetailsHref";

    // HREF to navigate back to Shared FS Summary Page
    public static final String SHARED_FS_SUMMARY_HREF = "SharedFSSummaryHref";
    public static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";

    // handler for archive pages (v44)
    public static final String POLICY_SUMMARY_HREF   = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF   = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";

    public static final String CHILD_ROLE = "RoleName";
    public static final String CHILD_STATICTEXT = "StaticText";

    public static final String CHILD_HIDDEN_SERVERNAME = "ServerName";
    public static final String CHILD_HIDDEN_FSNAME = "FileSystemName";

    // For legend
    public static final String CHILD_IMAGE  = "Image";

    protected FSArchivePoliciesView view;

    /**
     * Constructor
     */
    public FSArchivePoliciesViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        propertySheetModel = createPropertySheetModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_CONTAINER_VIEW, FSArchivePoliciesView.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_FS_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_SHARED_FS_HREF, CCHref.class);
        registerChild(CHILD_FS_DETAILS_HREF, CCHref.class);
        registerChild(FS_ARCHIVEPOL_HREF, CCHref.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(SHARED_FS_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_ROLE, CCStaticTextField.class);
        registerChild(CHILD_STATICTEXT, CCStaticTextField.class);
        registerChild(CHILD_HIDDEN_SERVERNAME, CCHiddenField.class);
        registerChild(CHILD_HIDDEN_FSNAME, CCHiddenField.class);
        registerChild(CHILD_IMAGE, CCImageField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Create child component
     * @param: child name: name
     * @return: child component
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering with name: " + name);

        View child = null;
        if (name.equals(CHILD_IMAGE)) {
            child = new CCImageField(this, name, null);
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
            child = new FSArchivePoliciesView(this, name, serverName, fsName);
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
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

            breadCrumbsModel =
                new CCBreadCrumbsModel("FSArchivePolicies.pageTitle");

            BreadCrumbUtil.createBreadCrumbs(
                this, CHILD_BREADCRUMB, breadCrumbsModel);
            child =
                new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(CHILD_FS_SUMMARY_HREF) ||
                   name.equals(CHILD_FS_DETAILS_HREF) ||
                   name.equals(CHILD_SHARED_FS_HREF) ||
                   name.equals(FS_ARCHIVEPOL_HREF) ||
                   name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(SHARED_FS_SUMMARY_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(CHILD_ROLE)) {
            if (SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
                child = new CCStaticTextField(this, name, "admin");
            } else {
                child = new CCStaticTextField(this, name, "oper");
            }
        } else if (name.equals(CHILD_STATICTEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_HIDDEN_SERVERNAME) ||
                   name.equals(CHILD_HIDDEN_FSNAME)) {
            child = new CCHiddenField(this, name, null);
        } else if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Create PageTitle Model
     * @return PageTitleModel
     */
    protected CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        CCPageTitleModel model = PageTitleUtil.createModel(
            "/jsp/fs/FSArchivePoliciesPageTitle.xml");
        TraceUtil.trace3("Exiting");
        return model;
    }

    /**
     * Create PropertySheet Model
     * @return PropertySheetModel
     */
    protected CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        CCPropertySheetModel model = PropertySheetUtil.createModel(
            "/jsp/fs/FSArchivePoliciesPropertySheet.xml");
        TraceUtil.trace3("Exiting");
        return model;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        // Check permission
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
            ((CCButton) getChild("SaveButton")).setDisabled(true);
            ((CCButton) getChild("ResetButton")).setDisabled(true);
            ((CCButton) getChild("CancelButton")).setDisabled(true);
        }


        String serverName = (String) getPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        String fsName = (String) getPageSessionAttribute(
                Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

        ((CCHiddenField)
            getChild(CHILD_HIDDEN_SERVERNAME)).setValue(serverName);
        ((CCHiddenField)
            getChild(CHILD_HIDDEN_FSNAME)).setValue(fsName);

        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString(
                "FSArchivePolicies.title.pageTitle", fsName));

        loadPropertySheetModel(serverName, fsName);

        view = (FSArchivePoliciesView) getChild(CHILD_CONTAINER_VIEW);
        try {
            view.populateData();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "FSArchivePoliciesViewBean()",
                "Unable to populate archive policy",
                serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSArchivePolicies.error.failedPopulate",
                ex.getSAMerrno(),
                ex.getMessage(),
                serverName);
        }


        TraceUtil.trace3("Exiting");
    }

    /**
     * Load the data for property sheet model
     */
    protected void loadPropertySheetModel(String serverName, String fsName) {
        String scanMethod = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SCAN_METHOD);
        String intervalValue = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.INTERVAL_VALUE);
        String intervalUnit = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.INTERVAL_UNIT);
        String logfile = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.LOGFILE_PATH);
        if (scanMethod == null &&
            intervalValue == null &&
            intervalUnit == null &&
            logfile == null) {

            // get archiving setup from system
            try {
                FSArchiveDirective fsad =
                    getFSArchiveDirective(serverName, fsName);
                if (fsad == null) {
                    throw new SamFSException(null, -1010);
                }
                scanMethod = Integer.toString(fsad.getFSArchiveScanMethod());
                long interval = fsad.getFSInterval();
                intervalValue =
                    (interval == -1 ? "" : Long.toString(interval));
                intervalUnit = Integer.toString(fsad.getFSIntervalUnit());
                logfile = fsad.getFSArchiveLogfile();
            } catch (SamFSException ex) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "FSArchivePoliciesViewBean()",
                    "Unable to populate propertysheet",
                    serverName);
                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "FSArchivePolicies.error.failedLoadPropertySheet",
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    serverName);
            }
        }
        if (scanMethod == null) {
            scanMethod = "-1";
        }
        if (intervalValue == null) {
            intervalValue = "";
        }
        if (intervalUnit == null) {
            intervalUnit = "-1";
        }
        if (logfile == null) {
            logfile = "";
        }
        propertySheetModel.setValue("scanMethodValue", scanMethod);
        propertySheetModel.setValue("intervalValue", intervalValue);
        propertySheetModel.setValue("intervalUnit", intervalUnit);
        propertySheetModel.setValue("logfileValue", logfile);
    }

    // get FSArchiveDirective obj for the given fs
    private FSArchiveDirective getFSArchiveDirective(
        String serverName, String fsName) throws SamFSException {

        FSArchiveDirective fsad = null;
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        SamQFSSystemArchiveManager
            archiveManager = sysModel.getSamQFSSystemArchiveManager();
        FSArchiveDirective[] fsads =
            archiveManager.getFSGeneralArchiveDirective();

        for (int i = 0; fsads != null && i < fsads.length; i++) {
            if (fsName.equals(fsads[i].getFileSystemName())) {
                fsad = fsads[i];
                break;
            }
        }

        return fsad;
    }

    public void handleSaveButtonRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        String scanMethod = (String)
            getDisplayFieldValue("scanMethodValue");
        String interval = (String)
            getDisplayFieldValue("intervalValue");
        String intervalUnit = (String)
            getDisplayFieldValue("intervalUnit");
        String logfile = (String) getDisplayFieldValue("logfileValue");

        boolean errorOccured = false;
        StringBuffer errors = new StringBuffer();

        // scan method
        int sm = Integer.parseInt(scanMethod);

        // interval unit
        int u = Integer.parseInt(intervalUnit);

        // interval value
        long intv = -1;
        interval = interval != null ? interval.trim() : "";
        if (!interval.equals("")) {
            try {
                intv = Long.parseLong(interval);
                if (intv < 0) {
                    errorOccured = true;
                    errors.append(SamUtil.getResourceString(
                        "FSArchivePolicies.error.intervalRange")).append(
                            "<br>");
                }
                if (u == -1) {
                    errorOccured = true;
                    errors.append(SamUtil.getResourceString(
                        "FSArchivePolicies.error.intervalDropDown")).append(
                            "<br>");
                }
            } catch (NumberFormatException nfe) {
                errorOccured = true;
                errors.append(SamUtil.getResourceString(
                    "FSArchivePolicies.error.intervalRange")).append("<br>");
            }
        } else {
            if (u != -1) {
                errorOccured = true;
                errors.append(SamUtil.getResourceString(
                    "FSArchivePolicies.error.intervalEmpty")).append("<br>");
            }
        }

        logfile = logfile != null ? logfile.trim() : "";
        if (!logfile.equals("") && (logfile.charAt(0) != '/')) {
            errorOccured = true;
            errors.append(SamUtil.getResourceString(
                "FSArchivePolicies.error.logfile")).append("<br>");
        }

        String serverName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        String fsName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME);

        if (!errorOccured) {
            try {
                FSArchiveDirective fsad =
                    getFSArchiveDirective(serverName, fsName);

                if (fsad == null) {
                    throw new SamFSException(null, -1010);
                }

                StringBuffer buf = new StringBuffer();
                buf.append("Saving FSArchiveDirective for File Systtem '")
                    .append(fsName)
                    .append("': scanMethod=")
                    .append(Integer.toString(sm))
                    .append("; interval=")
                    .append(Long.toString(intv))
                    .append("; interval unit=")
                    .append(Integer.toString(u))
                    .append("; logfile=")
                    .append(logfile);

                 TraceUtil.trace3(buf.toString());
                 fsad.setFSArchiveScanMethod(sm);
                 fsad.setFSInterval(intv);
                 fsad.setFSIntervalUnit(u);
                 fsad.setFSArchiveLogfile(logfile);
                 fsad.changeFSDirective();
                 TraceUtil.trace3("Done");
            } catch (SamFSException ex) {
                SamUtil.processException(
                    ex,
                    this.getClass(),
                    "handleSaveButtonRequest()",
                    "Failed to save archiving setup",
                    serverName);

                errorOccured = true;
                errors.append(ex.getMessage());
            }
        }

        if (errorOccured) {
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "FSArchivePolicies.error.save",
                -2024,
                errors.toString(),
                serverName);

            setPageSessionAttribute(
                Constants.PageSessionAttributes.SCAN_METHOD, scanMethod);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.INTERVAL_VALUE, interval);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.INTERVAL_UNIT, intervalUnit);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.LOGFILE_PATH, logfile);

            forwardTo(getRequestContext());
        } else {
            removePageSessionAttribute(
                Constants.PageSessionAttributes.SCAN_METHOD);
            removePageSessionAttribute(
                Constants.PageSessionAttributes.INTERVAL_VALUE);
            removePageSessionAttribute(
                Constants.PageSessionAttributes.INTERVAL_UNIT);
            removePageSessionAttribute(
                Constants.PageSessionAttributes.LOGFILE_PATH);
            forwardToPreviousPage(fsName);
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for Button 'Cancel'
     */
    public void handleCancelButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");
        forwardToPreviousPage(null);
        TraceUtil.trace3("Exiting");
    }

    protected void forwardToPreviousPage(String fsName) {
        ViewBean targetView = null;
        String s = null;

        Integer[] temp = (Integer [])
            getPageSessionAttribute(Constants.SessionAttributes.PAGE_PATH);
        Integer[] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);

        int index = path[path.length-1].intValue();

        PageInfo pageInfo = PageInfo.getPageInfo();
        String targetName = pageInfo.getPagePath(index).getCommandField();

        TraceUtil.trace3("target name = " + targetName);
        if (targetName.equals(CHILD_FS_DETAILS_HREF)) {
            targetView = getViewBean(FSDetailsViewBean.class);
        } else if (targetName.equals(CHILD_SHARED_FS_HREF)) {
            targetView = getViewBean(SharedFSDetailsViewBean.class);
        } else if (targetName.equals(CHILD_FS_SUMMARY_HREF)) {
            targetView = getViewBean(FSSummaryViewBean.class);
        } else if (targetName.equals(POLICY_DETAILS_HREF)) {
            targetView = getViewBean(PolicyDetailsViewBean.class);
        } else if (targetName.equals(CRITERIA_DETAILS_HREF)) {
            targetView = getViewBean(CriteriaDetailsViewBean.class);
        }

        if (targetView != null) {
            s = Integer.toString(
                BreadCrumbUtil.inPagePath(path, index, path.length - 1));
        }

        if (fsName != null) {
            SamUtil.setInfoAlert(
                targetView,
                CHILD_COMMON_ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "FSArchivePolicies.success.save", fsName),
                getServerName());
        }

        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
    }

    /**
     * Handle request for backto fs href
     * @param event RequestInvocationEvent event
     */
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_FS_SUMMARY_HREF);
        ViewBean targetView = getViewBean(FSSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for backto detail page
     * @param event requestInvocationEvent event
     */
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue(CHILD_FS_DETAILS_HREF);
        ViewBean targetView = getViewBean(FSDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()), s);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request for backto detail page
     * @param event requestInvocationEvent event
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

    // handle breadcrumb to the fs archive policies
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_ARCHIVEPOL_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSArchivePoliciesViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    // handle breadcrumb to the policy summary page
    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String s = (String)getDisplayFieldValue(POLICY_SUMMARY_HREF);
        ViewBean target = getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);
        forwardTo(target);
        TraceUtil.trace3("Exiting");
    }

    // handle breadcrumb to the policy details summary page
    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String s = (String)getDisplayFieldValue(POLICY_DETAILS_HREF);
        ViewBean target = getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
        TraceUtil.trace3("Exiting");
    }

    // handle breadcrumb to the criteria details summary page
    public void handleCriteriaDetailsHrefRequest(
        RequestInvocationEvent evt)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String s = (String)getDisplayFieldValue(CRITERIA_DETAILS_HREF);
        ViewBean target = getViewBean(CriteriaDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
        TraceUtil.trace3("Exiting");
    }

    // Handler to navigate back to Shared File System Summary Page
    public void handleSharedFSSummaryHrefRequest(
        RequestInvocationEvent evt)
        throws ServletException, IOException {

        String url = "/faces/jsp/fs/SharedFSSummary.jsp";

        TraceUtil.trace2("FSArchivePolicy: Navigate back to URL: " + url);

        String params =
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME
                 + "=" + getServerName()
                 + "&" + Constants.PageSessionAttributes.FILE_SYSTEM_NAME
                 + "=" + (String) getPageSessionAttribute(
                             Constants.PageSessionAttributes.FILE_SYSTEM_NAME);
        JSFUtil.forwardToJSFPage(this, url, params);
    }
}
