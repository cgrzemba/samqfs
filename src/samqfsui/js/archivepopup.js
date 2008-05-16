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

// ident	$Id: archivepopup.js,v 1.6 2008/05/16 19:39:12 am143972 Exp $

/* the functions in this file are used to handle launching of popups on the 
 * archive tab 
 */

/* all popup windows are to have these properties */
var property = "scrollbars=no,resizable=yes,height=500,width=700";

/* file names for the apply policy to filesytems popup */
var filesystems_array = null;
var selected_fs = null;
var dumpvalues = null;
var errorMessages = null;

/* launch the 'apply policy to filesystem[s]' popup */
function launchApplyPolicyPopup() {
    var url = "/samqfsui/archive/ApplyPolicyToFS?com_sun_web_ui_popup=true";
    var name = "apply_policy_popup";

    var win = launchPopup(url, name);

    if (win != null)
        win.focus();

    return win;
}

/* launch the 'view criteria' popup */
function launchViewCriteriaPopup() {
    var url = "/samqfsui/archive/FileChar?com_sun_web_ui_popup=true";
    var name = "file_char_popup";

    var win = launchPopup(url, name);

    if (win != null)
        win.focus();

    return win;
}

/* launch a popup window with the give name targetted at the specified url */
function launchPopup(url, name) {
   return window.open(url, name, property);
}

/* handler for the apply policy to wizard FS table in the popup */
function handleApplyFSPopupTableSelection(field) {
    // if deselect all is clicked 
    if (field.name == "ApplyPolicyToFS.filesystemTable.DeselectAllHref") {
        selected_fs = null;
        return;
    }

    // only deal with the selection radio buttons
    if (field.type != "radio") 
        return;

    if (filesystems_array == null)
        parseFileSystems(field.form);

    selected_fs = filesystems_array[field.value];
}

/* handler for the apply policy to fs popup submit button */
function handleApplyFSPopupSubmit(field) {
    // initialize the forms
    var theForm = field.form;
    var parentForm = window.opener.document.PolFileSystemForm;
    var valid = true;
    var message = "";
    parseErrorMessages(theForm);
    
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
        var path = trim(theForm["ApplyPolicyToFS.dumpView.savePath"].value);
        if (path == null || path.length == 0) {
            message += errorMessages[1] + "\n";
            valid = false;
        } else {
            if (!(path.substring(0,1) == "/")) {
                message += errorMessages[2] + "\n";
                valid = false;
            } else {
                // path must be correct so save it
                parentForm["PolFileSystem.dumpPath"].value = path;
            }
        }
    }
    if (isDumping(theForm) && !dumpvalues) {
        parentForm["PolFileSystem.dumpPath"].value = null;
    }
   
    if (!valid) {
        alert(message);
        return false;
    }

    // if we get this far everything should be correct
    parentForm["PolFileSystem.fsname"].value = selected_fs;
    
    // now prepare and submit the form
    var handler = "PolFileSystem.ApplyPolicyHref";
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
    var theField = field.form["ApplyPolicyToFS.dumpView.savePath"];

    if (field.value == "commit") {
        theField.disabled = true;
        dumpvalues = false;
    } else {
        theField.disabled = false;
        dumpvalues = true;
        theField.focus();
    }
}

/* parse the filesystem hidden string */
function parseFileSystems(theForm) {
    filesystems_array = theForm["ApplyPolicyToFS.filenames"].value.split(";");
}

/* parse the error message hidden string into  an array */
function parseErrorMessages(theForm) {
    errorMessages = theForm["ApplyPolicyToFS.errorMessages"].value.split(";");
}

/* determine if dumping is on or not */
function isDumping(theForm) {
    // determine if dumping is on
    var dumpOn = theForm["ApplyPolicyToFS.dumpOn"].value;

    if (dumpOn == "true") {
        return true;
    } else {
        return false;
    }
}
