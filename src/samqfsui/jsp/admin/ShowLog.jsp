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

// ident	$Id: ShowLog.jsp,v 1.16 2008/12/16 00:10:40 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.ShowLogViewBean">
    
<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>   
<script language="javascript" src="/samqfsui/js/admin/ShowLog.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="ShowLog.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="callEveryTime(); initializePopup(this)"
    bundleID="samBundle">

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="ShowLogForm" method="post">

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="ShowLog.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="true"
    showPageButtonsBottom="true">
      
<br />
<table>
    <tr>
        <td rowspan="3">
            <cc:spacer name="Spacer" height="1" width="10" />
        </td>
        <td width="50%">
            <cc:text
                name="StaticText"
                bundleID="samBundle"
                defaultValue="ShowLog.statictext.1" />
            <cc:textfield
                name="TextField"
                defaultValue="50"
                maxLength="5"
                autoSubmit="false"
                size="5" />
            <cc:text
                name="StaticText"
                bundleID="samBundle"
                defaultValue="ShowLog.statictext.2" />
            <cc:spacer
                name="Spacer"
                height="1"
                width="5" />
            <cc:button
                name="RefreshButton"
                bundleID="samBundle"
                title="ShowLog.button.tooltip"
                defaultValue="ShowLog.button.refresh"
                onClick="
                    if (validate()) {
                        refreshPage();
                    }
                    return false;
                " />
        </td>
        <td width="50%">
            <cc:spacer name="Spacer" height="1" width="5" />
            <cc:text
                name="StaticText"
                bundleID="samBundle"
                defaultValue="ShowLog.statictext.3" />
            <cc:dropdownmenu
                name="DropDownMenu"
                bundleID="samBundle"
                type="jump"
                onChange="refreshPage(); return false;" />
        </td>
    </tr>
    <tr>
        <td>&nbsp;</td>
        <td>
            <cc:spacer name="Spacer" height="1" width="5" />
            <cc:text
                name="StaticText"
                bundleID="samBundle"
                defaultValue="ShowLog.statictext.4" />
            <cc:text
                name="LastUpdateText"
                bundleID="samBundle"
                defaultValue="" />
            <br />
            <br />
        </td>
    </tr>
    <tr>
        <td colspan="2">
            <cc:textarea
                name="TextArea"
                bundleID="samBundle"
                defaultValue=""
                readOnly="true"
                cols="120"
                rows="27"
                title="ShowLog.pageTitle"/>
        </td>
    </tr>
</table>

</cc:pagetitle>

<cc:hidden name="ErrorMessage" />
<cc:hidden name="AutoRefreshRate" />

</jato:form>
</body>
</cc:header>
</jato:useViewBean> 
