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

// ident	$Id: DataClassDetails.js,v 1.10 2008/12/16 00:10:36 am143972 Exp $

    function isFSDeletable(field) {
        var deletable = field.form["DataClassDetails.fsDeletable"].value;       
        return deletable == "true";
    }
    
    function handleDataClassDetailsFSTableSelection(field) {
        var disabled  = true;
        var fsRemove =  "DataClassDetails.DataClassDetailsView.RemoveFS";

        if (field.name !=
            "DataClassDetails.DataClassDetailsView." +
            "DataClassDetailsFSTable.DeselectAllHref") {
            disabled = !isFSDeletable(field);
            selectedIndex = field.value;
        }
        ccSetButtonDisabled(fsRemove, "DataClassDetailsForm", disabled);
    }
    
    /*
     * handler for Criteria Details Page -> File Systems Table -> Remove Button
     */
    function handleFSRemove(field) {
        var msg = field.form["DataClassDetails.fsDeleteConfirmation"].value;
        var allFS = field.form["DataClassDetails.fsList"].value.split(",");
        field.form["DataClassDetails.fsname"].value = allFS[selectedIndex];
        return confirm(msg);
    }

    /*
     * Return the current server name that is being managed
     */
    function getServerKey() {
        return document.DataClassDetailsForm.elements[
            "DataClassDetails.ServerName"].value;
    }
    
    /*
     * launch the apply criteria to file system poup
     */
    function launchApplyCriteriaPopup(field) {
        var uri = "/archive/ApplyCriteriaToFileSystem";
        var name = "apply_criteria_fs";
        var psAttributes = field.form["DataClassDetails.psAttributes"].value;
        var params = "&psAttributes=" + psAttributes;

        var w = launchPopup(uri, name, getServerKey(), null, params);
        return false;
    }
    
    /*
     * Show After Date Value (Hack)
     */
    function showAfterDateValue() {
        document.DataClassDetailsForm.elements[
            "DataClassDetails.IncludeDate.textField"].value =
            document.DataClassDetailsForm.elements[
                "DataClassDetails.afterDateHiddenField"].value;
    }

/* class attribute helper functions */

function handleExpirationTimeTypeChange(field) {
  if (field.value == "date") {
    $("dateDiv").style.display="";
    $("durationDiv").style.display="none";
  } else {
    $("dateDiv").style.display="none";
    $("durationDiv").style.display="";
  }
}

function handleDurationTypeChange(field) {
  var formName = field.form.name;

  if (field.value == "none") {
    ccSetTextFieldDisabled("DataClassDetails.duration", formName, 1);
    ccSetDropDownMenuDisabled("DataClassDetails.durationUnit", formName, 1);
  } else {
    ccSetTextFieldDisabled("DataClassDetails.duration", formName, 0);
    ccSetDropDownMenuDisabled("DataClassDetails.durationUnit", formName, 0);
  }
}

function handleOnDedupClick(field) {
  if (field.checked) {
    $("absolutePathDiv").style.visibility="visible";
  } else {
    $("absolutePathDiv").style.visibility="hidden";
  }
}

function handleOnPeriodicAuditChange(field) {
  if (field.value == "none") {
    $("auditPeriodDiv").style.visibility="hidden";
  } else {
    $("auditPeriodDiv").style.visibility="visible";
  }
}

function initializeDataClassAttributes() {
  var dedupElement = $("dedup");
  var theForm = dedupElement.form;
  var exptElement = theForm.elements["DataClassDetails.expirationTimeType"];
  var paElement = $("DataClassDetails.periodicaudit");

  handleOnDedupClick(dedupElement);
  handleExpirationTimeTypeChange(exptElement);
  handleOnPeriodicAuditChange(paElement);
}

/* helper function to retrieve element by id */
function $(id) {
    return document.getElementById(id);
}
