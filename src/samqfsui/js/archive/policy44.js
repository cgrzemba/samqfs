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

// ident	$Id: policy44.js,v 1.8 2008/12/16 00:10:36 am143972 Exp $


/* 4.4 javascript functions for the policy summary page */
var formName = "PolicySummaryForm";
var pagePrefix = "PolicySummary.";
var viewPrefix = "PolicySummary.PolicySummaryView.";
var selectedPolicyIndex = null;

function getClientParams() {
    return null;
}

/* handler for the table selection */
function handlePolicyTableSelection(field) {
    var theForm = field.form;
    var deleteButton = viewPrefix + "DeletePolicy";
    var policyDeselect = viewPrefix + "PolicySummaryTable.DeselectAllHref";

    if (field.name == policyDeselect) {
        ccSetButtonDisabled(deleteButton, formName, 1);
    } else {
        if (isPolicyDeletable(field)) {
            ccSetButtonDisabled(deleteButton, formName, 0);
            selectedPolicyIndex = field.value;
        } else {
            ccSetButtonDisabled(deleteButton, formName, 1);
        }
    }
}

/* determine if the selected row contains a deletable policy */
function isPolicyDeletable(field) {
    var types = field.form[pagePrefix + "policyTypes"].value.split(";");

    // deletable policy types
    var custom     = "1";
    var no_archive = "2";
    var unassigned = "4";
    
    if (types[field.value] == custom     ||
        types[field.value] == no_archive ||
        types[field.value] == unassigned) {
        return true;
    } else {
        return false;
    }
}

/* handler for the delete button */
function handleDeletePolicy(field) {
    var msg = field.form[pagePrefix + "policyDeleteConfirmation"].value;

    // verify that the user does indeed want to delete the selected policy.
    if (confirm(msg)) {
        // proceed with the deletion
        var names = field.form[pagePrefix + "policyNames"].value.split(";");
        var policyName = names[selectedPolicyIndex];

        field.form[pagePrefix + "policyToDelete"].value = policyName;
        return true;
    } else {
        // terminate the deletion
        return false;
    }
}
