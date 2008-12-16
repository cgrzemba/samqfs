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

// ident	$Id: AddClusterView.java,v 1.10 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.MultiTableViewBase;
import com.sun.netstorage.samqfs.web.model.ClusterNodeInfo;

import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.table.CCActionTable;

import java.util.Map;
import java.net.UnknownHostException;


/**
 * AddClusterView is the view class in Add Server Page. When user is adding a
 * server that is a part of a cluster, this pagelet will be shown and ask users
 * if they want to add the members of the clusters to the managed server list.
 */
public class AddClusterView extends MultiTableViewBase
    implements CCPagelet {

    public static final String STATIC_TEXT = "StaticText";
    public static final String CLUSTER_TABLE = "AddClusterTable";
    public static final String NODE_NAMES = "NodeNames";

    /** create an instance of AddClusterView */
    public AddClusterView(View parent, Map models, String name) {
        super(parent, models, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * register page children
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(STATIC_TEXT, CCStaticTextField.class);
        registerChild(NODE_NAMES, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * create child for JATO display cycle
     */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;

        if (name.equals(STATIC_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CLUSTER_TABLE)) {
            child = createTable(name);
        } else if (name.equals(NODE_NAMES)) {
            child = new CCHiddenField(this, name, null);
        } else {
            CCActionTableModel model = super.isChildSupported(name);
            if (model != null) {
                child = super.isChildSupported(name).createChild(this, name);
            }
        }

        if (child == null) {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /** initialize table header and radio button */
    protected void initializeTableHeaders() {
        CCActionTableModel model = getTableModel(CLUSTER_TABLE);

        // set the column headers
        model.setActionValue("NameColumn", "AddCluster.heading.name");
        model.setActionValue("StatusColumn", "AddCluster.heading.status");
        model.setActionValue("IDColumn", "AddCluster.heading.id");
        model.setActionValue(
            "PrivateIPColumn", "AddCluster.heading.privateip");
    }

    /** populate the criteria table model */
    public void populateTableModel() {
        String alreadyAddedClusterNode =
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.CLUSTER_NODE);

        // Retrieve the handle of the Server Selection Table
        CCActionTableModel tableModel = getTableModel(CLUSTER_TABLE);
        tableModel.clear();

        int counter = 0;

        try {
            SamQFSAppModel appModel =
                SamQFSFactory.getSamQFSAppModel();
            SamQFSSystemModel sysModel =
                appModel.getSamQFSSystemModel(alreadyAddedClusterNode);

            // buf to concatenate all available node names and eventually
            // assigned to "NodeName" (Hidden).  This is used by the
            // handPreSubmit javascript in order to avoid adding a tiled view
            // to this node table
            NonSyncStringBuffer nodeBuf = new NonSyncStringBuffer();

            String clusterVersion = sysModel.getClusterVersion();
            String clusterName    = sysModel.getClusterName();

            // Set Table title
            tableModel.setTitle(
                SamUtil.getResourceString("AddCluster.tabletitle",
                new String [] {alreadyAddedClusterNode, clusterVersion}));

            ClusterNodeInfo [] nodeInfos = sysModel.getClusterNodes();

            CCActionTable myTable = (CCActionTable) getChild(CLUSTER_TABLE);

            // populate the table model

            for (int i = 0; i < nodeInfos.length; i++) {
                String nodeName = nodeInfos[i].getName();
                if (alreadyAddedClusterNode.equals(nodeName)) {
                    // this host is added already in the first OK clicked
                    continue;
                } else if (appModel.isHostBeingManaged(nodeName)) {
                    // this host already exists in host.conf
                    continue;
                } else {
                    // check if its already in the list
                    if (appModel.getInetHostName(nodeName) == null) {
                        // Disable Tooltip
                        CCCheckBox myCheckBox =
                            (CCCheckBox) myTable.getChild(
                                CCActionTable.CHILD_SELECTION_CHECKBOX +
                                counter);
                        myCheckBox.setTitle("");
                        myCheckBox.setTitleDisabled("");

                        if (counter++ > 0) {
                            tableModel.appendRow();
                        }

                        // Exception is caught, this host does not exist
                        // go ahead and add it
                        tableModel.setValue("NameText", nodeName);
                        tableModel.setValue(
                            "StatusText", nodeInfos[i].getStatus());
                        tableModel.setValue(
                            "IDText", new Integer(nodeInfos[i].getID()));
                        tableModel.setValue(
                            "PrivateIPText", nodeInfos[i].getPrivIP());

                        nodeBuf.append(nodeName).append(";");
                    }
                }
            }

            // assign node name strings to hidden field
            ((CCHiddenField) getChild(NODE_NAMES)).
                setValue(nodeBuf.toString());
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                this.getClass(),
                "populateTableModel",
                "unable to populate table model",
                alreadyAddedClusterNode);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "AddCluster.error.showall",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                alreadyAddedClusterNode);
        } catch (UnknownHostException ex) {
            SamUtil.setErrorAlert(
		getParentViewBean(),
		CommonSecondaryViewBeanBase.ALERT,
		"AddCluster.error.showall",
		-2551,
		ex.getMessage(),
                alreadyAddedClusterNode);
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        initializeTableHeaders();

        // set instructions
        String clusterNode =
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.CLUSTER_NODE);

        if (clusterNode != null) {
            // Set instructions if cluster table is shown
            ((CCStaticTextField) getChild(STATIC_TEXT)).setValue(
                SamUtil.getResourceString(
                    "AddCluster.instruction",
                    new String [] {clusterNode}));
        }

        TraceUtil.trace3("Exiting");
    }

    // implement the CCPagelet interface

    /**
     * return the appropriate pagelet jsp
     */
    public String getPageletUrl() {
        String clusterNode =
            (String) getParentViewBean().getPageSessionAttribute(
                Constants.PageSessionAttributes.CLUSTER_NODE);

        if (clusterNode != null) {
            return "/jsp/server/AddClusterPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }
}
