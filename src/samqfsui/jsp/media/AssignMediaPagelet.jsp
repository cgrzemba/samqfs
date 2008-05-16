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

// ident	$Id: AssignMediaPagelet.jsp,v 1.4 2008/05/16 19:39:21 am143972 Exp $
--%>

<%@ page language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script language="javascript">

    /* helper function to retrieve element by id */
    function $(id) {
        return document.getElementById(id);
    }

    // This javascript function will be called everytime when the pagelet is
    // loaded.  Add this call to onLoad() in pop up pages while using this
    // pagelet.
    function pageInit() {
        // toggle from-to field back to expression field if needed
        handleRadioMethod($("HiddenSelectionMethod"));

        // show table if needed
        var showTable = $("HiddenShowTable").value;
        $("TR_table").style.display =
            showTable == "true" ? "" : "none";
    }

    function handleRadioMethod(field) {
        if (field.value == "regex") {
            $("TR_to_from").style.display = "none";
            $("TD_to_from").style.display = "none";
            $("TR_expression").style.display = "";
            $("TD_expression").style.display = "";
        } else {
            $("TR_to_from").style.display = "";
            $("TD_to_from").style.display = "";
            $("TR_expression").style.display = "none";
            $("TD_expression").style.display = "none";
        }
    }

    /**
     * Called when user changes selection method radio button
     */
    function handleRadioSubmit(field) {
        var splitter = field.name.split("RadioMethod");
        var handler = splitter[0] + "HrefRadioMethod";
        var pageSession = field.form.elements["jato.pageSession"].value;
        field.form.action =
            field.form.action + "?" + handler +
            "=&jato.pageSession=" + pageSession;
        field.form.submit();
    }

    /**
     * Check if user selects a media type
     */
    function isMediaTypeSelected() {
        var selected = $("MenuType").value != -999;
        if (!selected) {
            alert($("HiddenNoMediaTypeMessage").value);
            $("MenuType").focus();
        }
        return selected;
    }

    /**
     * This method can be used in places other than onLoad()
     */
    function getPageMode() {
        var myForm = document.forms[0];
        if (myForm.name == "wizWinForm") {
            var pageModeObj =
                myForm.elements[
                    "WizardWindow.Wizard.AssignMediaView.HiddenPageMode"];
            return parseInt(pageModeObj.value);
        } else {
            return parseInt($("HiddenPageMode").value);
        }
    }

    // load wizardPageInit() when this pagelet is loaded in wizard
    if (document.forms[0].name == "wizWinForm") {
        WizardWindow_Wizard.pageInit = pageInit;
    }

</script>

<jato:pagelet>
    <cc:i18nbundle
        id="samBundle"
        baseName="com.sun.netstorage.samqfs.web.resource.Resources"/>

    <!-- feedback alert -->
    <cc:alertinline name="Alert" bundleID="samBundle"/>

    <table style="margin-left:10px" cellpadding="5" border="0">
        <tr id="TR_Name">
            <td valign="top">
                <cc:label
                    name="LabelName"
                    styleLevel="2"
                    bundleID="samBundle"
                    defaultValue="AssignMedia.label.name" />
            </td>
            <td>
                <cc:textfield
                    name="ValueName"
                    maxLength="16"/>
                <br />
                <cc:helpinline type="field">
                    <cc:text name="TextNameHelp"
                             bundleID="samBundle"
                             defaultValue="AssignMedia.help.name" />
                </cc:helpinline>
            </td>
        </tr>
        <tr id="TR_Type">
            <td valign="top">
                <cc:label
                    name="LabelType"
                    styleLevel="2"
                    bundleID="samBundle"
                    defaultValue="AssignMedia.label.type" />
            </td>
            <td>
                <cc:dropdownmenu
                    name="MenuType"
                    elementId="MenuType"
                    bundleID="samBundle"/>
            </td>
        </tr>
        <tr id="TR_Method">
            <td valign="top">
                <cc:label
                    name="LabelMethod"
                    styleLevel="2"
                    bundleID="samBundle"
                    defaultValue="AssignMedia.label.method" />
            </td>
            <td valign="top">
                <cc:radiobutton
                    name="RadioMethod"
                    bundleID="samBundle"
                    elementId="RadioMethod"
                    onClick="handleRadioMethod(this);
                             handleRadioSubmit(this);"/>
            </td>
        </tr>
        <tr>
            <td colspan="2">
                <cc:label
                    name="LabelInclude"
                    styleLevel="2"
                    bundleID="samBundle"
                    defaultValue="AssignMedia.label.include" />
            </td>
        </tr>
        <tr id="TR_to_from">
            <td colspan="2">
                <cc:text
                    name="TextInstructionToFrom"
                    defaultValue="AssignMedia.text.instruction.tofrom"
                    bundleID="samBundle" />
            </td>
        </tr>
        <tr id="TR_expression" style="display:none">
            <td colspan="2">
                <cc:text
                    name="TextInstructionRange"
                    defaultValue="AssignMedia.text.instruction.range"
                    bundleID="samBundle" />
            </td>
        </tr>
    </table>
    <table style="margin-left:20px" border="0">
        <tr>
            <td id="TD_to_from">
                <cc:label
                    name="LabelFrom"
                    defaultValue="AssignMedia.label.from"
                    styleLevel="2"
                    bundleID="samBundle" />
                <cc:textfield name="ValueFrom" size="10"/>
                <cc:spacer name="Spacer" width="10" />
                <cc:label
                    name="LabelTo"
                    defaultValue="AssignMedia.label.to"
                    styleLevel="2"
                    bundleID="samBundle" />
                <cc:textfield name="ValueTo" size="10" />
                <cc:spacer name="Spacer" width="15" />
            </td>
            <td id="TD_expression" style="display:none">
                <cc:label
                    name="LabelExpression"
                    defaultValue="AssignMedia.label.expression"
                    styleLevel="2"
                    bundleID="samBundle" />
                <cc:textfield name="ValueExpression" size="28"/>
            </td>
            <td>
                <cc:button
                    name="ButtonSelected"
                    defaultValue="AssignMedia.button.showselected"
                    onClick="if (!isMediaTypeSelected(this)) return false;"
                    bundleID="samBundle" />
                <cc:button
                    name="ButtonAll"
                    defaultValue="AssignMedia.button.showall"
                    onClick="if (!isMediaTypeSelected(this)) return false;"
                    bundleID="samBundle" />
            </td>
        </tr>
    </table>
    <table style="margin-left:10px" border="0" width="65%">
        <tr id="TR_table" style="display:none">
            <td colspan="2">
                <br />
                <img src="/com_sun_web_ui/images/other/dot.gif"
                     height="1" width="750" class="ConLin"/>
                <br />
                <br />
                <cc:actiontable
                    name="VolumeTable"
                    bundleID="samBundle"
                    title="<change_to_appropriate_title>"
                    selectionType="none"
                    showAdvancedSortIcon="false"
                    showLowerActions="false"
                    showPaginationControls="true"
                    showPaginationIcon="true"
                    showSelectionIcons="true"
                    maxRows="25"
                    page="1"/>
            </td>
        </tr>
    </table>

    <cc:hidden name="HiddenPageMode" elementId="HiddenPageMode"/>
    <cc:hidden name="HiddenShowTable" elementId="HiddenShowTable" />
    <cc:hidden name="HiddenSelectionMethod" elementId="HiddenSelectionMethod"/>
    <cc:hidden name="HiddenExpressionUsed" />
    <cc:hidden name="HiddenNoMediaTypeMessage"
               elementId="HiddenNoMediaTypeMessage" />

</jato:pagelet>
