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

// ident    $Id: FSDetails.js,v 1.3 2008/05/16 19:39:13 am143972 Exp $

/** 
 * This is the javascript file for File System Details Page
 */

    function getDropDownSelectedItem() {
        var index = -1;
        var myForm = document.FSDetailsForm;
        var dropDownMenu =
            myForm.elements["FSDetails.FSDetailsView.PageActionsMenu"];

        if (dropDownMenu != null) {
            index = dropDownMenu.value;
        }

        return index;
    }

    function resetDropDownMenu() {
        var myForm = document.FSDetailsForm;
        var dropDownMenu =
            myForm.elements["FSDetails.FSDetailsView.PageActionsMenu"];

        // reset to Operation
        dropDownMenu.options[0].selected = true;
    }
    
    function getServerKey() {
        return document.FSDetailsForm.elements["FSDetails.ServerName"].value;
    }

    function getClientParams() {
        var myForm = document.FSDetailsForm;
        var fsName = myForm.elements["FSDetails.FSDetailsView.fsName"].value;
        var serverName = getServerKey();
        var clientParams =
            "fsNameParam=" + fsName + "&" + "serverNameParam=" + serverName;
        return clientParams;
    }

    function enableComponents() {
        var myForm = document.FSDetailsForm;
        var enabledMenuOptions =
            myForm.elements[
                "FSDetails.FSDetailsView.HiddenDynamicMenuOptions"].value;

        var actionMenu = "FSDetails.FSDetailsView.PageActionsMenu";
        var formName      = "FSDetailsForm";

        var menuOptionStates = new Array();
        menuOptionStates[0] = true; // not used
        menuOptionStates[1] = true; // fsck
        menuOptionStates[2] = true; // mount
        menuOptionStates[3] = true; // umount
        menuOptionStates[4] = true; // delete
        menuOptionStates[5] = true; // archive activities
        menuOptionStates[6] = true; // schedule recovery points
    
        var options =
            myForm.elements[
                "FSDetails.FSDetailsView.HiddenDynamicMenuOptions"].value;
        var optionsArray = options.split(",");
        var optionsSize = optionsArray.length;

        for (i = 0; i < optionsSize; i++) {
            var index = parseInt(optionsArray[i]);
            if (index > 0 && index < menuOptionStates.length) {
                menuOptionStates[index] = false;
            }
        }

        for (i = 1; i < menuOptionStates.length; i++) {
            ccSetDropDownMenuOptionDisabled(
                actionMenu,
                formName,
                menuOptionStates[i],
                i);
        }
    }

    function showConfirmMsg(key) {
        var str =
            document.FSDetailsForm.elements["FSDetails.ConfirmMessages"].value;
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
        } else {
            return false; // this case should never be used
        }
    }
