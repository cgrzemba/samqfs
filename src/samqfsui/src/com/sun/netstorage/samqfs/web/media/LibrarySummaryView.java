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

// ident	$Id: LibrarySummaryView.java,v 1.53 2008/03/17 14:43:40 am143972 Exp $

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

import com.sun.netstorage.samqfs.web.media.wizards.AddLibraryImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.wizard.CCWizardWindow;

import java.io.IOException;
import javax.servlet.ServletException;


public class LibrarySummaryView extends CommonTableContainerView {

    public static final String CHILD_ACTIONMENU_HREF    = "ActionMenuHref";
    public static final String CHILD_TILED_VIEW = "LibrarySummaryTiledView";

    public static final String ROLE = "Role";

    // Use by javascript: onClick() of Import button
    // Cannot hard code the string SAMST due to i18n issue
    public static final String CHILD_SAMST_HIDDEN_FIELD = "SAMSTdriverString";
    public static final String CHILD_ACSLS_HIDDEN_FIELD = "ACSLSdriverString";

    // for wizard
    public static final String CHILD_FRWD_TO_CMDCHILD = "forwardToVb";
    private boolean wizardLaunched = false;
    private CCWizardWindowModel wizWinModel;
    private LibrarySummaryModel model = null;


    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public LibrarySummaryView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "LibrarySummaryTable";
        // Pass the xmlFile that is to be loaded

        model = new LibrarySummaryModel("/jsp/media/LibrarySummaryTable.xml");
        // initialize Add wizard
        initializeAddWizard();

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_ACTIONMENU_HREF, CCHref.class);
        registerChild(ROLE, CCHiddenField.class);
        registerChild(CHILD_TILED_VIEW, LibrarySummaryTiledView.class);
        registerChild(CHILD_SAMST_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_ACSLS_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(CHILD_FRWD_TO_CMDCHILD, BasicCommandField.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer().append(
            "Entering: child name is ").append(name).toString());

        View child = null;
        if (name.equals(CHILD_ACTIONMENU_HREF)) {
            child = new CCHref(this, name, null);
        } else if (name.equals(CHILD_TILED_VIEW)) {
            // The ContainerView used to register and create children
            // for "column" elements defined in the model's XML
            // document. Note that we're creating a seperate
            // ContainerView only to evoke JATO's TiledView behavior
            // for these elements. If TiledView behavior is not
            // required, creating a ContainerView object is not
            // necessary. By default, CCActionTable will attempt to
            // retrieve all children from it's parent (i.e., this view).
            child = new LibrarySummaryTiledView(this, model, name);
        } else if (name.equals(ROLE)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_SAMST_HIDDEN_FIELD) ||
            name.equals(CHILD_ACSLS_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_FRWD_TO_CMDCHILD)) {
            child = new BasicCommandField(this, name);
        } else {
            return super.createChild(model, name,
                LibrarySummaryView.CHILD_TILED_VIEW);
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     *
     * ADD Library Wizard
     *
     */
    public void handleSamQFSWizardAddButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");
        wizardLaunched = true;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Existing");
    }

    /**
     * View VSN
     * Forward to VSN Summary Page or Historian Page
     */
    public void handleViewVSNButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        ViewBean targetView = null;
        String rowKey = getSelectedRowKey(0);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            if (!rowKey.equals(Constants.MediaAttributes.HISTORIAN)) {
                Library myLibrary = MediaUtil.getLibraryObject(
                    getServerName(), getSelectedRowKey(0));
            }
        } catch (SamFSException samEx) {
            targetView = getParentViewBean();
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleViewVSNButtonRequest",
                "Failed to retrieve library license information",
                getServerName());
            SamUtil.setErrorAlert(
                targetView,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "LibrarySummary.error.viewvsn",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        if (rowKey.equals(Constants.MediaAttributes.HISTORIAN)) {
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.LIBRARY_NAME,
                Constants.MediaAttributes.HISTORIAN_NAME);
            targetView = getViewBean(HistorianViewBean.class);
        } else {
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.LIBRARY_NAME,
                rowKey);
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.FROM_TREE,
                new Boolean(false));
            targetView = getViewBean(VSNSummaryViewBean.class);
        }

        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));

        ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetView);
        TraceUtil.trace3("Existing");
    }

    /**
     * Handler when View Drive button is clicked
     * Forward to Drive Summary Page
     */

    public void handleViewDriveButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        ViewBean targetView = null;
        String rowKey = getSelectedRowKey(0);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            Library myLibrary = MediaUtil.getLibraryObject(
                getServerName(), getSelectedRowKey(0));

            // assign true/false to determine if shared information should be
            // shown
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.SHARE_CAPABLE,
                Boolean.toString(
                    MediaUtil.isDriveSharedCapable(getServerName(), rowKey)));
        } catch (SamFSException samEx) {
            targetView = getParentViewBean();
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleViewDriveButtonRequest",
                "Failed to retrieve library license information",
                getServerName());
            SamUtil.setErrorAlert(
                targetView,
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "LibrarySummary.error.viewdrive",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        getParentViewBean().setPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME,
            rowKey);
        targetView = getViewBean(LibraryDriveSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathForward(
            getParentViewBean(),
            PageInfo.getPageInfo().getPageNumber(
                getParentViewBean().getName()));


        ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetView);
        TraceUtil.trace3("Existing");
    }

    /**
     * Handler for Import Button
     */
    public void handleImportButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");

        String rowKey = getSelectedRowKey(0);
        String driver = getSelectedRowKey(1);

        if (driver.equals(
            SamUtil.getResourceString("library.driver.samst"))) {
            // Driver is SAMST
            try {
                Library myLibrary = MediaUtil.getLibraryObject(
                    getServerName(), getSelectedRowKey(0));

                LogUtil.info(this.getClass(), "handleImportButtonRequest",
                    new NonSyncStringBuffer().append("Start importing in ").
                    append(rowKey).toString());
                myLibrary.importVSN();
                LogUtil.info(this.getClass(), "handleImportButtonRequest",
                    new NonSyncStringBuffer().append("Done importing ").
                    append(rowKey).toString());
                setSuccessAlert("LibrarySummary.action.import", rowKey);

                getParentViewBean().forwardTo(getRequestContext());

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
                getParentViewBean().forwardTo(getRequestContext());
            }
        } else {
            // 4.5 ACSLS
            getParentViewBean().setPageSessionAttribute(
                Constants.PageSessionAttributes.LIBRARY_NAME,
                rowKey);
            ViewBean targetView = getViewBean(ImportVSNViewBean.class);

            BreadCrumbUtil.breadCrumbPathForward(
                getParentViewBean(),
                PageInfo.getPageInfo().getPageNumber(
                    getParentViewBean().getName()));

            ((CommonViewBeanBase) getParentViewBean()).forwardTo(targetView);
        }

        TraceUtil.trace3("Existing");
    }

    // Action Menu of Library Summary Page
    // 0 -> -- Operation --
    // 1 -> Unload
    // 2 -> Remove
    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");
        String rowKey = getSelectedRowKey(0);
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
                getServerName(), getSelectedRowKey(0));
            switch (value) {
                // Unload
                case 2:
                    op = "LibrarySummary.action.unload";
                    LogUtil.info(this.getClass(),
                        "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Start unloading tape in ").
                            append(rowKey).toString());
                    myLibrary.unload();
                    LogUtil.info(this.getClass(),
                        "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Done unloading tape in ").
                            append(rowKey).toString());
                    setSuccessAlert(op, rowKey);
                    break;

                // Delete
                case 3:
                    op = "LibrarySummary.action.remove";
                    LogUtil.info(this.getClass(),
                        "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Start removing library ").
                            append(rowKey).toString());
                    SamQFSSystemModel sysModel =
                        SamUtil.getModel(getServerName());
                    sysModel.getSamQFSSystemMediaManager().
                        removeLibrary(myLibrary);
                    LogUtil.info(this.getClass(),
                        "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer().append(
                            "Done removing library ").
                            append(rowKey).toString());
                    setSuccessAlert(op, rowKey);
                    getViewBean(LibrarySummaryViewBean.class).
                        forwardTo(getRequestContext());
                    return;
            }

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleActionMenuHrefRequest",
                new NonSyncStringBuffer("Failed to ").
                    append(op).append(" ").append(rowKey).toString(),
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

    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        checkRolePrivilege();
        model.setRowSelected(false);
        setAddWizardState();

        // assign the SAMST driver string to SAMSTdriverString
        ((CCHiddenField) getChild("SAMSTdriverString")).setValue(
            SamUtil.getResourceString("library.driver.samst"));
        ((CCHiddenField) getChild("ACSLSdriverString")).setValue(
            SamUtil.getResourceString("library.driver.acsls"));

        // Disable Tooltip
        CCActionTable myTable = (CCActionTable) getChild(CHILD_ACTION_TABLE);
        CCRadioButton myRadio = (CCRadioButton) myTable.getChild(
            CCActionTable.CHILD_SELECTION_RADIOBUTTON);
        myRadio.setTitle("");
        myRadio.setTitleDisabled("");

        TraceUtil.trace3("Exiting");
    }

    /**
     * Set up the data model and create the model for wizard button
     */
    private void initializeAddWizard() {
        TraceUtil.trace3("Entering");

        ViewBean view = getParentViewBean();
        NonSyncStringBuffer cmdChild =
            new NonSyncStringBuffer().
                append(view.getQualifiedName()).
                append(".").
                append("LibrarySummaryView.").
                append(CHILD_FRWD_TO_CMDCHILD);
        wizWinModel = AddLibraryImpl.createModel(cmdChild.toString());
        model.setModel("SamQFSWizardAddButton", wizWinModel);
        wizWinModel.setValue(
            "SamQFSWizardAddButton", "LibrarySummary.button1");

        TraceUtil.trace3("Exiting");
    }

    public void handleForwardToVbRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        wizardLaunched = false;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void populateData(String serverName) throws SamFSException {
        TraceUtil.trace3("Entering");
        model.initModelRows(serverName);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Choice:
     * 0  == return selected library name
     * 1  == return selected driver name
     * 2  == return selected library state
     */

    private String getSelectedRowKey(int choice)
        throws ModelControlException {
        TraceUtil.trace3("Entering");

        String returnValue = null;

        // restore the selected rows
        CCActionTable actionTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);

        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex, this.getClass(),
                "getSelectedRowKey",
                "Exception occurred within framework",
                getServerName());
            // just throw it, let it go to onUncaughtException()
            throw mcex;
        }

        LibrarySummaryModel libModel = (LibrarySummaryModel)
            actionTable.getModel();

        // single select Action Table, so just take the first row in the
        // array
        Integer [] selectedRows = libModel.getSelectedRows();
        int row = selectedRows[0].intValue();
        libModel.setRowIndex(row);

        // only use the hidden field for actions
        switch (choice) {
            case 0:
                // return library name
                returnValue = (String) libModel.getValue("NameHidden");
                TraceUtil.trace3(new NonSyncStringBuffer().append(
                "Selected Library Name is ").append(returnValue).toString());
                break;

            case 1:
                // return library driver
                returnValue = (String) libModel.getValue("DriverHidden");
                TraceUtil.trace3(new NonSyncStringBuffer().append(
                "Selected Library Driver is ").append(returnValue).toString());
                break;

            case 2:
                // return library device state
                returnValue = (String) libModel.getValue("StateHidden");
                TraceUtil.trace3(new NonSyncStringBuffer().append(
                "Selected Library State is ").append(returnValue).toString());
                break;
        }

        TraceUtil.trace3("Exiting");
        return returnValue;
    }

    private void setAddWizardState() {
        TraceUtil.trace3("Entering");

        // if modelName and implName existing, retrieve it
        // else create them then store in page session

        ViewBean view = getParentViewBean();
        String temp = (String) getParentViewBean().getPageSessionAttribute
            (AddLibraryImpl.WIZARDPAGEMODELNAME);
        String modelName = (String) view.getPageSessionAttribute(
            AddLibraryImpl.WIZARDPAGEMODELNAME);
        String implName =  (String) view.getPageSessionAttribute(
            AddLibraryImpl.WIZARDIMPLNAME);
        if (modelName == null) {
            modelName = new StringBuffer().append(
                AddLibraryImpl.WIZARDPAGEMODELNAME_PREFIX).append("_").append(
                HtmlUtil.getUniqueValue()).toString();
            view.setPageSessionAttribute(AddLibraryImpl.WIZARDPAGEMODELNAME,
                 modelName);
        }
        wizWinModel.setValue(AddLibraryImpl.WIZARDPAGEMODELNAME, modelName);

        if (implName == null) {
            implName = new StringBuffer().append(
                AddLibraryImpl.WIZARDIMPLNAME_PREFIX).append("_").append(
                HtmlUtil.getUniqueValue()).toString();
            view.setPageSessionAttribute(AddLibraryImpl.WIZARDIMPLNAME,
                implName);
        }
        wizWinModel.setValue
            (CCWizardWindowModelInterface.WIZARD_NAME, implName);
        TraceUtil.trace3("Exiting");
    }

    private void setSuccessAlert(String msg, String item) {
        TraceUtil.trace3("Entering");
        SamUtil.setInfoAlert(
            getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(msg, item),
            getServerName());
        TraceUtil.trace3("Exiting");
    }


    /**
     * checkRolePriviledge() checks if the user is a valid Admin
     * disable all action buttons / dropdown if not
     */
    private void checkRolePrivilege() {
        TraceUtil.trace3("Entering");

        if (SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.CONFIG)) {
            ((CCHiddenField) getChild(ROLE)).setValue("CONFIG");
            ((CCWizardWindow) getChild(
                "SamQFSWizardAddButton")).setDisabled(wizardLaunched);
        } else if (SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.MEDIA_OPERATOR)) {
            ((CCHiddenField) getChild(ROLE)).setValue("MEDIA");
        }

        TraceUtil.trace3("Exiting");
    }

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

} // end of LibrarySummaryView class
