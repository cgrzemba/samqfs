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

// ident	$Id: NewWizardFSName.js,v 1.14 2008/12/16 00:10:38 am143972 Exp $

/** 
 * This is the javascript file of New File System Wizard Select Name Page
 */

    WizardWindow_Wizard.nextClicked = myNextClicked;
    WizardWindow_Wizard.pageInit = wizardPageInit;

    var myForm = document.wizWinForm;
    var prefix = 'WizardWindow.Wizard.NewWizardFSNameView.';

    function getMessage() {
        return myForm.elements[prefix + 'HiddenMessage'].value;
    }

    function myNextClicked() {
        if (!isSharedEnabled()) {
            setTimeout("alert(getMessage())", 7000);
        }
        return true;
    }

    function wizardPageInit() {
        // if HA is selected, disable metadata and date in same device
        // disable Archive as well
        var hafs = myForm.elements[prefix + 'hafs'];
        if (hafs != null) {
            var metaSame =
                myForm.elements[prefix + 'metaLocationSelect'];
            if (hafs.checked) {
                metaSame[0].checked  = !hafs.checked;
                metaSame[0].disabled = hafs.checked;
                metaSame[1].checked  = hafs.checked;

                // disable archiving
                if (!isArchiveCheckNull()) {
                    var archiveCheckBox =
                        myForm.elements[prefix + 'archiveCheck'];
                    archiveCheckBox.checked = false;
                    ccSetCheckBoxDisabled(
                        prefix + 'archiveCheck', 'wizWinForm', hafs.checked);
                }
            }
        }

        var toggleHidden = myForm.elements[prefix + 'isAdvancedMode'];
        var labels = myForm.elements[prefix + 'ToggleButtonLabels'].value;
        var labelsArray = labels.split("###");

        if (toggleHidden.value == "true") {
            onButtonToggle(labelsArray[0]);
        } else {
            onButtonToggle(labelsArray[1]);
        }
    }

    /**
     * This method checks if the Archive check box exists or not.  In a plain
     * QFS Setup, the Archive Check Box does not exist.
     */
    function isArchiveCheckNull() {
        var checkbox = myForm.elements[prefix + 'archiveCheck'];
        return (checkbox == null);
    }

    function clearAllQFSCheckBoxes() {
        var checkbox = myForm.elements[prefix + 'sharedCheck'];
        checkbox.checked = false;
        checkbox = myForm.elements[prefix + 'archiveCheck'];
        checkbox.checked = false;
        var hafs = myForm.elements[prefix + 'hafs'];
        if (hafs != null)
            hafs.checked = false;
    }
   
    function ufsSelected() {
        // disable metadata location radio button
        ccSetRadioButtonDisabled(
            prefix + 'metaLocationSelect', 'wizWinForm', 1);
            
        // disable archive check box if it exists
        if (!isArchiveCheckNull()) {
            ccSetCheckBoxDisabled(prefix + 'archiveCheck', 'wizWinForm', 1);
        }
        
        ccSetCheckBoxDisabled (prefix + 'sharedCheck', 'wizWinForm', 1);
        ccSetCheckBoxDisabled (prefix + 'hafs',        'wizWinForm', 1);
        ccSetDropDownMenuDisabled (prefix + 'DAUDropDown', 'wizWinForm', 1);
        ccSetTextFieldDisabled(prefix + 'stripeValue', 'wizWinForm', 1);
        ccSetRadioButtonDisabled (prefix + 'allocSelect', 'wizWinForm', 1);
        ccSetTextFieldDisabled(prefix + 'numOfStripedGroupTextField', 'wizWinForm', 1);
        ccSetTextFieldDisabled(prefix + 'DAUSizeField', 'wizWinForm', 1);
        ccSetDropDownMenuDisabled (prefix + 'DAUSizeDropDown', 'wizWinForm', 1);

        clearAllQFSCheckBoxes();

        // Hide advanced section & disable advanced button,
        // they are not applicable to QFS
        ccSetButtonDisabled (prefix + 'ToggleButton',  'wizWinForm', 1);

        var labels = myForm.elements[prefix + 'ToggleButtonLabels'].value;
        var labelsArray = labels.split("###");
        onButtonToggle(labelsArray[1]);
    }
    
    function qfsSelected() {
        ccSetCheckBoxDisabled (prefix + 'hafs',         'wizWinForm', 0);
        ccSetCheckBoxDisabled (prefix + 'sharedCheck',  'wizWinForm', 0);
        ccSetCheckBoxDisabled (prefix + 'archiveCheck', 'wizWinForm', 0);
        ccSetRadioButtonDisabled(
            prefix + 'metaLocationSelect',
            'wizWinForm', 0);
        ccSetButtonDisabled (prefix + 'ToggleButton',  'wizWinForm', 0);
        ccSetDropDownMenuDisabled (prefix + 'DAUDropDown', 'wizWinForm', 0);
        ccSetTextFieldDisabled(prefix + 'stripeValue', 'wizWinForm', 0);
        ccSetRadioButtonDisabled (prefix + 'allocSelect', 'wizWinForm', 0);
        ccSetTextFieldDisabled(prefix + 'numOfStripedGroupTextField', 'wizWinForm', 0);
        ccSetTextFieldDisabled(prefix + 'DAUSizeField', 'wizWinForm', 0);
        ccSetDropDownMenuDisabled (prefix + 'DAUSizeDropDown', 'wizWinForm', 0);

        // pre-select Metadata and data to same device
        onClickMetaDataLocation("FSWizard.new.fstype.qfs.metaSame");
    }
    
    function onClickMetaDataLocation(selectionValue) {
        var value = selectionValue == "FSWizard.new.fstype.qfs.metaSame" ?
            "none" : "";

        $("separateDiv1").style.display=value;
        $("separateDiv2").style.display=value;
        $("separateDiv3").style.display=value;
        $("separateDiv4").style.display=value;

        var stripeTextValue;
        var DAUDropDownValue;
        var DAUTextFieldValue;
        var DAUSizeDropDownValue;
        var stripeText = myForm.elements[prefix + 'stripeValue'];
        var DAUDropDown = myForm.elements[prefix + 'DAUDropDown'];
        var DAUTextField = myForm.elements[prefix + 'DAUSizeField'];
        var DAUSizeDropDown = myForm.elements[prefix + 'DAUSizeDropDown'];

        // Meta & data on same device
        // DAU Size is 64kb if not set previously
        // DAUs per device is 0 if shared is enabled.  Otherwise it is
        // 128/<value of drop down>
        if (value == "none") {
            DAUDropDown.disabled = false;
            DAUDropDownValue = 64;
        // Meta Separate
        } else {
            DAUDropDownValue     = 64;
            DAUTextFieldValue    = 64;
            DAUSizeDropDownValue = 'kb';

            var daMethod = myForm.elements[prefix + 'allocSelect'];
            var selectedValue = "";
            if (daMethod[0].checked) {
                selectedValue = "FSWizard.new.qfs.singleAllocation";
            } else if (daMethod[1].checked) {
                selectedValue = "FSWizard.new.qfs.dualAllocation.label";
            } else {
                selectedValue = "FSWizard.new.qfs.stripedGroup";
            }
            onClickDataAllocationMethod(selectedValue);
        }

        if (isSharedEnabled()) {
            stripeTextValue = 0;
        } else {
            stripeTextValue = Math.round(128 / DAUDropDownValue);
        }

        // update values on screen
        DAUDropDown.value     = DAUDropDownValue;
        stripeText.value      = stripeTextValue;
        DAUTextField.value    = DAUTextFieldValue;
        DAUSizeDropDown.value = DAUSizeDropDownValue;
    }

    function onSelectDAUDropDown(field) {
        var stripeText = myForm.elements[prefix + 'stripeValue'];

        if (isSharedEnabled()) {
            stripeText.value = 0;
        } else {
            stripeText.value = 128 / parseInt(field.value);
        }
    }

    function onBlurDAUSizeField(field) {
        field.value = trim(field.value);
        var stripeText = myForm.elements[prefix + 'stripeValue'];
        var alloc = myForm.elements[prefix + 'allocSelect'];

        if (isSharedEnabled() || alloc[2].checked) {
            stripeText.value = 0;
        } else if (field.value != '') {
            var sizeDropDown = myForm.elements[prefix + 'DAUSizeDropDown'];
            if (sizeDropDown.value == 'mb') {
                stripeText.value = 1;
            } else {
                var myValue = parseInt(field.value);
                if (myValue >= 128) {
                    stripeText.value = 1;
                } else if (myValue >= 1 && myValue < 127) {
                    stripeText.value = Math.round(128 / myValue);
                }
            }
        }
    }

    function onChangeDAUSizeDropDown(field) {
        var stripeText = myForm.elements[prefix + 'stripeValue'];
        var alloc = myForm.elements[prefix + 'allocSelect'];
        var DAUTextField =
            myForm.elements[prefix + 'DAUSizeField'].value;                   

        if (isSharedEnabled() || alloc[2].checked) {
            stripeText.value = 0;
        } else if (field.value == 'mb') {
            stripeText.value = 1;
        } else if (trim(DAUTextField) != '') {
            var myValue = parseInt(DAUTextField);
            if (myValue >= 128) {
                stripeText.value = 1;
            } else if (myValue >= 1 && myValue < 127) {
                stripeText.value = Math.round(128 / myValue);
            }
        }
    }

    function onClickDataAllocationMethod(selectedValue) {
        var stripeText = myForm.elements[prefix + 'stripeValue'];
        var DAUDropDown = myForm.elements[prefix + 'DAUDropDown'];
        var DAUTextField = myForm.elements[prefix + 'DAUSizeField'];
        var DAUSizeDropDown = myForm.elements[prefix + 'DAUSizeDropDown'];

        if (selectedValue == 'FSWizard.new.qfs.stripedGroup') {
            ccSetTextFieldDisabled(
                prefix + 'numOfStripedGroupTextField',
                'wizWinForm',
                false);
            DAUTextField.value = 256;
            DAUSizeDropDown.value  = 'kb';
            stripeText.value  = 0;                   
        } else {
            ccSetTextFieldDisabled(
                prefix + 'numOfStripedGroupTextField',
                'wizWinForm',
                true);
            if (isSharedEnabled()) {
                stripeText.value  = 0;
            } else {
                stripeText.value  = 2;
            }

            if (selectedValue == prefix + 'allocSelect2') {
                DAUDropDown.value = 64;
            } else {
                DAUTextField.value = 64;
                DAUSizeDropDown.value  = 'kb';
            }
        }

        if (selectedValue == 'FSWizard.new.qfs.dualAllocation.label') { 
            ccSetDropDownMenuDisabled(
                prefix + 'DAUDropDown',
                'wizWinForm',
                false);
            ccSetTextFieldDisabled(
                prefix + 'DAUSizeField',
                'wizWinForm',
                true);
            ccSetDropDownMenuDisabled(
                prefix + 'DAUSizeDropDown',
                'wizWinForm',
                true);               
        } else {
            ccSetDropDownMenuDisabled(
                prefix + 'DAUDropDown',
                'wizWinForm',
                true);
            ccSetTextFieldDisabled(
                prefix + 'DAUSizeField',
                'wizWinForm',
                false);
            ccSetDropDownMenuDisabled(
                prefix + 'DAUSizeDropDown',
                'wizWinForm',
                false);
        }
    }
    
    function onSelectArchiveCheckBox() {
        var hafs = myForm.elements[prefix + 'hafs'];
        if (hafs != null)
            hafs.checked = false;
    }
    
    function onSelectSharedCheckBox(box) {
        if (box.checked) {
            // Shared is now enabled, change stripe width to 0
            var stripeText = myForm.elements[prefix + 'stripeValue'];
            stripeText.value = 0;

            // check hafs checkbox is we are on a cluster node
            // See CR 6496131
            // It is a valid case to create a HA file system without enabling
            // the shared flag.  Thus comment the following logic out.
            /*
            var hafs = myForm.elements[prefix + 'hafs'];
            if (hafs != null) {
                hafs.checked = true;
                // deselect & disable Archive because HA is selected
                ccSetCheckBoxDisabled(
                    prefix + 'archiveCheck', 'wizWinForm', true);
                var archiveBox = myForm.elements[prefix + 'archiveCheck'];
                archiveBox.checked = false;

                // disable metadata and data on same device radio button
                var metaSame =
                    myForm.elements[prefix + 'metaLocationSelect'];
                metaSame[0].checked  = false;
                metaSame[0].disabled = true;
                metaSame[1].checked  = true;

                onClickMetaDataLocation("FSWizard.new.fstype.qfs.metaSeparate");
            }
            */
        }
    }

    /* handler for the hafs check box */
    function hafsSelected(field) {
        // 1. set metadata separate and disable metadata location
        var metaSame =
            myForm.elements[prefix + 'metaLocationSelect'];
        metaSame[0].checked  = !field.checked;
        metaSame[0].disabled = field.checked;
        metaSame[1].checked  = field.checked;

        // If HA is enabled, disable meta & data on same device radio button.
        if (field.checked) {           
            onClickMetaDataLocation("FSWizard.new.fstype.qfs.metaSeparate");
        } else {
            onClickMetaDataLocation("FSWizard.new.fstype.qfs.metaSame");
        }

        // 2. disable archiving
        var archiveCheckBox = myForm.elements[prefix + 'archiveCheck'];
        archiveCheckBox.checked = false;
        ccSetCheckBoxDisabled(
            prefix + 'archiveCheck', 'wizWinForm', field.checked);

        // If user unchecks HA checkbox, Shared Check Box has to be unchecked
        // See CR 6496131
        // It is a valid case to create a HA file system without enabling
        // the shared flag.  Thus comment the following logic out
        /*
        if (!field.checked) {
            var checkbox = myForm.elements[prefix + 'sharedCheck'];
            checkbox.checked = false;
        }
        */
    }

    function onButtonToggle(selectedValue) {
        var toggleHidden = myForm.elements[prefix + 'isAdvancedMode'];
        var button = myForm.elements[prefix + 'ToggleButton'];
        var labels = myForm.elements[prefix + 'ToggleButtonLabels'].value;
        var labelsArray = labels.split("###");

        if (selectedValue == labelsArray[0]) {
            toggleHidden.value = "true";

            // update button label
            button.value = labelsArray[1];

            // Show advanced content
            $("advancedDiv").style.display="";

            // figure out which radio is selected & prepopulate rest of the
            // fields
            var radio = myForm.elements[prefix + 'metaLocationSelect'];
            if (radio[0].checked) {
                onClickMetaDataLocation("FSWizard.new.fstype.qfs.metaSame");
            } else {
                onClickMetaDataLocation("FSWizard.new.fstype.qfs.metaSeparate");
            }

        } else {
            toggleHidden.value = "false";

            // update button label
            button.value = labelsArray[0];

            // Hide advanced content
            $("advancedDiv").style.display="none";
        }
    }

    function isSharedEnabled() {
        var checkbox = myForm.elements[prefix + 'sharedCheck'];
        return checkbox.checked;
    }

    function isArchiveEnabled() {
        var checkbox = myForm.elements[prefix + 'archiveCheck'];
        return checkbox.checked;
    }

    /* helper function to retrieve element by id */
    function $(id) {
        return document.getElementById(id);
    }
