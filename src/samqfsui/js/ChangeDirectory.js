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

// ident	$Id: ChangeDirectory.js,v 1.7 2008/12/16 00:10:35 am143972 Exp $

/** 
 * Used by the caller of the Change Directory popup window.
 * Include this script in your jsp file and call this method to 
 * display the popup window.  Be sure to return the return value of this method.
 * Also, include /samqfsui/util/CommonPopup.js in your jsp file.
 */

    function showChangeDirPopup(parentFormName, 
                                parentReturnValueObjName, 
                                parentSubmitCmd,
                                pageTitleText, 
                                promptText,
                                loadValue) {
                 
        return showPopup("../util/ChangeDirectory",
                         "changeDirectory",
                         600,
                         400,
                         parentFormName,
                         parentReturnValueObjName,
                         parentSubmitCmd,
                         pageTitleText,
                         promptText,
                         loadValue);

    }
