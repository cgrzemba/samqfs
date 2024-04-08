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

// ident	$Id: scheduled_tasks.js,v 1.8 2008/12/16 00:10:36 am143972 Exp $

var prefix = "ScheduledTasks.";
var theFormName = "ScheduledTasksForm";
var selectedIndex = null;

function setSelectedSchedule(field) {
    var theForm = field.form;
    var selectedName= "ScheduledTasks.ScheduledTasksView.selectedName";
    var selectedId = "ScheduledTasks.ScheduledTasksView.selectedId";

    var names = 
        theForm.elements["ScheduledTasks.allScheduleNames"].value.split(";");
    var ids = 
       theForm.elements["ScheduledTasks.allScheduleIds"].value.split(";");

    theForm.elements[selectedName].value = names[selectedIndex];
    theForm.elements[selectedId].value = ids[selectedIndex];

    return true;
}

function handleEditSchedule(field) {
    if (selectedIndex == null) {
        /* we should never get here, but stranger things have happened */
        return false;
    }

    return setSelectedSchedule(field);
}

function handleRemoveSchedule(field) {
    if (selectedIndex == null) {
        /* we should never get here, but stranger things have happened */
        return false;
    }

    var val = setSelectedSchedule(field);
    var errmsg = field.form.elements["ScheduledTasks.errMsg"].value;
    if (confirm(errmsg)) {
      // submit the form
      return true;
    } else {
      // deselect selected radio button
      // disable new & edit buttons
      // reset selected index

      return false;
    }
}

function handleIdHrefSelection(field) {
  return true;
}

function handleNameHrefSelection(field) {
  return true;
}

function handleScheduleSelection(field) {
    var editButton = prefix + "ScheduledTasksView.Edit";
    var removeButton = prefix + "ScheduledTasksView.Remove";
    
    if (isDeselectAllClicked(field)) {
        // disable the edit & remove button
        ccSetButtonDisabled(editButton, theFormName, 1); 
        ccSetButtonDisabled(removeButton, theFormName, 1);
        
        // clear selected index
        selectedIndex = null;
    } else {
        // enable edit & remove button
        ccSetButtonDisabled(editButton, theFormName, 0);
        ccSetButtonDisabled(removeButton, theFormName, 0);
        
        // set selected index
        selectedIndex = field.value;
    }

 return true;
}

function isDeselectAllClicked(field) {
  return field.name.indexOf("DeselectAllHref") != -1;
}

function message() {
  alert("coming soon ...");
  return false;
}
