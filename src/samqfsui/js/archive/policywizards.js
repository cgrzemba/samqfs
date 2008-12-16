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

// ident	$Id: policywizards.js,v 1.12 2008/12/16 00:10:36 am143972 Exp $

/*
 * Javascript for the archive policy wizards:
 *   - New Archive Policy Wizard
 *   - New Criteria Wizard
 *   - new Copy Wizard
**/

var policyTypePrefix = "WizardWindow.Wizard.NewPolicyWizardSelectTypeView.";

function handlePolicyTypeSelection(field) {
    var nameField = field.form[policyTypePrefix + "PolicyNameTextField"];
    var numCopyField = field.form[policyTypePrefix + "NumCopiesTextField"];

    if (field.value == 'NoArchive') {
        nameField.value="no_archive";
        nameField.disabled=true;
        numCopyField.disabled=true;
    } else {
        if (nameField.value == 'no_archive') {
            nameField.value = "";
        }
        nameField.disabled=false;
        numCopyField.disabled=false;
        nameField.focus();
    }
}


/* functions used by the copy media parameters page of the new policy 
 * & new copy wizards 
**/
var copyMediaPrefix = "WizardWindow.Wizard.CopyMediaParametersView.";
var copyMediaExistingDrops = ["mediaType", "poolName"];
var copyMediaExistingTexts = ["from", "to", "list"];
var copyMediaNewDrops = ["host"];
var copyMediaNewTexts = ["name", "path"];

var rmPolicy = copyMediaPrefix + "rmPolicy";
var rmFS = copyMediaPrefix + "rmFS";
var rmAttributes = copyMediaPrefix + "rmAttributes";
var DISK = "133";
var STK_5800 = "137";
var NOVAL = "-999";
var NOVAL_LABEL = "--";
var poolMediaTypes = copyMediaPrefix + "poolMediaTypes";
var createPath = copyMediaPrefix + "createPath";

/*
function handleExistingVSNSelection(field) {
    var formName = field.form.name;

    // disable all new disk vsn fields
    disableDropDowns(copyMediaPrefix, copyMediaNewDrops, formName, 1);
    disableTextFields(copyMediaPrefix, copyMediaNewTexts, formName, 1);
    ccSetCheckBoxDisabled(createPath, formName, 1);

    // enable all existing vsn fields
    disableDropDowns(copyMediaPrefix, copyMediaExistingDrops, formName, 0);
    disableTextFields(copyMediaPrefix, copyMediaExistingTexts, formName, 0);
    if (field.form[copyMediaPrefix + "mediaType"].value != DISK) {
        disableRMFields(formName, 0);
    }
}

function handleNewDiskVSNSelection(field) {
    var formName = field.form.name;

    // disable all existing vsn fields
    disableDropDowns(copyMediaPrefix, copyMediaExistingDrops, formName, 1);
    disableTextFields(copyMediaPrefix, copyMediaExistingTexts, formName, 1);
    disableRMFields(formName, 1);

    // enable all new disk vsn fields
    disableDropDowns(copyMediaPrefix, copyMediaNewDrops, formName, 0);
    disableTextFields(copyMediaPrefix, copyMediaNewTexts, formName, 0);
    ccSetCheckBoxDisabled(createPath, formName, 0);
}
*/

function disableTextFields(prefix, fields, formName, disable) {
    for (var i = 0; i < fields.length; i++) {
        ccSetTextFieldDisabled(prefix + fields[i], formName, disable);
    }
}

function disableDropDowns(prefix, fields, formName, disable) {
    for (var i = 0; i < fields.length; i++) {
        ccSetDropDownMenuDisabled(prefix + fields[i], formName, disable);
    }
}

function handleMediaTypeChange(field) {
    var formName = field.form.name;
    var theForm = field.form;
    var poolName = copyMediaPrefix + "poolName";

    // disable fields when necessary
    if (field.value == DISK ||
        field.value == STK_5800) {
        disableRMFields(formName, 1);
    } else {
        if (field.value != NOVAL) {
            disableRMFields(formName, 0);
        } else {
            // set all the pools
            var allPools = 
                theForm[copyMediaPrefix + "allPools"].value.split(";");

            // remove the trailing empty element
            var size = allPools.length - 1;
            theForm.elements[poolName].options.length = size + 1;
            theForm.elements[poolName].options[0] = 
                new Option(NOVAL_LABEL, NOVAL);

            var j = 1;
            for (var i = 0; i < size; i++) {
                theForm.elements[poolName].options[j] = 
                    new Option(allPools[i], allPools[i]);
                j++;
            }

            // all done
            return;
        }
    }

    // parse the media type -> pool string
    var mtPools = theForm[poolMediaTypes].value.split(";");
    var found = false;
    var poolArrayRaw = null;

    // remove the trailing empty element
    var size = mtPools.length - 1;
    for (var i = 0; i < size && !found; i++) {
         var tempPoolArray = mtPools[i].split("=");
        if (tempPoolArray[0] == field.value) {
            found = true;
            poolArrayRaw = tempPoolArray[1];
        }
    }

    if (!found) {
        theForm.elements[poolName].options.length = 1;
        theForm.elements[poolName].options[0] = new Option(NOVAL_LABEL, NOVAL);

        return;
    }
    // set appropriate pools
    var pools = poolArrayRaw.split(",");
    var size = pools.length - 1; 
    theForm.elements[poolName].options.length = size;
    theForm.elements[poolName].options[0] = new Option(NOVAL_LABEL, NOVAL);
    for (var i = 1; i < size; i++) {
        theForm.elements[poolName].options[i] = new Option(pools[i], pools[i]);
    }
}

function disableRMFields(formName, disable) {
    ccSetDropDownMenuDisabled(rmAttributes, formName, disable);
    ccSetCheckBoxDisabled(rmPolicy, formName, disable);
    ccSetCheckBoxDisabled(rmFS, formName, disable);
}
