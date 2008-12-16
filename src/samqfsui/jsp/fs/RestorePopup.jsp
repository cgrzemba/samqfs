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

// ident	$Id: RestorePopup.jsp,v 1.5 2008/12/16 00:10:46 am143972 Exp $
--%>
<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.fs.RestorePopupViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript">

    function getFileChooserParams() {
        // get client side params
        var text =
            document.RestorePopupForm.elements["RestorePopup.MountPoint"].value;
        if (text == "") text = "/";
        return "&currentDirParam=" + text;
    }

    function assignFileChooserTextField() {
        var mountPoint =
            document.RestorePopupForm.elements["RestorePopup.MountPoint"].value;
        var fileName   =
            document.RestorePopupForm.elements["RestorePopup.FileName"].value;
        var restoreToPathObj =
            document.RestorePopupForm.elements[
                "RestorePopup.restoreToPathnameValue.browsetextfield"];
        restoreToPathObj.value = mountPoint + "/" + fileName;
    }

    function onCloseRestoreToPathChooser() {
        var restoreToPathObj =
            document.RestorePopupForm.elements[
                "RestorePopup.restoreToPathnameValue.browsetextfield"];
        var fileName   =
            document.RestorePopupForm.elements["RestorePopup.FileName"].value;
        var path = restoreToPathObj.value;
        
        path = path + "/" + fileName;
        restoreToPathObj.value = path;
    }

</script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="fs.restore.popup.browsertitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle"
    onLoad="initializePopup(this); assignFileChooserTextField();">

<!-- masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle"/>

<jato:form name="RestorePopupForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="fs.restore.popup.title"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<!-- PropertySheet -->
<cc:propertysheet
    name="PropertySheet" 
    bundleID="samBundle" 
    showJumpLinks="false"/>

<cc:hidden name="MountPoint" />
<cc:hidden name="FileName" />

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean> 
