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

// ident	$Id: VSNSearch.jsp,v 1.14 2008/12/16 00:10:49 am143972 Exp $
--%>

<%@ page info="Index" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.media.VSNSearchViewBean">

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="VSNSearch.browserpagetitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('21', 'VSNSearch');
        }"
    bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/media/VSNSearch.js"></script>

<jato:form name="VSNSearchForm" method="post">

<!-- BreadCrumb component-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="VSNSearch.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="true"
    showPageButtonsBottom="false">

<br />
<br />

<table border="0" width="40%">
<tr>
    <td><cc:spacer name="Spacer" width="15" height="1" /></td>
    <td>
        <cc:label
            name="Label"
            bundleID="samBundle"
            defaultValue="VSNSearch.col1"
            elementName="TextField" />
    </td>
    <td><cc:spacer name="Spacer" width="15" height="1" /></td>
    <td>
        <cc:textfield
            name="TextField"
            size="36"
            maxLength="6"
            bundleID="samBundle"
            autoSubmit="false"
            title="VSNSearch.textfieldtitle" />
    </td>
    <td><cc:spacer name="Spacer" width="5" height="1" /></td>
    <td align="left">
        <cc:button
            name="Button"
            bundleID="samBundle"
            defaultValue="VSNSearch.button"
            onClick="if (!validate()) return false;" />
    </td>
</tr>
<tr>
    <td><cc:spacer name="Spacer" width="15" height="1" /></td>
    <td><cc:spacer name="Spacer" width="15" height="1" /></td>
    <td><cc:spacer name="Spacer" width="15" height="1" /></td>
    <td>
        <cc:helpinline type="field">
            <cc:text
                name="HelpText"
                defaultValue="VSNSearch.helptext"
                bundleID="samBundle" />
        </cc:helpinline>
    </td>
    <td><cc:spacer name="Spacer" width="5" height="1" /></td>
    <td><cc:spacer name="Spacer" width="5" height="1" /></td>
</tr>
</table>

<cc:hidden name="ErrorMessages" />

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
