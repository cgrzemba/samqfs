<%--

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

// ident	$Id: ScheduledTasks.jsp,v 1.4 2008/03/17 14:40:29 am143972 Exp $
--%>

<%@page info="ScheduledTasks" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.ScheduledTasksViewBean">

<!-- page header -->
<cc:header
    pageTitle="scheduledtasks.title"
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('111', 'ScheduledTasks');
        }"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/admin/scheduled_tasks.js">
</script>

<!-- the form and its contents -->
<jato:form name="ScheduledTasksForm" method="POST">

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle" 
              bundleID="samBundle" 
              pageTitleText="admin.scheduledtasks.pagetitle" 
              showPageTitleSeparator="true" 
              showPageButtonsTop="false" 
              showPageButtonsBottom="false">

<cc:spacer name="spacer1" height="10"/>

<!-- scheduled tasks action table -->
<jato:containerView name="ScheduledTasksView">
    <cc:actiontable name="ScheduledTasksTable"
                    bundleID="samBundle"
                    title="admin.scheduledtasks.summary.tabletitle"
                    selectionType="single"
                    selectionJavascript="handleScheduleSelection(this);"
                    showAdvancedSortIcon="false"
                    showLowerActions="true"
                    showPaginationControls="true"
                    showPaginationIcon="true"
                    showSelectionIcons="true"
                    maxRows="25"
                    page="1" />  

<cc:hidden name="selectedName"/>
<cc:hidden name="selectedId"/>

</jato:containerView>

<cc:hidden name="allScheduleNames"/>
<cc:hidden name="allScheduleIds"/>
<cc:hidden name="errMsg"/>
</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>





























