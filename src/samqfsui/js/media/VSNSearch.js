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

// ident	$Id: VSNSearch.js,v 1.4 2008/03/17 14:40:27 am143972 Exp $

/** 
 * This is the javascript file of VSN Search Page
 */

    function getErrorString(index) {
        var errorMessages =
            document.VSNSearchForm.elements["VSNSearch.ErrorMessages"].value;
        var errArray = errorMessages.split("###");
	switch (index) {
            case 1:
                return errArray[0];
            case 2:
                return errArray[1];
            default:
		return "";
	}
    }

    function validate() {
        var tf   = document.VSNSearchForm;
	var textObj     = tf.elements["VSNSearch.TextField"];
        var textValue   = trim(textObj.value);

	if (isEmpty(textValue)) {
	    alert(getErrorString(1));
	    return false;
	} else if (!isValidVSNString(textValue)) {
	    alert(getErrorString(2));
	    return false;
	} else return true;
    }
