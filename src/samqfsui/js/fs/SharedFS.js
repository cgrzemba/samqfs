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

// ident	$Id: SharedFS.js,v 1.9 2009/03/04 21:54:40 ronaldso Exp $


// Used by drop down menu in summary page
function handleSummaryMenu(menu) {
    var selectedValue = menu.value;

    if (selectedValue == "unmount") {
        var message = document.getElementById(
                        "SharedFSForm:ConfirmUnmountFS").value;
        if (!confirm(message)) {
            resetDropDownMenu(menu);
            return false;
        }
    }
    return true;
}

function getClientTable() {
    return document.getElementById(
             "SharedFSClientForm:pageTitle:tableClientSummary");
}

function initClientTableRows() {
    getClientTable().initAllRows();
}

function handleButtonRemove() {
    if (handleOperation(true) == false) {
        return false;
    } else if (!confirm(getMessage(2))) {
        return false;
    } else {
        return true;
    }
}

function handleClientDropDownMenu(menu) {
    var selectedValue = menu.value;

    if (selectedValue == "editmo") {
        // Edit Mount Option only allows one selection
        if (handleOperation(false) == false) {
            resetDropDownMenu(menu);
            return false;
        }
    } else {
        // Mount / Unmount / Enable Access / Disable Access

        // Make sure user selects something before disabling access hosts
        if (handleOperation(true) == false) {
            resetDropDownMenu(menu);
            return false;
        }

        // show confirm messages if needed
        if (selectedValue == "unmount") {
            if (!confirm(getMessage(4))) {
                resetDropDownMenu(menu);
                return false;
            } else {
                return true;
            }
        } else if (selectedValue == "disableaccess") {
            if (!confirm(getMessage(3))) {
                resetDropDownMenu(menu);
                return false;
            } else {
                return true;
            }
        }
    }
    return true;
}

function handleOperation(allowMultiple) {
    var selections = getClientTable().getAllSelectedRowsCount();
    if (selections <= 0) {
        alert(getMessage(1));
        return false;
    } else if (!allowMultiple && selections > 1) {
        alert(getMessage(0));
        return false;
    } else {
        return true;
    }
}

function getServerName(formName) {
    return document.getElementById(formName + ":" + "hiddenServerName").value;
}

function getFSName(formName) {
    return document.getElementById(formName + ":" + "hiddenFSName").value;
}

/**
 * Remove the following method when the new wizard (Add Clients) is ready
 */
function launchAddClientsWizard(button) {
    var infoArr = button.name.split(":");
    var formName = infoArr[0];
    var fsName = getFSName(formName);
    var serverName = getServerName(formName);
    var params = "&SAMQFS_FS_NAME=" + fsName;
    var name = "addclients_" + fsName;
    var uri = "/faces/jsp/fs/wizards/AddClientsWizard.jsp";

    var win = launchPopup(uri, name, serverName, SIZE_WIZARD, params);

    win.focus();

    return false;
}

function getMountPoint(formName) {
    return document.getElementById(formName + ":" + "hiddenMountPoint").value;
}

function isMDSMounted(formName) {
    return document.getElementById(formName + ":" + "hiddenIsMDSMounted").value;
}

// REMOVE ENDS HERE ///////////////////////////////////////////////////////////

// Retrieve error message.
// 0 ==> None selected
// 1 ==> More than one selected
// 2 ==> Confirm message for remove
// 3 ==> Confirm message for disable
// 4 ==> Confirm message for unmount
function getMessage(choice) {
    var thisFormId = 'SharedFSClientForm';

    switch (choice) {
        case 0:
            return document.getElementById(
                      thisFormId + ":" + "NoMultipleOp").value;
        case 1:
            return document.getElementById(
                      thisFormId + ":" + "NoneSelected").value;
        case 2:
            return document.getElementById(
                      thisFormId + ":" + "ConfirmRemove").value;
        case 3:
            return document.getElementById(
                      thisFormId + ":" + "ConfirmDisable").value;
        case 4:
            return document.getElementById(
                      thisFormId + ":" + "ConfirmUnmount").value;
        default:
            return "";
    }

}

// Resetting drop down menu to select top selection
function resetDropDownMenu(menu) {
    menu.options[0].selected = true;
}
