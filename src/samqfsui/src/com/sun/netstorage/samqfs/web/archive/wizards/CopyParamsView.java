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

// ident	$Id: CopyParamsView.java,v 1.2 2008/05/16 18:38:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardImpl;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.wizard.WizardEvent;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.util.HashMap;

public class CopyParamsView extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "CopyParamsView";

    // View children
    public static final String ALERT = "Alert";
    public static final String ARCHIVE_AGE = "archiveAge";
    public static final String LABEL_ARCHIVE_AGE = "LabelArchiveAge";
    public static final String ARCHIVE_AGE_UNIT = "archiveAgeUnit";
    public static final String STATIC_TEXT = "StaticText";
    public static final String LABEL_ARCHIVE_MEDIA = "LabelArchiveMedia";
    public static final String ARCHIVE_MEDIA_MENU = "ArchiveMediaMenu";
    public static final String RESERVED = "Reserved";
    public static final String VOLUME_INSTRUCTION = "VolumeInstruction";

    // Menu definitions
    public static final String MENU_CREATE_POOL = "CREATE_POOL";
    public static final String MENU_CREATE_EXP = "CREATE_EXP";
    public static final String MENU_ANY_VOL = "##ANY##";

    /**
     * View Constructor
     * @param parent
     * @param model
     */
    public CopyParamsView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    /**
     * Main View Constructor
     * @param parent
     * @param model
     * @param pageName
     */
    public CopyParamsView(View parent, Model model, String pageName) {
        super(parent, pageName);

        setDefaultModel(model);
        registerChildren();
    }

    /**
     * Register view children
     */
    public void registerChildren() {
        registerChild(ALERT, CCAlertInline.class);
        registerChild(ARCHIVE_AGE, CCTextField.class);
        registerChild(LABEL_ARCHIVE_AGE, CCLabel.class);
        registerChild(ARCHIVE_AGE_UNIT, CCDropDownMenu.class);
        registerChild(STATIC_TEXT, CCStaticTextField.class);
        registerChild(LABEL_ARCHIVE_MEDIA, CCLabel.class);
        registerChild(ARCHIVE_MEDIA_MENU, CCDropDownMenu.class);
        registerChild(RESERVED, CCCheckBox.class);
        registerChild(VOLUME_INSTRUCTION, CCStaticTextField.class);
    }

    /**
     * Create View Children
     * @param name
     * @return
     */
    public View createChild(String name) {
        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(ARCHIVE_AGE_UNIT)
            || name.equals(ARCHIVE_MEDIA_MENU)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.startsWith("Label")) {
            return new CCLabel(this, name, null);
        } else if (name.equals(ARCHIVE_AGE)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(STATIC_TEXT)
            || name.equals(VOLUME_INSTRUCTION)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(RESERVED)) {
            return new CCCheckBox(this, name,
                Boolean.toString(true), Boolean.toString(false), false);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    private void populateDropDownMenus() throws SamFSException {

        // archive age units
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(ARCHIVE_AGE_UNIT);
        dropDown.setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                           SelectableGroupHelper.Time.values));

        String serverName = getServerName();

        // media types -- including disk
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        int [] mediaTypes = sysModel.getSamQFSSystemMediaManager().
            getAvailableArchiveMediaTypes();

        // vsn pools
        VSNPool [] pools = PolicyUtil.getAllVSNPools(serverName);

        String [] labels = new String[pools.length + mediaTypes.length + 2];
        String [] values = new String[pools.length + mediaTypes.length + 2];

        int counter = 0;
        for (int i = 0; i < mediaTypes.length; i++, counter++) {
            labels[counter] =
                SamUtil.getResourceString(
                    "archiving.copy.mediaparam.anyvol",
                    SamUtil.getMediaTypeString(mediaTypes[i]));
            values[counter] =
                MENU_ANY_VOL + "," + Integer.toString(mediaTypes[i]);
        }

        for (int i = 0; i < pools.length; i++, counter++) {
            labels[counter] =
                pools[i].getPoolName() + " (" +
                SamUtil.getMediaTypeString(pools[i].getMediaType()) + ")";
            values[counter] =
                pools[i].getPoolName() + "," +
                Integer.toString(pools[i].getMediaType());
        }

        labels[counter] =
            SamUtil.getResourceString(
                "archiving.copy.mediaparam.createpool");
        values[counter] = MENU_CREATE_POOL;

        counter++;

        labels[counter] =
            SamUtil.getResourceString(
                "archiving.copy.mediaparam.createexp");
        values[counter] = MENU_CREATE_EXP;

        dropDown = (CCDropDownMenu)getChild(ARCHIVE_MEDIA_MENU);
        dropDown.setOptions(new OptionList(labels, values));
        dropDown.setLabelForNoneSelected(
            "archiving.copy.mediaparam.noneselect");
    }

    /**
     * this method must start by checking the wizard model for any preset
     * values
     */
    private void initializeDisplayFields(
        CopyInfo copyInfo, boolean isCopyWizard) throws SamFSException {

        // Grab the wrapper from the CopyInfo datastructure, then grab the
        // ArchiveCopy object to start prepopulating information
        ArchiveCopyGUIWrapper myWrapper = null;
        ArchivePolCriteriaCopy criteriaCopy = null;
        ArchiveCopy archiveCopy = null;
        ArchiveVSNMap archiveVSNMap = null;
        long archiveAge = -1;
        int archiveAgeUnit = -1;
        String poolExpression = null;
        int mediaType = -1;

        if (copyInfo != null) {
            myWrapper = copyInfo.getCopyWrapper();
            criteriaCopy = myWrapper.getArchivePolCriteriaCopy();
            archiveCopy = myWrapper.getArchiveCopy();
            archiveVSNMap = archiveCopy.getArchiveVSNMap();

            archiveAge = criteriaCopy.getArchiveAge();
            archiveAgeUnit = criteriaCopy.getArchiveAgeUnit();
            mediaType  = archiveVSNMap.getArchiveMediaType();
            poolExpression = archiveVSNMap.getPoolExpression();
        }

        // Only used if this view is being used in NewCopyWizard
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // Set warning for mix media setup if copy number is 1
        if (getCopyNumber() == 1 || !isCopyWizard) {
            ((CCStaticTextField) getChild(VOLUME_INSTRUCTION)).setValue(
                "archiving.copy.mediaparam.instruction.2.firstcopy");
        } else {
            ((CCStaticTextField) getChild(VOLUME_INSTRUCTION)).setValue(
                "archiving.copy.mediaparam.instruction.2");
        }

        // archive age : default 4 minutes
        if (archiveAge == -1) {
            archiveAge = SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE;
        }

        if (archiveAgeUnit == -1) {
            archiveAgeUnit =
                SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT;
        }

        if (isCopyWizard) {
            // Prepopulate Archive Age if it's not set yet
            String archiveAgeStr = (String) wizardModel.getValue(ARCHIVE_AGE);
            if (archiveAgeStr == null || archiveAgeStr.length() == 0) {
                ((CCTextField) getChild(ARCHIVE_AGE)).
                    setValue(Long.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE));
                ((CCDropDownMenu) getChild(ARCHIVE_AGE_UNIT)).
                    setValue(Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT));
            }
        } else {
            ((CCTextField) getChild(
                ARCHIVE_AGE)).setValue(Long.toString(archiveAge));
            ((CCDropDownMenu) getChild(
                ARCHIVE_AGE_UNIT)).setValue(Integer.toString(archiveAgeUnit));
            ArchiveCopy copy = myWrapper.getArchiveCopy();
            // Reserve
            int reserveMethod = copy.getReservationMethod();
            // TODO: for non-copy wizard cases, make sure reserve check box
            // state is maintained for each copy
        }
    }

    // begin pagelet display
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        String serverName = getServerName();

        /**
         * This view is being used by both the NewArchivePolicyWizard and the
         * NewCopyWizard, and the CreateFSWizard.  In order to populate the
         * fields in this page correctly, we have to pass in the CopyInfo object
         * if this class is being used by the NewArchivePolicyWizard &
         * CreateFSWizard  This is because this page may be used by multiple
         * copies and thus we need to be careful on presenting the values that
         * users may have already entered.
         *
         * Simply passing null for CopyInfo parameter if this view is being
         * used by the NewCopyWizard.
         */

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        HashMap copyNumberHashMap =
            (HashMap) wizardModel.getValue(NewPolicyWizardImpl.COPY_HASHMAP);
        CopyInfo copyInfo =
            copyNumberHashMap == null ?
                null :getCopyInfoObject(copyNumberHashMap);

        try {
            populateDropDownMenus();

            // if copyNumberHashMap is null, this view is being used in
            // New Copy Wizard
            initializeDisplayFields(copyInfo, copyNumberHashMap == null);

        } catch (SamFSException sfe) {
            TraceUtil.trace1("Failed to populate copy param step!", sfe);
            SamUtil.processException(
                sfe,
                getClass(),
                "beingDisplay",
                "Error will initializing page",
                serverName);
            SamUtil.setErrorAlert(
                this,
                ALERT,
                "an exception occurred while populating dropdowns",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);
        }

        // Set label to red if error is detected
        setErrorLabel((SamWizardModel) getDefaultModel(), copyInfo != null);
    }

    /**
     * Validate user input
     * @return
     */
    public boolean validate(WizardEvent event) {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        boolean error = false;

        // Validate copy time
        String archiveAge = (String) wizardModel.getValue(ARCHIVE_AGE);
        archiveAge = archiveAge == null ? "" : archiveAge.trim();

        // verify that age is provided
        if (archiveAge.length() == 0) {
            error = true;
        } else {
            // verify age is numeric
            long age = -1;
            try {
                age = Long.parseLong(archiveAge);
                if (age < 0) {
                    error = true;
                }
            } catch (NumberFormatException nfe) {
                error = true;
            }
        }

        if (error) {
            setErrorMessage(
                event,
                LABEL_ARCHIVE_AGE,
                "archiving.copy.mediaparam.error.archiveage");
            return error;
        }

        // Validate if user picks a selection in the archive media drop down
        String archiveMenu = (String) wizardModel.getValue(ARCHIVE_MEDIA_MENU);
        if (archiveMenu.length() == 0) {
            // none select
            setErrorMessage(
                event,
                LABEL_ARCHIVE_MEDIA,
                "archiving.copy.mediaparam.error.archivemedia");
            return false;
        }
        return true;
    }

    /**
     * Retrieve the Copy Info structure to populate info
     */
    private CopyInfo getCopyInfoObject(HashMap copyNumberHashMap) {
        // Grab the HashMap that contains copy information from wizardModel
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // HashMap is null if called in New Copy Wizard
        if (copyNumberHashMap == null) {
            return null;
        }

        // Grab the copy number that this page represents,
        // the copy number is assigned in getPageClass()
        Integer copyNumber =
            (Integer) wizardModel.getValue(Constants.Wizard.COPY_NUMBER);

        // Retrieve the CopyInfo data structure from the HashMap
        CopyInfo info = (CopyInfo) copyNumberHashMap.get(copyNumber);

        return info;
    }

    private int getCopyNumber() {
        Integer copyNumber = (Integer) ((SamWizardModel) getDefaultModel()).
            getValue(Constants.Wizard.COPY_NUMBER);
        return
            copyNumber == null ? -1 : copyNumber.intValue();
    }

    private void setErrorMessage(
        WizardEvent event, String labelName, String message) {
        event.setSeverity(WizardEvent.ACKNOWLEDGE);
        event.setErrorMessage(message);
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        wizardModel.setValue(SamWizardImpl.VALIDATION_ERROR, labelName);
    }

    private void setErrorLabel(
        SamWizardModel wizardModel, boolean isPolicyWizard) {
        String labelName =
            (String) wizardModel.getValue(SamWizardImpl.VALIDATION_ERROR);
        if (labelName == null || labelName.length() == 0) {
            return;
        }

        CCLabel theLabel = (CCLabel) getChild(labelName);
        if (theLabel != null) {
            theLabel.setShowError(true);
        }

        // reset wizardModel field
        wizardModel.setValue(SamWizardImpl.VALIDATION_ERROR, "");
    }

        // implement the CCWizardPage
    public String getPageletUrl() {
        return "/jsp/archive/wizards/CopyParams.jsp";
    }

    private String getServerName() {
        SamWizardModel model = (SamWizardModel)getDefaultModel();
        String serverName = (String)
            model.getValue(Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

        return serverName;
    }
}
