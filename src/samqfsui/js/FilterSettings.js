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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: FilterSettings.js,v 1.8 2008/12/16 00:10:35 am143972 Exp $

/************************************************************
 * Instructions to the caller of the filter settings popup.
 * 1.  Include this script in your jsp file.
 * 2.  Include /samqfsui/js/samqfsui.js in your jsp file as FilterSettings.js
 *     uses stuff in samqfsui.js
 * 3.  Include /samqfsui/js/CommonPopup.js in your jsp file. 
 * 4.  Call showFilterSettingsPopup from the onClick event handler
 *     of some object on your page, and return this method's return value.
 ************************************************************/ 
 
    function showFilterSettingsPopup(parentFormName, 
                                     parentReturnValueObjName, 
                                     parentSubmitCmd,
                                     pageTitleText, 
                                     promptText,
                                     loadValue) {
        return showPopup("../util/FilterSettings",
                         "filterSettings",
                         700,
                         700,
                         parentFormName,
                         parentReturnValueObjName,
                         parentSubmitCmd,
                         pageTitleText,
                         promptText,
                         loadValue);      
    }

