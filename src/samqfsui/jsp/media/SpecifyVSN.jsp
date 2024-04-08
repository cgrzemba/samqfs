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

// ident	$Id: SpecifyVSN.jsp,v 1.17 2008/12/16 00:10:48 am143972 Exp $
--%>

<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.media.SpecifyVSNViewBean">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/media/SpecifyVSN.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="SpecifyVSN.browserPageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="initializePopup(this)"
    bundleID="samBundle">

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="SpecifyVSNForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="SpecifyVSN.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<br />

<table border="0" width="80%">
<tr>
    <td>
        <cc:radiobutton
            name="RadioButton1"
            elementId="radioStartEnd"
            onClick="
                if (this.id == 'radioStartEnd') {
                    setFieldEnable('startend');
                    startTextField.focus();
		 }" 
            bundleID="samBundle"
            styleLevel="3"
            dynamic="true" />
    </td>
    <td align="left" nowrap>
        <cc:label
            name="StartText"
            defaultValue="SpecifyVSN.radio1"
            bundleID="samBundle" />
    </td>
    <td align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="StartTextField"
            size="6"
            maxLength="6"
            elementId="startTextField"
            disabled="true"
            dynamic="true"
            bundleID="samBundle" />
    </td>
    <td align="left" rowspan="1" colspan="1">
        <cc:label
            name="EndText"
            defaultValue="SpecifyVSN.text1"
            bundleID="samBundle" />
    </td>
    <td align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="EndTextField"
            size="6"
            maxLength="6"
            elementId="endTextField"
            disabled="true"
            dynamic="true"
            bundleID="samBundle" />
    </td> 
</tr>
<tr>
    <td>
        <cc:radiobutton
            name="RadioButton2"
            elementId="radioOne"
            onClick="
                if (this.id == 'radioOne') {
                    setFieldEnable('one');
                    oneVsnTextField.focus();
                }"
            bundleID="samBundle"
            styleLevel="3"
            dynamic="true" />
    </td>
    <td align="left" nowrap>
        <cc:label
            name="OneVSNText"
            defaultValue="SpecifyVSN.radio2"
            bundleID="samBundle" />
    </td>
    <td colspan="3">
        <cc:textfield
            name="OneVSNTextField"
            maxLength="6" 
            elementId="oneVsnTextField"
            disabled="true"
            dynamic="true"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td><cc:spacer name="Spacer" height="1" /></td>
    <td><cc:spacer name="Spacer" height="1" /></td>
    <td align="left" rowspan="1" colspan="3">
        <cc:helpinline type="field">
            <cc:text
                name="OneVSNInstrText"
                defaultValue="SpecifyVSN.text2"
                bundleID="samBundle" />
        </cc:helpinline>
    </td>
</tr>
</table>
</cc:pagetitle>

<cc:hidden name="HiddenMessages" />

</jato:form>
</body>
</cc:header>
</jato:useViewBean>
