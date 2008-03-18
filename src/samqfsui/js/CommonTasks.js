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

// ident	$Id: CommonTasks.js,v 1.5 2008/03/17 14:40:24 am143972 Exp $

// Component level functions
var hide  = true;
var tleft;
var ttop; 
var ileft;
var colnum = 17; // number of tasks

// The prized coordinate locating function - Thank you Danny Goodman...
function getElementPosition(elemID) {
    var offsetTrail = document.getElementById(elemID);
    var offsetLeft = 0;
    var offsetTop = 0;


    while (offsetTrail) {
        offsetLeft += offsetTrail.offsetLeft;
        offsetTop += offsetTrail.offsetTop;
        offsetTrail = offsetTrail.offsetParent;
    }
    if (navigator.userAgent.indexOf("Mac") != -1 && 
        typeof document.body.leftMargin != "undefined") {
        offsetLeft += document.body.leftMargin;
        offsetTop += document.body.topMargin;
    }
   tleft=offsetLeft;
   ttop=offsetTop;

   //return {left:offsetLeft, top:offsetTop}; 

}

function getElementPosition2(elemID) {
    var offsetTrail = document.getElementById(elemID);
    var offsetLeft = 0;
    var offsetTop = 0;


    while (offsetTrail) {
        offsetLeft += offsetTrail.offsetLeft;
        offsetTop += offsetTrail.offsetTop;
        offsetTrail = offsetTrail.offsetParent;
    }
    if (navigator.userAgent.indexOf("Mac") != -1 && 
        typeof document.body.leftMargin != "undefined") {
        offsetLeft += document.body.leftMargin;
        offsetTop += document.body.topMargin;
    }
   ileft=offsetLeft;

   //return {left:offsetLeft, top:offsetTop}; 

}


function closeAll(num) {
  for(i=1;i<=colnum;i++) {
    if(document.getElementById("info"+i) && document.getElementById("togImg"+i)) {
      document.getElementById("info"+i).style.display = "none";   
      document.getElementById("togImg"+i).src = "/samqfsui/images/right_toggle.gif";
    }
  }
  document.getElementById("i"+num).focus();
}

function showDiv(num) {
document.getElementById("info"+num).style.display = "block";
}

function hideAllMenus() {
  for(i=1;i<=colnum;i++) {
    if(document.getElementById("info"+i) && document.getElementById("togImg"+i)) {
      document.getElementById("info"+i).style.display = "none";
      document.getElementById("togImg"+i).src = "/samqfsui/images/right_toggle.gif";  
    }
  }
}


// Toggle functions
function test(num) {
  getElementPosition2("togImg"+num);
  if (document.getElementById("info"+num).style.display != "block") {
    for(i=1;i<=colnum;i++) {
      if(i!=num && document.getElementById("togImg"+i) && document.getElementById("info"+i)) {
        document.getElementById("togImg"+i).src = "/samqfsui/images/right_toggle.gif";
        document.getElementById("info"+i).style.display = "none";
      }
    }
    document.getElementById("togImg"+num).src = "/samqfsui/images/right_toggle_selected.gif";

    getElementPosition("gif"+num);

    document.getElementById("info"+num).style.display = "block";
    document.getElementById("info"+num).style.top = (ttop + 10) + 'px';
    document.getElementById("info"+num).style.left = (tleft -1) + 'px';
    document.getElementById("info"+num).style.width = (ileft - tleft) + 29 + 'px';
    
    document.getElementById("close"+num).focus();
  }
  else if (document.getElementById("info"+num).style.display = "block"){
    for(i=1;i<=colnum;i++) {
      if(document.getElementById("togImg"+i)) {
        document.getElementById("togImg"+i).src = "/samqfsui/images/right_toggle.gif";
      }
    }
    document.getElementById("info"+num).style.display = "none";
  }
}


// Hover Functions
function hoverImg(num) {
  if (document.getElementById("info"+num).style.display != "block") {
    document.getElementById("togImg"+num).src = "/samqfsui/images/right_toggle_rollover.gif"
  }
  else { 
    document.getElementById("togImg"+num).src = "/samqfsui/images/right_toggle_selected.gif";
  }
}
function outImg(num) {
  if (document.getElementById("info"+num).style.display != "block") {
    document.getElementById("togImg"+num).src = "/samqfsui/images/right_toggle.gif";
  }
  else {
    document.getElementById("togImg"+num).src = "/samqfsui/images/right_toggle_selected.gif";
  }
}

// Brought in from dcc.js
function getLink(linkName) {
	return getLink(linkName, null);
}

function getLink(linkName, theDocument) {
        var link;
	var links;
        if (theDocument == null) {
           links = document.links;
        } else {
           links = theDocument.links;
        }
        if (links != null) {
	     var i;
             for (i = 0; i < links.length; i++) {
		var e = links[i];
		if (e.name) {
	          if (e.name == linkName) {
	           link = e;
	          }
	        }
             }
        }
	return link;
}

function getValue(elementName, formName) {
	 var theValue = null;
	 var element = ccGetElement(elementName,
				    formName);
	 if (element != null) {
	    if (element.value) {
	       if (element.value != "") {
		  theValue = element.value;
	       }
	    }
	 }
	 return theValue;
}


// FSM specific functions


/* this function is used to forward to pages that do not require any additional
 * information other than the server name
 */
function forwardToPage(uri) {
    var url =  uri + "?SERVER_NAME=" + getServerName();

    document.location.href = url;
}

/* return the name of the server currently being managed */
function getServerName() {
  var serverName =
    document.forms[formName].elements["CommonTasks.serverName"].value;
  return serverName;
}

var prefix = "CommonTasks.CommonTasksWizardsView.";
var formName = "CommonTasksForm";

function getClientParams() {
  // the new fs wizard needs the server name to be passed via a request param 
  var params = "&serverNameParam=" + getServerName();

    return params;
}

function launchWizard(wizardName) {
    var button = getElement(prefix + wizardName);
    if (button != null) {
        button.click();
    } else {
        alert("invalid wizard name : " + wizardName);
    }

    return false;
}

function getElement(name) {
    var theForm = document.forms[formName];

    var element = theForm.elements[name];

    return element;
}

/* launch the new disk vsn popup */
function addDiskVSN() {
  return launchPopup("/archive/NewDiskVSN", 
                     "new_disk_vsn_popup",
                     getServerName());
}

/* launch new vsn pool popup */
function addVSNPool() {
  var params = "&OPERATION=NEW";

  return launchPopup("/archive/NewEditVSNPool",
                     "new_vsn_pool",
                     getServerName(),
                     null,
                     params);
}

/* launch import vsns popup */
function importTapeVSN() {
    launchPopup("/util/LibrarySelector",
                "library_selector",
                getServerName(),
                "height=350,width=700",
                null);
}

/* schedule recovery point */
function scheduleRecoveryPoint() {
    launchPopup("/util/FileSystemSelector",
                "fs_selector",
                getServerName(),
                "height=350,width=700",
                null);
}

/* launch the overview popup */
function launchOverview() {
    launchPopup("/jsp/util/Overview.jsp",
                "overview",
                "servername");
}

function handleImportTapeVSNs(button) {
  var theForm = button.form;
  var s = theForm.elements["LibrarySelector.library"].value;

  var tokens = s.split(":");
  var driverType = tokens[1];

  if (driverType == "1001") { // samst
    return true;
  } else { // go to the import vsn page
    var serverName =
      opener.document.CommonTasksForm.elements["CommonTasks.serverName"].value;
    var url = "/samqfsui/media/ImportVSN" +
      "?SERVER_NAME=" + serverName +
      "&LIBRARY_NAME=" + tokens[0];

    opener.document.location.href = url;
    window.close();
  }
}
