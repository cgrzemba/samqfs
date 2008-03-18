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

// ident	$Id: AdvancedNetworkConfig.js,v 1.6 2008/03/17 14:40:26 am143972 Exp $

/** 
 * This is the javascript file of Advanced Network Config page
 */

    // return current server that are being managed
    function getServerKey() {
        return document.AdvancedNetworkConfigForm.elements[
            "AdvancedNetworkConfig.ServerName"].value;
    }

    function launchConfigWindow() {
        var clientParams =
            "&SAMQFS_FILE_SYSTEM_NAME=" + getFSName() +
            "&SELECTED_HOSTS="   + getSelectedHosts() +
            "&SHARED_MD_SERVER=" + getMDServerName() +
            "&SHARED_MDS_LIST="  + getMDSNames();
        var win =  launchPopup("/fs/AdvancedNetworkConfigSetup", 
                               "fsadvnwkcfgsetup",
                               getServerKey(),
                               SIZE_NORMAL,
                               clientParams);
        win.focus();
        return false;
    }

    function getSelectedHosts() {
        var viewName   =
            "AdvancedNetworkConfig.AdvancedNetworkConfigDisplayView";
        var prefix     = viewName +
            ".AdvancedNetworkConfigDisplayTable.SelectionCheckbox"; 
        var theForm    = document.AdvancedNetworkConfigForm; 
        var host_names =  
            theForm.elements[viewName + ".AllSharedHostNames"].value.split(","); 

        var selected_host_names = ""; 
        // loop through the check boxes and determine which ones are selected 
        for (var i = 0; i < host_names.length; i++) { 
            var check_box_name = prefix + i; 
            var theCheckBox = theForm.elements[check_box_name]; 

            if (theCheckBox.checked) {
                if (selected_host_names != "") {
                    selected_host_names += ",";
                }
                selected_host_names += host_names[i]; 
            } 
        }

        return selected_host_names;
    }

    function getMDServerName() {
       return document.AdvancedNetworkConfigForm.elements[
            "AdvancedNetworkConfig.AdvancedNetworkConfigDisplayView.MDServerName"].
            value;
    } 

    function getMDSNames() {
        return document.AdvancedNetworkConfigForm.elements[
            "AdvancedNetworkConfig.AdvancedNetworkConfigDisplayView.MDSNames"].
            value;
    }

    function getFSName() {
        return document.AdvancedNetworkConfigForm.elements[
            "AdvancedNetworkConfig.AdvancedNetworkConfigDisplayView.FSName"].
            value;
    }

    function toggleDisabledState(field) {
        var disabled = true;
        var prefix = "AdvancedNetworkConfig.AdvancedNetworkConfigDisplayView.";
        var actionButton = prefix + "ModifyButton";

        if (field != null) {
            var elementName = field.name;
            if (elementName ==
                (prefix + "AdvancedNetworkConfigDisplayTable.SelectAllHref")) {
                disabled = false;
            } else if (elementName ==
                (prefix + "AdvancedNetworkConfigDisplayTableDeselectAllHref")) {
                disabled = true;
            } else if (field.type == "checkbox" && field.checked) {
                disabled = false;
            } else if (field.type == "checkbox" && !field.checked){
                // This case is executed when user clicks on one of the
                // checkbox and deselect that check box.  The buttons cannot
                // be disabled unless there's no other checkboxes that are
                // checked.
                var selection =
                    prefix + "AdvancedNetworkConfigDisplayTable.Selection";
                var myForm = field.form;

                for (i = 0; i < myForm.elements.length; i++) {
                    var e = myForm.elements[i];
                    if (e.name.indexOf(selection) != -1) {
                        // check if "jato_boolean" is a part of the string
                        // skip if yes
                        if (e.name.indexOf("jato_boolean") == -1) {
                            if (e.checked) {
                                // set disabled to false and get off the loop
                                disabled = false;
                                break;
                            }
                        }
                    }
                }
            }
        }

        // Toggle action buttons disable state
        ccSetButtonDisabled(
            actionButton, "AdvancedNetworkConfigForm", disabled);
    }
