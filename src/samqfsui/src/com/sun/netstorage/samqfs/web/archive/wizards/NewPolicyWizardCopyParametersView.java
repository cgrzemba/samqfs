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

// ident	$Id: NewPolicyWizardCopyParametersView.java,v 1.18 2008/12/16 00:12:08 am143972 Exp $


package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.ReservationMethodHelper;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
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
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.util.HashMap;

/**
 * A ContainerView object for the pagelet for Copy Parameters Page.
 *
 */
public class NewPolicyWizardCopyParametersView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewPolicyWizardCopyParametersView";

    // Child view names (i.e. display fields).
    public static final String CHILD_ARCHIVE_AGE_TEXT = "ArchiveAgeText";
    public static final String CHILD_ARCHIVE_AGE_TEXTFIELD =
        "ArchiveAgeTextField";
    public static final String CHILD_ARCHIVE_AGE_DROPDOWN =
        "ArchiveAgeDropDown";
    public static final String CHILD_ARCHIVE_MEDTYPE_TEXT =
        "ArchiveMediaTypeText";
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
    public static final String CHILD_SPECIFY_VSN_TEXT = "SpecifyVSN";
    public static final String CHILD_ERROR = "errorOccur";
    public static final String CHILD_REQUIRED_TEXT = "RequiredText";
    public static final String CHILD_HELP_TEXT = "HelpText";
    public static final String CHILD_RESERVE_TEXT = "ReserveText";
    public static final String CHILD_RESERVE_DROPDOWN = "rmAttributes";
    public static final String CHILD_RESERVE_POLICY = "rmPolicy";
    public static final String CHILD_RESERVE_FS = "rmFS";

    private boolean previousError;
    private boolean error;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewPolicyWizardCopyParametersView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewPolicyWizardCopyParametersView(
        View parent, Model model, String name) {
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
        registerChild(CHILD_MEDTYPE_TEXT, CCLabel.class);
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
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_SPECIFY_VSN_TEXT, CCLabel.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        registerChild(CHILD_REQUIRED_TEXT, CCLabel.class);
        registerChild(CHILD_RESERVE_TEXT, CCLabel.class);
        registerChild(CHILD_RESERVE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_RESERVE_POLICY, CCCheckBox.class);
        registerChild(CHILD_RESERVE_FS, CCCheckBox.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        View child = null;
        if (name.equals(CHILD_ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(CHILD_ERROR)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_ARCHIVE_AGE_TEXT) ||
                   name.equals(CHILD_ARCHIVE_MEDTYPE_TEXT) ||
                   name.equals(CHILD_VSNPOOL_TEXT) ||
                   name.equals(CHILD_START_TEXT) ||
                   name.equals(CHILD_END_TEXT) ||
                   name.equals(CHILD_RANGE_TEXT) ||
                   name.equals(CHILD_SPECIFY_VSN_TEXT)||
                   name.equals(CHILD_REQUIRED_TEXT)||
                   name.equals(CHILD_MEDTYPE_TEXT)||
                   name.equals(CHILD_RESERVE_TEXT)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ARCHIVE_AGE_TEXTFIELD) ||
                   name.equals(CHILD_START_TEXTFIELD) ||
                   name.equals(CHILD_END_TEXTFIELD) ||
                   name.equals(CHILD_RANGE_TEXTFIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_RANGE_INSTR_TEXT) ||
                   name.equals(CHILD_HELP_TEXT)) {
            child = new CCStaticTextField(this, name, null);
        } else if (name.equals(CHILD_TAPE_DROPDOWN) ||
                   name.equals(CHILD_VSNPOOL_DROPDOWN)) {
            child = new CCDropDownMenu(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline myChild =  new CCAlertInline(this, name, null);
            myChild.setValue(CCAlertInline.TYPE_INFO);
            child = myChild;
        } else if (name.equals(CHILD_ARCHIVE_AGE_DROPDOWN)) {
            CCDropDownMenu myChild =  new CCDropDownMenu(this, name, null);
            OptionList archiveAgeOptions =
                new OptionList(SelectableGroupHelper.Time.labels,
                               SelectableGroupHelper.Time.values);
            myChild.setOptions(archiveAgeOptions);
            child = myChild;
        } else if (name.equals(CHILD_RESERVE_DROPDOWN)) {
            CCDropDownMenu myChild =  new CCDropDownMenu(this, name, null);
            OptionList reserveOptions =
                new OptionList(SelectableGroupHelper.ReservationMethod.labels,
                               SelectableGroupHelper.ReservationMethod.values);
            myChild.setOptions(reserveOptions);
            child = myChild;

        } else if (name.equals(CHILD_RESERVE_POLICY) ||
                   name.equals(CHILD_RESERVE_FS)) {
            child = new CCCheckBox(this, name, "true", "false", false);

        } else {
            throw new IllegalArgumentException("Invalid child [" + name + "]");
        }

        return child;
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
            url = "/jsp/archive/wizards/NewPolicyWizardCopyParameters.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        populateDropDownMenus(wizardModel);

        if (!error && !showPreviousError(wizardModel)) {
            try {
                prePopulateFields(wizardModel);
            } catch (SamFSException samEx) {
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "beginDisplay",
                    "Failed to pre-populate input fields",
                    getServerName());
                SamUtil.setErrorAlert(this,
                    NewPolicyWizardDiskCopyOptionView.CHILD_ALERT,
                    "NewArchivePolWizard.page4.failedPopulate",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            }
        }

        prePopulateReserveFields(wizardModel);

        TraceUtil.trace3("Exiting");
    }

    private void prePopulateReserveFields(SamWizardModel wizardModel) {
        TraceUtil.trace3("Entering");

        // Grab the HashMap that contains copy information from wizardModel
        HashMap copyNumberHashMap =
            (HashMap) wizardModel.getValue(NewPolicyWizardImpl.COPY_HASHMAP);

        // Grab the copy number that this page represents,
        // the copy number is assigned in getPageClass()
        Integer copyNumber =
            (Integer) wizardModel.getValue(Constants.Wizard.COPY_NUMBER);

        // Retrieve the CopyInfo data structure from the HashMap
        CopyInfo info =
            (CopyInfo) copyNumberHashMap.get(copyNumber);
        ArchiveCopyGUIWrapper myWrapper = null;

        if (info == null) {
            SamUtil.doPrint("HashMap contains null CopyInfo object!");
            SamUtil.doPrint(
                "Getting default wrapper, create CopyInfo and insert to map");
            try {
                ArchiveCopyGUIWrapper defaultWrapper = getDefaultWrapper();
                myWrapper = defaultWrapper;
                copyNumberHashMap.put(
                    copyNumber,
                    new CopyInfo(
                        NewPolicyWizardImpl.TAPE_BASE, defaultWrapper));
            } catch (SamFSException samEx) {
                TraceUtil.trace1("Failed to retrieve default wrapper!");
                return;
            }

            wizardModel.setValue(
                NewPolicyWizardImpl.COPY_HASHMAP, copyNumberHashMap);
        } else {
            myWrapper = info.getCopyWrapper();
        }

        // Grab the wrapper from the CopyInfo datastructure, then grab the
        // ArchiveCopy object to start prepopulating information
        ArchiveCopy copy = myWrapper.getArchiveCopy();

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

    private boolean showPreviousError(SamWizardModel wizardModel) {
        // see if any errors occcurred from previous page
        String error =
            (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);

        if (error != null && error.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            previousError = true;
            String msgs =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
            SamUtil.setErrorAlert(this,
                NewPolicyWizardCopyParametersView.CHILD_ALERT,
                "NewArchivePolWizard.error.carryover",
                code,
                msgs,
                getServerName());
            return true;
        } else {
            return false;
        }
    }

    private void enableTapeComponents() {
        ((CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN)).setDisabled(false);
        ((CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN)).setDisabled(false);
        ((CCTextField) getChild(CHILD_START_TEXTFIELD)).setDisabled(false);
        ((CCTextField) getChild(CHILD_END_TEXTFIELD)).setDisabled(false);
        ((CCTextField) getChild(CHILD_RANGE_TEXTFIELD)).setDisabled(false);
        ((CCCheckBox) getChild(CHILD_RESERVE_FS)).setDisabled(false);
        ((CCCheckBox) getChild(CHILD_RESERVE_POLICY)).setDisabled(false);
        ((CCDropDownMenu) getChild(CHILD_RESERVE_DROPDOWN)).setDisabled(false);
    }

    private void populateDropDownMenus(SamWizardModel wizardModel) {
        CCDropDownMenu mediaDropDownMenu =
            (CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN);

        CCDropDownMenu vsnDropDownMenu =
            (CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN);

        SamQFSSystemModel sysModel = null;
        try {
            sysModel = SamUtil.getModel(getServerName());

            // populate the media dropdown
            int [] mTypes  =
                sysModel.getSamQFSSystemMediaManager().getAvailableMediaTypes();
            String [] mediaLabels = new String [1];
            String [] mediaValues = new String [1];

            if (mTypes != null) {
                mediaLabels = new String [mTypes.length + 1];
                mediaValues = new String [mTypes.length + 1];
            }

            mediaLabels[0] = SelectableGroupHelper.NOVAL_LABEL;
            mediaValues[0] = SelectableGroupHelper.NOVAL;

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

            vsnLabels[0] = SelectableGroupHelper.NOVAL_LABEL;
            vsnValues[0] = SelectableGroupHelper.NOVAL;

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
                NewPolicyWizardCopyParametersView.CHILD_ALERT,
                "NewArchivePolWizard.page3.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());
        }
    }

    /**
     * To pre-populate all fields based on the wrapper information.
     * NOTE: We cannot rely on the wizard model to pre-populate this
     * page for us because this page is a part of the cycle,
     * and this page has to be re-used again and again.
     * @param wizardModel - The wizard Model
     */
    private void prePopulateFields(SamWizardModel wizardModel)
        throws SamFSException {
        TraceUtil.trace3("Entering");

        // Retrieve the CopyInfo data structure from the HashMap
        CopyInfo info = getCopyInfoObject(wizardModel);

        if (info == null) {
            resetAllFieldsToBlank();
            return;
        }

        // Grab the wrapper from the CopyInfo datastructure, then grab the
        // ArchiveCopy object to start prepopulating information
        ArchiveCopyGUIWrapper myWrapper = info.getCopyWrapper();
        ArchivePolCriteriaCopy criteriaCopy =
            myWrapper.getArchivePolCriteriaCopy();
        ArchiveCopy archiveCopy = myWrapper.getArchiveCopy();
        ArchiveVSNMap archiveVSNMap = archiveCopy.getArchiveVSNMap();

        // Archive Age
        long archiveAge = criteriaCopy.getArchiveAge();
        if (archiveAge != -1) {
            ((CCTextField) getChild(CHILD_ARCHIVE_AGE_TEXTFIELD)).
                setValue(Long.toString(archiveAge));
        } else {
            ((CCTextField) getChild(CHILD_ARCHIVE_AGE_TEXTFIELD)).
                setValue("");
        }
        int archiveAgeUnit = criteriaCopy.getArchiveAgeUnit();
        if (archiveAgeUnit != -1) {
            ((CCDropDownMenu) getChild(CHILD_ARCHIVE_AGE_DROPDOWN)).
                setValue(Integer.toString(archiveAgeUnit));
        } else {
            ((CCDropDownMenu) getChild(CHILD_ARCHIVE_AGE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        // Set the radio button if the copy is tape or disk based
        String type = info.getCopyType();

        // prepopulate the two dropdowns and three textfields
        String mediaType =
            SamUtil.getMediaTypeString(archiveVSNMap.getArchiveMediaType());
        if (mediaType != null && mediaType.length() != 0) {
            ((CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN)).
                setValue(mediaType);
        } else {
            ((CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN)).
                setValue(SelectableGroupHelper.NOVAL);
        }

        String poolName = archiveVSNMap.getPoolExpression();
        if (poolName != null && poolName.length() != 0) {
            ((CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN)).
                setValue(poolName);
        } else {
            ((CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN)).
                setValue(SelectableGroupHelper.NOVAL);
        }

        String startText = archiveVSNMap.getMapExpressionStartVSN();
        if (startText != null && startText.length() != 0) {
            ((CCTextField) getChild(CHILD_START_TEXTFIELD)).
                setValue(startText);
        } else {
            ((CCTextField) getChild(CHILD_START_TEXTFIELD)).setValue("");
        }

        String endText = archiveVSNMap.getMapExpressionEndVSN();
        if (endText != null && endText.length() != 0) {
            ((CCTextField) getChild(CHILD_END_TEXTFIELD)).
                setValue(endText);
        } else {
            ((CCTextField) getChild(CHILD_END_TEXTFIELD)).setValue("");
        }

        String rangeText = archiveVSNMap.getMapExpression();
        if (rangeText != null && rangeText.length() != 0) {
            ((CCTextField) getChild(CHILD_RANGE_TEXTFIELD)).
                setValue(rangeText);
        } else {
            ((CCTextField) getChild(CHILD_RANGE_TEXTFIELD)).setValue("");
        }

        TraceUtil.trace3("Exiting");
    }

    private void resetAllFieldsToBlank() {
        ((CCTextField) getChild(CHILD_ARCHIVE_AGE_TEXTFIELD)).setValue("");
        ((CCDropDownMenu) getChild(CHILD_ARCHIVE_AGE_DROPDOWN)).
            setValue(new Integer(SamQFSSystemModel.TIME_MINUTE).toString());

        toggleOptionFields(false);
    }

    private void resetAllTapeOptionFields() {
        ((CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN)).
            setValue(SelectableGroupHelper.NOVAL);
        ((CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN)).
            setValue(SelectableGroupHelper.NOVAL);
        ((CCTextField) getChild(CHILD_START_TEXTFIELD)).setValue("");
        ((CCTextField) getChild(CHILD_END_TEXTFIELD)).setValue("");
        ((CCTextField) getChild(CHILD_RANGE_TEXTFIELD)).setValue("");
    }

    private void toggleOptionFields(boolean turnDiskOn) {
        if (turnDiskOn) {
            ((CCDropDownMenu) getChild(CHILD_TAPE_DROPDOWN)).
                setDisabled(true);
            ((CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN)).
                setDisabled(true);
            ((CCTextField) getChild(CHILD_START_TEXTFIELD)).setDisabled(true);
            ((CCTextField) getChild(CHILD_END_TEXTFIELD)).setDisabled(true);
            ((CCTextField) getChild(CHILD_RANGE_TEXTFIELD)).setDisabled(true);
            ((CCDropDownMenu) getChild(CHILD_RESERVE_DROPDOWN)).
                setDisabled(true);
            ((CCCheckBox) getChild(CHILD_RESERVE_FS)).setDisabled(true);
            ((CCCheckBox) getChild(CHILD_RESERVE_POLICY)).setDisabled(true);
        } else {
            ((CCDropDownMenu)
                getChild(CHILD_TAPE_DROPDOWN)).setDisabled(false);
            ((CCDropDownMenu) getChild(CHILD_VSNPOOL_DROPDOWN)).
                setDisabled(false);
            ((CCTextField) getChild(CHILD_START_TEXTFIELD)).setDisabled(false);
            ((CCTextField) getChild(CHILD_END_TEXTFIELD)).setDisabled(false);
            ((CCTextField) getChild(CHILD_RANGE_TEXTFIELD)).setDisabled(false);
            ((CCDropDownMenu) getChild(CHILD_RESERVE_DROPDOWN)).
                setDisabled(false);
            ((CCCheckBox) getChild(CHILD_RESERVE_FS)).setDisabled(false);
            ((CCCheckBox) getChild(CHILD_RESERVE_POLICY)).setDisabled(false);
        }
    }

    private CopyInfo getCopyInfoObject(SamWizardModel wizardModel) {
        // Grab the HashMap that contains copy information from wizardModel
        HashMap copyNumberHashMap =
            (HashMap) wizardModel.getValue(NewPolicyWizardImpl.COPY_HASHMAP);

        // Grab the copy number that this page represents,
        // the copy number is assigned in getPageClass()
        Integer copyNumber =
            (Integer) wizardModel.getValue(Constants.Wizard.COPY_NUMBER);

        // Retrieve the CopyInfo data structure from the HashMap
        CopyInfo info = (CopyInfo) copyNumberHashMap.get(copyNumber);

        return info;
    }

    /**
     * To retrieve the default wrapper from the back-end
     * @return Default Wrapper object from the back-end
     */
    private ArchiveCopyGUIWrapper getDefaultWrapper() throws SamFSException {
        SamUtil.doPrint("getDefaultWrapper called");
        ArchiveCopyGUIWrapper wrapper =
            PolicyUtil.getArchiveManager(
                getServerName()).getArchiveCopyGUIWrapper();
        if (wrapper == null) {
            throw new SamFSException(null, -2023);
        } else {
            return wrapper;
        }
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
