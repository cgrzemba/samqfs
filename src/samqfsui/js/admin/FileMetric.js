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

// ident	$Id: FileMetric.js,v 1.5 2008/05/16 19:39:12 am143972 Exp $

/**
 * This is the javascript file for File System Metric
 */
    function getPathFromHref(field) {
        var myForm        = document.FileMetricSummaryForm;
        var firstArray = field.href.split("PathHref=");
        var secondArray = firstArray[1].split("&jato");
       
        // return the path name
        return secondArray[0];
    }
    
    function getServerKey() {
        return document.FileMetricSummaryForm.elements[
            "FileMetricSummary.ServerName"].value;
    }
    
    function getFsName() {
        return document.FileMetricSummaryForm.elements[
            "FileMetricSummary.fsnameOptions"].value;
    }

    function details(numFiles, numOnline) {
        var textArea = document.FileMetricSummaryForm.elements["FileMetricSummary.TextArea"];
        // set the textArea text to txt
        // set textArea to visible
        textArea.style.visibility="visible";
        textArea.style.bgcolor="#BEC7CC";
        textArea.value = 'No. of files: '.concat(numFiles).concat('\nNo.of bytes online: ').concat(numOnline);
        
    }
    
    function hideData() {
        var textArea = document.FileMetricSummaryForm.elements["FileMetricSummary.TextArea"];
        textArea.style.visibility="hidden";
    }

    function toggleDisabledState(field) {      
        var disabled = true;
        var delButton = "FileMetricSummary.DeleteButton";

        var myForm = document.FileMetricSummaryForm;

        var selectionName = "FileMetricSummary.fileMetricTable2.Selection";

        // checkbox or radioButton for row selection
        if (field != null) {
            selectedIndex     = field.value;
            var elementName   = field.name;
            if (field.type == "radio" && field.checked) {
                disabled = false;
            }
        }

        // Toggle delete button disable state
        ccSetButtonDisabled(delButton, "FileMetricSummaryForm", disabled);
    }

