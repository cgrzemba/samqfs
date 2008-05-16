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

// ident    $Id: FSDumpNowPopUp.js,v 1.4 2008/05/16 19:39:13 am143972 Exp $

/** 
 * This is the javascript file for Create Recovery Point Now page
 */

 function validate() {
        var tf   = document.FSDumpNowForm;
        var path  = trim(tf.elements["FSDumpNowPopUp.pathChooser.browsetextfield"].value);	

        if (isEmpty(path)) {
            alert(getErrorString(1));
            return false;
        }
        
        if (!checkPath(path)) {
            alert(getErrorString(2));
            return false;
        }
        
        return true;
    }

    function getErrorString(index) {
        var tf   = document.FSDumpNowForm;
        var str1 = tf.elements["FSDumpNow.LocationEmptyError"].value;
        var str2 = tf.elements["FSDumpNow.InvalidPathError"].value;

        switch (index) {
            case 1:
                return tf.elements["FSDumpNow.LocationEmptyError"].value;
            case 2:
                return tf.elements["FSDumpNow.InvalidPathError"].value;
            default:
                return "";
        }
    }

    function checkPath(path) {
        // Check that path does not include any bad chars.
        // Need to allow for colons, so I can't use isValidLogFile 
        // function.
        var badChars = new Array(20);
        badChars[0] = "*";
        badChars[1] = "?";
        badChars[2] = "\\";
        badChars[3] = "!";	
        badChars[4] = "&";
        badChars[5] = "%";
        badChars[6] = "'";
        badChars[7] = "\"";
        badChars[8] = "(";
        badChars[9] = ")";
        badChars[10] = "+";
        badChars[11] = ",";
        badChars[12] = ";";
        badChars[13] = "<";
        badChars[14] = ">";
        badChars[15] = "@";
        badChars[16] = "#";
        badChars[17] = "$";
        badChars[18] = "=";
        badChars[19] = "^";    

        if (!isValidString(path, badChars)) {
            return false;
        }

        return true;

    }
        
    function getFileChooserParams() {
        // get client side params
        return null;
    }
