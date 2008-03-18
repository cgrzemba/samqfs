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

// ident	$Id: Recycler.js,v 1.6 2008/03/17 14:40:25 am143972 Exp $

/** 
 * This is the javascript file of Recycler Page
 */

    function validate() {
        var tf = document.RecyclerForm;
        var libnumber = tf.elements["Recycler.libNumber"].value;
        for (var j = 0; j < libnumber; j++) {
                          
            var hwmValue = trim(tf.elements
                ["Recycler.RecyclerTableTiledView[" + 
                j + "].hwm"].value);
            var minigainValue = trim(tf.elements
                ["Recycler.RecyclerTableTiledView[" + 
                j + "].minigain"].value);
            var vsnValue = trim(tf.elements
                ["Recycler.RecyclerTableTiledView[" + 
                j + "].vsnlimit"].value);
            var sizeValue = trim(tf.elements
                ["Recycler.RecyclerTableTiledView[" + 
                j + "].sizelimit"].value);
            var sizeUnitValue = trim(tf.elements
                ["Recycler.RecyclerTableTiledView[" + 
                j + "].sizeunit"].value);
            
            if (!isEmpty(hwmValue)) {
                if (!isValidNum(hwmValue, "", "0", "", "100", "")) {
                    alert(getErrorString(2));
                    return false;
                }
            }
            if (!isEmpty(minigainValue)) {
                if (!isValidNum(minigainValue, "", "0", "", "100", "")) {
                    alert(getErrorString(4));
                    return false;
                }
            }
	    if (parseInt(minigainValue) > parseInt(hwmValue)) {
	        alert(getErrorString(13));
		return false;
	    }
            if (!isEmpty(vsnValue)) {
                if (!isPositiveInteger(vsnValue)) {
                    alert(getErrorString(6));
                    return false;
                }
            } 
            if (!isEmpty(sizeValue)) {
                if (sizeUnitValue == "dash") {
                    alert(getErrorString(11));
                    return false;
                } else {
                    if (!isPositiveInteger(sizeValue) ||
                        !isNumInRange (sizeValue, sizeUnitValue, "0", "b", 
                        "8000", "pb" )) {
                        alert(getErrorString(8));
                        return false;
                    }
                }
            } else {
                if (sizeUnitValue != "dash") {
                    alert(getErrorString(12));
                    return false;
                }
            }
        }
        
        var recyclerlog =
            trim(tf.elements["Recycler.recyclerlogValue"].value);
        if (!isEmpty(recyclerlog)) {
            if (isValidString(recyclerlog, "")) {
                if (!isValidLogFile(recyclerlog)) {
                    alert(getErrorString(10));
                    return false;
                }
            } else {
                alert(getErrorString(10));
                return false;
            }
        }
        
        
        return true;
    }

    function getErrorString(index, key) {
        index = index + 0; // convert index to integer
        var str =
            document.RecyclerForm.elements["Recycler.HiddenMessages"].value;
        var myMessageArray = str.split("###");
        
        if (index > 0 && index < 14) {
            return myMessageArray[index - 1];
        } else {
            return "";
        }
    }

