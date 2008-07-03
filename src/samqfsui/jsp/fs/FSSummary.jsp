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

// ident	$Id: FSSummary.jsp,v 1.46 2008/07/03 00:04:28 ronaldso Exp $
--%>
<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/fs/FSSummary.js"></script>
<script language="javascript" src="/samqfsui/js/fs/popups.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="FSSummary.title"
    copyrightYear="2008"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('0', 'FSSummary');
        }
        toggleDisabledState();"
    bundleID="samBundle">
    
<jato:form name="FSSummaryForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="FSSummary.title"
        showPageTitleSeparator="true"
        showPageButtonsTop="false"
        showPageButtonsBottom="false">
<br />

<!-- javascript hidden fields -->
<cc:hidden name='ConfirmMsg1' bundleID='samBundle'/>
<cc:hidden name='ConfirmMsg2' bundleID='samBundle'/>

<!-- Action Table -->
<jato:containerView name="FileSystemSummaryView">
  <cc:actiontable
    name="FileSystemSummaryTable"
    bundleID="samBundle"
    title="FSSummary.tabletitle"
    selectionType="single"
    selectionJavascript = "toggleDisabledState(this)"
    showAdvancedSortIcon="true"
    showLowerActions="true"
    showPaginationControls="true"
    showPaginationIcon="true"
    showSelectionIcons="true"
    maxRows="25"
    page="1"/>

    <cc:hidden name="SamfsckHiddenField1" />
    <cc:hidden name="SamfsckHiddenField2" />
    <cc:hidden name="GrowHiddenField" />
    <cc:hidden name="FSNAMEHiddenField" />

    <span style="display:none">
        <cc:wizardwindow name="SamQFSWizardGrowFSButton" />
    </span>

</jato:containerView>

<cc:hidden name="LicenseTypeHiddenField" />
<cc:hidden name="ServerName" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
