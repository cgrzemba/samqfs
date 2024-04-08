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

// ident	$Id: DataClassSummary.jsp,v 1.11 2008/12/16 00:10:42 am143972 Exp $
--%>


<script type="text/javascript"
    src="/samqfsui/js/archive/DataClassSummary.js">
</script>

<%@page info="DataClassSummary" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="samqfs"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.DataClassSummaryViewBean">

<!-- page header -->
<cc:header
    pageTitle="archiving.dataclass.summary.headertitle" 
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('11', 'DataClassSummary');
        }
        handleDataClassTableSelection();"
    bundleID="samBundle">

<!-- the form and its contents -->
<jato:form name="DataClassSummaryForm" method="POST">


<cc:spacer name="Spacer" width="1" height="10" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle" 
              bundleID="samBundle" 
              pageTitleText="archiving.dataclass.summary.pagetitle" 
              showPageTitleSeparator="true" 
              showPageButtonsTop="false" 
              showPageButtonsBottom="false">

<cc:spacer name="spacer" height="10" width="1"/>

<!-- data class table -->
<jato:containerView name="DataClassSummaryView">

    <!-- data class table -->
    <cc:actiontable 
                name="DataClassSummaryTable"
                bundleID="samBundle"
                title="archiving.dataclass.summary.table.title"
                selectionType="single"
                selectionJavascript="handleDataClassTableSelection(this)"
                showAdvancedSortIcon="false"
                showLowerActions="true"
                showPaginationControls="true"
                showPaginationIcon="true"
                showSelectionIcons="true"
                maxRows="25"
                page="1"/>

</jato:containerView>

<!-- dynamic buttons helper -->
<cc:hidden name="dataClassToDelete"/>
<cc:hidden name="dataClassDeleteConfirmation"/>

<!-- has permission to write? -->
<cc:hidden name="hasPermission" />

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
