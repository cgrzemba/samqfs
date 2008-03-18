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

// ident	$Id: NewCopyMediaParameters.jsp,v 1.11 2008/03/17 14:40:31 am143972 Exp $

--%>
<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

   function setFieldEnable(choice) {
        var formName = "wizWinForm";
        var prefix = "WizardWindow.Wizard.NewCopyMediaParameters.";
	var isDisk = false, isTape = false;
	if (choice == "disk") {
            isDisk = true;
	} else {
            isTape = true;
	}

        ccSetTextFieldDisabled(
            prefix + "DiskVolumeNameTextField", formName, !isDisk);
	ccSetTextFieldDisabled(
            prefix + "DiskDeviceTextField", formName, !isDisk);
    ccSetCheckBoxDisabled(
            prefix + "createPath", formName, !isDisk);
	ccSetTextFieldDisabled(
            prefix + "StartTextField", formName, !isTape);
	ccSetTextFieldDisabled(
            prefix + "EndTextField", formName, !isTape);
	ccSetTextFieldDisabled(
            prefix + "RangeTextField", formName, !isTape);
	ccSetDropDownMenuDisabled(
            prefix + "VSNPoolDropDownMenu", formName, !isTape);
	ccSetDropDownMenuDisabled(
            prefix + "TapeDropDown", formName, !isTape);
        ccSetDropDownMenuDisabled(
            prefix + "rmAttributes", formName, !isTape);
        ccSetCheckBoxDisabled(
            prefix + "rmFS", formName, !isTape);
        ccSetCheckBoxDisabled(
            prefix + "rmPolicy", formName, !isTape);
    }

</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <table>
        <tr>
            <td>
                <cc:label
                    name="ArchiveAgeText"
                    defaultValue="NewPolicyWizard.copymediaparameter.archiveAge"
                    showRequired="true"
                    bundleID="samBundle" />
                <cc:textfield
                    name="ArchiveAgeTextField"
                    bundleID="samBundle" />
                <cc:dropdownmenu
                    name="ArchiveAgeDropDown" 
                    bundleID="samBundle" />
            </td>
        </tr>
        </table>
    </td>
    <td></td>
    <td></td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="1">
        <cc:label
            name="ArchiveMediaTypeText" 
            defaultValue="NewPolicyWizard.copymediaparameter.selectCopyText"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1">
        <cc:helpinline type="field">
            <cc:text
                name="HelpText"
                defaultValue="NewPolicyWizard.copymediaparameter.help.43"
                bundleID="samBundle"
                escape="false" />
        </cc:helpinline>
    </td>
</tr>
<tr>
    <td>
        <table>
        <tr>
            <td valign="top" rowspan="4" colspan="1">
                <cc:radiobutton
                    name="MedTypeRadio2"
                    elementId="tapeRadio"
                    onClick="
                        if (this.id == 'tapeRadio') {
                            setFieldEnable('tape');
                        }"
                    styleLevel="3" 
                    bundleID="samBundle" 
                    dynamic="true" />
            </td>
            <th valign="center" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <cc:text
                    name="TapeText" 
                    defaultValue="NewPolicyWizard.copymediaparameter.tape.desc"
                    bundleID="samBundle" />
            </th>
        </tr>
        <tr>
            <td align ="left">
                <cc:label
                    name="MedTypeText" 
                    defaultValue=
                        "NewPolicyWizard.copymediaparameter.tape.mediaType"
                    bundleID="samBundle" />
            </td>
            <td valign="center" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <cc:dropdownmenu
                    name="TapeDropDown"
                    bundleID="samBundle" 
                    elementId="tapeDropDown"
                    disabled="true"
                    dynamic="true" />
            </td>
        </tr>
        <tr>
            <td valign="center" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <cc:label
                    name="VSNPoolText" 
                    defaultValue="NewPolicyWizard.copymediaparameter.tape.pool"
                    bundleID="samBundle" />
            </td>
            <td valign="center" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <cc:dropdownmenu
                    name="VSNPoolDropDownMenu" 
                    bundleID="samBundle" 
                    elementId="vsnpoolDropDown"
                    disabled="true"
                    dynamic="true" />
            </td>
        </tr>
        <tr>
            <td valign="top" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <cc:label
                    name="SpecifyVSNLabel" 
                    defaultValue=
                        "NewPolicyWizard.copymediaparameter.tape.specify"
                    bundleID="samBundle" />
            </td>
            <td valign="center" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <table>
                <tr>
                    <td valign="center" align="left" rowspan="1"
                        colspan="1" nowrap="true">
                        <cc:label
                            name="StartText" 
                            defaultValue=
                                "NewPolicyWizard.copymediaparameter.tape.start"
                            bundleID="samBundle" />
                        <cc:textfield
                            name="StartTextField"
                            elementId="startTextField"
                            disabled="true"
                            dynamic="true"
                            size="8"
                            bundleID="samBundle" />
                        <cc:label
                            name="EndText" 
                            defaultValue=
                                "NewPolicyWizard.copymediaparameter.tape.end"
                            bundleID="samBundle" />
                        <cc:textfield name="EndTextField" 
                            elementId="endTextField"
                            disabled="true"
                            dynamic="true"
                            size="8"
                            bundleID="samBundle" />
                    </td>
                </tr>
                <tr>
                    <td valign="center" align="left" rowspan="1"
                        colspan="1" nowrap="true">
                        <cc:helpinline type="field">
                            <cc:text
                                name="RangeInstrText"
                                defaultValue=
                                    "NewPolicyWizard.copymediaparameter.tape.inlinehelp.startEnd"
                                bundleID="samBundle" />
                        </cc:helpinline>
                    </td>
                </tr> 
                <tr>
                    <td valign="center" align="left" rowspan="1"
                        colspan="1" nowrap="true">
                        <cc:label
                            name="RangeText" 
                            defaultValue=
                                "NewPolicyWizard.copymediaparameter.tape.range"
                            bundleID="samBundle" />
                        <cc:textfield
                            name="RangeTextField"
                            elementId="rangeTextField"
                            disabled="true"
                            dynamic="true"
                            bundleID="samBundle" />
                    </td>
                </tr>
                <tr>
                    <td valign="center" align="left" rowspan="1"
                        colspan="1" nowrap="true">
                        <cc:helpinline type="field">
                            <cc:text
                                name="RangeInstrText"
                                defaultValue=
                                    "NewPolicyWizard.copymediaparameter.tape.inlinehelp.range"
                                bundleID="samBundle" />
                        </cc:helpinline>
                    </td>
                </tr> 
                </table>
            </td>
        </tr>
        <tr>
            <td>&nbsp;</td>
            <td>
                <cc:label
                    name="ReserveText" 
                    defaultValue=
                        "NewPolicyWizard.copymediaparameter.tape.reserveText"
                    escape="false"
                    bundleID="samBundle" />
            </td>
            <td nowrap='nowrap'>
                <cc:checkbox
                    name="rmPolicy" 
                    bundleID="samBundle"
                    dynamic="true"
                    label="archiving.reservation.method.policy"/>
                <cc:spacer name="Spacer" width="10" height="1" />
                <cc:checkbox
                    name="rmFS"
                    bundleID="samBundle"
                    dynamic="true"
                    label="archiving.reservation.method.fs"/>
                <cc:spacer name="Spacer" width="10" height="1" />
                <cc:dropdownmenu
                    name="rmAttributes" 
                    dynamic="true"
                    bundleID="samBundle" />          
            </td>
        </tr>
        <tr>
            <td colspan='3'>
                <cc:spacer name="Spacer" width="10" height="1" />
            </td>
        </tr>
        <tr>
            <td valign="top" rowspan="3">
                <cc:radiobutton
                    name="MedTypeRadio1"
                    elementId="diskRadio"
                    onClick="
                        if (this.id == 'diskRadio') {
                            setFieldEnable('disk');
                        }"
                    styleLevel="3"
                    bundleID="samBundle"
                    dynamic="true" />
            </td>
            <th valign="center" align="left" rowspan="1">
                <cc:text
                    name="DiskText" 
                    defaultValue="NewPolicyWizard.copymediaparameter.disk.desc"
                    bundleID="samBundle"/>
            </th>
            <td></td>
        </tr>
        <tr>
            <td valign="center" align="left" rowspan="1">
                <cc:label
                    name="DiskVolumeNameText" 
                    defaultValue="NewPolicyWizard.copymediaparameter.disk.name"
                    bundleID="samBundle"/>
            </td>
            <td valign="center" align="left" rowspan="1">
                <cc:textfield
                    name="DiskVolumeNameTextField"
                    elementId="diskVolumeNameTextField"
                    disabled="true"
                    dynamic="true"
                    bundleID="samBundle"
                    maxLength="31" />
            </td>
        </tr>
        <tr>
            <td valign="center" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <cc:label
                    name="DiskDeviceText" 
                    defaultValue="NewPolicyWizard.copymediaparameter.disk.path"
                    bundleID="samBundle"/>
            </td>
            <td valign="center" align="left" rowspan="1"
                colspan="1" nowrap="true">
                <cc:textfield
                    name="DiskDeviceTextField" 
                    elementId="diskDeviceTextField"
                    disabled="true"
                    dynamic="true"
                    bundleID="samBundle"
                    maxLength="127"/>
            </td>
        </tr>
        <tr>
            <td> </td>
            <td>
                <cc:label name="createPathLabel"
                          bundleID="samBundle"
                          elementName="createPath"
                          defaultValue="archiving.diskvsnpath.create.label"/>
            </td>
            <td>
                <cc:checkbox name="createPath"
                             dynamic="true"/>
            </td>
        </tr>
        </table>
    </td>
</tr>

<cc:hidden name="errorOccur" />
</jato:pagelet>
