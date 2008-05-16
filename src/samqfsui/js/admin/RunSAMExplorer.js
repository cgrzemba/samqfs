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

// ident	$Id: RunSAMExplorer.js,v 1.6 2008/05/16 19:39:12 am143972 Exp $

/** 
 * This is the javascript file of Generate SAM Explorer Pop Up
 */

    function getErrorMessage() {
        return document.RunSAMExplorerForm.elements[
            "RunSAMExplorer.ErrorMessage"].value;
    }

    function getConfirmMessage() {
        return document.RunSAMExplorerForm.elements[
            "RunSAMExplorer.ConfirmMessage"].value;
    }

    function showConfirmMessage() {
        return window.confirm(getConfirmMessage());
    }
    
    function validate() {
        var value = trim(document.RunSAMExplorerForm.elements[
            "RunSAMExplorer.TextField"].value);
        if (isValidNum(value, "", "1", "", "50000", "") != 1) {
            alert(getErrorMessage());
            return false;
        } else {
            return true;
        }
    }

