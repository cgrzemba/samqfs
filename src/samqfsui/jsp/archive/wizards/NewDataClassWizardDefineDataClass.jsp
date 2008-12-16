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

// ident	$Id: NewDataClassWizardDefineDataClass.jsp,v 1.11 2008/12/16 00:10:43 am143972 Exp $
--%>


<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var error = tf.elements[
           "WizardWindow.Wizard.NewDataClassWizardDefineDataClassView." +
            "errorOccur"].value;
        
        WizardWindow_Wizard.setNextButtonDisabled(error == "true", null);
        WizardWindow_Wizard.setPreviousButtonDisabled(error == "true", null);
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
    <td rowspan="14">
        <cc:spacer name="Spacer" height="1" width="10" />
    </td>
    <td nowrap='nowrap'>
        <cc:label
            name="DataClassNameText"
            defaultValue="archiving.dataclass.name.colon"
            showRequired="true"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:textfield
            name="DataClassName" 
            bundleID="samBundle"
            size="20"
            maxLength="127"/>
    </td>
</tr>
<tr>
    <td nowrap='nowrap'>
        <cc:label
            name="DescriptionText"
            defaultValue="archiving.dataclass.desc"
            bundleID="samBundle" />
    </td>
    <td colspan="3">
        <cc:textfield
            name="Description" 
            bundleID="samBundle"
            size="35"
            maxLength="127"/>
    </td>
</tr>
<tr>
    <td nowrap='nowrap'>
        <cc:label
            name="StartingDirText"
            defaultValue="NewArchivePolWizard.page1.startDirText"
            showRequired="true"
            bundleID="samBundle" />
    </td>
    <td colspan="3">
        <cc:textfield
            name="StartingDir" 
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
        <cc:dropdownmenu
            name="NamePatternDropDown"
            bundleID="samBundle" />          
        <cc:textfield
            name="NamePattern"
            bundleID="samBundle"
            size="20"
            maxLength="127" />
    </td>
</tr>
<tr>
    <td colspan="2">
        <cc:spacer name="Spacer" height="15" width="1" />
    </td>
</tr>
<tr>
    <td rowspan="1" colspan="1">
        <cc:label
            name="OwnerText"
            defaultValue="NewPolicyWizard.defineType.owner"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="1">
        <cc:textfield
            name="Owner"
            maxLength="31"
            bundleID="samBundle" />
        <cc:spacer name="Spacer" height="15" width="1" />
        <cc:label
            name="MinSizeText"
            defaultValue="NewPolicyWizard.defineType.minSize"
            bundleID="samBundle" />
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
            name="Group"
            maxLength="31"
            bundleID="samBundle" />
        <cc:spacer name="Spacer" height="15" width="1" />
        <cc:label
            name="MaxSizeText"
            defaultValue="NewPolicyWizard.defineType.maxSize"
            bundleID="samBundle" />
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
    <td colspan="2">
        <cc:spacer name="Spacer" height="15" width="1" />
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
            size="6"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="AccessAgeDropDown"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td rowspan="1">
        <cc:label
            name="IncludeFileDateText"
            defaultValue="archiving.dataclass.includedata"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="2">
        <cc:textfield
            name="IncludeFileDate"
            size="12"
            maxLength="10"
            bundleID="samBundle" />
        <br/>
        <cc:helpinline type="field">
            <cc:text
                name="HelpText"
                bundleID="samBundle"
                defaultValue="archiving.dataclass.wizard.inlinehelp.date"/>
        </cc:helpinline>
    </td>
</tr>
<tr>
    <td colspan="2">
        <cc:spacer name="Spacer" height="15" width="1" />
    </td>
</tr>
<tr>
    <td rowspan="1">
        <cc:label
            name="SelectPolicyText"
            defaultValue="archiving.dataclass.selectpolicy"
            bundleID="samBundle" />
    </td>
    <td rowspan="1" colspan="2">
        <cc:dropdownmenu
            name="SelectPolicyDropDown"
            bundleID="samBundle" />
    </td>
</tr>
</table>

<cc:hidden name="errorOccur" />

</jato:pagelet>
