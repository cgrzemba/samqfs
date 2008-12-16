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

// ident	$Id: MastheadUtil.java,v 1.51 2008/12/16 00:12:26 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.web.ui.model.CCMastheadModel;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.alarm.AlarmSummary;

public class MastheadUtil {

    /*
     * The requisite default constructor
     */
    public MastheadUtil() {
    }

    /*
     * Masthead Utilities
     */

    public static CCMastheadModel setMastHeadModel(
        CCMastheadModel mastheadModel, String fileName, String serverName)
        throws SamFSException {

        mastheadModel.setSrc(Constants.Masthead.PRODUCT_NAME_SRC);
        mastheadModel.setAlt(Constants.Masthead.PRODUCT_NAME_ALT);
        mastheadModel.setShowDate(true);
        mastheadModel.setCurrentAlarmsLabel("masthead.currentfaults");

        // set the field for help
        SamUtil.doPrint(new StringBuffer("HELP LINK: path is ").append(
            " fileName: ").append(fileName).toString());

        if (fileName != null) {
            mastheadModel.setHelpFileName(setFileName(fileName));
        }

        // add a refresh button here
        mastheadModel.addLink(
            FrameHeaderViewBean.REFRESH_HREF,
            "Masthead.refreshLinkName",
            "Masthead.refreshLinkTooltip",
            "Masthead.refreshLinkStatus");

        int critical = 0, major = 0, minor = 0;
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        AlarmSummary myAlarmSummary =
            sysModel.getSamQFSSystemAlarmManager().getAlarmSummary();
        critical = myAlarmSummary.getCriticalTotal();
        major    = myAlarmSummary.getMajorTotal();
        minor    = myAlarmSummary.getMinorTotal();

        // Set the correct alternative text and application name
        mastheadModel.setAlt(
            SamUtil.getResourceString("masthead.altText"));
        mastheadModel.setVersionProductName(
            SamUtil.getResourceString("masthead.altText"));

        mastheadModel.setCriticalAlarms(critical);
        mastheadModel.setMajorAlarms(major);
        mastheadModel.setMinorAlarms(minor);

        return mastheadModel;
    }

    public static String setFileName(String fileName) {
        if (fileName.equals("FSSummary")) {
            return HelpLinkConstants.FILESYSTEM_SUMMARY;
        } else if (fileName.equals("FSDetails")) {
            return HelpLinkConstants.FILESYSTEM_DETAILS;
        } else if (fileName.equals("FSArchivePolicies")) {
            return HelpLinkConstants.FILESYSTEM_ARCHIVEPOLICIES;
        } else if (fileName.equals("FSDevices")) {
            return HelpLinkConstants.FILESYSTEM_DEVICES;
        } else if (fileName.equals("FSMount")) {
            return HelpLinkConstants.FILESYSTEM_MOUNTOPTIONS;
        } else if (fileName.equals("FileBrowser")) {
            return HelpLinkConstants.FILE_BROWSER;
        } else if (fileName.equals("RecoveryPoints")) {
            return HelpLinkConstants.RECOVERY_POINTS;
        } else if (fileName.equals("NFSDetails")) {
            return HelpLinkConstants.NFS_DETAILS;
        } else if (fileName.equals("AdvancedNetworkConfig")) {
            return HelpLinkConstants.FILESYSTEM_ADVANCED_NETWORK_CONFIG;
        } else if (fileName.equals("PolicySummary")) {
            return HelpLinkConstants.ARCHIVE_SUMMARY;
        } else if (fileName.equals("PolicyDetails")) {
            return HelpLinkConstants.ARCHIVE_DETAILS;
        } else if (fileName.equals("TapeArchivePolCopy")) {
            return HelpLinkConstants.TAPE_ARCHIVE_COPYDETAILS;
        } else if (fileName.equals("DiskArchivePolCopy")) {
            return HelpLinkConstants.DISK_ARCHIVE_COPYDETAILS;
        } else if (fileName.equals("VSNPoolSummary")) {
            return HelpLinkConstants.ARCHIVE_VSNPOOLSUMMARY;
        } else if (fileName.equals("VSNPoolDetails")) {
            return HelpLinkConstants.ARCHIVE_VSNPOOLDETAILS;
        } else if (fileName.equals("ArchiveSetUp")) {
            return HelpLinkConstants.ARCHIVE_SETUP;
        } else if (fileName.equals("CriteriaDetails")) {
            return HelpLinkConstants.CRITERIA_DETAILS;
        } else if (fileName.equals("DiskVSNSummary")) {
            return HelpLinkConstants.DISK_VSN_SUMMARY;
        } else if (fileName.equals("CopyVSNs")) {
            return HelpLinkConstants.COPY_VSNS;
        } else if (fileName.equals("Recycler")) {
            return HelpLinkConstants.RECYCLER;
        } else if (fileName.equals("ArchiveActivity")) {
            return HelpLinkConstants.ACTIVITY_MANAGEMENT;
        } else if (fileName.equals("LibrarySummary")) {
            return HelpLinkConstants.MEDIA_LIBSUMMARY;
        } else if (fileName.equals("LibraryFaultsSummary")) {
            return HelpLinkConstants.MEDIA_DEVICEFAULT;
        } else if (fileName.equals("LibraryDriveSummary")) {
            return HelpLinkConstants.MEDIA_LIBRARYDRIVESUMMARY;
        } else if (fileName.equals("VSNSummary")) {
            return HelpLinkConstants.MEDIA_VSNSUMMARY;
        } else if (fileName.equals("VSNSearch")) {
            return HelpLinkConstants.MEDIA_VSNSEARCH;
        } else if (fileName.equals("VSNSearchResult")) {
            return HelpLinkConstants.MEDIA_VSNSEARCHRESULT;
        } else if (fileName.equals("VSNDetails")) {
            return HelpLinkConstants.MEDIA_VSNDETAILS;
        } else if (fileName.equals("Historian")) {
            return HelpLinkConstants.MEDIA_HISTORIAN;
        } else if (fileName.equals("ImportVSN")) {
            return HelpLinkConstants.MEDIA_IMPORT_VSN;
        } else if (fileName.equals("JobsSummary")) {
            return HelpLinkConstants.JOB_SUMMARY;
        } else if (fileName.equals("JobDetails")) {
            return HelpLinkConstants.JOB_DETAILS;
        } else if (fileName.equals("CurrentAlarmSummary")) {
            return HelpLinkConstants.FAULT_CURRENT;
        } else if (fileName.equals("AdminNotification")) {
            return HelpLinkConstants.ADMIN_NOTIFICATION;
        } else if (fileName.equals("ServerConfiguration")) {
            return HelpLinkConstants.SERVER_CONFIG;
        } else if (fileName.equals("ScheduledTasks")) {
            return HelpLinkConstants.SCHEDULED_TASKS;
        } else if (fileName.equals("SystemMonitoring")) {
            return HelpLinkConstants.MONITORING_CONSOLE;
        } else if (fileName.equals("MediaReport")) {
            return HelpLinkConstants.MEDIA_STATUS;
        } else if (fileName.equals("FileMetricSummary")) {
            return HelpLinkConstants.FILE_DATA_DISTRIBUTION;
        } else if (fileName.equals("FsReport")) {
            return HelpLinkConstants.FILE_SYSTEM_UTILIZATION;
        } else if (fileName.equals("Registration")) {
            return HelpLinkConstants.REGISTRATION;
        } else if (fileName.equals("AboutSAMQFS")) {
            return HelpLinkConstants.ABOUT_SAMQFS;
        } else if (fileName.equals("FirstTimeConfig")) {
            return HelpLinkConstants.FIRST_TIME_CONFIG;
        } else if (fileName.equals("SharedFS")) {
            return HelpLinkConstants.SHARED_FS_SUMMARY;
        } else if (fileName.equals("SharedFSMembers")) {
            return HelpLinkConstants.SHARED_FS_MEMBERS;
        } else {
            return "";
        }
    }
}
