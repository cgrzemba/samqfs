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

// ident	$Id: EditDiskVSNFlags.jsp,v 1.8 2008/03/17 14:40:31 am143972 Exp $
--%>

<%@page info="EditDiskVSNFlags" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>
<%@taglib uri="samqfs.tld" prefix="samqfs"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.archive.EditDiskVSNFlagsViewBean">

<!-- include helper javascript -->
<script type="text/javascript" src="/samqfsui/js/archive/diskvsn.js"> </script>
<script type="text/javascript" src="/samqfsui/js/popuphelper.js"></script>
<script type="text/javascript" src="/samqfsui/js/archive/diskvsn.js"></script>

<!-- header -->
<cc:header pageTitle="archiving.diskvsn.editflags.pagetitle"
           copyrightYear="2006"
           baseName="com.sun.netstorage.samqfs.web.resources.Resources"
           bundleID="samBundle"
           onLoad="initializePopup(this);">

<!-- masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle"/>

<!-- the form and its contents -->
<jato:form name="EditDiskVSNFlagsForm" method="POST"> 

<!-- feedback alert -->
<cc:alertinline name="Alert" bundleID="samBundle"/>

<!-- page title -->
<cc:pagetitle name="PageTitle"
              bundleID="samBundle"
              pageTitleText="archiving.diskvsn.editflags.pagetitle"
              showPageTitleSeparator="true"
              showPageButtonsTop="false"
              showPageButtonsBottom="true">

<!--- now the property sheet -->
<cc:propertysheet name="PropertySheet"
                  bundleID="samBundle"
                  addJavaScript="true"
                  showJumpLinks="false"/>

</cc:pagetitle>

<cc:hidden name="selected_vsn_name"/>
</jato:form>
</cc:header>
</jato:useViewBean>
