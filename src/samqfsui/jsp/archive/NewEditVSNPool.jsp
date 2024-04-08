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

// ident	$Id: NewEditVSNPool.jsp,v 1.25 2008/12/16 00:10:42 am143972 Exp $
--%>

<%@ page info="NewEditVSNPool" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.NewEditVSNPoolViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="NewEditVSNPool.browserPageTitle"
    copyrightYear="2007"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle"
    onLoad="if (opener != null) {
                initializePopup(this);
                pageInit();
            }">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/archive/vsnpools.js"></script>

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="NewEditVSNPoolForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle"/>

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText=""
    pageTitleHelpMessage=""
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

    <br />

    <!-- the assign media pagelets -->
    <cc:includepagelet name="AssignMediaView"/>

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean>
