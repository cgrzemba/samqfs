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

// ident	$Id: policydetails.js,v 1.12 2008/12/16 00:10:36 am143972 Exp $

/*
 * Javascript for the policy details page 
 */

/*
 * useful field names
 */
var formName = "PolicyDetails43Form";
var criteriaDeselect = "PolicyDetails43.PolicyDetails43View.CriteriaSummaryTable.DeselectAllHref";
var criteriaDelete = "PolicyDetails43.PolicyDetails43View.RemoveCriteria";
var copyDeselect = "PolicyDetails43.PolicyDetails43View.CopySummaryTable.DeselectAllHref";
var copyDelete ="PolicyDetails43.PolicyDetails43View.RemoveCopy";
var selectedCriteriaIndex;
var selectedCopyIndex;

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

/*
 * handler function for :
 * Policy Details Page -> Match Criteria Table -> Selection Radio Button
 */
function handleMatchCriteriaTableSelection(field) {
    if (field.name == criteriaDeselect) {
        ccSetButtonDisabled(criteriaDelete, formName, 1);
    } else {
        if (isCriteriaDeletable(field)) {
            ccSetButtonDisabled(criteriaDelete, formName, 0);
            selectedCriteriaIndex = field.value;
        } else {
            ccSetButtonDisabled(criteriaDelete, formName, 1);
        }
    }
}

/*
 * handler function for :
 * Policy Details Page -> Match Criteria Table -> Remove Button
 */
function handleRemoveCriteria(field) {
    var msg = field.form["PolicyDetails43.criteriaDeleteConfirmation"].value;
    var theForm = field.form;
    var indeces = theForm["PolicyDetails43.criteriaNumbers"].value.split(";");
    var ci = indeces[selectedCriteriaIndex];
    theForm["PolicyDetails43.selectedCriteriaNumber"].value = ci;

    return confirm(msg);
}

/*
 * handler function for :
 * Policy Details Page -> Copy Settings Table -> Selection Radio Button
 */
function handleCopySettingsTableSelection(field) {
    if (field.name == copyDeselect) {
        ccSetButtonDisabled(copyDelete, formName, 1);
    } else {
        if (isCopyDeletable(field)) {
            ccSetButtonDisabled(copyDelete, formName, 0);
            selectedCopyIndex = field.value;
        } else {
            ccSetButtonDisabled(copyDelete, formName, 1);
        }
    }
}

/* 
 * handler function for :
 * Policy Details Page -> Copy Settings Table -> Remove Button
 */
function handleRemoveCopy(field) {
    var msg = field.form["PolicyDetails43.copyDeleteConfirmation"].value;
    var theForm = field.form;
    var copyNumbers = theForm["PolicyDetails43.copyNumbers"].value.split(";");
    var cn = copyNumbers[selectedCopyIndex];
    theForm["PolicyDetails43.selectedCopyNumber"].value = cn;

    return confirm(msg);
}

function isCriteriaDeletable(field) {
    var deletable = field.form["PolicyDetails43.isCriteriaDeletable"].value;
    if (deletable == "true") {
        return true;
    } else {
        return false;
    }
}

function isCopyDeletable(field) {
    var deletable = field.form["PolicyDetails43.isCopyDeletable"].value;
    if (deletable == "true") {
        return true;
    } else {
        return false;
    }
}
