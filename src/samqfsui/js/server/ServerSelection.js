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

// ident	$Id: ServerSelection.js,v 1.10 2008/05/16 19:39:15 am143972 Exp $

/**
 * This is the javascript file of Server Selection Page
 */

    var selectedIndex    = -1;
    var prefix           = "ServerSelection.ServerSelectionView.";
    var prefixTiled      = prefix + "ServerSelectionTiledView[";

    function showConfirmMsg() {
        var str1 = document.ServerSelectionForm.elements
            ["ServerSelection.ConfirmMessageHiddenField"].value;

        if (!confirm(str1)) {
            return false;
        } else {
            return true;
        }
    }

    function toggleDisabledState(field) {
        var disabled         = true;
        var viewDisabled     = true;
        var ButtonRemove     = prefix + "RemoveButton";
        var ButtonViewConfig = prefix + "ViewConfigButton";

        // checkbox or radioButton for row selection
        if (field != null) {
            var elementName   = field.name;
            if (elementName != (prefix + "ServerSelectionTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                disabled = false;
                // update selected index
                selectedIndex = field.value;
            }
        }

        if (selectedIndex != -1) {
	    // Get the View Config flag to determine if the View Configuration
            // Button should be enabled or not
	    var hiddenFieldValue = document.ServerSelectionForm.elements[
                prefixTiled + selectedIndex + "].HiddenInfo"].value;
            var myArray = hiddenFieldValue.split("###");

            // myArray[1] contains ServerUtil.ALARM_???
	    if (myArray[1] <= 3) {
                viewDisabled = false;
	    }
        }

        // Toggle action buttons disable state
        ccSetButtonDisabled(
            ButtonRemove, "ServerSelectionForm", disabled);
        ccSetButtonDisabled(
            ButtonViewConfig, "ServerSelectionForm", viewDisabled);
    }

    /**
     * Retrieve the alarm type if the server
     * If alarm type is 4-6, (Defined in ServerUtil.java)
     * ALARM_DOWN                  = 4
     * ALARM_ACCESS_DENIED         = 5
     * ALARM_NOT_SUPPORTED         = 6
     *
     * Quit the javascript function.  Let the server side routine to display
     * the error messages.
     */
    function getAlarmType(tmpStr) {
        var start     = tmpStr.indexOf("[") + 1;
        var end       = tmpStr.indexOf("]");
        var tileIndex = parseInt(tmpStr.substring(start, end));

        // get the hidden info
        var form = document.ServerSelectionForm;
        var hidden = form.elements[
            "ServerSelection.ServerSelectionView.ServerSelectionTiledView["
            + tileIndex + "].HiddenInfo"].value;
        var tmpArray = hidden.split("###");
        return tmpArray[1];
    }

    /**
     * Retrieve the server name
     */
    function getServerString(tmpStr) {
        var start = tmpStr.indexOf("=") + 1;
        var end   = tmpStr.indexOf("###");
        return (tmpStr.substring(start, end));
    }

    /**
     * Create the URL that set the Content frame to what user desires.  The
     * URL should also contains the server name as a part of the request.
     */
    function createURL(field, whereToGo) {
        // convert href object to a string
        var tmpStr = field + "";
        var alarmType = getAlarmType(unescape(tmpStr));
        if (parseInt(alarmType) >= 4) {
            return true;
        }
        // grep the server name
        var servername = getServerString(unescape(tmpStr));

        // create URL and forward the page
        var url = "/samqfsui/util/FrameFormat?SERVER_NAME=" + servername +
                  "&DESTINATION=" + whereToGo;
        window.location.href = url;
        return false;
    }
