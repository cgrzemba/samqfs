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

// ident	$Id: ISPolicyWizardSelectDataClass.jsp,v 1.5 2008/12/16 00:10:43 am143972 Exp $
--%>

<%@ page language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%> 
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">

    var tf = document.wizWinForm;
    var prefix = "WizardWindow.Wizard.ISPolicyWizardSelectDataClassView.";

    function wizardPageInit() {
        var disabled = false;
        var error = tf.elements[prefix + "errorOccur"].value;
        if (error == "exception") {
            disabled = true;
        }

        WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
        WizardWindow_Wizard.setPreviousButtonDisabled(disabled, null);
    }
    
    function getValue(number) {
        var allNames = tf.elements[prefix + "AllClassNames"].value;
        var allNamesArr = allNames.split("###");
        if (number > allNamesArr.length - 1) return "";
        else return allNamesArr[parseInt(number)];
    }
   
    function handleDataClassTableSelection(field) {
        var selected = tf.elements[prefix + "SelectedClass"];
        if (field.name == prefix + "SelectDataClassTable.DeselectAllHref") {
            selected.value = "";
        } else {
            selected.value = getValue(field.value);
        }
    }

    WizardWindow_Wizard.pageInit = wizardPageInit;

</script>

<jato:pagelet>

<cc:i18nbundle
    id="samBundle"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:actiontable
    name="SelectDataClassTable"
    bundleID="samBundle"
    title="archiving.policy.wizard.selectdataclass.tabletitle"
    showAdvancedSortIcon="false"
    selectionJavascript="handleDataClassTableSelection(this)"
    showLowerActions="false"
    showPaginationControls="false"
    showPaginationIcon="false"
    showSelectionIcons="true"
    selectionType="single"
/>

<cc:hidden name="AllClassNames"/>
<cc:hidden name="SelectedClass"/>
<cc:hidden name="errorOccur" />
</jato:pagelet>
