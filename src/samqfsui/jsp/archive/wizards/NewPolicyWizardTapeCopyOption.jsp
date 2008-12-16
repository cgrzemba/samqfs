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

// ident	$Id: NewPolicyWizardTapeCopyOption.jsp,v 1.12 2008/12/16 00:10:44 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var error = tf.elements[
            "WizardWindow.Wizard.NewPolicyWizardTapeCopyOptionView.errorOccur"].value;

        if (error == "exception")
            disabled = true;

        WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
        WizardWindow_Wizard.setPreviousButtonDisabled(disabled, null);
   }

   WizardWindow_Wizard.pageInit = wizardPageInit;

</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />


<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="NewPolicyWizard.tapecopyoption.section.title1"
        showPageTitleSeparator="false"
        showPageButtonsTop="false"
        showPageButtonsBottom="false" />

<br />

<table>
<tr>
    <td><cc:spacer name="Spacer" width="10" height="1" /></td>
    <td valign="center" align="left" rowspan="1" colspan="2">
        <cc:label
            name="DrivesText"
            defaultValue="NewPolicyWizard.tapecopyoption.drives"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="DrivesTextField"
            size="6"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td></td>
    <td><cc:spacer name="Spacer" height="1" width="10" /></td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="DrivesMaxText"
            defaultValue="NewPolicyWizard.tapecopyoption.driveMax"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="DrivesMaxTextField"
            size="6"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="DrivesMaxSizeDropDown"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td></td>
    <td><cc:spacer name="Spacer" height="1" width="10" /></td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="DrivesMinText"
            defaultValue="NewPolicyWizard.tapecopyoption.driveMin"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" rowspan="1" colspan="4">
        <cc:textfield
            name="DrivesMinTextField"
            size="6"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="DrivesMinSizeDropDown"
            bundleID="samBundle" />
    </td>
</tr>
</table>


<table style="padding-left:10px">
<tr>
    <td valign="center" align="left" rowspan="1" colspan="2">
        <cc:label
            name="OfflineCopyText"
            defaultValue="NewPolicyWizard.tapecopyoption.offlineCopy"
            bundleID="samBundle" />
        <cc:dropdownmenu
            name="OfflineCopyDropDown"
            bundleID="samBundle" />
    </td>
</tr>
</table>

<br />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
        pageTitleText="NewPolicyWizard.tapecopyoption.section.title2"
        pageTitleHelpMessage="archiving.copyoption.manageworkqueue.text"
        showPageTitleSeparator="false"
        showPageButtonsTop="false"
        showPageButtonsBottom="false" />

<br />

<table>
<tr>
    <td><cc:spacer name="Spacer" width="10" height="1" /></td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="StartAgeTextField"
            size="6"
            bundleID="samBundle" />
    </td>
    <td>
        <cc:dropdownmenu
            name="StartAgeDropDown"
            bundleID="samBundle" />
    </td>
    <td nowrap='nowrap'>
        <cc:label
            name="StartAgeText"
            defaultValue="NewPolicyWizard.tapecopyoption.startAge"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td></td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="StartCountTextField"
            size="6"
            bundleID="samBundle" />
    </td>
    <td valign="center" align="left" nowrap='nowrap' colspan="2">
        <cc:label
            name="StartCountText"
            defaultValue="NewPolicyWizard.tapecopyoption.startCount"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td></td>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:textfield
            name="StartSizeTextField"
            size="6"
            bundleID="samBundle" />
    </td>
    <td colspan="2">
        <cc:dropdownmenu
            name="StartSizeDropDown"
            bundleID="samBundle" />
        <cc:label
            name="StartSizeText"
            defaultValue="NewPolicyWizard.tapecopyoption.startSize"
            bundleID="samBundle" />
</tr>
</table>

<cc:hidden name="errorOccur" />
</jato:pagelet>
