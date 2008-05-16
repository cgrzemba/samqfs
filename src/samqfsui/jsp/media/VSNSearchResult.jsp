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

// ident	$Id: VSNSearchResult.jsp,v 1.15 2008/05/16 19:39:22 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.media.VSNSearchResultViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="VSNSearchResult.browserpagetitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('21', 'VSNSearchResult');
        }"
    bundleID="samBundle">

<jato:form name="VSNSearchResultForm" method="post">

<!-- BreadCrumb component-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="VSNSearchResult.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false">

<!-- Action Table -->
<jato:containerView name="VSNSearchResultView">
    <cc:actiontable
        name="VSNSearchResultTable"
        bundleID="samBundle"
        title="VSNSearchResult.tabletitle"
        selectionType="none"
        showAdvancedSortIcon="true"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="true"
        showSelectionIcons="true"
        empty="VSNSearchResult.tableempty"
        maxRows="25"
        page="1"/>
</jato:containerView>

<br />
<br />

<tr>
    <td><cc:spacer name="Spacer" width="15" height="1" /></td>
    <td>
        <cc:label
            name="Label"
            bundleID="samBundle"
            defaultValue="VSNSearchResult.col1"
            elementName="SearchText" />
    </td>
    <td><cc:spacer name="Spacer" width="15" height="1" /></td>
    <td><cc:text name="SearchText" /></td>
</tr>

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
