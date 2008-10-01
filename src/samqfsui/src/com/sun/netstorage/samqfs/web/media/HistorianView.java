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

// ident	$Id: HistorianView.java,v 1.40 2008/10/01 22:43:33 ronaldso Exp $

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

import com.sun.netstorage.samqfs.web.media.wizards.ReservationImpl;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.model.media.VSN;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LargeDataSet;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModelInterface;

import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.wizard.CCWizardWindow;

import java.io.IOException;

import javax.servlet.ServletException;

public class HistorianView extends CommonTableContainerView {

    // child name for tiled view class
    public static final String CHILD_TILED_VIEW = "HistorianTiledView";

    private HistorianModel model = null;

    public static final String ROLE = "Role";

    // for wizard
    public static final String CHILD_FRWD_TO_CMDCHILD = "forwardToVb";
    private boolean wizardLaunched = false;
    private CCWizardWindowModel wizWinModel;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public HistorianView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        CHILD_ACTION_TABLE = "HistorianTable";
        LargeDataSet dataSet = new HistorianData(getServerName());
        model = new HistorianModel(dataSet, hasConfigPermission());

        // initialize reservation wizard
        initializeWizard();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(ROLE, CCHiddenField.class);
        registerChild(CHILD_TILED_VIEW, HistorianTiledView.class);
        registerChild(CHILD_FRWD_TO_CMDCHILD, BasicCommandField.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_TILED_VIEW)) {
            // The ContainerView used to register and create children
            // for "column" elements defined in the model's XML
            // document. Note that we're creating a seperate
            // ContainerView only to evoke JATO's TiledView behavior
            // for these elements. If TiledView behavior is not
            // required, creating a ContainerView object is not
            // necessary. By default, CCActionTable will attempt to
            // retrieve all children from it's parent (i.e., this view).
            child = new HistorianTiledView(this, model, name);
        } else if (name.equals(ROLE)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_FRWD_TO_CMDCHILD)) {
            child = new BasicCommandField(this, name);
        } else {
            return super.createChild(model, name,
                HistorianView.CHILD_TILED_VIEW);
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    public void handleExportButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        execute("ExportButton");
        TraceUtil.trace3("Exiting");
    }

    private void execute(String buttonName) throws ModelControlException {
        TraceUtil.trace3("Entering");
        String slotNum = getSelectedRowKey();

        try {
            Library myLibrary = getHistorian();
            VSN myVSN = null;

            if (myLibrary == null) {
                // Could not retrieve library
                throw new SamFSException(null, -2502);
            }

            try {
                myVSN = myLibrary.getVSN(Integer.parseInt(slotNum));
            } catch (NumberFormatException numEx) {
                // Could not retrieve VSN
                SamUtil.doPrint(
                    "NumberFormatException caught while " +
                    "parsing slotKey to integer.");
                throw new SamFSException(null, -2507);
            }

            if (myVSN == null) {
                // Could not retrieve VSN
                throw new SamFSException(null, -2507);
            }

            if (buttonName.equals("ExportButton")) {
                // Check Permission
                if (!(hasMediaOpPermission() || hasConfigPermission())) {
                    throw new SamFSException("common.nopermission");
                }

                LogUtil.info(
                    this.getClass(),
                    "handleExportButtonRequest",
                    new NonSyncStringBuffer(
                        "Start exporting tape with VSN in slot ").
                        append(slotNum).toString());
                myVSN.export();
                LogUtil.info(
                    this.getClass(),
                    "handleExportButtonRequest",
                    new NonSyncStringBuffer(
                        "Done exporting tape with VSN in slot ").
                        append(slotNum).toString());
                setSuccessAlert("Historian.action.export", slotNum);
            } else if (buttonName.equals("UnreserveButton")) {
                // Check Permission
                if (!hasConfigPermission()) {
                    throw new SamFSException("common.nopermission");
                }
                LogUtil.info(
                    this.getClass(),
                    "handleUnreserveButtonRequest",
                    new NonSyncStringBuffer(
                        "Start unreserving tape with VSN in slot ").
                        append(slotNum).toString());
                myVSN.reserve(null, null, -1, null);
                LogUtil.info(
                    this.getClass(),
                    "handleUnreserveButtonRequest",
                    new NonSyncStringBuffer(
                        "Done unreserving tape with VSN in slot ").
                        append(slotNum).toString());
                setSuccessAlert("Historian.action.unreserve", slotNum);
            }
        } catch (SamFSException samEx) {
            if (buttonName.equals("ExportButton")) {
                setErrorAlertWithProcessException(
                    samEx,
                    "handleExportButtonRequest",
                    "Failed to export tape",
                    "Historian.error.export");
            } else if (buttonName.equals("UnreserveButton")) {
                setErrorAlertWithProcessException(
                    samEx,
                    "handleUnreserveButtonRequest",
                    "Failed to unreserve tape",
                    "Historian.error.unreserve");
            }
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void handleSamQFSWizardReserveButtonRequest(
        RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        wizardLaunched = true;
        model.setRowSelected(false);
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    // Unreserve VSN
    public void handleUnreserveButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        execute("UnreserveButton");
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        checkRolePrivilege();
        setResWizardState();

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
    private void initializeWizard() {
        TraceUtil.trace3("Entering");
        ViewBean view = getParentViewBean();
        NonSyncStringBuffer cmdChild =
            new NonSyncStringBuffer().
                append(view.getQualifiedName()).
                append(".").
                append("HistorianView.").
                append(CHILD_FRWD_TO_CMDCHILD);

        wizWinModel = ReservationImpl.createModel(cmdChild.toString());
        model.setModel("SamQFSWizardReserveButton", wizWinModel);
        wizWinModel.setValue("SamQFSWizardReserveButton", "vsn.button.reserve");
        TraceUtil.trace3("Exiting");
    }

    public void handleForwardToVbRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");
        wizardLaunched = false;
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void populateData() throws SamFSException {
        TraceUtil.trace3("Entering");

        CCActionTable actionTable = (CCActionTable)
            getChild(CHILD_ACTION_TABLE);

        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            setErrorAlertWithProcessException(
                mcex,
                "populateData",
                "ModelControlException occurred within framework",
                "Historian.error.populateData");
        }

        model.initModelRows();
        model.setRowSelected(false);

        TraceUtil.trace3("Exiting");
    }

    /**
     * To return the slot number of the selected row
     * @return Slot Number of the selected row
     */

    private String getSelectedRowKey() throws ModelControlException {
        TraceUtil.trace3("Entering");
        // restore the selected rows
        CCActionTable actionTable = (CCActionTable)
            getChild(CHILD_ACTION_TABLE);
        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(mcex, this.getClass(),
                "getSelectedRowKey",
                "ModelControlException occurred within framework",
                getServerName());
            // just throw it, let it go to onUncaughtException()
            throw mcex;
        }

        HistorianModel hisModel = (HistorianModel) actionTable.getModel();

        // single select Action Table, so just take the first row in the
        // array
        Integer [] selectedRows = hisModel.getSelectedRows();
        int row = selectedRows[0].intValue();
        hisModel.setRowIndex(row);

        String concatString =
            (String) hisModel.getValue("InformationHidden");
        String[] returnValue = concatString.split("###");

        if (returnValue.length > 0) {
            TraceUtil.trace3("Exiting");
            return returnValue[0];
        } else {
            TraceUtil.trace3("Exiting - Invalid information key found!");
            return "";
        }
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

    private Library getHistorian() throws SamFSException {
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
        Library myLibrary = sysModel.getSamQFSSystemMediaManager().
            getLibraryByName(Constants.MediaAttributes.HISTORIAN_NAME);
        if (myLibrary == null) {
            throw new SamFSException(null, -2506);
        } else {
            TraceUtil.trace3("Exiting");
            return myLibrary;
        }
    }

    private void checkRolePrivilege() {
        if (hasConfigPermission()) {
            ((CCHiddenField) getChild(ROLE)).setValue("CONFIG");
            ((CCWizardWindow) getChild(
                "SamQFSWizardReserveButton")).setDisabled(wizardLaunched);
        } else if (hasMediaOpPermission()) {
            ((CCHiddenField) getChild(ROLE)).setValue("MEDIA");
        } else {
            // disable the radio button row selection column
            model.setSelectionType(CCActionTableModelInterface.NONE);
        }
    }

    private boolean hasConfigPermission() {
        return SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG);
    }

    private boolean hasMediaOpPermission() {
        return SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG);
    }

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }
} // end of HistorianView class
