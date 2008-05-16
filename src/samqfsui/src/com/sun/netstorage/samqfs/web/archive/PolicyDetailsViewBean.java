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

// ident	$Id: PolicyDetailsViewBean.java,v 1.17 2008/05/16 18:38:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

public class PolicyDetailsViewBean extends CommonViewBeanBase {
    // useful symbolic constants
    public static final String PAGE_NAME = "PolicyDetails";
    private static final String DEFAULT_URL = "/jsp/archive/PolicyDetails.jsp";

    // breadcrumbing children
    public static final String BREADCRUMB = "BreadCrumb";
    public static final String POLICY_SUMMARY_HREF = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";
    public static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    public static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    public static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";

    // add/remove button state helpers
    public static final String CRITERIA_DELETABLE = "isCriteriaDeletable";
    public static final String COPY_DELETABLE = "isCopyDeletable";
    public static final String CRITERIA_DELETE_CONFIRMATION =
        "criteriaDeleteConfirmation";
    public static final String COPY_DELETE_CONFIRMATION =
        "copyDeleteConfirmation";

    // children
    private static final String PAGE_TITLE = "PageTitle";
    private static final String VIEW_POLICIES = "defaultPolicyViewAllPolicies";
    private static final String POLICY_DESCRIPTION = "policyTypeDescription";

    // hidden fields
    public static final String CRITERIA_NUMBERS = "criteriaNumbers";
    public static final String COPY_NUMBERS = "copyNumbers";
    public static final String SELECTED_CRITERIA = "selectedCriteriaNumber";
    public static final String SELECTED_COPY = "selectedCopyNumber";
    public static final String MEDIA_TYPES = "copyMediaTypes";
    public static final String SELECTED_COPY_MEDIA = "selectedCopyMediaType";

    // pagelet views
    public static final String FILE_MATCH_VIEW = "FileMatchCriteriaView";
    public static final String COPY_INFO_VIEW = "CopyInformationView";
    public static final String COPY_SETTINGS_VIEW = "CopySettingsView";

    /**
     * constructor
     */
    public PolicyDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);
        TraceUtil.trace3("Entering");

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * register this container view's children
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CRITERIA_DELETABLE, CCHiddenField.class);
        registerChild(COPY_DELETABLE, CCHiddenField.class);
        registerChild(CRITERIA_DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(COPY_DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_DETAILS_HREF, CCHref.class);
        registerChild(FS_ARCHIVEPOL_HREF, CCHref.class);
        registerChild(CRITERIA_NUMBERS, CCHiddenField.class);
        registerChild(COPY_NUMBERS, CCHiddenField.class);
        registerChild(SELECTED_CRITERIA, CCHiddenField.class);
        registerChild(SELECTED_COPY, CCHiddenField.class);
        registerChild(FILE_MATCH_VIEW, FileMatchCriteriaView.class);
        registerChild(COPY_SETTINGS_VIEW, CopySettingsView.class);
        registerChild(COPY_INFO_VIEW, CopyInformationView.class);
        registerChild(VIEW_POLICIES, CCButton.class);
        registerChild(POLICY_DESCRIPTION, CCStaticTextField.class);
        registerChild(MEDIA_TYPES, CCHiddenField.class);
        registerChild(SELECTED_COPY_MEDIA, CCHiddenField.class);

        TraceUtil.trace3("Exiting");
    }

    /**
     * create the named child view
     */
    public View createChild(String name) {
        if (name.equals(CRITERIA_DELETABLE) ||
            name.equals(COPY_DELETABLE) ||
            name.equals(CRITERIA_DELETE_CONFIRMATION) ||
            name.equals(COPY_DELETE_CONFIRMATION) ||
            name.equals(CRITERIA_NUMBERS) ||
            name.equals(COPY_NUMBERS) ||
            name.equals(SELECTED_CRITERIA) ||
            name.equals(SELECTED_COPY) ||
            name.equals(MEDIA_TYPES) ||
            name.equals(SELECTED_COPY_MEDIA)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            CCBreadCrumbsModel bcModel =
                new CCBreadCrumbsModel("PolicyDetails.title");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF) ||
                   name.equals(FS_SUMMARY_HREF) ||
                   name.equals(FS_DETAILS_HREF) ||
                   name.equals(FS_ARCHIVEPOL_HREF)) {
            return new CCHref(this, name, null);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, new CCPageTitleModel(), name);
        } else if (name.equals(FILE_MATCH_VIEW)) {
            return new FileMatchCriteriaView(this, name);
        } else if (name.equals(COPY_SETTINGS_VIEW)) {
            return new CopySettingsView(this, name);
        } else if (name.equals(COPY_INFO_VIEW)) {
            return new CopyInformationView(this, name);
        } else if (name.equals(VIEW_POLICIES)) {
            return new CCButton(this, name, null);
        } else if (name.equals(POLICY_DESCRIPTION)) {
            return new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException("unknown child '" + name + "'");
        }
    }

    protected void populateFileMatchCriteriaView() {
        FileMatchCriteriaView fmcView =
            (FileMatchCriteriaView)getChild(FILE_MATCH_VIEW);
        fmcView.populateTableModel();
    }

    protected void populateCopyInformationView() {
        CopyInformationView civ =
            (CopyInformationView)getChild(COPY_INFO_VIEW);
        civ.populateTableModel();
    }

    protected void populateCopySettingsView() {
        CopySettingsView csv = (CopySettingsView)getChild(COPY_SETTINGS_VIEW);
        csv.populateTableModel();
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        CCPageTitleModel ptModel = (CCPageTitleModel)
            ((CCPageTitle)getChild(PAGE_TITLE)).getModel();

        // determine which of the three views should be displayed and populate
        // them
        String serverName = getServerName();
        String policyName = (String)
            getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        ArchivePolicy thePolicy = null;

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

            String description = "";
            switch (thePolicy.getPolicyType()) {
                case ArSet.AR_SET_TYPE_GENERAL:
                    populateFileMatchCriteriaView();
                    populateCopyInformationView();
                    description = "archiving.policydescription.custom";
                    break;

                case ArSet.AR_SET_TYPE_NO_ARCHIVE:
                    populateFileMatchCriteriaView();
                    description = "archiving.policydescription.noarchive";
                    break;

                case ArSet.AR_SET_TYPE_DEFAULT:
                    populateCopySettingsView();
                    populateCopyInformationView();
                    description = "archiving.policydescription.default";

                    setDisplayFieldValue(VIEW_POLICIES,
                        "archiving.defaultpolicy.viewallpolicies");
                    break;

                case ArSet.AR_SET_TYPE_ALLSETS_PSEUDO:
                      populateCopyInformationView();
                      description = "archiving.policydescription.allsets";
                    break;

                case ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT:
                    populateCopySettingsView();
                    populateCopyInformationView();
                    description = "archiving.policydescription.explicitdefault";
		    break;

                case ArSet.AR_SET_TYPE_UNASSIGNED:
                default:
            }

            setDisplayFieldValue(POLICY_DESCRIPTION,
                SamUtil.getResourceString(description,
                          PolicyUtil.getPolicyFSString(thePolicy)));
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "beginDisplay",
                                     "unable to determine policy type",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  SamUtil.getResourceString("-2000"),
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // Set different page title for allsets
        if (thePolicy != null &&
            ArSet.AR_SET_TYPE_ALLSETS_PSEUDO == thePolicy.getPolicyType()) {
            ptModel.setPageTitleText(SamUtil.getResourceString(
                "archiving.policy.details.allsetspagetitle"));
        } else {
            ptModel.setPageTitleText(SamUtil.getResourceString(
                "archiving.policy.details.pagetitle", policyName));
        }

        // set delete confirmation messages
        CCHiddenField field =
            (CCHiddenField)getChild(CRITERIA_DELETE_CONFIRMATION);
        field.setValue(
            SamUtil.getResourceString("archiving.criteria.delete.confirm"));

        field = (CCHiddenField)getChild(COPY_DELETE_CONFIRMATION);
        field.setValue(
            SamUtil.getResourceString("archiving.copy.delete.confirm"));
    }

    /**
     * hide the 'view all policies' button for all non-default policies
     */
    public boolean beginDefaultPolicyViewAllPoliciesDisplay(
                      ChildDisplayEvent evt) throws ModelControlException {
        if (getPolicyType() == ArSet.AR_SET_TYPE_DEFAULT) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * handler for the default policy 'View All Policies' button
     */
    public void handleDefaultPolicyViewAllPoliciesRequest(
        RequestInvocationEvent rie)  throws ServletException, IOException {

        // set the filesystem into page session
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        setPageSessionAttribute(
            Constants.PageSessionAttributes.FILE_SYSTEM_NAME, policyName);

        // target view bean
        ViewBean target = getViewBean(FSArchivePoliciesViewBean.class);

        BreadCrumbUtil.breadCrumbPathForward(this,
            PageInfo.getPageInfo().getPageNumber(this.getName()));

        forwardTo(target);
    }

    /**
     * convenience method used by the copy & criteria views to retrieve the
     * policy type
     */
    public short getPolicyType() {
        Integer policyType =
            (Integer)getPageSessionAttribute(Constants.Archive.POLICY_TYPE);
        return policyType.shortValue();
    }

    // handle breadcrumb to the policy summary page
    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    // handle breadcrumb to the policy details summary page - incase we loop
    // back here
    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    // handle breadcrumb to the criteria details summary page - incase we loop
    // back here
    public void handleCriteriaDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(CRITERIA_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(CriteriaDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    // handle breadcrumb to the filesystem page
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    // handle breadcrumb to the filesystem deatils page
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
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
}
