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

// ident	$Id: ServerConfigurationViewBean.java,v 1.30 2008/03/17 14:40:41 am143972 Exp $

package com.sun.netstorage.samqfs.web.admin;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.media.MediaUtil;
import com.sun.netstorage.samqfs.web.model.ClusterNodeInfo;
import com.sun.netstorage.samqfs.web.model.ConfigStatus;
import com.sun.netstorage.samqfs.web.model.DaemonInfo;
import com.sun.netstorage.samqfs.web.model.LogAndTraceInfo;
import com.sun.netstorage.samqfs.web.model.PkgInfo;
import com.sun.netstorage.samqfs.web.model.SamExplorerOutputs;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.SystemInfo;
import com.sun.netstorage.samqfs.web.model.fs.StageFile;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.server.ServerUtil;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCActionTableModelInterface;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCHiddenField;

/**
 *  This class is the view bean for the Host Configuration page
 */

public class ServerConfigurationViewBean extends CommonViewBeanBase {

    // Page information...
    private static final String PAGE_NAME = "ServerConfiguration";
    private static final
        String DEFAULT_DISPLAY_URL = "/jsp/admin/ServerConfiguration.jsp";

    public static final String SERVER_NAME = "ServerName";

    // Page Title, Property Sheet, and Action Table component.
    private CCPageTitleModel pageTitleModel = null;
    private CCPropertySheetModel propertySheetModel = null;
    private CCActionTableModel logTraceTable = null;
    private CCActionTableModel packagesTable = null;
    private CCActionTableModel configTable   = null;
    private CCActionTableModel daemonsTable  = null;
    private CCActionTableModel clusterTable  = null;
    private CCActionTableModel explorerTable = null;
    private CCActionTableModel reportsTable  = null;

    private static final String COMPRESSED_EXTENSION = ".tar.gz";

    /**
     * Constructor
     */
    public ServerConfigurationViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        pageTitleModel = createPageTitleModel();
        propertySheetModel = createPropertySheetModel();
        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        super.registerChildren();
        registerChild(SERVER_NAME, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        PropertySheetUtil.registerChildren(this, propertySheetModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     *
     * @param name The name of the child view
     * @return View The instantiated child view
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new StringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (super.isChildSupported(name)) {
            child = super.createChild(name);
        } else if (name.equals(SERVER_NAME)) {
            child = new CCHiddenField(this, name, getServerName());
        // PropertySheet Child
        } else if (PropertySheetUtil.isChildSupported(
            propertySheetModel, name)) {
            child = PropertySheetUtil.createChild(
                this, propertySheetModel, name);
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

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");

        loadPropertySheetModel(propertySheetModel);
        loadTableModels();
        loadMediaString();
        TraceUtil.trace3("Exiting");
    }

    private CCPropertySheetModel createPropertySheetModel() {
        TraceUtil.trace3("Entering");

        if (propertySheetModel == null)  {
            // Create Property Sheet Model
            propertySheetModel = PropertySheetUtil.createModel(
                "/jsp/admin/ServerConfigurationPropertySheet.xml");

            // Create all table models in the property sheet
            clusterTable = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/ClusterTable.xml");
            logTraceTable = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/LogTraceTable.xml");
            reportsTable = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/ReportsTable.xml");
            packagesTable = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/PackagesTable.xml");
            daemonsTable = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/DaemonsTable.xml");
            configTable = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/ConfigTable.xml");
            explorerTable = new CCActionTableModel(
                RequestManager.getRequestContext().getServletContext(),
                "/jsp/admin/ExplorerTable.xml");

            propertySheetModel.setModel("packagesTable", packagesTable);
            propertySheetModel.setModel("daemonsTable",  daemonsTable);
            propertySheetModel.setModel("configTable",   configTable);
            propertySheetModel.setModel("reportsTable", reportsTable);
            propertySheetModel.setModel("clusterTable", clusterTable);
            propertySheetModel.setModel("logTraceTable", logTraceTable);
            propertySheetModel.setModel("explorerTable", explorerTable);
        }
        TraceUtil.trace3("Exiting");
        return propertySheetModel;
    }

    /**
     * Load the actiontable model
     */
    private void loadTableModels() {
        TraceUtil.trace3("Entering");

        loadReportsTable(reportsTable);
        loadClusterTable(clusterTable);
        loadPackagesTable(packagesTable);
        loadDaemonsTable(daemonsTable);
        loadConfigTable(configTable);
        loadLogAndTraceTable(logTraceTable);
        loadExplorerTable(explorerTable);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Populate the cluster table if this server is a part of cluster.
     * If not, set this section invisible
     */
    private void loadReportsTable(CCActionTableModel actionTableModel) {
        TraceUtil.trace3("Entering");

        if (actionTableModel == null) {
            return;
        }

        actionTableModel.clear();
        actionTableModel.setActionValue(
            "ReportsPathColumn",
            "ServerConfiguration.reports.name");
        actionTableModel.setActionValue(
            "ReportsSizeColumn",
            "ServerConfiguration.reports.size");
        actionTableModel.setActionValue(
            "ReportsTimeColumn",
            "ServerConfiguration.reports.date");
        actionTableModel.setPrimarySortName("ReportsHiddenText");
        actionTableModel.setPrimarySortOrder(
            CCActionTableModelInterface.DESCENDING);

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            StageFile [] allReports = sysModel.getCISReportEntries();

            if (allReports.length == 0)  {
                // No reports found, or report directory does not exist
                propertySheetModel.setVisible("reports", false);
            } else {
                propertySheetModel.setVisible("reports", true);

                for (int i = 0; i < allReports.length; i++) {
                    if (i > 0) {
                        actionTableModel.appendRow();
                    }

                    actionTableModel.setValue(
                        "ReportsPathText", allReports[i].getName());
                    actionTableModel.setValue(
                        "ReportsPathHref",
                        SamQFSSystemModel.CIS_REPORT_DIR.
                            concat("/").concat(allReports[i].getName()));

                    actionTableModel.setValue(
                        "ReportsSizeText",
                        new Capacity(allReports[i].length(),
                                     SamQFSSystemModel.SIZE_B));
                    actionTableModel.setValue(
                        "ReportsTimeText",
                        SamUtil.getTimeString(allReports[i].getCreatedTime()));
                    actionTableModel.setValue(
                        "ReportsHiddenText",
                        new Long(allReports[i].getCreatedTime()));
                }
            }
        } catch (Exception ex) {
            TraceUtil.trace1("Failed to retrieve custom reports information.");
            TraceUtil.trace1("Cause: " + ex.getMessage());
            SamUtil.processException(ex, this.getClass(),
                "loadReportsTable()",
                "Failed to populate custom reports information",
                getServerName());
            actionTableModel.setEmpty(
                "ServerConfiguration.reports.failed");
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Populate the cluster table if this server is a part of cluster.
     * If not, set this section invisible
     */
    private void loadClusterTable(CCActionTableModel actionTableModel) {
        TraceUtil.trace3("Entering");

        if (actionTableModel == null) {
            return;
        }

        actionTableModel.clear();
        actionTableModel.setActionValue(
            "NodeNameColumn", "ServerConfiguration.cluster.name");
        actionTableModel.setActionValue(
            "NodeIDColumn", "ServerConfiguration.cluster.id");
        actionTableModel.setActionValue(
            "NodeStatusColumn", "ServerConfiguration.cluster.status");
        actionTableModel.setActionValue(
            "NodePrivateIPColumn", "ServerConfiguration.cluster.privateip");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            // Cluster is supported since 4.5
            if (!sysModel.isClusterNode())  {
                // server is not a cluster node
                propertySheetModel.setVisible("cluster", false);
            } else {
                propertySheetModel.setVisible("cluster", true);
                ClusterNodeInfo [] nodeInfos = sysModel.getClusterNodes();

                for (int i = 0; i < nodeInfos.length; i++) {
                    if (i > 0) {
                        actionTableModel.appendRow();
                    }

                    actionTableModel.setValue(
                        "NodeNameText", nodeInfos[i].getName());
                    actionTableModel.setValue(
                        "NodeIDText", new Integer(nodeInfos[i].getID()));
                    actionTableModel.setValue(
                        "NodeStatusText", nodeInfos[i].getStatus());
                    actionTableModel.setValue(
                        "NodePrivateIPText", nodeInfos[i].getPrivIP());
                }
            }
        } catch (Exception ex) {
            TraceUtil.trace1("Failed to retrieve cluster node information.");
            TraceUtil.trace1("Cause: " + ex.getMessage());
            SamUtil.processException(ex, this.getClass(),
                "loadClusterTable()",
                "Failed to populate cluster node information",
                getServerName());
            actionTableModel.setEmpty("ServerConfiguration.cluster.failed");
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Load the Log and Trace Table
     */
    private void loadLogAndTraceTable(CCActionTableModel actionTableModel) {
        TraceUtil.trace3("Entering");

        if (actionTableModel == null) {
            return;
        }

        actionTableModel.clear();
        actionTableModel.setActionValue(
            "LogTraceNameColumn", "ServerConfiguration.logtrace.name");
        actionTableModel.setActionValue(
            "LogTraceTypeColumn", "ServerConfiguration.logtrace.type");
        actionTableModel.setActionValue(
            "LogTraceStatusColumn", "ServerConfiguration.logtrace.status");
        actionTableModel.setActionValue(
            "LogTracePathColumn", "ServerConfiguration.logtrace.path");
        actionTableModel.setActionValue(
            "LogTraceFlagsColumn", "ServerConfiguration.logtrace.flags");
        actionTableModel.setActionValue(
            "LogTraceSizeColumn", "ServerConfiguration.logtrace.size");
        actionTableModel.setActionValue(
            "LogTraceModTimeColumn", "ServerConfiguration.logtrace.modTime");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            LogAndTraceInfo [] myLogAndTraceArray =
                sysModel.getLogAndTraceInfo();

            if (myLogAndTraceArray == null) {
                return;
            }

            for (int i = 0; i < myLogAndTraceArray.length; i++) {
                if (i != 0) {
                    actionTableModel.appendRow();
                }
                actionTableModel.setValue(
                    "LogTraceNameText", myLogAndTraceArray[i].getName());
                actionTableModel.setValue(
                    "LogTraceTypeText", myLogAndTraceArray[i].getType());

                boolean isLogTraceOn = myLogAndTraceArray[i].isOn();

                if (isLogTraceOn) {
                    actionTableModel.setValue(
                        "LogTraceStatusText",
                        SamUtil.getResourceString(
                            "ServerConfiguration.logtrace.on"));
                    if (myLogAndTraceArray[i].getPath() != null &&
                        myLogAndTraceArray[i].getPath().indexOf('/') == 0) {
                        actionTableModel.setValue(
                            "LogTracePathText",
                            myLogAndTraceArray[i].getPath());
                        actionTableModel.setValue(
                            "LogTracePathTextWithoutLink", "");
                    } else {
                        actionTableModel.setValue(
                            "LogTracePathText",
                            "");
                        actionTableModel.setValue(
                            "LogTracePathTextWithoutLink",
                            myLogAndTraceArray[i].getPath());
                    }

                    actionTableModel.setValue(
                        "PathHref", myLogAndTraceArray[i].getPath());
                    actionTableModel.setValue(
                        "LogTraceFlagText", myLogAndTraceArray[i].getFlags());
                    actionTableModel.setValue(
                        "LogTraceSizeText",
                         Capacity.newCapacity(myLogAndTraceArray[i].getSize(),
                             SamQFSSystemModel.SIZE_B).toString());
                    actionTableModel.setValue(
                        "LogTraceModTimeText",
                        SamUtil.getTimeString(
                            myLogAndTraceArray[i].getModtime()));
                } else {
                    actionTableModel.setValue(
                        "LogTraceStatusText",
                        SamUtil.getResourceString(
                            "ServerConfiguration.logtrace.off"));
                    actionTableModel.setValue(
                        "LogTracePathText", "");
                    actionTableModel.setValue(
                        "PathHref", "");
                    actionTableModel.setValue(
                        "LogTraceFlagText", "");

                    actionTableModel.setValue(
                        "LogTraceSizeText", "");

                    actionTableModel.setValue(
                        "LogTraceModTimeText", "");
                }
            }
        } catch (Exception ex) {
            TraceUtil.trace1("Failed to retrieve Log/Trace Information.");
            TraceUtil.trace1("Cause: " + ex.getMessage());
            SamUtil.processException(ex, this.getClass(),
                "loadLogAndTraceTable()",
                "Failed to populate log and trace files information",
                getServerName());
            actionTableModel.setEmpty("ServerConfiguration.logtrace.failed");
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Load the Packages Table
     */
    private void loadPackagesTable(CCActionTableModel actionTableModel) {
        TraceUtil.trace3("Entering");

        if (actionTableModel == null) {
            return;
        }
        actionTableModel.clear();
        actionTableModel.setActionValue(
            "PackageNameColumn", "ServerConfiguration.packages.name");
        actionTableModel.setActionValue(
            "PackageDescColumn", "ServerConfiguration.packages.desc");
        actionTableModel.setActionValue(
            "PackageVersionColumn", "ServerConfiguration.packages.version");
        actionTableModel.setActionValue(
            "PackageStatusColumn", "ServerConfiguration.packages.status");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            PkgInfo [] myPkgInfoArray = sysModel.getPkgInfo();

            if (myPkgInfoArray == null) {
                return;
            }

            for (int i = 0; i < myPkgInfoArray.length; i++) {
                if (i != 0) {
                    actionTableModel.appendRow();
                }
                actionTableModel.setValue(
                    "PackageNameText", myPkgInfoArray[i].getPKGINST());
                actionTableModel.setValue(
                    "PackageDescText", myPkgInfoArray[i].getNAME());
                actionTableModel.setValue(
                    "PackageVersionText", myPkgInfoArray[i].getVERSION());
                actionTableModel.setValue(
                    "PackageStatusText",
                    getStatusString(myPkgInfoArray[i].getSTATUS()));
            }
        } catch (Exception ex) {
            TraceUtil.trace1("Failed to retrieve packages Information.");
            TraceUtil.trace1("Cause: " + ex.getMessage());
            SamUtil.processException(ex, this.getClass(),
                "loadPackagesTable()",
                "Failed to populate packages information",
                getServerName());
            actionTableModel.setEmpty("ServerConfiguration.packages.failed");
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Load the Running Daemons Table
     */
    private void loadDaemonsTable(CCActionTableModel actionTableModel) {
        TraceUtil.trace3("Entering");

        if (actionTableModel == null) {
            return;
        }
        actionTableModel.clear();
        actionTableModel.setActionValue(
            "DaemonDescColumn", "ServerConfiguration.daemons.desc");
        actionTableModel.setActionValue(
            "DaemonNameColumn", "ServerConfiguration.daemons.name");
        actionTableModel.setActionValue(
            "BlankColumn", "                                                 ");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            DaemonInfo [] myInfoArray = sysModel.getDaemonInfo();

            if (myInfoArray == null) {
                return;
            }

            for (int i = 0; i < myInfoArray.length; i++) {
                if (i != 0) {
                    actionTableModel.appendRow();
                }
                actionTableModel.setValue(
                    "BlankText",
                    "                                                        ");
                actionTableModel.setValue(
                    "DaemonDescText", myInfoArray[i].getDescr());
                actionTableModel.setValue(
                    "DaemonNameText", myInfoArray[i].getName());
            }
        } catch (Exception ex) {
            TraceUtil.trace1("Failed to retrieve running daemons information.");
            TraceUtil.trace1("Cause: " + ex.getMessage());
            SamUtil.processException(ex, this.getClass(),
                "loadDaemonsTable()",
                "Failed to populate daemons information",
                getServerName());
            actionTableModel.setEmpty("ServerConfiguration.daemons.failed");
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Load the Configuration Files Table
     */
    private void loadConfigTable(CCActionTableModel actionTableModel) {
        TraceUtil.trace3("Entering");

        if (actionTableModel == null) {
            return;
        }
        actionTableModel.clear();
        actionTableModel.setActionValue(
            "ConfigNameColumn", "ServerConfiguration.config.name");
        actionTableModel.setActionValue(
            "ConfigStatusColumn", "ServerConfiguration.config.status");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            ConfigStatus [] myInfoArray = sysModel.getConfigStatus();

            if (myInfoArray == null) {
                return;
            }

            for (int i = 0; i < myInfoArray.length; i++) {
                if (i != 0) {
                    actionTableModel.appendRow();
                }
                actionTableModel.setValue(
                    "BlankText",
                    "                                                        ");
                actionTableModel.setValue(
                    "ConfigNameText", myInfoArray[i].getConfig());
                actionTableModel.setValue(
                    "FileHref", myInfoArray[i].getConfig());

                String status = myInfoArray[i].getStatus();
                status = status == null ? "" : status.trim();

                String content = myInfoArray[i].getMsg();
                if (content != null && content.length() != 0) {
                    actionTableModel.setValue(
                        "ConfigStatusTextWithLink", myInfoArray[i].getStatus());
                    actionTableModel.setValue(
                        "ConfigStatusText", "");
                    actionTableModel.setValue(
                        "StatusHref", myInfoArray[i].getConfig());
                } else {
                    actionTableModel.setValue(
                        "ConfigStatusText", myInfoArray[i].getStatus());
                    actionTableModel.setValue(
                        "ConfigStatusTextWithLink", "");
                    actionTableModel.setValue(
                        "StatusHref", "");
                }
            }
        } catch (Exception ex) {
            TraceUtil.trace1(
                "Failed to retrieve configuration files information.");
            TraceUtil.trace1("Cause: " + ex.getMessage());
            SamUtil.processException(ex, this.getClass(),
                "loadConfigTable()",
                "Failed to populate configuration files information",
                getServerName());
            actionTableModel.setEmpty("ServerConfiguration.config.failed");
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Populate the SAM Explorer table if server is Version 4.5+
     */
    private void loadExplorerTable(CCActionTableModel actionTableModel) {
        TraceUtil.trace3("Entering");

        if (actionTableModel == null) {
            return;
        }

        // Disable Generate Report button if user does not have CONFIG or
        // SAM CONTROL authorization
        ((CCButton) getChild("GenerateButton")).setDisabled(
             !SecurityManagerFactory.getSecurityManager().
                 hasAuthorization(Authorization.SAM_CONTROL));

        actionTableModel.clear();
        actionTableModel.setActionValue(
            "ReportPathColumn", "ServerConfiguration.explorer.path");
        actionTableModel.setActionValue(
            "ReportNameColumn", "ServerConfiguration.explorer.name");
        actionTableModel.setActionValue(
            "ReportSizeColumn", "ServerConfiguration.explorer.size");
        actionTableModel.setActionValue(
            "ReportCreateTimeColumn",
            "ServerConfiguration.explorer.createtime");
        actionTableModel.setActionValue(
            "ReportModTimeColumn", "ServerConfiguration.explorer.modtime");

        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

            // SamExplorer is supported since 4.5
            propertySheetModel.setVisible("explorer", true);
            SamExplorerOutputs [] outputs =
                sysModel.getSamExplorerOutputs();

            for (int i = 0; i < outputs.length; i++) {
                if (i > 0) {
                    actionTableModel.appendRow();
                }

                actionTableModel.setValue(
                    "ReportPathText", outputs[i].getPath());

                // Don't show HREF if file is compressed.
                // See CR 6467931
                String name = outputs[i].getName();
                if (name.endsWith(COMPRESSED_EXTENSION)) {
                    actionTableModel.setValue(
                        "ReportNameText", name);
                    actionTableModel.setValue(
                        "ReportNameTextWithLink", "");
                } else {
                    actionTableModel.setValue(
                        "ReportNameText", "");
                    actionTableModel.setValue(
                        "ReportNameTextWithLink", name);
                }

                actionTableModel.setValue(
                    "ReportPathHref",
                    new StringBuffer(
                        outputs[i].getPath()).append("/").append(
                        outputs[i].getName()).toString());
                actionTableModel.setValue(
                    "ReportSizeText",
                    Capacity.newCapacity(
                        outputs[i].getSize(),
                        SamQFSSystemModel.SIZE_B).toString());
                actionTableModel.setValue(
                    "ReportCreateTimeText",
                    SamUtil.getTimeString(outputs[i].getCreatedTime()));
                actionTableModel.setValue(
                    "ReportModTimeText",
                    SamUtil.getTimeString(outputs[i].getModifiedTime()));
            }
        } catch (Exception ex) {
            TraceUtil.trace1("Failed to retrieve SAM Explorer information.");
            TraceUtil.trace1("Cause: " + ex.getMessage());
            SamUtil.processException(ex, this.getClass(),
                "loadExplorerTable()",
                "Failed to populate SAM explorer information",
                getServerName());
            actionTableModel.setEmpty(
                    "ServerConfiguration.explorer.failed");
        }

        TraceUtil.trace3("Exiting");
    }

    private void loadPropertySheetModel(
        CCPropertySheetModel propertySheetModel) {
        TraceUtil.trace3("Entering");

        propertySheetModel.clear();

        // Fill it the property sheet values
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            SystemInfo mySystemInfo = sysModel.getSystemInfo();

            propertySheetModel.setValue(
                "NameValue", mySystemInfo.getHostname());
            propertySheetModel.setValue(
                "IDValue", mySystemInfo.getHostid());
            propertySheetModel.setValue(
                "OSValue",
                ServerUtil.createOSString(
                    mySystemInfo.getOSname(),
                    mySystemInfo.getRelease(),
                    mySystemInfo.getVersion()));
            propertySheetModel.setValue(
                "MachineValue", mySystemInfo.getMachine());
            propertySheetModel.setValue(
                "CPUValue", Integer.toString(mySystemInfo.getCPUs()));
            propertySheetModel.setValue(
                "MemoryValue",
                new Capacity(
                    mySystemInfo.getMemoryMB(),
                    SamQFSSystemModel.SIZE_MB).toString());

            propertySheetModel.setValue(
                "ArchValue",
                SamUtil.swapArchString(mySystemInfo.getArchitecture()));
            propertySheetModel.setValue(
                "IPAddressValue",
                createIPAddressString(mySystemInfo.getIPAddresses()));

        } catch (SamFSException samEx) {
            resetAllFieldsToBlank();

            TraceUtil.trace1(
                "Failed to retrieve server basic information.");
            TraceUtil.trace1("Cause: " + samEx.getMessage());

            SamUtil.processException(
                samEx,
                this.getClass(),
                "loadPropertySheet",
                "Failed to retrieve system information",
                getServerName());
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CHILD_COMMON_ALERT,
                "ServerConfiguration.error",
                samEx.getSAMerrno(),
                samEx.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
    }

    /**
     * To populate the media section with media devices entries
     */
    private void loadMediaString() {
        TraceUtil.trace3("Entering");
        StringBuffer allMediaStringBuf = new StringBuffer();

        // Grab all libraries objects, and pass to helper method to
        // generate the string as designed
        // If exception occurred, set error message in the MediaText value
        try {
            SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());
            Library allLibraries[] =
                sysModel.getSamQFSSystemMediaManager().getAllLibraries();

            // If there's no library, use MediaText defaultValue
            if (allLibraries == null || allLibraries.length == 0) {
                // If no libraries are found, do not show Media Section
                propertySheetModel.setVisible("media", false);
                return;
            } else {
                propertySheetModel.setVisible("media", true);
            }

            for (int i = 0; i < allLibraries.length; i++) {
                if (allMediaStringBuf.length() != 0) {
                    allMediaStringBuf.append("<br /><br />");
                }

                // Skip Historian entries
                if (!allLibraries[i].getName().equalsIgnoreCase(
                    Constants.MediaAttributes.HISTORIAN_NAME)) {
                    allMediaStringBuf.append(
                        MediaUtil.createLibraryEntry(allLibraries[i]));
                }
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to retrieve media information.");
            TraceUtil.trace1("Cause: " + samEx.getMessage());
            SamUtil.processException(
                samEx,
                this.getClass(),
                "loadMediaString",
                "Failed to retrieve media device information",
                getServerName());
            propertySheetModel.setValue("MediaText",
                SamUtil.getResourceString("ServerConfiguration.media.failed"));
            return;
        }

        // If there's nothing in buffer, use default String
        if (allMediaStringBuf.length() != 0) {
            propertySheetModel.setValue(
                "MediaText", allMediaStringBuf.toString());
        }

        TraceUtil.trace3("Exiting");
    }

    private String createIPAddressString(String origIPAddressString) {
        StringBuffer buf = new StringBuffer();
        String [] ipArray = origIPAddressString.split(" ");
        for (int i = 0; i < ipArray.length; i++) {
            if (i > 0) {
                buf.append("<br />");
            }
            buf.append(ipArray[i]);
        }
        return buf.toString();
    }

    private String getStatusString(String statusNumberString) {
        try {
            int status = Integer.parseInt(statusNumberString);

            if (status >= 0 && status <= 4) {
                return SamUtil.getResourceString(
                    new StringBuffer("ServerConfiguration.packages.").
                        append(status).toString());
            }
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1(
                "NumberFormatException while parsing status of packages!");
        }
        return SamUtil.getResourceString("ServerConfiguration.packages.error");
    }

    private void resetAllFieldsToBlank() {
        propertySheetModel.setValue("NameValue", "");
        propertySheetModel.setValue("IDValue", "");
        propertySheetModel.setValue("OSValue", "");
        propertySheetModel.setValue("MachineValue", "");
        propertySheetModel.setValue("CPUValue", "");
        propertySheetModel.setValue("MemoryValue", "");
        propertySheetModel.setValue("ArchValue", "");
        propertySheetModel.setValue("IPAddressValue", "");
    }
}
