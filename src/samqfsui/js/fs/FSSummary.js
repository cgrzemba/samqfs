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

// ident	$Id: FSSummary.js,v 1.26 2008/07/03 00:04:28 ronaldso Exp $

/**
 * This is the javascript file of File System Summary page
 */

// selected row index
var modelIndex = -1;
var initialized = false;
var versionNumber;

var formName = "FSSummaryForm";

// GUI components that will be dynamically enabled/disabled on client side

var samfsButtons = new Array();
samfsButtons[0] = "FSSummary.FileSystemSummaryView.ViewPolicyButton";
samfsButtons[1] = "FSSummary.FileSystemSummaryView.ViewFilesButton";
samfsButtons[2] = "FSSummary.FileSystemSummaryView.SamQFSWizardNewPolicyButton";

var qfsButtons = new Array();
qfsButtons[0] = "FSSummary.FileSystemSummaryView.ViewFilesButton";

var dynamicButtons = samfsButtons;

var dynamicActionMenu = "FSSummary.FileSystemSummaryView.ActionMenu";

var MAX_MENU_OPTIONS     = 10;
var MAX_MENU_OPTIONS_qfs = 8;

// NOTE: this field = num of options in dropdown menu minus the default one
var maxMenuOptions;

var dynamicButtonElements;
var dynamicButtonHiddenFields;
var dynamicMenuElement;
var dynamicMenuOptionHiddenFields;

function initialize() {
    var myForm   = document.FSSummaryForm;

    var licenseType = myForm.elements["FSSummary.LicenseTypeHiddenField"].value;

    if (licenseType == "QFS") {
        dynamicButtons = qfsButtons;
        maxMenuOptions = MAX_MENU_OPTIONS_qfs;
    } else {
        maxMenuOptions = MAX_MENU_OPTIONS;
    }

    dynamicButtonElements = new Array();
    dynamicButtonHiddenFields = new Array();
    dynamicMenuElements = new Array();
    dynamicMenuOptionHiddenFields = new Array();

    for (i = 0, k = 0, m = 0, n = 0; i < myForm.elements.length; i++) {
        var e = myForm.elements[i];
        for (j = 0; j < dynamicButtons.length; j++) {
            if (e.name == dynamicButtons[j]) {
                dynamicButtonElements[k] = e;
                break;
            } else if (e.name == (dynamicButtons[j] + ".DisabledHiddenField")) {
                dynamicButtonHiddenFields[k] = e;
                k++;
                break;
            }
        }

        if (e.name == dynamicActionMenu) {
            dynamicMenuElements[m] = e;
            m++;
        } else if (e.name.indexOf(
            dynamicActionMenu + ".OptionDisabledHiddenField") != -1) {
            dynamicMenuOptionHiddenFields[n] = e;
            n++;
        }

        if (n >= 2 * maxMenuOptions) {
            // assume there're always two action menu elements, and all buttons
            // appear before action menu
            initialized = true;
            break;
        }
    }
}

function toggleDisabledState(field) {

    var buttonStates = new Array();
    buttonStates[0] = true;
    buttonStates[1] = true;
    buttonStates[2] = true;
    buttonStates[3] = true;
    buttonStates[4] = true;
    buttonStates[5] = true;

    var menuOptionStates = new Array();
    menuOptionStates[0] = true; // not used
    menuOptionStates[1] = true;
    menuOptionStates[2] = true;
    menuOptionStates[3] = true;
    menuOptionStates[4] = true;
    menuOptionStates[5] = true;
    menuOptionStates[6] = true;
    menuOptionStates[7] = true;
    menuOptionStates[8] = true;
    menuOptionStates[9] = true;

    var myForm   = document.FSSummaryForm;
    var prefix      = "FSSummary.FileSystemSummaryView.";
    var prefixTiled = prefix + "FileSystemSummaryTiledView[";

    // checkbox or radioButton for row selection
    var selectionName = prefix + "FileSystemSummaryTable.Selection";

    if (field != null) {
        if (field.name == "FSSummary.FileSystemSummaryView.FileSystemSummaryTable.DeselectAllHref") {
            modelIndex = -1;
        } else if (field.name.indexOf(selectionName) != -1) {
            modelIndex = field.value;
        }
    } else {
        // check if there is row selected, since if user click
        // cancel to close the popup window, the actiontable row
        // selection still should be retained
        // Get the selected row index
        for (i = 0; i < myForm.elements.length; i++) {
            var e = myForm.elements[i];
            if (e.name.indexOf(selectionName) != -1) {
                if (e.checked) {
                    disabled = false;
                    modelIndex = myForm.elements[i].value;
                    break;
                }
            }
        }
    }

    if (!initialized) {
        initialize();
    }

    // get state values calculated on serverside and stored in the hidden field
    var enabledButtons = prefixTiled + modelIndex + "].HiddenDynamicButtons";
    var enabledMenuOptions = prefixTiled + modelIndex + "].HiddenDynamicMenuOptions";

    if (modelIndex != -1) {
      var buttons = myForm.elements[enabledButtons].value;
        var buttonArray = buttons.split(",");
        var buttonSize = buttonArray.length;

        for (var i = 0; i < buttonSize; i++) {
            var index = parseInt(buttonArray[i]);
            if (index >= 0 && index < dynamicButtons.length) {
                buttonStates[index] = false;
                buttonStates[index + dynamicButtons.length] = false;
            }
        }

        var options = myForm.elements[enabledMenuOptions].value;
        var optionsArray = options.split(",");
        var optionsSize = optionsArray.length;

        for (var i = 0; i < optionsSize; i++) {
            var index = parseInt(optionsArray[i]);
            if (index > 0 && index < menuOptionStates.length) {
                menuOptionStates[index] = false;
            }
        }
    }

    // Toggle action buttons disable state
    for (i = 0; i < dynamicButtonElements.length; i++) {
        //dynamicButtonElements[i].disabled = buttonStates[i];
        ccSetButtonState(
            dynamicButtonElements[i],
            dynamicButtonHiddenFields[i],
            null,
            null,
            buttonStates[i]);
    }
    for (i = 0; i < dynamicMenuElements.length; i++) {
        for (j = 1; j < maxMenuOptions; j++) {
            ccSetDropDownMenuOptionState(
                dynamicMenuElements[i],
                dynamicMenuOptionHiddenFields[i * maxMenuOptions + j],
                null,
                null,
                menuOptionStates[j],
                j);

        }
    }
}

function getSelectedFSName() {
    var fsName = null;

    if (modelIndex != -1) {
        var myForm        = document.FSSummaryForm;
        var prefix        = "FSSummary.FileSystemSummaryView.";
        var prefixTiled   = prefix + "FileSystemSummaryTiledView[";

        var fsNameField = prefixTiled + modelIndex + "].FSHiddenField";
        fsName = myForm.elements[fsNameField].value;
    }
    return fsName;
}

function showConfirmMsg(key) {
    var fm = document.FSSummaryForm;
    var str1 = fm.elements["FSSummary.ConfirmMsg1"].value;
    var str2 = fm.elements["FSSummary.ConfirmMsg2"].value;

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

function getServerKey() {
    return document.FSSummaryForm.elements["FSSummary.ServerName"].value;
}

function getClientParams() {
    var clientParams = null;
    var fsName = getSelectedFSName();
    if (fsName != null) {
        clientParams = "fsNameParam=" + fsName;
    }

    // not using top due to the use of HtmlUnit
    // var serverName = top.serverName;
    var serverName = getServerKey();
    clientParams = clientParams + "&serverNameParam=" + serverName;
    return clientParams;
}

function onClickGrowButton() {
    var hiddenGrowButton = document.FSSummaryForm.elements[
        "FSSummary.FileSystemSummaryView.SamQFSWizardGrowFSButton"];
    hiddenGrowButton.click();
}

function onClickShrinkButton() {
    var fsName = getSelectedFSName();
    var params = "&SAMQFS_FS_NAME=" + fsName;
    var name = "shrink_" + fsName;
    var uri = "/faces/jsp/fs/wizards/ShrinkFSWizard.jsp";

    var win = launchPopup(
                uri, name, getServerKey(), SIZE_WIZARD, encodeURI(params));
    win.focus();
    return false;
}

function handleDropDownOnChange(menu) {

    var value = parseInt(menu.value);

    switch (value) {
        // Check FS
        case 2:
            var fsNameParam = getClientParams();
            launchCheckFSPopup(menu, fsNameParam);
            resetDropDownMenu(menu);
            return false;

        // Unmount
        case 4:
            if (!showConfirmMsg(2)) {
                resetDropDownMenu(menu);
                return false;
            }
            break;

        // Grow
        case 5:
            onClickGrowButton();
            return false;

        // Delete
        case 6:
            if (!showConfirmMsg(1)) {
                resetDropDownMenu(menu);
                return false;
            }
            break;

        // Shrink
        case 9:
            onClickShrinkButton();
            resetDropDownMenu(menu);
            return false;

        default:
            return true;
    }
    return true;
}
