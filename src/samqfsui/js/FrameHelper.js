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

// ident	$Id: FrameHelper.js,v 1.11 2008/05/16 19:39:12 am143972 Exp $

/** 
 * This file is used to construct the frames format, and it also contains
 * a few wrapper javascript function to refresh certain frames, and to
 * highlight the appropriate tree node in the navigation system.
 */

// The following methods are used in FrameFormat.jsp //

    // nodeID has to be the same as what is defined in NavigationNodes.java
    function setSelectedNode(nodeID, pageID) {
        // highlight the appropriate node in the navigation tree
        top.frames.FrameNavigator.FrameNavigator_NavigationTree.yokeTo(nodeID);
        
        // now refresh the frame header and set the content page name
        // without setting page name, the HELP button will not work properly
        var URL = "/samqfsui/util/FrameHeader?SERVER_NAME=" + top.serverName +
                    "&PAGE_NAME=" + pageID;
        top.frames.FrameHeader.location.href = URL;
    }

    function getLocationParam(locString, key) {
        var start = locString.indexOf("?" + key + "=");
        if (start < 0) {
            start = locString.indexOf("&" + key + "=");
        }

        if (start < 0) {
            return '';
        }
        start += key.length + 2;
        var end = locString.indexOf("&", start);
        if (end < 0) {
            end = locString.length;
        }
        var result = locString.substring(start, end);
        return unescape(result);
    }

    function writeFrameEntry(
        frameID, scrolling, marginwidth, marginheight, serverName, destination)
    {
        // Pass all the request into each of the frame URL
        // serverName is a part of this request
        
        document.write(
            '<frame name="' + frameID + '" scrolling="' + scrolling + '"');
        document.write('marginwidth="' + marginwidth + '"');
        document.write('marginheight="' + marginheight + '"');
        
        var whereToGo = "";
        if (destination == "firsttime") {
            // TODO: Change to First Time Configuration Page
            whereToGo = "fs/FSSummary";
        } else if (destination == "lib") {
            whereToGo = "media/LibrarySummary";
            
        } else if (destination == "alarm") {
            whereToGo = "alarms/CurrentAlarmSummary";
        } else {
            whereToGo = "fs/FSSummary";
        }

        if (frameID == "Content") {
            document.write(
                'src="/samqfsui/' + whereToGo + '?SERVER_NAME='
                + serverName + '">');
            // reset destination
            destination = "fs";
        } else {
            document.write(
                'src="/samqfsui/util/' + frameID +
                '?SERVER_NAME=' + serverName + '">');
        }
    }

// The following methods are used in FrameHeader.jsp //

    function getContentFrameDocument() {
        return parent.Content.document;
    }

    function getContentFrameForm() {
        // assume one form per page
        return getContentFrameDocument().forms[0];
    }
    
    function getContentFramePageName() {
        // assume Form name is constructed by PAGE_NAME + "Form"
        var tmpArray = getContentFrameForm().name.split("Form");
        return tmpArray[0];
    }

    function forwardToAlarmSummary() {
        parent.Content.location.href =
            '/samqfsui/alarms/CurrentAlarmSummary?SERVER_NAME='
            + top.serverName;
    }

// The following method is used in FrameNavigator.jsp

    function handleServerMenuOnChange(menu) {
        var changeTo = menu.value;
        top.serverName = changeTo;
        parent.Content.location.href =
            '/samqfsui/util/CommonTasks?SERVER_NAME=' + changeTo;
        parent.FrameNavigator.location.href =
            '/samqfsui/util/FrameNavigator?SERVER_NAME=' + changeTo;
        parent.FrameHeader.location.href =
            '/samqfsui/util/FrameHeader?SERVER_NAME=' + changeTo;
    }

