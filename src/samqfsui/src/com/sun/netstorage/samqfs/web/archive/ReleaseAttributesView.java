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

// ident	$Id: ReleaseAttributesView.java,v 1.2 2008/11/05 20:24:48 ronaldso Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.web.fs.ChangeFileAttributesView;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCTextField;

/**
 * ReleaseAttributesView - view in CriteriaDetails page.
 */
public class ReleaseAttributesView extends ChangeFileAttributesView {

    public ReleaseAttributesView(
        View parent, String name, String serverName) {
        super(parent, name, serverName, false,
              ChangeFileAttributesView.PAGE_CRITERIA_DETAIL,
              ChangeFileAttributesView.MODE_RELEASE);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        super.registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * This method is called when the page is loaded
     * @Override
     * @param evt Display Event
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(evt);
        TraceUtil.trace3("Exiting");
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
     * Hide label if server does not support partial size setting
     * @param event
     * @return if the Size (kb) label is shown
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return isServerPatched();
    }

    /**
     * Hide partial size text field if server does not support partial size
     * setting
     * @param event
     * @return if the partial release size text field is shown
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginPartialReleaseSizeDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return isServerPatched();
    }

    /**
     * Populate the view components
     * @param propert the criteria property object that contains the current
     * property settings
     */
    protected void populateReleaseAttributes(ArchivePolCriteriaProp property) {
        CCRadioButton radio = (CCRadioButton) getChild(RADIO);
        CCRadioButton subRadio = (CCRadioButton) getChild(SUB_RADIO);
        CCCheckBox partialRelease = (CCCheckBox) getChild(PARTIAL_RELEASE);
        CCTextField partialSize = (CCTextField) getChild(PARTIAL_RELEASE_SIZE);

        int releaseAttr = property.getReleaseAttributes();

        // Never Release or
        // Default (when HWM is reached) or when one copy is made
        if ((releaseAttr & Criteria.ATTR_RELEASE_NEVER)
                        == Criteria.ATTR_RELEASE_NEVER) {
            TraceUtil.trace3("NEVER RELEASE!");
            // Never stage is set
            radio.setValue(Integer.toString(Releaser.NEVER));
            radio.resetStateData();
            subRadio.setDisabled(true);
            partialRelease.setChecked(false);
            partialRelease.setDisabled(true);
            partialSize.setDisabled(true);
            partialSize.setValue("");
        } else {
            // staging is set
            radio.setValue(RELEASE);
            radio.resetStateData();
            subRadio.setDisabled(false);

            // check if the flag is "when HWM is reached", or it is set
            // as release after one copy
            if ((releaseAttr & Criteria.ATTR_RELEASE_ALWAYS)
                            == Criteria.ATTR_RELEASE_ALWAYS) {
                TraceUtil.trace3("RELEASE ALWAYS WHEN 1!");
                subRadio.setValue(Integer.toString(Releaser.WHEN_1));
            } else {
                TraceUtil.trace3("RELEASE DEFAULT!");
                subRadio.setValue(Integer.toString(Releaser.RESET_DEFAULTS));
            }
            subRadio.resetStateData();

            // Set Partial Size Check Box if ATTR_RELEASE_PARTIAL is set
            if ((releaseAttr & Criteria.ATTR_RELEASE_PARTIAL)
                            == Criteria.ATTR_RELEASE_PARTIAL ||
                (releaseAttr & Criteria.ATTR_PARTIAL_SIZE)
                            == Criteria.ATTR_PARTIAL_SIZE) {
                partialRelease.setChecked(true);
                partialSize.setDisabled(false);

                if (isServerPatched() &&
                    (releaseAttr & Criteria.ATTR_PARTIAL_SIZE)
                            == Criteria.ATTR_PARTIAL_SIZE) {
                    TraceUtil.trace3("Release Partial Size is set, size is " +
                        property.getPartialSize());
                    partialSize.setValue(
                        Integer.toString(property.getPartialSize()));
                } else {
                    partialSize.setValue("");
                }
            } else {
                partialRelease.setChecked(false);
                partialSize.setDisabled(true);
                partialSize.setValue("");
            }
        }

        partialRelease.resetStateData();
        partialSize.resetStateData();

        // Override? (-d flag)
        if ((releaseAttr & Criteria.ATTR_RESET_RELEASE_DEFAULT)
                        == Criteria.ATTR_RESET_RELEASE_DEFAULT) {
            ((CCCheckBox) getChild(OVERRIDE)).setChecked(true);
        } else {
            ((CCCheckBox) getChild(OVERRIDE)).setChecked(false);
        }
    }

    /**
     * To save the release settings to the criteria property object
     * @param property criteria property object of which the settings are set to
     * @return an error string if an error message needs to be shown upon
     * data validation.
     */
    protected String saveReleaseSettings(ArchivePolCriteriaProp property) {
        CCRadioButton radio = (CCRadioButton) getChild(RADIO);
        CCRadioButton subRadio = (CCRadioButton) getChild(SUB_RADIO);
        CCCheckBox override = (CCCheckBox) getChild(OVERRIDE);
        CCCheckBox partialReleaseBox = (CCCheckBox) getChild(PARTIAL_RELEASE);
        int partialSize = 0;

        int newReleaseAttr = 0;
        String radioValue = (String) radio.getValue();
        String subRadioValue = (String) subRadio.getValue();
        TraceUtil.trace3("SAVE: radioValue: " + radioValue);
        TraceUtil.trace3("SAVE: subRadioValue: " + subRadioValue);
        if (radioValue.equals(Integer.toString(Releaser.NEVER))) {
            // Never Release
            TraceUtil.trace3("SAVE: NEVER RELEASE");
            newReleaseAttr |= Criteria.ATTR_RELEASE_NEVER;
        } else {
            if (subRadioValue.equals(Integer.toString(Releaser.WHEN_1))) {
                // Always after One Copy
                TraceUtil.trace3("SAVE: ALWAYS AFTER ONE COPY");
                newReleaseAttr |= Criteria.ATTR_RELEASE_ALWAYS;
            } else {
                TraceUtil.trace3("SAVE: WHEN SPACE IS NEEDED");
                // Reset attribute if "When space is needed" is selected
                property.resetReleaseAttributes();
            }

            String partialSizeStr = ((String) getDisplayFieldValue(
                                        PARTIAL_RELEASE_SIZE)).trim();
            setDisplayFieldValue(PARTIAL_RELEASE_SIZE, partialSizeStr);

            // Partial Size
            if (partialReleaseBox.isChecked()) {
                if (partialSizeStr.length() == 0) {
                    // Partial is selected without defining a size
                    newReleaseAttr |= Criteria.ATTR_RELEASE_PARTIAL;
                } else {
                    try {
                        partialSize = Integer.parseInt(partialSizeStr);
                        String errorStr = validate(property, partialSize);
                        if (errorStr != null) {
                            return errorStr;
                        }
                        newReleaseAttr |= Criteria.ATTR_PARTIAL_SIZE;
                    } catch (NumberFormatException numEx) {
                        TraceUtil.trace1(
                            "NumberFormatException caught for partialSize!");
                        return SamUtil.getResourceString(
                                "fs.filedetails.releasing.invalidReleaseSize");
                    } catch (SamFSException samEx) {
                        TraceUtil.trace1("SAM Exception caught! Reason: " +
                            samEx.getMessage());
                        return SamUtil.getResourceString(
                                "fs.filedetails.releasing.invalidReleaseSize");
                    }
                }
            }
        }

        // Check if Override is checked
        if (override.isChecked()) {
            TraceUtil.trace3("Override is checked!");
            newReleaseAttr |= Criteria.ATTR_RESET_RELEASE_DEFAULT;
        }
        TraceUtil.trace3("newReleaseAttr: " + newReleaseAttr);
        TraceUtil.trace3("partialSize: " + partialSize);

        // Check if server is patched to support multiple attributes.
        // If not, make sure the release attribute does not contain multiple
        // flags
        if (!isServerPatched() && isMultiFlag(newReleaseAttr, true)) {
            return SamUtil.getResourceString(
                        "fs.filedetails.releasing.multiflagnotsupported");
        }

        // Now save all settings unless user selects "when space is needed"
        if (newReleaseAttr != 0) {
            property.setReleaseAttributes(newReleaseAttr, partialSize);
        } else if (partialSize != 0) {
            // for users who choose "when space is needed" with a partial size
            property.setPartialSize(partialSize);
        }

        return null;
    }

    private String validate(ArchivePolCriteriaProp property, int partialSize)
        throws SamFSException {
        // Partial Size has to be between 8 and the file system(s) max partial
        // release size
        if (partialSize < 8) {
            return SamUtil.getResourceString(
                            "fs.filedetails.releasing.invalidReleaseSize");
        }
        FileSystem [] fs =
            property.getArchivePolCriteria().getFileSystemsForCriteria();
        for (int i = 0; i < fs.length; i++) {
            int maxSize = fs[i].getMountProperties().
                            getDefaultMaxPartialReleaseSize();
            int maxUnit = fs[i].getMountProperties().
                            getDefaultMaxPartialReleaseSizeUnit();
            TraceUtil.trace3("Validate partialSize, testing against fs name: " +
                fs[i].getName() + ", maxSize: " +
                maxSize + ", maxUnit: " + maxUnit);
            // Return error if the partialSize (in kB) is greater than the file
            // system maximum partial release size
            if (Capacity.newExactCapacity((long) maxSize, maxUnit).compareTo(
                Capacity.newExactCapacity(
                    (long) partialSize, SamQFSSystemModel.SIZE_KB)) < 0) {
                // error
                return SamUtil.getResourceString(
                "fs.filedetails.releasing.invalidReleaseSize.exceedfslimit");
            }
        }
        return null;
    }
}
