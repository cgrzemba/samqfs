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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: NewWizardFSNamePage.jsp,v 1.26 2008/05/16 19:39:20 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript"
    src="/samqfsui/js/fs/wizards/NewWizardFSName.js">
</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<cc:alertinline name="Alert" bundleID="samBundle" />
<br />

<table border="0">
<tr>
    <td valign="top">
        <cc:label
            name="label"
            bundleID="samBundle"
            defaultValue="FSWizard.new.fstype"
            showRequired="true" />
    </td>

    <td valign="center" align="left" rowspan="1" colspan="1">
        <table border="0">
        <tr>
            <td align="left" colspan="2">
                <cc:radiobutton
                    name="ufsTypeSelect"
                    bundleID="samBundle"
                    styleLevel="3"
                    dynamic="true"
                    onClick="ufsSelected()" />
            </td>
        </tr>
        <tr>
            <td align="left" colspan="1">
                <cc:radiobutton
                    name="qfsTypeSelect"
                    bundleID="samBundle"
                    styleLevel="3"
                    dynamic="true"
                    onClick="qfsSelected()" />
            </td>
            <td align="left" colspan="1">
                <cc:checkbox name="hafs"
                         bundleID="samBundle"
                             label="FSWizard.new.fstype.hafs"
                             dynamic="true"
                             onClick="hafsSelected(this);"/>
            </td>
        </tr>

        <tr>
            <td>
                <cc:spacer name="Spacer" width="10" />
            </td>
            <td>
                <table border="0">
                <tr>
                    <td colspan="2">
                        <cc:checkbox
                            name="archiveCheck"
                            bundleID="samBundle"
                            styleLevel="3"
                            dynamic="true"
                            label="FSWizard.new.archiveFeature"
                            onClick="onSelectArchiveCheckBox()" />                                  
                    </td>
                </tr>
                <tr>
                    <td colspan="2">
                        <cc:checkbox
                            name="sharedCheck"
                            bundleID="samBundle"
                            styleLevel="3"
                            dynamic="true"
                            label="FSWizard.new.qfsShared"
                            onClick="onSelectSharedCheckBox(this)" />
                    </td>
                <tr>
                </table>
            </td>
        </tr>      
        </table>
    </td>
</tr>
</table>

<br />

<div id="toggleDiv" style="padding-left:10px">
    <cc:button
        name="ToggleButton"
        bundleID="samBundle" 
        defaultValue="common.button.advance.show"
        onClick="onButtonToggle(this.value);return false;"
        dynamic="true"
        type="primary"/>
</div>

<div id="advancedDiv" style="display:none; padding-left:20px">
    <br />
    <cc:radiobutton
        name="metaLocationSelect"
        bundleID="samBundle"
        dynamic="true"
        styleLevel="3"
        onClick="onClickMetaDataLocation(this.value)" />

        <table border="0" style="padding-left:30px">
        <tr id="separateDiv1" style="display:none">
            <td>
                <cc:label name="label" bundleID="samBundle"
                          defaultValue="FSWizard.new.qfsselection" />
            </td>

            <td valign="center" align="left" rowspan="1" colspan="1">
                <cc:radiobutton name="allocSelect"
                                bundleID="samBundle"
                                styleLevel="3"
                                dynamic="true"
                                onClick="onClickDataAllocationMethod(this.value)"/>
            </td>
        </tr>
        
        <tr id="separateDiv2" style="display:none">
            <td></td>
            <td colspan="2" valign="top">
                <cc:spacer name="Spacer" height="10" width="60" />
                <cc:label name="label"
                          bundleID="samBundle"
                          defaultValue="FSWizard.new.numOfStripedGroup"
                          elementName="numOfStripedGroupTextField" />
                
                <cc:textfield name="numOfStripedGroupTextField"
                              bundleID="samBundle"
                              dynamic="true"
                              maxLength="3"
                              size="3"/>
            </td>
            </td>
        </tr>
        
        <tr>
            <td valign="center" align="left" rowspan="1" colspan="1">
                <cc:label name="label"
                          defaultValue="FSWizard.new.daulabel"
                          bundleID="samBundle" />
            </td>
            <td>
                <cc:dropdownmenu name="DAUDropDown"
                                 bundleID="samBundle"
                                 dynamic="true"
                                 type="standard"
                                 onChange="onSelectDAUDropDown(this)"/>
                <cc:label name="label"
                          defaultValue="common.unit.size.kb"
                          bundleID="samBundle" />
            </td>
        </tr>
        
        <tr>
            <td>&nbsp;</td>
            <td>
                <cc:helpinline type="field">
                    <cc:text name="DAUSizeHelp" bundleID="samBundle"
                             defaultValue="FSWizard.new.fsdauhelp" />
                </cc:helpinline>
            </td>
        </tr>
        
        <tr id="separateDiv3" style="display:none">
            <td>&nbsp;</td>
            <td valign="center" align="left" rowspan="1" colspan="2">
                <cc:textfield name="DAUSizeField"
                              bundleID="samBundle"
                              maxLength="5"
                              size="5"
                              dynamic="true"
                              onBlur="onBlurDAUSizeField(this);"/>
                <cc:dropdownmenu name="DAUSizeDropDown"
                                 bundleID="samBundle"
                                 dynamic="true"
                                 onChange="onChangeDAUSizeDropDown(this)"
                />
            </td>
        </tr>
        
        <tr id="separateDiv4" style="display:none">
            <td>&nbsp;</td>
            <td align="left" rowspan="1" colspan="2">
                <cc:helpinline type="field">
                    <cc:text name="DAUSizeHelp"
                             bundleID="samBundle"
                             escape="true"
                             defaultValue="FSWizard.new.newDAUHelp" />
                </cc:helpinline>
            </td>
        </tr>
        
        <tr>
            <td valign="center" align="left" rowspan="1" colspan="1">
                <cc:label name="label"
                          defaultValue="FSWizard.new.stripelabel"
                          bundleID="samBundle"/>
            </td>
            <td valign="center" align="left" rowspan="1" colspan="1">
                <cc:textfield name="stripeValue"
                              dynamic="true"
                              bundleID="samBundle"/>
            </td>
        </tr>
        <tr>
            <td>&nbsp;</td>
            <td align="left" rowspan="1" colspan="2">
                <cc:helpinline type="field">
                    <cc:text name="DAUSizeHelp"
                             bundleID="samBundle"
                             escape="true"
                             defaultValue="FSWizard.new.stripewidthhelp" />
                </cc:helpinline>
            </td>
        </tr>
        </table>
</div>

<cc:hidden name="HiddenMessage" />
<cc:hidden name="ToggleButtonLabels" />
<cc:hidden name="isAdvancedMode" />
</jato:pagelet>
