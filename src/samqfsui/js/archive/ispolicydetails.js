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

// ident	$Id: ispolicydetails.js,v 1.9 2008/12/16 00:10:36 am143972 Exp $

var formName = "ISPolicyDetailsForm";
var prefix = "ISPolicyDetails.ISPolicyDetailsView.";
var tvFieldPrefix = null;

/*
 *  global variables 
 */
var m_original_copies = new Array();
var m_new_copies = new Array();
var m_deleted_copies = new Array();
var m_modified_copies = new Array();

function $(id) {
  return document.getElementById(id);
}

function initializepolicydetailspage(field) {
  tvFieldPrefix = prefix + $("tiledViewPrefix").value;

  var copyList = $("availableCopyList").value;

  if (copyList == null) return;

  var copyArray = copyList.split(";");
  m_original_copies = copyArray;
  // make a copy of the original copy array
  for (var i = 0; i < m_original_copies.length; i++) {
      m_modified_copies[i] = m_original_copies[i];
  }

  /* this can probably be done higher up : the allsets poicy has neither the
   * add nor the remove button so no need to show/hide enable/disable them
   */
  if (copyArray.length == 5) {
      alert("TODO: allsets policy found");
      return;
  }

  // if only 1 copy is available ... disable its remove button
  if (copyArray.length == 1) {
    var rbName = tvFieldPrefix + "["+ copyArray[0] +"].removeCopy";

    // ccSetButtonDisabled(rbName, formName, 1);
  }

  for (var i = 0; i < copyArray.length; i++) {
    var copyNumber = parseInt(copyArray[i]);

    var tileNumber = copyNumber -1;
    showCopyFields(tileNumber);
  }
}

function addCopy(field) {
  // show the copy fields and the hide the add button
  showCopyFields(getTileNumberFromName(field.name));

  // disable the new copys edit options button until the copy has been saved
  var tile = getTileNumberFromName(field.name);

  var copyNumber = parseInt(tile) + 1;

  if (!isExistingCopy(copyNumber)) {
    var buttonName = tvFieldPrefix + "[" + tile + "].editCopyOptions";
    ccSetButtonDisabled(buttonName, formName, 1);

    // disable remove and edit advanced option buttons
    disableButtons(1);

    // add the number of the newly added copy to the list
    m_new_copies.push(parseInt(tile) + 1);
  } else {
    disableButtons(0);

    // adding a copy that originally existed, that means it had previously
    // been marked for deletion. Remove it from the deletetion list.
    var index = indexOf(m_deleted_copies, copyNumber);
    if (index != -1) {
      m_deleted_copies.splice(index, 1);
    }
    
    // now add the copy back to the modified copy list
    m_modified_copies.push(copyNumber);
  }

  return false;
}


function removeCopy(field) {
  // hide the copy fields and show the add button
  var tile = getTileNumberFromName(field.name);
  hideCopyFields(tile);

  var copyNumber = parseInt(tile) + 1;
  // enable add remove and edit options buttons
  disableButtons(0);

  // if deleting an existing copy, put in the list of 'to be deleted' copies
  if (isExistingCopy(copyNumber)) {
    m_deleted_copies.push(copyNumber);

    // remove this copy from the list of modified copies
    var index = indexOf(m_modified_copies, copyNumber);
    if (index != -1) {
        m_modified_copies.splice(index, 1);
    }
  } else { 
      // if deleting a new added copy, remove it from the new copy list
      // also, no need to add it to the deleted copies list since no 
      // server-sdie is required.
      var index = indexOf(m_new_copies, copyNumber);
      if (index != null) {
        m_new_copies.splice(index, 1);
      }
  }

  // if there is only one copy left, disable its remove button
  var lastCopy = getLastCopy();
  if (lastCopy != -1) {
    var tileNumber = lastCopy -1;

    var name = tvFieldPrefix + "[" + tileNumber + "].removeCopy";
    ccSetButtonDisabled(name, formName, 1);
  }
  
  // 
  return false;
}

function editCopyOptions(field) {
  return true;
}

function handleSaveButton(field) {
    if (isAllsetsPolicy()) {
        $("uiResult").value = "n=&m=1:2:3:4:5&d=";
        return true;
    }

    // encode all the values into a single value of the form:
    //  n=3&m=1&d=2
    // where n stands for new, m modified, and d deleted
    // encode new copies
    var result = "n=" + m_new_copies.join(":") + "&";

    // add modified copies
    result = result + "m=" + m_modified_copies.join(":") + "&";

    // add deleted copies
    result = result + "d=" + m_deleted_copies.join(":");

    // set the value in the hidden field and submit the form
    alert("submitted values will be : " + result);
    $("uiResult").value = result;

    return true;
}

/* this function checks */
function checkRequiredFields() {
}

function handleCancelButton(field) {
}

/* determine if the given copy is an already existing one 
 *  - otherwise its a newly created copy 
 */
function isExistingCopy(copyNumber) {

  for (var i = 0; i < m_original_copies.length; i++) {
    if (copyNumber == parseInt(m_original_copies[i])) 
      return true;
  }

  return false;
}

/*
 * this function compares the existing copies array and the deleted copies
 * one to determine if there is only one copy left. If so, it returns that 
 * copy's copy number otherwise, it returns a -1
 */
function getLastCopy() {
  if (m_modified_copies.length == 1)
     return m_modified_copies[0];

  return -1;
}


function disableButtons(disable) {
  // disable all existing copies remove and edit advanced options buttons
  for (var i = 0;i < m_original_copies.length; i++) {
     var tileNumber = parseInt(m_original_copies[i]) -1;
     var remove = tvFieldPrefix + "[" + tileNumber +  "].removeCopy";
     var edit = tvFieldPrefix + "[" + tileNumber + "].editCopyOptions";

     ccSetButtonDisabled(remove, formName, disable);
     ccSetButtonDisabled(edit, formName, disable);
  }

  // disable remaining add buttons
  // TODO: for now [for both simplicity and efficiency] disable all of them
  for (var i = 0; i < 4; i++) {
    var name = tvFieldPrefix + "[" + i + "].addCopy";

    ccSetButtonDisabled(name, formName, disable);
  }
}

function handleNeverExpireChange(field) {
  var tileNumber = getTileNumberFromName(field.name);
  var expireTime = tvFieldPrefix + "[" + tileNumber + "].expirationTime";
  var expireTimeUnit = tvFieldPrefix + "["+tileNumber+"].expirationTimeUnit";

  // if checked enable copy time and unit, else disable them
  if (field.checked) { // enable copy time and unit
    ccSetTextFieldDisabled(expireTime, formName, 1);
    ccSetDropDownMenuDisabled(expireTimeUnit, formName, 1);
  } else { // disable copy time and unit
    ccSetTextFieldDisabled(expireTime, formName, 0);
    ccSetDropDownMenuDisabled(expireTimeUnit, formName, 0);
  }
}


function handleExpirationTimeChange(field) {
  // verify that the value is a positive number
  if (!isInteger(field.value)) {
    alert("copy time expiration time must be a positive number");
  }
}

function handleCopyTimeChange(field) {
  // verify that the value is a positive number
  if (!isInteger(field.value)) {
    alert("copy time must be a positive number");
  }
}

function handleAvailableMediaHref(field) {
  var theForm = document.forms[formName];

  var name = tvFieldPrefix + "[" + 
    getTileNumberFromName(field.name) + "].availableMediaString";

  var availableVSNs = theForm.elements[name].value;

  alert(availableVSNs);
  return false;
}

/*
 * the tiled field names are of the format :
 * viewbean.view.tiledview[tilenumber].fieldname
 *
 * TODO: generalize this function
 */
function getTileNumberFromName(name) {
  if (name == null)
    return -1;

  var a1 = name.split("[");
  var tileNumber = a1[1].charAt(0);

  return tileNumber;
}

function isAllsetsPolicy() {
    var maxCopy = parseInt($("maxCopyCount").value);

    if (maxCopy == 5) {
        return true;
    } else {
        return false;
    }
}

/**/
function showCopyFields(tileNumber) {
  $("copyDivName-"+tileNumber).style.display="";
  $("noCopyDivName-"+tileNumber).style.display="none";
}

function hideCopyFields(tileNumber) {
  $("copyDivName-"+tileNumber).style.display="none";
  $("noCopyDivName-"+tileNumber).style.display="";
}


/**
 * utility function to find the location of an element in an array
 *
 * NOTE: this should be moved to a common location 
 * usage : array - the array in which to search
 *         element - the element to search for
 * returns an integer index - index of the element in the arry. -1 if the 
 * element is not found in the array.
 */
function indexOf(array, element) {
  // if array has no values or value not supplied return -1
  if (array == null || array.length == 0 || element == null) {
    return -1;
  }

  // loop through the array and return the index
  var index = -1;
  for (var i = 0; i < array.length; i++) {
    if (array[i] == element) {
      index = i;
      break;
    }
  }

  return index;
}
