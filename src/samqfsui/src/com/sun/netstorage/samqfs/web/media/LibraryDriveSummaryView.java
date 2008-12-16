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

// ident	$Id: LibraryDriveSummaryView.java,v 1.37 2008/12/16 00:12:13 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.media.Drive;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;

import java.io.IOException;
import javax.servlet.ServletException;



public class LibraryDriveSummaryView extends CommonTableContainerView {

    public static final
        String CHILD_TILED_VIEW = "LibraryDriveSummaryTiledView";
    private LibraryDriveSummaryModel model = null;

    // Child View names associated with change status pop-up
    public static final String CHILD_EQ_HIDDEN_FIELD = "EQHiddenField";

    public static final String ACTION_HREF = "ActionMenuHref";

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public LibraryDriveSummaryView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "LibraryDriveSummaryTable";
        model = new LibraryDriveSummaryModel(isSharedCapable());
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren(model);
        registerChild(CHILD_TILED_VIEW, LibraryDriveSummaryTiledView.class);
        registerChild(CHILD_EQ_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(ACTION_HREF, CCHref.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(CHILD_EQ_HIDDEN_FIELD)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_TILED_VIEW)) {
            // The ContainerView used to register and create children
            // for "column" elements defined in the model's XML
            // document. Note that we're creating a seperate
            // ContainerView only to evoke JATO's TiledView behavior
            // for these elements. If TiledView behavior is not
            // required, creating a ContainerView object is not
            // necessary. By default, CCActionTable will attempt to
            // retrieve all children from it's parent (i.e., this view).
            child = new LibraryDriveSummaryTiledView(this, model, name);
        } else if (name.equals(ACTION_HREF)) {
            return new CCHref(this, name, null);
        } else {
            return super.createChild(model, name,
                LibraryDriveSummaryView.CHILD_TILED_VIEW);
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /* Idle */
    public void handleIdleButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        executeCommand("handleIdleButtonRequest");
        TraceUtil.trace3("Existing");
    }

    /* Unload */
    public void handleUnloadButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        executeCommand("handleUnloadButtonRequest");
        TraceUtil.trace3("Existing");
    }

    /* Clean */
    public void handleCleanButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        executeCommand("handleCleanButtonRequest");
        TraceUtil.trace3("Existing");
    }

    public void handleActionMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        int value = 2;
        try {
            value = Integer.parseInt(
                (String) getDisplayFieldValue("ActionMenu"));
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("NumEx caught while parsing drop down selection!");
        }
        executeCommand("handleActionMenuHrefRequest" + value);

        // set the drop-down menu to default value
        ((CCDropDownMenu) getChild("ActionMenu")).setValue("0");

        TraceUtil.trace3("Existing");
    }


    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.CONFIG)) {
            // disable the radio button row selection column
            model.setSelectionType("none");
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
     * executeCommand() is a method that will perform all 3 actions (buttons)
     * on Library Drive Summary page. (idle, unload, clean)
     */
    private void executeCommand(String handlerName)
                throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3(new NonSyncStringBuffer().append("handlerName is ").
            append(handlerName).toString());
        String eq = getSelectedRowKey(0);

        try {
            String libraryName = getLibraryName();
            if (libraryName == null) {
                throw new SamFSException(null, -2502);
            }

            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            Library myLibrary = sysModel.getSamQFSSystemMediaManager().
               getLibraryByName(libraryName);
            if (myLibrary == null) {
                throw new SamFSException(null, -2502);
            } else {
                Drive allDrives[] = myLibrary.getDrives();
                Drive myDrive = null;

                if (allDrives == null) {
                    throw new SamFSException(null, -2503);
                } else {
                    for (int i = 0; i < allDrives.length; i++) {
                        if (allDrives[i].getEquipOrdinal() ==
                            Integer.parseInt(eq)) {
                            myDrive = allDrives[i];
                        }
                    }
                }

                if (myDrive == null) {
                    throw new SamFSException(null, -2503);
                } else {
                    String op = null;
                    // Idle
                    if (handlerName.equals("handleIdleButtonRequest")) {
                        op = "LibraryDriveSummary.action.idle";
                        LogUtil.info(this.getClass(), handlerName,
                            new NonSyncStringBuffer(
                                "Start idling drive with eq ").append(eq).
                                toString());
                        myDrive.idle();
                        LogUtil.info(this.getClass(), handlerName,
                            new NonSyncStringBuffer(
                                "Done idling drive with eq ").append(eq).
                                toString());

                    // Unload
                    } else if (handlerName.equals
                        ("handleUnloadButtonRequest")) {
                        op = "LibraryDriveSummary.action.unload";
                        LogUtil.info(this.getClass(), handlerName,
                            new NonSyncStringBuffer(
                                "Start unloading tape in drive with eq").append(
                                eq).toString());
                        myDrive.unload();
                        LogUtil.info(this.getClass(), handlerName,
                            new NonSyncStringBuffer(
                                "Done unloading tape in drive with eq").append(
                                eq).toString());

                    // Clean
                    } else if (handlerName.equals("handleCleanButtonRequest")) {
                        op = "LibraryDriveSummary.action.clean";
                        LogUtil.info(this.getClass(), handlerName,
                            new NonSyncStringBuffer(
                                "Start cleaning drive with eq ").append(eq).
                                toString());
                        myDrive.clean();
                        LogUtil.info(this.getClass(), handlerName,
                            new NonSyncStringBuffer(
                                "Done cleaning drive with eq ").append(eq).
                                toString());
                    } else if (
                        handlerName.startsWith("handleActionMenuHrefRequest")) {
                        int value =
                            Integer.parseInt(handlerName.substring(27, 28));
                        op = (value == 1) ?
                            "LibraryDriveSummary.action.share" :
                            "LibraryDriveSummary.action.unshare";
                        LogUtil.info(this.getClass(), handlerName,
                            value == 1 ?
                                new NonSyncStringBuffer(
                                    "Start sharing drive ").
                                    append(eq).toString() :
                                new NonSyncStringBuffer(
                                    "Start unsharing drive ").
                                    append(eq).toString());
                        myDrive.setShared(value == 1);
                        LogUtil.info(this.getClass(), handlerName,
                            value == 1 ?
                                new NonSyncStringBuffer(
                                    "Done sharing drive ").
                                    append(eq).toString() :
                                new NonSyncStringBuffer(
                                    "Done unsharing drive ").
                                    append(eq).toString());
                    }
                    setSuccessAlert(op, eq);
                }
            } // end if (myLibrary == null)

        } catch (SamFSException samEx) {
            String errString = null;
            if (handlerName.equals("handleIdleButtonRequest")) {
                errString = "LibraryDriveSummary.error.idle";
            } else if (handlerName.equals("handleUnloadButtonRequest")) {
                errString = "LibraryDriveSummary.error.unload";
            } else if (handlerName.equals("handleCleanButtonRequest")) {
                errString = "LibraryDriveSummary.error.clean";
            } else if (
                handlerName.startsWith("handleActionMenuHrefRequest")) {
                int value =
                    Integer.parseInt(handlerName.substring(27, 28));
                switch (value) {
                    case 1:
                        errString = "LibraryDriveSummary.error.share";
                        break;
                    case 2:
                        errString = "LibraryDriveSummary.error.unshare";
                        break;
                }
            }

            setErrorAlertWithProcessException(
                samEx,
                handlerName,
                errString,
                errString);
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void populateData(String serverName, String libraryName)
        throws SamFSException {
        TraceUtil.trace3("Entering");
        model.initModelRows(serverName, libraryName);
        model.setRowSelected(false);
        TraceUtil.trace3("Exiting");
    }

    private String getSelectedRowKey(int choice) throws ModelControlException {
        TraceUtil.trace3("Entering");
        String returnValue = null;

        // restore the selected rows
        CCActionTable actionTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);
        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(mcex, this.getClass(),
                "getSelectedRowKey()",
                "ModelControlException occurred within framework",
                getServerName());
            // just throw it, let it go to onUncaughtException()
            throw mcex;
        }

        LibraryDriveSummaryModel drvModel =
            (LibraryDriveSummaryModel) actionTable.getModel();

        // single select Action Table, so just take the first row in the
        // array
        Integer [] selectedRows = drvModel.getSelectedRows();
        int row = selectedRows[0].intValue();
        drvModel.setRowIndex(row);

        switch (choice) {
            case 0:
               // return drive eq
                returnValue = (String) drvModel.getValue("EQHidden");
                TraceUtil.trace3(new NonSyncStringBuffer(
                    "Selected Drive's EQ is ").append(returnValue).toString());
                break;

            case 1:
                // return drive state
                returnValue = (String) drvModel.getValue("StateHidden");
                TraceUtil.trace3(new NonSyncStringBuffer(
                    "Selected Drive's state is ").append(
                        returnValue).toString());
                break;
        }

        TraceUtil.trace3("Exiting");
        return returnValue;
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

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

    private String getLibraryName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
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

    private Drive getSelectedDrive()
        throws SamFSException, ModelControlException {

        String eqValue = getSelectedRowKey(0);
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        Library myLibrary = sysModel.getSamQFSSystemMediaManager().
            getLibraryByName(getLibraryName());
        if (myLibrary == null) {
            throw new SamFSException(null, -2502);
        }

        Drive allDrives[] = null, myDrive = null;
        allDrives = myLibrary.getDrives();
        if (allDrives == null) {
            throw new SamFSException(null, -2503);
        }

        for (int i = 0; i < allDrives.length; i++) {
            if (allDrives[i].getEquipOrdinal() ==
                Integer.parseInt(eqValue)) {
                myDrive = allDrives[i];
            }
        }
        if (myDrive == null) {
            throw new SamFSException(null, -2503);
        } else {
            return myDrive;
        }
    }

    private boolean isSharedCapable() {
        boolean sharedCapable =
            Boolean.valueOf(
                (String) getParentViewBean().getPageSessionAttribute(
                    Constants.PageSessionAttributes.SHARE_CAPABLE)).
                    booleanValue();
        return sharedCapable;
    }

} // end of LibraryDriveSummaryView class
