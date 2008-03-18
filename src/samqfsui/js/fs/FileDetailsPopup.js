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

// ident	$Id: FileDetailsPopup.js,v 1.4 2008/03/17 14:40:26 am143972 Exp $

    /**
     * Scroll to the bottom of the page if user is editing file attributes.
     */
    function scrollToBottomWhenNeeded() {
        if ('-1' != document.FileDetailsPopupForm.
            elements['FileDetailsPopup.PageMode'].value) {
            window.scroll(0, window.screen.availHeight);
        }
    }

    /**
     * Hide unnecessary components based on what the user is editing.
     * i.e. Show only archive buttons if user chooses to edit archiving
     *      attributes.
     *
     *     public static final int RELEASE = 1;
     *     public static final int STAGE   = 2;
     *
     */
    function hideComponents() {
        var mode = parseInt(document.FileDetailsPopupForm.
                                elements['FileDetailsPopup.PageMode'].value);
        if (mode == 1) {
            $("releaseDiv").style.visibility="visible";
        } else if ($("releaseDiv") != null) {
            $("releaseDiv").style.visibility="hidden";
        }
    }

    /**
     * Disable the unrelevant contents if never is selected
     */
    function handleRadioSelection(field) {
        var mode = parseInt(document.FileDetailsPopupForm.
                                elements['FileDetailsPopup.PageMode'].value);
        var myForm = field.form;
        var prefix = "FileDetailsPopup.ChangeFileAttributesView.";
        var radio = myForm.elements[prefix + "Radio"];
        var elementName;

        // no need to do anything for archive
        if (mode == 1) {
            elementName = prefix + "SubRadio";
            ccSetRadioButtonDisabled(
                elementName, myForm.name, radio[0].checked);
            var subRadio = myForm.elements[elementName];
            if (subRadio.value == undefined) {
                subRadio[0].checked = true;
            }
            elementName = prefix + "PartialRelease";
            ccSetCheckBoxDisabled(
                elementName, myForm.name, radio[0].checked);
            elementName = prefix + "PartialReleaseSize";
            var checkBox = myForm.elements[prefix + "PartialRelease"];
            ccSetTextFieldDisabled(
                elementName, myForm.name,
                radio[0].checked || !checkBox.checked);
        } else if (mode == 2) {
            elementName = prefix + "SubRadio";
            ccSetRadioButtonDisabled(
                elementName, myForm.name, radio[0].checked);
            var subRadio = myForm.elements[elementName];
            if (subRadio.value == undefined) {
                subRadio[0].checked = true;
            }
        }
    }

    /**
     * Disable partial release size text field if partial release is not checked
     */
    function handlePartialRelease(field) {
        ccSetTextFieldDisabled(
            "FileDetailsPopup.ChangeFileAttributesView.PartialReleaseSize",
            field.form.name, !field.checked);
    }

    /* helper function to retrieve element by id */
    function $(id) {
        return document.getElementById(id);
    }
