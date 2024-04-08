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

// ident	$Id: CopyMediaParameters.jsp,v 1.13 2008/12/16 00:10:43 am143972 Exp $

--%>
<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var error = tf.elements[
            "WizardWindow.Wizard.CopyMediaParametersView.errorOccur"].value;

        if (error == "exception") {
            disabled = true;
        }

        WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
        WizardWindow_Wizard.setPreviousButtonDisabled(disabled, null);
   }  
</script>

<script type="text/javascript" src="/samqfsui/js/archive/policywizards.js">
</script>

<jato:pagelet>
<cc:i18nbundle id="samBundle"
              baseName="com.sun.netstorage.samqfs.web.resources.Resources"/>

<cc:legend name="Legend" align="right" marginTop="10px" />

<style>
    td.indent10px{padding-left:10px;text-align:left}
</style>

<tr>
    <td valign="center" align="center" rowspan="1" colspan="2">
        <cc:alertinline name="Alert" bundleID="samBundle"/>
    </td>
</tr>

<tr>
    <td style="text-align:left">
        <cc:label name="archiveAgeLabel"
                  bundleID="samBundle"
                  elementName="archiveAge"
                  showRequired="true"
                  defaultValue="archiving.archiveage.label"/>
    </td>
    <td nowrap>
        <cc:textfield name="archiveAge"/>
        <cc:dropdownmenu name="archiveAgeUnit"
                         bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td valign="center" align="left" rowspan="1" colspan="2">
        <cc:label
            name="ArchiveMediaTypeText"
            defaultValue="NewPolicyWizard.copymediaparameter.selectCopyText"
            bundleID="samBundle" />
    </td>
</tr>

<tr>
    <td valign="center" align="left" rowspan="1" colspan="2">
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
    <td class="indent10px" style="width:50%">
        <cc:label name="mediaTypeLabel"
                  bundleID="samBundle"
                  elementName="mediaType"
                  defaultValue="archiving.mediatype.label"/>

    </td>
    <td nowrap>
        <cc:dropdownmenu name="mediaType"
                         bundleID="samBundle"
                         onChange="handleMediaTypeChange(this);"
                         dynamic="true" />

        <cc:spacer name="spacer" width="20"/>
        <cc:label name="poolNameLabel"
                  bundleID="samBundle"
                  elementName="poolName"
                  defaultValue="NewPolicyWizard.copymediaparameter.tape.pool"/>

        <cc:dropdownmenu name="poolName"
                         bundleID="samBundle"
                         dynamic="true" />
    </td>
</tr>

<tr>
    <td class="indent10px" valign="top">
        <cc:label name="rangeLabel"
                  elementName="from"
                  bundleID="samBundle"
                  defaultValue="NewPolicyWizard.copymediaparameter.tape.specify"/>
    </td>
    <td nowrap>
        <cc:textfield name="from"
                      bundleID="samBundle"
                      dynamic="true" />

        <span style="padding-left:10px">
        <cc:label name="toLabel"
                  bundleID="samBundle"
                  elementName="to"
                  defaultValue="archiving.to.label"/>
        </span>
        <cc:textfield name="to"
                      bundleID="samBundle"
                      dynamic="true" />
        <cc:helpinline type="field">
            <cc:text name="fromToInlineHelp"
                     defaultValue="NewPolicyWizard.copymediaparameter.tape.inlinehelp.startEnd"
                     bundleID="samBundle"/>
        </cc:helpinline>
    </td>
</tr>

<tr>
    <td></td>
    <td>
        <cc:textfield name="list"
                      bundleID="samBundle"
                      dynamic="true"
                      size="48"/>
        <cc:helpinline type="field">
            <cc:text name="listInlineHelp"
                     defaultValue="NewPolicyWizard.copymediaparameter.tape.inlinehelp.range"
                     bundleID="samBundle"/>
        </cc:helpinline>
    </td>
</tr>

<tr>
    <td class="indent10px">
        <cc:label name="rmLabel"
                  bundleID="samBundle"
                  escape="false"
                  defaultValue="NewPolicyWizard.copymediaparameter.tape.reserveText"/>
    </td>
    <td nowrap>
        <cc:checkbox name="rmPolicy"
                     bundleID="samBundle"
                     disabled="true"
                     dynamic="true"
                     label="archiving.reservation.method.policy"/>
        <cc:spacer name="spacer" width="10" height="1"/>
        <cc:checkbox name="rmFS"
                     bundleID="samBundle"
                     disabled="true"
                     dynamic="true"
                     label="archiving.reservation.method.fs"/>
        <cc:spacer name="spacer" width="10" height="1"/>
        <cc:dropdownmenu name="rmAttributes"
                         disabled="true"
                         dynamic="true"
                         bundleID="samBundle"/>
    </td>
</tr>

<cc:hidden name="errorOccur"/>
<cc:hidden name="allPools"/>
<cc:hidden name="poolMediaTypes"/>
</jato:pagelet>
