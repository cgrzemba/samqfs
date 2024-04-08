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
 * or https://illumos.org/license/CDDL.
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

// ident	$Id: AddSharedMember.js,v 1.17 2008/12/16 00:10:37 am143972 Exp $

/** 
 * This is the javascript file of Add Shared Member Page
 */

    /**
     * This method is called to validate if the mount point is valid or not
     */
    function checkForMountPoint() {
        var mountPoint = trim(document.AddSharedMemberForm.
            elements["AddSharedMember.MountPointValue"].value);
 
        if (isEmpty(mountPoint)) return false;
        else if (!isValidLogFile(mountPoint)) return false;
        else {
            var character = mountPoint.charAt(0);
            if (character != "/") return false;
            else return true;
        }
    }

    /**
     * This method is called to validate if the
     * secondary IP is the same as the primary IP
     */
    function checkForSecondaryIP() {
        var primaryIPStr   = getIPDropDown(1).value;
        var secondaryIPStr = getIPDropDown(2).value;

        return (primaryIPStr != secondaryIPStr)
    }

    /**
     * This method is called to validate if user selects a host name or not
     */
    function checkForHostName() {
        var hostName = document.AddSharedMemberForm.
            elements["AddSharedMember.HostDropDownValue"].value;
        return (hostName != "");
    }

    function populateHost() {
        resetHostDropDown();
        resetIPDropDown();

        var hostsList = document.AddSharedMemberForm.
            elements["AddSharedMember.HostNames"].value;
        if (hostsList == "") return;

        var hostsArray = hostsList.split("###");

        // Now check if user selects to add shared client or pmds
        // host entry will start with !!! if host has a different architecture
        // than the MDS of this shared-fs.  Skip the entry with !!! if user
        // wants to add PMDS.

        var type = document.AddSharedMemberForm.
            elements["AddSharedMember.MemberType"];
        var isClientSelected = type[0].checked;

        var pmdsCounter = 1;

        // Start with 1, 0 is "--Select a host --"
        for (i = 0; i < hostsArray.length; i++) {
            var tmpArray = hostsArray[i].split("!!!");
            // client
            if (isClientSelected) {
                if (tmpArray.length == 1) {
                    getIPDropDown(0).options[i+1] =
                        new Option(tmpArray[0], tmpArray[0]);
                } else {
                    getIPDropDown(0).options[i+1] =
                        new Option(tmpArray[1], tmpArray[1]);
                }
            // pmds
            } else if (tmpArray[0] != "") {
                getIPDropDown(0).options[pmdsCounter] =
                    new Option(tmpArray[0], tmpArray[0]);
                pmdsCounter++;
            }
        }
    }

    function populateIP(field) {
        var hostName = field.value;

        resetIPDropDown();

	if (hostName == "") return;

        var ipArray = getIPArray(hostName);

        // assign "---" to the secondary IP option field
        getIPDropDown(2).options[0] = new Option("---", "---");
    
        // omit the first element (hostName) in ipArray
        for (i = 1; i < ipArray.length; i++) {
            getIPDropDown(1).options[i-1] = new Option(ipArray[i], ipArray[i]);
            getIPDropDown(2).options[i] = new Option(ipArray[i], ipArray[i]);
        }
    }

    function getIPArray(hostName) {
        var hostEntryArray = getIPAddressString().split("###");

        for (i = 0; i < hostEntryArray.length; i++) {
            var ipArray = hostEntryArray[i].split(",");
            if (hostName == ipArray[0]) {
                return ipArray;
            }
        }
    }

    function resetHostDropDown() {
       for (i = getIPDropDown(0).options.length; i >= 1; i--) {
            getIPDropDown(0).options[i] = null;
       } 
    }

    function resetIPDropDown() {
        for (i = getIPDropDown(1).options.length; i >= 0; i--) {
            getIPDropDown(1).options[i] = null;
        }

        for (i = getIPDropDown(2).options.length; i >= 0; i--) {
            getIPDropDown(2).options[i] = null;
        }
    }

    function getIPAddressString() {
        return document.AddSharedMemberForm.
            elements["AddSharedMember.IPAddresses"].value;
    }


    /**
     * key == 0  (Host Name Drop Down)
     * key == 1  (Primary IP Drop Down)
     * key == 2  (Secondary IP Drop Down)
     */
    function getIPDropDown(key) {
        switch (key) {
            case 0:
                return document.AddSharedMemberForm.
                    elements["AddSharedMember.HostDropDownValue"];
            case 1:
                return document.AddSharedMemberForm.
                    elements["AddSharedMember.PrimaryIPDropDownValue"];
            case 2:
                return document.AddSharedMemberForm.
                    elements["AddSharedMember.SecondaryIPDropDownValue"];
        }
    }

    function validate() {
        var result = checkForHostName();
        if (!result) {
            alert(getErrorMessage(0));
            document.AddSharedMemberForm.
                elements["AddSharedMember.HostDropDownValue"].focus();
            return false;
        }

        result = checkForSecondaryIP();
        if (!result) {
            alert(getErrorMessage(1));
            getIPDropDown(2).focus();
            return false;
        }

        result = checkForMountPoint();
        if (!result) {
            alert(getErrorMessage(2));
            document.AddSharedMemberForm.
                elements["AddSharedMember.MountPointValue"].focus();
            return false;
        }

        return true; 

    }

    function getErrorMessage(option) {
        var messageArray = document.AddSharedMemberForm.
            elements["AddSharedMember.ErrorMessages"].value.split("###");
        return messageArray[option];
    }
