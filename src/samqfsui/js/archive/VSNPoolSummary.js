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

// ident	$Id: VSNPoolSummary.js,v 1.6 2008/03/17 14:40:25 am143972 Exp $

    function toggleDisabledState() {

      var disabled = true;
      var deletedisabled = true;
      var index = -1;
      
      var prefix        = "VSNPoolSummary.VSNPoolSummaryView.";
      var prefixTiled    = prefix + "VSNPoolSummaryTiledView[";
      
      var actionEdit = prefix + "Edit";
      var actionDelete = prefix + "Delete";
      
      // checkbox or radioButton for row selection
      var elementName   = prefix + "VSNPoolSummaryTable.Selection";
      var elementName2   = prefix +
        "VSNPoolSummaryTable.SelectionRadiobutton";
      
      var formName      = "VSNPoolSummaryForm";
      var myForm        = document.VSNPoolSummaryForm;
      
      var j = 0;
      var modelIndex = -1;
      
      for (i = 0; i < myForm.elements.length; i++) {
        
        var e = myForm.elements[i];
        
        if (e.name.indexOf(elementName) != -1) {
          if (e.checked) {
            index = j;
            disabled = false;
            break;
          }
          j++;
        }
      }
      
      // Get the model tiled view index of the selected row
      j = 0;
      for (i = 0; i < myForm.elements.length; i++) {
        var e = myForm.elements[i];
        
        if (e.name.indexOf(elementName2) != -1) {
          if (j == index) {
            modelIndex = myForm.elements[i].value;
          }
          j++;
        }
      }
      // Finally, get the use status of the vsnpool in selected row
      var elementName3 = prefixTiled + modelIndex + "].HiddenStatus";
      var isuse = "";
      
      for (i = 0; i < myForm.elements.length; i++) {
        var e = myForm.elements[i];
        
        if (e.name.indexOf(elementName3) != -1) {
          isuse = myForm.elements[i].value;
          break;
        }
      }
      
      if (isuse == "false")
		deletedisabled = false;
      
      // Toggle action buttons disable state
      
      ccSetButtonDisabled(actionEdit, formName, disabled);
      ccSetButtonDisabled(actionDelete, formName, deletedisabled);
    }

function showConfirmMsg(key) {
  
  var str1 = document.VSNPoolSummaryForm.elements["VSNPoolSummary.ConfirmMsg"].value;
  
  if (key == 1) {
	if (!confirm(str1))
      return false;
	else return true;
  } else return false; // this case should never be used
  
}

function getSelectedPool() {
  var myForm        = document.VSNPoolSummaryForm;
  var prefix        = "VSNPoolSummary.VSNPoolSummaryView.";
  var prefixTiled   = prefix + "VSNPoolSummaryTiledView[";
  var elementName1  = prefix + "VSNPoolSummaryTable.Selection";
  var i = 0, modelIndex = 0;
  
  // Get the selected Row Index
  for (i = 0; i < myForm.elements.length; i++) {
    var e = myForm.elements[i];
    
    if (e.name.indexOf(elementName1) != -1) {
      if (e.checked) {
        modelIndex = myForm.elements[i].value;
        break;
      }
    }
  }
  
        // Finally, get vsnpool name in the hidden field
  var elementName2 = prefixTiled + modelIndex + "].VSNPoolHiddenField";
  var poolName = myForm.elements[elementName2].value;
  return poolName;
}


var prefix = "VSNPoolSummary.VSNPoolSummaryView.";

function handleDeleteButton(field) {
  var msg =
    document.VSNPoolSummaryForm.elements["VSNPoolSummary.ConfirmMsg"].value;

  if (confirm(msg)) {
    // TODO: this method is very inefficient and should be rewritten at some
    // point 
    var theForm = document.VSNPoolSummaryForm;
    var poolName = getSelectedPool();
    theForm.elements[prefix + "selectedPool"].value = poolName;
    
    return true;
  } else {
    return false;
  }
}
