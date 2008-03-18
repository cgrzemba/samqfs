/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

// ident	$Id: ImportVSN.js,v 1.6 2008/03/17 14:40:27 am143972 Exp $

    function setFieldEnable(choice) {
        var formName = "ImportVSNForm"
        var prefix = "ImportVSN.";

        var isPool     = (choice == "pool");
        var isRange    = (choice == "range");
        var isRegEx    = (choice == "regex");

        ccSetDropDownMenuDisabled(
            prefix + "ScratchPoolMenu", formName, !isPool);
        ccSetTextFieldDisabled(prefix + "StartField", formName, !isRange);
        ccSetTextFieldDisabled(prefix + "EndField", formName, !isRange);
        ccSetTextFieldDisabled(prefix + "RegExField", formName, !isRegEx);
    }

    function toggleDisabledState(field) {
        var disabled = true;
        var myForm = document.ImportVSNForm;
        var prefix = "ImportVSN.ImportVSNView.";
        var actionButton = prefix + "ImportButton";

        // checkbox or radioButton for row selection
        var selectionName = prefix + "ImportVSNTable.Selection";

        if (field != null) {
            var elementName = field.name;
            if (elementName ==
                (prefix + "ImportVSNTable.SelectAllHref")) {
                disabled = false;
            } else if (elementName ==
                (prefix + "ImportVSNTable.DeselectAllHref")) {
                disabled = true;
            } else if (field.type == "checkbox" && field.checked) {
                disabled = false;
            } else if (field.type == "checkbox" && !field.checked){
                // This case is executed when user clicks on one of the
                // checkbox and deselect that check box.  The buttons cannot
                // be disabled unless there's no other checkboxes that are
                // checked.

                for (i = 0; i < myForm.elements.length; i++) {
                    var e = myForm.elements[i];
                    if (e.name.indexOf(selectionName) != -1) {
                        // check if "jato_boolean" is a part of the string
                        // skip if yes
                        if (e.name.indexOf("jato_boolean") == -1) {
                            if (e.checked) {
                                // set disabled to false and get off the loop
                                disabled = false;
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            // User uses FORWARD button
            for (i = 0; i < myForm.elements.length; i++) {
                var e = myForm.elements[i];
                if (e.name.indexOf(selectionName) != -1) {
                    if (e.checked) {
                        disabled = false;
                        break;
                    }
                }
            }
        }

        // Toggle action buttons disable state
        ccSetButtonDisabled(actionButton, "ImportVSNForm", disabled);
    }

    /* 
     * handler function for : 
     * Import VSN Page -> Import
     */ 
    function preSubmitHandler(field) {
        var viewName   = "ImportVSN.ImportVSNView";
        var prefix     = viewName + ".ImportVSNTable.SelectionCheckbox"; 
        var theForm    = field.form; 
        var vol_ids =  
            theForm.elements[viewName + ".VolumeIDs"].value.split(","); 

        var selected_volume_ids = "";

        // loop through the check boxes and determine which ones are selected 
        for (var i = 0; i < vol_ids.length; i++) { 
            var check_box_name = prefix + i; 
            var theCheckBox = theForm.elements[check_box_name]; 

            // CheckBox may be taken away based on the status of VSN
            if (theCheckBox != null) {
                if (theCheckBox.checked) {
                    if (selected_volume_ids != "") {
                        selected_volume_ids += ", ";
                    }
                    selected_volume_ids += vol_ids[i]; 
                }
            }
        } 

        theForm.elements[viewName + ".SelectedVolumeIDs"].value
            = selected_volume_ids; 
        return true; 
    }
