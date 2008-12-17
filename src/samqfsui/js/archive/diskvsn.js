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

// ident	$Id: diskvsn.js,v 1.19 2008/12/17 21:41:40 ronaldso Exp $

/*
 * The functions in this file are used to control the disk vsn summary page
 * and related popups.
 */
var property = "scrollbars=no,resizable=yes,height=500,width=700";

function getServerKey() {
    return document.DiskVSNSummaryForm.elements[
        "DiskVSNSummary.server_name"].value;
}

function launchNewDiskVSNPopup(field) {
  var theForm = field.form;
  var prefix = "DiskVSNSummary.";
  var name = "new_disk_vsn_popup";

  return launchPopup("/archive/NewDiskVSN", name, getServerKey());
}

/* handle submit new disk vsn */
function handleCreateDiskVSN(field) {
    var parentForm= window.opener.document.DiskVSNSummaryForm;
    var parentPrefix = "DiskVSNSummary.";
    var theForm = field.form;
    var prefix = "NewDiskVSN.";

    var valid = true;
    var message = "";

    // retrieve list of localized error messages
    var errorMessages =
        parentForm[parentPrefix + "messages"].value.split(";");
    var existingVSNs = parentForm[parentPrefix + "vsns"].value.split(";");

    // validate vsn name
    var vsnName = trim(theForm[prefix + "vsnName"].value);
    if (vsnName == null || vsnName.length == 0) {
        message += errorMessages[0] + "\n";
    } else {
        // make sure name is alpha-numeric
        if (!isValidDiskVSNName(vsnName)) {
            message += errorMessages[4] + "\n";
        }

        // make sure name doesn't exist already
        if (contains(vsnName, existingVSNs)) {
            message += errorMessages[1] + "\n";
        }
    }

    if (theForm[prefix + "mediaType"].value == 137) { // honey comb vsn
      // data ip. this can be either an ip or a hostname. besides making sure
      // there is a value, there not much else to validate
      var dataIP = theForm[prefix + "dataIP"].value;
      if (dataIP == null || dataIP.length == 0) {
        message += errorMessages[5] + "\n";
      }

      // data port
      var port = theForm[prefix + "port"].value;
      if (port == null || port.length == 0) {
        message += errorMessages[6] + "\n";
      } else if (!isPositiveInteger(port)) {
        message += errorMessages[7] + "\n";
      }
    } else { // regular disk vsn
      // validate host
      var vsnHost = theForm[prefix + "vsnHost"].value;

      // validate path
      var vsnPath = trim(theForm[prefix + "filechooser.browsetextfield"].value);
      if (vsnPath == null || vsnPath.length == 0) {
        message += errorMessages[2] + "\n";
      } else {
        if (!(vsnPath.substring(0,1) == "/")) {
          message += errorMessages[3] + "\n";
        }

        // make sure path contains no non-ASCII characters
        if (!isValidDiskPath(vsnPath)) {
          message += errorMessages[8] + "\n";
        }
      }
    }

      // only proceed if all the values validated fine
    if (message.length > 0) {
        alert(message);
        return false;
    } else {
        return true;
    }
}

// this should perhaps go into the samqfsui.js
/* determine if a given array [array] contains a given value [value] */
function contains(value, array) {
    for (var i = 0; i < array.length; i++) {
        if (trim(value) == trim(array[i])) {
            return true;
        }
    }
    return false;
}

// this should perhaps go into the samqfsui.js
/* a utility function to retrieve the value associated the <A> element
 *
 * <a href="http://host:port/uri?hrefname=value" name="hrefname">text</a>
 *
 * return the 'value' part of hrefname=value
 */
function getHrefValue(a_element) {
    var href_array = a_element.href.split("?");
    var params = href_array[1].split("=");
    var temp = params[1].split("&");
    var selected_vsn = temp[0];

    return selected_vsn;
}

/*
 *  handler for the Edit Disk VSN Media Flags url
 */
function handleMediaFlagsEditorHref(field) {
    var name = "edit_disk_flags_popup";

    var selected_vsn = getHrefValue(field);

    var params = "&selected_vsn_name=" + selected_vsn;

    var win = launchPopup(
                "/archive/EditDiskVSNFlags",
                name, getServerKey(), null, params);
    return false;
}


/* handler for the submit button on the edit media flags window */
function handleEditDiskVSNFlagsSubmit(field) {
    var prefix = "EditDiskVSNFlags.";
    var theForm = field.form;

    // these are ordered and this order should be maintained in the view bean
    // [badMedia;unavailable;readOnly;labeled]

    var flags = "";

    flags += theForm[prefix + "badMedia"].checked + ";";
    flags += theForm[prefix + "unavailable"].checked + ";";
    flags += theForm[prefix + "readOnly"].checked + ";";
    flags += theForm[prefix + "labeled"].checked;


    // set the action and submit the form
    var parentForm = window.opener.document.DiskVSNSummaryForm;

    parentForm["DiskVSNSummary.selected_vsn_name"].value =
        theForm[prefix + "selected_vsn_name"].value;
    parentForm["DiskVSNSummary.flags"].value = flags;

    var handler = "DiskVSNSummary.editMediaFlags";

    var pageSession = parentForm.elements["jato.pageSession"].value;

    var actionArray = parentForm.action.split("?");
    var action = actionArray[0];

    parentForm.action =
        action + "?" + handler + "=&jato.pageSession=" + pageSession;
    parentForm.submit();

    return true;
}

var selected_vsn = null;
var prefix = "DiskVSNSummary.DiskVSNSummaryView.";
var deselectRadio = prefix + "DiskVSNSummaryTable.DeselectAllHref";
var editButton = prefix + "EditMediaFlags";

function handleDiskVSNSummaryTableSelection(field) {
    var formName = "DiskVSNSummaryForm";
    var myForm   = document.forms[0];
    var disabled = true;
    var vsns = myForm.elements["DiskVSNSummary.vsns"].value.split(";");

    if (field != null) {
        if (field.name != deselectRadio) {
            selected_vsn = vsns[field.value];
            disabled = false;
        }
    } else {
        // User uses FORWARD/BACK button
        var selection = prefix + "DiskVSNSummaryTable.Selection";
        for (i = 0; i < myForm.elements.length; i++) {
            var e = myForm.elements[i];
            if (e.name.indexOf(selection) != -1) {
                if (e.checked) {
                    disabled = false;
                    selected_vsn = vsns[myForm.elements[i].value];
                    break;
                }
            }
        }
    }

    ccSetButtonDisabled(editButton, formName, disabled);
}

/*
 * handler for the edit media attributes button
 */
function handleEditMediaFlagsButton(field) {
  // get selected vsn
  var name = "edit_disk_flags_popup";

  var params = "&selected_vsn_name=" + selected_vsn;

  var win = launchPopup(
            "/archive/EditDiskVSNFlags", name, getServerKey(), null, params);

  return false;
}

function isValidDiskVSNName(name) {
  for (var i = 0; i < name.length; i++) {
    if (!isValidFileNameCharacter(name.charAt(i)))
      return false;
  }

  return true;
}

function isValidDiskPath(path) {
    for (var i = 0; i < path.length; i++) {
      if (!("/" == path.charAt(i) || isValidFileNameCharacter(path.charAt(i))))
        return false;
    }

    return true;
}

function getFileChooserParams() {
  var theForm = document.forms["NewDiskVSNForm"];
  var host = theForm.elements["NewDiskVSN.vsnHost"].value;
  var path = theForm.elements["NewDiskVSN.filechooser.browsetextfield"].value;

  if (path == null || path == "")
    path = "\/";

  return "&serverNameParam=" + host + "&currentDirParam="+path;
}

/* name of the filechooser text field */
var fcPath = "NewDiskVSN.filechooser.browsetextfield";

/* name of the filechooser browse button */
var fcBrowse = "NewDiskVSN.filechooser.browseServer";

/*
 * handler for the media type change
 * this function is called when a user selects a disk vsn media type -
 * i.e. chooses between creating a regular disk vsn and a honey comb target
 */
function handleMediaTypeChange(field) {
    var theForm = field.form;

    var diskDiv = document.getElementById("diskDiv");
    var hcDiv = document.getElementById("honeyCombDiv");

    if (field.value == "133") { // regular disk vsn
      diskDiv.style.display="";
      hcDiv.style.display="none";
    } else { // a honey archiving volume
      diskDiv.style.display="none";
      hcDiv.style.display="";
    }
}

/*
 * this function is called when the new disk-based vsn popup is launched. It
 * initializes the <div> targets to make the correct one for the selected media
 * type is displayed and the other one hidden accordingly.
 */
function initDivs() {
  // locate the media type dropdown field
  var theForm = document.forms["NewDiskVSNForm"];
  var mediaTypeField = theForm.elements["NewDiskVSN.mediaType"];

  // call the regular 'handleMediaTypeChange' to do the actual work. This is
  // necssarily to maintain consistency and avoid duplication of code
  if (mediaTypeField != null && mediaTypeField.value != null) {
    // this should always be the case.
    handleMediaTypeChange(mediaTypeField);
  }
}

/*
 * handler functions for the new disk vsn popup
 */
function handleHostChange(field) {
  var host = field.value;
  var theForm = field.form;
  var formName = "NewDiskVSNForm";

  if (hostSupportsRemoteFileChooser(host, theForm)) {
    ccSetButtonDisabled(fcBrowse, formName, 0);
  } else {
    ccSetButtonDisabled(fcBrowse, formName, 1);
  }
}

/*
 * check if the selected host supports the remote file chooser
 */
function hostSupportsRemoteFileChooser(host, theForm) {
  var supportedHosts =
    theForm.elements["NewDiskVSN.RFCCapableHosts"].value.split(";");

  var found = false;
  for (var i = 0; !found && i < supportedHosts.length; i++) {
    if (host == supportedHosts[i]) {
      found = true;
      break;
    }
  }

  return found;
}

function handleCloseButton(button) {
  if (button.value == "Cancel") {
    window.close();
    return false;
  }

  var parentForm = opener.document.forms[0];
  if (parentForm.name == "CommonTasksForm") {
    var url = "/samqfsui/archive/DiskVSNSummary";
    var serverName = parentForm.elements["CommonTasks.serverName"].value;
    url += "?SERVER_NAME=" + serverName;

    opener.document.location.href = url;
    window.close();
    return false;
  } else {
    return cleanupPopup(button);
  }
}

