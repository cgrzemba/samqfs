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

// ident	$Id: FileBrowser.jsp,v 1.14 2008/03/17 14:40:33 am143972 Exp $
--%>
<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.fs.FileBrowserViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript" src="/samqfsui/js/CommonPopup.js"></script>
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>
<script type="text/javascript" src="/samqfsui/js/FilterSettings.js"></script>
<script type="text/javascript" src="/samqfsui/js/fs/FileBrowser.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="fs.filebrowser.pagetitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('41', 'FileBrowser');
        }
        handleToggleComponent();
        handleFileSelection();
        handleUnmountedFS();"
    bundleID="samBundle">

<jato:form name="FileBrowserForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="fs.filebrowser.pagetitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="false"/>

<!-- PropertySheet -->
<cc:propertysheet
    name="PropertySheet" 
    bundleID="samBundle" 
    showJumpLinks="false"/>

<div id="toggleDiv" align="right" style="padding-right:10px">    
    <cc:radiobutton
        name="ModeRadio"
        bundleID="samBundle"
        layout="horizontal"
        elementId="modeRadio"
        styleLevel="2"
        onClick="handleToggleRadio(this)" />
    <cc:dropdownmenu
        name="RecoveryMenu"
        bundleID="samBundle"
        commandChild="RecoveryMenuHref"
        onChange="
            if (this.value == '') return false;"
        type="jump"
        dynamic="true"
        disabled="true" />
</div>

<jato:containerView name="FileBrowserView">
    <cc:actiontable 
        name="FilesTable"
        bundleID="samBundle"
        title="<change to current directory>"
        selectionType="single"
        selectionJavascript="handleFileSelection(this);"
        showAdvancedSortIcon="false"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="true"
        showSelectionIcons="true"
        maxRows="25"
        page="1"/>
        
</jato:containerView>

<cc:hidden name="fileNames"/>
<cc:hidden name="filterReturnValue"/>
<cc:hidden name="filterValue"/>
<cc:hidden name="filterPageTitle"/>
<cc:hidden name="filterPrompt"/>

<!-- To determine if current directory is a part of SAM file system -->
<cc:hidden name="isArchiving" />

<!-- To determine if current directory supports recovery point mode -->
<cc:hidden name="supportRecovery" />

<!-- Keep track of the file system name & Mount Point for view details pop up -->
<cc:hidden name="fsInfo" />

<!-- Recovery Point Path -->
<cc:hidden name="snapPath" />

<!-- Determine if the mount file system msg has to be shown or not -->
<cc:hidden name="MountMessage" />

<!-- Selected File Entry Name -->
<cc:hidden name="SelectedFile" />

<!-- Role of the user, for javascript to determine if drop down items need to
     be disabled -->
<cc:hidden name="Role" />


</jato:form>
</cc:header>
</jato:useViewBean> 
