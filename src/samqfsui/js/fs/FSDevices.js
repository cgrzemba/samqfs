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

// ident    $Id: FSDevices.js,v 1.4 2008/12/16 00:10:37 am143972 Exp $

/**
* This is the javascript file for the File System Devices View
*/

function getForm() {
    return document.forms[0];
}

function getPageName() {
    var strArray = getForm().action.split("/");
    return strArray[strArray.length - 1];
}

function getViewName() {
    return getPageName() + ".FSDevicesView";
}

function getTableName() {
    return "FSDevicesTable";
}

function handleButton(enable) {
    var prefix = getViewName() + "." +
    getTableName() + ".SelectionCheckbox";
    var theForm = getForm();
    var allDevices =
    theForm.elements[getViewName() + ".AllDevices"].value;
    var allArray = allDevices.split(",");
    var selected_devs = "";

    // loop through the check boxes and determine which ones are selected
    for (var i = 0; i < allArray.length; i++) {
        var check_box_name = prefix + i;
        var theCheckBox = theForm.elements[check_box_name];

        if (theCheckBox.checked) {
            selected_devs += allArray[i] + ",";
        }
    }

    var noMountMsg = theForm.elements[getViewName() + ".NoMountMsg"].value;
    var sharedClientMsg =
        theForm.elements[getViewName() + ".NoSharedClientMsg"].value;

    // Show error message if nothing is selected
    if (selected_devs.length == 0) {
        alert(theForm.elements[getViewName() + ".NoSelectionMsg"].value);
        return false;
        // Otherwise warn user that he/she is about to delete the selected
        // expressions
    } else if (noMountMsg.length != 0) {
        alert(noMountMsg);
        return false;
    } else if (sharedClientMsg.length != 0) {
        alert(sharedClientMsg);
        return false;
    } else {
        // Remove trailing semi-colon
        selected_devs =
        selected_devs.substring(0, selected_devs.length - 1);

        // Warn user if disable allocation is clicked
        if (!enable) {
            var result = window.confirm(getForm().elements[
                getViewName() + ".DisableMsg"].value);
            if (!result) {
                return false;
            }
        }

        theForm.elements[getViewName() + ".SelectedDevices"].value
            = selected_devs;
    }
    return true;
}
