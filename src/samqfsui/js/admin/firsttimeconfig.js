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

// ident	$Id: firsttimeconfig.js,v 1.4 2008/12/16 00:10:36 am143972 Exp $

var formName = "FirstTimeConfigForm";
var prefix = "FirstTimeConfig.";

/* handler for the more information link on the instruction paragraph */
function handleMoreInformation() {
    alert("coming soon ...");
    return false;
}

/* 
 * common wizard helper function. This allows developers to pass
 * client-side parameters to the wizard instance.
 */
function getClientParams() {
    var params = "&serverNameParam=" + getServerName();
    
    return params;
}

/* retrieve the currently managed server */
function getServerName() {
  var theForm = document.forms[formName];
  var serverName = theForm.elements[prefix + "serverName"].value;

  return serverName;
}

/* handler for the add library href */
function launchAddLibraryWizard() {
  var theForm = document.forms[formName];
  var buttonName = "FirstTimeConfig.FirstTimeConfigView.addLibraryWizard";
  var button = theForm.elements[buttonName];
  
  if (button != null) {
      button.click();
  }
}

/* handler for the Create File System href */
function createFileSystem() {
    var theForm = document.forms[formName];
    var buttonName = "FirstTimeConfig.FirstTimeConfigView.newFileSystemWizard";
    var button = theForm.elements[buttonName];

    if (button != null) {
        button.click();
    }
}

/* create disk vsn handler */
function addDiskVolume() {
  return launchPopup("/archive/NewDiskVSN",
                     "new_disk_volume",
                     getServerName());
}

/* create a new vsn pool */
function addVolumePool() {
  launchPopup("/archive/NewEditVSNPool",
              "new_volume_pool",
              getServerName(),
              SIZE_VOLUME_ASSIGNER,
              "&OPERATION=NEW");

  return false;
}

/* import tape volumes */
function importTapeVolumes() {
  launchPopup("/util/LibrarySelector",
              "library_selector",
              getServerName(),
              SIZE_SMALL,
              null);

  return false;
}

/* schedule recovery point */
function scheduleRecoveryPoint() {
  launchPopup("/util/FileSystemSelector",
              "fs_selector",
              getServerName(),
              SIZE_SMALL,
              null);

  return false;
}

/* handle the OK button on the library selector popup window */
function handleImportTapeVSNs(button) {
  var theForm = button.form;
  var s = theForm.elements["LibrarySelector.library"].value;

  var tokens = s.split(":");
  var driverType = tokens[1];

  if (driverType == "1001") { // samst
    return true;
  } else { // go to the import vsn page
    var url = "/samqfsui/media/ImportVSN" +
      "?SERVER_NAME=" + getServerName() +
      "&LIBRARY_NAME=" + tokens[0];

    opener.document.location.href = url;
    window.close();
  }
}

/* fowrad to the e-mail alerts page */
function setEmailAlerts() {
  return forwardToPage("/samqfsui/admin/AdminNotification");
}

/* forward control the page with the given URI. Note: This function
 * should only be used with pages that do not require additional
 * information
 */
function forwardToPage(uri) {
  var fullURL = uri + "?SERVER_NAME=" + getServerName();

  document.location.href = fullURL;

  return false;
}

