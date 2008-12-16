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

// ident	$Id: CopyOptionsViewBean.java,v 1.23 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import java.io.IOException;
import java.util.List;
import javax.servlet.ServletException;

/**
 * CopyOptions - Advanced copy options page
 */
public class CopyOptionsViewBean extends CommonViewBeanBase {
    public static final String PAGE_NAME = "CopyOptions";
    private static final String DEFAULT_URL = "/jsp/archive/CopyOptions.jsp";

    // bread crumbing
    public static final String BREADCRUMB = "BreadCrumb";
    private static final String POLICY_SUMMARY_HREF = "PolicySummaryHref";
    private static final String POLICY_DETAILS_HREF = "PolicyDetailsHref";
    private static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";
    private static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    private static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    private static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";
    private static final String COPY_OPTIONS_HREF = "CopyOptionsHref";
    private static final String SHARED_FS_SUMMARY_HREF = "SharedFSSummaryHref";
    // the two pagelet views
    public static final String DISK_VIEW = "DiskCopyOptionsView";
    public static final String TAPE_VIEW = "TapeCopyOptionsView";

    // hidden fields for js help
    public static final String HARD_RESET = "hardReset";

    private CCPageTitleModel ptModel = null;
    private static String pageTitleXMLFile =
        "/jsp/archive/CopyOptionsPageTitle.xml";

    public CopyOptionsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        ptModel = PageTitleUtil.createModel(pageTitleXMLFile);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        super.registerChildren();
        PageTitleUtil.registerChildren(this, ptModel);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_DETAILS_HREF, CCHref.class);
        registerChild(FS_ARCHIVEPOL_HREF, CCHref.class);
        registerChild(COPY_OPTIONS_HREF, CCHref.class);
        registerChild(SHARED_FS_SUMMARY_HREF, CCHref.class);
        registerChild(DISK_VIEW, DiskCopyOptionsView.class);
        registerChild(TAPE_VIEW, TapeCopyOptionsView.class);
        registerChild(HARD_RESET, CCHiddenField.class);
    }

    public View createChild(String name) {
        if (name.equals(HARD_RESET)) {
            return new CCHiddenField(this, name, "false");
        } else if (name.equals(POLICY_SUMMARY_HREF) ||
            name.equals(POLICY_DETAILS_HREF) ||
            name.equals(CRITERIA_DETAILS_HREF) ||
            name.equals(FS_SUMMARY_HREF) ||
            name.equals(FS_DETAILS_HREF) ||
            name.equals(FS_ARCHIVEPOL_HREF) ||
            name.equals(COPY_OPTIONS_HREF) ||
            name.equals(SHARED_FS_SUMMARY_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            CCBreadCrumbsModel bcModel =
                new CCBreadCrumbsModel("CopyOptions.title");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (name.equals(DISK_VIEW)) {
            return new DiskCopyOptionsView(this, name);
        } else if (name.equals(TAPE_VIEW)) {
            return new TapeCopyOptionsView(this, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    /**
     * handler for the save button
     */
    public void handleSaveRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        // determine copy type and proceed appropriately.
        Integer copyMediaType =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE);
        String serverName = getServerName();
        CopyOptionsViewBase copyView = null;
        if (copyMediaType.intValue() == BaseDevice.MTYPE_DISK ||
            copyMediaType.intValue() == BaseDevice.MTYPE_STK_5800) {
            copyView = (CopyOptionsViewBase)getChild(DISK_VIEW);
        } else {
            copyView = (CopyOptionsViewBase)getChild(TAPE_VIEW);
        }

        try {
            List errors = copyView.validateCopyOptions();
            if (errors.size() > 0) {
                copyView.printErrorMessages(errors);

                // only a hard reset will work since values are partially saved
                ((CCHiddenField)getChild(HARD_RESET)).setValue("true");
            } else {
                // show success in the policy details page
                forwardToPreviousPage(true);
            }
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     getClass(),
                                     "handleSaveRequest",
                                     "unable to save copy options",
                                     serverName);

            SamUtil.setWarningAlert(this,
                                    CHILD_COMMON_ALERT,
                                    "ArchiveConfig.warning.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     getClass(),
                                     "handleSaveRequest",
                                     "unable to save copy options",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.warning.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "HandleSaveRequest",
                                     "Unable to save copy options",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        forwardTo(getRequestContext());
    }

    /**
     * handler for the Reset button
     */
    public void handleResetRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        // perform a hard reset
        PolicyUtil.resetArchiveManager(this);
        ((CCHiddenField)getChild(HARD_RESET)).setValue("false");

        forwardTo(getRequestContext());
    }

    /**
     * handler for the cancel button
     */
    public void handleCancelRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {

        Boolean bool = Boolean.valueOf(getDisplayFieldStringValue(HARD_RESET));

        if (bool.booleanValue()) {
            PolicyUtil.resetArchiveManager(this);

            ((CCHiddenField)getChild(HARD_RESET)).setValue("false");
        }

        forwardToPreviousPage(false);
    }

    /**
     * we can get to this method from either, handle save or handle cancel.
     */
    private void forwardToPreviousPage(boolean save) {
        TraceUtil.trace3("Entering");
        CommonViewBeanBase target = null;

        // get the last link in the breadcrumb
        Integer[] temp = (Integer [])getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        Integer[] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);

        int index = path[path.length-1].intValue();

        // get the name of the link
        PageInfo pageInfo = PageInfo.getPageInfo();
        String targetHref = pageInfo.getPagePath(index).getCommandField();


        // two possible pages can go back to
        if (targetHref.equals(PolicyDetailsViewBean.POLICY_DETAILS_HREF)) {
            target =  (CommonViewBeanBase)
                getViewBean(PolicyDetailsViewBean.class);
        } else if (targetHref
                   .equals(ISPolicyDetailsViewBean.IS_POLICY_DETAILS_HREF)) {
            target = (CommonViewBeanBase)
                getViewBean(ISPolicyDetailsViewBean.class);
        }

        // display field value for the Href
        String s = Integer.toString
                (BreadCrumbUtil.inPagePath(path, index, path.length-1));

        if (save) {
            Integer cn =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_NUMBER);
            String cns = cn == null ? "" : cn.toString();

            SamUtil.setInfoAlert(target,
                                 "Alert",
                                 "success.summary",
                                 SamUtil.getResourceString(
                                     "ArchivePolCopy.msg.save", cns),
                                 getServerName());
        }

        // go back to the last page in the breadcrumb
         BreadCrumbUtil.breadCrumbPathBackward(this,
                PageInfo.getPageInfo().getPageNumber(target.getName()), s);

         this.forwardTo(target);

        TraceUtil.trace3("Exiting");
    }

    /**
     * begin display ...
     */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // initialize page title buttons
        ptModel.setValue("Save", "archiving.save");
        ptModel.setValue("Cancel", "archiving.cancel");

        // set page title
        Integer copyNumber =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_NUMBER);
        String policyName = (String)
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);

        String serverName = getServerName();

        ptModel.setPageTitleText(
            SamUtil.getResourceString("archiving.copy.detail.pagetitle",
                           new String [] {policyName,
            SamUtil.getResourceString("archiving.copynumber",
                             new String [] {copyNumber.toString()})}));

        // initialize copy options
        Integer copyMediaType = (Integer)
            getPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE);
        TraceUtil.trace3("media type = " + copyMediaType);
        CopyOptionsViewBase copyView = null;
        if (copyMediaType.intValue() == BaseDevice.MTYPE_DISK ||
            copyMediaType.intValue() == BaseDevice.MTYPE_STK_5800) {
            copyView = (CopyOptionsViewBase)getChild(DISK_VIEW);
        } else {
            copyView = (CopyOptionsViewBase)getChild(TAPE_VIEW);
        }

        try {
            copyView.loadCopyOptions();
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     getClass(),
                                     "loadCopyOptions",
                                     "unable to load copy options",
                                     serverName);

            SamUtil.setInfoAlert(this,
                                 CHILD_COMMON_ALERT,
                                 "ArchiveConfig.error.summary",
                                 "ArchiveConfig.warning.detail",
                    serverName);
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     getClass(),
                                     "loadCopyOptions",
                                     "unable to load copy options",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.details",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "loadCopyOptions",
                                     "unable to load copy options",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // disable save & reset buttons if no config authorization
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {

            ((CCButton)getChild("Save")).setDisabled(true);
        }
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

    // handle breadcrumb to the policy  summary page - incase we loop
    // back here
    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_SUMMARY_HREF);
        ViewBean target = getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the policy details summary page - incase we loop
    // back here
    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_DETAILS_HREF);
        ViewBean target = getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the criteria details summary page - incase we loop
    // back here
    public void handleCriteriaDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(CRITERIA_DETAILS_HREF);
        ViewBean target = getViewBean(CriteriaDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the filesystem page
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_SUMMARY_HREF);
        ViewBean target = getViewBean(FSSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the filesystem deatils page
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_DETAILS_HREF);
        ViewBean target = getViewBean(FSDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the fs archive policies
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_ARCHIVEPOL_HREF);
        ViewBean target = getViewBean(FSArchivePoliciesViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this, target,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

}
