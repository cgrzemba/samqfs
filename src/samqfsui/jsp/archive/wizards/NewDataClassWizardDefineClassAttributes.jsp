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

// ident	$Id
--%>


<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">
    
    var tf       = document.wizWinForm;
    var formName = "wizWinForm";
    var prefix   =
        "WizardWindow.Wizard.NewDataClassWizardDefineClassAttributesView.";
    var selected = "";

    function handleExpirationTimeTypeChange(field) {
        if (field == null) {
            var radio = tf.elements[prefix + "expirationTimeType"];
            for (var i = 0; i < radio.length; i++) {
                if (radio[i].checked) {
                    selected = radio[i].value;
                    break;
                }
            }
        } else {
            selected = field.value;
        }

        if (selected == "duration") {
            document.getElementById("dateDiv").style.display="none";
            document.getElementById("durationDiv").style.display="";
        } else {
            // also falls in this case if field.value is null
            document.getElementById("dateDiv").style.display="";
            document.getElementById("durationDiv").style.display="none";
        }
    }
    
    function handlePeriodicAuditChange(field) {
        if (field == null) {
            field = tf.elements[prefix + "periodicaudit"];
        }
        if (field.value == "0") {
            document.getElementById("auditPeriodDiv").style.display="none";
        } else {
            document.getElementById("auditPeriodDiv").style.display="";
        }
    }
    
    function handleDeDupChange(field) {
        ccSetCheckBoxDisabled(
            "WizardWindow.Wizard." +
            "NewDataClassWizardDefineClassAttributesView.bitbybit",
            field.form.name,
            !field.checked);
    }

    function wizardPageInit() {
        var disabled = false;
        var tf = document.wizWinForm;
        var error = tf.elements[
           "WizardWindow.Wizard.NewDataClassWizardDefineClassAttributesView." +
            "errorOccur"].value;
        
        WizardWindow_Wizard.setNextButtonDisabled(error == "true", null);
        WizardWindow_Wizard.setPreviousButtonDisabled(error == "true", null);
        
        // Set the correct style for interchangable components
        handleExpirationTimeTypeChange();
        handlePeriodicAuditChange();
   }

   WizardWindow_Wizard.pageInit = wizardPageInit;

</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle
    name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="archiving.dataclass.attributes" />


<table border="0" style="margin-left:10px;margin-top:10px">
<tr>
    <td colspan="2">
        <cc:checkbox
            name="autoworm"
            label="archiving.dataclass.attributes.enableworm"
            bundleID="samBundle" />
    </td>
</tr>
<tr>
    <td nowrap style="width:35%">
        &nbsp;&nbsp;&nbsp;&nbsp;
        <cc:label
            name="expirationTimeLabel"
            defaultValue="archiving.dataclass.expirationtime"
            bundleID="samBundle" />
    </td>
    <td nowrap style="width:35%">
        <cc:radiobutton
            name="expirationTimeType"
            layout="horizontal"
            onClick="return handleExpirationTimeTypeChange(this)"/>
    </td>
</tr>
<tr>
    <td></td>
    <td>
        <div id="dateDiv">
            <cc:textfield name="absolute_expiration_time" />
            <br/>
            <cc:helpinline type="field">
                <cc:text
                    name="HelpText"
                    bundleID="samBundle"
                    defaultValue="archiving.dataclass.wizard.inlinehelp.date"/>
            </cc:helpinline>
            <cc:hidden name="absolute_expiration_time-hidden" />
        </div>
        <div id="durationDiv" style="display:none">
            <cc:textfield
                name="relative_expiration_time"
                size="10"
                dynamic="true"/>
            <cc:dropdownmenu
                name="relative_expiration_time_unit"
                dynamic="true"
                bundleID="samBundle"/>
        </div>
    </td>
</tr>
<tr>
    <td></td>
    <td>
        <cc:checkbox
            name="neverExpire"
            label="archiving.dataclass.attributes.neverexpire"
            bundleID="samBundle"/>
    </td>
</tr>
</table>

<table border="0" style="width:90%;margin-left:10px;margin-top:10px">
<tr>
    <td nowrap>
        <cc:checkbox
            name="autodelete"
            label="archiving.dataclass.attributes.filedeletion"
            bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td nowrap>
        <cc:checkbox
            name="dedup"
            label="archiving.dataclass.attributes.detectduplicate"
            onClick="handleDeDupChange(this)"
            bundleID="samBundle"/>
        <div id="absolutePathDiv" style="text-indent:40px">
            <cc:checkbox
                name="bitbybit"
                label="archiving.dataclass.attributes.absolutepath"
                dynamic="true"
                disabled="true"
                bundleID="samBundle"/>
        </div>
    </td>
</tr>
</table>

<table border="0" style="width:80%;margin-left:10px;margin-top:5px">
<tr>
    <td nowrap>
        <cc:label
            name="periodicAuditLabel"
            elementName="periodicaudit"
            defaultValue="archiving.dataclass.attributes.periodicaudit"
            bundleID="samBundle"/>
        <cc:spacer name="Spacer" width="5" height="1" />
        <cc:dropdownmenu
            name="periodicaudit"
            bundleID="samBundle"
            onChange="return handlePeriodicAuditChange(this)"/>
    </td>
</tr>
<tr>
    <td nowrap>
        <div id="auditPeriodDiv" style="text-indent:40px;display:none">
            <cc:label
                name="auditPeriodLabel"
                elementName="auditperiod"
                defaultValue="archiving.dataclass.attributes.auditperiod"
                bundleID="samBundle"/>
            <cc:spacer name="Spacer" width="5" height="1" />
            <cc:textfield
                name="auditperiod"
                size="5"
                maxLength="10"/>
            <cc:dropdownmenu
                name="auditperiodunit"
                bundleID="samBundle"/>
        </div>
    </td>
</tr>
</table>

<br />
<cc:pagetitle
    name="PageTitle" 
    bundleID="samBundle"
    pageTitleText="archiving.dataclass.logging" />

<table border="0" style="width:90%;margin-left:10px;margin-top:10px">
<tr>
    <td style="width:50%" nowrap>
        <cc:checkbox
            name="log_data_audit"
            label="archiving.dataclass.log.audit"
            bundleID="samBundle"/>
    </td>
    <td nowrap>
        <cc:checkbox
            name="log_deduplication"
            label="archiving.dataclass.log.duplicate"
            bundleID="samBundle"/>
    </td>
</tr>
<tr>
    <td nowrap>
        <cc:checkbox
            name="log_autoworm"
            label="archiving.dataclass.log.worm"
            bundleID="samBundle"/>
    </td>
    <td nowrap>
        <cc:checkbox
            name="log_autodeletion"
            label="archiving.dataclass.log.deletion"
            bundleID="samBundle"/>
    </td>
</tr>
</table>

<cc:hidden name="errorOccur" />

</jato:pagelet>
