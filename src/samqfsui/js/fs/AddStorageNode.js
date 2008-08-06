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

// ident	$Id: AddStorageNode.js,v 1.3 2008/08/06 17:41:48 ronaldso Exp $

var prefix = "AddStorageNodeForm:AddStorageNodeWizard";

function handleWizardDismiss() {
    var parentFormId = opener.document.forms[0].id;
    var url =
        "SharedFSForm" == parentFormId ?
            "/samqfsui/faces/jsp/fs/SharedFSSummary.jsp" :
            "/samqfsui/faces/jsp/fs/SharedFSStorageNode.jsp";
    Wizard_AddStorageNodeForm_AddStorageNodeWizard.closeAndForward(
          parentFormId,
          url,
          true);
}

function getExcludeTable() {
    return document.getElementById(
        "AddStorageNodeForm:AddStorageNodeWizard:step_id_selectstorage:tableExclude");
}

function initExcludeTableRows() {
    getExcludeTable().initAllRows();
}

function clearTextFields() {
    var stepPrefix = prefix + ":SubStepCreateGroup:step_id_create_group:";
    document.getElementById(stepPrefix + "fieldBlockSize").value = "";
    document.getElementById(stepPrefix + "fieldBlockPerDevice").value = "";
}

function init() {
    // When enter is pressed, click OK
    document.onkeypress = onKeyPressEnter;
}

function onKeyPressEnter(theEvent) {
    if (theEvent != null) {
        // Mozilla support
        if (theEvent.which == 13) {
            // Enter key pressed.
            clickNext();
        }
    } else if (event != null) {
        // IE support
        if (event.keyCode == 13) {
            // Enter key pressed.
            clickNext();
        }
    }
}

function clickNext() {
    var button = document.getElementById(
        "AddStorageNodeForm:AddStorageNodeWizard:AddStorageNodeWizard_next");
    if (null == button) {
        button = document.getElementById(
        "AddStorageNodeForm:AddStorageNodeWizard:AddStorageNodeWizard_finish");
    }
    
    if (null != button) {
        button.click();
    }
}
