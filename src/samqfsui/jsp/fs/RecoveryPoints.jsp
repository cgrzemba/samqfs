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

// ident	$Id: RecoveryPoints.jsp,v 1.9 2008/12/16 00:10:46 am143972 Exp $
--%>
<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.fs.RecoveryPointsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/fs/popups.js"></script>
<script type="text/javascript" src="/samqfsui/js/fs/RecoveryPoints.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="fs.recoverypoints.pagetitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('42', 'RecoveryPoints');
        }
        handleFileSelection();"
    bundleID="samBundle">

<jato:form name="RecoveryPointsForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="fs.recoverypoints.pagetitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false"/>
    
<!-- PropertySheet -->
<cc:propertysheet
    name="PropertySheet" 
    bundleID="samBundle" 
    showJumpLinks="false"/>

<jato:containerView name="RecoveryPointsView">
    <cc:actiontable 
        name="RecoveryPointsTable"
        bundleID="samBundle"
        title="fs.recoverypoints.tabletitle.blank"
        empty="fs.recoverypoints.table.blank"
        selectionType="single"
        selectionJavascript="handleFileSelection(this);"
        showAdvancedSortIcon="true"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="true"
        showSelectionIcons="true"
        showSelectionSortIcon="true"
        maxRows="25"
        page="1"/>
        
<cc:hidden name="FileNames" />
<cc:hidden name="SelectedFile"/>

</jato:containerView>

<cc:hidden name="Messages" />
<cc:hidden name="RetainCheckBoxHelper" />
<cc:hidden name="ServerName" />
<cc:hidden name="Role"/>

</jato:form>
</cc:header>
</jato:useViewBean> 
