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

// ident	$Id: DataClassSummary.js,v 1.6 2008/05/16 19:39:13 am143972 Exp $

    var formName   = "DataClassSummaryForm";
    var pagePrefix = "DataClassSummary.";
    var viewPrefix = "DataClassSummary.DataClassSummaryView.";
    var actionMenu = viewPrefix + "ActionMenu";
    var selectedIndex = "";

    function getClientParams() {
        return null;
    }
    
    function hasWriteRole() {
        var permission = document.DataClassSummaryForm.elements[
            pagePrefix + "hasPermission"].value;
        return (permission == "true");
    }

    function handleDataClassTableSelection(field) {
        var deleteButton = viewPrefix + "DeleteDataClass";
        var deselectAll =  viewPrefix + "DataClassSummaryTable.DeselectAllHref";

        var deleteDisable      = true;
        var modifyPolicyDisable = true;
        var disable            = true;
        var permission         = hasWriteRole();

        if (field != null && field.name != deselectAll) {
            selectedIndex = field.value;
            disable            = false;
        } else {
            // User uses FORWARD/BACK button
            var myForm = document.DataClassSummaryForm;
            var selection = viewPrefix + "DataClassSummaryTable.Selection";
            for (i = 0; i < myForm.elements.length; i++) {
                var e = myForm.elements[i];
                if (e.name.indexOf(selection) != -1) {
                    if (e.checked) {
                        disabled = false;
                        selectedIndex = myForm.elements[i].value;
                        break;
                    }
                }
            }
        }

        if (selectedIndex != "") {
            deleteDisable      = getRowKey(3, selectedIndex) == "false";
            modifyPolicyDisable = getRowKey(2, selectedIndex) == "false";
        }
        
        ccSetButtonDisabled(
            deleteButton, formName, deleteDisable || !permission);
        ccSetDropDownMenuOptionDisabled(actionMenu, formName, deleteDisable, 1);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, modifyPolicyDisable, 2);
        ccSetDropDownMenuOptionDisabled(actionMenu, formName, disable, 3);
        ccSetDropDownMenuOptionDisabled(actionMenu, formName, disable, 4);
        ccSetDropDownMenuOptionDisabled(actionMenu, formName, disable, 5);
        ccSetDropDownMenuOptionDisabled(actionMenu, formName, disable, 6);
    }
    
    /* return flag to determine which button/drop down items to disable */
    /* choice 0 == Policy Name */
    /* choice 1 == Criteria Index */
    /* choice 2 == Modify Policy */
    /* choice 3 == Delete        */
    /* choice 4 == Class Name    */
    function getRowKey(choice, selectedIndex) {
        var tiledViewPrefix = viewPrefix + "DataClassSummaryTiledView[";
        var info = document.DataClassSummaryForm.elements[
            tiledViewPrefix + selectedIndex + "].PolicyInfo"
            ].value;
        var infoArray = info.split("###");
        return infoArray[choice];
    }
    
    function copyEntryInfoToHiddenField(field) {
        // assign policy name and criteria key to hidden field
        field.form.elements[pagePrefix + "dataClassToDelete"].value =
            getRowKey(0, selectedIndex) + "###" +
            getRowKey(1, selectedIndex) + "###" +
            getRowKey(4, selectedIndex);
    }
    
    function getDropDownSelectedItem(dropdown) {
        var item = -1;
        if (dropdown != null) {
            item = dropdown.value;
        }
        return item;
    }
    
    function resetDropDownMenu(dropdown) {
        if (dropdown != null) {
            dropdown.options[0].selected = true;
        }
    }
