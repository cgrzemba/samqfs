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

// ident	$Id: CopyVSNsViewBean.java,v 1.32 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
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
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * Policy Copy VSN Map Details outer-most containerview
 */
public class CopyVSNsViewBean extends CommonViewBeanBase {
    private static final String PAGE_NAME = "CopyVSNs";
    private static final String DEFAULT_URL = "/jsp/archive/CopyVSNs.jsp";

    // children
    private static final String BREADCRUMB = "BreadCrumb";
    private static final String TEXT_FREE_SPACE = "TextFreeSpace";
    private static final String MEDIA_TYPE_LABEL = "LabelMediaType";
    private static final String FREE_SPACE_LABEL = "LabelFreeSpace";
    private static final String RESET_MESSAGE = "ResetMessage";
    private static final String SERVER_NAME = "ServerName";
    private static final String POLICY_NAME = "PolicyName";
    private static final String COPY_NUMBER = "CopyNumber";
    private static final String DELETE_ALL_MESSAGE = "DeleteAllMessage";

    // bread crumbing urls
    private static final String POLICY_SUMMARY_HREF = "PolicySummaryHref";
    private static final String POLICY_DETAILS_HREF = "PolicyDetailsHref";
    private static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";
    private static final String POLICY_TAPECOPY_HREF = "PolicyTapeCopyHref";
    private static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    private static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    private static final String SHARED_FS_SUMMARY_HREF = "SharedFSSummaryHref";
    private static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";
    private static final String COPY_VSNS_HREF = "CopyVSNsHref";

    // child views
    private static final String MEDIA_TYPE = "mediaType";

    private static final String PT_FILE = "/jsp/archive/CopyVSNsPageTitle.xml";
    private CCPageTitleModel ptModel = null;

    private static final String CHILD_CONTAINER_VIEW = "MediaExpressionView";

    // the archive copy that is being edited
    private ArchivePolicy thePolicy = null;

    /** create an instance of this view bean */
    public CopyVSNsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // create the page title model
        ptModel = PageTitleUtil.createModel(PT_FILE);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * register this view's children
     */
    public void registerChildren() {
        // register typical view bean children
        super.registerChildren();

        registerChild(CHILD_CONTAINER_VIEW, MediaExpressionView.class);
        registerChild(MEDIA_TYPE_LABEL, CCLabel.class);
        registerChild(FREE_SPACE_LABEL, CCLabel.class);
        registerChild(MEDIA_TYPE, CCDropDownMenu.class);
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(TEXT_FREE_SPACE, CCStaticTextField.class);
        registerChild(RESET_MESSAGE, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(DELETE_ALL_MESSAGE, CCHiddenField.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(POLICY_TAPECOPY_HREF, CCHref.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_DETAILS_HREF, CCHref.class);
        registerChild(FS_ARCHIVEPOL_HREF, CCHref.class);
        registerChild(SHARED_FS_SUMMARY_HREF, CCHref.class);
        registerChild(COPY_VSNS_HREF, CCHref.class);
        registerChild(POLICY_NAME, CCHiddenField.class);
        registerChild(COPY_NUMBER, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, ptModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * create an named child view
     */
    public View createChild(String name) {
        if (name.equals(CHILD_CONTAINER_VIEW)) {
            return new MediaExpressionView(this, name, false);
        } else if (name.equals(MEDIA_TYPE)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(TEXT_FREE_SPACE)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(RESET_MESSAGE) ||
                   name.equals(SERVER_NAME) ||
                   name.equals(POLICY_NAME) ||
                   name.equals(COPY_NUMBER) ||
                   name.equals(DELETE_ALL_MESSAGE)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            CCBreadCrumbsModel bcModel =
                new CCBreadCrumbsModel("CopyVSNs.title");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF) ||
                   name.equals(POLICY_TAPECOPY_HREF) ||
                   name.equals(FS_SUMMARY_HREF) ||
                   name.equals(FS_DETAILS_HREF) ||
                   name.equals(FS_ARCHIVEPOL_HREF) ||
                   name.equals(SHARED_FS_SUMMARY_HREF) ||
                   name.equals(COPY_VSNS_HREF)) {
            return new CCHref(this, name, null);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (name.startsWith("Label")) {
            return new CCLabel(this, name, null);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    /**
     * This method is called when the page displays
     * @param evt
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // set the page title text
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer copyNumber =
            (Integer)getPageSessionAttribute(Constants.Archive.COPY_NUMBER);

        ptModel.setPageTitleText(
            SamUtil.getResourceString("archiving.policy.copyvsns.pagetitle",
                new String[] {policyName,
            SamUtil.getResourceString("archiving.copynumber",
                                      new String [] {copyNumber.toString()})}));

        String serverName = getServerName();

        ((CCHiddenField) getChild(RESET_MESSAGE)).setValue(
            SamUtil.getResourceString("CopyVSNs.message.hardreset").concat(
                ";").concat(
            SamUtil.getResourceString("CopyVSNs.message.newtype")));
        ((CCHiddenField) getChild(SERVER_NAME)).setValue(getServerName());
        ((CCHiddenField) getChild(POLICY_NAME)).setValue(policyName);
        ((CCHiddenField) getChild(
            COPY_NUMBER)).setValue(Integer.toString(copyNumber));
        ((CCHiddenField) getChild(DELETE_ALL_MESSAGE)).setValue(
            SamUtil.getResourceString("CopyVSNs.message.cannotdeleteall"));

        try {
            populateMediaTypeMenu(getVSNMap());

            ((CCStaticTextField) getChild(TEXT_FREE_SPACE)).setValue(
                Capacity.newCapacity(
                    getVSNMap().getAvailableSpace(),
                    SamQFSSystemModel.SIZE_MB));

        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "populateCopyVSNsPage",
                                     "unable to retrieve copy vsn information",
                                     serverName);

            SamUtil.setWarningAlert(this,
                                    CHILD_COMMON_ALERT,
                                    "ArchiveConfig..warning.summary",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     getClass(),
                                     "populateCopyVSNsPage",
                                     "unable to retrieve copy vsn information",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "populateCopyVSNsPage",
                                     "unable to retrieve copy vsn information",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
    }

    private void populateMediaTypeMenu(ArchiveVSNMap vsnMap)
        throws SamFSException {
        CCDropDownMenu menu = (CCDropDownMenu) getChild(MEDIA_TYPE);

        // Check Permission
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            menu.setDisabled(true);
        }

        int mediaType = vsnMap.getArchiveMediaType();

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        int [] mediaTypes = sysModel.getSamQFSSystemMediaManager().
                                    getAvailableArchiveMediaTypes();

        boolean exist = false;
        for (int i = 0; i < mediaTypes.length; i++) {
            if (mediaTypes[i] == mediaType) {
                exist = true;
                break;
            }
        }

        int count = exist ? mediaTypes.length : mediaTypes.length + 1;
        String [] mediaTypeLabels = new String[count];
        String [] mediaTypeValues = new String[count];

        int index = 0;
        if (!exist) {
            mediaTypeLabels[0] = SamUtil.getMediaTypeString(mediaType);
            mediaTypeValues[0] = Integer.toString(-999);
            index = 1;
        }

        for (int j = 0, i = index; j < mediaTypes.length; i++, j++) {
            mediaTypeLabels[i] = SamUtil.getMediaTypeString(mediaTypes[j]);
            mediaTypeValues[i] = Integer.toString(mediaTypes[j]);
        }

        menu.setOptions(
            new OptionList(mediaTypeLabels, mediaTypeValues));

        String mediaTypeString = mediaType == -1 ?
            "media.type.unknown" : Integer.toString(mediaType);

        menu.setValue(mediaTypeString);
    }

    public ArchivePolicy getPolicy() throws SamFSException {
        if (thePolicy == null) {
            String serverName = getServerName();
            String policyName = (String)
                getPageSessionAttribute(Constants.Archive.POLICY_NAME);

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            // make sure the policy wasn't deleted in the process
            if (thePolicy == null) {
                throw new SamFSException(null, -2000);
            }
        }
        return thePolicy;
    }

    public ArchiveVSNMap getVSNMap() throws SamFSException {
        Integer copyNumber = (Integer)
                getPageSessionAttribute(Constants.Archive.COPY_NUMBER);
        ArchiveCopy theCopy = getPolicy().getArchiveCopy(copyNumber.intValue());
        if (theCopy == null) {
            throw new SamFSException(null, -2006);
        }
        return theCopy.getArchiveVSNMap();
    }

    // bread crumbing links
    // handle breadcrumb to the policy summary page
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

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
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
