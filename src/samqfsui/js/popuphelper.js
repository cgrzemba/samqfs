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

// ident	$Id: popuphelper.js,v 1.16 2008/05/16 19:39:12 am143972 Exp $

// window sizes
// If you add a size here, you must add a else if case in launchPopUp so
// the pop up window will have the centralize position on the screen    
var SIZE_MONITOR_CONSOLE = "height=500,width=1000";
var SIZE_VOLUME_ASSIGNER = "height=700,width=800";
var SIZE_LARGE = "height=700,width=900";
var SIZE_NORMAL= "height=600,width=700";
var SIZE_SMALL= "height=350,width=700";
var SIZE_DETAILS = "height=600,width=500";
var SIZE_TINY = "height=250,width=600";
var APP_CONTEXT = "/samqfsui";
var POPUP_ATTRIBUTE = "?com_sun_web_ui_popup=true";

var myPopup = null;

/*
 * access : public
 * 
 * alternative launch method which takes uri and name
 */
function launchPopup(uri, name) {
  return launchPopup(uri, name, getServerName());
}

/* 
 * access : public
 *
 * alternative launch method which takes only the required parameters
 */
function launchPopup(uri, name, serverName) {
  return launchPopup(uri, name, serverName, SIZE_NORMAL, null);
}

/*
 * access : public
 *
 * alternative launch method which takes the required parameters and the size
 */
function launchPopup(uri, name, serverName, size) {
  return launchPopup(uri, name, serverName, size, null);
}

/*
 * access : public
 *
 * the official function to launch popups.
 *
 * parameters:
 * uri - the uri of the popup e.g. /archive/NewDiskVSN
 * name - name of the popup window e.g. new_disk_vsn
 * serverName - name of the SAM/QFS server being managed
 * size - the dimensions of the popup window [optional]
 * params - extra parameters to be appended to the request
 *          should be of the format: &name=value&name=value [optional]
 *
 * returns 
 * window - the popup window object
 */
function launchPopup(uri, name, serverName, size, params) {
  // verify that the mandatory parameters are provided.
  if (uri == null ||
      name == null) {
    // we should never get here
    alert("Error! uri, and popup name are required parameters.");
    return;
  }

  // retrieve the server name
  if (serverName == null) {
    serverName = getServerName();
  } 

  if (size == null) {
    size = SIZE_NORMAL;
  }

  // construct the window properties
  var default_height = 600;
  var default_width = 700;

  if (size == SIZE_MONITOR_CONSOLE) {
    default_height = 500;
    default_width  = 1000;
  } else if (size == SIZE_LARGE) {
    default_height = 700;
    default_width  = 900;
  } else if (size == SIZE_SMALL) {
    default_height = 350;
    default_width  = 700;
  } else if (size == SIZE_DETAILS) {
    default_height = 600;
    default_width  = 500;
  } else if (size == SIZE_TINY) {
    default_height = 250;
    default_width = 600;
  }

  var sc = window.screen;

  var properties = "scrollbars=1,resizable=1";
  var location = ",screenY=" + (sc.availHeight/2 - default_height/2) +
                 ",screenX=" + (sc.availWidth/2 - default_width/2);

  properties += location;
  properties += "," + size;
  
  // construct the complete url
  var url = APP_CONTEXT + uri + POPUP_ATTRIBUTE + "&SERVER_NAME=" + serverName;
  if (params != null) {
    url = url + params;
  }
  var win = window.open(url, name, properties);
  if (win != null) {
    win.focus();
  }
  return win;
}

/* 
 * access : public
 * 
 * this function is used to initialize popups. It should be called from the 
 * when the popup page loads like so 
 * <body onLoad="initializePopup(this);" >
 */
function initializePopup(field) {
  myPopup = new Popup(name, opener.document.forms[0].name, opener.location);
}

/*
 * access : public
 *  
 * once the popup has completed its task, it should call this function to 
 * perform basic clean up, close the popup window and refresh the parent page 
 * if necessary.
 */
function cleanupPopup(field) {

  // if the parent page hasn't changed, refresh it.
  if (opener.document.forms[0].name == myPopup.parentForm) {
    refreshParentPage(field);
  }
  
  window.close();
  return false;
}

/* 
 * access : private
 *
 * part of popup helper's private interface. This function should NOT be called 
 * directly.
 */
function refreshParentPage(field) {
  var parentForm = opener.document.forms[0];
  var popupHandler = ".RefreshHref";

  // retrieve the container view name by removing the word form from the parent
  // form name
  var pfArray = myPopup.parentForm.split("Form");
  popupHandler = pfArray[0] + popupHandler;

  // retrieve the action that loaded the parent form and remove the original
  // query string
  var pfaArray = parentForm.action.split("?");
  var action = pfaArray[0];

  // retrieve the page session string from the parent page and append it to the
  // query string
  var psString = parentForm.elements["jato.pageSession"].value;

  var queryString = "?" + popupHandler + "=&jato.pageSession=" + psString;

  // set the new action and submit the form
  parentForm.action = action + queryString;

  // finally submit the form
  parentForm.submit();
}

/* 
 * access : private
 * 
 * part of popup helper's private interface. This function should NOT be called 
 * directly.
 */
function Popup(name, parentForm, parentLocation) {
  // data members
  this.name = name;
  this.parentForm = parentForm;
  this.parentLocation = parentLocation;
}

/*
 * access : private
 * retrieve the currently managed servername from the action drop down menu
 *
 * NOTE: For now, this is a private function that should not be called outside
 * of this file. It relies on the server drop down being named: 
 * ChangeServerActionMenu
 *
 * This method is modified due to the use of Tree Navigation.
 * Simply call top.serverName to grab the server name instead of retreiving
 * the name from the server drop down, which is no longer exists in the
 * scope tag.
 */
function getServerName() {
  return top.serverName;
}

/* these should probably go to the samqsfui.js file */
/* TODO: should be moved to a common place */
function getPrefix(field) {
  // just incase field is an href find the form via document
  var theForm = field.form != null ? field.form : document.forms[0];
  
  var rawname = theForm.name.split("Form");
  var prefix = rawname[0];
  
  return prefix;
}

/* another common function */
function isTableDeselectButton(field) {
  return field.name.indexOf("Table.DeselectAllHref") == -1 ? false : true;
}
