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

// ident	$Id: CriteriaDetails.jsp,v 1.14 2008/05/16 19:39:17 am143972 Exp $
--%>

<%@page info="CriteriaDetails" language="java"%>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato" %>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc" %>
<%@page import="com.sun.netstorage.samqfs.mgmt.arc.ArSet" %>
<%@page import="com.sun.netstorage.samqfs.web.util.Constants" %>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/criteriadetails44.js">
</script>
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- page header -->
<cc:header
    pageTitle="archiving.criteria.details.headertitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('12', 'CriteriaDetails');
        }"
    bundleID="samBundle">

<!-- the form -->
<jato:form name="CriteriaDetailsForm" method="POST">

<!-- bread crumbing -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/> <br />

<!-- page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.criteria.details.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="true"
              showPageButtonsBottom="true">

<!-- message text for default policies -->
&nbsp;
<cc:text name="message" bundleID="samBundle"/>

<!-- the property sheet -->
<cc:propertysheet name="PropertySheet"
                  bundleID="samBundle"
                  addJavaScript="true"
                  showJumpLinks="false"/>

<cc:spacer name="spacer1" height="20" newline="true"/>

<!-- the two the tables -->
<jato:containerView name="CriteriaDetailsView">

<% 
Integer policyType = 
    (Integer)viewBean.getPageSessionAttribute(Constants.Archive.POLICY_TYPE);

if (policyType.intValue() == ArSet.AR_SET_TYPE_GENERAL) {
%>

<!-- copy settings for criteria table -->
<cc:actiontable name="CriteriaDetailsCopySettingsTable"
                bundleID="samBundle"
                title="archiving.criteria.copysettings.table.title"
                showAdvancedSortIcon="false"
                showLowerActions="false"
                showPaginationControls="false"
                showPaginationIcon="false"
                showSelectionIcons="false"/>

<cc:spacer name="spacer2" height="20" newline="true"/>
<% } %>

<cc:actiontable name="CriteriaDetailsFSTable"
                bundleID="samBundle"
                title="archiving.criteria.fs.table.title"
                selectionType="single"
             selectionJavascript="handleCriteriaDetailsFSTableSelection(this)"
                showAdvancedSortIcon="false"
                showLowerActions="true"
                showPaginationControls="true"
                showPaginationIcon="true"
                showSelectionIcons="true"
                maxRows="25"
                page="1"/>

</jato:containerView>

<cc:spacer name="spacer3" height="20" width="1"/>

<div style="padding-left:10px">
<cc:text name="globalCriteriaText"
         bundleID="samBundle"/>
</div>

<cc:hidden name="fsname"/>
<cc:hidden name="dumpPath"/>
<cc:hidden name="fsDeletable"/>
<cc:hidden name="fsDeleteConfirmation"/>
<cc:hidden name="psAttributes"/>
<cc:hidden name="fsList"/>
<cc:hidden name="ServerName"/>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
