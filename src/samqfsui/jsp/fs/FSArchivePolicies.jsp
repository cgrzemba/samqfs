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

// ident	$Id: FSArchivePolicies.jsp,v 1.23 2008/12/16 00:10:44 am143972 Exp $

--%>
<%@ page info="Index" language="java" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean">

<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>

<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header
    pageTitle="FSArchivePolicies.pageTitle"
    copyrightYear="2006"
    baseName="com.sun.netstorage.samqfs.web.resources.Resources"
    onLoad="
        if (parent.serverName != null) {
            parent.setSelectedNode('0', 'FSArchivePolicies');
        }
        toggleDisabledState();"
    bundleID="samBundle">

<script language="javascript">

    function toggleDisabledState(field) {

        var removeDisabled = true;

        var prefix = "FSArchivePolicies.FSArchivePoliciesView.";
        var tableName = prefix + "FSArchivePoliciesTable";
        var formName = "FSArchivePoliciesForm";
        var removeButton = prefix + "RemoveButton";

        // enable the remove button only when a row is selected
        if (field != null) {
            if (field.name == tableName + ".SelectAllHref") {
                removeDisabled = false;
            } else if (field.name == tableName + ".DeselectAllHref") {
                removeDisabled = true;
            } else if (field.type == "radio" && field.checked) {
                var index = field.value;
                var selectedIndex = parseInt(index);
                // assume the default policy is always the last one in the table
                var defaultPolicyIndex = parseInt(getTableSize()) - 1;
                var isGlobalCriteria =
                    document.FSArchivePoliciesForm.elements[
                        prefix + "FSArchivePoliciesTiledView[" + index
                            + "].IsGlobalCriteriaHiddenField"].value;
                if (selectedIndex == defaultPolicyIndex) {
                    // cannot delete the default policy
                    removeDisabled = true;
                    field.checked = false;
                } else if (isGlobalCriteria == "1") {
                    // cannot delete the global criteria
                    removeDisabled = true;
                    field.checked = false;
                    var alertMsg = "<cc:text name='StaticText' bundleID='samBundle'
                        defaultValue='FSArchivePolicies.msg.removeGlobalCriteria'/>";
                    alert(alertMsg);
                } else {
                    removeDisabled = false;
                }
            }
        }

        // now toggle remove button disable state
        ccSetButtonDisabled(removeButton, formName, removeDisabled);
    }

    function showConfirmMsg(key) {

        var str1 = "<cc:text name='StaticText' bundleID='samBundle'
            defaultValue='FSArchivePolicies.msg.removePolicyCriteria'/>";

        if (key == 1) {
            if (!confirm(str1)) {
                return false;
            } else {
                return true;
            }
        } else {
            return false; // this case should never be used
        }
    }

    function getTableSize() {
        var myForm = document.FSArchivePoliciesForm;
        var size = myForm.elements[
            "FSArchivePolicies.FSArchivePoliciesView.ModelSizeHiddenField"].value;
        return size;
    }
    
    function getServerKey() {
        var myForm = document.FSArchivePoliciesForm;
        return myForm.elements["FSArchivePolicies.ServerName"].value;
    }

    function getClientParams() {
        var myForm = document.FSArchivePoliciesForm;
        var fsName = myForm.elements["FSArchivePolicies.FileSystemName"].value;
        var clientParams =
            "fsNameParam=" + fsName + "&" + "serverNameParam=" + getServerKey();
        return clientParams;
    }

</script>

<jato:form name="FSArchivePoliciesForm" method="post">

<!-- Bread Crumb componente-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<br>
<cc:alertinline name="Alert" bundleID="samBundle" /><br />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
    pageTitleText="FSArchivePolicies.title"
    showPageTitleSeparator="false"
    showPageBottomSpacer="true"
    showPageButtonsTop="false"
    showPageButtonsBottom="true">

    <!-- PropertySheet -->
    <cc:propertysheet name="PropertySheet"
        bundleID="samBundle"
        showJumpLinks="false"/>

</cc:pagetitle>

<br><br>

<!-- Action Table -->
<jato:containerView name="FSArchivePoliciesView">

  <cc:actiontable
    name="FSArchivePoliciesTable"
    bundleID="samBundle"
    title="FSArchivePolicies.actionTable.title"
    selectionType="single"
    selectionJavascript="toggleDisabledState(this)"
    showAdvancedSortIcon="true"
    showLowerActions="true"
    showPaginationControls="true"
    showPaginationIcon="true"
    showSelectionIcons="true"
    maxRows="25"
    page="1"/>

<cc:hidden name="AddCriteriaHiddenField" />
<cc:hidden name="ReorderNewOrderHiddenField" />
<cc:hidden name="ModelSizeHiddenField" />

<cc:hidden name="unreorderableCriteria"/>
</jato:containerView>

<br>
<table>
<tr>
<td></td>
<td>
<cc:spacer name="Spacer" width="5" height="1" />
<cc:image name="Image" bundleID="samBundle"
    defaultValue="/samqfsui/images/required.gif" />
<cc:spacer name="Spacer" width="11" height="1" />
<cc:text name="StaticText" bundleID="samBundle"
    defaultValue="FSArchivePolicies.text.globalCriteria" />
</td>
<td> </td>
</tr>
<tr>
<td></td>
<td>
<cc:spacer name="Spacer" width="5" height="1" />
<cc:image name="Image" bundleID="samBundle"
    defaultValue="/samqfsui/images/required.gif" />
<cc:image name="Image" bundleID="samBundle"
    defaultValue="/samqfsui/images/required.gif" />
<cc:spacer name="Spacer" width="1" height="1" />
<cc:text name="StaticText" bundleID="samBundle"
    defaultValue="FSArchivePolicies.text.defaultPolicy" />
</td>
<td> </td>
</tr>
</table>



<cc:hidden name="ServerName" />
<cc:hidden name="FileSystemName" />

</jato:form>
</cc:header>
</jato:useViewBean>
