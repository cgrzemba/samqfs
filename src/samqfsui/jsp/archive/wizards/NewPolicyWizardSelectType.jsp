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

// ident	$Id: NewPolicyWizardSelectType.jsp,v 1.10 2008/03/17 14:40:32 am143972 Exp $
--%>


<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var error = tf.elements[
           "WizardWindow.Wizard.NewPolicyWizardSelectTypeView.errorOccur"].value;
        if (error == "exception") {
            disabled = true;
        }

        WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
   }

   WizardWindow_Wizard.pageInit = wizardPageInit;

</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:legend name="Legend" align="right" marginTop="10px" />


<table>
<tr>
    <td>
        <cc:spacer name="Spacer" width="10" height="1" />
    </td>
    <td valign="top" nowrap='nowrap'>
        <cc:label
            name="Label"
            showRequired="true"
            defaultValue="NewPolicyWizard.defineType.createTo"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:spacer name="Spacer" width="10" height="1" />
    </td>
    <td>
        <cc:dropdownmenu
            name="ArchiveMenu"
            bundleID="samBundle"
            type="standard"/>
    </td>
</tr>
</table>


<br />

<table>
<tr>
    <td rowspan="3">
        <cc:spacer name="Spacer" height="1" width="10" />
    </td>
    <td nowrap='nowrap'>
        <cc:label
            name="StartingDir"
            defaultValue="NewPolicyWizard.defineType.startingDir"
            showRequired="true"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield
            name="StartingDirTextField" 
            bundleID="samBundle"
            size="35"
            maxLength="127"/>
    </td>
</tr>

<tr>
    <td nowrap='nowrap'>
        <cc:label
            name="NamePatternText"
            defaultValue="NewPolicyWizard.defineType.namePattern"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield
            name="NamePatternTextField"
            bundleID="samBundle"
            size="35"
            maxLength="127" />
    </td>
</tr>

<tr>
    <td>&nbsp;</td>
    <td>
        <cc:helpinline type="field">
            <cc:text
                name="HelpText"
                bundleID="samBundle"
                escape="false"
                defaultValue="ArchivePolDetails.nameHelp" />
        </cc:helpinline>
    </td>
</tr>
</table>

<cc:spacer name="Spacer" height="5" width="1" />

<table style="padding-left:10px">
<tr>
    <td rowspan="1" colspan="1">
        <cc:label
            name="OwnerText"
            defaultValue="NewPolicyWizard.defineType.owner"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="1">
        <cc:textfield
            name="OwnerTextField"
            maxLength="31"
            bundleID="samBundle" />
    </td>
    <td><cc:spacer name="Spacer" height="1" width="10" /></td>
    <td rowspan="1" colspan="1">
        <cc:label
            name="MinSizeText"
            defaultValue="NewPolicyWizard.defineType.minSize"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="1">
        <cc:textfield
            name="MinSizeTextField"
            size="6"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="MinSizeDropDown"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td rowspan="1" colspan="1">
        <cc:label
            name="GroupText"
            defaultValue="NewPolicyWizard.defineType.group"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="1">
        <cc:textfield
            name="GroupTextField"
            maxLength="31"
            bundleID="samBundle" />
    </td>
    <td><cc:spacer name="Spacer" height="1" width="10" /></td>
    <td rowspan="1" colspan="1">
        <cc:label
            name="MaxSizeText"
            defaultValue="NewPolicyWizard.defineType.maxSize"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="1">
        <cc:textfield
            name="MaxSizeTextField"
            size="6"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="MaxSizeDropDown"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td rowspan="1" colspan="1">
        <cc:label
            name="AccessAgeText"
            defaultValue="NewPolicyWizard.defineType.accessAge"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="1">
        <cc:textfield
            name="AccessAgeTextField"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="1">
        <cc:dropdownmenu
            name="AccessAgeDropDown"
            bundleID="samBundle" />
    </td>
    <td></td>
    <td></td>
</tr>
<tr>
    <td rowspan="1" colspan="1">
        <cc:label
            name="ApplyToFSText"
            showRequired="true"
            defaultValue="NewPolicyWizard.defineType.applyToFS"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="5">
        <cc:selectablelist
            name="SelectionListApplyToFS"
            bundleID="samBundle"
            multiple="true"
            size="4"/>
    </td>
</tr>
</table>

<cc:hidden name="noArchiveExists" />
<cc:hidden name="errorOccur" />

</jato:pagelet>
