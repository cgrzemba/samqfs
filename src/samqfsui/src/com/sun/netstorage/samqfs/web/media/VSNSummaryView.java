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

// ident	$Id: VSNSummaryView.java,v 1.52 2008/11/06 00:47:08 ronaldso Exp $

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
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.wizard.CCWizardWindow;

import java.io.IOException;
import java.util.StringTokenizer;
import javax.servlet.ServletException;


public class VSNSummaryView extends CommonTableContainerView {

    // child name for tiled view class
    public static final String CHILD_TILED_VIEW = "VSNSummaryTiledView";
    public static final String CHILD_ACTIONMENU_HREF = "ActionMenuHref";

    public static final String ROLE = "Role";

    // for wizard
    public static final String CHILD_FRWD_TO_CMDCHILD = "forwardToVb";
    private VSNSummaryModel model = null;
    private boolean wizardLaunched = false;
    private boolean writeRole = false;
    private CCWizardWindowModel wizWinModel;

    private String serverName = null, libraryName = null;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public VSNSummaryView(
        View parent, String name, String serverName, String libraryName) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "VSNSummaryTable";
        this.serverName = serverName;
        this.libraryName = libraryName;
        LargeDataSet dataSet = new VSNSummaryData(serverName, libraryName);
        model =
            new VSNSummaryModel(
                dataSet,
                SecurityManagerFactory.getSecurityManager().
                    hasAuthorization(Authorization.CONFIG));

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
        registerChild(CHILD_TILED_VIEW, VSNSummaryTiledView.class);
        registerChild(CHILD_ACTIONMENU_HREF, CCHref.class);
        registerChild(CHILD_FRWD_TO_CMDCHILD, BasicCommandField.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        if (name.equals(CHILD_ACTIONMENU_HREF)) {
            TraceUtil.trace3("Exiting");
            return new CCHref(this, name, null);
        } else if (name.equals(ROLE)) {
           return new CCHiddenField(this, name, null);
        } else  if (name.equals(CHILD_TILED_VIEW)) {
            // The ContainerView used to register and create children
            // for "column" elements defined in the model's XML
            // document. Note that we're creating a seperate
            // ContainerView only to evoke JATO's TiledView behavior
            // for these elements. If TiledView behavior is not
            // required, creating a ContainerView object is not
            // necessary. By default, CCActionTable will attempt to
            // retrieve all children from it's parent (i.e., this view).
            TraceUtil.trace3("Exiting");
            return new VSNSummaryTiledView(this, model, name);
        } else if (name.equals(CHILD_FRWD_TO_CMDCHILD)) {
            TraceUtil.trace3("Exiting");
            return new BasicCommandField(this, name);
        } else {
            TraceUtil.trace3("Exiting");
            return super.createChild(model, name,
                VSNSummaryView.CHILD_TILED_VIEW);
        }
    }

    /**
     * Handler for action menu
     */
    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException,
        NumberFormatException {

        TraceUtil.trace3("Entering");
        String slotKey = getSelectedRowKey(0);
        int value = Integer.parseInt(
            (String) getDisplayFieldValue("ActionMenu"));
        String errMsg = null;

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

            VSN myVSN = getSelectedVSN(slotKey);
            String op = null;

            switch (value) {
                case 0:
                    // -- Operation -- selected, do nothing
                    break;

                case 1:
                    // Audit
                    op = "VSNSummary.msg.audit";
                    errMsg = "VSNSummary.error.audit";
                    LogUtil.info(this.getClass(), "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer(
                        "Start auditing slot ").append(slotKey).toString());
                    myVSN.audit();
                    LogUtil.info(this.getClass(), "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer("Done auditing slot ").
                        append(slotKey).toString());
                    break;

                case 2:
                    // Load
                    op = "VSNSummary.msg.load";
                    errMsg = "VSNSummary.error.load";
                    LogUtil.info(this.getClass(), "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer(
                        "Start loading tape to slot ").append(slotKey).
                        toString());
                    myVSN.load();
                    LogUtil.info(this.getClass(), "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer(
                        "Done loading tape to slot ").append(slotKey).
                        toString());

                    // Set Page Session Attribute to take care of Bug 5013874
                    // Need to disable LOAD drop down menu item when
                    // SAM is loading the tape into the drive.
                    SamUtil.doPrint(new NonSyncStringBuffer(
                        "Setting VSN_BEING_LOADED to ").append(myVSN.getVSN()).
                        toString());
                    // First check if there are already a list of VSN
                    // that's being loaded.  Append the VSN name to the list
                    String existingList = (String) getParentViewBean().
                        getPageSessionAttribute(
                            Constants.PageSessionAttributes.VSN_BEING_LOADED);

                    if (existingList != null && existingList.length() != 0) {
                        existingList = new NonSyncStringBuffer(
                            existingList).append("###").append(myVSN.getVSN()).
                            toString();
                        getParentViewBean().setPageSessionAttribute(
                            Constants.PageSessionAttributes.VSN_BEING_LOADED,
                            existingList);
                    } else {
                        getParentViewBean().setPageSessionAttribute(
                            Constants.PageSessionAttributes.VSN_BEING_LOADED,
                            myVSN.getVSN());
                    }
                    break;

                case 3:
                    // Export
                    op = "VSNSummary.msg.export";
                    errMsg = "VSNSummary.error.export";
                    LogUtil.info(this.getClass(), "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer(
                        "Start exporting tape out from slot ").append(slotKey).
                        toString());
                    myVSN.export();
                    LogUtil.info(this.getClass(), "handleActionMenuHrefRequest",
                        new NonSyncStringBuffer(
                        "Done exporting tape out from slot ").append(slotKey).
                        toString());
                    break;
            }
            setSuccessAlert(op, slotKey);

        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleActionMenuHrefRequest",
                errMsg,
                serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                errMsg,
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        // set the drop-down menu to default value
        ((CCDropDownMenu) getChild("ActionMenu")).setValue("0");
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler function for Reserve VSN Button
     * @param RequestInvocationEvent event
     */
    public void handleSamQFSWizardReserveButtonRequest
        (RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {

        TraceUtil.trace3("Entering");
        wizardLaunched = true;
        model.setRowSelected(false);
        getParentViewBean().forwardTo(getRequestContext());

        TraceUtil.trace3("Exiting");
    }

    /**
     * Handler function for Unreserve VSN Button
     * @param RequestInvocationEvent event
     */
    public void handleUnreserveButtonRequest(RequestInvocationEvent event)
                throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String slotKey = getSelectedRowKey(0);
        try {
            VSN myVSN = getSelectedVSN(slotKey);
            LogUtil.info(this.getClass(), "handleUnreserveButtonRequest",
                "Start unreserving VSN");
            myVSN.reserve(null, null, -1, null);
            LogUtil.info(this.getClass(), "handleUnreserveButtonRequest",
                "Done unreserving VSN");
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleUnreserveButtonRequest",
                "VSNSummary.error.unreserve",
                serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "VSNSummary.error.unreserve",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                serverName);
        }

        SamUtil.setInfoAlert(
            getParentViewBean(),
            CommonViewBeanBase.CHILD_COMMON_ALERT,
            "success.summary",
            SamUtil.getResourceString(
                "VSNSummary.msg.unreserve", slotKey),
            serverName);
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Update the hidden field that keeps track which VSN has been loading.
     */
    private void updateLoadVSNHiddenField() throws ModelControlException {
        TraceUtil.trace3("Entering");

        String loadVSN = (String) getParentViewBean().
            getPageSessionAttribute(
            Constants.PageSessionAttributes.VSN_BEING_LOADED);

        SamUtil.doPrint(new NonSyncStringBuffer(
            "updateLoadVSNHiddenField: loadVSN is ").append(loadVSN).
            toString());

        if (loadVSN != null && loadVSN.length() != 0) {
            for (int i = 0; i < model.getSize(); i++) {
                model.setRowIndex(i);
                String vsnName = (String) model.getValue("VSNText");

                // if vsnLoading array is not empty
                // check to see each element (VSN) is already loaded
                // remove the VSN from the loadVSN list if yes

                if (loadVSN.indexOf(vsnName) != -1) {
                    String informationHiddenField =
                        (String) model.getValue("InformationHiddenField");
                    String[] returnValue = informationHiddenField.split("###");

                    if (returnValue.length > 4 &&
                        returnValue[4].equals("loaded")) {
                        SamUtil.doPrint(new NonSyncStringBuffer(
                            "Remove ").append(vsnName).append(
                            " from LOAD_VSN...").toString());
                        String updateLoadVSN =
                            makeNewLoadVSNStr(loadVSN, vsnName);
                        SamUtil.doPrint(new NonSyncStringBuffer(
                            "Updated LOAD_VSN: ").append(updateLoadVSN).
                            toString());
                        ((CCHiddenField) getParentViewBean().getChild(
                            "LoadVSNHiddenField")).setValue(updateLoadVSN);
                        getParentViewBean().setPageSessionAttribute(
                            Constants.PageSessionAttributes.VSN_BEING_LOADED,
                            updateLoadVSN);
                    }
                }
            }
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * This method takes two Strings and remove the second string from the
     * first one, along with the delimitor "###".  It then returns the updated
     * String.
     */
    private String makeNewLoadVSNStr(String loadVSN, String vsnName) {
        TraceUtil.trace3("Entering");

        NonSyncStringBuffer resultStrBuf = new NonSyncStringBuffer();
        String[] vsnLoading = loadVSN.split("###");
        for (int i = 0; i < vsnLoading.length; i++) {
            if (!(vsnName.equals(vsnLoading[i]))) {
                if (resultStrBuf.length() != 0) {
                    resultStrBuf.append("###");
                }
                resultStrBuf.append(vsnLoading[i]);
            }
        }
        TraceUtil.trace3("Exiting");
        return resultStrBuf.toString();
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        updateLoadVSNHiddenField();
        checkRolePrivilege();
        setResWizardState();

        // Set table title
        if (libraryName != null && libraryName.length() != 0) {
            model.setTitle(
                SamUtil.getResourceString(
                    "VSNSummary.pageTitle1", libraryName));
        } else {
            model.setTitle(SamUtil.getResourceString("VSNSummary.pageTitle"));
        }

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
            new NonSyncStringBuffer(
                view.getQualifiedName()).
                append(".").
                append("VSNSummaryView.").
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

        CCActionTable actionTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);

        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "populateData",
                 "ModelControlException occurred within framework",
                 serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "Historian.error.populateData",
                -2509,
                "ModelControlException occurred within framework",
                serverName);
        }

        model.initModelRows();
        model.setRowSelected(false);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Choice:
     *  0  == return selected slot num
     *  1  == return selected vsn name
     *  2  == return selected barcode
     *  3  == return isReserved
     *  4  == return isLoaded
     */
    private String getSelectedRowKey(int choice)
        throws ModelControlException {
        TraceUtil.trace3(new NonSyncStringBuffer(
            "Entering: choice is ").append(choice).toString());

        // restore the selected rows
        CCActionTable actionTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);


        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "getSelectedRowKey",
                "ModelControlException occurred within framework",
                serverName);
            // just throw it, let it go to onUncaughtException()
            throw mcex;
        }

        VSNSummaryModel vsnModel = (VSNSummaryModel) actionTable.getModel();

        // single select Action Table, so just take the first row in the
        // array
        Integer [] selectedRows = vsnModel.getSelectedRows();
        int row = selectedRows[0].intValue();
        vsnModel.setRowIndex(row);

        String concatString =
            (String) vsnModel.getValue("InformationHiddenField");
        StringTokenizer tokens = new StringTokenizer(concatString, "###");
        int count = tokens.countTokens();
        String[] returnValue = new String[count];

        for (int i = 0; i < count; i++) {
            returnValue[i] = tokens.nextToken();
        }

        if (choice >= 0 && choice <= 4) {
            TraceUtil.trace3("Exiting");
            return returnValue[choice];
        } else {
            TraceUtil.trace3(
                "Exiting - Wrong key provided in getSelectedRowKey()");
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
                ReservationImpl.WIZARDPAGEMODELNAME_PREFIX).append("_").
                append(HtmlUtil.getUniqueValue()).toString();
            view.setPageSessionAttribute(
                ReservationImpl.WIZARDPAGEMODELNAME,
                modelName);
        }
        wizWinModel.setValue(ReservationImpl.WIZARDPAGEMODELNAME, modelName);

        if (implName == null) {
            implName = new NonSyncStringBuffer().append(
                ReservationImpl.WIZARDIMPLNAME_PREFIX).append("_").append(
                HtmlUtil.getUniqueValue()).toString();
            view.setPageSessionAttribute(
                ReservationImpl.WIZARDIMPLNAME,
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
            serverName);
        TraceUtil.trace3("Exiting");
    }

    /**
     * getSelectedVSN() returns the selected VSN Object
     */
    private VSN getSelectedVSN(String slotKey)
        throws SamFSException, ModelControlException {
        TraceUtil.trace3("Entering");

        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

        Library myLibrary =
            sysModel.getSamQFSSystemMediaManager().
                getLibraryByName(libraryName);
        if (myLibrary == null) {
            throw new SamFSException(null, -2502);
        }

        VSN myVSN = null;
        try {
            myVSN = myLibrary.getVSN(Integer.parseInt(slotKey));
        } catch (NumberFormatException numEx) {
            // Could not retrieve VSN
            SamUtil.doPrint(new NonSyncStringBuffer(
                "NumberFormatException caught while ").append(
                "parsing slotKey to integer.").toString());
            throw new SamFSException(null, -2507);
        }

        if (myVSN == null) {
            // Could not retrieve VSN
            throw new SamFSException(null, -2507);
        } else {
            TraceUtil.trace3("Exiting");
            return myVSN;
        }
    }

    /**
     * Check if user has permission to perform media operations.
     */
    private void checkRolePrivilege() {
        TraceUtil.trace3("Entering");

        if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {
            ((CCHiddenField) getChild(ROLE)).setValue("CONFIG");
            ((CCWizardWindow) getChild(
                "SamQFSWizardReserveButton")).setDisabled(wizardLaunched);
        } else if (SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.MEDIA_OPERATOR)) {
            ((CCHiddenField) getChild(ROLE)).setValue("MEDIA");
        } else {
            // disable the radio button row selection column
            model.setSelectionType(CCActionTableModelInterface.NONE);
        }

        TraceUtil.trace3("Exiting");
        }

} // end of VSNSummaryView class
