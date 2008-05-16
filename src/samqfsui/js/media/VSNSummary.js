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

// ident	$Id: VSNSummary.js,v 1.14 2008/05/16 19:39:14 am143972 Exp $

/**
 * This is the javascript file of VSN Summary Page
 */

    var prefix        = "VSNSummary.VSNSummaryView.";
    var prefixTiled   = prefix + "VSNSummaryTiledView[";
    var selectedIndex = -1;
    
    function showConfirmMsg(choice) {
        var str = document.VSNSummaryForm.elements
            ["VSNSummary.ConfirmMessageHiddenField"].value;
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

    /**
     * To return a boolean on the permission the user has.
     * flag == 0 (return true if user has CONFIG permission)
     * flag == 1 (return true if user has MEDIA permisson)
     */
    function hasPermission(flag) {
        var role = document.VSNSummaryForm.
            elements["VSNSummary.VSNSummaryView.Role"].value;
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
	var disabled = true;
	var reserveDisabled = true;
	var loadDisabled = true;
        var isDeselectAllHref = false;

        var configRole = hasPermission(0);
        var mediaRole  = hasPermission(1);

	var button_label     = prefix + "LabelButton";
	var button_reserve   = prefix + "SamQFSWizardReserveButton";
	var button_unreserve = prefix + "UnreserveButton";
	var button_editvsn   = prefix + "EditVSNButton";
	var actionMenu       = prefix + "ActionMenu";
	var formName         = "VSNSummaryForm";
	var myForm	     = document.VSNSummaryForm;

        // checkbox or radioButton for row selection
        var selectionName = prefix + "VSNSummaryTable.Selection";

	if (field != null) {
	    // Do not update selectedIndex here .. not until checking if
            // DeselectAllHref is clicked
            
            var elementName = field.name;
            if (elementName != (prefix + "VSNSummaryTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                // update selected index
                selectedIndex = field.value;
                disabled = false;
            } else if (elementName ==
                (prefix + "VSNSummaryTable.DeselectAllHref")) {
                isDeselectAllHref = true;
            }
	} else {
            // User uses BACK/FORWARD button
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
	
        if (selectedIndex != -1 && !isDeselectAllHref) {
	    var isReserved   = getSelectedRowKey(3);
	    var isLoaded     = getSelectedRowKey(4);
            var vsnName      = getSelectedRowKey(1);
            var isBeingLoaded =
                myForm.elements["VSNSummary.LoadVSNHiddenField"].value;

            if (isLoaded != "loaded" &&
                isBeingLoaded.indexOf(vsnName) == -1) {
                loadDisabled = false;
            }

            if (isReserved == "false") {
                reserveDisabled = false;
            }
        }

        // Toggle action buttons disable state

	ccSetButtonDisabled(button_label, formName, disabled || !configRole);
	ccSetButtonDisabled(
            button_reserve, formName, reserveDisabled || !configRole);
	ccSetButtonDisabled(
            button_unreserve, formName,
            !reserveDisabled || selectedIndex == -1 ||
            isDeselectAllHref || !configRole);
	ccSetButtonDisabled(button_editvsn, formName, disabled || !configRole);

	ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, disabled || !configRole, 1);
	ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, loadDisabled || !configRole, 2);
	ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, disabled || !(mediaRole || configRole), 3);
    }

    function getSlotNumber() {
        return getSelectedRowKey(0);
    }
    
    function getSlotNumberFromHref(field) {
        var myForm        = document.VSNSummaryForm;
        var myFieldArray = field.name.split(".");
	var elementName = myFieldArray[0] + "." + myFieldArray[1] + "." 
            + myFieldArray[2] + ".InformationHiddenField";
        var infoField = myForm.elements[elementName].value;
        var myArray = infoField.split("###");
        
        // return the slot number
	return (myArray[0]);
    }
    
    function getLibraryName() {
        return (document.VSNSummaryForm.elements[
            "VSNSummary.LibraryNameHiddenField"].value);
    }

    function getServerKey() {
        return (document.VSNSummaryForm.elements[
            "VSNSummary.ServerNameHiddenField"].value);
    }

    function getClientParams() {
        var param = "";
        var libraryName = getLibraryName();
        var slotNumber = getSlotNumber();
        var serverName = getServerKey();

        param += "serverNameParam=" + serverName;
        param += "&libraryNameParam=" + libraryName;
        param += "&slotNumberParam=" + slotNumber;

        return param;
    }
    
    /**
     * Choice: 0  == return selected SlotNum
     *         1  == return selected vsn name
     *         2  == return selected barcode
     *         3  == return isReserved
     *         4  == return isLoaded
     */
    function getSelectedRowKey(key) {
        var myForm = document.VSNSummaryForm;
        var informationHiddenField = myForm.elements
            [prefixTiled + selectedIndex + "].InformationHiddenField"].value;
        var myArray = informationHiddenField.split("###");
        
        if (key >= 0 && key <= 4) {
            // convert to integer
            key = key + 0;
            return myArray[key];
        } else {
            return "";
        }
    }

    function resetDropDownMenu(dropdown) {
        if (dropdown != null) {
            dropdown.options[0].selected = true;
        }
    }

    function handleSwitchLibraryMenu(menu) {
        var URL = "/samqfsui/media/VSNSummary?SERVER_NAME=" +
                    getServerKey() + "&SAMQFS_LIBRARY_NAME=" + menu.value;
        window.location.href = URL;
    }
