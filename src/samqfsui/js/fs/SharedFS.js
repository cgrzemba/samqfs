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

// ident	$Id: SharedFS.js,v 1.3 2008/07/16 23:45:03 ronaldso Exp $


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

function getSNTable() {
    return document.getElementById(
             "SharedFSStorageNodeForm:pageTitle:tableStorageNodeSummary");
}

function initClientTableRows() {
    getClientTable().initAllRows();
}

function initSnTableRows() {
    getSNTable().initAllRows();
}

function handleButtonRemove(client) {
    if (handleOperation(client, true) == false) {
        return false;
    } else if (client == 1 && !confirm(getMessage(1, 2))) {
        return false;
    } else if (client == 0 && !confirm(getMessage(0, 5))) {
        return false;
    } else {
        return true;
    }
}

function handleClientDropDownMenu(menu) {
    var selectedValue = menu.value;

    if (selectedValue == "editmo") {
        // Edit Mount Option only allows one selection
        if (handleOperation(1, false) == false) {
            resetDropDownMenu(menu);
            return false;
        }
    } else {
        // show confirm messages if needed
        if (selectedValue == "unmount") {
            if (!confirm(getMessage(1, 4))) {
                resetDropDownMenu(menu);
                return false;
            } else {
                return true;
            }
        } else if (selectedValue == "disableaccess") {
            if (!confirm(getMessage(1, 3))) {
                resetDropDownMenu(menu);
                return false;
            } else {
                return true;
            }
        }
        // Otherwise everything else allows multiple selection
        if (handleOperation(1, true) == false) {
            resetDropDownMenu(menu);
            return false;
        }
    }
    return true;
}

function handleSnDropDownMenu(menu) {
    var selectedValue = menu.value;
    if (selectedValue == "editmo") {
        // Edit Mount Option only allows one selection
        if (handleOperation(0, false) == false) {
            resetDropDownMenu(menu);
            return false;
        }
    } else {
        // show confirm messages if needed
        if (selectedValue == "unmount") {
            if (!confirm(getMessage(0, 7))) {
                resetDropDownMenu(menu);
                return false;
            } else {
                return true;
            }
        } else if (selectedValue == "disablealloc") {
            if (!confirm(getMessage(0, 6))) {
                resetDropDownMenu(menu);
                return false;
            } else {
                return true;
            }
        } else if (selectedValue == "clearfault") {
            if (!confirm(getMessage(0, 8))) {
                resetDropDownMenu(menu);
                return false;
            } else {
                return true;
            }
        }
        // Otherwise everything else allows multiple selection
        if (handleOperation(0, true) == false) {
            resetDropDownMenu(menu);
            return false;
        }
    }
    return true;
}

function handleOperation(client, allowMultiple) {
    var selections =
        client == 1 ?
          getClientTable().getAllSelectedRowsCount() :
          getSNTable().getAllSelectedRowsCount();
    if (selections <= 0) {
        alert(getMessage(client, 1));
        return false;
    } else if (!allowMultiple && selections > 1) {
        alert(getMessage(client, 0));
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

function launchAddStorageNodeWizard(button) {
    var infoArr = button.name.split(":");
    var formName = infoArr[0];
    var fsName = getFSName(formName);
    var params = "&SAMQFS_FS_NAME=" + fsName;
    var name = "addsn_" + fsName;
    var uri = "/faces/jsp/fs/wizards/AddStorageNodeWizard.jsp";

    var win = launchPopup(
                uri, name, getServerName(formName),
                SIZE_WIZARD, encodeURI(params));
    win.focus();
    return false;

}

/**
 * Remove the following method when the new wizard (Add Clients) is ready
 */
function launchAddClientsWizard(button) {
    var infoArr = button.name.split(":");
    var formName = infoArr[0];

    alert("Coming soon!");
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
function getMessage(page, choice) {
    var thisFormId;
    if (page == 1) {
        thisFormId = 'SharedFSClientForm';
    } else {
        thisFormId = 'SharedFSStorageNodeForm';
    }
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
        case 5:
            return document.getElementById(
                      thisFormId + ":" + "ConfirmRemoveSn").value;
        case 6:
            return document.getElementById(
                      thisFormId + ":" + "ConfirmDisableSn").value;
        case 7:
            return document.getElementById(
                      thisFormId + ":" + "ConfirmUnmountSn").value;
        case 8:
            return document.getElementById(
                      thisFormId + ":" + "ConfirmClearFaultSn").value;
        default:
            return "";
    }

}

// Resetting drop down menu to select top selection
function resetDropDownMenu(menu) {
    menu.options[0].selected = true;
}
