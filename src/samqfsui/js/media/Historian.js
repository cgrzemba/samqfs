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

// ident        $Id: Historian.js,v 1.11 2008/05/16 19:39:14 am143972 Exp $

/**
 * This is the javascript file of Historian Page
 */

    var prefix        = "Historian.HistorianView.";
    var prefixTiled   = prefix + "HistorianTiledView[";
    var selectedIndex = -1;
    
    function showConfirmMsg(choice) {
        var str = document.HistorianForm.elements
            ["Historian.ConfirmMessageHiddenField"].value;
        var arr = str.split("###");      
        if (!confirm(arr[choice])) {
            return false;
        } else {
            return true;
        }
    }

    /**
     * To return a boolean on the permission the user has.
     * flag == 0 (return true if user has CONFIG permission)
     * flag == 1 (return true if user has MEDIA permisson)
     */
    function hasPermission(flag) {
        var role = document.HistorianForm.
            elements["Historian.HistorianView.Role"].value;
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
        var isDeselectAllHref = false;

        var configRole = hasPermission(0);
        var mediaRole  = hasPermission(1);

        var button_export    = prefix + "ExportButton";
        var button_reserve   = prefix + "SamQFSWizardReserveButton";
        var button_unreserve = prefix + "UnreserveButton";
        var button_editvsn   = prefix + "EditVSNButton";
        var formName         = "HistorianForm";
        var myForm           = document.HistorianForm;

        // checkbox or radioButton for row selection
        var selectionName = prefix + "HistorianTable.Selection";

        if (field != null) {
            // Do not update selectedIndex here .. not until checking if
            // DeselectAllHref is clicked
            
            var elementName = field.name;
            if (elementName != (prefix + "HistorianTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                // update selected index
                selectedIndex = field.value;
                disabled = false;
            } else if (elementName ==
                (prefix + "HistorianTable.DeselectAllHref")) {
                isDeselectAllHref = true;
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
        
        if (selectedIndex != -1 && !isDeselectAllHref) {
            var isReserved   = getSelectedRowKey(1);

            if (isReserved == "false") {
                reserveDisabled = false;
            }
        }

        // Toggle action buttons disable state

        ccSetButtonDisabled(
            button_export, formName, disabled || !(mediaRole || configRole));
        ccSetButtonDisabled(
            button_reserve, formName, reserveDisabled || !configRole);
        ccSetButtonDisabled(
            button_unreserve, formName,
            !reserveDisabled || selectedIndex == -1 ||
            isDeselectAllHref || !configRole);
        ccSetButtonDisabled(
            button_editvsn, formName, disabled || !configRole);
    }

    function getSlotNumber() {
        return getSelectedRowKey(0);
    }
    
    function getSlotNumberFromHref(field) {
        var myForm        = document.HistorianForm;
        var myFieldArray = field.name.split(".");
        var elementName = myFieldArray[0] + "." + myFieldArray[1] + "." 
            + myFieldArray[2] + ".InformationHidden";
        var infoArray = myForm.elements[elementName].value;
        var myArray = infoArray.split("###");
        
        // return the slot number
        return (myArray[0]);
    }
    

    function getServerKey() {
        return document.HistorianForm.elements
            ["Historian.ServerNameHiddenField"].value;
    }

   
    function getClientParams() {
        var param = "";
        var serverName = getServerKey();
        var slotNumber = getSlotNumber();

        param += "serverNameParam=" + serverName;
        param += "&libraryNameParam=<Historian>";
        param += "&slotNumberParam=" + slotNumber;

        return param;
    }
    
    /**
     * Choice: 0  == return selected SlotNum
     *         1  == return isReserved
     */
    function getSelectedRowKey(key) {
        var myForm = document.HistorianForm;
        var informationHiddenField = myForm.elements
            [prefixTiled + selectedIndex + "].InformationHidden"].value;
        var myArray = informationHiddenField.split("###");
        
        if (key == 1 || key == 0) {
            return myArray[key];
        } else {
            return "";
        }
    }
