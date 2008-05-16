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
// ident	$Id: CommonTasks.jsp,v 1.6 2008/05/16 19:39:24 am143972 Exp $
--%>

<%@page info="CommonTasks" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="samqfs"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.util.CommonTasksViewBean">

<!-- stylesheet -->
<link href="/samqfsui/js/css_master.css" type="text/css" rel="stylesheet" />

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript" src="/samqfsui/js/CommonTasks.js"></script>

<!-- page header -->
<cc:header
    pageTitle="commontasks.headertitle" 
    copyrightYear="2007" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('6', 'CommonTasks');
        }"
    bundleID="samBundle">

<!-- the form and its contents -->
<jato:form name="CommonTasksForm" 
           defaultCommandChild="forwardToPage" 
           method="POST">

<div class="TskPgeFllPge">

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle" 
              bundleID="samBundle" 
              pageTitleText="commontasks.pagetitle" 
              showPageTitleSeparator="true" 
              showPageButtonsTop="false" 
              showPageButtonsBottom="false">

<div style="margin:5px 0px 0px 10px">
<cc:label name="description" 
          bundleID="samBundle"
          defaultValue="commontasks.pagetitle.description"
          styleLevel="3"/>
</div>

<%@page import="com.sun.netstorage.samqfs.web.util.CommonTasksViewBean"%>
<%
boolean qfsonly = ((CommonTasksViewBean)viewBean).isQFSOnly();
if (qfsonly) {
%>
<table cellspacing="5" cellpadding="5">
<tr><td class="TskPgeTpBx"/></tr>
<tr><td class="contentCell"> 
<samqfs:taskssection name="overviewTasks"/>
</td></tr>
<tr><td class="contentCell">
<samqfs:taskssection name="firstTimeUseTasks"/>
</td></tr>
<tr><td class="contentCell">
<samqfs:taskssection name="fileSystemTasks"/>
</td></tr>
<tr><td class="contentCell">
<samqfs:taskssection name="observabilityTasks"/>
</td></tr>
</table>

<% } else { %>
<table cellspacing="5" cellpadding="5">
<tr><td class="TskPgeTpBx" colspan="3"/></tr>
<tr><td class="contentCell" style="width="40%">
<samqfs:taskssection name="overviewTasks"/>
</td><td colspan="2"/></tr>
<tr><td class="contentCell" style="width:40%">
<samqfs:taskssection name="firstTimeUseTasks"/>
</td><td style="width:10%"/>
<td class="contentCell" style="width:40%">
<samqfs:taskssection name="fileSystemTasks"/>
</td></tr>

<tr><td class="contentCell">
<samqfs:taskssection name="archiveTasks"/>
</td><td/><td class="contentCell">
<samqfs:taskssection name="observabilityTasks"/>
</td></tr>

<tr><td class="contentCell">
<samqfs:taskssection name="storageAdministrationTasks"/>
</td><td colspan="2"/></tr>
<tr><td class="TskPgeBtmTr" colspan="3"/></tr>
</table>
<%}%>

<div id="wizardButtons" style="display: none">
<jato:containerView name="CommonTasksWizardsView">

<% if (qfsonly) { %>
<samqfs:wizardwindow name="newFileSystemWizard"/>
<%} else {%>
<samqfs:wizardwindow name="newPolicyWizard"/>
<samqfs:wizardwindow name="newDataClassWizard"/>
<samqfs:wizardwindow name="newFileSystemWizard"/>
<samqfs:wizardwindow name="addLibraryWizard"/>
<%}%>
</jato:containerView>
</div>

<cc:hidden name="serverName"/>
</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
