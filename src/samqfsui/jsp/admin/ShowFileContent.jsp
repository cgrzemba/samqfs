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

// ident	$Id: ShowFileContent.jsp,v 1.7 2008/12/16 00:10:40 am143972 Exp $
--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.admin.ShowFileContentViewBean">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript"
    src="/samqfsui/js/admin/ShowFileContent.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="ShowFileContent.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
    onLoad="initializePopup(this);"
    bundleID="samBundle">

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="ShowFileContentForm" method="post">

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="ShowFileContent.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="true"
    showPageButtonsBottom="true">
      
<br />
<table>
    <tr>
        <td align="left">
            <cc:spacer name="Spacer" height="1" width="20" />
            <cc:button
                name="PreviousButton"
                bundleID="samBundle"
                title="ShowFileContent.tooltip.previous"
                titleDisabled="ShowFileContent.tooltip.previous.disable"
                defaultValue="ShowFileContent.button.previous" />
            <cc:spacer name="Spacer" height="1" width="5" />
            <cc:text
                name="LineNumber"
                bundleID="samBundle"
                defaultValue="ShowFileContent.statictext.1" />
            <cc:spacer name="Spacer" height="1" width="5" />
            <cc:button
                name="NextButton"
                bundleID="samBundle"
                title="ShowFileContent.tooltip.next"
                titleDisabled="ShowFileContent.tooltip.next.disable"
                defaultValue="ShowFileContent.button.next" />
        </td>
        <td align="right">
            <cc:text
                name="StaticText"
                bundleID="samBundle"
                defaultValue="ShowFileContent.statictext.2" />
            <cc:textfield
                name="TextField"
                defaultValue=""
                maxLength="7"
                autoSubmit="false"
                size="7" />
            <cc:button
                name="GoButton"
                bundleID="samBundle"
                title="ShowFileContent.tooltip.go"
                titleDisabled="ShowFileContent.tooltip.go.disable"
                defaultValue="ShowFileContent.button.go"
                onClick="return validateTextField();" />
            
    </tr>
    <tr>
        <td colspan="2">
            <cc:spacer name="Spacer" height="1" width="20" />
            <cc:textarea
                name="TextArea"
                bundleID="samBundle"
                defaultValue="ShowFileContent.error"
                readOnly="true"
                cols="120"
                rows="27"
                title="ShowFileContent.pageTitle"/>
        </td>
    </tr>  
</table>

<cc:hidden name="ErrorMessage" />

</cc:pagetitle>

</jato:form>
</body>
</cc:header>
</jato:useViewBean> 
