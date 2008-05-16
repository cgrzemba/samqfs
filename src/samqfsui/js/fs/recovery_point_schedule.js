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

// ident	$Id: recovery_point_schedule.js,v 1.7 2008/05/16 19:39:13 am143972 Exp $

var prefix = "RecoveryPointSchedule.RecoveryPointScheduleView.";
var theFormName = "RecoveryPointScheduleForm";
var theForm = document.forms[theFormName];

function getFileChooserParams(field) {
  var chooserName = getFileChooserFieldName(field);

  var textFieldName = prefix + chooserName + ".browsetextfield";
  var textField = field.form.elements[textFieldName];

  var path = "\/";
  if (textField != null && 
      textField.value != null && 
      textField.value.length !=0) {
    path = textField.value;
  } 

  // if dealing with the 'add' excluded dirs file chooser, default to the fs
  // mount point
  var prefix = "RecoveryPointSchedule.RecoveryPointScheduleView.";


  if (field.name == (prefix + "excludeDirs.browseServer")) {
    var mountPoint = field.form.elements[prefix + "mountPoint"].value;
    if (mountPoint != null) {
      // escape path separators to preserve them
      path = mountPoint.replace("/", "\/");
    }
  }
  
  return "&currentDirParam=" + path;
}

function getFileChooserFieldName(field) {
  var nameTokens = field.name.split(".");

  var index = nameTokens.length > 0 ? nameTokens.length - 2 : -1;
  
  return nameTokens[index];
}

function removeExcludeDir(field) {
  var theForm = field.form;

  var list = theForm.elements[prefix + "contentsExcludeList"];

  // start from 1 so we can skip the label item
  for (var i=1; i < list.options.length; i++) {
    if (list.options[i].selected) {
      list.options[i--] = null; // counteract the list retraction
    }
  }

  return false;
}

function getExcludedDirs(field) {
  var theForm = field.form;

  var listField = theForm.elements[prefix + "contentsExcludeList"];
  var dirs = "";
  for (var i = 1; i < listField.options.length; i++) {
    dirs = dirs + listField.options[i].value + ":";
  }

  if (dirs.length > 0) {
    theForm.elements[prefix + "excludeDirList"].value = dirs;
  }
}

function validateSchedule(field) {
  getExcludedDirs(field);
  
  // check required fields
  var state = ps_CheckRequiredFields();
  
  return state;
}

function onClickRetentionType(field) {
  if (field.value == "always") { // disable retention period
	ccSetTextFieldDisabled((prefix + "retentionValue"), theFormName, 1);
	ccSetDropDownMenuDisabled((prefix + "retentionUnit"), theFormName, 1);
  } else { // enable retention period
	ccSetTextFieldDisabled((prefix + "retentionValue"), theFormName, 0);
	ccSetDropDownMenuDisabled((prefix + "retentionUnit"),theFormName, 0);
  }
}

function validateRetentionPeriod(field) {
  if (!isPositiveInteger(field.value)) {
    var errmsg = field.form.elements["RecoveryPointSchedule.errMsg"].value;
    alert(errmsg);
    field.value = "";
  }
}
