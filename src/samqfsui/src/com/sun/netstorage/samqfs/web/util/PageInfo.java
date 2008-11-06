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

// ident	$Id: PageInfo.java,v 1.37 2008/11/06 00:47:09 ronaldso Exp $

package com.sun.netstorage.samqfs.web.util;

import java.util.HashMap;

/**
 * This class contains all of the breadcrumb information for all the pages
 * within the application.
 * This class utilizes two data structures. The first is an array that holds
 * references to the PagePaths which, in turn, holds the breadcrumb
 * display information for a page.  The second is a hash table which
 * allows callers to get the number associated with a particular page
 * by supplying the PAGE_NAME.
 */

public class PageInfo {
    private static PageInfo pageInfo = null;
    private PagePath[] pagePaths;
    private HashMap pageNames;

    public static PageInfo getPageInfo() {
        if (pageInfo == null) {
            pageInfo = new PageInfo();
        }
        return pageInfo;
    }

    private PageInfo() {

        pagePaths = new PagePath[38];

        pagePaths[0] = new PagePath(
            "FileSystemDetailsHref",
            "FSDetails.pageTitle",
            "FSDetails.breadcrumbmouseover");

        pagePaths[1] = new PagePath(
            "FSArchivePolicyHref",
            "FSArchivePolicies.pageTitle",
            "FSArchivePolicies.breadcrumbmouseover");

        pagePaths[2] = new PagePath(
            "FileSystemSummaryHref",
            "FSSummary.title",
            "FSSummary.breadcrumbmouseover");

        pagePaths[3] = new PagePath(
            "VSNPoolSummaryHref",
            "VSNPoolSummary.pageTitle",
            "VSNPoolSummary.breadcrumbmouseover");

        pagePaths[4] = new PagePath(
            "VSNPoolDetailsHref",
            "VSNPoolDetails.pageTitle",
            "VSNPoolDetails.breadcrumbmouseover");

        pagePaths[5] = new PagePath(
            "CurrentJobsHref",
            "CurrentJobs.title",
            "CurrentJobs.mouseover");

        pagePaths[6] = new PagePath(
            "PendingJobsHref",
            "PendingJobs.title",
            "PendingJobs.mouseover");

        pagePaths[7] = new PagePath(
            "AllJobsHref",
            "AllJobs.title",
            "AllJobs.mouseover");

        pagePaths[8] = new PagePath(
            "CurrentAlarmSummaryHref",
            "CurrentAlarmSummary.title",
            "CurrentAlarmSummary.mouseover");

        pagePaths[9] = new PagePath(
            "LibrarySummaryHref",
            "LibrarySummary.pageTitle",
            "LibrarySummary.breadcrumbmouseover");

        pagePaths[10] = new PagePath(
            "HistorianHref",
            "Historian.pageTitle",
            "Historian.breadcrumbmouseover");

        pagePaths[11] = new PagePath(
            "LibraryDriveSummaryHref",
            "LibraryDriveSummary.pageTitle",
            "LibraryDriveSummary.breadcrumbmouseover");

        pagePaths[12] = new PagePath(
            "VSNSummaryHref",
            "VSNSummary.pageTitle",
            "VSNSummary.breadcrumbmouseover");

        pagePaths[13] = new PagePath(
            "VSNDetailsHref",
            "VSNDetails.pageTitle",
            "VSNDetails.breadcrumbmouseover");

        pagePaths[14] = new PagePath(
            "VSNSearchHref",
            "VSNSearch.pageTitle",
            "VSNSearch.breadcrumbmouseover");

        pagePaths[15] = new PagePath(
            "VSNSearchResultHref",
            "VSNSearchResult.pageTitle",
            "VSNSearchResult.breadcrumbmouseover");

        pagePaths[16] = new PagePath(
            "LibraryFaultsSummaryHref",
            "LibraryFaultsSummary.browserpagetitle",
            "LibraryFaultsSummary.breadcrumbmouseover");

        pagePaths[17] = new PagePath(
            "PolicyTapeCopyHref",
            "PolicyCopy.title",
            "ArchivePolCopy.breadcrumbmouseover");

        pagePaths[18] = new PagePath(
            "ArchiveSetupHref",
            "ArchiveSetup.title",
            "ArchiveSetup.breadcrumbmouseover");

        pagePaths[19] = new PagePath(
            "SharedFSDetailsHref",
            "SharedFSDetails.pageTitle",
            "SharedFSDetails.breadcrumbmouseover");

        pagePaths[20] = new PagePath(
            "ServerSelectionHref",
            "ServerSelection.pageTitle",
            "ServerSelection.breadcrumbmouseover");

        pagePaths[21] = new PagePath(
            "VersionHighlightHref",
            "VersionHighlight.pageTitle",
            "VersionHighlight.breadcrumbmouseover");

        pagePaths[22] = new PagePath(
            "PolicySummaryHref",
            "PolicySummary.title",
            "PolicySummary.breadcrumbmouseover");

        pagePaths[23]  = new PagePath(
            "PolicyDetailsHref",
            "PolicyDetails.title",
            "PolicyDetails.breadcrumbmouseover");

        pagePaths[24]  = new PagePath(
            "CriteriaDetailsHref",
            "CriteriaDetails.title",
            "CriteriaDetails.breadcrumbmouseover");

        pagePaths[25]  = new PagePath(
            "CopyOptionsHref",
            "CopyOptions.title",
            "CopyOptions.breadcrumbmouseover");

        pagePaths[26]  = new PagePath(
            "CopyVSNsHref",
            "CopyVSNs.title",
            "copyVSNs.breadcrumbmouseover");

        pagePaths[27] = new PagePath(
            "NFSDetailsHref",
            "NFSDetails.pageTitle",
            "NFSDetails.breadcrumbmouseover");

        pagePaths[28] = new PagePath(
            "AdvancedNetworkConfigHref",
            "AdvancedNetworkConfig.browsertitle",
            "AdvancedNetworkConfig.breadcrumbmouseover");

        pagePaths[29] = new PagePath(
            "ImportVSNHref",
            "ImportVSN.browsertitle",
            "ImportVSN.breadcrumbmouseover");

        pagePaths[30] = new PagePath(
            "DataClassSummaryHref",
            "archiving.dataclass.summary.headertitle",
            "archiving.dataclass.summary.breadcrumbmouseover");

        pagePaths[31] = new PagePath(
            "DataClassDetailsHref",
            "archiving.dataclass.details.headertitle",
            "archiving.dataclass.details.breadcrumbmouseover");

        pagePaths[32] = new PagePath(
            "ISPolicyDetailsHref",
            "PolicyDetails.title",
            "PolicyDetails.breadcrumbmouseover");

        pagePaths[33] = new PagePath(
            "RecoveryPointsHref",
            "fs.recoverypoints.pagetitle",
            "fs.recoverypoints.breadcrumbmouseover");

        pagePaths[34] = new PagePath(
            "ScheduledTasksHref",
            "admin.scheduledtasks.pagetitle",
            "admin.scheduledtasks.breadcrumbmouseover");

        pagePaths[35] = new PagePath(
            "RecoveryPointScheduleHref",
            "fs.recoverypointschedule.pagetitle",
            "fs.recoverypointschedule.breadcrumbmouseover");

        pagePaths[36] = new PagePath(
            "SharedFSSummaryHref",
            "SharedFS.pagetitle",
            "SharedFS.title.breadcrumbmouseover");

        pagePaths[37] = new PagePath(
            "SharedFSClientsHref",
            "SharedFS.tab.clients",
            "");

        pageNames = new HashMap();

        // also make a hash table for reverse name lookup
        pageNames.put("FSDetails", new Integer(0));
        pageNames.put("FSArchivePolicies", new Integer(1));
        pageNames.put("FSSummary", new Integer(2));
        pageNames.put("VSNPoolSummary", new Integer(3));
        pageNames.put("VSNPoolDetails", new Integer(4));
        pageNames.put("CurrentJobs", new Integer(5));
        pageNames.put("PendingJobs", new Integer(6));
        pageNames.put("AllJobs", new Integer(7));
        pageNames.put("CurrentAlarmSummary", new Integer(8));
        pageNames.put("LibrarySummary", new Integer(9));
        pageNames.put("Historian", new Integer(10));
        pageNames.put("LibraryDriveSummary", new Integer(11));
        pageNames.put("VSNSummary", new Integer(12));
        pageNames.put("VSNDetails", new Integer(13));
        pageNames.put("VSNSearch", new Integer(14));
        pageNames.put("VSNSearchResult", new Integer(15));
        pageNames.put("LibraryFaultsSummary", new Integer(16));
        pageNames.put("TapeArchivePolCopy43", new Integer(17));
        pageNames.put("ArchiveSetUp", new Integer(18));
        pageNames.put("SharedFSDetails", new Integer(19));
        pageNames.put("ServerSelection", new Integer(20));
        pageNames.put("VersionHighlight", new Integer(21));
        pageNames.put("PolicySummary", new Integer(22));
        pageNames.put("PolicyDetails", new Integer(23));
        pageNames.put("CriteriaDetails", new Integer(24));
        pageNames.put("CopyOptions", new Integer(25));
        pageNames.put("CopyVSNs", new Integer(26));
        pageNames.put("NFSDetails",  new Integer(27));
        pageNames.put("AdvancedNetworkConfig", new Integer(28));
        pageNames.put("ImportVSN", new Integer(29));
        pageNames.put("DataClassSummary", new Integer(30));
        pageNames.put("DataClassDetails", new Integer(31));
        pageNames.put("ISPolicyDetails", new Integer(32));
        pageNames.put("RecoveryPoints", new Integer(33));
        pageNames.put("ScheduledTasks", new Integer(34));
        pageNames.put("RecoveryPointSchedule", new Integer(35));
        pageNames.put("SharedFSSummary", new Integer(36));
        pageNames.put("SharedFSClients", new Integer(37));
    }

    public PagePath getPagePath(int index) {
        return pagePaths[index];
    }

    public Integer getPageNumber(String name) {
        return (Integer) pageNames.get(name);
    }
}
