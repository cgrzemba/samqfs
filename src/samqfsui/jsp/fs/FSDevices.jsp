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

// ident	$Id: FSDevices.jsp,v 1.13 2008/05/16 19:39:19 am143972 Exp $
--%>
<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.fs.FSDevicesViewBean">

<script language="javascript" src="/samqfsui/js/fs/FSDevices.js"/>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="FSDevices.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('0', 'FSDevices');
        }"
    bundleID="samBundle">

<jato:form name="FSDevicesForm" method="post">

<!-- Bread Crumb component-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="<insert>"
        showPageTitleSeparator="true"
        showPageButtonsTop="false"
        showPageButtonsBottom="false">
<br/>
<jato:containerView name="FSDevicesView">
    <!-- Action Table -->
    <cc:actiontable
        name="FSDevicesTable"
        bundleID="samBundle"
        title="FSDevices.tabletitle"
        selectionType="multiple"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="true"
        maxRows="25"
        page="1"/>
    <cc:hidden name="AllDevices"/>
    <cc:hidden name="SelectedDevices"/>
    <cc:hidden name="NoSelectionMsg"/>
    <cc:hidden name="DisableMsg"/>
</jato:containerView>
</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean>
