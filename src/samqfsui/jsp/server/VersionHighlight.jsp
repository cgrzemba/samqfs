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

// ident	$Id: VersionHighlight.jsp,v 1.16 2008/12/16 00:10:51 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.server.VersionHighlightViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header pageTitle="VersionHighlight.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle" >

<!-- Masthead -->
<cc:primarymasthead name="Masthead" bundleID="samBundle" />

<!-- Navigation Tabs -->
<jato:content name="LocalTabs">
    <cc:tabs name="Tabs" bundleID="samBundle" />
</jato:content>

<jato:form name="VersionHighlightForm" method="post">

<!-- Bread Crumb component -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="VersionHighlight.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false">

<br />

<table border="0">
<tr>
    <td align="right">
        <cc:image
            name="Image"
            bundleID="samBundle"
            defaultValue="/samqfsui/images/samqfs_new.gif"
            alt="version.highlight.versionstatus.new" />
        <cc:spacer name="Spacer" width="1" height="1" />
        <cc:text name="Text" bundleID="samBundle"
            defaultValue="version.highlight.versionstatus.new" />

        <cc:spacer name="Spacer" width="5" height="1" />

        <cc:image
            name="Image"
            bundleID="samBundle"
            defaultValue="/samqfsui/images/samqfs_upgrade.gif"
            alt="version.highlight.versionstatus.upgrade" />
        <cc:spacer name="Spacer" width="1" height="1" />
        <cc:text name="Text" bundleID="samBundle"
            defaultValue="version.highlight.versionstatus.upgrade" />

        <cc:spacer name="Spacer" width="5" height="1" />

        <cc:image
            name="Image"
            bundleID="samBundle"
            defaultValue="/samqfsui/images/samqfs_available.gif"
            alt="version.highlight.versionstatus.available" />
        <cc:spacer name="Spacer" width="1" height="1" />
        <cc:text name="Text" bundleID="samBundle"
            defaultValue="version.highlight.versionstatus.available" />

        <cc:spacer name="Spacer" width="5" height="1" />
    </td>
    <td>&nbsp;</td>
</tr>
<tr>
    <td width="80%">
        <!-- Action Table -->
        <jato:containerView name="VersionHighlightView">
            <cc:actiontable
                name="VersionHighlightTable"
                bundleID="samBundle"
                title="VersionHighlight.tabletitle"
                summary=""
                selectionType="none"
                showAdvancedSortIcon="false"
                showLowerActions="false"
                showPaginationControls="false"
                showPaginationIcon="false"
                showSelectionIcons="false"
            />
        </jato:containerView>
    </td>
    <td width="30%">
        <cc:spacer name="Spacer" height="1" width="300" />
    </td>
</tr>
</table>

<cc:spacer name="Spacer" width="5" height="5" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
