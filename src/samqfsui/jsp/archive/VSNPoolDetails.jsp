
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

// ident	$Id: VSNPoolDetails.jsp,v 1.17 2008/03/17 14:40:31 am143972 Exp $
--%>
<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.VSNPoolDetailsViewBean">

<cc:header
    pageTitle="VSNPoolDetails.pageTitle" 
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('22', 'VSNPoolDetails');
        }"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/archive/vsnpools.js"></script>

<script language="javascript">

    function showConfirmMsg(key) {

	var str1 = "<cc:text name='StaticText' bundleID='samBundle' defaultValue='VSNPoolSummary.confirmMsg1'/>";

	if (key == 1) {
	if (!confirm(str1))
		return false;
	else return true;
	 } else return false; // this case should never be used

    }

    function getSelectedPoolName() {

        var myForm = document.VSNPoolDetailsForm;
        var poolName = myForm.elements["VSNPoolDetails.VSNPoolNameField"].value;

        return poolName;
    }

</script>

<!-- Form -->
<jato:form name="VSNPoolDetailsForm" method="post">

<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" /> 

<br>
<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />
<br>

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
<jato:containerView name="VSNPoolDetailsView">
  <cc:actiontable
    name="VSNPoolDetailsTable"
    bundleID="samBundle"
    title="VSNPoolDetails.tabletitle"
    selectionType="none"
    showAdvancedSortIcon="true"
    showLowerActions="true"
    showPaginationControls="true"
    showPaginationIcon="false"
    showSelectionIcons="true"
    page="1"/>
</jato:containerView>

<cc:spacer name="spacer1" height="20"/>

<div style="padding-left:20px">
<cc:text name="reservedVSNMessage"
    escape="false"
    bundleID="samBundle"/>
</div>

</cc:pagetitle>

<cc:hidden name="NEHiddenField1" />
<cc:hidden name="NEHiddenField2" />
<cc:hidden name="NEHiddenField3" />
<cc:hidden name="NEHiddenField4" />
<cc:hidden name="NEHiddenField5" />
<cc:hidden name="VSNPoolNameField" />
<cc:hidden name="deleteConfirmation" bundleID="samBundle"/>
<cc:hidden name="ServerName"/>

</jato:form>
</cc:header>
</jato:useViewBean> 

