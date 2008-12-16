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

// ident	$Id: addclients.js,v 1.5 2008/12/16 00:10:38 am143972 Exp $

function $(id) {
  return document.getElementById(id);
}

function handleAddButton(button) {
  var valid = false;
  var prefix = getFieldPrefix(button.name);
  var addValue = trim($(prefix + "editableHostList_field").value);


  if ((addValue != null) && (addValue != "")) {
    if (button.name.indexOf("byipaddress") != -1) { // the ip address page
      // verify that its a valid ip range
      var ipTokens = addValue.split(".");

      if ((ipTokens != null) && (ipTokens.length == 4)) {
        if (ipTokens[3].indexOf("-") == -1) { // single ip address
          valid = isValidIP(addValue);
        } else { // ip range
          var aNumber = true;
          for (var i = 0; aNumber && i < 3; i++) { // test for the 3 entries
            aNumber = isInteger(ipTokens[i]);
          }

          if (aNumber) {
            // validate the last value to make its  a range i.e. NN-NN          
            var lastTokens = ipTokens[3].split("-");

            if (lastTokens != null && lastTokens.length == 2) {
              for (var j = 0; aNumber && j < lastTokens.length;j++) {
                aNumber = isInteger(lastTokens[j]);
              }
            }
          }

          valid = aNumber;
        }
      }

      // alert the user if not a valid ip address
      if (!valid) {
        alert(addValue + " is not a valid ip address or ip address range");
      }
    } else { // by host name
      // let the back end handle this
      valid = true;
    }
  } else {
    alert("Host Name cannot be empty");
  }

  return valid;
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

/*
 * this function is called when the user clicks on the 'close' button on the
 * wizard popup.
 */
function dismissWizard() {
  var forwardTo = "/samqfsui/faces/jsp/SharedFSSummary.jsp";
  var parentFormId = opener.document.forms[0].id;
  
  if (parentFormId == "SharedFSClientForm") {
    forwardTo = "/samqfsui/faces/jsp/SharedFSClient.jsp";
  }
  Wizard_AddClientsWizardForm_AddClientsWizard.closeAndForward(parentFormId,
                                                               forwardTo,
                                                               true);
}

function launchMHSPopup(field) {

}
