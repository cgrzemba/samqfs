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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: ShowLog.js,v 1.8 2008/12/16 00:10:36 am143972 Exp $

/** 
 * This is the javascript file of Show Log Pop-Up
 */

    function getErrorMessage() {
        return document.ShowLogForm.elements["ShowLog.ErrorMessage"].value;
    }
    
    function validate() {
        var value = document.ShowLogForm.elements["ShowLog.TextField"].value;
        if (isValidNum(value, "", "10", "", "50000", "") != 1) {
            alert(getErrorMessage());
            return false;
        } else {
            return true;
        }
    }

    function refreshPage() {
        var tf   = document.ShowLogForm;
        var commandChildName = "ShowLog.RefreshButton";
        var autoRefreshHiddenField =
            document.ShowLogForm.elements["ShowLog.AutoRefreshRate"];

        autoRefreshHiddenField.value =
            document.ShowLogForm.elements["ShowLog.DropDownMenu"].value;

        // Set Form action URL and submit
        tf.action = tf.action + "?" + "com_sun_web_ui_popup=true&" +
                commandChildName +
                "=&jato.pageSession=" + 
        tf.elements["jato.pageSession"].value;
        tf.submit();
    }

    function getAutoRefreshRate() {
        var rate = document.ShowLogForm.
            elements["ShowLog.AutoRefreshRate"].value;
        if (rate == "") {
            return 9999000;
        } else {
            return parseInt(rate);
        }
    }

    function callEveryTime() {
        setInterval(refreshPage, getAutoRefreshRate());

        var myTextArea = document.ShowLogForm.elements["ShowLog.TextArea"];

        // preset focus to the bottom of the text area
        myTextArea.scrollTop = myTextArea.scrollHeight;
    }

