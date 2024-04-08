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

// ident	$Id: FSAddPolicies.jsp,v 1.15 2008/12/16 00:10:44 am143972 Exp $
--%>
<%@ page info="Index" language="java" %>
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.FSAddPoliciesViewBean">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>
<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="FSAddPolicyCriteria.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    bundleID="samBundle"
    onLoad="window.opener.document.FSArchivePoliciesForm.target='_self'; 
           initializePopup(this);">

<script language="javascript">

    var selectedRows = 0;
    var totalRows = -1;

    function updateSelectedRows(field) {

        var prefix = "FSAddPolicies.FSAddPoliciesView.";
        var tableName = prefix + "FSAddPoliciesTable";
        var formName = "FSAddPoliciesForm";
        var submitButton = "FSAddPolicies.Submit";
        var submitButtonDisabled = true;

        if (totalRows < 0) {
            totalRows = document.FSAddPoliciesForm.elements[
                prefix + "NumberOfRowsHiddenField"].value;
        }

        // update selected rows counter
        if (field != null) {
            if (field.name == tableName + ".SelectAllHref") {
                selectedRows = totalRows;
            } else if (field.name == tableName + ".DeselectAllHref") {
                selectedRows = 0;
            } else if (field.type == "checkbox") {
                if(field.checked) {
                    selectedRows++;
                } else {
                    selectedRows--;
                }
            }
        }

        if (selectedRows > 0) {
            submitButtonDisabled = false;
        }
        ccSetButtonDisabled(submitButton, formName, submitButtonDisabled);
    }

    function doSubmit() {

        // Form Name
        var tf   = document.FSAddPoliciesForm;
        var pf   = window.opener.document.FSArchivePoliciesForm;

        // Command Child Name
        var commandChildName = "<cc:text name='AddPoliciesHref' />";

        // Parent window Hidden field name
        var addPoliciesHiddenField = "<cc:text name='AddPoliciesHiddenField' />";

        var checkboxName =
            "FSAddPolicies.FSAddPoliciesView.FSAddPoliciesTable.SelectionCheckbox";
        var policyName =
            "FSAddPolicies.FSAddPoliciesView.PolicyNameHiddenField";
        var criteriaNumber =
            "FSAddPolicies.FSAddPoliciesView.CriteriaNumberHiddenField";

        var i, counter = 0;
        var rowChecked = false;
        var newPolicyCriteriaString = "";

        for (i = 0; i < tf.elements.length; i++) {
            var e = tf.elements[i];

            if (e.name.indexOf(checkboxName) != -1 && e.type == "checkbox") {
                if (e.checked) {
                    rowChecked = true;
                } else {
                    rowChecked = false;
                }
            } else if (e.name.indexOf(policyName) != -1 && rowChecked) {
                if (counter > 0) {
                    newPolicyCriteriaString += ",";
                }
                newPolicyCriteriaString += tf.elements[i].value;
                counter++;
            } else if (e.name.indexOf(criteriaNumber) != -1 && rowChecked) {
                newPolicyCriteriaString += ":";
                newPolicyCriteriaString += tf.elements[i].value;
            }
        }

        // save the selected policy criteria and submit the form
        tf.elements["FSAddPolicies.addedPolicyCriteria"].value = 
            newPolicyCriteriaString;
            return true;
    }

    function isChecked(target, myArray, length) {
        for (var i = 0; i < length; i++) {
            if (myArray[i] == target) {
                return 1;
            }
        }

        return 0;
    }
		
    function resetURL() {

        var pf = window.opener.document.FSArchivePoliciesForm;

        // Command Child Name
        var commandChildName = "<cc:text name='CancelHref' />";

        // Set Form action URL and submit
        pf.action = pf.action + "?" + commandChildName +
                "=&jato.pageSession=" + 
        pf.elements["<%=ViewBean.PAGE_SESSION_ATTRIBUTE_NVP_NAME %>"].value;

        pf.submit();
    }
	
</script>


<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="FSAddPoliciesForm" method="post">
<!-- inline alart -->
<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
    pageTitleText="FSAddPolicyCriteria.title"
    showPageTitleSeparator="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">


<br>

<!-- Action Table -->
<jato:containerView name="FSAddPoliciesView">
    <cc:actiontable
        name="FSAddPoliciesTable"
        bundleID="samBundle"
        title="FSAddPolicyCriteria.tableTitle"
        selectionType="multiple"
        selectionJavascript="updateSelectedRows(this)"
        showAdvancedSortIcon="true"
        showLowerActions="true"
        showPaginationControls="true"
        showPaginationIcon="true"
        showSelectionIcons="true"
        maxRows="25"
        page="1"/>

<cc:hidden name="NumberOfRowsHiddenField" />
</jato:containerView>

<cc:hidden name="addedPolicyCriteria"/>
</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean>
