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

// ident	$Id: NFSShareEditPagelet.jsp,v 1.4 2008/03/17 14:40:33 am143972 Exp $ 
--%>

<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"/>
    
<cc:pagetitle
    name="EditPageTitle" 
    bundleID="samBundle"
    pageTitleText="filesystem.nfs.options.pageTitle"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

<br />

<table width="80%">
<!-- read only option and access list -->
<tr>
    <td rowspan="6"><cc:spacer name="Spacer1" width="10" /></td>
    <td>
        <cc:label
            name="roLabel"
            styleLevel="2"
            defaultValue = "fileSystem.nfs.options.label.readOnly" 
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:checkbox
            name="roCheckBox"
            styleLevel="3"
            onClick="javascript: if (this.checked) {
                ccSetTextFieldDisabled(
                    'NFSDetails.NFSShareEditView.roAccessValue',
                    'NFSDetailsForm',
                    false);
                } else {
                    ccSetTextFieldDisabled(
                    'NFSDetails.NFSShareEditView.roAccessValue',
                    'NFSDetailsForm',
                    true);
                }"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:label
            name="roAccessLabel"
            styleLevel="2"
            defaultValue = "fileSystem.nfs.options.label.accessList"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:textfield
            name="roAccessValue"
            maxLength="255"
            dynamic="true"
            disabled="true"
            autoSubmit="false"
            bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td colspan="2"><cc:spacer name="Spacer1" width="10" /></td>
    <td valign="center" align="left" colspan="2">
        <cc:helpinline type="field">
            <cc:text
		name="roAccessHelpText"
		defaultValue="fileSystem.nfs.options.help.roAccess"
		bundleID="samBundle"
		escape="false" />
	</cc:helpinline>
    </td>
</tr>

<tr>
    <td>
        <cc:label
            name="rwLabel"
            styleLevel="2"
            defaultValue = "fileSystem.nfs.options.label.readWrite" 
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:checkbox
            name="rwCheckBox"
            styleLevel="3"
            onClick="javascript: if (this.checked) {
                ccSetTextFieldDisabled(
                    'NFSDetails.NFSShareEditView.rwAccessValue',
                    'NFSDetailsForm',
                    false);
                } else {
                    ccSetTextFieldDisabled(
                    'NFSDetails.NFSShareEditView.rwAccessValue',
                    'NFSDetailsForm',
                    true);
                }"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:label
            name="rwAccessLabel"
            styleLevel="2"
            defaultValue = "fileSystem.nfs.options.label.accessList" 
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:textfield
            name="rwAccessValue"
            maxLength="255"
            dynamic="true"
            disabled="true"
            autoSubmit="false"
            bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td colspan="2"><cc:spacer name="Spacer1" width="10" /></td>
    <td align="left" colspan="2">
        <cc:helpinline type="field">
            <cc:text
		name="rwAccessHelpText"
		defaultValue="fileSystem.nfs.options.help.rwAccess"
		bundleID="samBundle"
		escape="false" />
	</cc:helpinline>
    </td>
</tr>

<tr>
    <td>
        <cc:label
            name="rootLabel"
            styleLevel="2"
            defaultValue = "fileSystem.nfs.options.label.root" 
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:checkbox
            name="rootCheckBox"
            styleLevel="3"
            onClick="javascript: if (this.checked) {
                ccSetTextFieldDisabled(
                    'NFSDetails.NFSShareEditView.rootAccessValue',
                    'NFSDetailsForm',
                    false);
                } else {
                    ccSetTextFieldDisabled(
                    'NFSDetails.NFSShareEditView.rootAccessValue',
                    'NFSDetailsForm',
                    true);
                }"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:label
            name="rootAccessLabel"
            styleLevel="2"
            defaultValue = "fileSystem.nfs.options.label.accessList" 
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:textfield
            name="rootAccessValue"
            maxLength="255"
            dynamic="true"
            disabled="true"
            autoSubmit="false"
            bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td colspan="2"><cc:spacer name="Spacer1" width="10" /></td>
    <td align="left" colspan="2">
        <cc:helpinline type="field">
            <cc:text
		name="rootAccessHelpText"
		defaultValue="fileSystem.nfs.options.help.rootAccess"
		bundleID="samBundle"
		escape="false" />
	</cc:helpinline>
    </td>
</tr>
</table>

<br/><br/>

<table>
<tr>
    <td>
        <cc:spacer name="Spacer1" width="20" />
        <cc:helpinline type="field">
            <cc:text
		name="accessDelimitHelpText"
		defaultValue="fileSystem.nfs.options.help.delimitAccessEntry"
		bundleID="samBundle"
		escape="false" />
	</cc:helpinline>
    </td>
</tr>
</table>

<cc:hidden name="ConfirmMessage" />

</cc:pagetitle>
</jato:pagelet>
