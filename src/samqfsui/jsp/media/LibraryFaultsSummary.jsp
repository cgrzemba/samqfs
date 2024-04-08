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

// ident	$Id: LibraryFaultsSummary.jsp,v 1.16 2008/12/16 00:10:48 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.media.LibraryFaultsSummaryViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="LibraryFaultsSummary.browserpagetitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('21', 'LibraryFaultsSummary');
        }
        toggleDisabledState();"
    bundleID="samBundle">
    
<script language="javascript"
    src="/samqfsui/js/media/LibraryFaultsSummary.js">
</script>

<jato:form name="LibraryFaultsSummaryForm" method="post">

<!-- BreadCrumb component-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />


<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="LibraryFaultsSummary.title"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false">

<br />

<!-- Action Table -->
<jato:containerView name="LibraryFaultsSummaryView">
    <cc:actiontable
        name="LibraryFaultsSummaryTable"
        bundleID="samBundle"
        title="LibraryFaultsSummary.tabletitle"
        selectionType="multiple"
        selectionJavascript = "toggleDisabledState(this)"
        showAdvancedSortIcon="true"
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
