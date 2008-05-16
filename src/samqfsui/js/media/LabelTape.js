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

// ident    $Id: LabelTape.js,v 1.8 2008/05/16 19:39:14 am143972 Exp $

/** 
 * This is the javascript file of Label Tape Pop Up Page
 */

    function getErrorString(index) {
        index = index + 0; // convert to integer
        var str =
            document.LabelTapeForm.elements["LabelTape.HiddenMessages"].value;

        var myMessageArray = str.split("###");
        if (index > 0 && index < 4) {
            return myMessageArray[index - 1];
        } else {
            return "";
        }
    }

    function render() {
        if (!confirm(getErrorString(3))) {
            return false;
        } else {
            return true;
        }
    }


    function validate() {
        var tf   = document.LabelTapeForm;
        var labelObj  = tf.elements["LabelTape.nameValue"];
        var labelName = trim(labelObj.value);

        // check to see if labelName is empty
        if (isEmpty(labelName)) {
            alert(getErrorString(2));
            labelObj.focus();
            return false;

        } else {
            // if not empty, check validity of string
            if (!isValidVSNString(labelName)) {
                alert(getErrorString(1));
                labelObj.focus();
                return false;
            }
        }

        return true;
    }
