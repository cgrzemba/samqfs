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

// ident	$Id: CopyVSNs.jsp,v 1.15 2008/04/09 20:37:27 ronaldso Exp $
--%>

<%@page info="CopyVSNs" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato" %>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.CopyVSNsViewBean">

<!-- include helper javascript -->
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript"
        src="/samqfsui/js/archive/vsnpools.js"></script>
<script language="javascript" src="/samqfsui/js/archive/copyvsns.js"></script>

<!-- page header -->
<cc:header
    pageTitle="archiving.policy.copyvsns.headertitle"
    copyrightYear="2008"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('12', 'CopyVSNs');
        }"
    bundleID="samBundle">

<!-- the form -->
<jato:form name="CopyVSNsForm" method="POST">

<!-- bread crumbing -->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- alert for feedback -->
<cc:alertinline name="Alert" bundleID="samBundle"/> <br />

<!-- page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.policy.copyvsns.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="true"
              showPageButtonsBottom="true">

<!-- body -->
<cc:spacer name="spacer" height="10" width="1"/>

<table cellspacing="10">
    <tr><td class="indent">
        <cc:label name="LabelMediaType"
                  defaultValue="archiving.mediatype"
                  bundleID="samBundle"
                  elementName="mediaType"/>
    </td><td>
        <cc:dropdownmenu name="mediaType"
                         bundleID="samBundle"
                         onClick="saveMenuValue(this)"
                         onChange="handleMediaTypeChange(this)"/>
    </td><td>
        <cc:spacer name="Spacer" width="20" />
        <cc:label name="LabelFreeSpace"
                  defaultValue="archiving.freespace"
                  bundleID="samBundle"
                  elementName="TextFreeSpace"/>
    </td><td>
        <cc:text name="TextFreeSpace"
                 bundleID="samBundle"/>
    </td></tr>
</table>
<br>

<!-- Action Table -->
<jato:containerView name="MediaExpressionView">
  <cc:actiontable
    name="MediaExpressionTable"
    bundleID="samBundle"
    title="MediaAssignment.tabletitle.copyvsn"
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
    <cc:text name="inheritAllSetsMessage"
      bundleID="samBundle"/>
  </div>

  <cc:hidden name="Expressions"/>
  <cc:hidden name="SelectedExpression"/>
  <cc:hidden name="SelectedPool"/>
  <cc:hidden name="MediaType"/>
  <cc:hidden name="poolBoolean"/>
  <cc:hidden name="deleteConfirmation" bundleID="samBundle"/>
  <cc:hidden name="ServerName"/>
  <cc:hidden name="hasPermission"/>
  <cc:hidden name="NoSelectionMsg"/>
  <cc:hidden name="deletePoolConfirmation"/>
  <cc:hidden name="NoPermissionMsg"/>
</jato:containerView>

</cc:pagetitle>

<cc:hidden name="ResetMessage"/>
<cc:hidden name="DeleteAllMessage"/>
<cc:hidden name="ServerName"/>
<cc:hidden name="PolicyName"/>
<cc:hidden name="CopyNumber"/>


</jato:form>
</cc:header>
</jato:useViewBean>
