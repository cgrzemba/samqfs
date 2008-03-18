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

// ident	$Id: AddClusterNodePopupViewBean.java,v 1.13 2008/03/17 14:43:32 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.ClusterNodeInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.model.fs.GenericFileSystem;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.table.CCActionTable;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import javax.servlet.ServletException;

/**
 * the add cluster node to file system popup parent view
 */
public class AddClusterNodePopupViewBean extends CommonSecondaryViewBeanBase {
    public static final String PAGE_NAME = "AddClusterNodePopup";
    public static final String DEFAULT_URL = "/jsp/fs/AddClusterNodePopup.jsp";

    public static final String NODE_TABLE = "FSDAddClusterNodeTable";
    public static final String NODE_NAMES = "nodeNames";
    public static final String SELECTED_NODES = "selectedNodes";
    public static final String NO_SELECTION_MSG = "noSelectionMsg";

    private CCPageTitleModel ptModel = null;
    private CCActionTableModel tableModel = null;

    /** default constructor */
    public AddClusterNodePopupViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // create table & page title model
        ptModel = getPageTitleModel();
        tableModel = getTableModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register this view's children */
    protected void registerChildren() {
        registerChild(NODE_NAMES, CCHiddenField.class);
        registerChild(SELECTED_NODES, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, ptModel);
        registerChild(NODE_TABLE, CCActionTable.class);
        tableModel.registerChildren(this);
        super.registerChildren();
        registerChild(NO_SELECTION_MSG, CCHiddenField.class);
    }

    /** create a named child view */
    public View createChild(String name) {
        if (name.equals(NODE_NAMES) ||
            name.equals(SELECTED_NODES) ||
            name.equals(NO_SELECTION_MSG)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(NODE_TABLE)) {
            return new CCActionTable(this, tableModel, name);
        } else if (tableModel.isChildSupported(name)) {
            return tableModel.createChild(this, name);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name +"'");
        }
    }

    private CCActionTableModel getTableModel() {
        return new CCActionTableModel(
            RequestManager.getRequestContext().getServletContext(),
            "/jsp/fs/AddClusterNodeTable.xml");
    }

    private CCPageTitleModel getPageTitleModel() {
        return PageTitleUtil.createModel("/jsp/fs/AddClusterNodePageTitle.xml");
    }

    /** get the list of nodes already visible to file system */
    private List getCurrentNodes() throws SamFSException {
        String serverName = getServerName();
        List nodes = new ArrayList();
        GenericFileSystem filesystem = SamUtil.getModel(serverName).
            getSamQFSSystemFSManager().getGenericFileSystem(getFSName());

        if (filesystem == null) {
            return nodes;
        }

        GenericFileSystem [] instance = filesystem.getHAFSInstances();

        for (int i = 0; i < instance.length; i++) {
            nodes.add(instance[i].getHostName());
        }

        return nodes;
    }

    /** populate the table model */
    private void populateTableModel() {
        // set the selection type
        tableModel.setSelectionType(CCActionTableModel.MULTIPLE);

        String serverName = getServerName();
        String fsName = getFSName();

        CCActionTable theTable = (CCActionTable)getChild(NODE_TABLE);
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ClusterNodeInfo [] nodeInfo = sysModel.getClusterNodes();
            List currentNodes = getCurrentNodes();

            // loop through the nodes and only add those that don't already see
            // the file system
            StringBuffer buffer = new StringBuffer();
            int rowCount = 0;

            for (int i = 0; i < nodeInfo.length; i++) {
                if (currentNodes.contains(nodeInfo[i].getName()))
                    continue;

                if (i > 0) {
                    tableModel.appendRow();
                }


                // populate the table model
                tableModel.setValue("NodeNameText", nodeInfo[i].getName());
                tableModel.setValue("StateText", nodeInfo[i].getStatus());
                tableModel.setValue("IDText", new Integer(nodeInfo[i].getID()));
                tableModel.setValue("PrivateAddressText",
                nodeInfo[i].getPrivIP());

                buffer.append(nodeInfo[i].getName()).append(";");

                // disable tool tips
                ((CCCheckBox)theTable.getChild(
                CCActionTable.
                    CHILD_SELECTION_CHECKBOX + rowCount)).setTitle("");

                // bump the row count
                rowCount++;
            }

            // all the nodes are already part of this fs, disabled the add to
            // nodes button
            if (rowCount < 1)
                ((CCButton)getChild("Submit")).setDisabled(true);

            // save the node names
            ((CCHiddenField)getChild(NODE_NAMES)).setValue(buffer.toString());
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "populateTableModel",
                                     "unable to retrieve cluster nodes",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "Error retrieving cluster information",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);

            // disable the add button
            ((CCButton)getChild("Submit")).setDisabled(true);
        }
    }

   /** retrieve the name of the file system we are dealing with */
    private String getFSName() {
        String fsName = (String)getPageSessionAttribute(Constants.fs.FS_NAME);
        if (fsName == null) {
            fsName = RequestManager.getRequest().
                getParameter(Constants.fs.FS_NAME);
            setPageSessionAttribute(Constants.fs.FS_NAME, fsName);
        }
        return fsName;
    }

    /** JSP has begun dispaly cycle */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        super.beginDisplay(evt);

        // initialize table headers
        // borrow these from the server selection page to avoid duplication
        tableModel.setActionValue("NodeName", "AddCluster.heading.name");
        tableModel.setActionValue("State", "AddCluster.heading.status");
        tableModel.setActionValue("ID", "AddCluster.heading.id");
        tableModel.setActionValue("PrivateAddress",
                                  "AddCluster.heading.privateip");

        // populate the table model
        populateTableModel();

        // resolve & set error strings
        ((CCHiddenField)getChild(NO_SELECTION_MSG)).setValue(
            SamUtil.getResourceString("fs.cluster.addnode.noselection"));
    }

    /** handler for the submit button */
    public void handleSubmitRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {

        String temp = getDisplayFieldStringValue(SELECTED_NODES);
        String [] selectedNode = temp.split(";");

        // the currently managed server and fs name
        String serverName = getServerName();
        String fsName = getFSName();

        try {
            SamQFSSystemFSManager fsManager = SamUtil.getModel(serverName).
                getSamQFSSystemFSManager();
            FileSystem fs = fsManager.getFileSystem(getFSName());

            for (int i = 0; i < selectedNode.length; i++) {
                fsManager.addHostToHAFS(fs, selectedNode[i]);
            }

            SamUtil.setInfoAlert(this,
                                  ALERT,
                                  "success.summary",
                                  "fs.cluster.addnode.success",
                                  serverName);
            setSubmitSuccessful(true);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "handleSubmitRequest",
                                     "unable to add cluster nodes to fs",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  ALERT,
                                  "fs.cluster.addnode.failure",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // recycle the page
        forwardTo(getRequestContext());
    }
}
