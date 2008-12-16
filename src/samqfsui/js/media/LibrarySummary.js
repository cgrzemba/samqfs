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

// ident        $Id: LibrarySummary.js,v 1.15 2008/12/16 00:10:38 am143972 Exp $

    var selectedIndex = -1;
    var prefix        = "LibrarySummary.LibrarySummaryView.";
    var prefixTiled   = prefix + "LibrarySummaryTiledView[";
    
    function showConfirmMsg(choice) {
        var str = document.LibrarySummaryForm.elements
                ["LibrarySummary.ConfirmMessageHiddenField"].value;
        var arr = str.split("###");
        if (!confirm(arr[choice])) {
            return false;
        } else {
            return true;
        }
    }

    function getDropDownSelectedItem(dropdown) {
        var item = -1;
        if (dropdown != null) {
            item = dropdown.value;
        }
        return item;
    }

    function getLibInformation(key) {
        var elementName;
        if (key == 1)  {
            elementName = prefixTiled + selectedIndex + "].DriverHidden";
        } else if (key == 2) {
            elementName = prefixTiled + selectedIndex + "].NameHidden";
        }

        return document.LibrarySummaryForm.elements[elementName].value;
    }

    /**
     * To return a boolean on the permission the user has.
     * flag == 0 (return true if user has CONFIG permission)
     * flag == 1 (return true if user has MEDIA permisson)
     */
    function hasPermission(flag) {
        var role = document.LibrarySummaryForm.
            elements["LibrarySummary.LibrarySummaryView.Role"].value;
        switch (flag) {
            case 0:
                return ("CONFIG" == role);
            case 1:
                return ("MEDIA"  == role);
            default:
                return false;
        }
    }

    function toggleDisabledState(field) {
        var disabled       = true;
        var vsnDisabled    = true;
        var unloadDisabled = true;
        var myForm         = document.LibrarySummaryForm;

        // checkbox or radioButton for row selection
        var selectionName = prefix + "LibrarySummaryTable.Selection";

        var configRole = hasPermission(0);
        var mediaRole  = hasPermission(1);

        // Add Button does not need to be disabled
        // it will be disabled in VB
        var viewVSNActionButton   = prefix + "ViewVSNButton";
        var viewDriveActionButton = prefix + "ViewDriveButton";
        var importActionButton    = prefix + "ImportButton";
        var actionMenu            = prefix + "ActionMenu";
        var formName              = "LibrarySummaryForm";

        if (field != null) {
            // Do not update selectedIndex here .. not until checking if
            // DeselectAllHref is clicked
            
            var elementName = field.name;
            if (elementName != (prefix + "LibrarySummaryTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                // update selected index
                selectedIndex = field.value;
                disabled = false;
                vsnDisabled = false;
                unloadDisabled = false;
            }
        } else {
            // User uses BACK button
            for (i = 0; i < myForm.elements.length; i++) {
                var e = myForm.elements[i];
                if (e.name.indexOf(selectionName) != -1) {
                    if (e.checked) {
                        disabled = false;
                        vsnDisabled = false;
                        unloadDisabled = false;
                        selectedIndex = myForm.elements[i].value;
                        break;
                    }
                }
            }
        }


        if (selectedIndex != -1) {
            // Get the library name in the hidden field
            var elementName2 = prefixTiled + selectedIndex + "].NameHidden";
            var elementName3 = prefixTiled + selectedIndex + "].DriverHidden";

            var libName = myForm.elements[elementName2].value;

            if (libName == "<Historian>") {
                disabled = true;
                unloadDisabled = true;
            }

            var driver = myForm.elements[elementName3].value;

            if (driver != getSAMSTString()) {
                unloadDisabled = true;
            }        
        }

        // Toggle action buttons disable state

        ccSetButtonDisabled(viewVSNActionButton, formName, vsnDisabled);
        ccSetButtonDisabled(viewDriveActionButton, formName, disabled);
        ccSetButtonDisabled(
            importActionButton, formName,
            disabled || !(mediaRole || configRole));

        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, disabled || !configRole , 1);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, unloadDisabled || !configRole, 2);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, disabled || !configRole, 3);
    }

    function resetDropDownMenu(dropdown) {
        if (dropdown != null) {
            dropdown.options[0].selected = true;
        }
    }
    
    function getSAMSTString() {
        return (document.LibrarySummaryForm.elements[
            "LibrarySummary.LibrarySummaryView.SAMSTdriverString"].value);
    }

    function getACSLSString() {
        return (document.LibrarySummaryForm.elements[
            "LibrarySummary.LibrarySummaryView.ACSLSdriverString"].value);
    }

    function getServerInformation(choice) {
        // 0 ==> Server Name
        // 1 ==> Server Version
        var infoStr = document.LibrarySummaryForm.elements[
            "LibrarySummary.ServerInformation"].value;
        var infoArr = infoStr.split("###");
        return infoArr[choice];
    }
 
    function getClientParams() {
        var param = "serverNameParam=" + getServerInformation(0);
        param    += "&serverVersionParam=" + getServerInformation(1);
        return param;
    }

    function is45ACSLS() {
        return (getServerInformation(1) != "1.3") &&
            (getLibInformation(1) == getACSLSString());
    }

    function popUpWhenNeeded() {
        var driver = getLibInformation(1);
        if (!(driver == getSAMSTString() || is45ACSLS())) {
            launchPopup(
                '/media/SpecifyVSN',
                'specifyvsn',
                getServerKey(),
                SIZE_NORMAL,
                '&SAMQFS_LIBRARY_NAME=' + getLibInformation(2));
            return false;
        }

        return true;
    }

    function getServerKey() {
        return getServerInformation(0);
    }
