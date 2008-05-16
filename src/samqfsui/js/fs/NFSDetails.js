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

// ident $Id: NFSDetails.js,v 1.8 2008/05/16 19:39:13 am143972 Exp $

/** 
 * This is the javascript file for NFS Details
 */

    var selectedIndex = "";
    
    function toggleDisabledState(field) {
        var buttonDisabled  = true;
        var shareDisabled   = true;
        var unshareDisabled = true;
        var prefix          = "NFSDetails.NFSShareDisplayView.";
        var shareState      = "";
        
        var actionButton1 = prefix + "EditButton";
        var actionButton2 = prefix + "RemoveButton";
        var actionMenu    = prefix + "ActionMenu";
        var myForm        = document.NFSDetailsForm;
        var selection     = prefix + "NFSShareDisplayTable.Selection";

        // checkbox or radioButton for row selection
        if (field != null) {
            selectedIndex     = field.value;
            document.NFSDetailsForm.elements[
                "NFSDetails.SelectedIndex"].value = selectedIndex;
            var elementName   = field.name;
            
            if (elementName !=
                (prefix + "tablevalue.DeselectAllHref")
                && field.type == "radio" && field.checked) {
                buttonDisabled = false;
            }
        } else {
            // User uses FORWARD/BACK button
            for (i = 0; i < myForm.elements.length; i++) {
                var e = myForm.elements[i];
                if (e.name.indexOf(selection) != -1) {
                    if (e.checked) {
                        buttonDisabled = false;
                        break;
                    }
                }
            }
        }

        if (selectedIndex != "") {
            // get nfssharedState value calculated on serverside and
            // stored in the hidden field
            var hiddenShareState = 
                document.NFSDetailsForm.elements[
                    "NFSDetails.SharedStateHiddenField"].value;
            var shareStateArray = hiddenShareState.split("###");
            shareState = shareStateArray[selectedIndex];
            if (shareState == "true") {
                unshareDisabled = false;
            } else {
                shareDisabled = false;
            }
        }

        // Toggle action buttons disable state
        ccSetButtonDisabled(actionButton1, "NFSDetailsForm", buttonDisabled);
        ccSetButtonDisabled(actionButton2, "NFSDetailsForm", buttonDisabled);
        
        ccSetDropDownMenuOptionDisabled(
            actionMenu, 'NFSDetailsForm', unshareDisabled, 2);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, 'NFSDetailsForm', shareDisabled, 1);
    }

    function showConfirmMsg(key) {
        var str = document.NFSDetailsForm.elements
            ["NFSDetails.ConfirmMessageHiddenField"].value;
        var msgArray = str.split("###");
        
        var str1 = msgArray[0];
        var str2 = msgArray[1];
        
        if (key == 1) {
            if (!confirm(str1)) {
                return false;
            } else {
                return true;
            }
        } else if (key == 2) {
            if (!confirm(str2)) {
                return false;
            } else {
                return true;
            }
        }
        return false;
    }
    
    function getServerKey() {
        return (document.NFSDetailsForm.elements[
            "NFSDetails.ServerNameHiddenField"].value);
    }
     
    function handlePathMenuChange(menu) {
        var text = "/";
        if (menu.value != "/") {
            text = menu.value;
        }

        document.NFSDetailsForm.elements[
            "NFSDetails.NFSShareAddView.pathChooser.browsetextfield"].value
        = text;
    }

    /* Invoked when the Browse button is clicked in Add */
    function getFileChooserParams() {
        // get client side params
        var text = document.NFSDetailsForm.elements[
            "NFSDetails.NFSShareAddView.pathChooser.browsetextfield"].value;
        if (text == "") text = "/";
        return "&currentDirParam=" + text + "&homeDirParam=" + text;
    }

    /* Invoke when Cancel button is clicked in Add/Edit */
    function cancel() {
        var url = "/samqfsui/fs/NFSDetails.jsp?SERVER_NAME=" + getServerKey();
        window.location.href = url;
    }

    /* Invoked when Save button is clicked in Edit */
    function confirmEmptyRootAccessList() {
        var prefix = "NFSDetails.NFSShareEditView.";
        var tf     = document.NFSDetailsForm;
        var rootAccess = trim(tf.elements[prefix + "rootAccessValue"].value);
        var rootChk = tf.elements[prefix + "rootCheckBox"];
        var message = tf.elements[prefix + "ConfirmMessage"].value;

        if (rootChk && rootChk.checked) {
            if (isEmpty(rootAccess)) {
                if (!confirm(message)) {
                    // setFocus to Root access list
                    var rootAccObj  = tf.elements[prefix + "rootAccessValue"];
                    rootAccObj.focus();
                    return false;
                } else {
                    return true;
                }
            }
        }

        return true;
    }

