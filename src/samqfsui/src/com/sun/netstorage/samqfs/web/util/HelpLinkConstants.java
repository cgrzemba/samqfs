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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: HelpLinkConstants.java,v 1.25 2008/10/22 19:52:11 ronaldso Exp $

package com.sun.netstorage.samqfs.web.util;

public interface HelpLinkConstants {

    // Server tab pages
    public static final String SERVERS = "samqfs-serversummary.html";
    public static final String SITE_CONFIG = "samqfs-siteinformation.html";
    public static final
        String VERSION_HIGHLIGHT = "samqfs-versionhighlights.html";
    public static final
        String FILE_BROWSER = "samqfs-filebrowser.html";
    public static final
        String RECOVERY_POINTS = "samqfs-recoverypoints.html";

    // Getting Started
    public static final String ABOUT_SAMQFS = "samqfs-aboutsamqfs.html";
    public static final
        String FIRST_TIME_CONFIG = "samqfs-firsttimeconfig.html";

    // FS pages
    public static final String FILESYSTEM_SUMMARY =
        "samqfs-filesystemssummary.html";
    public static final String FILESYSTEM_DETAILS =
        "samqfs-filesystemdetails.html";
    public static final String FILESYSTEM_ARCHIVEPOLICIES =
        "samqfs-filesystemarchivepolicies.html";
    public static final String FILESYSTEM_DEVICES =
        "samqfs-filesystemdevices.html";
    public static final String FILESYSTEM_MOUNTOPTIONS =
        "samqfs-filesystemeditmountoptions.html";
    public static final String NFS_DETAILS =
        "samqfs-filesystemeditnfsproperties.html";
    public static final String FILESYSTEM_ADVANCED_NETWORK_CONFIG =
        "samqfs-advancednetworkconfig.html";
    public static final
        String SHARED_FS_SUMMARY = "samqfs-sharedfssummary.html";
    public static final String
        SHARED_FS_MEMBERS = "samqfs-members.html";

    // Archive pages
    public static final String ARCHIVE_SUMMARY =
        "samqfs-archivepoliciessummary.html";
    public static final String ARCHIVE_DETAILS =
        "samqfs-policydetails.html";
    public static final String TAPE_ARCHIVE_COPYDETAILS =
        "samqfs-policycopytapeoptions.html";
    public static final String DISK_ARCHIVE_COPYDETAILS =
        "samqfs-policycopydiskoptions.html";
    public static final String ARCHIVE_VSNPOOLSUMMARY =
        "samqfs-vsnpoolsummary.html";
    public static final String ARCHIVE_VSNPOOLDETAILS =
        "samqfs-vsnpooldetails.html";
    public static final String ARCHIVE_SETUP =
        "samqfs-generalarchivingsetup.html";
    public static final String CRITERIA_DETAILS =
        "samqfs-policycriteriadetails.html";
    public static final String DISK_VSN_SUMMARY = "samqfs-diskvsns.html";
    public static final String COPY_VSNS = "samqfs-policycopyvsns.html";
    public static final String RECYCLER = "samqfs-recycler.html";

    // Media pages
    public static final String MEDIA_LIBSUMMARY =
        "samqfs-librarysummary.html";
    public static final String MEDIA_DEVICEFAULT =
        "samqfs-faultsummary.html";
    public static final String MEDIA_VSNSUMMARY =
        "samqfs-libraryvsnsummary.html";
    public static final String MEDIA_VSNSEARCH = "samqfs-vsnsearch.html";
    public static final String MEDIA_VSNSEARCHRESULT =
        "samqfs-searchresults.html";
    public static final String MEDIA_VSNDETAILS = "samqfs-vsndetails.html";
    public static final String MEDIA_HISTORIAN = "samqfs-historian.html";
    public static final String MEDIA_LIBRARYDRIVESUMMARY =
        "samqfs-drivedetails.html";
    public static final String MEDIA_IMPORT_VSN = "samqfs-importvsn.html";

    // Job pages
    public static final String JOB_SUMMARY = "samqfs-jobssummary.html";
    public static final String JOB_DETAILS = "samqfs-jobdetails.html";

    // Fault pages
    public static final String FAULT_CURRENT = "samqfs-faultsummary.html";

    // Admin pages
    public static final String ADMIN_NOTIFICATION =
        "samqfs-notificationsummary.html";
    public static final
        String SERVER_CONFIG = "samqfs-serverconfiguration.html";
    public static final
        String ACTIVITY_MANAGEMENT = "samqfs-activitymanagement.html";

    public static final
        String SCHEDULED_TASKS = "samqfs-scheduledtasks.html";
    public static final
        String MONITORING_CONSOLE = "samqfs-monitoringconsole.html";
    public static final
        String MEDIA_STATUS = "samqfs-mediastatus.html";
    public static final
        String FILE_DATA_DISTRIBUTION = "samqfs-filedatadistribution.html";
    public static final
        String FILE_SYSTEM_UTILIZATION = "samqfs-filesystemutilization.html";
    public static final
        String REGISTRATION = "samqfs-registration.html";
}
