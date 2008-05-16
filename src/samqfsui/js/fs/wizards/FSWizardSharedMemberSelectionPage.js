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

// ident	$Id: FSWizardSharedMemberSelectionPage.js,v 1.3 2008/05/16 19:39:14 am143972 Exp $

/** 
 * This is the javascript file of New File System Wizard Select Shared Member Page
 */

    WizardWindow_Wizard.pageInit = wizardPageInit;
    WizardWindow_Wizard.nextClicked = myNextClicked;

    var tf = document.wizWinForm;
    var internalCounter = 0;
    var initialCounter  = -1;
    var totalItems = -1;
    var tokenString = "";

    var viewNameKey = "PageView";
    var viewName = "";

    var NOVAL = "---";
    var NOVAL_LABEL = "---";


    function myNextClicked() {
        setTimeout("alert(getMessage())", 7000);
        return true;
    }

    function wizardNextClicked() {
        return true;
    }

    function wizardPageInit() {
        var disabled = false;

        if (viewName == "") {
            getPageViewName();
        }

        var string = tf.elements[viewName + ".errorOccur"];
        var error = string.value;

        if (error == "exception") {
            disabled = true;
        }

        populateSecondaryIPDropDown();

        WizardWindow_Wizard.setNextButtonDisabled(disabled, null);
        WizardWindow_Wizard.setPreviousButtonDisabled(disabled, null);
    }

    function getPageViewName() {
        var e = tf.elements[0];
        var viewNameIndex = e.name.indexOf(viewNameKey);
        if (viewNameIndex != -1) {
            viewName = e.name.substr(0, viewNameIndex + viewNameKey.length);
        }
    }

    function resetSecondaryIPDropDown(dropDown) {
       for (i = dropDown.options.length; i >= 1; i--) {
            dropDown.options[i] = null;
       }
    }

    function populateSecondaryIPDropDown() {
        var ips = tf.elements[viewName + ".ipAddresses"].value;
        var ipArray = ips.split(";");
        var secDropDown = tf.elements[viewName + ".secServerIP"];
        var primaryValue = tf.elements[viewName + ".serverIP"].value;
        var selectedSecIPValue =
            tf.elements[viewName + ".selectedSecondaryIP"].value;

        resetSecondaryIPDropDown(secDropDown);
        secDropDown.options[0] = new Option(NOVAL_LABEL, NOVAL);
        for (var i = 0; i < ipArray.length; i++) {
            // Skip what primary ip has selected
            if (primaryValue == ipArray[i]) {
                continue;
            }

            var currentLength = secDropDown.length;
            secDropDown.options[currentLength] =
                new Option(ipArray[i], ipArray[i]);
            if (selectedSecIPValue == ipArray[i]) {
                secDropDown.options[currentLength].selected = true;
            }
        }
    }

    function extractNumber(targetString) {
        var start = 0;
        var end   = 0;
        for (i = 0; i < targetString.length; i++) {
            if (targetString.charAt(i) == '[') {
                start = i;
            } else if (targetString.charAt(i) == ']') {
                end = i;
                break;
            }
        }
        return parseInt(targetString.substring(start + 1, end));
    }

    function populateTableSecondaryIPDropDown(primaryDropDown) {
        var items = "";
        for (i = primaryDropDown.options.length - 1; i >= 0; i--) {
            if (items != "") {
                items += ";";
            }
            items = items + primaryDropDown.options[i].value;
        }
        var tiledNumber = extractNumber(primaryDropDown.name);
        var secDropDown = tf.elements[viewName +
            ".FSWizardSharedMemberSelectionPageTiledView[" +
            tiledNumber + "].secondaryIPTextField"];
        resetSecondaryIPDropDown(secDropDown);
        var ipArray = items.split(";");
        secDropDown.options[0] = new Option(NOVAL_LABEL, NOVAL);
        for (var i = 0; i < ipArray.length; i++) {
            // Skip what primary ip has selected
            if (primaryDropDown.value == ipArray[i]) {
                continue;
            }
            var currentLength = secDropDown.length;
                secDropDown.options[currentLength] =
                    new Option(ipArray[i], ipArray[i]);
        }
    }

    function handleTableSelection(field) {
        if (viewName == "") {
            getPageViewName();
        }

        if (tokenString == "") {
            tokenString = tf.elements[viewName + ".initSelected"].value;
            var myArray = tokenString.split(",");

            initialCounter = myArray[0] - 0;
            totalItems     = myArray[1] - 0;
        }

        var showValue = 0;

        // Check if the field is select all / deselect all
        // Set counter to totalItems if the field is select all
        // or set the counter to zero if the field is deselect all
        // Also, check if the field type is checkbox to ensure the
        // add/substract one if what we intend to issue

        if (field.name == viewName + ".DeviceSelectionTable.SelectAllHref") {
            showValue = totalItems;
            internalCounter = totalItems;
            initialCounter  = 0;
        } else if (field.name == viewName + ".DeviceSelectionTable.DeselectAllHref") {
            showValue = 0;
            internalCounter = 0;
            initialCounter  = 0;
        } else if (field.type == "checkbox") {
            if (field.checked) {
                internalCounter += 1;
            } else {
                internalCounter -= 1;
            }

            showValue = initialCounter + internalCounter;
        }

        tf.elements[viewName + ".counter"].value =
            showValue;
    }

    function getMessage() {
        return document.wizWinForm.elements[
            "WizardWindow.Wizard.FSWizardSharedMemberSelectionPageView." +
            "HiddenMessage"].value;
    }


    function toggleOthers(field) {
        var myFieldNameArray = field.name.split(".");
        var checkBoxName = myFieldNameArray[4];

        var componentPrefix =
            myFieldNameArray[0] + "." + myFieldNameArray[1] + "." +
            myFieldNameArray[2] + "." + myFieldNameArray[3] + ".";

        var primaryDropDownName = componentPrefix + "primaryIPTextField";
        var secondaryDropDownName = componentPrefix + "secondaryIPTextField";

        var sharedClient =
            document.wizWinForm.elements[
                componentPrefix + "clientTextField"];
        var potentialMDS =
            document.wizWinForm.elements[
                componentPrefix + "potentialMetadataServerTextField"];

        if (sharedClient.checked || potentialMDS.checked) {
            // if either checkbox is checked, enable both drop down menus
            ccSetDropDownMenuDisabled(
                primaryDropDownName, "wizWinForm", false);
            ccSetDropDownMenuDisabled(
                secondaryDropDownName, "wizWinForm", false);
        } else {
            // disable if both check boxes are not checked
            ccSetDropDownMenuDisabled(
                primaryDropDownName, "wizWinForm", true);
            ccSetDropDownMenuDisabled(
                secondaryDropDownName, "wizWinForm", true);
        }
        if (field.checked) {
            if (checkBoxName == "clientTextField" && potentialMDS.checked) {
                // if shared client is checked, uncheck potentialMDS
                potentialMDS.checked = false;
            } else if (checkBoxName == "potentialMetadataServerTextField"
                && sharedClient.checked) {
                // if potentialMDS is checked, uncheck shared client
                sharedClient.checked = false;
            }
        }

    }

