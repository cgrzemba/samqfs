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

// ident	$Id: LibraryFaultsSummary.js,v 1.8 2008/05/16 19:39:14 am143972 Exp $

/** 
 * This is the javascript file of Library Faults Summary Page
 */

    function showConfirmMsg() {
        var str1 = document.LibraryFaultsSummaryForm.elements
            ["LibraryFaultsSummary.ConfirmMessageHiddenField"].value;
        if (!confirm(str1)) {
            return false;
        } else {
            return true;
        }
    }

    function toggleDisabledState(field) {      
        var disabled = true;
        var myForm = document.LibraryFaultsSummaryForm;
        var prefix = "LibraryFaultsSummary.LibraryFaultsSummaryView.";
        var actionButton0 = prefix + "AcknowledgeButton";
        var actionButton1 = prefix + "DeleteButton";

        // checkbox or radioButton for row selection
        var selectionName = prefix + "LibraryFaultsSummaryTable.Selection";


        if (field != null) {
            var elementName = field.name;
            if (elementName ==
                (prefix + "LibraryFaultsSummaryTable.SelectAllHref")) {
                disabled = false;
            } else if (elementName ==
                (prefix + "LibraryFaultsSummaryTable.DeselectAllHref")) {
                disabled = true;
            } else if (field.type == "checkbox" && field.checked) {
                disabled = false;
            } else if (field.type == "checkbox" && !field.checked){
                // This case is executed when user clicks on one of the checkbox
                // and deselect that check box.  The buttons cannot be disabled
                // unless there's no other checkboxes that are checked.

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
        ccSetButtonDisabled(
            actionButton0, "LibraryFaultsSummaryForm", disabled);
        ccSetButtonDisabled(
            actionButton1, "LibraryFaultsSummaryForm", disabled);
    }
