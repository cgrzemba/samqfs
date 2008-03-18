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

// ident        $Id: SpecifyVSN.js,v 1.8 2008/03/17 14:40:27 am143972 Exp $

/** 
 * This is the javascript file of Specify VSN Pop Up Page
 */

    function getErrorString(index) {
        index = index + 0; // convert index to integer
        var str =
            document.SpecifyVSNForm.elements["SpecifyVSN.HiddenMessages"].value;
        var myMessageArray = str.split("###");      
        if (index > 0 && index < 8) {
            return myMessageArray[index - 1];
        } else {
            return "";
        }
    }

    function validate() {
        var tf   = document.SpecifyVSNForm;
        var radio = tf.elements["SpecifyVSN.RadioButton1"];
        var startObj   = tf.elements["SpecifyVSN.StartTextField"];
        var startValue = trim(startObj.value);
        var endObj     = tf.elements["SpecifyVSN.EndTextField"];
        var endValue   = trim(endObj.value);
        var oneObj   = tf.elements["SpecifyVSN.OneVSNTextField"];
        var oneValue = trim(oneObj.value);

        if (!radio[0].checked && !radio[1].checked) {
            alert(getErrorString(1));
            return false;
        } else if (radio[0].checked) {
            if (isEmpty(startValue)) {
                alert (getErrorString(2));
                startObj.focus();
                return false;
            } else if (!isValidVSNString(startValue)) {
                alert(getErrorString(5));
                startObj.focus();
                return false;
            }

            if (isEmpty(endValue)) {
                alert (getErrorString(3));
                endObj.focus();
                return false;
            } else if (!isValidVSNString(endValue)) {
                alert(getErrorString(6));
                endObj.focus();
                return false;
            }

        } else if (radio[1].checked) {
            if (isEmpty(oneValue)) {
                alert(getErrorString(4));
                oneObj.focus();
                return false;
            } else if (!isValidVSNString(oneValue)) {
                alert(getErrorString(7));
                oneObj.focus();
                return false;
            }
        }

        return true;
    }

    function setFieldEnable(choice) {
        var formName = "SpecifyVSNForm";
        var prefix = "SpecifyVSN.";
        var isStartEnd = false, isOne = false;
        if (choice == "startend") {
            isStartEnd = true;
        } else {
            isOne = true;
        }

        ccSetTextFieldDisabled(prefix + "StartTextField", formName, !isStartEnd);
        ccSetTextFieldDisabled(prefix + "EndTextField", formName, !isStartEnd);
        ccSetTextFieldDisabled(prefix + "OneVSNTextField", formName, !isOne);
    }
