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

// ident	$Id: FrameNavigatorViewBean.java,v 1.29 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.web.ui.model.CCNavNode;
import com.sun.web.ui.model.CCTreeModel;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.tree.CCClientSideTree;

/**
 *  This class is ViewBean of the Base Frame Format.
 */

public class FrameNavigatorViewBean extends ViewBeanBase {

    /**
     * cc components from the corresponding jsp page(s)...
     */

    private static final String TREE  = "NavigationTree";
    private static final String SERVER_MENU = "ServerMenu";
    private static final String MANAGE_HREF = "ManageServerHref";
    private static final String TEXT  = "Text";
    public  static final String SERVER_NAME = "ServerName";

    private static final String URL = "/jsp/util/FrameNavigator.jsp";
    private static final String PAGE_NAME = "FrameNavigator";

    // boolean to keep track if current server is QFS setup
    private boolean isQFSStandAlone = false;

    // Node IDs are defined in util/NavigationNodes.java

    /**
     * Constructor
     *
     * @param name of the page
     * @param page display URL
     * @param name of tab
     */
    public FrameNavigatorViewBean() {
        super(PAGE_NAME);

        // set the address of the JSP page
        setDefaultDisplayURL(URL);
        registerChildren();

        // Get the SAMFS_SERVER_INFO session attribute from the serverInfo
        // object check if this server has a stand alone QFS license
        isQFSStandAlone =
            SamUtil.getSystemType(getServerName()) == SamQFSSystemModel.QFS;

        TraceUtil.trace1(
            new StringBuffer("Frame Navigator: server: ")
                .append(getServerName())
                .append(" isQFSStandAlone: ")
                .append(isQFSStandAlone).toString());
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(TREE, CCClientSideTree.class);
        registerChild(SERVER_MENU, CCDropDownMenu.class);
        registerChild(MANAGE_HREF, CCHref.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(TEXT, CCStaticTextField.class);
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        if (name.equals(TREE)) {
            return new CCClientSideTree(this, name, createTreeModel());
        } else if (name.equals(SERVER_MENU)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            // Set child options and "none selected" label.
            myChild.setOptions(createServerList());
            myChild.setValue(getServerName());
            setSessionServerInfo();
            return (View) myChild;
        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, getServerName());
        } else if (name.equals(MANAGE_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else {
            // Should not come here
            throw new IllegalArgumentException(
                "Invalid child name [" + name + "]");
        }
    }

    /**
     * Create the tree model
     * This method creates how the left hand side navigation tree looks like.
     */
    private CCTreeModel createTreeModel() {
        CCTreeModel model = new CCTreeModel();

        // Create Node HashMap if needed
        NavigationNodes naviNodes = new NavigationNodes(getServerName());
        CCNavNode node  = null, child = null;

        // Add first level leafs to the tree

        // Getting Started should be the first node
        model.addNode(naviNodes.getNavigationNode(
                      NavigationNodes.NODE_GETTING_STARTED));

        // Archive Media (SAM only)
        if (!isQFSStandAlone) {
            node = naviNodes.getNavigationNode(NavigationNodes.NODE_STORAGE);
            model.addNode(node);
        }

        // File Systems & NFS
        // File System & NFS Shares node is always visible for any version/
        // SAM and Q setups
        node = naviNodes.getNavigationNode(NavigationNodes.NODE_FILE_SYSTEM);
        model.addNode(node);

        // Archive Administration (SAM only)
        if (!isQFSStandAlone) {
            node = naviNodes.getNavigationNode(NavigationNodes.NODE_ARCHIVE);
            model.addNode(node);
        }

        node = naviNodes.getNavigationNode(NavigationNodes.NODE_FB_RECOVERY);
        if (isQFSStandAlone) {
            node.setTooltip("node.dataaccess.tooltip.qfs");
            node.setStatus("node.dataaccess.tooltip.qfs");
        }
        model.addNode(node);

        // Monitoring is visible for all setups
        node = naviNodes.getNavigationNode(NavigationNodes.NODE_MONITORING);
        if (isQFSStandAlone) {
            node.setTooltip("node.admin.monitoring.tooltip.qfs");
            node.setStatus("node.admin.monitoring.tooltip.qfs");
            node.setValue(createURL("alarms/CurrentAlarmSummary.jsp"));
        }
        model.addNode(node);

        // Metrics & Reports (SAM only)
        // Call out separate node "System Details" for the QFS only machines
        if (!isQFSStandAlone) {
            node = naviNodes.getNavigationNode(
                        NavigationNodes.NODE_REPORTS);
            model.addNode(node);
        } else {
            model.addNode(naviNodes.getNavigationNode(
                      NavigationNodes.NODE_SERVER_CONFIG));
        }

        /**
         * Getting Started
         * 1. About SAM & QFS
         * 2. First Time Configuration
         * 3. Registration
         */
        node = (CCNavNode) model.getNodeById(
                            NavigationNodes.NODE_GETTING_STARTED);
        if (node != null) {
            node.removeAllChildren();

            node.addChild(
                naviNodes.getNavigationNode(
                        NavigationNodes.NODE_ABOUT_SAMQFS));
            node.addChild(
                naviNodes.getNavigationNode(
                        NavigationNodes.NODE_FIRST_TIME_CONFIG));
            node.addChild(
                naviNodes.getNavigationNode(
                        NavigationNodes.NODE_REGISTRATION));
        }

        /**
         * Archive Media (SAM only)
         * 1. Tape Libraries
         * 2. Tape Volumes
         * 3. Disk Volumes
         * 4. Volume Pools
         */
        if (!isQFSStandAlone) {
            node = (CCNavNode) model.getNodeById(NavigationNodes.NODE_STORAGE);
            node.removeAllChildren();

            node.addChild(
                naviNodes.getNavigationNode(NavigationNodes.NODE_LIBRARY));
            node.addChild(
                naviNodes.getNavigationNode(NavigationNodes.NODE_TAPE_VSN));
            node.addChild(
                naviNodes.getNavigationNode(NavigationNodes.NODE_DISK_VSN));
            node.addChild(
                naviNodes.getNavigationNode(NavigationNodes.NODE_VSN_POOL));
        }

        /**
         * File System & NFS Shares
         * 1. File Systems
         * 2. NFS Shares
         */
        node = (CCNavNode) model.getNodeById(
                                NavigationNodes.NODE_FILE_SYSTEM);
        if (node != null) {
            node.removeAllChildren();

            node.addChild(
                naviNodes.getNavigationNode(NavigationNodes.NODE_FS));

            // NFS
            node.addChild(
                naviNodes.getNavigationNode(NavigationNodes.NODE_NFS));
        }

        /**
         * Archive Administration (SAM only)
         * 1. Policies
         * 2. Storage Recycling
         * 3. Global Parameters
         * 4. Archive Activity
         */
        if (!isQFSStandAlone) {
            node = (CCNavNode) model.getNodeById(NavigationNodes.NODE_ARCHIVE);
            if (node != null) {
                node.removeAllChildren();
                // Policies
                node.addChild(
                    naviNodes.getNavigationNode(NavigationNodes.NODE_POLICY));

                // Recycler
                node.addChild(
                    naviNodes.getNavigationNode(NavigationNodes.NODE_RECYCLER));

                // General Setup
                node.addChild(
                    naviNodes.getNavigationNode(NavigationNodes.NODE_GENERAL));

                // Archive Activities
                node.addChild(
                    naviNodes.getNavigationNode(NavigationNodes.NODE_ACTIVITY));
            }
        }

        /**
         * File Browser & Recovery
         * 1. File Browser
         * 2. Recovery Points (SAM only)
         * 3. Scheduled Tasks (SAM only)
         */
        node = (CCNavNode) model.getNodeById(NavigationNodes.NODE_FB_RECOVERY);
        if (node != null) {
            node.removeAllChildren();

            node.addChild(
                naviNodes.getNavigationNode(NavigationNodes.NODE_FILE_BROWSER));

            if (isQFSStandAlone) {
                node.setLabel("node.dataaccess.qfs");
                node.setTooltip("node.dataaccess.tooltip.qfs");
                node.setStatus("node.dataaccess.tooltip.qfs");
            } else {
                // Recovery Point - For SAM setups only
                node.addChild(naviNodes.getNavigationNode(
                        NavigationNodes.NODE_RECOVERY_POINTS));
                node.addChild(naviNodes.getNavigationNode(
                        NavigationNodes.NODE_SCHEDULED_TASKS));
            }
        }

        /**
         * Monitoring
         * 1. Dashboard (SAM-only)
         * 2. Faults
         * 3. Email Alerts
         * 4. Jobs
         */
        node = (CCNavNode) model.getNodeById(NavigationNodes.NODE_MONITORING);
        if (node != null) {
            node.removeAllChildren();

            if (!isQFSStandAlone) {
                node.addChild(naviNodes.getNavigationNode(
                            NavigationNodes.NODE_DASHBOARD));
            }

            node.addChild(naviNodes.getNavigationNode(
                            NavigationNodes.NODE_FAULT));

            node.addChild(naviNodes.getNavigationNode(
                            NavigationNodes.NODE_NOTIFICATION));

            // Jobs
            node.addChild(naviNodes.getNavigationNode(
                            NavigationNodes.NODE_JOBS));
        }

        /**
         * Metrics & Reports (SAM only)
         * 1. Media Status
         * 2. File Data Distribution
         * 3. File System Utilization
         * 4. System Details
         */
        if (!isQFSStandAlone) {
            node = (CCNavNode) model.getNodeById(
                            NavigationNodes.NODE_REPORTS);
            if (node != null) {
                node.removeAllChildren();

                node.addChild(naviNodes.getNavigationNode(
                        NavigationNodes.NODE_MEDIA_REPORTS));

                node.addChild(naviNodes.getNavigationNode(
                        NavigationNodes.NODE_FS_METRICS));

                node.addChild(naviNodes.getNavigationNode(
                        NavigationNodes.NODE_FS_REPORTS));

                node.addChild(naviNodes.getNavigationNode(
                        NavigationNodes.NODE_SERVER_CONFIG));
            }
        }

        return model;
    }

    private String createURL(String suffix) {
        return new StringBuffer("/samqfsui/")
            .append(suffix)
            .append("?")
            .append(Constants.PageSessionAttributes.SAMFS_SERVER_NAME)
            .append("=")
            .append(getServerName()).toString();
    }

    /**
     * Create OptionList for Change Server Drop Down
     */
    private OptionList createServerList() {
        // Retrieve all the server names from host.conf
        SamQFSSystemModel[] allSystemModel = null;
        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
            allSystemModel = appModel.getAllSamQFSSystemModels();
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Failed to retrieve server Version number", samEx);
            // Eat the exception, do not throw anything up
        }

        if (allSystemModel == null || allSystemModel.length == 0) {
            return new OptionList();
        }

        StringBuffer serverBuf = new StringBuffer();
        for (int i = 0; i < allSystemModel.length; i++) {
            if (serverBuf.length() > 0) {
                serverBuf.append("###");
            }
            serverBuf.append(allSystemModel[i].getHostname());
        }
        String [] serverArray = serverBuf.toString().split("###");
        return new OptionList(serverArray, serverArray);
    }

    private String getServerName() {
        String serverName = (String) getPageSessionAttribute(
                            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        if (serverName == null) {
            serverName = RequestManager.getRequest().getParameter(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
            setPageSessionAttribute(
                Constants.PageSessionAttributes.SAMFS_SERVER_NAME, serverName);
        }

        return serverName;
    }

    private void setSessionServerInfo() {
        try {
            ServerInfo serverInfo = SamUtil.getServerInfo(getServerName());
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Error setting up server info!");
            TraceUtil.trace1(samEx.getMessage());
        }
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        ((CCDropDownMenu) getChild(SERVER_MENU)).setValue(getServerName());
    }
}
