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

// ident	$Id: CommonPopup.js,v 1.11 2008/12/16 00:10:35 am143972 Exp $

/** 
 * Call this method from your javascript for your popup.
 */

    function showPopup(pagePath,
                       windowName,
                       windowWidth,
                       windowHeight,
                       parentFormName, 
                       parentReturnValueObjName, 
                       parentSubmitCmd,
                       pageTitleText, 
                       promptText,
                       loadValue) {
        var url = escape(pagePath) + "?parentFormName=" + makeURLSafe(parentFormName) +
                  "&parentReturnValueObjName=" + makeURLSafe(parentReturnValueObjName) +
                  "&parentSubmitCmd=" + makeURLSafe(parentSubmitCmd) +
                  "&pageTitleText=" + makeURLSafe(pageTitleText) +
                  "&promptText=" + makeURLSafe(promptText) +
                  "&loadValue=" + makeURLSafe(loadValue) +
                  "&com_sun_web_ui_popup=true";
        var win = window.open(url, 
                    windowName, 
                    "width=" + windowWidth + ",height=" + windowHeight + ",resizable,scrollbars");
        
        if (win != null) {
            win.focus();
        }

        return false;         
    }

/** Call this method for a simple way to close the popup and store a single string return value
 * in the parent.
 */
    function doSubmit(viewName, returnValue) {
        // Get forms
        var thisFormObj   = document.forms[0];
        var parentFormName = thisFormObj.elements[viewName + ".parentFormName"].value;
        var parentFormObj   = window.opener.document.forms[parentFormName];

        // Save return value in parent form.  
        var parentReturnObjName = thisFormObj.elements[viewName + ".parentReturnValueObjName"].value;
        var parentReturnObj = parentFormObj.elements[parentReturnObjName];
        parentReturnObj.value = returnValue;

        // Refresh parent page.
        var submitCmd = thisFormObj.elements[viewName + ".parentSubmitCmd"].value;
        parentFormObj.action = parentFormObj.action + "?" + submitCmd;
        parentFormObj.submit();
    } 
