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

// ident	$Id: AdminNotification.js,v 1.11 2008/03/17 14:40:24 am143972 Exp $

/**
 * This is the javascript file of Admin Notification Page
 */
    function getResourceKey(field) {
        var param = field.href.split("infoHref=");
        var resourceKey = param[1];
        return resourceKey;
    }

    function getServerKey() {
        return document.AdminNotificationForm.elements[
            "AdminNotification.ServerName"].value;
    }
    
    function getServerConfig() {
        return document.AdminNotificationForm.elements[
            "AdminNotification.AdminNotificationView.ServerConfig"].value;
    }

    function validateCheckBoxes() {
        var myForm = document.AdminNotificationForm;

        var menu = document.AdminNotificationForm.elements[
            "AdminNotification.AdminNotificationView.SubscriberList"];
        var email = getDropDownSelectedItem(menu);
        var msg = document.AdminNotificationForm.elements[
            "AdminNotification.Messages"].value;

        var numcheck = 10; // default: 4.6 SamFS setup
        var serverConfig = getServerConfig();
        // values are sent in View
        if (serverConfig == 0) { // QFS 
            // only checkbox 6
            var qCheckbox = "AdminNotification.AdminNotificationView.check6";
            if ((myForm.elements[qCheckbox].checked  == false) &&
                (email != "AddNewSubscriptionEmail")) {
                if (!confirm(msg)) {
                    return false;
                } else {
                    return true;
                }
            } else { return true;}
        } else if (serverConfig == 1) { // 4.5 SamFS setup
            numcheck = 6;
        }

        var prefix = "AdminNotification.AdminNotificationView.check";
        
        var isChk = false;
        for (var i = 1; i <= numcheck; i++) {
            var checkbox_name = prefix + i;
            var checkbox = myForm.elements[checkbox_name];
            
            if (checkbox.checked == true) {
                // checkbox is checked
                isChk = true;
            }
        }
        if ((isChk == false) && (email != "AddNewSubscriptionEmail")) {
            if (!confirm(msg)) {
                return false;
            } else {
                return true;
            }
        }
    }
    
    function setStateCheckBoxes(state) {
        var myForm = document.AdminNotificationForm;
        var prefix = "AdminNotification.AdminNotificationView.check";
        var numcheck = 10; // default: 4.6 SamFS setup
        var serverConfig = getServerConfig();
        // values are sent in View
        if (serverConfig == 1) {  // Rel 4.5 SamFS setup
            numcheck = 6;
        }

        for (var i = 1; i <= numcheck; i++) {
            var checkbox_name = prefix + i;
            var checkbox = myForm.elements[checkbox_name];
            checkbox.checked = state;
        }
        return false;
    }
