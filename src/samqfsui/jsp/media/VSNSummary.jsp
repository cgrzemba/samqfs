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

// ident	$Id: VSNSummary.jsp,v 1.29 2008/04/03 02:21:38 ronaldso Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.media.VSNSummaryViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="VSNSummary.browserpagetitle"
    copyrightYear="2003"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('25', 'VSNSummary');
        }
        toggleDisabledState();"
    bundleID="samBundle">

<jato:form name="VSNSummaryForm" method="post">

<script language="javascript"
    src="/samqfsui/js/media/VSNSummary.js">
</script>
<script language="javascript"
    src="/samqfsui/js/popuphelper.js">
</script>

<!-- Bread Crumb component -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<table width="100%">
<tr>
    <td width="70%">
        <cc:pagetitle
            name="PageTitle"
            bundleID="samBundle"
            pageTitleText="VSNSummary.pageTitle"
            showPageTitleSeparator="true"
            showPageButtonsTop="false"
            showPageButtonsBottom="false">
    </td>
    <td width="30%">
        <cc:label
            name="Label"
            bundleID="samBundle"
            defaultValue="VSNSummary.switchlibrary"
            styleLevel="2"/>
        <cc:dropdownmenu
            name="SwitchLibraryMenu"
            bundleID="samBundle"
            onChange="handleSwitchLibraryMenu(this); return false;"
            type="default" />
    </td>
</table>

<br />

<!-- Action Table -->
<jato:containerView name="VSNSummaryView">
    <cc:actiontable
        name="VSNSummaryTable"
        bundleID="samBundle"
        title="VSNSummary.pageTitle"
        selectionType="single"
        selectionJavascript = "toggleDisabledState(this)"
        showAdvancedSortIcon="true"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="false"
        showSelectionIcons="true"
        page="1"/>
        
    <cc:hidden name="Role" />
</jato:containerView>

<cc:hidden name="LoadVSNHiddenField" />
<cc:hidden name="LibraryNameHiddenField" />
<cc:hidden name="ConfirmMessageHiddenField" />
<cc:hidden name="ServerNameHiddenField" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
