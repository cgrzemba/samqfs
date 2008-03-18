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

// ident	$Id: AllJobs.js,v 1.5 2008/03/17 14:40:27 am143972 Exp $

/** 
 * This is the javascript file of All Jobs page
 */

function toggleDisabledState() {

    var disabled = true;
    var prefix        = "AllJobs.AllJobsView.";
    var prefixTiled = prefix + "AllJobsTiledView[";

    var actionButton0 = prefix + "Button0";

    // checkbox or radioButton for row selection
    var elementName1   = prefix + "AllJobsTable.Selection";

    var formName      = "AllJobsForm";
    var myForm        = document.AllJobsForm;

    var modelIndex = -1;
    var jobType = "";
    var jobCondition = "";
    var i = 0;

    for (i = 0; i < myForm.elements.length; i++) {

        var e = myForm.elements[i];

        if (e.name.indexOf(elementName1) != -1) {
            if (e.checked) {
                disabled = false;
                modelIndex = myForm.elements[i].value;
                break;
            }
        }
    }

    var elementName2 = prefixTiled + modelIndex + "].JobHiddenType"; 
    var elementName3 = prefixTiled + modelIndex + "].JobHiddenCondition";

    for (i = 0; i < myForm.elements.length; i++) {

        var e = myForm.elements[i];
        if (e.name.indexOf(elementName2) != -1) {
            jobType = myForm.elements[i].value;

            // split this string
            var jobTypeSplit = jobType.split(",");			

            // archive scan, archive copy with
            // condition as pending		
            if (jobTypeSplit[0] == "Jobs.jobType1" ||
                jobTypeSplit[0] == "Jobs.jobType5") {

                if (jobTypeSplit[1] == "Jobs.conditionPending") {
                    disabled = true;
                    break;
                }
            }
            // release with current condition
            if (jobTypeSplit[0] == "Jobs.jobType3") { 		
                if (jobTypeSplit[1] == "Jobs.conditionCurrent") {
                    disabled = true;
                    break;
                }
            }
            // samfsck with condition repair
            if (jobType == "Jobs.jobType6,Jobs.conditionCurrent,repair") {
                disabled = true;
                break;
            }

            // metadata dump cannot be cancelled
            // release with current condition
            if (jobTypeSplit[0] == "Jobs.jobType.dump") { 		
                if (jobTypeSplit[1] == "Jobs.conditionCurrent") {
                    disabled = true;
                    break;
                }
            }
        }
    }	 	

    // Toggle action buttons disable state

    ccSetButtonDisabled(actionButton0, formName, disabled);
}

function showConfirmMsg(key) {

var str1 = document.AllJobsForm.elements["AllJobs.ConfirmMsg"].value;

if (key == 1) {
if (!confirm(str1))
	return false;
else return true;
 } else return false; // this case should never be used

}
 
