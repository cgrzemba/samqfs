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

// ident	$Id: ISPolicyWizardConfigureCopy.js,v 1.5 2008/12/16 00:10:37 am143972 Exp $

    var NOVAL = "-999";
    var NOVAL_LABEL = "--";

    function handleExpireCheckBoxClick(field) {
        var prefix = "WizardWindow.Wizard.ISPolicyWizardConfigureCopyView.";
        var exp    = prefix + "ExpirationTimeTextField";
        ccSetTextFieldDisabled(exp, "wizWinForm", field.checked);
        field.form.elements[exp].value = "";
        ccSetDropDownMenuDisabled(
            prefix + "ExpirationTimeDropDown", "wizWinForm", field.checked);
    }
    
    function getContent(key, value) {
        var array = value.split(",");
        return array[key];
    }

    function resetScratchPoolDropDown(dropDown) {
        for (i = dropDown.options.length; i >= 1; i--) {
            dropDown.options[i] = null;
       }
    }

    function populateScratchPoolDropDown() {
        var prefix   =
            "WizardWindow.Wizard.ISPolicyWizardConfigureCopyView.";
        var theForm      = document.wizWinForm;
        var field        = theForm.elements[prefix + "MediaPoolDropDown"];
        var poolType     = parseInt(getContent(1, field.value));
        var poolName     = getContent(0, field.value);
        var scratchMenu = theForm.elements[prefix + "ScratchPoolDropDown"];
        var poolInfo = theForm.elements[prefix + "PoolInfo"].value;
        var poolArray = poolInfo.split("###");
        
        var selectedScratchPool =
            theForm.elements[prefix + "SelectedScratchPool"].value;
      
        resetScratchPoolDropDown(scratchMenu);
        scratchMenu.options[0] = new Option(NOVAL_LABEL, NOVAL);

        for (var i = 0; i < poolArray.length; i++) {
            var eachPoolInfo = poolArray[i].split("&");
            if (poolType == parseInt(eachPoolInfo[2]) &&
                poolName != eachPoolInfo[0]) {
                var currentLength = scratchMenu.length;
                scratchMenu.options[currentLength] =
                    new Option(eachPoolInfo[0] + " (" + eachPoolInfo[1] + ")",
                               eachPoolInfo[0] + "," + eachPoolInfo[2]);
                               
                // set the user selected scratch pool to be selected
                if ((eachPoolInfo[0] + "," + eachPoolInfo[2])
                                            == selectedScratchPool) {
                    scratchMenu.options[currentLength].selected = true;
                }
            }
        }
    }
    
    function wizardPageInit() {      
        populateScratchPoolDropDown();
    }

