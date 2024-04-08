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

// ident	$Id: LibraryFaultsSummaryView.java,v 1.28 2008/12/16 00:12:14 am143972 Exp $

package com.sun.netstorage.samqfs.web.media;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;

import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.table.CCActionTable;
import com.sun.web.ui.view.html.CCCheckBox;

import java.io.IOException;
import java.util.ArrayList;

import javax.servlet.ServletException;
import javax.servlet.http.HttpSession;

public class LibraryFaultsSummaryView extends CommonTableContainerView {

    // child name for tiled view class
    public static final String CHILD_TILED_VIEW =
        "LibraryFaultsSummaryTiledView";

    // for filter use
    public static final String CHILD_FILTERMENU_HREF    = "FilterMenuHref";
    private ArrayList updatedModelIndex = new ArrayList();
    private LibraryFaultsSummaryModel model = null;
    private String showType = null;

    /**
     * Construct an instance with the specified properties.
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public LibraryFaultsSummaryView(View parent, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "LibraryFaultsSummaryTable";
        model = new LibraryFaultsSummaryModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_TILED_VIEW, LibraryFaultsSummaryTiledView.class);
        registerChild(CHILD_FILTERMENU_HREF, CCHref.class);
        super.registerChildren(model);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        TraceUtil.trace3("Entering");

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
            child = new LibraryFaultsSummaryTiledView(this, model, name);
        } else if (name.equals(CHILD_FILTERMENU_HREF)) {
            child = new CCHref(this, name, null);
        } else {
            child = super.createChild(model, name,
                LibraryFaultsSummaryView.CHILD_TILED_VIEW);
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    public void handleAcknowledgeButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        execute(0);
        TraceUtil.trace3("Exiting");
    }

    public void handleDeleteButtonRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        execute(1);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Handle request to handle the filter drop-down menu
     *  @param RequestInvocationEvent event
     */
    public void handleFilterMenuHrefRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        String selectedType = (String) getDisplayFieldValue("FilterMenu");

        // This needs to be kept in session, do not touch
        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();

        if (selectedType != null) {
            if (selectedType.equals("allItemsOption")) {
                selectedType = Constants.Alarm.ALARM_ALL;
            }
            session.setAttribute(
                Constants.SessionAttributes.MEDIAFILTER_MENU,
                selectedType);
        } else {
            session.setAttribute(
                Constants.SessionAttributes.MEDIAFILTER_MENU,
                Constants.Alarm.ALARM_ALL);
        }

        model.setFilter(selectedType);

        try {
            model.initModelRows(getServerName(), getLibraryName());
        } catch (SamFSException samEx) {
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleFilterMenuHrefRequest()",
                "Failed to populate faults summary information",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "CurrentAlarmSummary.error.failedPopulate",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }

        updatedModelIndex = model.getLatestIndex();
        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.SAM_CONTROL)) {
            // disable the radio button row selection column
            model.setSelectionType("none");
        } else {
            // de-select all rows and disable selection tooltip
            CCActionTable myTable =
                (CCActionTable) getChild(CHILD_ACTION_TABLE);

            for (int i = 0; i < model.getSize(); i++) {
                model.setRowSelected(i, false);
                CCCheckBox myCheckBox =
                (CCCheckBox) myTable.getChild(
                     CCActionTable.CHILD_SELECTION_CHECKBOX + i);
                myCheckBox.setTitle("");
                myCheckBox.setTitleDisabled("");
            }
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * method to execute acknowledge, or delete operation
     */
    private void execute(int buttonID) throws ModelControlException {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3(new NonSyncStringBuffer("buttonID is ").
            append(buttonID).toString());

        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        String idString = null, op = null;
        boolean hasError = false;

        if (buttonID == 0) {
            idString = "handleAcknowledgeButtonRequest";
        } else if (buttonID == 1) {
            idString = "handleDeleteButtonRequest";
        }

        // restore the selected rows
        CCActionTable actionTable =
            (CCActionTable) getChild(CHILD_ACTION_TABLE);
        try {
            actionTable.restoreStateData();
        } catch (ModelControlException mcex) {
            SamUtil.processException(
                mcex,
                this.getClass(),
                "execute",
                "ModelControlException occurred within framework",
                getServerName());
            // just throw it, let it go to onUncaughtException()
            throw mcex;
        }

        LibraryFaultsSummaryModel alarmModel = (LibraryFaultsSummaryModel)
            actionTable.getModel();

        // multiple select Action Table
        Integer [] selectedRows = alarmModel.getSelectedRows();
        long alarmIndexes[] = new long[selectedRows.length];
        long alarmIndex = -1;

        try {
            if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.SAM_CONTROL)) {
                throw new SamFSException("common.nopermission");
            }

            for (int k = 0; k < selectedRows.length; k++) {
                int row = selectedRows[k].intValue();
                alarmModel.setRowIndex(row);
                alarmIndex = Long.parseLong(
                    (String) alarmModel.getValue("IDHidden"));
                TraceUtil.trace3(new NonSyncStringBuffer(
                    "execute: alarmIndex is ").append(alarmIndex).toString());

                alarmIndexes[k] = alarmIndex;

                // Construct the alert message
                if (buffer.length() > 0) {
                    buffer.append(", ");
                }
                buffer.append(alarmIndex);
            }

            // Set Acknowledge of each selected alarms
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            switch (buttonID) {
                case 0:
                    op = "LibraryFaultsSummary.action.acknowledge";
                    LogUtil.info(this.getClass(), idString,
                        new NonSyncStringBuffer(
                        "Start acknowledging the alarm").append(alarmIndex).
                        toString());
                    sysModel.getSamQFSSystemAlarmManager().
                        acknowledgeAlarm(alarmIndexes);
                    LogUtil.info(this.getClass(), idString,
                        "Done acknowledging the alarm");
                    break;

                case 1:
                    op = "LibraryFaultsSummary.action.delete";
                    LogUtil.info(this.getClass(), idString,
                        new NonSyncStringBuffer(
                        "Start deleting alarm ID ").append(alarmIndex).
                        toString());
                    sysModel.getSamQFSSystemAlarmManager().
                        deleteAlarm(alarmIndexes);
                    LogUtil.info(this.getClass(), idString,
                        new NonSyncStringBuffer("Done deleting the alarm ").
                        append(alarmIndex).toString());
                        break;
            }
        } catch (SamFSException ex) {
            hasError = true;
            String errString = (buttonID == 0) ?
                "LibraryFaultsSummary.error.acknowledge" :
                "LibraryFaultsSummary.error.delete";

            SamUtil.processException(
                ex,
                this.getClass(),
                "execute",
                errString,
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                errString,
                ex.getSAMerrno(), ex.getMessage(),
                getServerName());
        }

        if (!hasError) {
            setSuccessAlert(op, buffer.toString());
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    public void populateData(String serverName, String libraryName, String type)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        showType = type;
        SamUtil.doPrint(new NonSyncStringBuffer("showType is ").
            append(showType).toString());
        ((CCDropDownMenu) getChild("FilterMenu")).setValue(showType);

        model.setFilter(showType);
        model.initModelRows(serverName, libraryName);
        updatedModelIndex = model.getLatestIndex();
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

    private String getLibraryName() {
        return (String) getParentViewBean().getPageSessionAttribute(
            Constants.PageSessionAttributes.LIBRARY_NAME);
    }

    private String getServerName() {
        return (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
    }

} // end of LibraryFaultsSummaryView class
