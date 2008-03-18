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

// ident	$Id: VSNPoolDetailsViewBean.java,v 1.33 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.web.media.VSNDetailsViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
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
import com.sun.web.ui.view.html.CCStaticTextField;
import java.io.IOException;
import java.util.StringTokenizer;
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
    private static final String CHILD_BACKTOVSNSUM_HREF = "VSNPoolSummaryHref";
    public static final String CHILD_NE_HIDDEN_FIELD1 = "NEHiddenField1";
    public static final String CHILD_NE_HIDDEN_FIELD2 = "NEHiddenField2";
    public static final String CHILD_NE_HIDDEN_FIELD3 = "NEHiddenField3";
    public static final String CHILD_NE_HIDDEN_FIELD4 = "NEHiddenField4";
    public static final String CHILD_NE_HIDDEN_FIELD5 = "NEHiddenField5";
    public static final String CHILD_POOL_NAME = "VSNPoolNameField";
    public static final String CHILD_EDITVSNPOOL_HREF =	"EditVSNPoolHref";
    public static final String CHILD_STATIC_TEXT = "StaticText";
    public static final String CHILD_CONTAINER_VIEW = "VSNPoolDetailsView";
    public static final String RESERVED_VSN_MESSAGE = "reservedVSNMessage";
    public static final String DELETE_CONFIRMATION = "deleteConfirmation";
    private static final String SERVER_NAME = "ServerName";

    private CCPageTitleModel pageTitleModel;
    private CCPropertySheetModel propertySheetModel;
    private VSNPoolDetailsModel vsnModel;
    private CCBreadCrumbsModel breadCrumbsModel;
    public boolean disableButton = false;

    /**
     * Constructor
     */
    public VSNPoolDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        propertySheetModel = createPropertySheetModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_CONTAINER_VIEW, VSNPoolDetailsView.class);
    	registerChild(CHILD_BACKTOVSNSUM_HREF, CCHref.class);
        registerChild(CHILD_NE_HIDDEN_FIELD1, CCHiddenField.class);
        registerChild(CHILD_NE_HIDDEN_FIELD2, CCHiddenField.class);
        registerChild(CHILD_NE_HIDDEN_FIELD3, CCHiddenField.class);
        registerChild(CHILD_NE_HIDDEN_FIELD4, CCHiddenField.class);
        registerChild(CHILD_NE_HIDDEN_FIELD5, CCHiddenField.class);
        registerChild(CHILD_POOL_NAME, CCHiddenField.class);
        registerChild(CHILD_EDITVSNPOOL_HREF, CCHref.class);
        registerChild(CHILD_STATIC_TEXT, CCStaticTextField.class);
        registerChild(RESERVED_VSN_MESSAGE, CCStaticTextField.class);
        registerChild(DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
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
        } else if (name.equals(CHILD_STATIC_TEXT) ||
                   name.equals(RESERVED_VSN_MESSAGE)) {
            return new CCStaticTextField(this, name, null);

        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("VSNPoolDetails.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            CCBreadCrumbs child = new CCBreadCrumbs(this, breadCrumbsModel,
                                                    name);
            return child;
        } else if (name.equals(CHILD_CONTAINER_VIEW)) {
            return new VSNPoolDetailsView(this, name);
        } else if (name.equals(CHILD_BACKTOVSNSUM_HREF)
                   || name.equals(CHILD_EDITVSNPOOL_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(CHILD_NE_HIDDEN_FIELD1)
                   || name.equals(CHILD_NE_HIDDEN_FIELD2)
                   || name.equals(CHILD_NE_HIDDEN_FIELD3)
                   || name.equals(CHILD_NE_HIDDEN_FIELD4)
                   || name.equals(CHILD_NE_HIDDEN_FIELD5)
                   || name.equals(CHILD_POOL_NAME)
                   || name.equals(DELETE_CONFIRMATION)
                   || name.equals(SERVER_NAME)) {
            CCHiddenField child = new CCHiddenField
                (this, name, null);
            TraceUtil.trace3("Exiting");
            return child;
        } else
            throw new IllegalArgumentException("invalid child '" + name + "'");
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                                    "/jsp/archive/VSNPoolDetailsPageTitle.xml");
            pageTitleModel.setValue("Edit", "common.edit");
            pageTitleModel.setValue("Delete", "common.delete");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");

        propertySheetModel = PropertySheetUtil.createModel(
                                    "/jsp/archive/VSNPoolDetailsPropSheet.xml");

        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    public void loadPropertySheetModel() {
        String serverName = getServerName();
        String poolName =
            (String)getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            VSNPool vsnPool = sysModel.getSamQFSSystemArchiveManager().
                getVSNPool(poolName);
            if (vsnPool == null)
                throw new SamFSException(null, -2010);

            propertySheetModel.setValue("vsnPoolNameText", poolName);
            propertySheetModel.setValue("mediaTypeText",
                            SamUtil.getMediaTypeString(vsnPool.getMediaType()));

            Capacity freeSpace = new Capacity(vsnPool.getSpaceAvailable(),
                                              SamQFSSystemModel.SIZE_MB);
            propertySheetModel.setValue("spaceAvailableText", freeSpace);

            propertySheetModel.setValue("regexpText",
                                        vsnPool.getVSNExpression());
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

            SamUtil.setErrorAlert(this,
                                  VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
                                  "VSNPoolDetails.error." +
                                  "failedPopulateVSNPoolDetails",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
            disableButton = true;
        }
    }


    public void handleVSNPoolSummaryHrefRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        String s = (String) getDisplayFieldValue("VSNPoolSummaryHref");
        ViewBean target = getViewBean(VSNPoolSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
        TraceUtil.trace3("Exiting");
    }

    // handle edit
    public void handleEditRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        this.forwardTo();
        TraceUtil.trace3("Exiting");
    }

    // handle delete
    public void handleDeleteRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String serverName = getServerName();
        String poolName =
            (String)getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            sysModel.getSamQFSSystemArchiveManager().deleteVSNPool(poolName);
            // sleep for 5 seconds to allow sam-amld to start

            // TODO: still needed for 4.3 servers
            try {
                Thread.sleep(5000);
            }
            catch (InterruptedException iex) {
                TraceUtil.trace3("InterruptedException Caught: Reason: " +
                                 iex.getMessage());
            }

            CommonViewBeanBase target =
                (CommonViewBeanBase)getViewBean(VSNPoolSummaryViewBean.class);

            showAlert(target, "VSNPoolSummary.delete.alert", poolName);
            // forward to the parent as there is nothing more to show
            // if a pool is deleted
            forwardTo(target);
        }
        catch (SamFSException smfex) {

            String processMsg = null;
            String errMsg = null;
            String errCause = null;

            boolean multiMsgOccurred = false;
            boolean warningOccurred = false;

            if (smfex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.error.detail";
                multiMsgOccurred = true;
            } else if (smfex instanceof SamFSWarnings) {
                warningOccurred = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.warning.detail";
            } else {
                processMsg = "Failed to delete VSN Pool";
                errMsg = "VSNPoolDetails.error.failedDelete";
                errCause = smfex.getMessage();
            }

            SamUtil.processException(smfex, this.getClass(),
                                     "handleDeleteRequest",
                                     processMsg, serverName);

            // if the vsn pool was successfully deleted, don't stay
            // on this page, since there is nothing to show.
            if (!multiMsgOccurred && !warningOccurred) {

                SamUtil.setErrorAlert(this,
                                     VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
                                      errMsg,
                                      smfex.getSAMerrno(),
                                      errCause,
                                      serverName);
                this.forwardTo();
            } else {
                ViewBean target = getViewBean(VSNPoolSummaryViewBean.class);
                if (multiMsgOccurred)
                    SamUtil.setErrorAlert(
                        target,
                        VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
                        errMsg,
                        smfex.getSAMerrno(),
                        errCause,
                        serverName);
                else if (warningOccurred)
                    SamUtil.setWarningAlert(target,
                                    VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
                                            errMsg,
                                            errCause);

                // sleep for 5 seconds to allow sam-amld to start
                try {
                    Thread.sleep(5000);
                }
                catch (InterruptedException iex) {
                    TraceUtil.trace3("InterruptedException Caught: Reason: " +
                                     iex.getMessage());
                }
                forwardTo(target);
            }
        }
        TraceUtil.trace3("Exiting");
    }

    private void checkRolePrivilege() {
        TraceUtil.trace3("Entering");

    	if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {

            ((CCButton)getChild("Edit")).setDisabled(true);
            ((CCButton)getChild("Delete")).setDisabled(true);
        }

        TraceUtil.trace3("Exiting");
    }

    private void disableButtons() {
        ((CCButton)getChild("Edit")).setDisabled(true);
        ((CCButton)getChild("Delete")).setDisabled(true);
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        checkRolePrivilege();
        loadPropertySheetModel();

        String poolName =
            (String)getPageSessionAttribute(Constants.Archive.VSN_POOL_NAME);

        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString("VSNPoolDetails.title", poolName));

        VSNPoolDetailsView view =
            (VSNPoolDetailsView)getChild(CHILD_CONTAINER_VIEW);

        String serverName = getServerName();
        ((CCHiddenField)getChild(SERVER_NAME)).setValue(serverName);

        try {
            view.populateData();

            // disable the delete button if pool is in use
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            boolean inuse = sysModel.
                getSamQFSSystemArchiveManager().isPoolInUse(poolName);

            if (inuse) {
                ((CCButton)getChild("Delete")).setDisabled(true);
            }
        }
        catch (SamFSException smfex) {
            SamUtil.processException(smfex, this.getClass(),
                                     "VSNPoolDetailsViewBean()",
                                     "Cannot populate vsns", serverName);

            SamUtil.setErrorAlert(this,
                                  VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
                        "VSNPoolDetails.error.failedPopulateVSNPoolDetails",
                                  smfex.getSAMerrno(),
                                  smfex.getMessage(), serverName);
        }

        if (disableButton)
            disableButtons();
        disableButton = false;

        ((CCHiddenField) getChild("VSNPoolNameField")).setValue(poolName);

        Boolean containsReservedVSN =
            (Boolean)RequestManager.getRequestContext().getRequest().
            getAttribute(Constants.Archive.CONTAINS_RESERVED_VSN);

        if (containsReservedVSN != null
            && containsReservedVSN.booleanValue()) {
            String msg =
                SamUtil.getResourceString("archiving.reservedvsninpool",
                                          Constants.Symbol.DAGGER);
            CCStaticTextField child =
                (CCStaticTextField)getChild(RESERVED_VSN_MESSAGE);
            child.setValue(msg);
        }

        // set delete confirmation msg
        ((CCHiddenField)getChild(DELETE_CONFIRMATION)).setValue(
            SamUtil.getResourceString("VSNPoolSummary.confirmMsg1"));
        TraceUtil.trace3("Exiting");
    }

    /**
     * handler for the edit button
     */
    public void handleEditVSNPoolHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        String start  = null, end = null;
        // value1 holds the NAME field
        // value2 holds the drop down menu value
        // value3 holds both the start & end value,
        // read below how to parse it
        // value4 holds the range
        // value5 holds the radio button value

        String vsnName = (String) getDisplayFieldValue(CHILD_NE_HIDDEN_FIELD1);
        TraceUtil.trace3("vsnName is " + vsnName);
        if (vsnName != null)
            vsnName = vsnName.trim();

        String mediaType = (String)
		getDisplayFieldValue(CHILD_NE_HIDDEN_FIELD2);
        TraceUtil.trace3("mediaType is " + mediaType);
        if (mediaType != null)
            mediaType = mediaType.trim();

        String startend = (String)
            getDisplayFieldValue(CHILD_NE_HIDDEN_FIELD3);
        TraceUtil.trace3("startend is " + startend);
        if (startend != null)
            startend = startend.trim();

        String vsnRange = (String)
            getDisplayFieldValue(CHILD_NE_HIDDEN_FIELD4);
        TraceUtil.trace3("vsnRange is " + vsnRange);
        if (vsnRange != null)
            vsnRange = vsnRange.trim();

        String specifyVSN = (String)
            getDisplayFieldValue(CHILD_NE_HIDDEN_FIELD5);
        if (specifyVSN != null)
            specifyVSN = specifyVSN.trim();

        if (specifyVSN != null && specifyVSN.equals("startend")) {
            StringTokenizer tokens = new StringTokenizer(startend, ",");
            start = tokens.nextToken();
            end   = tokens.nextToken();
        }
        String express = null;
        if (specifyVSN != null && specifyVSN.equals("range"))
            express = vsnRange;
        else if (specifyVSN != null && specifyVSN.equals("startend")) {
            // make up a regular express here
            // express = start + "-" + end;
            express = SamQFSUtil.createExpression(start, end);
        }

        int media = Integer.parseInt(mediaType);

        String serverName = getServerName();
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            VSNPool existedPool = null;
            VSNPool[] pools = sysModel.
                getSamQFSSystemArchiveManager().getAllVSNPools();
            for (int i = 0; i < pools.length; i++) {
                if (pools[i].getPoolName().equals(vsnName)) {
                    existedPool = pools[i];
                }
            }
            LogUtil.info(this.getClass(),
                         "handleEditVSNPoolHrefRequest",
                         "Start editing VSN pool " + vsnName);

            existedPool.setMemberVSNs(media, express);
            sysModel.getSamQFSSystemArchiveManager().
                updateVSNPool(vsnName);

            LogUtil.info(this.getClass(),
                         "handleEditVSNPoolHrefRequest",
                         "Done editing VSN pool " + vsnName);
            showAlertDetail("VSNPoolDetails.edit.alert", vsnName);
        } catch (SamFSException smfex) {

            String processMsg = null;
            String errMsg = null;
            String errCause = null;
            boolean warning = false;
            if (smfex instanceof SamFSMultiMsgException) {
                processMsg = Constants.Config.ARCHIVE_CONFIG;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.error.detail";
            } else if (smfex instanceof SamFSWarnings) {
                warning = true;
                processMsg = Constants.Config.ARCHIVE_CONFIG_WARNING;
                errMsg = "ArchiveConfig.error";
                errCause = "ArchiveConfig.warning.detail";
            } else {
                processMsg = "Failed to edit VSN Pool";
                errMsg = "VSNPoolDetails.error.failedEdit";
                errCause = smfex.getMessage();
            }

            SamUtil.processException(smfex, this.getClass(),
                                     "handleEditVSNPoolHrefRequest",
                                     processMsg, serverName);
            if (!warning)
                SamUtil.setErrorAlert(this,
                                     VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
                                      errMsg,
                                      smfex.getSAMerrno(),
                                      errCause, serverName);
            else
                SamUtil.setWarningAlert(this,
                                    VSNPoolDetailsViewBean.CHILD_COMMON_ALERT,
                                        errMsg,
                                        errCause);

        }
        // sleep for 5 seconds to allow sam-amld to start
        try {
            Thread.sleep(5000);
        }
        catch (InterruptedException iex) {
            TraceUtil.trace3("InterruptedException Caught: Reason: " +
                             iex.getMessage());
        }
        this.forwardTo();
        TraceUtil.trace3("Exiting");
    }

    private void showAlert(ViewBean targetView, String operation, String key) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(targetView,
                             VSNPoolSummaryViewBean.CHILD_COMMON_ALERT,
                             "success.summary",
                             SamUtil.getResourceString(operation, key),
                             getServerName());

        TraceUtil.trace3("Exiting");
    }

    private void showAlertDetail(String operation, String key) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(this,
                             VSNDetailsViewBean.CHILD_COMMON_ALERT,
                             "success.summary",
                             SamUtil.getResourceString(operation, key),
                             getServerName());
        TraceUtil.trace3("Exiting");
    }

}
