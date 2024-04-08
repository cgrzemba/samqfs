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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: ISPolicyDetailsViewBean.java,v 1.18 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
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
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import javax.servlet.ServletException;

public class ISPolicyDetailsViewBean extends CommonViewBeanBase {
    // useful symbolic constants
    public static final String PAGE_NAME = "ISPolicyDetails";
    private static final String DEFAULT_URL =
        "/jsp/archive/ISPolicyDetails.jsp";

    // breadcrumbing children
    public static final String BREADCRUMB = "BreadCrumb";
    public static final String POLICY_SUMMARY_HREF = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";
    public static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    public static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    public static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";
    public static final String DATA_CLASS_SUMMARY_HREF  =
        "DataClassSummaryHref";
    public static final String IS_POLICY_DETAILS_HREF = "ISPolicyDetailsHref";

    // begin delete marker
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
    private static final String MAX_COPY_COUNT = "maxCopyCount";
    private static final String DESCRIPTION = "policyDescription";

    // hidden fields
    public static final String CRITERIA_NUMBERS = "criteriaNumbers";
    public static final String COPY_NUMBERS = "copyNumbers";
    public static final String SELECTED_CRITERIA = "selectedCriteriaNumber";
    public static final String SELECTED_COPY = "selectedCopyNumber";
    public static final String MEDIA_TYPES = "copyMediaTypes";
    public static final String SELECTED_COPY_MEDIA = "selectedCopyMediaType";
    // end delete marker

    // pagelet views
    public static final String POLICY_VIEW = "ISPolicyDetailsView";
    public static final String ALLSETS_POLICY_VIEW =
        "ISAllsetsPolicyDetailsView";

    // child models
    private CCPageTitleModel ptModel = null;

    /**
     * constructor
     */
    public ISPolicyDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);
        TraceUtil.trace3("Entering");

        ptModel = createPageTitleModel();
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
        registerChild(DATA_CLASS_SUMMARY_HREF, CCHref.class);
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
        registerChild(VIEW_POLICIES, CCButton.class);
        registerChild(POLICY_DESCRIPTION, CCStaticTextField.class);
        registerChild(MEDIA_TYPES, CCHiddenField.class);
        registerChild(SELECTED_COPY_MEDIA, CCHiddenField.class);
        registerChild(POLICY_VIEW, ISPolicyDetailsView.class);
        registerChild(ALLSETS_POLICY_VIEW, ISAllsetsPolicyDetailsView.class);

        // page title
        PageTitleUtil.registerChildren(this, ptModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * create the named child view
     */
    public View createChild(String name) {
        if (name.equals(DESCRIPTION)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(POLICY_VIEW)) {
            return new ISPolicyDetailsView(this, name);
        } else if (name.equals(ALLSETS_POLICY_VIEW)) {
            return new ISAllsetsPolicyDetailsView(this, name);
        } else if (name.equals(CRITERIA_DELETABLE) ||
            name.equals(COPY_DELETABLE) ||
            name.equals(CRITERIA_DELETE_CONFIRMATION) ||
            name.equals(COPY_DELETE_CONFIRMATION) ||
            name.equals(CRITERIA_NUMBERS) ||
            name.equals(COPY_NUMBERS) ||
            name.equals(SELECTED_CRITERIA) ||
            name.equals(SELECTED_COPY) ||
            name.equals(MEDIA_TYPES) ||
            name.equals(SELECTED_COPY_MEDIA) ||
            name.equals(MAX_COPY_COUNT)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            CCBreadCrumbsModel bcModel =
                new CCBreadCrumbsModel("PolicyDetails.title");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(DATA_CLASS_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF) ||
                   name.equals(FS_SUMMARY_HREF) ||
                   name.equals(FS_DETAILS_HREF) ||
                   name.equals(FS_ARCHIVEPOL_HREF)) {
            return new CCHref(this, name, null);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (name.equals(PAGE_TITLE)) {
            return new CCPageTitle(this, ptModel, name);
        } else if (name.equals(VIEW_POLICIES)) {
            return new CCButton(this, name, null);
        } else if (name.equals(POLICY_DESCRIPTION)) {
            return new CCStaticTextField(this, name, null);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else {
            throw new IllegalArgumentException("unknown child '" + name + "'");
        }
    }

    private CCPageTitleModel createPageTitleModel() {
        return PageTitleUtil.createModel("/jsp/archive/PageTitle.xml");
    }

    public String getPolicyName() {
        TraceUtil.trace3("Entering");

        String policyName = (String)
            getPageSessionAttribute(Constants.Archive.POLICY_NAME);

        if (policyName == null) {
            policyName = RequestManager.
                getRequestContext().getRequest().getParameter("POLICY_NAME");
            setPageSessionAttribute(Constants.Archive.POLICY_NAME, policyName);
        }

        TraceUtil.trace3("Exiting");
        return policyName;
    }

    /** the jsp has begun displaying */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String policyName = getPolicyName();

        // set page title text and load confirmation messages.
        CCPageTitleModel ptModel = (CCPageTitleModel)
            ((CCPageTitle)getChild(PAGE_TITLE)).getModel();
        ptModel.setPageTitleText(SamUtil.getResourceString(
            "archiving.policy.details.pagetitle", policyName));
        CCHiddenField field = (CCHiddenField)
            getChild(COPY_DELETE_CONFIRMATION);
        field.setValue(SamUtil.
                       getResourceString("archiving.copy.delete.confirm"));

        field = (CCHiddenField)getChild(MAX_COPY_COUNT);
        if (policyName.equals(ArchivePolicy.POLICY_NAME_ALLSETS)) {
            field.setValue("5");
        } else {
            field.setValue("4");
        }

        // set the policy description
        CCLabel label = (CCLabel)getChild(DESCRIPTION);
        try {
            ((CCLabel)getChild(DESCRIPTION)).setValue(
                SamUtil.getModel(serverName).getSamQFSSystemArchiveManager()
                    .getArchivePolicy(policyName).getPolicyDescription());
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "beginDisplay",
                           "unable to retrieve the current policy description",
                                     serverName);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * convenience method used by the copy & criteria views to retrieve the
     * policy type
     */
    public short getPolicyType() {
        TraceUtil.trace3("Entering");

        Integer policyType =
            (Integer)getPageSessionAttribute(Constants.Archive.POLICY_TYPE);
        String serverName = getServerName();
        if (policyType == null) {
            try {
                short pt = SamUtil.getModel(serverName)
                    .getSamQFSSystemArchiveManager()
                    .getArchivePolicy(getPolicyName()).getPolicyType();

                policyType = new Integer((int)pt);
                setPageSessionAttribute(Constants.Archive.POLICY_TYPE,
                                        policyType);
            } catch (SamFSException sfe) {
                SamUtil.processException(sfe,
                                         getClass(),
                                         "getPolicyType",
                                 "unable to determine the current policy type",
                                         serverName);
            }
        }

        TraceUtil.trace3("Exiting");
        return policyType.shortValue();
    }

    /** handler for the save button */
    public void handleSaveRequest(RequestInvocationEvent rie)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        List errors = new ArrayList();
        boolean success = false;
        try {
            ArchivePolicy thePolicy = SamUtil.getModel(getServerName())
                    .getSamQFSSystemArchiveManager()
                    .getArchivePolicy(getPolicyName());

            if (thePolicy.getPolicyType() ==
                ArSet.AR_SET_TYPE_ALLSETS_PSEUDO) {
                errors = ((ISAllsetsPolicyDetailsView)
                    getChild(ALLSETS_POLICY_VIEW)).save();
            } else {
                errors = ((ISPolicyDetailsView)getChild(POLICY_VIEW)).save();
            }

            // update hthe policy
            if (errors.size() < 1) {
                thePolicy.updatePolicy();

                // set the success message here
                success = true;
            } else {
                // TODO: set error messages
            }
        } catch (SamFSException sfe) {
            // set error messages
            // TODO:

            // set success to false
            success = false;
        }

        // if we get this far, we need just recyle the page
        forwardToPreviousPage(success);
        TraceUtil.trace3("Exiting");
    }

    /** handler for the cancel button */
    public void handleCancelRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        forwardToPreviousPage(false);
    }

    /** handler for the edit copy options */
    public void handleEditCopyOptionsRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Handler for copy options");

        forwardTo(getRequestContext());
    }

    public void forwardToPreviousPage(boolean saved) {
        TraceUtil.trace3("Entering");

        if (!saved) {
           forwardTo(getRequestContext());
           return;
        }

        CommonViewBeanBase target = null;

        // get the last link in the breadcrumb
        Integer [] temp = (Integer [])
            getPageSessionAttribute(Constants.SessionAttributes.PAGE_PATH);
        Integer [] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);

        int index = path[path.length -1].intValue();

        PageInfo pageInfo = PageInfo.getPageInfo();
        String targetHref = pageInfo.getPagePath(index).getCommandField();

        if (targetHref.equals(POLICY_SUMMARY_HREF)) {
        	target = (CommonViewBeanBase)
				getViewBean(PolicySummaryViewBean.class);
		} else if (targetHref.equals(DATA_CLASS_SUMMARY_HREF)) {
			target = (CommonViewBeanBase)
				getViewBean(DataClassSummaryViewBean.class);
		}

        // set success message
        SamUtil.setInfoAlert(target,
                             target.CHILD_COMMON_ALERT,
                             "success.summary",
             SamUtil.getResourceString("archiving.policydetails.save.success",
                              getPolicyName()),
                              getServerName());

        // display field for the Href
        String s = Integer.
            toString(BreadCrumbUtil.inPagePath(path, index, path.length-1));

        TraceUtil.trace3("breadcrumbutil s = " + s);
        // go back to the last page in the breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);

    }

    // handle breadcrumb to the data class summary page
    public void handleDataClassSummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(DATA_CLASS_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(DataClassSummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
        TraceUtil.trace3("Exiting");
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
