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

// ident	$Id: NavigationNodes.java,v 1.26 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.web.ui.model.CCNavNode;
import java.util.HashMap;


/**
 * This class contains the left side tree navigation node information.
 * The tree is constructed in util/FrameNavigatorViewBean based on the
 * system setup.
 */
public class NavigationNodes  {

    // Node ID
    /**
     * Constant Values for the Application Tabs
     */
    public static final int NODE_FB_RECOVERY = 4;
    public static final int NODE_FILE_SYSTEM = 5;
    public static final int NODE_ARCHIVE = 1;
    public static final int NODE_STORAGE = 2;
    public static final int NODE_MONITORING = 6;
    public static final int NODE_REPORTS = 39;
    public static final int NODE_GETTING_STARTED = 7;

    // Node under Getting Started
    public static final int NODE_ABOUT_SAMQFS = 200;
    public static final int NODE_FIRST_TIME_CONFIG = 201;
    public static final int NODE_REGISTRATION = 112;

    // Node under Archive Media (storage)
    public static final int NODE_LIBRARY = 21;
    public static final int NODE_VSN_POOL = 22;
    public static final int NODE_DISK_VSN = 23;
    public static final int NODE_TAPE_VSN = 25;

    // Node under File Systems & NFS
    public static final int NODE_FS = 0;
    public static final int NODE_NFS = 43;

    // Node under archive
    public static final int NODE_POLICY = 12;
    public static final int NODE_RECYCLER = 13;
    public static final int NODE_GENERAL = 14;
    public static final int NODE_ACTIVITY = 37;

    // Node under File Browser & Recovery
    public static final int NODE_FILE_BROWSER = 41;
    public static final int NODE_RECOVERY_POINTS = 42;
    public static final int NODE_SCHEDULED_TASKS = 111;

    // Node under Monitoring
    public static final int NODE_DASHBOARD = 100;
    public static final int NODE_FAULT = 32;
    public static final int NODE_NOTIFICATION = 38;
    public static final int NODE_JOBS = 113;

    // Node under Metrics & Reports
    public static final int NODE_MEDIA_REPORTS = 101;
    public static final int NODE_FS_METRICS = 102;
    public static final int NODE_FS_REPORTS = 103;
    public static final int NODE_SERVER_CONFIG = 31;

    private HashMap nodeMap;
    private String serverName;

    public NavigationNodes(String serverName) {
        this.serverName = serverName;
        if (nodeMap == null) {
            createNodeMap();
        }
    }

    private void createNodeMap() {
        nodeMap = new HashMap();

        // CCNavNode(id, label, tooltip, status)
        CCNavNode myNode = null;

        // Getting Started
        myNode =
            new CCNavNode(
            NODE_GETTING_STARTED, "node.gettingstarted",
            "node.gettingstarted.tooltip",
            "node.gettingstarted.tooltip");
        myNode.setValue(createURL("faces/jsp/admin/AboutSAMQFS.jsp"));
        myNode.setExpanded(false);
        nodeMap.put(new Integer(NODE_GETTING_STARTED), myNode);

        // About SAM-QFS
        myNode =
            new CCNavNode(
            NODE_ABOUT_SAMQFS, "node.aboutsamqfs",
            "node.aboutsamqfs.tooltip",
            "node.aboutsamqfs.tooltip");
        myNode.setValue(createURL("faces/jsp/admin/AboutSAMQFS.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_ABOUT_SAMQFS), myNode);

        // First Time Configuration
        myNode =
            new CCNavNode(
            NODE_FIRST_TIME_CONFIG, "node.firsttimeconfig",
            "node.firsttimeconfig.tooltipp",
            "node.firsttimeconfig.tooltip");
        myNode.setValue(createURL("admin/FirstTimeConfig.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_FIRST_TIME_CONFIG), myNode);

         // Product Registration CNS
        myNode =
            new CCNavNode(
            NODE_REGISTRATION, "node.admin.registration",
            "node.admin.registration.tooltip",
            "node.admin.registration.tooltip");
        myNode.setValue(createURL("admin/Registration.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_REGISTRATION), myNode);

        /**
         * File Browsing & Recovery
         */
        myNode =
            new CCNavNode(
            NODE_FB_RECOVERY, "node.dataaccess",
            "node.dataaccess.tooltip",
            "node.dataaccess.tooltip");
        myNode.setValue(createURL("fs/FileBrowser.jsp"));
        myNode.setExpanded(false);
        nodeMap.put(new Integer(NODE_FB_RECOVERY), myNode);

        // File Browser (leaf)
        myNode =
            new CCNavNode(
            NODE_FILE_BROWSER, "node.dataaccess.filebrowser",
            "node.dataaccess.filebrowser.tooltip",
            "node.dataaccess.filebrowser.tooltip");
        myNode.setValue(createURL("fs/FileBrowser.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_FILE_BROWSER), myNode);

        // Recovery Points (leaf)
        myNode =
            new CCNavNode(
            NODE_RECOVERY_POINTS, "node.dataaccess.recoverypoints",
            "node.dataaccess.recoverypoints.tooltip",
            "node.dataaccess.recoverypoints.tooltip");
        myNode.setValue(createURL("fs/RecoveryPoints.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_RECOVERY_POINTS), myNode);


        // File System & NFS Shares

        // File System
        myNode =
            new CCNavNode(
            NODE_FILE_SYSTEM, "node.filesystem",
            "node.filesystem.tooltip",
            "node.filesystem.tooltip");
        myNode.setValue(createURL("fs/FSSummary.jsp"));
        nodeMap.put(new Integer(NODE_FILE_SYSTEM), myNode);

       // File Systems
        myNode =
            new CCNavNode(
            NODE_FS, "node.filesystem.fs",
            "node.filesystem.fs.tooltip",
            "node.filesystem.fs.tooltip");
        myNode.setValue(createURL("fs/FSSummary.jsp"));
        myNode.setAcceptsChildren(false);
        myNode.setExpanded(false);
        nodeMap.put(new Integer(NODE_FS), myNode);

        // NFS (leaf)
        myNode =
            new CCNavNode(
            NODE_NFS, "node.dataaccess.nfs",
            "node.dataaccess.nfs.tooltip",
            "node.dataaccess.nfs.tooltip");
        myNode.setValue(createURL("fs/NFSDetails.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_NFS), myNode);

        /**
         * Archive Manangement
         */
        myNode =
            new CCNavNode(
            NODE_ARCHIVE, "node.archive",
            "node.archive.tooltip",
            "node.archive.tooltip");
        myNode.setExpanded(false);
        myNode.setValue(createURL("archive/PolicySummary.jsp"));
        nodeMap.put(new Integer(NODE_ARCHIVE), myNode);

        // Policies
        myNode =
            new CCNavNode(
            NODE_POLICY, "node.archive.policy",
            "node.archive.policy.tooltip",
            "node.archive.policy.tooltip");
        myNode.setValue(createURL("archive/PolicySummary.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_POLICY), myNode);

        // Recycler (Keep for 4.5 servers)
        myNode =
            new CCNavNode(
            NODE_RECYCLER, "node.archive.recycler",
            "node.archive.recycler.tooltip",
            "node.archive.recycler.tooltip");
        myNode.setValue(createURL("archive/Recycler.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_RECYCLER), myNode);

        // General Setup (Keep for 4.5 servers)
        myNode =
            new CCNavNode(
            NODE_GENERAL, "node.archive.general",
            "node.archive.general.tooltip",
            "node.archive.general.tooltip");
        myNode.setValue(createURL("archive/ArchiveSetUp.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_GENERAL), myNode);

        // Archive Activity
        myNode =
            new CCNavNode(
            NODE_ACTIVITY, "node.admin.activity",
            "node.admin.activity.tooltip",
            "node.admin.activity.tooltip");
        myNode.setValue(createURL("archive/ArchiveActivity.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_ACTIVITY), myNode);

        /**
         * Storage Management
         */
        myNode =
            new CCNavNode(
            NODE_STORAGE, "node.storage",
            "node.storage.tooltip", "node.storage.tooltip");
        myNode.setValue(createURL("media/LibrarySummary.jsp"));
        myNode.setExpanded(false);
        nodeMap.put(new Integer(NODE_STORAGE), myNode);


        // Tape VSNs
        myNode =
            new CCNavNode(
            NODE_TAPE_VSN, "node.storage.tapevsn",
            "node.storage.tapevsn.tooltip",
            "node.storage.tapevsn.tooltip");
        myNode.setValue(createURL("media/VSNSummary.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_TAPE_VSN), myNode);

        // Disk VSNs
        myNode =
            new CCNavNode(
            NODE_DISK_VSN, "node.storage.diskvsn",
            "node.storage.diskvsn.tooltip",
            "node.storage.diskvsn.tooltip");
        myNode.setValue(createURL("archive/DiskVSNSummary.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_DISK_VSN), myNode);

        // VSN Pools
        myNode =
            new CCNavNode(
            NODE_VSN_POOL, "node.storage.vsnpool",
            "node.storage.vsnpool.tooltip",
            "node.storage.vsnpool.tooltip");
        myNode.setValue(createURL("archive/VSNPoolSummary.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_VSN_POOL), myNode);

        // Tape Libraries
        myNode =
            new CCNavNode(
            NODE_LIBRARY, "node.storage.library",
            "node.storage.library.tooltip",
            "node.storage.library.tooltip");
        myNode.setValue(createURL("media/LibrarySummary.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_LIBRARY), myNode);

        // Server Info
        myNode =
            new CCNavNode(
            NODE_SERVER_CONFIG, "node.admin.serverinfo",
            "node.admin.serverinfo.tooltip",
            "node.admin.serverinfo.tooltip");
        myNode.setValue(createURL("admin/ServerConfiguration.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_SERVER_CONFIG), myNode);

        // Fault
        myNode =
            new CCNavNode(
            NODE_FAULT, "node.admin.fault",
            "node.admin.fault.tooltip",
            "node.admin.fault.tooltip");
        myNode.setValue(createURL("alarms/CurrentAlarmSummary.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_FAULT), myNode);

        // Jobs
        // jobs summary page
        myNode = new CCNavNode(NODE_JOBS,
                              "node.admin.job",
                              "node.admin.job.tooltip",
                              "node.admin.job.tooltip");
        myNode.setValue(createURL("faces/jsp/jobs/Jobs.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_JOBS), myNode);

        // scheduled tasks
        myNode = new CCNavNode(NODE_SCHEDULED_TASKS,
                               "node.admin.scheduledtasks",
                               "node.admin.scheduledtasks.tooltip",
                               "node.admin.scheduledtasks.tooltip");
        myNode.setValue(createURL("admin/ScheduledTasks.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_SCHEDULED_TASKS), myNode);

        // Monitoring
        myNode =
            new CCNavNode(
            NODE_MONITORING, "node.admin.monitoring",
            "node.admin.monitoring.tooltip",
            "node.admin.monitoring.tooltip");
        myNode.setValue(createURL("monitoring/SystemMonitoring.jsp"));
        nodeMap.put(new Integer(NODE_MONITORING), myNode);

        // Dashboard
        myNode =
            new CCNavNode(
            NODE_DASHBOARD, "node.dashboard",
            "node.dashboard.tooltip",
            "node.dashboard.tooltip");
        myNode.setValue(createURL("monitoring/SystemMonitoring.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_DASHBOARD), myNode);

        // Notification
        myNode =
            new CCNavNode(
            NODE_NOTIFICATION, "node.admin.notification",
            "node.admin.notification.tooltip",
            "node.admin.notification.tooltip");
        myNode.setValue(createURL("admin/AdminNotification.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_NOTIFICATION), myNode);


        // Metrics & Reports
        myNode =
            new CCNavNode(
            NODE_REPORTS, "node.admin.metrics",
            "node.admin.metrics.tooltip",
            "node.admin.metrics.tooltip");
        myNode.setValue(createURL("admin/MediaReport.jsp"));
        nodeMap.put(new Integer(NODE_REPORTS), myNode);

        // Media Report
        myNode =
            new CCNavNode(
            NODE_MEDIA_REPORTS, "node.admin.metrics.mediastatus",
            "node.admin.metrics.mediastatus.tooltip",
            "node.admin.metrics.mediastatus.tooltip");
        myNode.setValue(createURL("admin/MediaReport.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_MEDIA_REPORTS), myNode);

        // FS Report
        myNode =
            new CCNavNode(
            NODE_FS_REPORTS, "node.admin.metrics.fsutil",
            "node.admin.metrics.fsutil.tooltip",
            "node.admin.metrics.fsutil.tooltip");
        myNode.setValue(createURL("admin/FsReport.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_FS_REPORTS), myNode);

        // FS Metric
        myNode =
            new CCNavNode(
            NODE_FS_METRICS, "node.admin.metrics.filedistribution",
            "node.admin.metrics.filedistribution.tooltip",
            "node.admin.metrics.filedistribution.tooltip");
        myNode.setValue(createURL("admin/FileMetricSummary.jsp"));
        myNode.setAcceptsChildren(false);
        nodeMap.put(new Integer(NODE_FS_METRICS), myNode);
    }

    private String createURL(String suffix) {
        return new StringBuffer("/samqfsui/")
            .append(suffix)
            .append("?")
            .append(Constants.PageSessionAttributes.SAMFS_SERVER_NAME)
            .append("=")
            .append(serverName).toString();
    }

    public CCNavNode getNavigationNode(int nodeID) {
        return (CCNavNode) nodeMap.get(new Integer(nodeID));
    }
}
