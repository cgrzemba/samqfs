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

// ident	$Id: VSNPoolDetailsViewBean.java,v 1.37 2008/12/16 00:10:56 am143972 Exp $

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
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCHref;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * ViewBean used to display the details of a VSN Pool.
 */
public class VSNPoolDetailsViewBean extends CommonViewBeanBase {
    // Page information...
    private static final String PAGE_NAME = "VSNPoolDetails";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/archive/VSNPoolDetails.jsp";

    private static final String CHILD_BREADCRUMB = "BreadCrumb";
    private static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    private static final String SHARED_FS_SUMMARY_HREF = "SharedFSSummaryHref";
    private static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    private static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";
    private static final String CHILD_BACKTOVSNSUM_HREF = "VSNPoolSummaryHref";
    private static final String CHILD_EDITVSNPOOL_HREF = "EditVSNPoolHref";
    private static final String POLICY_SUMMARY_HREF = "PolicySummaryHref";
    private static final String POLICY_DETAILS_HREF = "PolicyDetailsHref";
    private static final String COPY_VSNS_HREF = "CopyVSNsHref";
    private static final String CHILD_CONTAINER_VIEW = "MediaExpressionView";

    private CCPageTitleModel pageTitleModel;
    private CCPropertySheetModel propertySheetModel;
    private CCBreadCrumbsModel breadCrumbsModel;

    private VSNPool vsnPool = null;

    /**
     * Constructor
     */
    public VSNPoolDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);

        TraceUtil.initTrace();
        pageTitleModel = createPageTitleModel();
        propertySheetModel = createPropertySheetModel();
        registerChildren();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_CONTAINER_VIEW, MediaExpressionView.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_DETAILS_HREF, CCHref.class);
        registerChild(SHARED_FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_ARCHIVEPOL_HREF, CCHref.class);
    	registerChild(CHILD_BACKTOVSNSUM_HREF, CCHref.class);
        registerChild(CHILD_EDITVSNPOOL_HREF, CCHref.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(COPY_VSNS_HREF, CCHref.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        if (super.isChildSupported(name)) {
               return super.createChild(name);
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            return PageTitleUtil.createChild(this, pageTitleModel, name);
        } else if (PropertySheetUtil.isChildSupported(
                                       propertySheetModel, name)) {
            return PropertySheetUtil.createChild(this,
                                                 propertySheetModel, name);
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("VSNPoolDetails.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            CCBreadCrumbs child = new CCBreadCrumbs(this, breadCrumbsModel,
                                                    name);
            return child;
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            return new MediaExpressionView(this, name, true);
        } else if (name.equals(CHILD_BACKTOVSNSUM_HREF)
                || name.equals(FS_SUMMARY_HREF)
                || name.equals(FS_DETAILS_HREF)
                || name.equals(SHARED_FS_SUMMARY_HREF)
                || name.equals(FS_ARCHIVEPOL_HREF)
                || name.equals(CHILD_EDITVSNPOOL_HREF)
                || name.equals(POLICY_SUMMARY_HREF)
                || name.equals(POLICY_DETAILS_HREF)
                || name.equals(COPY_VSNS_HREF)) {
            return new CCHref(this, name, null);
        } else
            throw new IllegalArgumentException("invalid child '" + name + "'");
    }

    private CCPageTitleModel createPageTitleModel() {
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                                    "/jsp/archive/VSNPoolDetailsPageTitle.xml");
        }
        return pageTitleModel;
    }

    private CCPropertySheetModel createPropertySheetModel() {
        propertySheetModel = PropertySheetUtil.createModel(
                                    "/jsp/archive/VSNPoolDetailsPropSheet.xml");
        return propertySheetModel;
    }

    public void loadPropertySheetModel() {
        String serverName = getServerName();
        String poolName = getVSNPoolName();

        try {
            VSNPool vsnPool = getVSNPool();

            propertySheetModel.setValue("vsnPoolNameText", poolName);
            propertySheetModel.setValue(
                "mediaTypeText",
                SamUtil.getMediaTypeString(vsnPool.getMediaType()));

            Capacity freeSpace = new Capacity(vsnPool.getSpaceAvailable(),
                                              SamQFSSystemModel.SIZE_MB);
            propertySheetModel.setValue("spaceAvailableText", freeSpace);

            propertySheetModel.setValue(
                "totalVolumeText",
                Integer.toString(vsnPool.getNoOfVSNsInPool()));
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "createPropertySheetModel",
                                     "Could not retrieve VSN details",
                                     serverName);
            SamUtil.setWarningAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  sfw.getMessage());
        } catch (SamFSMultiMsgException smme) {
            SamUtil.processException(smme,
                                     this.getClass(),
                                     "createPropertySheetModel",
                                     "Could not retrieve VSN details",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error.summary",
                                  smme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "createPropertySheetModel",
                                     "Could not retrieve VSN details",
                                     serverName);

            SamUtil.setErrorAlert(
                this,
            VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
            "MediaAssignment.error.failedPopulateVSNPoolDetails",
            sfe.getSAMerrno(),
            sfe.getMessage(),
            serverName);
        }
    }


    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        loadPropertySheetModel();
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("VSNPoolDetails.title",
                                      getVSNPoolName()));
    }

    public VSNPool getVSNPool() throws SamFSException {
        if (vsnPool == null) {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            vsnPool = sysModel.getSamQFSSystemArchiveManager().
                getVSNPool(getVSNPoolName());
            if (vsnPool == null)
                throw new SamFSException(null, -2010);
        }

        return vsnPool;
    }

    private String getVSNPoolName() {
        return (String)
            getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);
    }

    public void handleVSNPoolSummaryHrefRequest(RequestInvocationEvent event) {
        String s = (String) getDisplayFieldValue(CHILD_BACKTOVSNSUM_HREF);
        ViewBean target = getViewBean(VSNPoolSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt) {
        String s = (String) getDisplayFieldValue(POLICY_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt) {
        String s = (String) getDisplayFieldValue(POLICY_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    public void handleCopyVSNsHrefRequest(RequestInvocationEvent evt) {
        String s = (String) getDisplayFieldValue(COPY_VSNS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(CopyVSNsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    // handle breadcrumb to the filesystem page
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent evt) {
        String s = (String)getDisplayFieldValue(FS_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }

    // handle breadcrumb to the filesystem details page
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent evt) {
        String s = (String)getDisplayFieldValue(FS_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
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

    // handle breadcrumb to the fs archive policies
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent evt) {
        String s = (String)getDisplayFieldValue(FS_ARCHIVEPOL_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSArchivePoliciesViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        this.forwardTo(target);
    }
}
