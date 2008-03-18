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

// ident	$Id: SharedFSDetails.js,v 1.8 2008/03/17 14:40:26 am143972 Exp $

/** 
 * This is the javascript file of Shared FS Details Page
 */

    // selected row index
    var modelIndex = -1;

    function getServerKey() {
        return (document.SharedFSDetailsForm.elements[
            "SharedFSDetails.SharedFSDetailsView.HiddenHostName"].value);
    }

    function getFsName() {
        return (document.SharedFSDetailsForm.elements[
            "SharedFSDetails.SharedFSDetailsView.HiddenFsName"].value);
    }

    function isMDSMounted() {
        return (document.SharedFSDetailsForm.elements[
            "SharedFSDetails.SharedFSDetailsView.MDSMounted"].value);
    }

    function toggleDisabledState(field) {
        var disabled      = true;
        var mountdisabled  = true;
        var umountdisabled = true;
        
        var prefix        = "SharedFSDetails.SharedFSDetailsView.";
        var prefixTiled    = prefix + "SharedFSDetailsTiledView[";
        var actionButton1 = prefix + "DeleteButton";
        var actionMenu    = prefix + "ActionMenu";

        var formName = "SharedFSDetailsForm";
        var myForm   = document.SharedFSDetailsForm;

        // checkbox or radioButton for row selection
        if (field != null) {
            // Do not update selectedIndex here .. not until checking if
            // DeselectAllHref is clicked

            var elementName   = field.name;
            if (elementName != (prefix + "SharedFSDetailsTable.DeselectAllHref")
                && field.type == "radio"
                && field.checked) {
                disabled = false;
                modelIndex = field.value;
            } else if (elementName ==
                (prefix + "SharedFSDetailsTable.DeselectAllHref")) {
                // reset if deselect is clicked
                disabled       = true;
                mountdisabled  = true;
                umountdisabled = true;
                modelIndex     = -1;
            }
        }

        var elementName3 = prefixTiled + modelIndex + "].HiddenMount";
        var elementName4 = prefixTiled + modelIndex + "].HiddenType";
        var elementName1 = prefix + "HiddenAllMount";
        var elementName5 = prefix + "HiddenAllClientMount";
        var elementName6 = prefix + "MDSMounted";

        if (modelIndex != -1) {
            var selectedMenu = myForm.elements[elementName3].value;
            var selectedType = myForm.elements[elementName4].value;
            var allMount = myForm.elements[elementName1].value;
            var allClientMount = myForm.elements[elementName5].value;
            var mdMount = myForm.elements[elementName6].value;
            if (selectedType == "Metadata Server") {
                if (selectedMenu == "unmounted") {
                    mountdisabled = false;
                } else if (selectedMenu == "mounted"
                    && allClientMount == "allClientUnMounted") {
                    umountdisabled = false;
                }
            } else {
                if (selectedMenu == "unmounted" && mdMount == "true") {
                    mountdisabled = false;
                } else if (selectedMenu == "mounted") {
                    umountdisabled = false;
                }
            }
        }

        // Toggle action buttons disable state
        ccSetDropDownMenuOptionDisabled(actionMenu, formName, mountdisabled, 2);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, formName, umountdisabled, 3);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, "SharedFSDetailsForm", disabled, 1);
        ccSetButtonDisabled(
            actionButton1, "SharedFSDetailsForm",
            allMount == "false" || disabled);
    }

    function getDropDownSelectedItem(dropdown) {
        var item = -1;
        if (dropdown != null) {
            item = dropdown.value;
        }
        return item;
    }

    function getSelectedFSName() {
        var fsName = null;

        if (modelIndex != -1) {
            var myForm        = document.SharedFSDetailsForm;
            var prefix        = "SharedFSDetails.SharedFSDetailsView.";
            var prefixTiled   = prefix + "SharedFSDetailsTiledView[";

            var fsNameField = prefixTiled + modelIndex + "].FSHiddenField";
            fsName = myForm.elements[fsNameField].value;
        }
        return fsName;
    }

    function getMountPoint() {
        return document.SharedFSDetailsForm.elements[
            "SharedFSDetails.MountPoint"].value;
    }

    function showConfirmMsg(key) {
        var messageArray =
            document.SharedFSDetailsForm.elements[
                "SharedFSDetails.ConfirmMessages"].value.split("###");

        if (key > 2) return false;

        if (!confirm(messageArray[key - 1])) {
            return false;
        } else {
            return true;
        }
    }

    function resetDropDownMenu(dropdown) {
        if (dropdown != null) {
            dropdown.options[0].selected = true;
        }
    }

