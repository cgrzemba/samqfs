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

// ident	$Id: FSSamfsck.jsp,v 1.19 2008/12/16 00:10:45 am143972 Exp $
--%>

<%@ page info="Index" language="java" %>
<%@ page import="com.iplanet.jato.view.ViewBean" %>
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.FSSamfsckViewBean">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/popuphelper.js"></script>


<!-- Define the resource bundle, html, head, meta, stylesheet and body tags -->
<cc:header pageTitle="FSSamfsck.pagetitle"
          copyrightYear="2006"
          baseName="com.sun.netstorage.samqfs.web.resources.Resources" 
          bundleID="samBundle"
          onLoad="initializePopup(this);">

<script>
    function validate() {
        var tf   = document.FSSamfsckForm;
        var path  = trim(tf.elements["FSSamfsck.TextField"].value);	

        if (isEmpty(path)) {
            alert(getErrorString(1));
            return false;
        } else {
            if (!isValidLogFile(path)) {
                alert(getErrorString(2));
                return false;
            }
        }
        return true;
    }

    function getErrorString(index) {

        var str1 = "<cc:text name='StaticText' bundleID='samBundle' defaultValue='FSSamfsck.errMsg1'/>";
        var str2 = "<cc:text name='StaticText' bundleID='samBundle' defaultValue='FSSamfsck.errMsg2'/>";

        if (index == 1) {
            return str1;
        } else if (index == 2) {
            return str2;
        } else {
            return null;
        }
    }

    function doSubmit() {

        // Form Name
        var tf = document.FSSamfsckForm;
        var pf = window.opener.document.<cc:text name='ParentForm'/>;
        if (pf == null) {
            return false;
        }
        var parent = "<cc:text name='ParentForm'/>";

        // Set Command Child Name and Parent Hidden field values
        var commandChildName;
        var samfsckHiddenType;
        var samfsckHiddenLog;
        if (parent == "FSSummaryForm") {
            commandChildName  = "FSSummary.FileSystemSummaryView.SamfsckHref";
            samfsckHiddenType = "FSSummary.FileSystemSummaryView.SamfsckHiddenField1";
            samfsckHiddenLog  = "FSSummary.FileSystemSummaryView.SamfsckHiddenField2";
        } else {
            commandChildName = "FSDetails.FSDetailsView.SamfsckHref";
            samfsckHiddenType = "FSDetails.FSDetailsView.SamfsckHiddenAction";
            samfsckHiddenLog  = "FSDetails.FSDetailsView.SamfsckHiddenLog";
        }

        // Element Names in Reorder Policy Page
        var radio = tf.elements["FSSamfsck.fsckType"];
        var text  = tf.elements["FSSamfsck.TextField"];

        if (radio[0]) {
            for (var i = 0; i < radio.length; i++) {
                if (radio[i].checked && i == 0) {
                    pf.elements[samfsckHiddenType].value = "check";
                } else if (radio[i].checked && i == 1) {
                    pf.elements[samfsckHiddenType].value = "both";
                }
            }
        }

        pf.elements[samfsckHiddenLog].value = text.value;
	
        // Because this is launched from drop down
        // Need to modify pf.action before submit
        var myActionArray = pf.action.split("?");
        var myAction = myActionArray[0];

        // Set Form action URL and submit
        pf.action = myAction + "?" + commandChildName + "=&jato.pageSession=" +
            pf.elements["<%=ViewBean.PAGE_SESSION_ATTRIBUTE_NVP_NAME %>"].value;

        pf.submit();
        return true;
    }

    function resetURL() {

        var pf = window.opener.document.<cc:text name='ParentForm'/>;
        if (pf == null) {
            return;
        }
        
        var parentForm = "<cc:text name='ParentForm'/>";

        // Because this is launched from drop down
        // Need to modify pf.action before submit
        var myActionArray = pf.action.split("?");
        var myAction = myActionArray[0];

        // Command Child Name
        var commandChildName;
        if (parentForm == "FSSummaryForm") {
            commandChildName  = "FSSummary.FileSystemSummaryView.CancelHref";
        } else {
            commandChildName = "FSDetails.FSDetailsView.CancelHref";
        }

        // Set Form action URL and submit
        pf.action = myAction + "?" + commandChildName + "=&jato.pageSession=" +
            pf.elements["<%=ViewBean.PAGE_SESSION_ATTRIBUTE_NVP_NAME %>"].value;

        pf.submit();
    }

    function disableRadioButton() {

        var myForm = document.FSSamfsckForm;
        var radio  = myForm.elements["FSSamfsck.fsckType2"];
        var isMounted = myForm.elements["FSSamfsck.MountHidden"];
        if (isMounted.value == "yes") {
            radio.disabled = true;
        }
    }  

</script>


<!-- Masthead -->
<cc:secondarymasthead name="SecondaryMasthead" bundleID="samBundle" />

<jato:form name="FSSamfsckForm" method="post">

<cc:alertinline name="Alert" bundleID="samBundle" />

<cc:pagetitle name="PageTitle" bundleID="samBundle"
	pageTitleText="FSSamfsck.pageTitle"
        showPageTitleSeparator="true"
        showPageButtonsTop="false"
        showPageButtonsBottom="true">

<cc:hidden name="MountHidden" />

<div align="right">
  <cc:label name="Label" bundleID="samBundle"
    defaultValue="page.required" styleLevel="3"
    showRequired="true" />
</div>

<table width="70%">
<tr>
  <td rowspan="4">
    <cc:spacer name="Spacer1" width="10" />
  </td>
  <td>
    <cc:radiobutton name="fsckType" bundleID="samBundle" styleLevel="3" />
  </td>
</tr>

<tr><td><cc:spacer name="Spacer1" width="10" /></td></tr>
<tr><td><cc:spacer name="Spacer1" width="10" /></td></tr>

<tr>
  <td valign="center" align="left" rowspan="1" colspan="1">
    <cc:label name="Label" styleLevel="2"
                defaultValue="FSSamfsck.pathlabel"
                showRequired="true"
                bundleID="samBundle" />
  </td>
  <td>
    <cc:textfield name="TextField" bundleID="samBundle" autoSubmit="false" 
	maxLength="126" />
  </td>
</tr>
</table>

</cc:pagetitle>

</jato:form>
</cc:header>
</jato:useViewBean> 
