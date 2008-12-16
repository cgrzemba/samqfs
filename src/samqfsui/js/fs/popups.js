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

// ident	$Id: popups.js,v 1.12 2008/12/16 00:10:37 am143972 Exp $

/*
 * launch the 'check fs' popup. This function is called from both the File
 * System summary page and the File System details page.
 */
function launchCheckFSPopup(field, csparams) {
    var params = "&" + csparams + "&" + "parentForm=" + field.form.name;

    return launchPopup("/fs/FSSamfsck", 
                       "check_fs_popup", 
                       getServerKey(), 
                       null, 
                       params);
}

/*
 * launch the 'snapshot now' popup. This function is called from the Recovery
 * points page (js/fs/RecoveryPoints.js).
 */
function launchDumpNowPopup(field, csparams) {
    var params = "&" + csparams + "&" + "parentForm=" + field.form.name;

    return launchPopup("/fs/FSDumpNowPopUp", 
                       "dump_now_popup", 
                       getServerKey(), 
                       null, 
                       "&amp;"+params);
}

function launchAddCriteriaToFSPopup(field, csparams) {
}

function launchReorderCriteriaPopup(field, csparams) {
}
