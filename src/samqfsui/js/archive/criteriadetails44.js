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

// ident	$Id: criteriadetails44.js,v 1.6 2008/03/17 14:40:25 am143972 Exp $

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

var selectedIndex = null;
/*
 * handler for :
 * Criteria Details Page -> Files Systems Table -> Add
 */
function old_launchApplyCriteriaPopup(field) {
    var url = 
    "/samqfsui/archive/ApplyCriteriaToFileSystem?com_sun_web_ui_popup=true";

    var psAttributes = field.form["CriteriaDetails.psAttributes"].value;

    url += "&psAttributes=" + psAttributes;

    var name = "_apply_policy_popup_";
    var win = window.open(url, name, property);

    return win;
}

/*
 * launch the apply criteria to file system poup
 */
function launchApplyCriteriaPopup(field) {
    var uri = "/archive/ApplyCriteriaToFileSystem";
    var name = "apply_criteria_fs";
    var psAttributes = field.form["CriteriaDetails.psAttributes"].value;
    var params = "&psAttributes=" + psAttributes;
    var serverName = document.CriteriaDetailsForm.elements[
        "CriteriaDetails.ServerName"].value;

    var w = launchPopup(uri, name, serverName, null, params);
    return false;
}

/*
 * handler function for :
 * Criteria Details Page -> File Systems Table -> Add -> Table Checkboxes
 */
function handleApplyCriteriaPopupTableSelection(field) {
}

/*
 * handler function for :
 * Criteria Details Page -> File Systems Table -> Add -> Submit
 */
function preSubmitHandler(field) {
    var prefix = "ApplyCriteriaToFileSystem.filesystemTable.SelectionCheckbox";
    var theForm = field.form;
    var fs_names = 
        theForm.elements["ApplyCriteriaToFileSystem.filenames"].value.split(";");

    // there is actually n-1 fs names since there is a trailing ';'
    fs_count = fs_names.length - 1;

    var selected_fs_names = "";
    // loop through the check boxes and determine which ones are selected
    for (var i = 0; i < fs_count; i++) {
        var check_box_name = prefix + i;
        var theCheckBox = theForm.elements[check_box_name];

        if (theCheckBox.checked) {
          selected_fs_names +=fs_names[i] + ";";
        }
    }
    
    if (selected_fs_names.length == 0) {
      alert(theForm.elements["ApplyCriteriaToFileSystem.error_no_fs_selected"].value);
      return false;
    } else {
      theForm.elements["ApplyCriteriaToFileSystem.selected_filesystems"].value
        = selected_fs_names;
      return true;
    }
}

/* 
 * handler function for :
 * Criteria Details Page -> File Systems Table -> Add -> Submit
 */
function handleApplyCriteriaToFileSystemPopupSubmit(field) {
    // parse errors just incase we need them
    errorMessages = 
        field.form["ApplyCriteriaToFileSystem.errorMessages"].value.split(";");
   
    // initialize the forms
    var theForm = field.form;
    var parentForm = window.opener.document.CriteriaDetailsForm;
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
        var path = trim(theForm["ApplyCriteriaToFileSystem.dumpView.savePath"].value);
        if (path == null || path.length == 0) {
            message += errorMessages[1] + "\n";
            valid = false;
        } else {
            if (!(path.substring(0,1) == "/")) {
                message += errorMessages[2] + "\n";
                valid = false;
            } else {
                // path must be correct so save it
                parentForm["CriteriaDetails.dumpPath"].value = path;
            }
        }
    }
    if (isDumping(theForm) && !dumpvalues) {
        parentForm["CriteriaDetails.dumpPath"].value = null;
    }
   
    if (!valid) {
        alert(message);
        return false;
    }

    // if we get this far everything should be correct
    parentForm["CriteriaDetails.fsname"].value = selected_fs;
    
    // now prepare and submit the form
    var handler = "CriteriaDetails.ApplyCriteriaHref";
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
    var theField = field.form["ApplyCriteriaToFileSystem.dumpView.savePath"];

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
    var dumpOn = theForm["ApplyCriteriaToFileSystem.dumpOn"].value;

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
    var deselect = "CriteriaDetails.CriteriaDetailsView.CriteriaDetailsFSTable.DeselectAllHref";
    var fsRemove =  "CriteriaDetails.CriteriaDetailsView.RemoveFS";
    var viewPolicies = "CriteriaDetails.CriteriaDetailsView.ViewPolicies";

    var formName = "CriteriaDetailsForm";
    
    if (field.name == deselect) {
        ccSetButtonDisabled(fsRemove, formName, 1);
        ccSetButtonDisabled(viewPolicies, formName, 1);
    } else {
        if (isFSDeletable(field)) {
            ccSetButtonDisabled(fsRemove, formName, 0);
        } else {
            ccSetButtonDisabled(fsRemove, formName, 1);
        }
        ccSetButtonDisabled(viewPolicies, formName, 0);
        selectedIndex = field.value;
    }
}

function isFSDeletable(field) {
    var deletable = field.form["CriteriaDetails.fsDeletable"].value;
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
    var msg = field.form["CriteriaDetails.fsDeleteConfirmation"].value;
    var allFS = field.form["CriteriaDetails.fsList"].value.split(",");
    field.form["CriteriaDetails.fsname"].value = allFS[selectedIndex];

    return confirm(msg);
}

/*
 * handler for view policies button
 */
function handleViewPolicies(field) {
    var allFS = field.form["CriteriaDetails.fsList"].value.split(",");

    field.form["CriteriaDetails.fsname"].value = allFS[selectedIndex];
}
