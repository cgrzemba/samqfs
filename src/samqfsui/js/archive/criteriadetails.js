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

// ident	$Id: criteriadetails.js,v 1.5 2008/03/17 14:40:25 am143972 Exp $

/*
 * Javascript for the Criteria Details Page 
 */

/*
 * useful fields 
 */
var property = "scrollbars=no,resizable=yes,height=500,width=700";

// fields used by the apply criteria to fs popup window
var selected_fs = null;
var all_fs = null;
var dumpvalues = null;
var errorMessages = null;

/*
 * handler for :
 * Criteria Details Page -> Files Systems Table -> Add
 */
function launchApplyPolicyPopup() {
    var url = "/samqfsui/archive/ApplyPolicyToFS43?com_sun_web_ui_popup=true";
    var name = "appy_policy_popup_43";
    var win = window.open(url, name, property);
    return win;
}

/*
 * handler function for :
 * Criteria Details Page -> File Systems Table -> Add -> Selection Radio Button
 */
function handleApplyFSPopupTableSelection(field) {

    // if deselect button is clicked clear selected fs
    if (field.name == "ApplyPolicyToFS43.filesystemTable.DeselectAllHref") {
        selected_fs = null;
        return
    }
    
    if (field.type != "radio") {
        return;
    }

    // process selection
    if (all_fs == null) {
        all_fs = field.form["ApplyPolicyToFS43.filenames"].value.split(";");
    }
    selected_fs = all_fs[field.value];
}


/* 
 * handler function for :
 * Criteria Details Page -> File Systems Table -> Add -> Submit
 */
function handleApplyFSPopupSubmit(field) {
    // parse errors just incase we need them
    errorMessages = 
        field.form["ApplyPolicyToFS43.errorMessages"].value.split(";");
   
    // initialize the forms
    var theForm = field.form;
    var parentForm = window.opener.document.CriteriaDetails43Form;
    var valid = true;
    var message = "";
    
    // validate the form
    if (selected_fs == null) {
        message += errorMessages[0] + "\n";
        valid = false;
    }

    // just in case the user didn't click on the 
    if (isDumping(theForm) && dumpvalues == null)
        dumpvalues = true;

    if (isDumping(theForm) && dumpvalues) { 
        // validate the path
        var path = trim(theForm["ApplyPolicyToFS43.dumpView.savePath"].value);
        if (path == null || path.length == 0) {
            message += errorMessages[1] + "\n";
            valid = false;
        } else {
            if (!(path.substring(0,1) == "/")) {
                message += errorMessages[2] + "\n";
                valid = false;
            } else {
                // path must be correct so save it
                parentForm["CriteriaDetails43.dumpPath"].value = path;
            }
        }
    }
    if (isDumping(theForm) && !dumpvalues) {
        parentForm["CriteriaDetails43.dumpPath"].value = null;
    }
   
    if (!valid) {
        alert(message);
        return false;
    }

    // if we get this far everything should be correct
    parentForm["CriteriaDetails43.fsname"].value = selected_fs;
    
    // now prepare and submit the form
    var handler = "CriteriaDetails43.ApplyCriteriaHref";
    var pageSession = parentForm.elements["jato.pageSession"].value;

    var actionArray = parentForm.action.split("?");
    var action = actionArray[0];
   
    parentForm.action = 
        action + "?" + handler + "=&jato.pageSession=" + pageSession;

    // submit the form
    parentForm.submit();
    return true;
}

/* onclick handler for the radio buttons */
function handleCommitSaveOnclick(field) {
    var theField = field.form["ApplyPolicyToFS43.dumpView.savePath"];

    if (field.value == "commit") {
        theField.disabled = true;
        dumpvalues = false;
    } else {
        theField.disabled = false;
        dumpvalues = true;
        theField.focus();
    }
}

/* determine if dumping is on or not */
function isDumping(theForm) {
    // determine if dumping is on
    var dumpOn = theForm["ApplyPolicyToFS43.dumpOn"].value;

    if (dumpOn == "true") {
        return true;
    } else {
        return false;
    }
}

/*
 * handler function for : 
 * Criteria Details Page -> File Systems Table -> Radio Button Selection
 */
function handleCriteriaDetailsFSTableSelection(field) {
    var deselect = "CriteriaDetails43.CriteriaDetailsView.CriteriaDetailsFSTable.DeselectAllHref";
    var fsRemove =  "CriteriaDetails43.CriteriaDetailsView.RemoveFS";
    var formName = "CriteriaDetails43Form";
    
    if (field.name == deselect) {
        ccSetButtonDisabled(fsRemove, formName, 1);
    } else {
        if (isFSDeletable(field)) {
            ccSetButtonDisabled(fsRemove, formName, 0);
        } else {
            ccSetButtonDisabled(fsRemove, formName, 1);
        }
    }
}

function isFSDeletable(field) {
    var deletable = field.form["CriteriaDetails43.fsDeletable"].value;
    if (deletable == "true") {
        return true;
    } else {
        return false;
    }
}

/*
 * handler for Criteria Details Page -> File Systems Table -> Remove Button
 */
function handleFSRemove(field) {
    var msg = field.form["CriteriaDetails43.fsDeleteConfirmation"].value;

    return confirm(msg);
}
