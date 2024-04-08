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

// ident	$Id: AddLibrarySelectType.jsp,v 1.12 2008/12/16 00:10:49 am143972 Exp $

--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript"
    src="/samqfsui/js/media/wizards/AddLibrarySelectType.js"></script>
<script type="text/javascript">
    WizardWindow_Wizard.nextClicked = myNextClicked;
</script>

<style>
    td.indent{padding-left:30px; text-align:left}
</style>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<br />
<div style="text-indent:10px">
    <cc:text name="Text"
             defaultValue="AddLibrary.selecttype.instruction"
             bundleID="samBundle"/>
</div>
<br />

<cc:alertinline name="Alert" bundleID="samBundle" />

<br />

<table>
    <tr>
        <td valign="top" rowspan="6">
            <cc:spacer
                name="spacer"
                width="10" />
            <cc:label
                name="LabelType"
                defaultValue="AddLibrary.selecttype.label"
                bundleID="samBundle"/>
        </td>
        <td colspan="2">
            <cc:radiobutton
                name="RadioType1"
                elementId="radioDirect"
                title="AddLibrary.type.direct"
                onClick="changeComponentState(1);"
                styleLevel="3"
                bundleID="samBundle"
                dynamic="true" />
        </td>
    </tr>
    <tr>
        <td colspan="2">
            <cc:radiobutton
                name="RadioType2"
                elementId="radioAcsls"
                title="AddLibrary.type.acsls"
                onClick="changeComponentState(2);"
                styleLevel="3"
                bundleID="samBundle"
                dynamic="true" />
        </td>
    </tr>
    <tr>
        <td class="indent">
            <cc:label
                name="LabelHost"
                defaultValue="AddLibrary.selecttype.label.acslshost"
                bundleID="samBundle"/>
        </td>
        <td>
            <cc:textfield
                name="ACSLSHostName"
                elementId="ACSLSHostName"
                size="20"
                dynamic="true"
                disabled="true" />
        </td>
    </tr>
    <tr>
        <td class="indent">
            <cc:label
                name="LabelPort"
                defaultValue="AddLibrary.selecttype.label.acslsport"
                bundleID="samBundle"/>
        </td>
        <td>
            <cc:textfield
                name="ACSLSPortNumber"
                elementId="ACSLSPortNumber"
                size="20"
                dynamic="true"
                disabled="true" />
        </td>
    </tr>
    <tr>
        <td colspan="2">
            <cc:radiobutton
                name="RadioType3"
                elementId="radioNetwork"
                title="AddLibrary.type.network"
                onClick="changeComponentState(3);"
                styleLevel="3"
                bundleID="samBundle"
                dynamic="true" />
        </td>
    </tr>
    <tr>
        <td class="indent" colspan="2">
            <cc:dropdownmenu
                name="LibraryDriver"
                elementId="LibraryDriver"
                dynamic="true"
                disabled="true"
                bundleID="samBundle"
                type="standard"/>
        </td>
    </tr>
</table>

<cc:hidden name="HiddenMessage" />
</jato:pagelet>
