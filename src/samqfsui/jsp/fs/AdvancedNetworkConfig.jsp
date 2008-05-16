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

// ident	$Id: AdvancedNetworkConfig.jsp,v 1.9 2008/05/16 19:39:19 am143972 Exp $
--%>
<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.fs.AdvancedNetworkConfigViewBean">

<script language="javascript"
        src="/samqfsui/js/popuphelper.js">
</script>
<script language="javascript"
        src="/samqfsui/js/fs/AdvancedNetworkConfig.js">
</script>


<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="AdvancedNetworkConfig.browsertitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('0', 'AdvancedNetworkConfig');
        }"
    bundleID="samBundle">

<jato:form name="AdvancedNetworkConfigForm" method="post">

<!-- Bread Crumb component-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="AdvancedNetworkConfig.pagetitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false">
<br />

<jato:containerView name="AdvancedNetworkConfigDisplayView">
    <cc:actiontable
        name="AdvancedNetworkConfigDisplayTable"
        bundleID="samBundle"
        title="AdvancedNetworkConfig.display.tabletitle"
        selectionType="multiple"
        selectionJavascript = "toggleDisabledState(this)"
        showAdvancedSortIcon="false"
        showLowerActions="false"
        showPaginationControls="false"
        showPaginationIcon="false"
        showSelectionIcons="true" />
        
    <cc:hidden name="MDSNames" />
    <cc:hidden name="AllSharedHostNames" />
    <cc:hidden name="FSName" />
    <cc:hidden name="MDServerName" />

</jato:containerView>

<cc:hidden name="ServerName"/>

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 


