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

// ident	$Id: AdvancedNetworkConfigSetup.js,v 1.6 2008/12/16 00:10:37 am143972 Exp $

/** 
 * This is the javascript file of Advanced Network Config Setup page
 */

    function preSubmitHandler(field) {
        var viewName         =
            "AdvancedNetworkConfigSetup.AdvancedNetworkConfigSetupView";
        var prefix         = viewName + ".AdvancedNetworkConfigSetupTiledView[";
        var suffix_primary   = "].PrimaryIPDropDown";
        var suffix_secondary = "].SecondaryIPDropDown";
        
        var theForm     = field.form; 
        var NumberOfMDS =  
            theForm.elements[viewName + ".NumberOfMDS"].value; 

        var selection = ""; 

        // loop through the tiled view and gather IP Addresses 
        for (var i = 0; i < NumberOfMDS; i++) { 
            var thePrimaryDropDown =
                theForm.elements[prefix + i + suffix_primary];
            var theSecondaryDropDown =
                theForm.elements[prefix + i + suffix_secondary];

            if (selection != "") {
                selection += ",";
            }
            selection +=
                thePrimaryDropDown.value + " " + theSecondaryDropDown.value;
        }

        theForm.elements[viewName + ".IPAddresses"].value = selection;
    }
