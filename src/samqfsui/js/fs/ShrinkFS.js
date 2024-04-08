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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: ShrinkFS.js,v 1.3 2008/12/16 00:10:37 am143972 Exp $

function handleWizardDismiss() {
    var parentForm = opener.document.forms[0];

    var popupHandler = ".RefreshHref";

    // retrieve the container view name by removing the word form from the
    // parent form name
    var pfArray = parentForm.name.split("Form");
    popupHandler = pfArray[0] + popupHandler;

    // retrieve the action that loaded the parent form and remove the original
    // query string
    var pfaArray = parentForm.action.split("?");
    var action = pfaArray[0];

    // retrieve the page session string from the parent page and append it to
    // the query string
    var psString = parentForm.elements["jato.pageSession"].value;

    var queryString = "?" + popupHandler + "=&jato.pageSession=" + psString;

    // set the new action and submit the form
    parentForm.action = action + queryString;

    // finally submit the form
    parentForm.submit();

    window.close();
}

function getExcludeTable() {
    return document.getElementById(
        "ShrinkFSForm:ShrinkFSWizard:step_id_selectstorage:tableExclude");
}

function getAvailableTable() {
    return document.getElementById(
        "ShrinkFSForm:ShrinkFSWizard:SubStepSpecifyDevice:" +
        "step_id_specifydevice:tableAvailable");
}

function initExcludeTableRows() {
    getExcludeTable().initAllRows();
}

function initAvailableTableRows() {
    getAvailableTable().initAllRows();
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
        "ShrinkFSForm:ShrinkFSWizard:ShrinkFSWizard_next");
    if (null == button) {
        button = document.getElementById(
        "ShrinkFSForm:ShrinkFSWizard:ShrinkFSWizard_finish");
    }
    
    if (null != button) {
        button.click();
    }
}

