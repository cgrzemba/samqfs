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

// ident	$Id: ErrorHandle.jsp,v 1.15 2008/12/16 00:10:51 am143972 Exp $
--%>


<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.util.ErrorHandleViewBean">

<script language="javascript">
    
    function handleClick() {
        // Check to see if the button name is Close or Home
        var popup =
            document.ErrorHandleForm.elements["ErrorHandle.isPopUP"].value;
        if (popup == "true") {
            // Close the window
            window.close();
        } else {
            // Reload the frame
            top.location.href = "/samqfsui/util/FrameFormat";
        }
    }

</script>

<cc:header
    pageTitle="ErrorHandle.pagetitle" 
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    bundleID="samBundle">

<!-- Form -->
<jato:form name="ErrorHandleForm" method="post">

<!-- Page title -->
<cc:pagetitle
    name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="ErrorHandle.title"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">
    
<br>

<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />
<br>
<br>
<br>

</cc:pagetitle>

<cc:hidden name="isPopUP" />

</jato:form>
</cc:header>
</jato:useViewBean> 
