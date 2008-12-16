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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: LibraryDriveSummary.jsp,v 1.23 2008/12/16 00:10:48 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.media.LibraryDriveSummaryViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="LibraryDriveSummary.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('21', 'LibraryDriveSummary');
        }
        disableUnloadforACSLS();
        toggleDisabledState();"
    bundleID="samBundle">

<script language="javascript"
    src="/samqfsui/js/samqfsui.js">
</script>
<script language="javascript"
    src="/samqfsui/js/media/LibraryDriveSummary.js">
</script>
<script language="javascript"
    src="/samqfsui/js/popuphelper.js">
</script>

<jato:form name="LibraryDriveSummaryForm" method="post">

<!-- Bread Crumb component -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<br />
<!-- Page title -->
<cc:pagetitle
    name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="LibraryDriveSummary.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false">
<br />

<!-- PropertySheet -->
<cc:propertysheet
    name="PropertySheet" 
    bundleID="samBundle" 
    showJumpLinks="true"/>

<jato:containerView name="LibraryDriveSummaryView">
    <cc:spacer
        name="Spacer"
        width="20" />
    <cc:actiontable
        name="LibraryDriveSummaryTable"
        bundleID="samBundle"
        title="LibraryDriveSummary.tabletitle"
        selectionType="single"
        selectionJavascript = "toggleDisabledState(this)"
        showAdvancedSortIcon="true"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="true"
        showSelectionIcons="true"
        maxRows="25"
        page="1"/>

    <!-- Hidden Fields for Change Status Pop Up Values -->
    <cc:hidden name="EQHiddenField" />
</jato:containerView>

<cc:hidden name="LibraryName" />
<cc:hidden name="ServerName" />
<cc:hidden name="Driver" />
<cc:hidden name="DriversString" />
<cc:hidden name="ConfirmMessage" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
