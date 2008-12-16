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

// ident	$Id: ServerConfiguration.js,v 1.10 2008/12/16 00:10:35 am143972 Exp $

/** 
 * This is the javascript file of Server Configuration Page
 */

    function getPathFromHref(field) {
        var myForm        = document.ServerConfigurationForm;
        var firstArray = field.href.split("PathHref=");
        var secondArray = firstArray[1].split("&jato");
       
        // return the path name
        return secondArray[0];
    }
    
    function getFileNameFromHref(field, showContent) {
        var myForm        = document.ServerConfigurationForm;
        var firstArray;

        if (showContent == 1) {
            firstArray = field.href.split("FileHref=");
        } else if (showContent == 0) {
            firstArray = field.href.split("StatusHref=");
        } else if (showContent == 2) {
            firstArray = field.href.split("ReportPathHref=");
        } else if (showContent == 3) {
            firstArray = field.href.split("ReportsPathHref=");
        }
        var secondArray = firstArray[1].split("&jato");
       
        // return the path name
        return secondArray[0];
    }

    function getServerKey() {
        return document.ServerConfigurationForm.elements[
            "ServerConfiguration.ServerName"].value;
    }
