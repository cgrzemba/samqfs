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

// ident	$Id: policy.js,v 1.14 2008/12/16 00:10:36 am143972 Exp $


/*
 * Javascript for the Policy summary Page 
 */

/* useful fields names 
 */
var formName = "PolicySummary43Form";
var generalDeselect = "PolicySummary43.PolicySummary43View.GeneralPolicySummaryTable.DeselectAllHref";
var generalDelete = "PolicySummary43.PolicySummary43View.DeleteGeneralPolicy";
var otherDeselect = "PolicySummary43.PolicySummary43View.OtherPolicySummaryTable.DeselectAllHref";
var otherDelete = "PolicySummary43.PolicySummary43View.DeleteOtherPolicy";
var confirmField = "PolicySummary43.policyDeleteConfirmation";
var selectedOtherPolicyIndex;
var selectedGeneralPolicyIndex;

/* new policy wizard handler 
 */
function getClientParams() {
    return null;
}

/* handler function for :
 * Policy Summary Page -> General Policies Table -> Delete Button
 */
function handleDeleteGeneralPolicy(field) {
    var msg = field.form[confirmField].value;
    if (confirm(msg)) {
        var gp_names = 
            field.form["PolicySummary43.generalPolicyNames"].value.split(";");
        var name = gp_names[selectedGeneralPolicyIndex];

        field.form["PolicySummary43.policyToDelete"].value = name;

        return true;
    } else {
        return false;
    }
}

/*
 * handler function for :
 * Policy Summary Page -> General Policies Table -> Selection Radio button
 */
function handleGeneralPolicyTableSelection(field) {
    if (field.name == generalDeselect) {
        ccSetButtonDisabled(generalDelete, formName , 1);
    } else {
        ccSetButtonDisabled(generalDelete, formName, 0);
        selectedGeneralPolicyIndex = field.value;
    }
}

/*
 * handler function for :
 * Policy Summary Page -> Other Policies Table -> Delete Button
 */
function handleDeleteOtherPolicy(field) {
    var msg = field.form[confirmField].value;
    if (confirm(msg)) {
        var op_names = 
            field.form["PolicySummary43.otherPolicyNames"].value.split(";");
        var name = op_names[selectedOtherPolicyIndex];

        field.form["PolicySummary43.policyToDelete"].value = name;

        return true;
    } else {
        return false;
    }
}

/*
 * handler function for :
 * Policy Summary Page -> Other Policies -> Selection Radio Button
 */
function handleOtherPolicyTableSelection(field) {
    if (field.name == otherDeselect) {
        ccSetButtonDisabled(otherDelete, formName, 1);
    } else {
        if (isNoArchivePolicy(field)) {
            ccSetButtonDisabled(otherDelete, formName, 0);
            selectedOtherPolicyIndex = field.value;
        } else {
            ccSetButtonDisabled(otherDelete, formName, 1);
        }
    }
}

function isNoArchivePolicy(field) {
    var theForm = field.form;
    var no_archive = "archiving.policy.type.noarchive";
    var policyTypes = 
        theForm["PolicySummary43.otherPolicyTypes"].value.split(";");
    var index = field.value;

    return policyTypes[index] == no_archive;
}
