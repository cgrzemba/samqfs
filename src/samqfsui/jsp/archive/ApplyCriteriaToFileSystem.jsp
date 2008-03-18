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

// ident	$Id: ApplyCriteriaToFileSystem.jsp,v 1.8 2008/03/17 14:40:30 am143972 Exp $
--%>

<%@page info="ApplyCriteriaToFileSystem" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean 
    className="com.sun.netstorage.samqfs.web.archive.ApplyCriteriaToFileSystemViewBean">
<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/criteriadetails44.js"></script>
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript" src="/samqfsui/js/samqfsui.js"></script>

<!-- header -->
<cc:header pageTitle="archiving.apply.criteria.tofs" 
           copyrightYear="2006" 
           baseName="com.sun.netstorage.samqfs.web.resources.Resources"
           bundleID="samBundleID"
            onLoad="initializePopup(this);">

<!-- masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundleID"/>

<!-- and now the form and its elements --->
<jato:form name="ApplyCriteriaToFileSystemForm" method="POST">

<!-- list of error messages -->
<cc:hidden name="errorMessages"/>

<!-- table row indeces -->
<cc:hidden name="filenames"/>

<!-- page title -->
<cc:pagetitle name="pageTitle" 
    bundleID="samBundleID" 
    pageTitleText="archiving.apply.criteria.tofs"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<cc:alertinline name="alert" bundleID="samBundleID"/>

<cc:actiontable name="filesystemTable"
                bundleID="samBundleID"
                title="archiving.listof.filesystems"
                selectionType="multiple" 
                selectionJavascript="handleApplyCriteriaPopupTableSelection(this)"
                showAdvancedSortIcon="false"
                showLowerActions="false"
                showPaginationControls="false"
                showPaginationIcon="false"
                showSelectionIcons="true" />

<cc:hidden name="selected_filesystems"/>
<cc:hidden name="error_no_fs_selected"/>
</cc:pagetitle>
</jato:form>

</cc:header>
</jato:useViewBean>
