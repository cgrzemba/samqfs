
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

// ident	$Id: FSWizardDeviceSelectionPage.jsp,v 1.15 2008/09/10 17:40:23 ronaldso Exp $
--%>
<%@ page language="java" %>
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<script type="text/javascript">
    var tf = document.wizWinForm;

    WizardWindow_Wizard.pageInit = wizardPageInit;

    var internalCounter = 0;
    var initialCounter  = -1;
    var totalItems = -1;
    var tokenString = "";

    var viewNameKey = "PageView";
    var viewName = "";

    function wizardPageInit() {

        if (viewName == "") {
            getPageViewName();
        }

        var string = tf.elements[viewName + ".errorOccur"];
        var error = string.value;

        if (error == "exception") {
            WizardWindow_Wizard.setNextButtonDisabled(true, null);
            WizardWindow_Wizard.setPreviousButtonDisabled(true, null);
        }
    }

    function getPageViewName() {
        var e = tf.elements[0];
        var viewNameIndex = e.name.indexOf(viewNameKey);
        if (viewNameIndex != -1) {
            viewName = e.name.substr(0, viewNameIndex + viewNameKey.length);
        }
    }

    function handleTableSelection(field) {

        // first check if the table is single-select
        // If the field type is radio, user is creating a UFS
        // No counter in UFS, return here
        if (field.type == "radio") {
            return;
        }


        if (viewName == "") {
            getPageViewName();
        }

        if (tokenString == "") {
            tokenString = tf.elements[viewName + ".initSelected"].value;
            var myArray = tokenString.split(",");

            initialCounter = myArray[0] - 0;
            totalItems     = myArray[1] - 0;
        }

        var showValue = 0;

        // Check if the field is select all / deselect all
        // Set counter to totalItems if the field is select all
        // or set the counter to zero if the field is deselect all
        // Also, check if the field type is checkbox to ensure the
        // add/substract one if what we intend to issue

        if (field.name == viewName + ".DeviceSelectionTable.SelectAllHref") {
            showValue = totalItems;
            internalCounter = totalItems;
            initialCounter  = 0;
        } else if (field.name == viewName + ".DeviceSelectionTable.DeselectAllHref") {
            showValue = 0;
            internalCounter = 0;
            initialCounter  = 0;
        } else if (field.type == "checkbox") {
            if (field.checked) {
                internalCounter += 1;
            } else {
                internalCounter -= 1;
            }

            showValue = initialCounter + internalCounter;
        }
        tf.elements[viewName + ".counter"].value =
            showValue;
    }

    function wizardNextClicked() {
        return true;
    }

    WizardWindow_Wizard.nextClicked = wizardNextClicked;

</script>

<jato:pagelet>

<cc:i18nbundle id="samBundle"
 baseName="com.sun.netstorage.samqfs.web.resources.Resources" />

<table>
    <tr>
        <td><cc:spacer name="Spacer" width="5" /></td>
        <td><cc:text name="AdditionalInstruction"
                     bundleID="samBundle"
                     defaultValue="fs.selectedevice.sharedfsgrow"/></td>
    </tr>
</table>

<table width="500">
<tr>
<td>
<cc:alertinline name="Alert" bundleID="samBundle" /><br />
</td>
</tr>
</table>

<br />
<cc:spacer name="Spacer" width="10" />
<cc:label name="counterLabel" defaultValue="FSWizard.deviceSelectionPage.counterLabel"
        bundleID="samBundle" />
<cc:spacer name="Spacer" width="10" />
<cc:textfield name="counter" bundleID="samBundle" size="3" disabled="true"/>

<cc:hidden name="initSelected" />

<tr>
<td valign="center" align="left" rowspan="1" colspan="1">
<cc:actiontable
    name="DeviceSelectionTable"
    bundleID="samBundle"
    title="FSWizard.deviceSelectionTable.title"
    selectionType="multiple"
    selectionJavascript="handleTableSelection(this)"
    showAdvancedSortIcon="false"
    showLowerActions="false"
    showPaginationControls="false"
    showPaginationIcon="false"
    showSelectionIcons="false"/>
</td>
</tr>

<cc:hidden name="errorOccur" />

</jato:pagelet>
