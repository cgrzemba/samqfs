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

// ident	$Id: AddServer.js,v 1.10 2008/03/17 14:40:28 am143972 Exp $

/** 
 * This is the javascript file of Add Server Page
 */

    function getErrorString(index) {
        index = index + 0; // convert index to integer
        var tf = document.AddServerForm;
        var str = tf.elements["AddServer.HiddenMessages"].value;
        var myMessageArray = str.split("###");
        if (index > 0 && index < 5) {
            return myMessageArray[index - 1];
        } else {
            return "";
        }
    }

    function isAlphaNumericDashUnderscorePeriod(c) {
        return (isDigit(c) || (c == "-") || (c == "_") || (c == ".") ||
            ((c >= "a") && (c <= "z")) ||
            ((c >= "A") && (c <= "Z")));
    }

    function isValidServerString(myString) {
        for (var i = 0; i < myString.length; i++) {
           var c = myString.charAt(i);
           if (!isAlphaNumericDashUnderscorePeriod(c)) return false;
        }
        return true;
    }

    function validate() {
        var tf   = document.AddServerForm;
        var field = trim(tf.elements["AddServer.nameValue"].value);

        if (isEmpty(field)) {
            alert(getErrorString(1));
            return false;
        } else if (!isValidString(field, "")) {
            alert(getErrorString(2));
            return false;
        } else if (!isValidServerString(field)) {
            alert(getErrorString(3));
            return false;
	}
        return true;
    }

    function isClusterPagelet() {
        var item = document.AddServerForm.elements[
            "AddServer.AddClusterView.NodeNames"];
        return (item != null);
    }

    /* 
     * handler function for : 
     * Server Selection Page -> Add -> Submit 
     */ 
    function preSubmitHandler(field) {
        var viewName   = "AddServer.AddClusterView";
        var prefix     = viewName + ".AddClusterTable.SelectionCheckbox"; 
        var theForm    = field.form; 
        var node_names =  
            theForm.elements[viewName + ".NodeNames"].value.split(";"); 

        // there is actually n-1 node names since there is a trailing ';' 
        node_count = node_names.length - 1; 

        var selected_node_names = ""; 
        // loop through the check boxes and determine which ones are selected 
        for (var i = 0; i < node_count; i++) { 
            var check_box_name = prefix + i; 
            var theCheckBox = theForm.elements[check_box_name]; 

            if (theCheckBox.checked) {
                if (selected_node_names != "") {
                    selected_node_names += ", ";
                }
                selected_node_names += node_names[i]; 
            } 
        } 

        if (selected_node_names.length == 0) { 
            alert(getErrorString(4)); 
            return false; 
        } else { 
            theForm.elements["AddServer.SelectedNodes"].value
                = selected_node_names; 
            return true; 
        } 
    } 
