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

// ident	$Id: ChangeDirectory.jsp,v 1.9 2008/12/16 00:10:51 am143972 Exp $
--%>
<%@ page language="java" %>
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>


<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.util.ChangeDirectoryViewBean">

<script language="javascript">

    function doSubmit() {
        // Get forms
        var thisFormObj   = document.ChangeDirForm;
        var parentFormName = thisFormObj.elements["ChangeDirectory.parentFormName"].value;
        var parentFormObj   = window.opener.document.forms[parentFormName];

        // Save directory in parent form.  
        var newDir = thisFormObj.elements["ChangeDirectory.newDirValue"].value;
        var parentReturnObjName = thisFormObj.elements["ChangeDirectory.parentReturnValueObjName"].value;
        var parentReturnObj = parentFormObj.elements[parentReturnObjName];
        parentReturnObj.value = newDir;

        // Refresh parent page.
        var submitCmd = thisFormObj.elements["ChangeDirectory.parentSubmitCmd"].value;
        parentFormObj.action = parentFormObj.action + "?" + submitCmd;
        parentFormObj.submit();
    } 

</script>


<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="ChangeDirectory.pageTitle" 
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    bundleID="samBundle"
    onLoad="document.ChangeDirForm.elements['ChangeDirectory.newDirValue'].focus();">

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="ChangeDirForm" method="post">

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<br />
<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:propertysheet name="PropertySheet" 
              bundleID="samBundle" 
              showJumpLinks="false"/>


</cc:pagetitle>

<cc:hidden name="parentFormName" />
<cc:hidden name="parentReturnValueObjName" />
<cc:hidden name="parentSubmitCmd" />

</jato:form>
</body>
</cc:header>
</jato:useViewBean> 
