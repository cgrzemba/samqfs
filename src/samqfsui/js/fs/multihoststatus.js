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

// ident	$Id: multihoststatus.js,v 1.3 2008/10/22 20:57:03 kilemba Exp $

// NOTE: This file MUST be used in conjunction with the samqfsui.js

var xmlDoc = null;
var interval = null;
var request = null;
var prefix = "MultiHostStatusForm:";
var done = false;
var hostErrorArray = null;

/* called once the multi host status window is launched */
function mhsOnLoad() {
  // start ajax calls
  start();
}

/* begin the process of repeatedly requesting updates */
function start() {
  // retrieve server name from form
  var serverName = $(prefix + "serverName").value;

  // retrieve server name from form
  var jobId = $(prefix + "jobId").value;
  var action="mult_host_status";


  var params = "requested_action=" + action;
  params = params + "&SERVER_NAME=" + serverName;
  params = params + "&SAMQFS_JOB_ID=" + jobId;

  var url ="/samqfsui/ansyc?" + params;

  // request for data updates every 3 seconds
  interval = setInterval("submitRequest('"+url+"')", 1000);
}

/* end the process of repeatedly requesting updates */
function stop() {
  // clear interval to stop future requests
  clearInterval(interval);
}

/* submit a ajax request */
function submitRequest(url) {
  request = createRequest();

  if (request != null) {
    request.onreadystatechange = handleResponse;
    request.open("POST", url, true);
    request.send(null);
  } else {
    ; // TODO: we need a way to indicate errors
  }
}

/* handle the response from an ajax request */
function handleResponse() {
  if (request.readyState == 4) {
    if (request.status == 200) { // response information is ready to
				 // be processed. 

      xmlDoc = loadXMLString(request.responseText);
      if (xmlDoc != null) { // response xml parsed correctly
	updateSummaryInformation(xmlDoc);

	updateHostErrorList();

	// if there are no more pending hosts, stop ajax calls.
	if (done) stop();
      } // end xmlDoc
    } // end request.status
  } // end request.readyState
}


/* function to update the summary fields i.e.
   - total
   - succeeded
   - failed
   - pending
*/
function updateSummaryInformation(xmlDoc) {
  // total
  var total = getElementValue(xmlDoc, "total");
  $(prefix + "totalText").innerHTML = total;

  // succeeded
  var succeeded = getElementValue(xmlDoc, "succeeded");
  $(perfix + "succeededText").innerHTML = succeeded;

  // failed
  var failed = getElementValue(xmlDoc, "failed");
  $(prefix + "failedText").innerHTML = failed;

  // pending
  var pending = getElementValue(xmlDoc, "pending");
  $(prefix + "pendingText").innerHTML = pending;

  // if there are no pending hosts, update the done criteria
  done = total == (succeeded + failed);
}

/* function to update the list of hosts with erros. This function also
   creates a map of host -> error definitions for a quick look-up of
   host errors when a host is selected.
*/
function updateHostErrorList(xmlDoc) {
  // retrieve all host nodes
  var hostNodes = xmlDoc.getElementsByTagName("host");
  if (hostNodes == null || hostNodes.length == 0)
    return; // no hosts with error found

  hostErrorArray = new Array();
  // loop through the list of hosts with errors and update the host
  // with error array
  for (var i = 0; i < hostNodes.length; i++) {

    var hostName = null;
    var hostError = null;
  }
}

/* 
   function to update the error box for a selected host. This function
   is called when a host is selected in the 'hosts with error' list to
   retrieve the detailed error description returned for that
   particular host.
*/
function handleHostWithErrorSelection(field) {
}

// utility functions for processing XML response 
// NOTE: These functions are specific to the MHS schema -

/* retrieve the text value of the given element. NOTE: this assumes that
   the given tag is an leaf element. Do not call this function if this
   is not the case
*/
function getElementValue(xmlDoc, tagName) {
  var value = null;
  
  var element = xmlDoc.getElementsByTagName(tagName);
  if (element != null) {
    value = element[0].childNodes[0].nodeValue;
  }

  return value;
}

/* retrieve the string value of a given leaf node */
function getNodeValue(node) {
  var value = null;

  if (node != null) {
  }
}

// called when a host in the 'host with error' list is selected
var hostErrorArray = null;
function hostWithErrorSelected(field) {
  var host = field.value;

  if (hostErrorArray == null) {
    parseHostErrorArray();
  }

  var error = hostErrorArray[host];

  $(prefix + "hostErrorDetail").innerHTML = error;

  return false;
}

// parse the list host:error,host:error list to an array indexed by the host
// name
function parseHostErrorArray() {
  hostErrorArray = new Array();

  var raw = $(prefix + "hostError").value;
  var hostError = raw.split(",");
  
  for (var i = 0; hostError != null && i < hostError.length;i++) {
    var rawHostError = hostError[i].split("=");
    var host = rawHostError[0];
    var error = rawHostError[1];

    hostErrorArray[host] = error;
  }
}

// close the multi-host status display window
function handleCloseButton(field) {
  if (window.opener != null) {
    var parentForm = window.opener.document.forms[0];
    if (parentForm != null) {
      parentForm.submit();
    }
  }


  window.close();

  return false;
}
