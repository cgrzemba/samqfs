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

// ident	$Id: ArchiveSetUp.js,v 1.10 2008/05/16 19:39:12 am143972 Exp $

function validate() {
    var tf = document.ArchiveSetUpForm;
    var medianumber = tf.elements["ArchiveSetUp.mediaNumber"].value;
    var libnumber = tf.elements["ArchiveSetUp.libNumber"].value;

    for (var i = 0; i < libnumber; i++) {
	    var archiveDriveCount = trim(tf.elements
                ["ArchiveSetUp.ArchiveSetUpView.DriveLimitsTiledView[" +
	        i + "].MaxDrivesForArchive"].value);
            var stageDriveCount = trim(tf.elements
                ["ArchiveSetUp.ArchiveSetUpView.DriveLimitsTiledView[" +
	        i + "].MaxDrivesForStage"].value);
	    var maxDrive = trim(tf.elements
	                ["ArchiveSetUp.ArchiveSetUpView.DriveLimitsTiledView[" +
	                i + "].MaxDrives"].value);
	    var maxDriveErrMsg = trim(tf.elements
	                ["ArchiveSetUp.ArchiveSetUpView.DriveLimitsTiledView[" +
	                i + "].MaxDrivesErrMsg"].value);
	    if (!isEmpty(archiveDriveCount)) {
	        if (!isValidNum(archiveDriveCount, "", "0", "", maxDrive, "")) {
	            alert(maxDriveErrMsg);
	            return false;
	        }
	    }
            if (!isEmpty(stageDriveCount)) {
	        if (!isValidNum(stageDriveCount, "", "0", "", maxDrive, "")) {
                    alert(maxDriveErrMsg);
	            return false;
	        }
	    }
    }

    for (var j = 0; j < medianumber; j++) {
        var buffer = trim(tf.elements["ArchiveSetUp.ArchiveSetUpView.MediaParametersTiledView[" + j + "].ArchiveBufferSize"].value);
        var maxarchive = trim(tf.elements["ArchiveSetUp.ArchiveSetUpView.MediaParametersTiledView[" + j + "].MaxArchiveFileSize"].value);
        var minfs = trim(tf.elements["ArchiveSetUp.ArchiveSetUpView.MediaParametersTiledView[" + j + "].MinSizeForOverflow"].value);
        if (!isEmpty(buffer)) {
            if (!isValidNum(buffer, "", "2", "", "1024", "")) {
                alert(getErrorString(4));
                return false;
            }
        }

        if (!isEmpty(maxarchive)) {
            if (!isPositiveInteger(maxarchive)) {
                alert(getErrorString(6));
                return false;
            }
        }

        if (!isEmpty(minfs)) {
            if (!isPositiveInteger(minfs)) {
                alert(getErrorString(8));
                return false;
            }
        }
        // now for stage parameters
        var buffer = trim(tf.elements["ArchiveSetUp.ArchiveSetUpView.MediaParametersTiledView[" + j + "].StageBufferSize"].value);
        if (!isEmpty(buffer)) {
            if (!isValidNum(buffer, "", "2", "", "1024", "")) {
                alert(getErrorString(24));
                return false;
            }
        }
    }
    
    var archiveLog = trim(tf.elements["ArchiveSetUp.ArchiveLogFile"].value);
    if (!isEmpty(archiveLog)) {
        if (isValidString(archiveLog, "")) {
            if (!isValidLogFile(archiveLog)) {
                alert(getErrorString(10));
                return false;
            }
        } else {
            alert(getErrorString(10));
            return false;
        }
    }

    var stageLog = trim(tf.elements["ArchiveSetUp.StageLogFile"].value);
    if (!isEmpty(stageLog)) {
        if (isValidString(stageLog, "")) {
            if (!isValidLogFile(stageLog)) {
                alert(getErrorString(22));
                return false;
            }
        } else {
            alert(getErrorString(22));
            return false;
        }
    }
        
    var releaseLog = trim(tf.elements["ArchiveSetUp.ReleaseLogFile"].value);
    if (!isEmpty(releaseLog)) {
        if (isValidString(releaseLog, "")) {
            if (!isValidLogFile(releaseLog)) {
                alert(getErrorString(23));
                return false;
            }
        } else {
            alert(getErrorString(23));
            return false;
        }
    }

    var interval = trim(tf.elements["ArchiveSetUp.ArchiveInterval"].value);
    var intervalDropDown = 
                tf.elements["ArchiveSetUp.ArchiveIntervalUnits"].value;
        if (!isEmpty(interval)) {
            // If expressed in seconds, maximum is 99999999
            // If expressed in minutes, maximum is 1666666
            // If expressed in hours, maximum is 27777
            // If expressed in days, maximum is 1157
            // If expressed in weeks, maximum is 165
            if (intervalDropDown == 5 && (!isInteger(interval) ||
                ((parseInt(interval) <= 0) || (parseInt(interval) > 99999999)))) {
                alert(getErrorString(12));
                return false;
            } else if (intervalDropDown == 6 && (!isInteger(interval) ||
                ((parseInt(interval) <= 0) || (parseInt(interval) > 1666666)))) {
                alert(getErrorString(13));
                return false;
            } else if (intervalDropDown == 7 && (!isInteger(interval) ||
                ((parseInt(interval) <= 0) || (parseInt(interval) > 27777)))) {
                alert(getErrorString(14));
                return false;
            } else if (intervalDropDown == 8 && (!isInteger(interval) ||
                ((parseInt(interval) <= 0) || (parseInt(interval) > 1157)))) {
                alert(getErrorString(15));
                return false;  
            } else if (intervalDropDown == 9 && (!isInteger(interval) ||
                ((parseInt(interval) <= 0) || (parseInt(interval) > 165)))) {
                alert(getErrorString(16));
                return false;
            }
        }
        // empty fields with valid unit are acceptable
        // it just means reset
    

    var ageString = trim(tf.elements["ArchiveSetUp.ReleaseAge"].value);
        var ageDropDown = tf.elements["ArchiveSetUp.ReleaseAgeUnits"].value;
        if (!isEmpty(ageString)) {
            // If expressed in seconds, maximum is 2147483645
            // If expressed in minutes, maximum is 35791394
            // If expressed in hours, maximum is 596523
            // If expressed in days, maximum is 24855
            // If expressed in weeks, maximum is 3550
            if (ageDropDown == 5 && (!isInteger(ageString) ||
                ((parseInt(ageString) <= 0) || (parseInt(ageString) > 2147483645)))) {
                alert(getErrorString(17));
                return false;
            } else if (ageDropDown == 6 && (!isInteger(ageString) ||
                ((parseInt(ageString) <= 0) || (parseInt(ageString) > 35791394)))) {
                alert(getErrorString(18));
                return false;
            } else if (ageDropDown == 7 && (!isInteger(ageString) ||
                ((parseInt(ageString) <= 0) || (parseInt(ageString) > 596523)))) {
                alert(getErrorString(19));
                return false;
            } else if (ageDropDown == 8 && (!isInteger(ageString) ||
                ((parseInt(ageString) <= 0) || (parseInt(ageString) > 24855)))) {
                alert(getErrorString(20));
                return false;  
            } else if (ageDropDown == 9 && (!isInteger(ageString) ||
                ((parseInt(ageString) <= 0) || (parseInt(ageString) > 3550)))) {
                alert(getErrorString(21));
                return false;
            }
        }
        // empty fields with valid unit are acceptable
        // it just means reset
    
    return true;
}

function getErrorString(index) {
    var tf = document.ArchiveSetUpForm;
    index = index + 0; // convert index to integer
    var str = tf.elements["ArchiveSetUp.HiddenMessages"].value;
    var myMessageArray = str.split("###");
        
    if (index > 0 && index <= 24) {
        return myMessageArray[index - 1];
    } else {
        return "";
    }
}

