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

// ident	$Id: blockallocation.js,v 1.1 2008/07/16 21:55:55 kilemba Exp $

var prefix = "WizardWindow.Wizard.NewWizardBlockAllocationView.";
var formName = "wizWinForm";

function handleAllocationMethodChange(radio) {
  var allocationMethod = radio.value;
  var theForm = document.forms[formName];

  if (allocationMethod == "single") {
    // hide these fields
    theForm.elements[prefix + "stripedGroupText"].style.display = "none";
    theForm.elements[prefix + "blockSizeDropDown"].style.display = "none";

    // show these fields
    theForm.elements[prefix + "blockSizeText"].style.display="";
    theForm.elements[prefix + "blockSizeUnit"].style.display="";
  } else if (allocationMethod == "dual") {
    // hide these fields
    theForm.elements[prefix + "blockSizeText"].style.display="none";
    theForm.elements[prefix + "blockSizeUnit"].style.display="none";
    theForm.elements[prefix + "stripedGroupText"].style.display = "none";

    // show these fields
    theForm.elements[prefix + "blockSizeDropDown"].style.display = "";
  } else if (allocationMethod == "striped") {
    // hide these fields
    theForm.elements[prefix + "blockSizeDropDown"].style.display = "none";

    // show these fields
    theForm.elements[prefix + "blockSizeText"].style.display="";
    theForm.elements[prefix + "blockSizeUnit"].style.display="";
    theForm.elements[prefix + "stripedGroupText"].style.display = "";
  }
}
