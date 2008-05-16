<%--

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

// ident	$Id: DiskVSNSummary.jsp,v 1.11 2008/05/16 19:39:17 am143972 Exp $
--%>

<%@page info="DiskVSNSummary" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.DiskVSNSummaryViewBean">

<!-- include the helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/diskvsn.js"></script>
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- page header -->
<cc:header
    pageTitle="archiving.diskvsn.summary.headertitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('23', 'DiskVSNSummary');
        }
        handleDiskVSNSummaryTableSelection();"
    bundleID="samBundle">

<!-- the form and its contents -->
<jato:form name="DiskVSNSummaryForm" method="POST">

<!-- feedback alert -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="pageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.diskvsn.summary.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="false"
              showPageButtonsBottom="false">
              
<br />

<!-- disk vsn table -->
<jato:containerView name="DiskVSNSummaryView">
    <cc:actiontable name="DiskVSNSummaryTable"
                    bundleID="samBundle"
                    title="archiving.diskvsn.summarytable.title"
                    selectionType="single"
                    selectionJavascript="handleDiskVSNSummaryTableSelection(this);"
                    showAdvancedSortIcon="false"
                    showLowerActions="true"
                    showPaginationControls="true"
                    showPaginationIcon="true"
                    maxRows="25"
                    page="1"/>
</jato:containerView>

<!-- js helper fields -->
<cc:hidden name="name"/>
<cc:hidden name="host"/>
<cc:hidden name="path"/>
<cc:hidden name="createPath"/>
<cc:hidden name="vsns"/>
<cc:hidden name="messages"/>
<cc:hidden name="server_name"/>
<cc:hidden name="selected_vsn_name"/>
<cc:hidden name="flags"/>
</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
