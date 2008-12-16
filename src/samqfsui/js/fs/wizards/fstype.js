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

// ident	$Id: fstype.js,v 1.4 2008/12/16 00:10:38 am143972 Exp $

var formName = "wizWinForm";
var theForm = document.wizWinForm;
var prefix = "WizardWindow.Wizard.NewWizardFSTypeView.";

/* handler for the HAFS check box */
function handleHAFSCheckBox(checkbox) {
    var archiving = theForm.elements[prefix + "archivingCheckBox"];
    var shared = theForm.elements[prefix + "sharedCheckBox"];

    /*--OSD related--
    var hpc = theForm.elements[prefix + "HPCCheckBox"];
    var matfs = theForm.elements[prefix + "matfsCheckBox"];
    */

    if (checkbox.checked) {
        // disable archiving. Its not supported with HAFS
        archiving.checked = false;
        ccSetCheckBoxDisabled(prefix + "archivingCheckBox", formName, 1);
        
        // if shared is selected, disable the HPC check box. HAFS is not
        // supported with HPC
        /*--OSD related--
        if (shared.checked) {
            hpc.checked = false;
            ccSetCheckBoxDisabled(prefix + "HPCCheckBox", formName, 1);
        }
        if (matfs != null) {
            matfs.checked = false;
            ccSetCheckBoxDisabled(matfs.name, formName, 1);
        }
        */
    } else {
        // enable archiving
        ccSetCheckBoxDisabled(prefix + "archivingCheckBox", formName, 0);

        // if shared is checked, enable hpc
        /*--OS -related--
        if (shared.checked) {
            ccSetCheckBoxDisabled(prefix + "HPCCheckBox", formName, 0);
        }
        if (matfs != null && !shared.ckecked && !archiving.checked) {
            ccSetCheckBoxDisabled(matfs.name, formName, 0);
        }
        */
    }
}

/* handler for the archiving checkbox */
function handleArchivingCheckBox(checkbox) {
    var hafs = checkbox.form.elements[prefix + "HAFSCheckBox"];
    var shared = checkbox.form.elements[prefix + "sharedCheckBox"];

    /*--OSD related--
    var matfs = checkbox.form.elements[prefix + "matfsCheckBox"];
    */

    if (checkbox.checked) {
        
        // disable HAFS & MATFS. Archiving HAFS/MATFS are not supported
        if (hafs != null) {
            ccSetCheckBoxDisabled(prefix + "HAFSCheckBox", formName, 1);
        }

        /*--OSD related--
        if (matfs != null) {
            matfs.checked = false;
            ccSetCheckBoxDisabled(prefix + "matfsCheckBox", formName, 1);
        }
        */

        // if the current server has no archive media, warn the user
        var hasArchiveMedia =
          checkbox.form.elements[prefix + "hasArchiveMedia"].value;
        if (hasArchiveMedia != "true") {
          alert(checkbox.form.elements[prefix + "archiveMediaWarning"].value);
        }
    }  else {
        if (hafs != null) {
            ccSetCheckBoxDisabled(prefix + "HAFSCheckBox", formName, 0);
        }

        /*--OSD related--
        if (matfs != null && !shared.checked) {
            ccSetCheckBoxDisabled(prefix + "matfsCheckBox", formName, 0);
        }
        */
    }
    
    return false;
}

/* handler for the shared fs check box */
function handleSharedCheckBox(checkbox) {
    var hafs = theForm.elements[prefix + "HAFSCheckBox"];    
    var archiving = theForm.elements[prefix + "archivingCheckBox"];

    var hpcCheckBox = theForm.elements[prefix + "HPCCheckBox"];
    var matfs = theForm.elements[prefix + "matfsCheckBox"];

    if (hpcCheckBox != null)
        handleHPCCheckBox(hpcCheckBox);

    if (checkbox.checked) {
        // enable hpc checkbox if HAFS not selected
        if (hafs != null && hafs.checked) {
            ccSetCheckBoxDisabled(prefix + "HPCCheckBox", formName, 1);
        } else {
            ccSetCheckBoxDisabled(prefix + "HPCCheckBox", formName, 0);
        }
        if (matfs != null) {
            matfs.checked = false;
            ccSetCheckBoxDisabled(prefix + "matfsCheckBox", formName, 1);
        }
    } else {
        // disable hpc checkbox and fs name
        hpcCheckBox.checked = false;
        ccSetCheckBoxDisabled(prefix + "HPCCheckBox", formName, 1);
        ccSetTextFieldDisabled(prefix + "FSName", formName, 1);

        // if HAFS available enable it just incase it had been disabled by
        // clicking on HPC
        if (hafs != null) {
            ccSetCheckBoxDisabled(hafs.name, formName, 0);
        }
        if (matfs != null && !archiving.checked) {
            ccSetCheckBoxDisabled(matfs.name, formName, 0);
        }
        
    }
    return false;
}

/* handler for the HPC Configuration check box */
function handleHPCCheckBox(checkbox) {
    var hafs = checkbox.form.elements[prefix + "HAFSCheckBox"];

    if (checkbox.checked) {
        // disable HAFS, its not supported under HPC
        if (hafs != null) {
            hafs.checked = false;
            ccSetCheckBoxDisabled(prefix + "HAFSCheckBox", formName, 1);
        }            
    } else {
        if (hafs != null) {
            ccSetCheckBoxDisabled(prefix + "HAFSCheckBox", formName, 0);
        }
    }
    
    return false;
}

/* handler for the matfs check box */
function handleMATFSCheckBox(button) {
    var hafs = theForm.elements[prefix + "HAFSCheckBox"];
    var shared = theForm.elements[prefix + "sharedCheckBox"];
    var archiving = theForm.elements[prefix + "archivingCheckBox"];

    if (button.checked) {
        if (hafs != null) ccSetCheckBoxDisabled(hafs.name, formName, 1);
        ccSetCheckBoxDisabled(prefix + "sharedCheckBox", formName, 1);
        ccSetCheckBoxDisabled(prefix + "archivingCheckBox", formName, 1);
    } else {
        if (hafs != null) ccSetCheckBoxDisabled(hafs.name, formName, 0);
        ccSetCheckBoxDisabled(prefix + "sharedCheckBox", formName, 0);
        ccSetCheckBoxDisabled(prefix + "archivingCheckBox", formName, 0);
    }

    return false;
}

