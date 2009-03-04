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

// ident	$Id: FSDetails.jsp,v 1.37 2009/03/04 21:54:41 ronaldso Exp $
--%>
<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean">

<cc:header
    pageTitle="FSDetails.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('0', 'FSDetails');
        }
        enableComponents();"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/fs/popups.js"></script>
<script language="javascript" src="/samqfsui/js/fs/cluster.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/fs/FSDetails.js"></script>
<script language="javascript" src="/samqfsui/js/fs/FSDevices.js"></script>

<!-- Form -->
<jato:form name="FSDetailsForm" method="post">

<!-- Bread Crumb componente-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />

<jato:containerView name="FSDetailsView">

<!-- Page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="FSDetails.pageTitle"
              pageTitleHelpMessage='fs.details.instruction'
              showPageTitleSeparator="true"
              showPageButtonsTop="false"
              showPageButtonsBottom="false">

<!-- PropertySheet -->
<cc:propertysheet name="PropertySheet"
              bundleID="samBundle"
              showJumpLinks="false"/>

<cc:hidden name="SamfsckHiddenAction" />
<cc:hidden name="SamfsckHiddenLog" />

<cc:hidden name="fsName" />
<cc:hidden name="HiddenDynamicMenuOptions" />

</cc:pagetitle>

</jato:containerView>

<!-- sunplex manager link -->
<cc:includepagelet name="SunPlexManagerView"/>

<!-- cluster node list table -->
<cc:includepagelet name="FSDClusterView"/>

<br><br>
<jato:containerView name="FSDevicesView">
    <cc:spacer name="Spacer" width="10" />
    <cc:text
        name="Instruction"
        bundleID="samBundle"
        defaultValue="FSDevices.instruction" />
    <br /><br />
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
    <cc:hidden name="NoMountMsg"/>
    <cc:hidden name="NoSharedClientMsg"/>
</jato:containerView>

<cc:hidden name="ServerName" />
<cc:hidden name="ConfirmMessages" />

</jato:form>
</cc:header>
</jato:useViewBean>

