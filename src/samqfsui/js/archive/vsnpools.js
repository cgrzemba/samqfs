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

// ident	$Id: vsnpools.js,v 1.10 2008/04/03 02:21:38 ronaldso Exp $

var POPUP_URI = "/archive/NewEditVSNPool";

function getPageName(thisForm) {
    var strArray = thisForm.action.split("/");
    return strArray[strArray.length - 1];
}
    
function getServerKey(field) {
    return field.form.elements[getPageName(field.form) + ".ServerName"].value;
}

function launchNewPoolPopup(field) {
    var params = "&OPERATION=NEW";
    var win = launchPopup(POPUP_URI,
                          "new_vsn_pool", 
                          getServerKey(field),
                          SIZE_VOLUME_ASSIGNER,
                          params);
    return false;
}

function launchEditPoolPopup(field) {
  var params = "&OPERATION=EDIT";

  var poolName = getSelectedPoolName(field);
  params += "&SAMQFS_vsn_pool_name=" + poolName;
  
  var win = launchPopup(POPUP_URI,
                        "edit_vsn_pool",
                        getServerKey(field),
                        SIZE_VOLUME_ASSIGNER,
                        params);

  return false;
}

// table selection related global variables
var selectedIndex = null;

function handleDeletePoolButton(field) {
    var theForm = field.form;
    var confirmMsg =
        theForm.elements["VSNPoolSummary.deleteConfirmation"].value;

    if (!confirm(confirmMsg))
        return false;

    var selectedPoolName = getSelectedPoolName(field);    
    theForm.elements["VSNPoolSummary.VSNPoolSummaryView.selectedPool"].value =
        selectedPoolName;
}

function handleDetailsDeletePoolButton(field) {
    var confirmMsg = 
        field.form.elements["VSNPoolDetails.deleteConfirmation"].value;

    return confirm(confirmMsg);
}

/* handle table selection */
function handlePoolSelection(field) {
    var prefix = "VSNPoolSummary";
  
    // table container view is usually ... <pageName>.<pageNameView>
    var buttonPrefix = prefix + "." + prefix + "View";
    var editButton = buttonPrefix + ".Edit";
    var deleteButton = buttonPrefix + ".Delete";
  
    var formName = document.forms[0].name;

    if (field != null && isTableDeselectButton(field)) {
        ccSetButtonDisabled(editButton, formName, 1);
        ccSetButtonDisabled(deleteButton, formName, 1);
    } else if (field != null) {
        ccSetButtonDisabled(editButton, formName, 0);

        // check for pools in use
        if (!isPoolInUse(field.value)) {
          ccSetButtonDisabled(deleteButton, formName, 0);
        } else {
          // make sure to disable pool if selected after selecting a pool thats
          // not in use
          ccSetButtonDisabled(deleteButton, formName, 1);
        }

        selectedIndex = field.value;
    } else {
        // User uses FORWARD/BACK button
        var myForm = document.forms[0];
        var selection = buttonPrefix + ".VSNPoolSummaryTable.Selection";
        for (i = 0; i < myForm.elements.length; i++) {
            var e = myForm.elements[i];
            if (e.name.indexOf(selection) != -1) {
                if (e.checked) {
                    selectedIndex = myForm.elements[i].value;
                    ccSetButtonDisabled(editButton, formName, 0);

                    if (!isPoolInUse(selectedIndex)) {
                        ccSetButtonDisabled(deleteButton, formName, 0);
                    } else {
                        ccSetButtonDisabled(deleteButton, formName, 1);
                    }

                    break;
                }
            }
        }
    }
}

function isPoolInUse(fieldValue) {
    var theForm = document.forms[0];
    var inUsePools =
        theForm.elements["VSNPoolSummary.poolsInUse"].value.split(";");

    for (var i = 0; i < inUsePools.length - 1; i++) {
        if (fieldValue == inUsePools[i])
        return true;
    }
    return false;
}

function getSelectedPoolName(field) {
    var theForm = field.form;
    var prefix = getPrefix(field);
    var poolNames = theForm.elements[prefix + ".poolNames"].value.split(";");

    return poolNames[selectedIndex];
}



function handleVSNRadio(field) {
    var prefix = getPrefix(field);

    if (field.value == "startend") {
        ccSetTextFieldDisabled(prefix + ".start", field.form.name, 0);
        ccSetTextFieldDisabled(prefix + ".end", field.form.name, 0);
        ccSetTextFieldDisabled(prefix + ".vsnRange", field.form.name, 1);
    } else {
        ccSetTextFieldDisabled(prefix + ".start", field.form.name, 1); 
        ccSetTextFieldDisabled(prefix + ".end", field.form.name, 1);
        ccSetTextFieldDisabled(prefix + ".vsnRange", field.form.name, 0);
    }
}

function handleCloseButton(button) {
    if (button.value == "Cancel") {
        window.close();
        return false;
    }

    var parentForm = opener.document.forms[0];
    if (parentForm.name == "CommonTasksForm") {
        var url = "/samqfsui/archive/VSNPoolSummary";
        var serverName = parentForm.elements["CommonTasks.serverName"].value;
        url += "?SERVER_NAME=" + serverName;

        opener.document.location.href = url;
        window.close();
        return false;
    }

    return cleanupPopup(button);
}
