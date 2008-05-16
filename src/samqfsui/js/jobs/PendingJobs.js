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

// ident	$Id: PendingJobs.js,v 1.5 2008/05/16 19:39:14 am143972 Exp $

/** 
 * This is the javascript file of All Jobs page
 */

function toggleDisabledState() {

    var disabled = true;
    var prefix        = "PendingJobs.PendingJobsView.";
	var prefixTiled = prefix + "PendingJobsTiledView[";

    var actionButton0 = prefix + "Button0";

    // checkbox or radioButton for row selection
    var elementName1   = prefix + "PendingJobsTable.Selection";

    var formName      = "PendingJobsForm";
    var myForm        = document.PendingJobsForm;

	var modelIndex = -1;
	var jobType = "";
	var i = 0;

	// get the selected row index

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

	// get the job type in the hidden field
	var elementName2 = prefixTiled + modelIndex + "].JobHiddenType";

	for (i = 0; i < myForm.elements.length; i++) {
	
		var e = myForm.elements[i];
		if (e.name.indexOf(elementName2) != -1) {
	
		    jobType = myForm.elements[i].value;
		    if (jobType == "Jobs.jobType1" || 
			jobType == "Jobs.jobType5") {
			disabled = true;
			break;
		    }
		}
	}

    // Toggle action buttons disable state

    ccSetButtonDisabled(actionButton0, formName, disabled);
}

function showConfirmMsg(key) {

	var str1 = document.PendingJobsForm.elements["PendingJobs.ConfirmMsg"].value;

	if (key == 1) {
	if (!confirm(str1))
		return false;
	else return true;
	 } else return false; // this case should never be used

}
