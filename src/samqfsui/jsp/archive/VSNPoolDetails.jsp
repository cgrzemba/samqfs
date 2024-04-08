
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

// ident	$Id: VSNPoolDetails.jsp,v 1.20 2008/12/16 00:10:43 am143972 Exp $
--%>
<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.VSNPoolDetailsViewBean">

<cc:header
    pageTitle="VSNPoolDetails.pageTitle"
    copyrightYear="2008"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('22', 'VSNPoolDetails');
        }"
    bundleID="samBundle">

<!-- Form -->
<jato:form name="VSNPoolDetailsForm" method="post">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/archive/vsnpools.js"></script>
<script language="javascript"
        src="/samqfsui/js/archive/mediaexpression.js"></script>


<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />

<!-- Page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="VSNPoolDetails.pageTitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="true"
              showPageButtonsBottom="true">

<cc:propertysheet name="PropertySheet"
              bundleID="samBundle"
              showJumpLinks="false"/>

<!-- Action Table -->
<jato:containerView name="MediaExpressionView">
  <cc:actiontable
    name="MediaExpressionTable"
    bundleID="samBundle"
    title="MediaAssignment.tabletitle"
    selectionType="multiple"
    showAdvancedSortIcon="true"
    showLowerActions="true"
    showPaginationControls="true"
    showPaginationIcon="false"
    showSelectionIcons="true"
    page="1"/>
  <br />
  <div style="padding-left:20px">
    <cc:text name="reservedVSNMessage"
      escape="false"
      bundleID="samBundle"/>
  </div>

  <cc:hidden name="VSNPoolNameField" />
  <cc:hidden name="MediaType" />
  <cc:hidden name="Expressions"/>
  <cc:hidden name="SelectedExpression"/>
  <cc:hidden name="deleteConfirmation" bundleID="samBundle"/>
  <cc:hidden name="ServerName"/>
  <cc:hidden name="hasPermission"/>
  <cc:hidden name="NoSelectionMsg"/>
  <cc:hidden name="deletePoolConfirmation"/>
  <cc:hidden name="NoPermissionMsg"/>
</jato:containerView>

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean>
