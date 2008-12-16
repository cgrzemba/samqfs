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

// ident	$Id: NFSDetails.jsp,v 1.18 2008/12/16 00:10:45 am143972 Exp $
--%>
<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@page import="com.sun.netstorage.samqfs.web.fs.NFSShareDisplayView" %>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.NFSDetailsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>
<script type="text/javascript" src="/samqfsui/js/fs/NFSDetails.js"></script>
 
<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header 
    pageTitle="filesystem.nfs.details.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('43', 'NFSDetails');
        }
        toggleDisabledState();"
    bundleID="samBundle">

<jato:form name="NFSDetailsForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="filesystem.nfs.details.pageTitle"
        showPageTitleSeparator="true"
        showPageButtonsTop="false"
        showPageButtonsBottom="false">

<br />

<!-- PropertySheet -->
<table width="100%">
<tr valign="top">
    <td width="40%">
        <jato:containerView name="NFSShareDisplayView">
            <cc:actiontable
                name="NFSShareDisplayTable"
                bundleID="samBundle"
                title="filesystem.nfs.sharedDirectories.tableTitle"
                showAdvancedSortIcon="false"
                showPaginationControls="true"
                selectionJavascript="toggleDisabledState(this)"
                showLowerActions="false"
                showPaginationIcon="true"
                showSelectionIcons="true"
                showSortingRow="false"
                selectionType="single"
                maxRows="25"
                page="1" />
        </jato:containerView>
    </td>
    <td width="60%">
        <!-- show either add/edit pagelet if necessary -->
<% 
    Integer flag = (Integer) viewBean.
            getPageSessionAttribute(NFSShareDisplayView.FLAG);
    if (flag != null)
        if (flag.intValue() == 1) {
%>
        <cc:includepagelet name="NFSShareAddView"/>
<%      } else if (flag.intValue() == 2) { %>
        <cc:includepagelet name="NFSShareEditView"/>
<% } %>

    </td>
</tr>
</table>

<cc:hidden name="SharedStateHiddenField" /> 
<cc:hidden name="DirNamesHiddenField" />
<cc:hidden name="ConfirmMessageHiddenField" />
<cc:hidden name="SelectedIndex" />
<cc:hidden name="ServerNameHiddenField" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 


