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

// ident    $Id: RecoveryPoints.js,v 1.11 2008/05/16 19:39:13 am143972 Exp $

/** 
 * This is the javascript file for Recovery Points Page
 */

    var modelIndex      = -1;
    var prefix          = "RecoveryPoints.RecoveryPointsView.";
    var prefixTiled     = prefix + "tiled_view[";

    /**
     * To return a boolean on the permission the user has.
     */
    function hasPermission() {
        var role =
            document.RecoveryPointsForm.elements["RecoveryPoints.Role"].value;
        return ("FILE" == role);
    }

    /* Invoked when the Browse button is clicked in Add */
    function getFileChooserParams() {
        // get client side params
        var text = document.RecoveryPointsForm.elements[
            "RecoveryPoints.selectSnapshotPath.pathHidden"].value;
        if (text == "") text = "/";
        return "&currentDirParam=" + text;
    }
    
    /* Confirm messages */
    /* 0 => Create Index */
    /* 1 => Delete Index */
    /* 2 => Delete Snapshot file */
    /* 3 => (Alert Message) Cannot retain file permanently if it is broken */

    function getMessage(choice) {
        var message =
        document.RecoveryPointsForm.elements["RecoveryPoints.Messages"].value;
        var messages = message.split("###");
        
        return messages[choice];
    }
    
    /**
     * Helper function to extract the selected file name from a list of names
     */
    function getSelectedFileName() {
        var nameList =
            document.RecoveryPointsForm.elements[prefix + "FileNames"].value;
        var nameArray = nameList.split("###");
        return nameArray[modelIndex];
    }

    function handleFileSelection(field) {
        var disabled      = true;

        var buttonCreate         = prefix + "createIndex";
        var buttonDelete         = prefix + "deleteIndex";
        var buttonDeleteSnapshot = prefix + "deleteDump";

        var formName      = "RecoveryPointsForm";
        var myForm        = document.RecoveryPointsForm;

        // checkbox or radioButton for row selection
        if (field != null) {
            // Do not update selectedIndex here .. not until checking if
            // DeselectAllHref is clicked

            var elementName   = field.name;
            if (elementName != (prefix + "RecoveryPointsTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                disabled = false;
                modelIndex = field.value;
            }
        } else {
            // User uses BACK button
            var selectionName = prefix + "RecoveryPointsTable.Selection";
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

        var indexed            = false;
        var processingOrBroken = false;
        var retainPerm         = false;

        if (modelIndex != -1) {               
            var prefixWithIndex = prefixTiled + modelIndex + "].";

            // check if entry is already indexed
            indexed =
                "true" ==
                    myForm.elements[prefixWithIndex + "indexed"].value;

            processingOrBroken =
                "true" ==
                    myForm.elements[prefixWithIndex + "processingOrBroken"].value;
            
            // check if entry is marked retained permanently
            retainPerm =
                "true" == 
                    myForm.elements[prefixWithIndex + "retentionValue"].value;
                    
            // assign selected file name into hidden field
            var selected = getSelectedFileName();
            myForm.elements[prefix + "SelectedFile"].value = selected;
        }

        // Create Index
        ccSetButtonDisabled(
            buttonCreate, "RecoveryPointsForm",
            disabled || indexed || processingOrBroken || !hasPermission());

        // Delete Index
        ccSetButtonDisabled(
            buttonDelete, "RecoveryPointsForm",
            disabled || !indexed || processingOrBroken || !hasPermission());

        // Delete Recovery Point
        ccSetButtonDisabled(
            buttonDeleteSnapshot, "RecoveryPointsForm",
            disabled || retainPerm || !hasPermission());
        
    }
    
    function onClickRetainPermanently(checkBoxObj) {
        var theForm = document.RecoveryPointsForm;
        modelIndex = getRowIndexFromName(checkBoxObj.name);
        var prefixWithIndex = prefixTiled + modelIndex + "].";

        var processingOrBroken = "true" ==
            theForm.elements[prefixWithIndex + "processingOrBroken"].value;

        if (processingOrBroken) {
            alert(getMessage(3));
            checkBoxObj.checked = false;
            return;
        }

        // assign necessary values to the retain helper hidden field
        var retentionValue =
            theForm.elements[prefixWithIndex + "retentionBox"].checked;
        var fileName = getSelectedFileName();
        var helper = theForm.elements["RecoveryPoints.RetainCheckBoxHelper"];
        helper.value = fileName + "###" + retentionValue;

        // submit the form
        theForm.action =
            theForm.action +
            "?RecoveryPoints.RecoveryPointsView.RetainHref=";
        theForm.submit();
    }

    function getServerKey() {
        return document.RecoveryPointsForm.elements[
            "RecoveryPoints.ServerName"].value;
    }

    function getFSName() {
        var menu = document.RecoveryPointsForm.elements[
                            "RecoveryPoints.currentFSValue"];
        return menu[menu.selectedIndex].value;
    }

    function getSelectedPath() {
        var menu = document.RecoveryPointsForm.elements[
                            "RecoveryPoints.currentSnapshotPathValue"];
        if (menu.selectedIndex == -1)
            return "";
        else
            return menu[menu.selectedIndex].value;
    }

    function handleCreateNow(field) {
        var param = "fsNameParam=" + getFSName() +
                    "&serverNameParam=" + getServerKey() +
                    "&selectedPath=" + getSelectedPath();
        launchDumpNowPopup(field, param);
    }
