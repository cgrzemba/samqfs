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

// ident	$Id: FsReport.js,v 1.3 2008/03/17 14:40:24 am143972 Exp $

/**
 * This is the javascript file for File System Metric
 */
    function getPathFromHref(field) {
        var myForm        = document.FsReportForm;
        var firstArray = field.href.split("PathHref=");
        var secondArray = firstArray[1].split("&jato");
       
        // return the path name
        return secondArray[0];
    }
    
    function getServerKey() {
        return document.FsReportForm.elements[
            "FsReport.ServerName"].value;
    }
    
    function toggleDisabledState(field) {      
        var disabled = true;
        var delButton = "FsReport.FsReportView.DeleteButton";

        var myForm = document.FsReportForm;

        var selectionName = "FsReport.FsReportView.reportsSummaryTable.Selection";

        // checkbox or radioButton for row selection
        if (field != null) {
            selectedIndex     = field.value;
            var elementName   = field.name;
            if (field.type == "radio" && field.checked) {
                disabled = false;
            }
        }

        // Toggle delete button disable state
        ccSetButtonDisabled(delButton, "FsReportForm", disabled);
    }