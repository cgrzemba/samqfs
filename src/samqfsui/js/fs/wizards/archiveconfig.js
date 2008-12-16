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

// ident	$Id: archiveconfig.js,v 1.13 2008/12/16 00:10:38 am143972 Exp $

var theFormName = "wizWinForm";
var theForm = document.wizWinForm;
var prefix = "WizardWindow.Wizard.NewWizardArchiveConfigView.";

// override CCWizard functions
WizardWindow_Wizard.pageInit = initWizardPage;
WizardWindow_Wizard.nextClicked = handleNextButton;

// error messages . sequence {show,hide,pools}
var message = null;

function $(id) {
  return document.getElementById(id);
}

function initWizardPage() {
    // initialize the error message array
    message = theForm.elements[prefix + "errMsg"].value.split("##");

    var ptype = theForm.elements[prefix + "ptype"].value;

    if (ptype == "existing") {
        $("newPolicyDiv").style.display="none";
    } else if (ptype == "new") {
        $("newPolicyDiv").style.display="";
    } else {
        // undefined
    
    }

    // if the policy type radio button exists, change it
    var policyType = theForm.elements[prefix + "policyTypeExisting"];
    if (policyType) {
        policyType.value = ptype;
    }

    // init copy 3 and 4 block
    var moreCopies = theForm.elements[prefix + "copy3and4Status"].value;
    if (moreCopies == "visible") {
      $("copy3and4Div").style.display="";
    } else {
      $("copy3and4Div").style.display="none";
    }
    
    // init the more copies button depending on copy 2 state
    if (theForm.elements[prefix + "enableCopyTwo"].checked) {
      ccSetButtonDisabled(prefix + "showCopy3and4",
                          theFormName,
                          0);
    } else {
      ccSetButtonDisabled(prefix + "showCopy3and4",
                          theFormName,
                          1);
    }
}

function policyTypeChange(field) {
  var policyType = null;
  if (field.value == "existing") {
    $("newPolicyDiv").style.display="none";

    // enable existing policy name
    ccSetDropDownMenuDisabled(prefix + "existingPolicyName",
                              theFormName, 
                              0);
    policyType = "existing";
  } else {
    $("newPolicyDiv").style.display="";
    
    // disable existing policy name
    ccSetDropDownMenuDisabled(prefix + "existingPolicyName",
                              theFormName,
                              1);
    policyType = "new";
  }

  // update ptype field
  theForm.elements[prefix + "ptype"].value = policyType;
}

function enableCopyTwoChange(field) {
  if (field.checked) {
    // enable copy two archive age & media
    ccSetTextFieldDisabled(prefix + "archiveAgeTwo",
                           theFormName,
                           0);
    ccSetDropDownMenuDisabled(prefix + "archiveAgeTwoUnit",
                              theFormName,
                              0);
    ccSetDropDownMenuDisabled(prefix + "mediaTwo",
                              theFormName,
                              0);

    // enable the 'create more copies' button
    ccSetButtonDisabled(prefix + "showCopy3and4",
                        theFormName,
                        0);
  } else {
    // disable copy two archive age & media
    ccSetTextFieldDisabled(prefix + "archiveAgeTwo",
                           theFormName,
                           1);
    ccSetDropDownMenuDisabled(prefix + "archiveAgeTwoUnit",
                              theFormName,
                              1);
    ccSetDropDownMenuDisabled(prefix + "mediaTwo",
                              theFormName,
                              1);

    // disable the 'create more copies' button
    ccSetButtonDisabled(prefix + "showCopy3and4",
                        theFormName,
                        1);
  }
}

function enableCopyThreeChange(field) {
  if (field.checked) {
    // enable copy three archive age & media
    ccSetTextFieldDisabled(prefix + "archiveAgeThree",
                           theFormName,
                           0);
    ccSetDropDownMenuDisabled(prefix + "archiveAgeThreeUnit",
                              theFormName,
                              0);
    ccSetDropDownMenuDisabled(prefix + "mediaThree",
                              theFormName,
                              0);
    // enable create copy four
    ccSetCheckBoxDisabled(prefix + "enableCopyFour",
                          theFormName,
                          0);
  } else {
    // disable copy three archive age & media
    ccSetTextFieldDisabled(prefix + "archiveAgeThree",
                           theFormName,
                           1);
    ccSetDropDownMenuDisabled(prefix + "archiveAgeThreeUnit",
                              theFormName,
                              1);
    ccSetDropDownMenuDisabled(prefix + "mediaThree",
                              theFormName,
                              1);    

    // disable copy 4 to prevent users from creating it before 3
    ccSetCheckBoxDisabled(prefix + "enableCopyFour",
                          theFormName,
                          1);
  }
}

function enableCopyFourChange(field) {
  if (field.checked) {
    // enable copy four archive age & media
    ccSetTextFieldDisabled(prefix + "archiveAgeFour",
                           theFormName,
                           0);
    ccSetDropDownMenuDisabled(prefix + "archiveAgeFourUnit",
                              theFormName,
                              0);
    ccSetDropDownMenuDisabled(prefix + "mediaFour",
                              theFormName,
                              0);
  } else {
    // disable copy four archive age & media
    ccSetTextFieldDisabled(prefix + "archiveAgeFour",
                           theFormName,
                           1);
    ccSetDropDownMenuDisabled(prefix + "archiveAgeFourUnit",
                              theFormName,
                              1);
    ccSetDropDownMenuDisabled(prefix + "mediaFour",
                              theFormName,
                              1);    
  }
}

function handleShowCopy3and4(field) {
  var state = field.form.elements[prefix + "copy3and4Status"].value;

  if (state == "visible") { // toggle to hidden
    $("copy3and4Div").style.display="none";
    field.form.elements[prefix + "copy3and4Status"].value = "hidden";
    field.value = message[0];

    // uncheck create copy three and four
    var ff = field.form.elements[prefix + "enableCopyThree"];
    enableCopyThreeChange(ff.checked=false);
    ff = field.form.elements[prefix + "enableCopyFour"];
    enableCopyFourChange(ff.checked=false);

  } else {
    $("copy3and4Div").style.display="";
    field.form.elements[prefix + "copy3and4Status"].value="visible";
    field.value = message[1];
  }

  return false;
}

function validateArchiveAge(ageFields) {
  var valid = true;
  for (var i = 0; i < ageFields.length && valid; i++) {
    var archiveAge = theForm.elements[prefix + ageFields[i]].value;
    if (!isPositiveInteger(archiveAge)) {
      valid = false;
    }
  }

  return valid;
}

function handleNextButton() {
  // if using an existing policy, there is nothing to validate. 
  // go ahead and submit the form
  if (theForm.elements[prefix + "ptype"].value == "existing")
    return true;

  // validate archive age & media pools if creating a new policy
  var mediaFields = new Array();
  var ageFields = new Array();

  var index = -1;
  mediaFields[++index] = "mediaOne";
  ageFields[index]= "archiveAgeOne";

  if (theForm.elements[prefix + "enableCopyTwo"].checked) {
    mediaFields[++index] = "mediaTwo";
    ageFields[index] = "archiveAgeTwo";
  }
  if (theForm.elements[prefix + "enableCopyThree"].checked) {
    mediaFields[++index] = "mediaThree";
    ageFields[index] = "archiveAgeThree";
  }
  if (theForm.elements[prefix + "enableCopyFour"].checked) {
    mediaFields[++index] = "mediaFour";
    ageFields[index] = "archiveAgeFour";
  }

  // validate archive ages
  if (!validateArchiveAge(ageFields)) {
    alert(message[3]);
    return false;
  }
    
  // make sure all copies are not assigned the same pool
  var samePool = true;
  var length = mediaFields.length;
  for (var i = 1; length > 1 && samePool && i < length; i++) {
    if (theForm.elements[prefix + "mediaOne"].value !=
        theForm.elements[prefix + mediaFields[i]].value ) {
      samePool = false;
    }
  }
  
  if (samePool && mediaFields.length > 1) {
    return confirm(message[2]);
  }

  return true;
}
