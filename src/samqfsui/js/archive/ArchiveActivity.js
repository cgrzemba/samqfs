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

// ident	$Id: ArchiveActivity.js,v 1.4 2008/05/16 19:39:12 am143972 Exp $

    function checkSelection(field) {
        var tf = document.ArchiveActivityForm;
        var radio = tf.elements["ArchiveActivity.archiveValue"];
        var fsMenu = tf.elements["ArchiveActivity.ArchiveMenu"];

        if ("All" != fsMenu.value) {
            // Restart & Rerun cannot apply to a single file system
            if (radio.length == 5 && (radio[0].checked || radio[3].checked)) {

                var errMsg = tf.elements["ArchiveActivity.ErrorMessage"].value;
                alert(errMsg);
                fsMenu.value = "All";
                return false;
            }
        }
        return true;
    }
