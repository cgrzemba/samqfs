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

// ident	$Id: FSDClusterView.java,v 1.17 2008/03/17 14:43:33 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.CommonTableContainerView;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import javax.servlet.ServletException;

/** cluster node view */
public class FSDClusterView extends CommonTableContainerView
                            implements CCPagelet {

    public static final String PAGE_NAME = "FSDClusterView";
    public static final String NODE_NAMES = "nodeNames";
    public static final String NODE_TO_REMOVE = "nodeToRemove";
    public static final String MOUNTED = "mountedNodes";
    public static final String REMOVE_CONFIRM = "removeConfirmation";

    // private children
    private CCActionTableModel tableModel = null;
    private int sunPlexManagerState = -1;

    /** constructor */
    public FSDClusterView(View parent, String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        CHILD_ACTION_TABLE = "FSDClusterTable";
        tableModel = getTableModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /** register this view's children */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(NODE_NAMES, CCHiddenField.class);
        registerChild(NODE_TO_REMOVE, CCHiddenField.class);
        registerChild(MOUNTED, CCHiddenField.class);
        registerChild(REMOVE_CONFIRM, CCHiddenField.class);
        super.registerChildren(tableModel);
        TraceUtil.trace3("Exiting");
    }

    /** create a named child view */
    public View createChild(String name) {
        if (name.equals(NODE_NAMES) ||
            name.equals(NODE_TO_REMOVE) ||
            name.equals(MOUNTED) ||
            name.equals(REMOVE_CONFIRM)) {
            return new CCHiddenField(this, name, null);
        } else {
            return super.createChild(tableModel, name);
        }
    }

    private CCActionTableModel getTableModel() {
        return new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/fs/FSDClusterTable.xml");
    }

    /** populate the cluster node table */
    private void populateTableModel() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        try {
            String fsName = (String)
                parent.getPageSessionAttribute(Constants.fs.FS_NAME);

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);

            GenericFileSystem filesystem = sysModel.
                getSamQFSSystemFSManager().getGenericFileSystem(fsName);

            // Need to check if a file system is indeed in a HA setup.
            // This is a work-around for a problem when cluster packages
            // are installed but HA is not configured.
            if (!filesystem.isHA()) {
                return;
            }

            GenericFileSystem [] instance = filesystem.getHAFSInstances();
            StringBuffer buffer = new StringBuffer();
            StringBuffer mounted = new StringBuffer();

            // remove tooltips
            ((CCRadioButton)((CCActionTable)getChild(CHILD_ACTION_TABLE)).
             getChild(CCActionTable.CHILD_SELECTION_RADIOBUTTON)).setTitle("");

            for (int i = 0; i < instance.length; i++) { // for each instance
                String hostName = instance[i].getHostName();

                if (i > 0) {
                    tableModel.appendRow();
                }

                buffer.append(hostName).append(";");
                tableModel.setValue("HostNameText", hostName);
                switch (instance[i].getState()) {
                    case FileSystem.MOUNTED:
                        tableModel.setValue("StateText",
                            SamUtil.getResourceString("FSSummary.mount"));
                        mounted.append(hostName).append(";");
                        // disable selection on mounted file systems
                        tableModel.setSelectionVisible(i, false);
                        break;
                    case FileSystem.UNMOUNTED:
                        tableModel.setValue("StateText",
                            SamUtil.getResourceString("FSSummary.unmount"));
                        break;
                    default: // do nothing
                }

                // remove any prior selections
                tableModel.setRowSelected(false);
            }
            ((CCHiddenField)getChild(NODE_NAMES)).setValue(buffer.toString());
            ((CCHiddenField)getChild(MOUNTED)).setValue(mounted.toString());
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "populateTableModel",
                                     "unable to retrieve cluster instances",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "fs.cluster.loadinfo.error",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }
    }

    /** the pagelet jsp has entered its display cycle. */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        // table buttons
        tableModel.setActionValue("Add", "common.button.add");
        tableModel.setActionValue("Remove", "common.button.remove");

        // set column headers
        tableModel.setActionValue("HostName", "common.columnheader.hostname");
        tableModel.setActionValue("State", "common.columnheader.state");

        // send the remove confirmation alert message to the client.
        ((CCHiddenField)getChild(REMOVE_CONFIRM)).setValue(
            SamUtil.getResourceString("fs.cluster.removenode.confirmation"));

        // populate the table tmodel
        populateTableModel();

        // disable the add button if the user does not have enough permissions
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.FILESYSTEM_OPERATOR)) {
            ((CCButton)getChild("Add")).setDisabled(true);
            tableModel.setSelectionType(CCActionTableModel.MULTIPLE);
        } else {
            ((CCButton)getChild("Add")).setDisabled(false);
        }

        // always disable the remove button
        ((CCButton)getChild("Remove")).setDisabled(true);
    }

    public void handleAddRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        getParentViewBean().forwardTo(getRequestContext());
    }

    /** remove a selected cluster node from the current HAFS instance */
    public void handleRemoveRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String nodeToRemove = getDisplayFieldStringValue(NODE_TO_REMOVE);
        String fsName = (String)
            parent.getPageSessionAttribute(Constants.fs.FS_NAME);

        boolean backToSummaryPage = false;
        try {
            SamQFSSystemFSManager fsManager =
                SamUtil.getModel(serverName).getSamQFSSystemFSManager();
            FileSystem fs = fsManager.getFileSystem(fsName);
            fsManager.removeHostFromHAFS(fs, nodeToRemove);

            // if we remove the current node fromt he fs, we must go back to
            // the file system summary page
            if (serverName.trim().equals(nodeToRemove.trim())) {
                backToSummaryPage = true;
            } else {
                SamUtil.setInfoAlert(parent,
                                     parent.CHILD_COMMON_ALERT,
                                     "success.summary",
                  SamUtil.getResourceString("fs.details.removenode.success",
                                            new String [] {
                                                nodeToRemove, fsName}),
                                     serverName);
            }
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "handleRemoveRequest",
                                     "unable to remove cluster node from hafs",
                                     serverName);

            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                  SamUtil.getResourceString("fs.details.removenode.error",
                                            new String [] {
                                                nodeToRemove, fsName}),
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        if (backToSummaryPage) {
            CommonViewBeanBase target = (CommonViewBeanBase)
                getViewBean(FSSummaryViewBean.class);
            SamUtil.setInfoAlert(target,
                                 target.CHILD_COMMON_ALERT,
                                 "success.summary",
            SamUtil.getResourceString("fs.details.removenode.success",
                                      new String [] {nodeToRemove, fsName}),
                                      serverName);
            parent.forwardTo(target);
        } else {
            parent.forwardTo(getRequestContext());
        }
    }

    // implement the CCPagelet interface
    public String getPageletUrl() {
        boolean hafs = false;
        boolean samqfs = false;

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();
        String fsName = (String)
            parent.getPageSessionAttribute(Constants.fs.FS_NAME);

        GenericFileSystem gfs = null;
        try {
            gfs = SamUtil.getModel(serverName).
                getSamQFSSystemFSManager().getGenericFileSystem(fsName);

            hafs = gfs.isHA();
            samqfs = (gfs.getFSTypeByProduct() != GenericFileSystem.FS_NONSAMQ);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "getPageletUrl",
                                     "unable to determine if cluster node",
                                     serverName);
            SamUtil.setErrorAlert(parent,
                                  parent.CHILD_COMMON_ALERT,
                                  "fs.details.iscluster.error",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // determine if we should show this view or not.
        if (samqfs &&
            hafs &&
            !(((FileSystem)gfs).getShareStatus()
                        == FileSystem.SHARED_TYPE_CLIENT)) {
            return "/jsp/fs/FSDClusterPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }
}
