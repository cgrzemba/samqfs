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

// ident    $Id: FileBrowser.js,v 1.13 2008/05/16 19:39:13 am143972 Exp $

/**
 * This is the javascript file for the File Brwoser Page
 */

    var modelIndex      = -1;
    var uniqueWindowKey = 1;
    var prefix          = "FileBrowser.FileBrowserView.";
    var prefixTiled     = prefix + "tiled_view[";
    var selectedFile    = "";

    /**
     * To return a boolean on the permission the user has.
     */
    function hasPermission() {
        var role = document.FileBrowserForm.elements["FileBrowser.Role"].value;
        return ("FILE" == role);
    }

    function handlePathMenuChange(menu) {
        var text = "/";
        if (menu.value != "" && menu.value != "/") {
            text = menu.value;
        }

        document.FileBrowserForm.elements["FileBrowser.Dir"].value = text;
    }

    function launchFilterPopup(field) {
        var formName      = field.form.name;
        var returnValue   = "FileBrowser.filterReturnValue";
        var commandChild  = "FileBrowser.filterHandler=";
        var pageTitleText =
            field.form.elements["FileBrowser.filterPageTitle"].value;
        var promptText = field.form.elements["FileBrowser.filterPrompt"].value;
        var loadValue  = field.form.elements["FileBrowser.filterValue"].value;

        return showFilterSettingsPopup(
            formName,
            returnValue,
            commandChild,
            pageTitleText,
            promptText,
            loadValue);
    }

    function launchStagePopup(field) {
        var myForm      = document.FileBrowserForm;
        var fileNames   = field.form.elements[
            "FileBrowser.fileNames"].value.split(";");
        var fileToStage = fileNames[modelIndex];

        var prefixWithIndex = prefixTiled + modelIndex + "].";

        // retrieve if the entry is a directory or a file
        var hidden    = myForm.elements[prefixWithIndex + "HiddenInfo"].value;
        var hiddenArr = hidden.split("###");
        var isDir     = hiddenArr[2] == "true";
        var snapPath    = myForm.elements["FileBrowser.snapPath"].value;
        if ("live" == snapPath) snapPath = "";

        var params = "&filetostage=" + fileToStage +
                     "&isdir=" + isDir +
                     "&fsname=" + getFSInfo(0) +
                     "&mountpoint=" + getFSInfo(1) +
                     "&snappath=" + snapPath;
        var name = "stage_file_popup";
        var uri = "/fs/StagePopup";

        // need to collect parameters : fs name, file name at the very least
        var win = launchPopup(uri, name, null, null, encodeURI(params));
        win.focus();
        return false;
    }

    function launchRestorePopup(field, entire) {
        var myForm      = document.FileBrowserForm;
        var snapPath    = myForm.elements["FileBrowser.snapPath"].value;

        var params;
        var name = "restore_file_popup";
        var uri = "/fs/RestorePopup";

        // Restore entire recovery point
        if (entire) {
            params = "&filetorestore=" + "##all##" +
                     "&fsname=" + getFSInfo(0) +
                     "&mountpoint=" + getFSInfo(1) +
                     "&snappath=" + snapPath;
        } else {
            var fileNames   = field.form.elements[
                "FileBrowser.fileNames"].value.split(";");
            var fileToRestore = fileNames[modelIndex];

            var prefixWithIndex = prefixTiled + modelIndex + "].";

            // retrieve if the entry is a directory or a file
            var hidden = myForm.elements[prefixWithIndex + "HiddenInfo"].value;
            var hiddenArr = hidden.split("###");
            var isDir     = hiddenArr[2] == "true";

            var snapPath    = myForm.elements["FileBrowser.snapPath"].value;
            if ("live" == snapPath) snapPath = "";

            params = "&filetorestore=" + fileToRestore +
                     "&fsname=" + getFSInfo(0) +
                     "&mountpoint=" + getFSInfo(1) +
                     "&isdir=" + isDir +
                     "&snappath=" + snapPath;
        }

        // need to collect parameters : fs name, file name at the very least
        var win = launchPopup(uri, name, null, null, encodeURI(params));
        win.focus();
        return false;
    }

    /**
     * getFSInfo returns either the file system name, or its mount point
     * 0 => fs name
     * 1 => fs mount point
     */
    function getFSInfo(value) {
        var fsInfo = document.FileBrowserForm.elements[
            "FileBrowser.fsInfo"].value;
        var fsInfoArray = fsInfo.split("###");

        return fsInfoArray[parseInt(value)];
    }

    function launchViewDetailsPopup(field) {
        var myForm      = document.FileBrowserForm;
        var isArchiving = field.form.elements[
            "FileBrowser.isArchiving"].value;
        var fileNames = field.form.elements[
            "FileBrowser.fileNames"].value.split(";");
        var fileToView = fileNames[modelIndex];

        // retrieve if the entry is a directory or a file
        var prefixWithIndex = prefixTiled + modelIndex + "].";
        var hidden    = myForm.elements[prefixWithIndex + "HiddenInfo"].value;
        var hiddenArr = hidden.split("###");
        var isDir     = hiddenArr[2] == "true";

        var snapPath    = myForm.elements["FileBrowser.snapPath"].value;
        if ("live" == snapPath) snapPath = "";

        var params = "&fsname=" + getFSInfo(0) +
                     "&mountpoint=" + getFSInfo(1) +
                     "&filetoview=" + fileToView +
                     "&isdir=" + isDir +
                     "&isarchiving=" + isArchiving +
                     "&snappath=" + snapPath;
        var name = "file_details_popup_" + uniqueWindowKey;
        uniqueWindowKey++;
        var uri = "/fs/FileDetailsPopup";

        // need to collect parameters : fs name, file name at the very least
        var win = launchPopup(uri, name, null, SIZE_DETAILS, params);
        win.focus();
        return false;
    }

    /* helper function to retrieve element by id */
    function $(id) {
        return document.getElementById(id);
    }

    function handleToggleComponent() {
        var supportToggle =
            document.FileBrowserForm.elements[
                "FileBrowser.supportRecovery"].value;
        if (supportToggle != "true") {
            $("toggleDiv").style.visibility="hidden";
        } else {
            $("toggleDiv").style.visibility="";
        }
    }

    function handleToggleRadio(field) {
        if (field.value == "recovery") {
            ccSetDropDownMenuDisabled(
                "FileBrowser.RecoveryMenu",
                field.form.name,
                false);
        } else {
            ccSetDropDownMenuDisabled(
                "FileBrowser.RecoveryMenu",
                field.form.name,
                true);

            // set the action and submit the form
            var handler = "FileBrowser.LiveDataHref";
            var pageSession = field.form.elements["jato.pageSession"].value;
            field.form.action =
                field.form.action + "?" + handler +
                "=&jato.pageSession=" + pageSession;
            field.form.submit();
        }
    }

    function handleUnmountedFS() {
        var myForm  = document.FileBrowserForm;
        var message = myForm.elements["FileBrowser.MountMessage"].value;
        if (message != "") {
            if (confirm(message)) {
                // refresh page and mount fs
                var handler = "FileBrowser.MountHref";
                var pageSession = myForm.elements["jato.pageSession"].value;
                myForm.action =
                    myForm.action + "?" + handler +
                    "=&jato.pageSession=" + pageSession;
                myForm.submit();
            }
        }
    }

    function handleEnterBasePath(field) {
        // set the action and submit the form
        var handler = "FileBrowser.Apply";
        var pageSession = field.form.elements["jato.pageSession"].value;
        field.form.action =
            field.form.action + "?" + handler +
            "=&jato.pageSession=" + pageSession;
        field.form.submit();
    }

    function handleFileSelection(field) {
        var disabled      = true;
        var stageDisabled = true;

        var buttonView    = prefix + "ViewDetails";
        var actionMenu    = prefix + "ActionMenu";
        var formName      = "FileBrowserForm";
        var myForm        = document.FileBrowserForm;

        // checkbox or radioButton for row selection
        if (field != null) {
            // Do not update selectedIndex here .. not until checking if
            // DeselectAllHref is clicked

            var elementName   = field.name;
            if (elementName != (prefix + "FilesTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                disabled = false;
                modelIndex = field.value;
            }
        } else {
            // User uses BACK button
            var selectionName = prefix + "FilesTable.Selection";
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

        var isArchiving      = false;
        var online           = false;
        var noValidCopy      = false;
        var isDir            = false;

        var modeRadio      = myForm.elements["FileBrowser.ModeRadio"];
        var isRecoveryMode = modeRadio[1].checked;

        if (modelIndex != -1) {
            // If current directory is not even an archiving file system,
            // Archive/Stage/Release should not be allowed
            isArchiving =
                myForm.elements["FileBrowser.isArchiving"].value == "true";

            var prefixWithIndex = prefixTiled + modelIndex + "].";

            // check if the file/directory is online
            var hidden = myForm.elements[prefixWithIndex + "HiddenInfo"].value;
            var hiddenArr = hidden.split("###");
            online = hiddenArr[1] == 1;

            // check if there are any valid copies
            noValidCopy = hiddenArr[0] == "true";

            // If entry is a directory, allow Stage to pop up
            // directory is always online
            isDir = hiddenArr[2] == "true";

            // save selectedFile for javascript pop up and java handler
            var fileNames   = myForm.elements[
                                "FileBrowser.fileNames"].value.split(";");
            selectedFile    = fileNames[modelIndex] + "###" + hiddenArr[2];
            myForm.elements["FileBrowser.SelectedFile"].value = selectedFile;
        }

        // Archive
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName,
            disabled || !isArchiving || isRecoveryMode || !hasPermission(),
            1);

        // Release
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName,
            disabled || !isArchiving || isRecoveryMode || (!online && !isDir)
                                     || !hasPermission(),
            2);

        // Stage
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName,
            disabled || !isArchiving || (online && !isDir)
                     || (noValidCopy && !isDir) || isRecoveryMode
                     || !hasPermission(),
            3);

        // Restore
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName,
            disabled || !isRecoveryMode || !hasPermission(),
            4);

        // Restore Entire Recovery Point
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, !isRecoveryMode || !hasPermission(),
            5);

        // View Details
        ccSetButtonDisabled(
            buttonView, "FileBrowserForm", disabled);
    }
