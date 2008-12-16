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

// ident	$Id: NewCriteriaMatchCriteria.jsp,v 1.9 2008/12/16 00:10:43 am143972 Exp $

--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var error = tf.elements[
           "WizardWindow.Wizard.NewCriteriaMatchCriteria.errorOccur"].value;
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
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"/>

<cc:legend name="Legend" align="right" marginTop="10px" />

<cc:alertinline name="Alert" bundleID="samBundle" /><br />

<table>
<tr>
    <td>
        <cc:label
            name="StartingDirLabel"
            elementName="StartingDir"
            defaultValue="archiving.startingdir.colon"
            bundleID="samBundle"
            showRequired="true" />
    </td>
    <td>
        <cc:textfield name="StartingDir" bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td>
        <cc:label
            name="NamePatternLabel" 
            elementName="NamePattern"
            defaultValue="archiving.namepattern.colon"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:textfield name="NamePattern" maxLength="127" bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td>&nbsp;</td>
    <td>
        <cc:helpinline type="field">
            <cc:text
                name="NamePatternHelp" 
                defaultValue="ArchivePolDetails.nameHelp"
                escape="false"
                bundleID="samBundle"/>
        </cc:helpinline>    
    </td>
</tr>
</table>

<br />
<br />

<table>
<tr>
    <td>
        <cc:label
            name="OwnerLabel"
            elementName="Owner"
            defaultValue="archiving.owner.colon"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:textfield name="Owner" bundleID="samBundle"/>
    </td>
    
    <td>
        <cc:spacer name="Spacer" height="1" width="40" />
    </td>
    
    <td>
        <cc:label
            name="MinSizeLabel"
            elementName="MinSize"
            defaultValue="archiving.minimumsize.colon"
            bundleID="samBundle"/>
    </td>
    <td nowrap>
        <cc:textfield name="MinSize" bundleID="samBundle" size="5" />
        <cc:dropdownmenu name="MinSizeUnits" bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td>
        <cc:label
            name="GroupLabel" 
            elementName="Group"
            defaultValue="archiving.group.colon"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:textfield name="Group" bundleID="samBundle"/>
    </td>
    <td></td>
    <td>
        <cc:label
            name="MaxSizeLabel"
            elementName="MaxSize"
            defaultValue="archiving.maximumsize.colon"
            bundleID="samBundle"/>
    </td>
    <td nowrap>
        <cc:textfield name="MaxSize" bundleID="samBundle" size="5"/>
        <cc:dropdownmenu name="MaxSizeUnits" bundleID="samBundle"/>
    </td>
</tr>
</table>

<br />

<table>
<tr>
    <td>
        <cc:label
            name="AccessAgeLabel"
            elementName="AccessAge"
            defaultValue="archiving.accessage.colon"
            bundleID="samBundle"/>
    </td>
    <td nowrap>
        <cc:textfield name="AccessAge" bundleID="samBundle"/>
        <cc:dropdownmenu name="AccessAgeUnits" bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td>&nbsp;</td>
    <td>&nbsp;</td>
</tr>

<tr>
    <td>
        <cc:label
            name="StagingLabel" 
            elementName="Staging"
            defaultValue="archiving.staging.colon"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:dropdownmenu name="Staging" bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td>
        <cc:label
            name="ReleasingLabel"
            elementName="Releasing"
            defaultValue="archiving.releasing.colon"
            bundleID="samBundle"/>
    </td>
    <td>
        <cc:dropdownmenu name="Releasing" bundleID="samBundle"/>
    </td>
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
<cc:hidden name="errorOccur" />

</jato:pagelet>
