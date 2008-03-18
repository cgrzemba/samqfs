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

// ident	$Id: AddLibrarySelectType.js,v 1.7 2008/03/17 14:40:28 am143972 Exp $


    function getMessage() {
        return document.wizWinForm.elements[
            "WizardWindow.Wizard.AddLibrarySelectTypeView.HiddenMessage"].
            value;
    }

    function isMediaDiscovery() {
        // if direct-attached or ACSLS is selected, media discovery takes place
        var radio = document.wizWinForm.elements[
            "WizardWindow.Wizard.AddLibrarySelectTypeView.LibraryType"];
        if (radio == null) return true;

        if (radio[0]) {
            if (radio[0].checked || radio[1].checked) {
                // Direct-attached or ACSLS Discovery
                return true;
            } 
        }
        return false;
    }

    function myNextClicked() {
        if (isMediaDiscovery()) {
            setTimeout("alert(getMessage())", 7000);
        }
        return true;
    }

    function changeComponentState(value) {
        ccSetDropDownMenuDisabled(
            "WizardWindow.Wizard.AddLibrarySelectTypeView.LibraryDriver",
            "wizWinForm", value != 3);
        ccSetTextFieldDisabled(
            "WizardWindow.Wizard.AddLibrarySelectTypeView.ACSLSHostName",
            "wizWinForm", value != 2);
        ccSetTextFieldDisabled(
            "WizardWindow.Wizard.AddLibrarySelectTypeView.ACSLSPortNumber",
            "wizWinForm", value != 2);

        switch (value) {
            case 2:
                document.getElementById("ACSLSHostName").focus();
                break;
            case 3:
                document.getElementById("LibraryDriver").focus();
                break;
        }
    }

    function wizardPageInit() {
        WizardWindow_Wizard.setFocusElementName(
            "WizardWindow.Wizard.AddLibrarySelectTypeView.ACSLSHostName");
    }
