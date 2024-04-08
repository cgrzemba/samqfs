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

// ident	$Id: LibraryDriveSummary.js,v 1.15 2008/12/16 00:10:38 am143972 Exp $

/**
 * This is the javascript file of Library Drive Summary Page
 */

    var selectedIndex = -1;
    var prefix        = "LibraryDriveSummary.LibraryDriveSummaryView.";
    var prefixTiled   = prefix + "LibraryDriveSummaryTiledView[";
    

    function showConfirmMsg(choice) {
        var str = document.LibraryDriveSummaryForm.elements
                ["LibraryDriveSummary.ConfirmMessage"].value;
        var strArray = str.split("###");
        if (!confirm(strArray[choice])) {
            return false;
        } else {
            return true;
        }
    }

    function toggleDisabledState(field) {
        var disabled = true, unloaddisabled = true;
        var formName = "LibraryDriveSummaryForm";
        var myForm   = document.LibraryDriveSummaryForm;

        var actionButton0 = prefix + "ChangeStateButton";
        var actionButton1 = prefix + "IdleButton";
        var actionButton2 = prefix + "UnloadButton";
        var actionButton3 = prefix + "CleanButton";

        // For ACSLS libraries that are installed in Version 4.5+ servers
        var actionMenu    = prefix + "ActionMenu";

        // checkbox or radioButton for row selection
        var selectionName = prefix + "LibraryDriveSummaryTable.Selection";

        if (field != null) {
            // Do not update selectedIndex here .. not until checking if
            // DeselectAllHref is clicked

            var elementName = field.name;
            if (elementName !=
                (prefix + "LibraryDriveSummaryTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                // update selected index
                selectedIndex = field.value;
                disabled = false;
                unloaddisabled = false;
            }
        } else {
            // User uses BACK button
            for (i = 0; i < myForm.elements.length; i++) {
                var e = myForm.elements[i];
                if (e.name.indexOf(selectionName) != -1) {
                    if (e.checked) {
                        disabled = false;
                        selectedIndex = myForm.elements[i].value;
                        break;
                    }
                }
            }
        }

        if (selectedIndex != -1) {
            // Get the vsn name in the hidden field
            // disable UNLOAD if vsn name is "none", means no VSN
            var elementName = prefixTiled + selectedIndex + "].VSNHidden";
            var vsnName =
                document.LibraryDriveSummaryForm.elements[elementName].value;

            if (vsnName == "") {
                unloaddisabled = true;
            }

            var menu = document.LibraryDriveSummaryForm.elements[actionMenu];
            if (menu != null) {
                var elementName = prefixTiled + selectedIndex + "].SharedHidden";
                var sharedValue =
                    document.LibraryDriveSummaryForm.elements[elementName].value;
                ccSetDropDownMenuOptionDisabled(
                    actionMenu, formName, disabled || sharedValue == "true", 1);
                ccSetDropDownMenuOptionDisabled(
                    actionMenu, formName, disabled || sharedValue != "true", 2);
            }
        }

        // Toggle action buttons disable state

        ccSetButtonDisabled(actionButton0, formName, disabled);
        ccSetButtonDisabled(actionButton1, formName, disabled);
        ccSetButtonDisabled(actionButton2, formName, unloaddisabled);
        ccSetButtonDisabled(actionButton3, formName, disabled);
    }

    function getSelectedEQ() {
        var elementName = prefixTiled + selectedIndex + "].EQHidden";
        return (document.LibraryDriveSummaryForm.elements[elementName].value);
    }
    
    function getLibraryName() {
        return (document.LibraryDriveSummaryForm.elements
            ["LibraryDriveSummary.LibraryName"].value);
    }

    function getServerKey() {
        return (document.LibraryDriveSummaryForm.elements
            ["LibraryDriveSummary.ServerName"].value);
    }

    function getLibInformation(key) {
        if (key == 1)  {
            return document.LibraryDriveSummaryForm.elements[
                "LibraryDriveSummary.Driver"].value;
        } else if (key == 2) {
            return getLibraryName();
        }
    }
    function getDriverString(choice) {
        // 0 ==> SAMST string
        // 1 ==> ACSLS String
        var infoStr = document.LibraryDriveSummaryForm.elements[
            "LibraryDriveSummary.DriversString"].value;
        var infoArr = infoStr.split("###");
        return infoArr[choice];
    }

    function popUpWhenNeeded() {
        var driver = getLibInformation(1);
        if (driver != getDriverString(0) && driver != getDriverString(1)) {
            launchPopup(
                '/media/SpecifyVSN',
                'specifyvsn',
                getServerKey(),
                SIZE_NORMAL,
                '&SAMQFS_LIBRARY_NAME=' + getLibInformation(2));
            return false;
        } else {
            return true;
        }
    }

    function disableUnloadforACSLS() {
        if (getLibInformation(1) == getDriverString(1)) {
            ccSetDropDownMenuOptionDisabled(
                "LibraryDriveSummary.ActionMenu",
                "LibraryDriveSummaryForm", true, 2);
        }
    }
