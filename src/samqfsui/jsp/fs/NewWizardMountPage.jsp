
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

// ident	$Id: NewWizardMountPage.jsp,v 1.11 2008/05/16 19:39:20 am143972 Exp $

--%>
<%@ page language="java" %> 
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">
    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var string = tf.elements["WizardWindow.Wizard.NewWizardMountView.errorOccur"];
        var error = string.value;
        if (error == "exception")
        disabled = true;
        WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
        WizardWindow_Wizard.setPreviousButtonDisabled(disabled, null);
    }
    WizardWindow_Wizard.pageInit = wizardPageInit;

</script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
 baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:legend name="Legend" align="right" marginTop="10px" />

<table>
    <tr>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:label name="fsNameLabel" defaultValue="FSWizard.new.fsnameLabel"
                      bundleID="samBundle" showRequired="true" />
        </td>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:textfield name="fsNameValue"
                          bundleID="samBundle"
                          maxLength="28"
                          size="50"
                          dynamic="true" />
        </td>
    </tr>
    
    <tr>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:label name="Label" defaultValue="FSWizard.new.mountlabel"
                      bundleID="samBundle" showRequired="true"/>
        </td>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:textfield name="mountValue"
                          bundleID="samBundle"
                          size="50"
                          maxLength="127"/>
        </td>
    </tr>
    
    <tr>
        <td></td>
        <td>
            <cc:helpinline type="field">
                <cc:text name="createText" bundleID="samBundle"
                         defaultValue="samqfsui.fs.wizards.new.mountPage.createMountLabel" />
            </cc:helpinline>
        </td>
    </tr>
    
    <tr>
        <td></td>
        <td>
            <cc:checkbox name="bootTimeCheckBox"
                         bundleID="samBundle"
                         styleLevel="3"
                         label="samqfsui.fs.wizards.new.mountPage.mountBootLabel"
            />
        </td>
    </tr>
    
    <tr>
        <td></td>
        <td>
            <cc:checkbox name="mountOptionCheckBox"
                         bundleID="samBundle"
                         styleLevel="3"
                         label="samqfsui.fs.wizards.new.mountPage.mountOptionLabel"
            />
        </td>
    </tr>
    
    <tr>
        <td></td>
        <td>
            <cc:checkbox name="optimizeCheckBox"
                         bundleID="samBundle"
                         styleLevel="3"
                         label="samqfsui.fs.wizards.new.mountPage.optimizeLabel"
            />
        </td>
    </tr>
    
    <tr>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:label name="Label" defaultValue="FSWizard.new.hwmlabel"
                      bundleID="samBundle"/>
        </td>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:textfield name ="hwmValue" bundleID="samBundle"/>
        </td>
    </tr>
    
    <tr>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:label name="Label" defaultValue="FSWizard.new.lwmlabel"
                      bundleID="samBundle"/>
        </td>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:textfield name ="lwmValue" bundleID="samBundle"/>
        </td>
    </tr>
    
    <tr>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:label name="Label" defaultValue="FSWizard.new.tracelabel"
                      bundleID="samBundle"/>
        </td>
        <td valign="center" align="left" rowspan="1" colspan="1">
            <cc:dropdownmenu name ="traceDropDown" bundleID="samBundle" type="standard" />
        </td>
    </tr>
</table>

<cc:hidden name="errorOccur" />

</jato:pagelet>
