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

// ident	$Id: policydetails44.js,v 1.8 2008/03/17 14:40:25 am143972 Exp $

/*
 * Javascript for the 4.4 policy details page 
*/

/*
 * generate client-side parameters for the add policy criteria wizard 
 */
function getAddPolicyCriteriaWizardParams() {
    return null;
}

/*
 * generate client-side parameters for the add policy copy wizard
 */
function getAddPolicyCopyWizardParams() {
    return null;
}

/*
 * Gather any required client-side parameters before launching the wizard
 */
function getClientParsm() {
    return null;
}

var selectedCriteriaIndex = null;
var selectedCopyIndex = null;

var formName = "PolicyDetailsForm";
var criteriaPrefix = "PolicyDetails.FileMatchCriteriaView.";
var copyPrefix = "PolicyDetails.CopyInformationView.";
var criteriaDeselect = "PolicyDetails.FileMatchCriteriaView.FileMatchCriteriaTable.DeselectAllHref";
var copyDeselect = "PolicyDetails.CopyInformationView.CopyInformationTable.DeselectAllHref";

var criteriaArray = null;
var copyArray = null;

function handleFileMatchCriteriaTableSelection(field) {
    if (field.name == criteriaDeselect) {
        // disable everything
        disableButton(criteriaPrefix+"RemoveCriteria", formName, 1);
        disableButton(criteriaPrefix+"EditCriteria", formName, 1);
    } else {
        if (isCriteriaDeletable(field)) {
            disableButton(criteriaPrefix+"RemoveCriteria", formName, 0);
            disableButton(criteriaPrefix+"EditCriteria", formName, 0);
        } else {
            disableButton(criteriaPrefix+"EditCriteria", formName, 0);
            disableButton(criteriaPrefix+"RemoveCriteria", formName, 1);
        }
        selectedCriteriaIndex = field.value;
    }
}

function handleRemoveCriteria(field) {
    var msg = field.form["PolicyDetails.criteriaDeleteConfirmation"].value;
    var selectedCriteria = criteriaArray[selectedCriteriaIndex];
    field.form["PolicyDetails.selectedCriteriaNumber"].value=selectedCriteria;

    return confirm(msg);
}

function handleEditCriteria(field) {
    var selectedCriteria = criteriaArray[selectedCriteriaIndex];
    field.form["PolicyDetails.selectedCriteriaNumber"].value=selectedCriteria;
}

function handleCopyInformationTableSelection(field) {
    if (field.name == copyDeselect) {
        // disable everything
        disableButton(copyPrefix+"RemoveCopy", formName, 1);
        disableButton(copyPrefix+"EditAdvancedOptions", formName, 1);
        disableButton(copyPrefix+"EditVSNAssignments", formName, 1);
    } else {
        if (isCopyDeletable(field)) {
            disableButton(copyPrefix+"RemoveCopy", formName, 0);
            disableButton(copyPrefix+"EditAdvancedOptions", formName, 0);
            disableButton(copyPrefix+"EditVSNAssignments", formName, 0);
        } else {
            disableButton(copyPrefix+"RemoveCopy", formName, 1);
            disableButton(copyPrefix+"EditAdvancedOptions", formName, 0);
            disableButton(copyPrefix+"EditVSNAssignments", formName, 0);
        }
        selectedCopyIndex = field.value;
    }  
}

function handleRemoveCopy(field) {
    var msg = field.form["PolicyDetails.copyDeleteConfirmation"].value;
    var selectedCopy = copyArray[selectedCopyIndex];
    field.form["PolicyDetails.selectedCopyNumber"].value=selectedCopy;

    return confirm(msg);
}

function handleEditAdvancedOptions(field) {
    var prefix = "PolicyDetails.";

    var selectedCopy = copyArray[selectedCopyIndex];
    field.form[prefix + "selectedCopyNumber"].value=selectedCopy;
    var mediaTypes = field.form[prefix + "copyMediaTypes"].value.split(",");

    var copy = mediaTypes[selectedCopyIndex];
    field.form[prefix + "selectedCopyMediaType"].value = copy;
}

function handleEditVSNAssignments(field) {
    var prefix = "PolicyDetails.";

    var selectedCopy = copyArray[selectedCopyIndex];
    field.form[prefix + "selectedCopyNumber"].value=selectedCopy;
    var mediaTypes = field.form[prefix + "copyMediaTypes"].value.split(",");

    var copy = mediaTypes[selectedCopyIndex];
    field.form[prefix + "selectedCopyMediaType"].value = copy;
}

function disableButton(field, formName, disable) {
    ccSetButtonDisabled(field, formName, disable);
}

function isCriteriaDeletable(field) {
    var criteria = field.form["PolicyDetails.criteriaNumbers"].value;

    criteriaArray = criteria.split(",");

    /* do not delete the last criteria of a policy */
    if (criteriaArray.length > 1) {
        return true;
    } else {
        return false;
    }
}

function isCopyDeletable(field) {
    var copies = field.form["PolicyDetails.copyNumbers"].value;

    copyArray = copies.split(",");

    /* do not delete the last copy or any copies at if dealing with the 
     * allssets policy, denoted by 5 copies
     */
    if (copyArray.length > 1 && copyArray.length < 5) {
        return true;
    } else {
        return false;
    }
}

/* handler for the default policy copy settings save button */
function handleSave(field) {
    return true;
}
