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

// ident	$Id: copyvsns.js,v 1.6 2008/03/17 14:40:25 am143972 Exp $

/* handler for the reset button */
function handleResetButton(field) {
    var hardReset = field.form["CopyVSNs.hardReset"].value;

    /* determine if we should perform a soft or hard reset */
    if (hardReset == "true") {
        // perform a hard reset via the backend
        return true;
    } else {
        // perform a soft reset
        field.form.reset();
        return false;
    }

}

var prefix = "CopyVSNs.";
var saveButton = prefix + "Save";
var poolDropDown = prefix + "poolDefined";

/* handler for the media type drop down */
function handleMediaTypeChange(field) {
    var formName = field.form.name;
    var theForm = field.form;

    var poolArray = null;
    // disable/enable save button
    if (field.value == "-999") {
        // disable save
        ccSetButtonDisabled(saveButton, formName, 1);
        poolArray = new Array();
        poolArray[0] = "--";
    } else {
        // enable 
        ccSetButtonDisabled(saveButton, formName, 0);
        poolArray = getPoolsForMediaType(theForm, field.value);
    }

    // vsn pool dropdown
    var size = poolArray.length;
    theForm.elements[poolDropDown].options.length = size;
    for (var i = 0; i < size; i++) {
        theForm.elements[poolDropDown].options[i] = 
            new Option(poolArray[i], poolArray[i]);
    }
}

function getPoolsForMediaType(form, mediaType) {

    var rawArray = form.elements[prefix + "all_pools"].value.split(";");
    var poolArray = null;
    
    for (var i = 0; i < rawArray.length; i++) {
        var temp = rawArray[i].split("=");
        if (temp[0] == mediaType) {
            poolArray = temp[1].split(",");
            break;
        }
    }

    return poolArray;
}
