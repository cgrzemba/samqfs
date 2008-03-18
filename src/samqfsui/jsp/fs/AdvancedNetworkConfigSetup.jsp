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

// ident	$Id: AdvancedNetworkConfigSetup.jsp,v 1.4 2008/03/17 14:40:32 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className=
    "com.sun.netstorage.samqfs.web.fs.AdvancedNetworkConfigSetupViewBean">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript"
    src="/samqfsui/js/fs/AdvancedNetworkConfigSetup.js">
</script>


<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="AdvancedNetworkConfig.setup.browsertitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="initializePopup(this);"
    bundleID="samBundle">


<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="AdvancedNetworkConfigSetupForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="AdvancedNetworkConfig.setup.pagetitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">
    
<br />

<jato:containerView name="AdvancedNetworkConfigSetupView">

    <table>
    <tr>
        <td><cc:spacer name="Spacer" width="10" /></td>
        <td>
            <cc:text name="Instruction" defaultValue="" bundleID="samBundle" />
        </td>
    </tr>
    <table>

    <br /><br />

    <cc:actiontable
        name="AdvancedNetworkConfigSetupTable"
        bundleID="samBundle"
        title="AdvancedNetworkConfig.setup.tabletitle"
        selectionType="none"
        showAdvancedSortIcon="false"
        showLowerActions="false"
        showPaginationControls="false"
        showPaginationIcon="false"
        showSelectionIcons="false" />
        
    <cc:hidden name="NumberOfMDS" />
    <cc:hidden name="IPAddresses" />

</jato:containerView>

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean> 
