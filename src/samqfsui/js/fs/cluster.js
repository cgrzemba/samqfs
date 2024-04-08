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

// ident	$Id: cluster.js,v 1.13 2008/12/16 00:10:37 am143972 Exp $

/* handler for the fs details -> cluster nodes table -> add button */
function launchAddClusterNodePopup(field) {
  var fsName = field.form.elements["FSDetails.FSDetailsView.fsName"].value;

  var params = "&SAMQFS_FILE_SYSTEM_NAME=" + fsName;
  var name = "add_cluster_node_to_fs";
  var uri = "/fs/AddClusterNodePopup";

  var win = launchPopup(uri, name, null, null, params);
  win.focus();
  return false;
}

/* handler for the fs details -> add popup -> submit button */
function addClusterNodes(field) {

  var theForm = field.form;

  var prefix = "AddClusterNodePopup.FSDAddClusterNodeTable.SelectionCheckbox";
  var selected_nodes = "";
  var all_nodes =
    theForm.elements["AddClusterNodePopup.nodeNames"].value.split(";");

  var foundSelection = false;
  for (var i = 0; i < all_nodes.length - 1; i++) {
    var checkbox_name = prefix + i;
    var checkbox = theForm.elements[checkbox_name];
    if (checkbox.checked) {
      selected_nodes += all_nodes[i] + ";";
      foundSelection = true;
    }
  }

 
  // make sure we a selection before proceeding
  if (!foundSelection) {
    alert(theForm.elements["AddClusterNodePopup.noSelectionMsg"].value);
    return false;
  }

  theForm.elements["AddClusterNodePopup.selectedNodes"].value = selected_nodes;

  return true;
}

var nodeToRemoveIndex = null;
/* handler for the fs details -> cluster node table -> remove button */
function removeClusterNode(field) {
  var theForm = field.form;

  var removeConfirmation =
    theForm.elements["FSDetails.FSDClusterView.removeConfirmation"].value;

  // confirm that the user does infact want to remove the named cluster node.
  if (!confirm(removeConfirmation))
    return false;

  var all_nodes =
    theForm.elements["FSDetails.FSDClusterView.nodeNames"].value.split(";");
  var nodeToRemove = all_nodes[nodeToRemoveIndex];
  
  theForm.elements["FSDetails.FSDClusterView.nodeToRemove"].value =
    nodeToRemove;
  theForm.submit();
}

/* handler for the fs details -> cluster node table radio buttons */
function handleFSDClusterTableSelection(field) {
  var deselectAll = "FSDetails.FSDClusterView.FSDClusterTable.DeselectAllHref";
  var formName = "FSDetailsForm";
  var removeButton = "FSDetails.FSDClusterView.Remove";

  if (field.name == deselectAll) {
    ccSetButtonDisabled(removeButton, formName, 1);
    nodeToRemoveIndex = null;
  } else {
    ccSetButtonDisabled(removeButton, formName, 0);
    nodeToRemoveIndex = field.value;
  }
}

/* handler for the fs details -> add cluster node popup -> table selection */
function handleAddClusterNodeTableSelection(field) {
  ; // nothing to do here for now
}

/* 
 * new fs wizard -> select nodes page -> handler for the selection list.
 */
function handleClusterNodeSelection(field) {
}

/*
 * fsd cluster view -> manage state of table radio buttons
 */
function setClusterNodeSelectionState(theForm) {
}
