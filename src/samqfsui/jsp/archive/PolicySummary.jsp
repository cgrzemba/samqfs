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

// ident	$Id: PolicySummary.jsp,v 1.11 2008/07/23 21:25:27 kilemba Exp $
--%>

<%@page info="PolicySummary" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.PolicySummaryViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/policy44.js"> 
</script>

<!-- page header -->
<cc:header
    pageTitle="archiving.policy.summary.headertitle" 
    copyrightYear="2006" 
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('12', 'PolicySummary');
        }"
    bundleID="samBundle">

<!-- the form and its contents -->
<jato:form name="PolicySummaryForm" method="POST">

<cc:spacer name="Spacer" width="1" height="10" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle" 
              bundleID="samBundle" 
              pageTitleText="archiving.policy.summary.pagetitle" 
              showPageTitleSeparator="true" 
              showPageButtonsTop="false" 
              showPageButtonsBottom="false">

<cc:spacer name="spacer" height="10" width="1"/>

<!-- general policies table -->
<jato:containerView name="PolicySummaryView">

    <!-- general policies table -->
    <cc:actiontable 
                name="PolicySummaryTable"
                bundleID="samBundle"
                title="archiving.policy.summary.table.title"
                selectionType="single"
                selectionJavascript="handlePolicyTableSelection(this)"
                showAdvancedSortIcon="false"
                showLowerActions="true"
                showPaginationControls="true"
                showPaginationIcon="true"
                showSelectionIcons="true"
                maxRows="25"
                page="1"/>

<div style="margin:10px">
<cc:helpinline>
    <cc:text bundleID="samBundle" name="SAMFSWarning"/>
</cc:helpinline>
</div>
</jato:containerView>

<!-- dynamic buttons helper -->
<cc:hidden name="policyTypes"/>
<cc:hidden name="policyNames"/>
<cc:hidden name="policyToDelete"/>
<cc:hidden name="policyDeleteConfirmation"/>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
