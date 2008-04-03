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

// ident	$Id: CopyVSNExtension.jsp,v 1.1 2008/04/03 02:21:38 ronaldso Exp $
--%>

<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean
    className="com.sun.netstorage.samqfs.web.archive.CopyVSNExtensionViewBean">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<script language="javascript">
    
    function getForm() {
        return document.forms[0];
    }

    function getPageName() {
        var strArray = getForm().action.split("/");
        return strArray[strArray.length - 1];
    }

    function changeComponentState(flag) {
        var boxName = getPageName() + ".ExistingPool";
        ccSetSelectableListDisabled(boxName, getForm().name, !flag);
    }

</script>

<style>
    td.indent1{padding-left:30px; text-align:left}
    td.indent2{padding-left:60px; padding-top:10px;text-align:left}
</style>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="CopyVSNs.extension.pagetitle"
    copyrightYear="2008"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="initializePopup(this)"
    bundleID="samBundle">

<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="CopyVSNExtensionForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle"
    bundleID="samBundle"
    pageTitleText="CopyVSNs.extension.pagetitle"
    pageTitleHelpMessage=""
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<br />

<table>
    <tr>
        <td class="indent1" colspan="2">
            <cc:radiobutton
                name="RadioType1"
                elementId="Radio_CreateExpression"
                title="CopyVSNs.extension.choice.createexp"
                onClick="changeComponentState(false);"
                styleLevel="3"
                bundleID="samBundle"
                dynamic="true" />
        </td>
    </tr>
    <tr>
        <td class="indent1" colspan="2">
            <cc:radiobutton
                name="RadioType2"
                elementId="radioAcsls"
                title="CopyVSNs.extension.choice.createpool"
                onClick="changeComponentState(false);"
                styleLevel="3"
                bundleID="samBundle"
                dynamic="true" />
        </td>
    </tr>
    <tr>
        <td class="indent1" colspan="2">
            <cc:radiobutton
                name="RadioType3"
                elementId="radioNetwork"
                title="CopyVSNs.extension.choice.useexistingpool"
                onClick="changeComponentState(true);"
                styleLevel="3"
                bundleID="samBundle"
                dynamic="true" />
        </td>
    </tr>
    <tr>
        <td valign="top" class="indent2">
            <cc:label
                name="Label"
                bundleID="samBundle"
                defaultValue="CopyVSNs.extension.label"/>
        </td>
        <td>
            <cc:selectablelist
                name="ExistingPool"
                multiple="true"
                dynamic="true"
                disabled="true"
                bundleID="samBundle"/>
        </td>
    </tr>
    <tr>
        <td></td>
        <td>
            <cc:helpinline type="field">
                <cc:text
                    name="HelpText"
                    defaultValue="CopyVSNs.extension.help"
                    bundleID="samBundle" />
            </cc:helpinline>
        </td>
    </tr>
    </table>

<br />

</cc:pagetitle>

<cc:hidden name="MediaType"/>
<cc:hidden name="ServerName"/>
<cc:hidden name="PolicyName"/>
<cc:hidden name="CopyNumber"/>

</jato:form>
</cc:header>
</jato:useViewBean> 
