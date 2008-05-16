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

// ident	$Id: AddServer.jsp,v 1.17 2008/05/16 19:39:24 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.server.AddServerViewBean">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/server/AddServer.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="AddServer.browserPageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        initializePopup(this);
        document.AddServerForm.elements['AddServer.nameValue'].focus();"
    bundleID="samBundle">


<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="AddServerForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="AddServer.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<cc:legend name="Legend" align="right" marginTop="10px" />

<table width="70%">
<tr>
    <td><cc:spacer name="Spacer1" width="10" /></td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label name="Label"
            styleLevel="2"
            defaultValue="AddServer.psheet1"
            showRequired="true"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield name="nameValue"
            bundleID="samBundle"
            autoSubmit="false" 
            maxLength="31" />
    </td>
</tr>
</table>

<cc:spacer name="spacer" height="20" width="1"/>

<cc:alertinline name="Alert2" bundleID="samBundle" />

<!-- the pagelets for the cluster table if necessary -->
<cc:includepagelet name="AddClusterView"/>

</cc:pagetitle>

<cc:hidden name="HiddenMessages" />
<cc:hidden name="SelectedNodes" />


</jato:form>
</cc:header>
</jato:useViewBean> 
