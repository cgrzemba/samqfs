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

// ident	$Id: FSDumpNowPopUp.jsp,v 1.20 2008/12/16 00:10:45 am143972 Exp $
--%>
<%@ page info="Index" language="java" %>
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.FSDumpNowPopUpViewBean">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/fs/FSDumpNowPopUp.js"></script>

<cc:header pageTitle="FSDumpNow.pageTitle" 
          copyrightYear="2006"
          baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
          bundleID="samBundle"
          onLoad="initializePopup(this);">

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<!-- Form -->
<jato:form name="FSDumpNowForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<br />
<!-- Page title -->
<cc:pagetitle
    name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="FSDumpNow.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<!-- PropertySheet -->
<cc:propertysheet name="PropertySheet" 
    bundleID="samBundle" 
    addJavaScript="true"
    showJumpLinks="true" />

<cc:hidden name="LocationEmptyError" />
<cc:hidden name="InvalidPathError" />

</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean> 

