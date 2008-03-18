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

// ident	$Id: CurrentAlarmSummary.jsp,v 1.19 2008/03/17 14:40:30 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
   className="com.sun.netstorage.samqfs.web.alarms.CurrentAlarmSummaryViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="CurrentAlarmSummary.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('32', 'CurrentAlarmSummary');
        }
        toggleDisabledState();"
    bundleID="samBundle">

<script language="javascript"
    src="/samqfsui/js/alarms/CurrentAlarmSummary.js">
</script>
<script language="javascript"
    src="/samqfsui/js/popuphelper.js">
</script>

<jato:form name="CurrentAlarmSummaryForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<table width="100%">
<tr>
    <td width="70%">
        <cc:pagetitle
            name="PageTitle"
            bundleID="samBundle"
            pageTitleText="CurrentAlarmSummary.title"
            showPageTitleSeparator="true"
            showPageButtonsTop="false"
            showPageButtonsBottom="false">
        <br />
            <cc:spacer
                name="Spacer"
                height="5"
                width="10" />

            <cc:label
                name="LogLabel"
                defaultValue="CurrentAlarmSummary.label"
                bundleID="samBundle" />

            <cc:dropdownmenu
                name="DropDownMenu"
                bundleID="samBundle"
                type="standard"
                onChange="
                    var button = 'CurrentAlarmSummary.ViewButton';
                    ccSetButtonDisabled(
                        button, 'CurrentAlarmSummaryForm', this.value == '');
                "/>
            <cc:button
                name="ViewButton"
                bundleID="samBundle"
                type="primary"
                disabled="true"
                dynamic="true"
                defaultValue="CurrentAlarmSummary.button"
                onClick="launchShowLogPopup(this); return false;"/>
    </td>
</tr>
</table>

<br /><br />    

<!-- Action Table -->
<jato:containerView name="CurrentAlarmSummaryView">
    <cc:actiontable
        name="CurrentAlarmSummaryTable"
        bundleID="samBundle"
        title="CurrentAlarmSummary.tabletitle"
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
<cc:hidden name="ServerName" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
