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

// ident	$Id: AddServerViewBean.java,v 1.22 2008/12/16 00:12:24 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.ClusterNodeInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;

import com.sun.netstorage.samqfs.web.util.CommonSecondaryViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.LogUtil;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;

import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import java.net.UnknownHostException;
import java.util.HashMap;
import java.util.Map;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;

/**
 *  This class is the view bean for the Add Server page
 */

public class AddServerViewBean extends CommonSecondaryViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "AddServer";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/server/AddServer.jsp";

    public static final String ALERT = "Alert2";

    // child that used in javascript confirm messages
    public static final String CHILD_HIDDEN_MESSAGES = "HiddenMessages";

    // hidden field for the handlePreSubmit javascript to assign selected nodes
    public static final String SELECTED_NODES = "SelectedNodes";

    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_NAME_VALUE = "nameValue";

    // Pagelet view for Add Cluster View
    public static final String ADD_CLUSTER_VIEW = "AddClusterView";

    // Page Title Attributes and Components.
    private CCPageTitleModel pageTitleModel = null;

    // table models
    private Map models = null;

    /**
     * Constructor
     */
    public AddServerViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        pageTitleModel = createPageTitleModel();
        registerChildren();
        initializeTableModels();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_NAME_VALUE, CCTextField.class);
        registerChild(CHILD_HIDDEN_MESSAGES, CCHiddenField.class);
        registerChild(SELECTED_NODES, CCHiddenField.class);
        registerChild(ADD_CLUSTER_VIEW, AddClusterView.class);
        registerChild(ALERT, CCAlertInline.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
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
        } else if (name.equals(CHILD_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_HIDDEN_MESSAGES)) {
            child = new CCHiddenField(this, name,
                new NonSyncStringBuffer(
                    SamUtil.getResourceString("AddServer.errMsg1")).
                        append(ServerUtil.delimitor).append(
                    SamUtil.getResourceString("AddServer.errMsg2")).
                        append(ServerUtil.delimitor).append(
                    SamUtil.getResourceString("AddServer.errMsg3")).
                        append(ServerUtil.delimitor).append(
                    SamUtil.getResourceString("AddCluster.error.selectone")).
                        toString());
        } else if (name.equals(SELECTED_NODES)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_NAME_VALUE)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(ADD_CLUSTER_VIEW)) {
            child = new AddClusterView(this, models, name);
        } else if (name.equals(ALERT)) {
            child = new CCAlertInline(this, name, null);
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Handler function to handle the request of Submit button
     * NOTE: The handler function is located in ServerSelectionViewBean
     * due to the submit button behavior issue.
     */

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = PageTitleUtil.createModel(
                "/jsp/server/AddServerPageTitle.xml");
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    private void initializeTableModels() {
	models = new HashMap();
	ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

	// server table
	CCActionTableModel model = new CCActionTableModel(
            sc, "/jsp/server/AddClusterTable.xml");

	models.put(AddClusterView.CLUSTER_TABLE, model);
    }

    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");

        String clusterNode = (String) getParentViewBean().
            getPageSessionAttribute(
                Constants.PageSessionAttributes.CLUSTER_NODE);
        // Check if we are adding a single host, or adding multiple cluster
        // nodes
        if (clusterNode == null) {
            addSingleHost();
        } else {
            addClusterHosts();
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * Populate all cluster members in the table that resides in the cluster
     * view.
     */
    public void populateAddClusterTableModel() {
        AddClusterView clusterView =
            (AddClusterView) getChild(ADD_CLUSTER_VIEW);
        clusterView.populateTableModel();
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        super.beginDisplay(evt);

        String clusterNode = (String) getParentViewBean().
            getPageSessionAttribute(
                Constants.PageSessionAttributes.CLUSTER_NODE);

        if (clusterNode != null && clusterNode.length() != 0) {
            populateAddClusterTableModel();

            ((CCLabel) getChild(CHILD_LABEL)).setValue(
                SamUtil.getResourceString("AddServer.label.addedserver"));
            ((CCTextField) getChild(CHILD_NAME_VALUE)).setReadOnly(true);
        }
        TraceUtil.trace3("Exiting");
    }

    private void addSingleHost() {
        SamQFSAppModel appModel = null;
        boolean hasError = false;
        String nameInput =
            (String) ((CCTextField) getChild(CHILD_NAME_VALUE)).getValue();
        nameInput = nameInput.trim();

        try {
            appModel = SamQFSFactory.getSamQFSAppModel();
            if (appModel == null) {
                throw new SamFSException(null, -2501);
            }

            LogUtil.info(this.getClass(), "handleCancelHrefRequest",
                new NonSyncStringBuffer().append("Start adding host ").append(
                nameInput).toString());
            appModel.addHost(nameInput, true);
            LogUtil.info(this.getClass(), "handleCancelHrefRequest",
                new NonSyncStringBuffer().append("Done adding host ").append(
                nameInput).toString());
            SamUtil.setInfoAlert(
                getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "ServerSelection.action.add", nameInput),
                "");
        } catch (SamFSException ex) {
            hasError = true;
            SamUtil.processException(ex, this.getClass(),
                "handleSubmitRequest()",
                "Failed to add server",
                nameInput);
            SamUtil.setErrorAlert(getParentViewBean(),
                CommonSecondaryViewBeanBase.ALERT,
                "ServerSelection.error.add",
                ex.getSAMerrno(),
                ex.getMessage(),
                nameInput);
        }

        if (!hasError) {
            try {
                SamQFSSystemModel sysModel = SamUtil.getModel(nameInput);
                boolean isClusterNode = sysModel.isClusterNode();

                if (isClusterNode &&
                    hasMoreNodesToAdd(appModel, sysModel)) {
                    getParentViewBean().setPageSessionAttribute(
                        Constants.PageSessionAttributes.CLUSTER_NODE,
                        nameInput);
                } else {
                    setSubmitSuccessful(true);
                }
            } catch (SamFSException ex) {
                SamUtil.processException(ex, this.getClass(),
                    "handleSubmitRequest()",
                    "Failed to check if the added host is a part of cluster",
                    nameInput);
                SamUtil.setErrorAlert(getParentViewBean(),
                    CommonSecondaryViewBeanBase.ALERT,
                    "AddCluster.error.showall",
                    ex.getSAMerrno(),
                    ex.getMessage(),
                    nameInput);
            }
        }
    }

    private boolean hasMoreNodesToAdd(
        SamQFSAppModel appModel, SamQFSSystemModel sysModel) {

        try {
            // retrieve the list of nodes that are eligible to be added
            ClusterNodeInfo [] nodeInfos = sysModel.getClusterNodes();

            // if the array is null or empty, return false
            if (nodeInfos == null || nodeInfos.length == 0) {
                return false;
            }

            // check each host in nodeinfos, return true if there is at least
            // one eligible host to be added
            for (int i = 0; i < nodeInfos.length; i++) {
                if (appModel.getInetHostName(nodeInfos[i].getName()) == null &&
                    !appModel.isHostBeingManaged(nodeInfos[i].getName())) {
                    return true;
                } else {
                    continue;
                }
            }

        } catch (SamFSException ex) {
            TraceUtil.trace1("Exception caught while checking if " +
               "there are more cluster nodes to add!");
            return false;
        } catch (UnknownHostException ex) {
            TraceUtil.trace1("Exception caught while checking if " +
		"there are more cluster nodes to add!");
            return false;
	}

        return false;
    }

    private void addClusterHosts() {
        String clusterNode = (String) getParentViewBean().
            getPageSessionAttribute(
                Constants.PageSessionAttributes.CLUSTER_NODE);

        String serverlist = (String) getDisplayFieldValue(SELECTED_NODES);
        String [] serverArray = serverlist.split(", ");

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            if (appModel == null) {
                throw new SamFSException(null, -2501);
            }

            for (int i = 0; i < serverArray.length; i++) {
                LogUtil.info(
                    this.getClass(),
                    "addClusterHosts",
                    new NonSyncStringBuffer("Start adding node ").append(
                        serverArray[i]).toString());
                appModel.addHost(serverArray[i], true);
                LogUtil.info(
                    this.getClass(),
                    "addClusterHosts",
                    new NonSyncStringBuffer("Done adding node ").append(
                        serverArray[i]).toString());
            }

            SamUtil.setInfoAlert(
                getParentViewBean(),
                ALERT,
                "success.summary",
                SamUtil.getResourceString(
                    "ServerSelection.action.add", serverlist),
                "");
            setSubmitSuccessful(true);

        } catch (SamFSException samEx) {
            TraceUtil.trace1("SamFSException caught!");
            TraceUtil.trace1(
                "Error adding  selected cluster nodes. Reason: " +
                samEx.getMessage());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                ALERT,
                "AddCluster.error.showall",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                clusterNode);
        }

        removePageSessionAttribute(
            Constants.PageSessionAttributes.CLUSTER_NODE);
    }
}
