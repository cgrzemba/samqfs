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

// ident	$Id: addclients.js,v 1.1 2008/07/16 23:47:27 kilemba Exp $

function $(id) {
  return document.getElementById(id);
}

function handleAddButton(button) {
  var prefix = getFieldPrefix(button.name);

  var textFieldName = prefix + "hostNameText";
  var listBoxName = prefix + "selectedHostList";

  var textField = $(textFieldName);
  var hostname = textField.value;
  
  if (hostname != null) {
    hostname = trim(hostname); // trim defined in samqfsui.js
    
    if (hostname.length > 0) {
      insertHostIntoList(hostname, listBoxName);
      textField.value = "";
      textField.focus();
    }
  }


  // do not submit the form
  return false;
}


function handleRemoveButton(button) {
  // do not submit the form
  return false;
}

/* 
 * retrieve the component id prefix
 */
function getFieldPrefix(name) {
  var index = name.lastIndexOf(":");

  return name.substr(0, index + 1);
}

/*
 * insert the name/ip/ip range typed in the textfield into the list box.
 */
function insertHostIntoList(hostname, listBoxName) {
  var listbox = $(listBoxName);

  var options = listbox.options;
  var optionCount = options.length;
  
  options.length = optionCount + 1;
  options[optionCount] = new Option(hostname, hostname);
}
