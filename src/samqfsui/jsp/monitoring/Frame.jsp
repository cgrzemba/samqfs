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

// ident	$Id: Frame.jsp,v 1.7 2008/05/16 19:39:23 am143972 Exp $
--%>

<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.monitoring.FrameViewBean">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript">

    function getServerName() {
        return document.FrameForm.elements["Frame.ServerName"].value;
    }

    function getTiledNumber(field) {
        var myFieldArray = field.name.split(".");
        return myFieldArray[2].substring(21, myFieldArray[2].length-1);
    }

    function getPathFromHref(field) {
        var tiledNumber = getTiledNumber(field);
        var elementName =
            "Frame.SingleTableView.SingleTableTiledView["
            + tiledNumber + "].LogPath";
        return document.FrameForm.elements[elementName].value;       
    }
    
    function launchStageFilesList(field) {
        var tiledNumber = getTiledNumber(field);
        var elementName =
            "Frame.SingleTableView.SingleTableTiledView["
            + tiledNumber + "].StagingQIDText";
        var jobID = document.FrameForm.elements[elementName].value;
        var param = '&SAMQFS_SHOW_CONTENT=false' +
            '&SAMQFS_SHOW_LINE_CONTROL=false' +
            '&SAMQFS_STAGE_Q_LIST=true' +
            '&SAMQFS_RESOURCE_KEY_NAME=' + jobID;
        launchPopup(
            '/admin/ShowFileContent',
            'content',
            getServerName(),
            SIZE_NORMAL,
            param);
    }

</script>

<cc:header
    pageTitle="" 
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    onLoad="
        parent.dashboard_setSelectedNode(
            document.FrameForm.elements['Frame.PageID'].value)"
    bundleID="samBundle">

<jato:form name="FrameForm" method="post">

<!-- Action Table -->
<jato:containerView name="SingleTableView">
    <br />
    <cc:actiontable
        name="ActionTable"
        bundleID="samBundle"
        title="<change to the appropriate title>"
        selectionType="none"
        showAdvancedSortIcon="false"
        showLowerActions="false"
        showPaginationControls="false"
        showPaginationIcon="false"
        showSelectionIcons="false"/>

</jato:containerView>

<cc:hidden name="PageID" />
<cc:hidden name="ServerName" />

</jato:form>
</cc:header>

</jato:useViewBean> 
