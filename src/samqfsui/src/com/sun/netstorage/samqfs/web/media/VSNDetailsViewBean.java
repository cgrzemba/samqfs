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

// ident	$Id: VSNDetailsViewBean.java,v 1.56 2008/12/16 00:12:14 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.HtmlUtil;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.BasicCommandField;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.archive.VSNPoolDetailsViewBean;
import com.sun.netstorage.samqfs.web.archive.VSNPoolSummaryViewBean;
import com.sun.netstorage.samqfs.web.media.wizards.ReservationImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
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
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.wizard.CCWizardWindow;

import java.io.IOException;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the VSNDetails page
 */

public class VSNDetailsViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "VSNDetails";
    private static final String DEFAULT_DISPLAY_URL =
        "/jsp/media/VSNDetails.jsp";

    public static final String CHILD_BREADCRUMB = "BreadCrumb";
    public static final String CHILD_PAGEACTIONMENU_HREF = "PageActionMenuHref";
    public static final String CHILD_LIB_SUMMARY_HREF    = "LibrarySummaryHref";
    public static final
        String CHILD_LIB_DRIVE_SUMMARY_HREF = "LibraryDriveSummaryHref";

    public static final String CHILD_VSN_SUMMARY_HREF = "VSNSummaryHref";
    public static final String CHILD_HISTORIAN_HREF   = "HistorianHref";
    public static final String CHILD_VSN_SEARCH_HREF  = "VSNSearchHref";
    public static final
        String CHILD_VSN_SEARCH_RESULT_HREF = "VSNSearchResultHref";
    public static final
        String CHILD_VSN_POOL_SUMMARY_HREF  = "VSNPoolSummaryHref";
    public static final
        String CHILD_VSN_POOL_DETAILS_HREF  = "VSNPoolDetailsHref";

    // Hidden fields for javascript
    public static final String CHILD_LIBNAME = "LibraryNameHiddenField";
    public static final String CHILD_SLOTNUMBER = "slotNumber";
    public static final String CHILD_HIDDEN_MESSAGE = "HiddenMessage";

    public static final String ROLE = "Role";
    public static final String CHILD_FRWD_TO_CMDCHILD = "forwardToVb";

    // Hiddenfield for Server Name
    public static final
        String CHILD_SERVER_NAME_HIDDEN_FIELD = "ServerNameHiddenField";

    // Hidden field to indicate if VSN is loaded in a drive (for javascript)
    public static final String CHILD_IS_VSN_IN_DRIVE = "isVSNInDrive";

    private boolean wizardLaunched = false;
    private CCWizardWindowModel wizWinModel;
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;
    private CCBreadCrumbsModel breadCrumbsModel = null;

    private String libraryName = null, slotNumber = null;
    private boolean
        comeFromVSNSearch = false, comeFromVSNSearchResult = false,
        isVSNInDrive = false;

    private static final int OP_AUDIT  = 1;
    private static final int OP_LOAD   = 2;
    private static final int OP_EXPORT = 3;
    private static final int OP_UNRESERVED = 10;

    /**
     * Constructor
     */
    public VSNDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        createPageTitleModel();
        createPropertySheetModel();
        initializeWizard();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        super.registerChildren();
        registerChild(ROLE, CCHiddenField.class);
        registerChild(CHILD_BREADCRUMB, CCBreadCrumbs.class);
        registerChild(CHILD_LIB_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_LIB_DRIVE_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_VSN_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_HISTORIAN_HREF, CCHref.class);
        registerChild(CHILD_VSN_SEARCH_HREF, CCHref.class);
        registerChild(CHILD_VSN_SEARCH_RESULT_HREF, CCHref.class);
        registerChild(CHILD_VSN_POOL_SUMMARY_HREF, CCHref.class);
        registerChild(CHILD_VSN_POOL_DETAILS_HREF, CCHref.class);
        registerChild(CHILD_PAGEACTIONMENU_HREF, CCHref.class);
        registerChild(CHILD_HIDDEN_MESSAGE, CCHiddenField.class);
        registerChild(CHILD_LIBNAME, CCHiddenField.class);
        registerChild(CHILD_SLOTNUMBER, CCHiddenField.class);
        registerChild(CHILD_IS_VSN_IN_DRIVE, CCHiddenField.class);
        registerChild(CHILD_FRWD_TO_CMDCHILD, BasicCommandField.class);
        registerChild(CHILD_SERVER_NAME_HIDDEN_FIELD, CCHiddenField.class);
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
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());
        View child = null;

        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(ROLE)) {
            child = new CCHiddenField(this, name, null);
        // static text for javascript messages
        } else if (name.equals(CHILD_HIDDEN_MESSAGE)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString("media.confirm.unreserve.vsn")).
                    append("###").append(
                    SamUtil.getResourceString("media.confirm.export.vsn")).
                    toString());
        } else if (name.equals(CHILD_BREADCRUMB)) {
            breadCrumbsModel =
                new CCBreadCrumbsModel("VSNDetails.pageTitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, breadCrumbsModel);
            child = new CCBreadCrumbs(this, breadCrumbsModel, name);
        } else if (name.equals(CHILD_LIB_SUMMARY_HREF) ||
            name.equals(CHILD_PAGEACTIONMENU_HREF) ||
            name.equals(CHILD_LIB_DRIVE_SUMMARY_HREF) ||
            name.equals(CHILD_VSN_SUMMARY_HREF) ||
            name.equals(CHILD_HISTORIAN_HREF) ||
            name.equals(CHILD_VSN_SEARCH_HREF) ||
            name.equals(CHILD_VSN_SEARCH_RESULT_HREF) ||
            name.equals(CHILD_VSN_POOL_SUMMARY_HREF) ||
            name.equals(CHILD_VSN_POOL_DETAILS_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(CHILD_LIBNAME) ||
            name.equals(CHILD_SLOTNUMBER) ||
            name.equals(CHILD_IS_VSN_IN_DRIVE)) {
            child = new CCHiddenField(this, name, null);
        // For server name hidden field
        } else if (name.equals(CHILD_SERVER_NAME_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name, getServerName());
        } else if (name.equals(CHILD_FRWD_TO_CMDCHILD)) {
            child = new BasicCommandField(this, name);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        // PropertySheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
        } else {
            TraceUtil.trace3("Exiting");
            throw new IllegalArgumentException(new NonSyncStringBuffer(
                "Invalid child name [").append(name).append("]").toString());
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
        checkRolePrivilege();
        loadPropertySheetModel();
        setResWizardState();

        // set the hidden field "LibName" with the libraryName
        // This is used by a javascript function
        ((CCHiddenField) getChild(
            "LibraryNameHiddenField")).setValue(libraryName);
        ((CCHiddenField) getChild(CHILD_IS_VSN_IN_DRIVE)).
            setValue(Boolean.toString(isVSNInDrive));
        ((CCHiddenField) getChild("slotNumber")).setValue(slotNumber);

        disableNonApplicableItems();
        resetDropDownMenu();

        TraceUtil.trace3("Exiting");
    }

    /**
     * resetDropDownMenu()
     * To reset Drop Down Menu to "-- Operation --"
     */
    private void resetDropDownMenu() {
        ((CCDropDownMenu) getChild("PageActionMenu")).setValue("0");
    }

    /**
     * disableNonApplicableItems()
     * To disable buttons and drop down items if needed
     * Note:
     * if only one item in the drop down needs to be disabled,
     * the disabling of that item will happen in Javascript
     * since we can only disable the entire dropdown.
     */

    private void disableNonApplicableItems() {
        TraceUtil.trace3("Entering");
        // Get the Library Name
        // if the name is Historian, the VSN is in the Historian
        // Otherwise, the VSN is in a tape library. No disable is needed.
        if (libraryName.equals(
            Constants.MediaAttributes.HISTORIAN_NAME)) {
            ((CCButton) getChild("LabelPageActionButton")).setDisabled(true);
        }
        TraceUtil.trace3("Exiting");
    }

    private void createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/media/VSNDetailsPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    private void createPropertySheetModel() {
        TraceUtil.trace3("Entering");
        if (propertySheetModel == null)  {
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/media/VSNDetailsPropertySheet.xml");
        }
        TraceUtil.trace3("Exiting");
    }

    private void loadPropertySheetModel() {
        TraceUtil.trace3("Entering");
        propertySheetModel.clear();

        // Fill it the property sheet values
        try {
            VSN myVSN = getSelectedVSN();

            if (myVSN != null) {
                propertySheetModel.setValue(
                    "slotValue",
                    slotNumber);
                propertySheetModel.setValue(
                    "vsnValue",
                    myVSN.getVSN());
                propertySheetModel.setValue(
                    "barcodeValue",
                    myVSN.getBarcode());
                propertySheetModel.setValue(
                    "capacityValue",
                    new Capacity(
                        myVSN.getCapacity(),
                        SamQFSSystemModel.SIZE_MB).toString());
                propertySheetModel.setValue(
                    "freespaceValue",
                    new Capacity(
                        myVSN.getAvailableSpace(),
                        SamQFSSystemModel.SIZE_MB).toString());

                if (myVSN.getCapacity() != 0) {
                    propertySheetModel.setValue(
                        "spaceconsumedValue",
                        new Long(
                            (long) 100 * (myVSN.getCapacity() -
                                myVSN.getAvailableSpace()) /
                                myVSN.getCapacity()));
                } else {
                    propertySheetModel.setValue(
                        "spaceconsumedValue",
                        new Long(0));
                }
                propertySheetModel.setValue(
                    "blocksizeValue",
                    new Long(myVSN.getBlockSize()));
                propertySheetModel.setValue(
                    "accesscountValue",
                    new Long(myVSN.getAccessCount()));

                propertySheetModel.setValue(
                    "labeltimeValue",
                    SamUtil.getTimeString(myVSN.getLabelTime()));

                propertySheetModel.setValue(
                    "mounttimeValue",
                    SamUtil.getTimeString(myVSN.getMountTime()));

                propertySheetModel.setValue(
                    "moditimeValue",
                    SamUtil.getTimeString(myVSN.getModificationTime()));

                propertySheetModel.setValue(
                    "reservationValue",
                    myVSN.isReserved() ? "VSNDetails.yes" : "VSNDetails.no");

                propertySheetModel.setValue(
                    "timeValue",
                    myVSN.isReserved() ?
                        SamUtil.getTimeString(myVSN.getReservationTime()) :
                        "");

                NonSyncStringBuffer reservedBy = new NonSyncStringBuffer();
                if (myVSN.isReserved()) {
                    String policyName = myVSN.getReservedByPolicyName();
                    if (policyName != null && policyName.length() > 0) {
                        reservedBy.append(SamUtil.getResourceString(
                            "VSNDetails.reservebyap")).append(" ");
                        reservedBy.append(policyName);
                        reservedBy.append(";");
                    }

                    String fsName = myVSN.getReservedByFileSystemName();
                    if (fsName != null && fsName.length() > 0) {
                        reservedBy.append(SamUtil.getResourceString(
                            "VSNDetails.reservebyfs")).append(" ");
                        reservedBy.append(fsName).append(";");
                    }

                    int type = myVSN.getReservationByType();
                    String name = myVSN.getReservationNameForType();
                    switch (type) {
                    case VSN.OWNER:
                        reservedBy.append(
                            SamUtil.getResourceString(
                                "VSNDetails.reservebyuser"));
                        reservedBy.append(" ").append(name);
                        break;
                    case VSN.GROUP:
                        reservedBy.append(
                            SamUtil.getResourceString(
                                "VSNDetails.reservebygroup"));
                        reservedBy.append(" ").append(name);
                        break;
                    case VSN.STARTING_DIR:
                        reservedBy.append(
                            SamUtil.getResourceString(
                                "VSNDetails.reservebydir"));
                        reservedBy.append(" ").append(name);
                        break;
                    }
                }

                propertySheetModel.setValue(
                    "reservedValue",
                    reservedBy.toString());

                String assocLibName = null;

                if (libraryName != null && libraryName.length() != 0) {
                    assocLibName = libraryName;
                } else {
                    assocLibName = "wizard.notapplicable";
                }
                propertySheetModel.setValue(
                    "assocLibValue",
                    assocLibName);

                propertySheetModel.setValue(
                    "damagedValue",
                    myVSN.isMediaDamaged() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "duplicateValue",
                    myVSN.isDuplicateVSN() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "readonlyValue",
                    myVSN.isReadOnly() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "writeprotectValue",
                    myVSN.isWriteProtected() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "foreignValue",
                    myVSN.isForeignMedia() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "recycleValue",
                    myVSN.isRecycled() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "fullValue",
                    myVSN.isVolumeFull() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "unavailableValue",
                    myVSN.isUnavailable() ? "VSNDetails.yes"
                        : "VSNDetails.no");
                propertySheetModel.setValue(
                    "needAuditValue",
                    myVSN.isNeedAudit() ? "VSNDetails.yes"
                        : "VSNDetails.no");

                // Disable either Reserve or Unreserve button
                if (myVSN.isReserved()) {
                    ((CCButton) getChild(
                        "SamQFSWizardReserveButton")).setDisabled(true);
                } else {
                    ((CCButton) getChild(
                        "UnreservePageActionButton")).setDisabled(true);
                }
            } else {
                // Cannot retrieve VSN
                throw new SamFSException(null, -2507);
            }
        } catch (SamFSException samEx) {
            setErrorAlertWithProcessException(
                samEx,
                "loadPropertySheet",
                "Failed to load VSN information",
                "VSNDetails.error.loadpsheet");
        }
        TraceUtil.trace3("Exiting");
    }

    public void handleLibrarySummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("LibrarySummaryHref");
        ViewBean targetView = getViewBean(LibrarySummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SLOT_NUMBER);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handleLibraryDriveSummaryHrefRequest(
        RequestInvocationEvent event) throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("LibraryDriveSummaryHref");
        ViewBean targetView = getViewBean(LibraryDriveSummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SLOT_NUMBER);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.EQ);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("VSNSummaryHref");
        ViewBean targetView = getViewBean(VSNSummaryViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SLOT_NUMBER);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNPoolDetailsHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("VSNPoolDetailsHref");
        ViewBean targetView = getViewBean(VSNPoolDetailsViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNPoolSummaryHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {

        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("VSNPoolSummaryHref");
        ViewBean targetView = getViewBean(VSNPoolSummaryViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handleHistorianHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("HistorianHref");
        ViewBean targetView = getViewBean(HistorianViewBean.class);
        removePageSessionAttribute(
            Constants.PageSessionAttributes.SLOT_NUMBER);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSearchHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("VSNSearchHref");
        ViewBean targetView = getViewBean(VSNSearchViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handleVSNSearchResultHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        String str = (String) getDisplayFieldValue("VSNSearchResultHref");
        ViewBean targetView = getViewBean(VSNSearchResultViewBean.class);
        BreadCrumbUtil.breadCrumbPathBackward(
            this,
            PageInfo.getPageInfo().getPageNumber(targetView.getName()),
            str);
        forwardTo(targetView);

        TraceUtil.trace3("Exiting");
    }

    public void handlePageActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        String value = (String) getDisplayFieldValue("PageActionMenu");
        try {
            execute(Integer.parseInt(value));
        } catch (NumberFormatException numEx) {
            // Should not happen at all
            TraceUtil.trace1("NumberFormatException caught in " +
                             "handlePageActionMenuHrefRequest!");
            forwardTo(getRequestContext());
        }
        TraceUtil.trace3("Exiting");
    }

    private void execute(int value) {
        TraceUtil.trace3("Entering");
        String slotValue = getSlotNumber();
        String op = null;
        boolean hasError = false;
        String errMsg = null, handlerName = "handlePageActionMenuHrefRequest";

        try {
            // Check Permission
            // Export needs MEDIA, otherwise CONFIG is required
            if (3 == value && !SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.MEDIA_OPERATOR)) {
                throw new SamFSException("common.nopermission");
            } else if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.CONFIG)) {
                throw new SamFSException("common.nopermission");
            }

            VSN myVSN = getSelectedVSN();

            if (myVSN != null) {
                switch (value) {
                    case OP_AUDIT:
                        op = "VSNDetails.action.audit";
                        errMsg = "VSNDetails.error.audit";
                        LogUtil.info(this.getClass(),
                            "handlePageActionMenuHrefRequest",
                            "Start auditing slot ".concat(slotValue));
                        myVSN.audit();
                        LogUtil.info(this.getClass(),
                            "handlePageActionMenuHrefRequest",
                            "Done auditing slot ".concat(slotValue));
                        break;

                    case OP_LOAD:
                        op = "VSNDetails.action.load";
                        errMsg = "VSNDetails.error.load";
                        LogUtil.info(this.getClass(),
                            "handlePageActionMenuHrefRequest",
                            "Start loading slot ".concat(slotValue));
                        myVSN.load();
                        LogUtil.info(this.getClass(),
                            "handlePageActionMenuHrefRequest",
                            "Done loading slot ".concat(slotValue));
                        break;

                    case OP_EXPORT:
                        op = "VSNDetails.action.export";
                        errMsg = "VSNDetails.error.export";
                        LogUtil.info(this.getClass(),
                            "handlePageActionMenuHrefRequest",
                            new NonSyncStringBuffer(
                                "Start exporting tape out from slot ").
                                append(slotValue).toString());
                        myVSN.export();
                        LogUtil.info(this.getClass(),
                            "handlePageActionMenuHrefRequest",
                            new NonSyncStringBuffer(
                                "Done exporting tape out from slot ").
                                append(slotValue).toString());

                        // After exporting the VSN, page should be redirected
                        // to VSN Summary page or Historian page
                        int originalLocation = getOriginalLocation();
                        switch (originalLocation) {
                            case 1:
                                setSuccessAlert(
                                    getViewBean(VSNSummaryViewBean.class),
                                    CHILD_COMMON_ALERT,
                                    op,
                                    slotValue);
                                break;

                            case 2:
                                setSuccessAlert(
                                    getViewBean(HistorianViewBean.class),
                                    CHILD_COMMON_ALERT,
                                    op,
                                    slotValue);
                                break;

                            default:
                                setSuccessAlert(
                                    getViewBean(LibrarySummaryViewBean.class),
                                    CHILD_COMMON_ALERT,
                                    op,
                                    slotValue);
                                break;

                        }
                        break;

                    case OP_UNRESERVED:
                        op = "VSNDetails.action.unreserve";
                        errMsg = "VSNDetails.error.unreserve";
                        handlerName = "handleUnreservePageActionButtonRequest";
                        LogUtil.info(this.getClass(),
                            "handlePageButton3Request",
                            new NonSyncStringBuffer(
                                "Start unreserving tape in slot ").
                                append(slotValue).toString());
                        myVSN.reserve(null, null, -1, null);
                        LogUtil.info(this.getClass(),
                            "handlePageButton3Request",
                            new NonSyncStringBuffer(
                                "Done unreserving tape in slot ").
                                append(slotValue).toString());
                        break;
                }

                if (value != 0) {
                    setSuccessAlert(
                       getParentViewBean(),
                       CommonViewBeanBase.CHILD_COMMON_ALERT,
                       op,
                       slotValue);
                }
            } else {
                throw new SamFSException(null, -2507);
            }
        } catch (SamFSException samEx) {
            hasError = true;
            setErrorAlertWithProcessException(
                samEx,
                handlerName,
                errMsg,
                errMsg);
        }

        if (hasError || value != 3) {
            resetDropDownMenu();
            this.forwardTo(getRequestContext());
        } else {
            ViewBean targetView = null;
            String str = null;
            int originalLocation = getOriginalLocation();

            switch (originalLocation) {
                case 1:
                    targetView = getViewBean(VSNSummaryViewBean.class);
                    if (comeFromVSNSearch) {
                        str = getResultBreadCrumbPath(2);
                    } else if (comeFromVSNSearchResult) {
                        str = getResultBreadCrumbPath(3);
                    } else {
                        str = getResultBreadCrumbPath(1);
                    }
                    removePageSessionAttribute(
                        Constants.PageSessionAttributes.VSN_NAME);
                    break;

                case 2:
                    targetView = getViewBean(HistorianViewBean.class);
                    if (comeFromVSNSearch) {
                        str = getResultBreadCrumbPath(2);
                    } else if (comeFromVSNSearchResult) {
                        str = getResultBreadCrumbPath(3);
                    } else {
                        str = getResultBreadCrumbPath(1);
                    }
                    removePageSessionAttribute(
                        Constants.PageSessionAttributes.VSN_NAME);
                    break;

                default:
                    targetView = getViewBean(LibrarySummaryViewBean.class);
                    removePageSessionAttribute(
                        Constants.SessionAttributes.PAGE_PATH);
                    break;
            }

            if (originalLocation == 1 || originalLocation == 2) {
                BreadCrumbUtil.breadCrumbPathBackward(
                    this,
                    PageInfo.getPageInfo().getPageNumber(targetView.getName()),
                    str);
            }

            forwardTo(targetView);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * handleSamQFSWizardReserveButtonRequest(event)
     * Handler of Reserve button
     */
    public void handleSamQFSWizardReserveButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        wizardLaunched = true;
        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler function for Unreserve VSN Button
     * @param RequestInvocationEvent event
     */
    public void handleUnreservePageActionButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        execute(OP_UNRESERVED);
        TraceUtil.trace3("Exiting");
    }

    private void checkRolePrivilege() {
        TraceUtil.trace3("Entering");

        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            ((CCHiddenField) getChild(ROLE)).setValue("CONFIG");
            ((CCWizardWindow) getChild(
                "SamQFSWizardReserveButton")).setDisabled(wizardLaunched);
            ((CCButton)
                getChild("LabelPageActionButton")).setDisabled(false);
            ((CCButton)
                getChild("UnreservePageActionButton")).setDisabled(false);
            ((CCButton)
                getChild("EditVSNPageActionButton")).setDisabled(false);
        } else if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {
            ((CCHiddenField) getChild(ROLE)).setValue("MEDIA");
            ((CCWizardWindow) getChild(
                "SamQFSWizardReserveButton")).setDisabled(true);
        } else {
            ((CCWizardWindow) getChild(
                "SamQFSWizardReserveButton")).setDisabled(true);
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Set up the data model and create the model for wizard button
     */
    private void initializeWizard() {
        TraceUtil.trace3("Entering");

        NonSyncStringBuffer cmdChild =
            new NonSyncStringBuffer().
                append(this.getQualifiedName()).
                append(".").
                append(CHILD_FRWD_TO_CMDCHILD);
        wizWinModel = ReservationImpl.createModel(cmdChild.toString());
        pageTitleModel.setModel(
            "SamQFSWizardReserveButton",
            wizWinModel);
        wizWinModel.setValue(
            "SamQFSWizardReserveButton",
            "VSNDetails.PageActionButton2");
        TraceUtil.trace3("Exiting");
    }

    public void handleForwardToVbRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        wizardLaunched = false;
        forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    private VSN getSelectedVSN() throws SamFSException {
        TraceUtil.trace3("Entering");

        // reset flag
        isVSNInDrive = false;

        // retrieve libraryName, eq, soltNumber
        libraryName = getLibraryName();
        slotNumber = getSlotNumber();

        if (libraryName == null || libraryName.length() == 0) {
            throw new SamFSException(null, -2502);
        }
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        VSN myVSN = null;

        if (slotNumber != null) {
            SamUtil.doPrint(new NonSyncStringBuffer(
                "VSNDetail: getSelectedVSN slot num: ").
                append(slotNumber).toString());
        } else {
            SamUtil.doPrint("VSNDetail: getSelectedVSN slot num: null");
            SamUtil.doPrint("VSNDetail: slotKey is null");
            throw new SamFSException(null, -2507);
        }

        Library myLibrary = sysModel.getSamQFSSystemMediaManager().
            getLibraryByName(libraryName);

        if (myLibrary == null) {
            throw new SamFSException(null, -2501);
        }

        try {
            myVSN = myLibrary.getVSN(Integer.parseInt(slotNumber));

            // Now check if the VSN is in drive
            Drive myDrive = myVSN.getDrive();
            if (myDrive != null) {
                isVSNInDrive = true;
            }
        } catch (NumberFormatException numEx) {
            // Could not retrieve VSN
            SamUtil.doPrint(
                "NumberFormatException caught while parsing slotKey.");
            throw new SamFSException(null, -2507);
        }

        TraceUtil.trace3("Exiting");
        return myVSN;
    }

    private void setResWizardState() {
        TraceUtil.trace3("Entering");

        // if modelName and implName existing, retrieve it
        // else create them then store in page session

        ViewBean view = getParentViewBean();
        String temp = (String) getParentViewBean().getPageSessionAttribute
                        (ReservationImpl.WIZARDPAGEMODELNAME);
        String modelName = (String) view.getPageSessionAttribute(
                ReservationImpl.WIZARDPAGEMODELNAME);
        String implName =  (String) view.getPageSessionAttribute(
                ReservationImpl.WIZARDIMPLNAME);
        if (modelName == null) {
            modelName = new NonSyncStringBuffer().append(
                ReservationImpl.WIZARDPAGEMODELNAME_PREFIX).append("_").append(
                HtmlUtil.getUniqueValue()).toString();
            view.setPageSessionAttribute(ReservationImpl.WIZARDPAGEMODELNAME,
                modelName);
        }
        wizWinModel.setValue(ReservationImpl.WIZARDPAGEMODELNAME, modelName);

        if (implName == null) {
            implName = new NonSyncStringBuffer().append(
                ReservationImpl.WIZARDIMPLNAME_PREFIX).append("_").append(
                HtmlUtil.getUniqueValue()).toString();
            view.setPageSessionAttribute(ReservationImpl.WIZARDIMPLNAME,
                implName);
        }
        wizWinModel.setValue(
            CCWizardWindowModelInterface.WIZARD_NAME, implName);
        TraceUtil.trace3("Exiting");
    }

    private void setSuccessMsg(String slotValue, long jobID, String operation) {
        MediaUtil.setLabelTapeInfoAlert(getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(
                "VSNSummary.labeltapejob",
                new String[] {operation, slotValue, Long.toString(jobID)}));
    }

    private String getLibraryName() {
        libraryName = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
        return libraryName;
    }

    private String getSlotNumber() {
        slotNumber = (String) getPageSessionAttribute(
            Constants.PageSessionAttributes.SLOT_NUMBER);
        return slotNumber;
    }

    private void setSuccessAlert(
        ViewBean vb,
        String alertName,
        String operation,
        String slotValue) {
        SamUtil.setInfoAlert(
            vb,
            alertName,
            "success.summary",
            SamUtil.getResourceString(operation, slotValue),
            getServerName());
    }

    private void setErrorAlertWithProcessException(
        Exception ex,
        String handlerName,
        String backupMessage,
        String alertKey) {

        SamUtil.processException(
            ex, this.getClass(), handlerName,
            backupMessage, getServerName());

        if (ex instanceof SamFSException) {
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                alertKey,
                ((SamFSException) ex).getSAMerrno(),
                ((SamFSException) ex).getMessage(),
                getServerName());
        } else {
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                alertKey,
                -99001,
                "ModelControlException caught in the framework",
                getServerName());
        }
    }

    private String getPriorTargetName(int numberOfLevelsUp) {
        Integer[] temp = (Integer []) getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        Integer[] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);
        int index = path[path.length - numberOfLevelsUp].intValue();
        PageInfo pageInfo = PageInfo.getPageInfo();
        return pageInfo.getPagePath(index).getCommandField();
    }

    /**
     * Return where the control should return to after export operation
     * through VSN Search
     * 0 => Library Summary
     * 1 => VSN Summary
     * 2 => Historian
     */
    private int getOriginalLocation() {
        // Check one level up
        int originalLocation = getOriginalLocationHelper(getPriorTargetName(1));
        switch (originalLocation) {
            case 0:
            case 1:
            case 2:
                return originalLocation;
            case 3:
                // It's coming from VSN Search Page, check one level deeper
                // to determine where the page should return to
                comeFromVSNSearch = true;
                return getOriginalLocationHelper(getPriorTargetName(2));
            case 4:
                // It's coming from VSN Search Result Page, check two level
                // deeper to determine where the page should return to
                comeFromVSNSearchResult = true;
                return getOriginalLocationHelper(getPriorTargetName(3));
            default:
                return 0;
        }
    }

    /**
     * Helper function of getOriginalLocation
     */
    private int getOriginalLocationHelper(String hrefName) {
        if (hrefName.equals("VSNSummaryHref")) {
	    return 1;
	} else if (hrefName.equals("HistorianHref")) {
            return 2;
        } else if (hrefName.equals("VSNSearchHref")) {
            return 3;
        } else if (hrefName.equals("VSNSearchResultHref")) {
            return 4;
        } else {
            return 0;
        }
    }

    /**
     * Helper method to figure out the correct content of the breadcrumb links
     */
    private String getResultBreadCrumbPath(int numberOfLevelsUp) {
        Integer[] temp = (Integer []) getPageSessionAttribute(
            Constants.SessionAttributes.PAGE_PATH);
        Integer[] path = BreadCrumbUtil.getBreadCrumbDisplay(temp);
        int index = path[path.length - numberOfLevelsUp].intValue();
        return Integer.toString(BreadCrumbUtil.inPagePath(
            path, index, path.length - numberOfLevelsUp));
    }
}
