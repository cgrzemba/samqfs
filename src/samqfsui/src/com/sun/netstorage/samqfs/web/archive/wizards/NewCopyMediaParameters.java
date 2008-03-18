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

// ident	$Id: NewCopyMediaParameters.java,v 1.16 2008/03/17 14:43:30 am143972 Exp $


package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.ReservationMethodHelper;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for Media Parameters Page
 *
 */
public class NewCopyMediaParameters extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewCopyMediaParameters";

    // Child view names (i.e. display fields).
    public static final String CHILD_ARCHIVE_AGE_TEXT = "ArchiveAgeText";
    public static final String CHILD_ARCHIVE_AGE_TEXTFIELD =
        "ArchiveAgeTextField";
    public static final String CHILD_ARCHIVE_AGE_DROPDOWN =
        "ArchiveAgeDropDown";
    public static final String CHILD_ARCHIVE_MEDTYPE_TEXT =
        "ArchiveMediaTypeText";
    public static final String CHILD_TAPE_TEXT = "TapeText";
    public static final String CHILD_DISK_TEXT = "DiskText";
    public static final String CHILD_MEDTYPE_RADIO1 = "MedTypeRadio1";
    public static final String CHILD_MEDTYPE_RADIO2 = "MedTypeRadio2";
    public static final String CHILD_DISK_VOLUME_TEXT = "DiskVolumeNameText";
    public static final String CHILD_DISK_VOLUME_TEXTFIELD =
        "DiskVolumeNameTextField";

    // adding in textfield for device path and hostname
    public static final String CHILD_DISK_DEVICE_TEXT = "DiskDeviceText";
    public static final String CHILD_DISK_DEVICE_TEXTFIELD =
        "DiskDeviceTextField";
    public static final String CHILD_MEDTYPE_TEXT = "MedTypeText";
    public static final String CHILD_TAPE_DROPDOWN = "TapeDropDown";
    public static final String CHILD_VSNPOOL_TEXT = "VSNPoolText";
    public static final String CHILD_VSNPOOL_DROPDOWN = "VSNPoolDropDownMenu";
    public static final String CHILD_START_TEXT = "StartText";
    public static final String CHILD_START_TEXTFIELD = "StartTextField";
    public static final String CHILD_END_TEXT = "EndText";
    public static final String CHILD_END_TEXTFIELD = "EndTextField";
    public static final String CHILD_RANGE_TEXT = "RangeText";
    public static final String CHILD_RANGE_TEXTFIELD = "RangeTextField";
    public static final String CHILD_RANGE_INSTR_TEXT = "RangeInstrText";
    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_SPECIFY_VSN_TEXT = "SpecifyVSNLabel";
    public static final String CHILD_RESERVE_TEXT = "ReserveText";
    public static final String CHILD_RESERVE_DROPDOWN = "rmAttributes";
    public static final String CHILD_RESERVE_POLICY = "rmPolicy";
    public static final String CHILD_RESERVE_FS = "rmFS";
    public static final String CREATE_PATH_LABEL = "createPathLabel";
    public static final String CREATE_PATH = "createPath";
    public static final String CHILD_ERROR = "errorOccur";
    public static final String CHILD_REQUIRED_TEXT = "RequiredText";
    public static final String CHILD_HELP_TEXT = "HelpText";

    private boolean error = false;
    private boolean previousError = false;

    // stage attributes options
    private static OptionList radioOptions1 =
        new OptionList(
            new String [] {""},
            new String [] {NewCopyWizardImpl.DISK});

    private static OptionList radioOptions2 =
        new OptionList(
            new String [] {""},
            new String [] {NewCopyWizardImpl.TAPE});

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewCopyMediaParameters(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewCopyMediaParameters(View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");

        registerChild(CHILD_HELP_TEXT, CCStaticTextField.class);
        registerChild(CHILD_ARCHIVE_AGE_TEXT, CCLabel.class);
        registerChild(CHILD_ARCHIVE_AGE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_ARCHIVE_AGE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_ARCHIVE_MEDTYPE_TEXT, CCLabel.class);
        registerChild(CHILD_MEDTYPE_RADIO1, CCRadioButton.class);
        registerChild(CHILD_MEDTYPE_RADIO2, CCRadioButton.class);
        registerChild(CHILD_MEDTYPE_TEXT, CCLabel.class);
        registerChild(CHILD_TAPE_TEXT, CCStaticTextField.class);
        registerChild(CHILD_DISK_TEXT, CCStaticTextField.class);
        registerChild(CHILD_DISK_VOLUME_TEXT, CCLabel.class);
        registerChild(CHILD_DISK_VOLUME_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_DISK_DEVICE_TEXT, CCLabel.class);
        registerChild(CHILD_DISK_DEVICE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_TAPE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_VSNPOOL_TEXT, CCLabel.class);
        registerChild(CHILD_VSNPOOL_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_START_TEXT, CCLabel.class);
        registerChild(CHILD_START_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_END_TEXT, CCLabel.class);
        registerChild(CHILD_END_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_RANGE_TEXT, CCLabel.class);
        registerChild(CHILD_RANGE_INSTR_TEXT, CCStaticTextField.class);
        registerChild(CHILD_RANGE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_SPECIFY_VSN_TEXT, CCLabel.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        registerChild(CHILD_REQUIRED_TEXT, CCLabel.class);
        registerChild(CHILD_RESERVE_TEXT, CCLabel.class);
        registerChild(CHILD_RESERVE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_RESERVE_POLICY, CCCheckBox.class);
        registerChild(CHILD_RESERVE_FS, CCCheckBox.class);
        registerChild(CREATE_PATH_LABEL, CCLabel.class);
        registerChild(CREATE_PATH, CCCheckBox.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.equals(CHILD_ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(CHILD_ERROR)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_ARCHIVE_AGE_TEXT) ||
                   name.equals(CHILD_ARCHIVE_MEDTYPE_TEXT) ||
                   name.equals(CHILD_DISK_VOLUME_TEXT) ||
                   name.equals(CHILD_DISK_DEVICE_TEXT) ||
                   name.equals(CHILD_VSNPOOL_TEXT) ||
                   name.equals(CHILD_START_TEXT) ||
                   name.equals(CHILD_END_TEXT) ||
                   name.equals(CHILD_RANGE_TEXT) ||
                   name.equals(CHILD_SPECIFY_VSN_TEXT) ||
                   name.equals(CHILD_REQUIRED_TEXT) ||
                   name.equals(CHILD_MEDTYPE_TEXT) ||
                   name.equals(CHILD_RESERVE_TEXT) ||
                   name.equals(CREATE_PATH_LABEL))  {
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ARCHIVE_AGE_TEXTFIELD) ||
                   name.equals(CHILD_DISK_VOLUME_TEXTFIELD) ||
                   name.equals(CHILD_DISK_DEVICE_TEXTFIELD) ||
                   name.equals(CHILD_START_TEXTFIELD) ||
                   name.equals(CHILD_END_TEXTFIELD) ||
                   name.equals(CHILD_RANGE_TEXTFIELD)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(CHILD_ARCHIVE_AGE_DROPDOWN)) {
            CCDropDownMenu child =  new CCDropDownMenu(this, name, null);
            OptionList archiveAgeOptions =
                new OptionList(SelectableGroupHelper.Time.labels,
                               SelectableGroupHelper.Time.values);
            child.setOptions(archiveAgeOptions);
            return child;
        } else if (name.equals(CHILD_RESERVE_DROPDOWN)) {
            CCDropDownMenu child =  new CCDropDownMenu(this, name, null);
            OptionList reserveOptions =
                new OptionList(
                    SelectableGroupHelper.ReservationMethod.labels,
                    SelectableGroupHelper.ReservationMethod.values);
            child.setOptions(reserveOptions);
            return child;
        } else if (name.equals(CHILD_RESERVE_POLICY) ||
                   name.equals(CHILD_RESERVE_FS) ||
                   name.equals(CREATE_PATH)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(CHILD_MEDTYPE_RADIO1)) {
            CCRadioButton child =  new CCRadioButton(this, name, null);
            child.setOptions(radioOptions1);
            return child;
        } else if (name.equals(CHILD_MEDTYPE_RADIO2)) {
            CCRadioButton child =
                new CCRadioButton(this, CHILD_MEDTYPE_RADIO1, null);
            child.setOptions(radioOptions2);
            return child;

        } else if (name.equals(CHILD_TAPE_TEXT) ||
            name.equals(CHILD_DISK_TEXT) ||
            name.equals(CHILD_RANGE_INSTR_TEXT) ||
            name.equals(CHILD_HELP_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_TAPE_DROPDOWN) ||
            name.equals(CHILD_VSNPOOL_DROPDOWN)) {
            return new CCDropDownMenu(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child [" + name + "]");
        }
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");
        String url = null;
        if (!previousError) {
            url = "/jsp/archive/NewCopyMediaParameters.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event)
        throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        populateDropDownMenus(wm);

        if (!error) {
            preSelectRadioButton(wm);
        }

        prePopulateReserveFields(wm);

        // TODO: enable this when you are ready implementing wizard control
        // showWizardValidationError(wm);
        TraceUtil.trace3("Exiting");
    }

    private void preSelectRadioButton(SamWizardModel wm) {
        String radioSelection =
            (String) wm.getValue(NewCopyWizardImpl.MEDIA_TYPE);

        // No selection yet
        if (radioSelection != null
            && radioSelection.length() != 0
            && radioSelection.equals(NewCopyWizardImpl.DISK)) {
            enableDiskComponents();
        } else {
            enableTapeComponents();
        }
    }

    private void enableDiskComponents() {
        ((CCRadioButton) getChild(
            CHILD_MEDTYPE_RADIO1)).setValue(NewCopyWizardImpl.DISK);
        ((CCTextField)
            getChild(CHILD_DISK_VOLUME_TEXTFIELD)).setDisabled(false);
        ((CCTextField)
            getChild(CHILD_DISK_DEVICE_TEXTFIELD)).setDisabled(false);
    }

    private void enableTapeComponents() {
        ((CCRadioButton) getChild(
            CHILD_MEDTYPE_RADIO1)).setValue(NewCopyWizardImpl.TAPE);
        ((CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN)).setDisabled(false);
        ((CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN)).setDisabled(false);
        ((CCTextField) getChild(CHILD_START_TEXTFIELD)).setDisabled(false);
        ((CCTextField) getChild(CHILD_END_TEXTFIELD)).setDisabled(false);
        ((CCTextField) getChild(CHILD_RANGE_TEXTFIELD)).setDisabled(false);
    }

    private void populateDropDownMenus(SamWizardModel wm) {
        CCDropDownMenu mediaDropDownMenu =
            (CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN);

        CCDropDownMenu vsnDropDownMenu =
            (CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN);

        CCDropDownMenu archiveAgeDropDownMenu =
            (CCDropDownMenu) getChild(CHILD_ARCHIVE_AGE_DROPDOWN);

        SamQFSSystemModel sysModel = null;
        try {
            sysModel = SamUtil.getModel(getServerName());

            // set archive age unit to minute if nothing is defined
            String ageUnit = (String) wm.getValue(
                CHILD_ARCHIVE_AGE_DROPDOWN);

            if (ageUnit == null ||
                SelectableGroupHelper.NOVAL.equals(ageUnit)) {
                archiveAgeDropDownMenu.setValue(
                    new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
            }

            // populate the media dropdown
             int [] mTypes  =
                sysModel.getSamQFSSystemMediaManager().getAvailableMediaTypes();
            String [] mediaLabels = new String [1];
            String [] mediaValues = new String [1];

            if (mTypes != null) {
                mediaLabels = new String [mTypes.length + 1];
                mediaValues = new String [mTypes.length + 1];
            }

            mediaLabels[0] = "--";
            mediaValues[0] = "--";

            int k = 1;
            if (mTypes != null) {
                for (int j = 0; j < mTypes.length; j++) {
                    mediaLabels[k] =
                        SamUtil.getMediaTypeString(mTypes[j]);
                    mediaValues[k] =
                        SamUtil.getMediaTypeString(mTypes[j]);
                    k++;
                }
            }

            OptionList mediaOptionList =
                new OptionList(mediaLabels, mediaValues);
            mediaDropDownMenu.setOptions(mediaOptionList);

            // populate the VSN Pools dropdown
            VSNPool [] vsnPools = null;
            try {
                vsnPools = sysModel.
                    getSamQFSSystemArchiveManager().getAllVSNPools();
            } catch (Exception e) {
                // Exception is thrown if catalog daemon is not running
                // That is ok
            }
            if (vsnPools == null) {
                vsnPools = new VSNPool[0];
            }

            String [] vsnLabels = new String [vsnPools.length + 1];
            String [] vsnValues = new String [vsnPools.length + 1];

            vsnLabels[0] = "--";
            vsnValues[0] = "--";
            int m = 1;
            for (int j = 0; j < vsnPools.length; j++) {
                vsnLabels[m] = vsnPools[j].getPoolName();
                vsnValues[m] = vsnPools[j].getPoolName();
                m++;
            }

            OptionList vsnOptionList = new OptionList(vsnLabels, vsnValues);
            vsnDropDownMenu.setOptions(vsnOptionList);

        } catch (SamFSException smfex) {
            error = true;

            ((CCHiddenField)
                getChild(CHILD_ERROR)).setValue(Constants.Wizard.EXCEPTION);

            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginDisplay",
                "Failed to populate copy media parameters",
                getServerName());

            SamUtil.setErrorAlert(
                this,
                NewCopyMediaParameters.CHILD_ALERT,
                "NewArchivePolWizard.page3.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());
        }
    }

    private void prePopulateReserveFields(SamWizardModel wm) {
        ArchiveCopy copy = ((ArchiveCopyGUIWrapper)
            wm.getValue(NewCopyWizardImpl.COPY_GUIWRAPPER)).getArchiveCopy();

        // Reserve
        int reserveMethod = copy.getReservationMethod();
        ReservationMethodHelper rmh = new ReservationMethodHelper();
        rmh.setValue(reserveMethod);

        // set the drop down list
        CCDropDownMenu rmAttributes =
            (CCDropDownMenu) getChild(CHILD_RESERVE_DROPDOWN);
        if (rmh.getAttributes() == 0) {
            rmAttributes.setValue(SelectableGroupHelper.NOVAL);
        } else {
            rmAttributes.setValue(Integer.toString(rmh.getAttributes()));
        }

        // set the policy check box
        CCCheckBox rmPolicy = (CCCheckBox)getChild(CHILD_RESERVE_POLICY);
        String set = rmh.getSet() == 0 ? "false" : "true";
        rmPolicy.setValue(set);

        // set the fs check box
        CCCheckBox rmFS = (CCCheckBox)getChild(CHILD_RESERVE_FS);
        String fs = rmh.getFS() == 0 ? "false" : "true";
        rmFS.setValue(fs);
    }

    private void showWizardValidationError(SamWizardModel wm) {
        String err = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);

        if (err != null && err.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs = (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "NewArchivePolWizard.error.carryover";
            previousError = true;
            String errorDetails =
                (String) wm.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wm.getValue(Constants.Wizard.ERROR_SUMMARY);

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    previousError = false;
                } else {
                    previousError = true;
                }
            }

            if (previousError) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    code,
                    msgs,
                    getServerName());
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
