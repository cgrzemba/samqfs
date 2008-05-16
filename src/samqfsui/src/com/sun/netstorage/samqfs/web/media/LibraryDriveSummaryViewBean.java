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

// ident	$Id: LibraryDriveSummaryViewBean.java,v 1.23 2008/05/16 18:38:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.server.ServerUtil;
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
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;

import java.io.IOException;

import javax.servlet.ServletException;


/**
 *  This class is the view bean for the Library Drive Summary page
 */

public class LibraryDriveSummaryViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "LibraryDriveSummary";

    public static final String BREADCRUMB = "BreadCrumb";
    public static final String LIBRARY_SUMMARY_HREF = "LibrarySummaryHref";

    // cc components from the corresponding jsp page(s)...
    public static final String CONTAINER_VIEW = "LibraryDriveSummaryView";
    public static final String LIBRARY_NAME   = "LibraryName";

    // serverName hidden field for pop ups
    public static final String SERVER_NAME    = "ServerName";

    // hidden field for confirm message
    public static final String CONFIRM_MESSAGE = "ConfirmMessage";

    public static final String DRIVER = "Driver";
    public static final String DRIVERS_STRING   = "DriversString";

    // Page Title / Property sheet Attributes and Components.
    private CCPageTitleModel pageTitleModel  = null;
    private CCPropertySheetModel propertySheetModel  = null;

    // For BreadCrumb
    private CCBreadCrumbsModel breadCrumbsModel = null;

    /**
     * Constructor
     */
    public LibraryDriveSummaryViewBean() {
        super(PAGE_NAME, "/jsp/media/LibraryDriveSummary.jsp");
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
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(LIBRARY_SUMMARY_HREF, CCHref.class);
        registerChild(LIBRARY_NAME, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(DRIVER, CCHiddenField.class);
        registerChild(DRIVERS_STRING, CCHiddenField.class);
        registerChild(CONFIRM_MESSAGE, CCHiddenField.class);
        registerChild(CONTAINER_VIEW, LibraryDriveSummaryView.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
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
            child = new LibraryDriveSummaryView(this, name);
        } else if (name.equals(LIBRARY_NAME) ||
            name.equals(SERVER_NAME) ||
            name.equals(DRIVERS_STRING) ||
            name.equals(CONFIRM_MESSAGE) ||
            name.equals(DRIVER)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("LibraryDriveSummary.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(LIBRARY_SUMMARY_HREF)) {
            child = new CCHref(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        // PropertySheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");

        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/LibraryDetailsPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private void loadPageTitleModel(CCPageTitleModel pageTitleModel) {
        TraceUtil.trace3("Entering");
        pageTitleModel.setPageTitleText(
            SamUtil.getResourceString(
                "LibraryDriveSummary.pageTitle1", getLibraryName()));
        TraceUtil.trace3("Exiting");
    }

    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/media/LibraryDetailsPropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    private void loadPropertySheetModel(
        CCPropertySheetModel propertySheetModel) {
        TraceUtil.trace3("Entering");

        propertySheetModel.clear();

        // Fill it the property sheet values
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            Library myLibrary = sysModel.getSamQFSSystemMediaManager().
                getLibraryByName(getLibraryName());
            if (myLibrary == null) {
                throw new SamFSException(null, -2502);
            }

            int alarmInfo[] = null;

            propertySheetModel.setValue(
                "vendorValue",
                myLibrary.getVendor());
            propertySheetModel.setValue(
                "productValue",
                myLibrary.getProductID());
            propertySheetModel.setValue(
                "eqValue",
                new Integer(myLibrary.getEquipOrdinal()));
            propertySheetModel.setValue(
                "firmwareValue",
                myLibrary.getFirmwareLevel());
            propertySheetModel.setValue(
                "catalogValue",
                myLibrary.getCatalogLocation());
            propertySheetModel.setValue(
                "capacityValue",
                new Capacity(
                    myLibrary.getTotalCapacity(),
                    SamQFSSystemModel.SIZE_MB).toString());
            propertySheetModel.setValue(
                "freespaceValue",
                new Capacity(
                    myLibrary.getTotalFreeSpace(),
                    SamQFSSystemModel.SIZE_MB).toString());

            // Only show serial number if library is a SAMST library
            propertySheetModel.setValue(
                "serialValue",
                myLibrary.getDriverType() == Library.SAMST ?
                myLibrary.getSerialNo() : "");

            if (myLibrary.getTotalCapacity() != 0) {
                String percent = Long.toString(
                    (long) 100 * (myLibrary.getTotalCapacity() -
                    myLibrary.getTotalFreeSpace()) /
                    myLibrary.getTotalCapacity());
                propertySheetModel.setValue(
                    "spaceconsumedValue",
                    percent);
            } else {
                propertySheetModel.setValue(
                    "spaceconsumedValue",
                    "0");
            }

            propertySheetModel.setValue(
                "typeValue",
                SamUtil.getMediaTypeString(myLibrary.getMediaType()));

            Alarm allAlarms[] = myLibrary.getAssociatedAlarms();
            propertySheetModel.setValue(
                "noAlarm",
                "");

            if (allAlarms == null || allAlarms.length == 0) {
                propertySheetModel.setValue(
                    "Alarm",
                    "");
            } else {
                alarmInfo = SamUtil.getAlarmInfo(allAlarms);
                propertySheetModel.setValue(
                    "Alarm",
                    MediaUtil.getAlarm(alarmInfo[0]));
            }

            if (myLibrary.getDrives() != null) {
                propertySheetModel.setValue(
                    "devicesValue",
                    new Integer(myLibrary.getDrives().length));
            } else {
                propertySheetModel.setValue(
                    "devicesValue",
                    new Integer(0));
            }

            // Messages
            String [] messages = myLibrary.getMessages();

            if (messages == null || messages.length == 0) {
                propertySheetModel.setValue(
                    "statusMsgValue",
                    "");
            } else {
                NonSyncStringBuffer strBuf = new NonSyncStringBuffer();
                for (int i = 0; i < messages.length; i++) {
                    if (i != 0) {
                        strBuf.append("<br>");
                    } else {
                        strBuf.append(messages[i]);
                    }
                }
                propertySheetModel.setValue(
                    "statusMsgValue",
                    strBuf.toString());
            }

            // Status Information
            int [] statusInfoCodes = myLibrary.getDetailedStatus();

            if (statusInfoCodes == null || statusInfoCodes.length == 0) {
                propertySheetModel.setValue(
                    "statusInfoValue",
                    "");
            } else {
                NonSyncStringBuffer strBuf = new NonSyncStringBuffer();
                for (int i = 0; i < statusInfoCodes.length; i++) {
                    if (i != 0) {
                        strBuf.append("<br>");
                    } else {
                        if (statusInfoCodes[i] == SamQFSSystemMediaManager.
                            RM_STATUS_AUTO_LIB_ALL_SLOTS_OCCUPIED) {
                            // use 10017a (Library)
                            strBuf.append(SamUtil.getResourceString(
                                new NonSyncStringBuffer(
                                Integer.toString(statusInfoCodes[i])).
                                append("a").toString()));
                        } else {
                            strBuf.append(SamUtil.getResourceString(
                                Integer.toString(statusInfoCodes[i])));
                        }
                    }
                }
                propertySheetModel.setValue(
                    "statusInfoValue",
                    strBuf.toString());
            }
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "loadPropertySheet",
                "Failed to retrieve Library information",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CHILD_COMMON_ALERT,
                "LibraryDetails.error.loadpsheet",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }


    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        checkRolePrivilege();

        // set the drop-down menu to default value
        ((CCDropDownMenu) getChild("ActionMenu")).setValue("0");

        // populate the page title
        loadPageTitleModel(pageTitleModel);
        // populate the action table model
        LibraryDriveSummaryView view = (LibraryDriveSummaryView)
            getChild(CONTAINER_VIEW);
        try {
            loadPropertySheetModel(propertySheetModel);
            view.populateData(getServerName(), getLibraryName());
            setHiddenFields();
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "LibraryDriveSummaryViewBean()",
                "Failed to populate library drive summary",
                getServerName());
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "LibraryDriveSummary.error.populate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * checkRolePriviledge() checks if the user is a valid Admin
     * disable all action buttons / dropdown if not
     */
    private void checkRolePrivilege() {
        TraceUtil.trace3("Entering");
        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {
            ((CCButton) getChild("ImportButton")).setDisabled(false);
        }

        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            ((CCDropDownMenu) getChild("ActionMenu")).setDisabled(false);
        }
        TraceUtil.trace3("Exiting");
    }

    private void setHiddenFields() throws SamFSException {
        ((CCHiddenField) getChild(SERVER_NAME)).setValue(getServerName());
        ((CCHiddenField) getChild(LIBRARY_NAME)).setValue(getLibraryName());
        ((CCHiddenField) getChild(CONFIRM_MESSAGE)).setValue(
            new StringBuffer(
                SamUtil.getResourceString("media.confirm.delete.library")).
                append("###").append(
                SamUtil.getResourceString("media.confirm.unload.library")).
                append("###").append(
                SamUtil.getResourceString("media.confirm.unload.drive")).
                toString());
        ((CCHiddenField) getChild(DRIVERS_STRING)).setValue(
            new NonSyncStringBuffer(
                SamUtil.getResourceString("library.driver.samst")).
                    append(ServerUtil.delimitor).
                    append(SamUtil.getResourceString("library.driver.acsls")).
                    toString());
        ((CCHiddenField) getChild(DRIVER)).setValue(
            SamUtil.getLibraryDriverString(
                SamUtil.getModel(getServerName()).getSamQFSSystemMediaManager().
                    getLibraryByName(getLibraryName()).getDriverType()));
    }

    private String getLibraryName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
    }

    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String str = (String) getDisplayFieldValue(LIBRARY_SUMMARY_HREF);
        ViewBean targetView = getViewBean(LibrarySummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SHARE_CAPABLE);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);
        TraceUtil.trace3("Exiting");
    }

    public void handleAlarmsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        ViewBean targetViewBean =
            getViewBean(LibraryFaultsSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));
        forwardTo(targetViewBean);
        TraceUtil.trace3("Exiting");
    }

    /**
     * View VSN
     * Forward to VSN Summary Page or Historian Page
     */
    public void handleViewVSNButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        ViewBean targetView = getViewBean(VSNSummaryViewBean.class);
        getParentViewBean().setPageSessionAttribute(
            Constants.PageSessionAttributes.FROM_TREE,
            new Boolean(false));
        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));
        ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetView);
        TraceUtil.trace3("Existing");
    }

    /**
     *
     * Import
     *
     */
    public void handleImportButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String libName = (String) getDisplayFieldValue(LIBRARY_NAME);
        String driver  = (String) getDisplayFieldValue(DRIVER);

        if (driver.equals(
            SamUtil.getResourceString("library.driver.samst"))) {
            try {
                Library myLibrary = MediaUtil.getLibraryObject(
                    getServerName(), libName);

                if (driver.equals(
                    SamUtil.getResourceString("library.driver.samst"))) {
                    // Driver is SAMST
                    LogUtil.info(this.getClass(), "handleImportButtonRequest",
                        new NonSyncStringBuffer().append("Start importing in ").
                        append(libName).toString());
                    myLibrary.importVSN();
                    LogUtil.info(this.getClass(), "handleImportButtonRequest",
                        new NonSyncStringBuffer().append("Done importing ").
                        append(libName).toString());
                    setSuccessAlert(
                        getParentViewBean(),
                        "LibrarySummary.action.import", libName);
                }

            } catch (SamFSException samEx) {
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "handleImportButtonRequest",
                    "Failed to import VSN",
                    getServerName());
                SamUtil.setErrorAlert(
                    getParentViewBean(),
                    CommonViewBeanBase.CHILD_COMMON_ALERT,
                    "LibrarySummary.error.import",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            }
            getParentViewBean().forwardTo(getRequestContext());
        } else {
            // 4.5 ACSLS
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.LIBRARY_NAME,
                libName);
            ViewBean targetView = getViewBean(ImportVSNViewBean.class);

            BreadCrumbUtil.breadCrumbPathForward(
                getParentViewBean(),
                PageInfo.getPageInfo().getPageNumber(
                    getParentViewBean().getName()));

            ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetView);
        }

        TraceUtil.trace3("Existing");
    }

    /**
     * Action Menu of Library Details Page
     * 0 -> -- Operation --
     * 1 -> Unload
     * 2 -> Remove
     */
    public void handlePageActionsMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String libName = (String) getDisplayFieldValue(LIBRARY_NAME);

        int value = 0;
        String op = null;

        try {
            value = Integer.parseInt(
                (String) getDisplayFieldValue("ActionMenu"));
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("NumEx caught while parsing drop down selection!");
        }

        try {
            Library myLibrary = MediaUtil.getLibraryObject(
                                    getServerName(), libName);
            switch (value) {
                // Unload
                case 2:
                    op = "LibrarySummary.action.unload";
                    LogUtil.info(this.getClass(),
                        "handlePageActionsMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Start unloading tape in ").
                            append(libName).toString());
                    myLibrary.unload();
                    LogUtil.info(this.getClass(),
                        "handlePageActionsMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Done unloading tape in ").
                            append(libName).toString());
                    setSuccessAlert(getParentViewBean(), op, libName);
                    break;

                // Delete
                case 3:
                    op = "LibrarySummary.action.remove";
                    LogUtil.info(this.getClass(),
                        "handlePageActionsMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Start removing library ").
                            append(libName).toString());
                    SamQFSSystemModel sysModel =
                        SamUtil.getModel(getServerName());
                    sysModel.getSamQFSSystemMediaManager().
                        removeLibrary(myLibrary);
                    LogUtil.info(this.getClass(),
                        "handlePageActionsMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Done removing library ").
                            append(libName).toString());
                    setSuccessAlert(
                        getViewBean(LibrarySummaryViewBean.class), op, libName);

                    removePageSessionAttribute(
                        Constants.PageSessionAttributes.LIBRARY_NAME);
                    ViewBean targetView =
                        getViewBean(LibrarySummaryViewBean.class);
                    removePageSessionAttribute(
                        Constants.SessionAttributes.PAGE_PATH);
                    ((CommonViewBeanBase)
                        getParentViewBean()).forwardTo(targetView);
                    return;
            }

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handlePageActionsMenuHrefRequest",
                new NonSyncStringBuffer("Failed to ").
                    append(op).append(" ").append(libName).toString(),
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                (value == 2) ?
                    "LibrarySummary.error.unload" :
                    "LibrarySummary.error.remove",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        // set the drop-down menu to default value
        ((CCDropDownMenu) getChild("ActionMenu")).setValue("0");
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    private void setSuccessAlert(ViewBean vb, String msg, String item) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            vb,
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(msg, item),
            getServerName());
        TraceUtil.trace3("Exiting");
    }
}
