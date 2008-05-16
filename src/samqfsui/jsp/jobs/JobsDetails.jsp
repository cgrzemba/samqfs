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

// ident	$Id: JobsDetails.jsp,v 1.12 2008/05/16 19:39:21 am143972 Exp $
--%>
<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@ page import="com.iplanet.jato.view.ViewBean" %>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.jobs.JobsDetailsViewBean">

<cc:header
    pageTitle="JobsDetails.pageTitle" 
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    bundleID="samBundle"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('33', 'JobsDetails');
        }
        javascript:setInterval('refreshPage()', getRefreshTimeInterval() + 0);">

<script language="javascript">

    function refreshPage() {
        // Form Name
        var tf	 = document.JobsDetailsForm;

        // Command Child Name
        var commandChildName = "JobsDetails.RefreshSubmitHref";

        // Set Form action URL and submit
        tf.action =
            tf.action + "?" + commandChildName + "=&jato.pageSession=" +
        tf.elements["<%=ViewBean.PAGE_SESSION_ATTRIBUTE_NVP_NAME %>"].value;
        tf.submit();
    }

    function getRefreshTimeInterval() {
	var tf = document.JobsDetailsForm;
	var refreshRate =
            tf.elements["JobsDetails.RefreshRateHiddenField"].value;

	if (refreshRate == "") {
            refreshRate = 10000;
	}
	return refreshRate * 1000;
    }

    function getFSName() {
        var myForm = document.JobsDetailsForm;
        var fsName = myForm.elements["JobsDetails.JobsDetailsView.fsNameHidden"].value;
        return fsName;
    }


    function getClientParams() {
        var fsNameParam = null;
        var fsName = getFSName();
        if (fsName != null) {
            fsNameParam = "fsNameParam=" + fsName;
        }
        return fsNameParam;
    }

</script>

<!-- Form -->
<jato:form name="JobsDetailsForm" method="post">

<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<br />

<tr>
    <td>
        <cc:spacer name="Spacer" width="300" />
    </td>
    <td>
        <cc:label name="RefreshLabel"
            bundleID="samBundle"
            defaultValue="Page refresh rate: " />
    </td>
</tr>
   
<cc:spacer name="Spacer" width="5" />

<cc:dropdownmenu name="RefreshMenu"
    bundleID="samBundle"
    commandChild="RefreshMenuHref"
    type="jump" />

<br /><br />

<jato:containerView name="JobsDetailsView">

    <!-- Page title -->
    <cc:pagetitle
        name="PageTitle"
        bundleID="samBundle"
        pageTitleText="JobsDetails.pageTitle"
        showPageTitleSeparator="true"
        showPageButtonsTop="true"
        showPageButtonsBottom="true" />

    <cc:propertysheet
        name="PropertySheet"
        bundleID="samBundle"
        showJumpLinks="true"/>

</jato:containerView>

<cc:hidden name="RefreshRateHiddenField" />

</jato:form>
</cc:header>
</jato:useViewBean> 

