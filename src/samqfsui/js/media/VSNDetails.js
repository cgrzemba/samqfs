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

// ident	$Id: VSNDetails.js,v 1.11 2008/05/16 19:39:14 am143972 Exp $

/** 
 * This is the javascript file of VSN Details Page
 */

    /**
     * To return a boolean on the permission the user has.
     * flag == 0 (return true if user has CONFIG permission)
     * flag == 1 (return true if user has MEDIA permisson)
     */
    function hasPermission(flag) {
        var role = document.VSNDetailsForm.
            elements["VSNDetails.Role"].value;
        switch (flag) {
            case 0:
                return ("CONFIG" == role);
            case 1:
                return ("MEDIA"  == role);
            default:
                return false;
        }
    }

    function disableNonApplicableItems() {
        var configRole = hasPermission(0);
        var mediaRole  = hasPermission(1);
        var actionMenu = "VSNDetails.PageActionMenu";
        var libName =
            document.VSNDetailsForm.elements[
                "VSNDetails.LibraryNameHiddenField"].value;

        // disable LOAD operation if the VSN is already in drive
        var isVSNLoaded = document.VSNDetailsForm.elements[
            "VSNDetails.isVSNInDrive"].value;

        ccSetDropDownMenuOptionDisabled(
            actionMenu, "VSNDetailsForm",
            libName == "Historian" || !configRole, 1);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, "VSNDetailsForm",
            libName == "Historian" || isVSNLoaded == "true" || !configRole, 2);
        ccSetDropDownMenuOptionDisabled(
            actionMenu, "VSNDetailsForm", !(mediaRole || configRole), 3);
    }
    
    function getSlotNumber() {
        var number =
            document.VSNDetailsForm.elements["VSNDetails.slotNumber"].value;
        return number;
    }
    
    function getLibraryName() {
        var libName =
            document.VSNDetailsForm.elements[
                "VSNDetails.LibraryNameHiddenField"].value;
        return libName;
    }
    
    function showConfirmMsg(choice) {
        var str =
            document.VSNDetailsForm.elements["VSNDetails.HiddenMessage"].value;
        var arr = str.split("###");
        if (!confirm(arr[choice])) {
            return false;
        } else {
            return true;
        }
    }

    function getServerKey() {
        return (document.VSNDetailsForm.elements[
            "VSNDetails.ServerNameHiddenField"].value);
    }
    
    function getClientParams() {
        var param = null;
        var serverName = getServerKey();
        var libraryName = getLibraryName();

        param = "serverNameParam=" + serverName;
         var slotNumber = getSlotNumber();
            param += "&libraryNameParam=" + libraryName + "&slotNumberParam="
                + slotNumber;
        return param;
    }

    function resetDropDownMenu(dropdown) {
        if (dropdown != null) {
            dropdown.options[0].selected = true;
        }
    }

    function getDropDownSelectedItem(dropdown) {
        var item = -1;
        if (dropdown != null) {
            item = dropdown.value;
        }
        return item;
    }
