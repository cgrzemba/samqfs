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

// ident	$Id: AllJobs.jsp,v 1.16 2008/05/16 19:39:20 am143972 Exp $
--%>

<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.jobs.AllJobsViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="AllJobs.title"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('36', 'AllJobs');
        }
        toggleDisabledState();"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/jobs/AllJobs.js"></script>

<jato:form name="AllJobsForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="AllJobs.title"
        showPageTitleSeparator="true"
        showPageButtonsTop="true"
        showPageButtonsBottom="true">

<br>

<!-- javascript value -->
<cc:hidden name='ConfirmMsg'/>

<!-- Action Table -->
<jato:containerView name="AllJobsView">
  <cc:actiontable
    name="AllJobsTable"
    bundleID="samBundle"
    title="AllJobs.title"
    selectionType="multiple"
    selectionJavascript = "toggleDisabledState()"
    showAdvancedSortIcon="true"
    showLowerActions="true"
    showPaginationControls="true"
    showPaginationIcon="true"
    showSelectionIcons="true"
    maxRows="25"
    page="1"/>
</jato:containerView>

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean>
