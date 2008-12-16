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

// ident	$Id: ServerSelection.jsp,v 1.21 2008/12/16 00:10:51 am143972 Exp $
--%>

<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.server.ServerSelectionViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header pageTitle="ServerSelection.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle"
    onLoad="toggleDisabledState()" >

<script language="javascript"
    src="/samqfsui/js/server/ServerSelection.js">
</script>
<script language="javascript"
    src="/samqfsui/js/popuphelper.js">
</script>

<style>
    td.indent1{padding-left:10px; text-align:left}
    td.indent2{padding-left:30px; padding-top:20px; text-align:left}
</style>

<!-- Masthead -->
<cc:primarymasthead name="Masthead"
    bundleID="samBundle" />

<!-- Navigation Tabs -->
<jato:content name="LocalTabs">
    <cc:tabs name="Tabs" bundleID="samBundle" />
</jato:content>

<jato:form name="ServerSelectionForm"
    method="post">

<cc:alertinline name="Alert"
    bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
    pageTitleText="ServerSelection.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false">

<br />

<table>
<tr>
    <td class="indent1">
        <cc:text
            name="StaticText1"
            defaultValue="ServerSelection.help.1"
            escape="false"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td class="indent2">
        <cc:text
            name="StaticText2"
            defaultValue="ServerSelection.help.2"
            escape="false"
            bundleID="samBundle" />
    </td>
</tr>
</table>

<br /><br />

<!-- Action Table -->
<jato:containerView name="ServerSelectionView">
    <cc:actiontable
        name="ServerSelectionTable"
        bundleID="samBundle"
        title="ServerSelection.tabletitle"
        selectionType="single"
        selectionJavascript = "toggleDisabledState(this)"
        showAdvancedSortIcon="false"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="true"
        showSelectionIcons="true"
        maxRows="25"
        page="1"/>

</jato:containerView>

<cc:hidden name="ConfirmMessageHiddenField" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean>
