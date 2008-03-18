<%--
/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: RecoveryPointSchedule.jsp,v 1.7 2008/03/17 14:40:34 am143972 Exp $
--%>
<%@ page info="RecoveryPointSchedule" language="java" %> 
<%@taglib uri="/WEB-INF/tld/com_iplanet_jato/jato.tld" prefix="jato"%>
<%@taglib uri="/WEB-INF/tld/com_sun_web_ui/cc.tld" prefix="cc"%>

<jato:useViewBean className="com.sun.netstorage.samqfs.web.fs.RecoveryPointScheduleViewBean">
<cc:header pageTitle="fs.recoverypointschedule.title" 
           copyrightYear="2006"
           baseName="com.sun.netstorage.samqfs.web.resources.Resources"
           onLoad="
                if (parent.serverName != null) {
                    parent.setSelectedNode('0', 'RecoveryPointSchedule');
            }"
           bundleID="samBundle">

<script language="javascript" src="/samqfsui/js/samqfsui.js"></script>
<script language="javascript" src="/samqfsui/js/fs/recovery_point_schedule.js"></script>

<%--
    <script>
        function init() {
            var typeObj = getSelectedRadioButton(
                                document.FSScheduleDumpForm, 
                                "FSScheduleDump.ScheduleDumpView.retentionType");
            onClickRetentionType(typeObj);
            updateExcludeDirButtons();
            onChangeCompression();
        }

        function isValidPositiveData(fieldId) {
            var value = trim(fieldId.value);
            if (!isEmpty(value)) {
                if (!isPositiveInteger(value))
                    return false;
            }
            return true;
        }

        function getSelectedSchemaObj() {
            return document.FSScheduleDumpForm
                .elements['FSScheduleDump.ScheduleDumpView.selectedSchema'];
        }

        function checkNamePrefix() {
            // Check that file name does not include any bad chars.
            // Need to allow for colons, so I can't use isValidLogFile 
            // function.
            var badChars = new Array(20);
            badChars[0] = "*";
            badChars[1] = "?";
            badChars[2] = "\\";
            badChars[3] = "!";	
            badChars[4] = "&";
            badChars[5] = "%";
            badChars[6] = "'";
            badChars[7] = "\"";
            badChars[8] = "(";
            badChars[9] = ")";
            badChars[10] = "+";
            badChars[11] = ",";
            badChars[12] = ";";
            badChars[13] = "<";
            badChars[14] = ">";
            badChars[15] = "@";
            badChars[16] = "#";
            badChars[17] = "$";
            badChars[18] = "=";
            badChars[19] = "^";    
            
            var namePrefixObj = document.FSScheduleDumpForm.elements[
                "FSScheduleDump.ScheduleDumpView.namePrefixValue"];
                
            if (!isValidString(namePrefixObj.value, badChars)) {
                    return false;
            }

            return true;
        }

        function checkLocation() {
            var locationObj = document.FSScheduleDumpForm.elements[
                "FSScheduleDump.ScheduleDumpView.folderChooser.browsetextfield"];
            var mountPoint = document.FSScheduleDumpForm.elements[
                "FSScheduleDump.ScheduleDumpView.mountPoint"].value;                
            var locationStart = locationObj.value.substr(0, mountPoint.length);

            if (locationStart == mountPoint) {
                // Check next char in location to be sure it's a slash.
                // If not, it is a different directory
                if (locationObj.value.charAt(mountPoint.length) == "/") {
                    // Prompt user:  what are you doing??!!!
                    // Don't put them on the same fs
                    var prompt = document.FSScheduleDumpForm.elements[
                        "FSScheduleDump.ScheduleDumpView.mountPointPrompt"].value;
                    if (confirm(prompt)) {
                        // User wants to save anyway
                        return true;
                    } else {
                        return false;
                    }
                }
            }
            return true;
        }
        
        function getFileChooserParams() {
            // get client side params
            //return "&serverNameParam=rumble.east";
            return null;
        }
                
        function onClickRetentionType(typeObj) {
            var theForm = document.FSScheduleDumpForm;
            if (typeObj.value == "always") {
                // Disable "until" type controls
                ccSetTextFieldDisabled("FSScheduleDump.ScheduleDumpView.retentionValue", "FSScheduleDumpForm", true);
                ccSetDropDownMenuDisabled("FSScheduleDump.ScheduleDumpView.retentionUnit", "FSScheduleDumpForm", true);
            } else {
                // Enable "until" type controls
                ccSetTextFieldDisabled("FSScheduleDump.ScheduleDumpView.retentionValue", "FSScheduleDumpForm", false);
                ccSetDropDownMenuDisabled("FSScheduleDump.ScheduleDumpView.retentionUnit", "FSScheduleDumpForm", false);
            }        
        }

        function updateExcludeDirButtons() {
            // Can only select 10 directories.
            var theForm = document.FSScheduleDumpForm;
            var listObj = theForm.elements["FSScheduleDump.ScheduleDumpView.contentsExcludeList"];
            
            // Get option count and add 1 for the "dummy"
            if (listObj.length >= 11) {
                // Can't add any more
                ccSetButtonDisabled("FSScheduleDump.ScheduleDumpView.contentsExcludeAdd.browseServer",
                                    "FSScheduleDumpForm", true);
            } else {
                ccSetButtonDisabled("FSScheduleDump.ScheduleDumpView.contentsExcludeAdd.browseServer",
                                    "FSScheduleDumpForm", false);
            }
            
            if (listObj.length <= 1) {
                // Can't remove
                ccSetButtonDisabled("FSScheduleDump.ScheduleDumpView.contentsExcludeRemove",
                                    "FSScheduleDumpForm", true);
            } else {
                ccSetButtonDisabled("FSScheduleDump.ScheduleDumpView.contentsExcludeRemove",
                                    "FSScheduleDumpForm", false);            
            }
        }        
        
        function onClickEditNotifications() {
            var prompt = document.FSScheduleDumpForm.elements[
                            "FSScheduleDump.ScheduleDumpView.notificationEditPrompt"].value;
            if (confirm(prompt)) {
                // Forward to notifications page
                return true;
            } else {
                return false;
            }
        }
        
        function onChangeCompression() {
            var compressionObj = document.FSScheduleDumpForm.elements["FSScheduleDump.ScheduleDumpView.compressionValue"];
            var autoIndexObj = document.FSScheduleDumpForm.elements[
                            "FSScheduleDump.ScheduleDumpView.autoIndexValue"];
            if (compressionObj.value == "compress") {
                // Don't allow auto indexing b/c server can't use dump files
                // compressed with "compress" unlike with "gzip"
                autoIndexObj.checked = false;
                ccSetCheckBoxDisabled(autoIndexObj.name, "FSScheduleDumpForm", true);  
            } else {
                ccSetCheckBoxDisabled(autoIndexObj.name, "FSScheduleDumpForm", false);
            }
        }
        
        function onClickContentsExcludeRemove(buttonObj) {
            // Only can remove if somthing is selected in the list.
            var listObj = document.FSScheduleDumpForm.elements["FSScheduleDump.ScheduleDumpView.contentsExcludeList"];
            if (listObj.selectedIndex == -1) {
                var promptObj = document.FSScheduleDumpForm.elements["FSScheduleDump.ScheduleDumpView.removePrompt"];
                alert(promptObj.value);
                return;
            }
            
            // Remove it
            onClickWithAnchorJump(buttonObj, 'snapshotContentsSection');
        }
    </script>
--%>

<!-- Form -->
<jato:form name="RecoveryPointScheduleForm" method="post">
    
<!-- Bread Crumb componente-->
<cc:breadcrumbs name="BreadCrumb" bundleID="samBundle" />

<!-- Alert -->
<cc:alertinline name="Alert" bundleID="samBundle" /><br />

<!-- Page title -->
<cc:pagetitle name="PageTitle" 
              bundleID="samBundle"
              pageTitleText="recoverypointschedule.title"
              showPageTitleSeparator="true"
              showPageButtonsTop="true"
              showPageButtonsBottom="true">

<!-- Property Sheet -->
<jato:containerView name="RecoveryPointScheduleView">
<cc:propertysheet name="PropertySheet" 
                  bundleID="samBundle" 
                  addJavaScript="true"
                  showJumpLinks="true" />

</jato:containerView>

<cc:hidden name="errMsg"/>
</cc:pagetitle>
</jato:form>
</cc:header>
</jato:useViewBean> 

