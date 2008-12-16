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

// ident	$Id: StageAttributesView.java,v 1.3 2008/12/16 00:10:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
import com.sun.netstorage.samqfs.web.fs.ChangeFileAttributesView;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCRadioButton;

/**
 * StageAttributesView - view in CriteriaDetails page.
 */
public class StageAttributesView extends ChangeFileAttributesView {

    public StageAttributesView(
        View parent, String name, String serverName) {
        super(parent, name, serverName, false,
              ChangeFileAttributesView.PAGE_CRITERIA_DETAIL,
              ChangeFileAttributesView.MODE_STAGE);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        super.registerChildren();

        TraceUtil.trace3("Exiting");
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(evt);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Hide partial release check box
     * @param event
     * @return boolean to indicate if we want to show this component
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginPartialReleaseDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return false;
    }

    /**
     * Hide label
     * @param event
     * @return boolean to indicate if we want to show this component
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return false;
    }

    /**
     * Hide Partial Release text field
     * @param event
     * @return boolean to indicate if we want to show this component
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginPartialReleaseSizeDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return false;
    }

    /**
     * Hide partial release size help text
     * @param event
     * @return boolean to indicate if we want to show this component
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginHelpTextDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return false;
    }

    /**
     * Hide the submit button of the pagelet, use the SAVE button in the page
     * to save the changes
     * @param event
     * @return boolean to indicate if we want to show this component
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginSubmitDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return false;
    }

    /**
     * Populate view components
     * @param property that contains the current criteria property settings
     */
    protected void populateStageAttributes(ArchivePolCriteriaProp property) {
        CCRadioButton radio = (CCRadioButton) getChild(RADIO);
        CCRadioButton subRadio = (CCRadioButton) getChild(SUB_RADIO);

        int stageAttr = property.getStageAttributes();

        // Never Stage or
        // Default (when a file is accessed) or Associative staging
        if ((stageAttr & Criteria.ATTR_STAGE_NEVER)
                      == Criteria.ATTR_STAGE_NEVER) {
            // Never stage is set
            TraceUtil.trace3("NEVER STAGE!");
            radio.setValue(Integer.toString(Stager.NEVER));
            radio.resetStateData();
            subRadio.setDisabled(true);
        } else {
            // staging is set
            radio.setValue(STAGE);
            radio.resetStateData();
            subRadio.setDisabled(false);

            // check if the flag is "when a file is accessed", or it is set
            // as associative staging
            if ((stageAttr & Criteria.ATTR_STAGE_ASSOCIATIVE)
                          == Criteria.ATTR_STAGE_ASSOCIATIVE) {
                TraceUtil.trace3("STAGE ASSOCIATIVE!");
                subRadio.setValue(Integer.toString(Stager.ASSOCIATIVE));
            } else {
                TraceUtil.trace3("STAGE DEFAULT!");
                subRadio.setValue(Integer.toString(Stager.RESET_DEFAULTS));
            }
            subRadio.resetStateData();
        }

        // Override? (-d flag)
        if ((stageAttr & Criteria.ATTR_RESET_STAGE_DEFAULT)
                        == Criteria.ATTR_RESET_STAGE_DEFAULT) {
            ((CCCheckBox) getChild(OVERRIDE)).setChecked(true);
        } else {
            ((CCCheckBox) getChild(OVERRIDE)).setChecked(false);
        }
    }

    /**
     * To save the stage settings to the criteria property object
     * @param property criteria property object of which the settings are set to
     * @return an error string if an error message needs to be shown upon
     * data validation.
     */
    protected String saveStageSettings(ArchivePolCriteriaProp property) {
        CCRadioButton radio = (CCRadioButton) getChild(RADIO);
        CCRadioButton subRadio = (CCRadioButton) getChild(SUB_RADIO);
        CCCheckBox override = (CCCheckBox) getChild(OVERRIDE);

        int newStageAttr = 0;
        String radioValue = (String) radio.getValue();
        String subRadioValue = (String) subRadio.getValue();

        if (radioValue.equals(Integer.toString(Stager.NEVER))) {
            // Never Release
            newStageAttr |= Criteria.ATTR_STAGE_NEVER;
        } else {
            if (subRadioValue.equals(Integer.toString(Stager.ASSOCIATIVE))) {
                // Associative
                newStageAttr |= Criteria.ATTR_STAGE_ASSOCIATIVE;
            } else {
                // When a file is accessed, reset stage attribute
                 property.resetStageAttributes();
            }
        }

        // Check if override is checked
        if (override.isChecked()) {
            TraceUtil.trace3("Override is checked!");
            newStageAttr |= Criteria.ATTR_RESET_STAGE_DEFAULT;
        }
        TraceUtil.trace3("newStageAttr: " + newStageAttr);

        // Check if server is patched to support multiple attributes.
        // If not, make sure the release attribute does not contain multiple
        // flags
        if (!isServerPatched() && isMultiFlag(newStageAttr, false)) {
            return SamUtil.getResourceString(
                        "fs.filedetails.staging.multiflagnotsupported");
        }

        // Now save all settings unless user selects "when a file is accessed"
        if (newStageAttr != 0) {
            property.setStageAttributes(newStageAttr);
        }
        return null;
    }
}
