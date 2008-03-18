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

// ident	$Id: NewDataClassWizardConfigureCopyView.java,v 1.12 2008/03/17 14:43:31 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
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
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.util.HashMap;

/**
 * A ContainerView object for the pagelet for configure copy in New Data
 * Class Wizard.
 *
 */
public class NewDataClassWizardConfigureCopyView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME =
        "NewDataClassWizardConfigureCopyView";

    // Child view names (i.e. display fields).
    public static final String ALERT = "Alert";
    public static final String COPY_TIME_LABEL = "CopyTimeText";
    public static final String EXPIRATION_TIME_LABEL = "ExpirationTimeText";
    public static final String MEDIA_POOL_LABEL = "MediaPoolText";
    public static final String SCRATCH_POOL_LABEL = "ScratchPoolText";
    public static final String COPY_MIGRATION_TO_LABEL = "CopyMigrationToText";
    public static final String ENABLE_RECYCLING_LABEL = "EnableRecyclingText";
    public static final String COPY_TIME = "CopyTimeTextField";
    public static final String COPY_TIME_UNIT = "CopyTimeDropDown";
    public static final String EXPIRATION_TIME = "ExpirationTimeTextField";
    public static final String EXPIRATION_TIME_UNIT = "ExpirationTimeDropDown";
    public static final String NEVER_EXPIRE = "NeverExpireCheckBox";
    public static final String MEDIA_POOL = "MediaPoolDropDown";
    public static final String SCRATCH_POOL = "ScratchPoolDropDown";
    public static final String COPY_MIGRATE_TO = "CopyMigrateToDropDown";
    public static final String ENABLE_RECYCLING = "EnableRecyclingCheckBox";
    public static final String POOL_INFO = "PoolInfo";
    public static final String SELECTED_SCRATCH_POOL = "SelectedScratchPool";

    private boolean prevErr;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewDataClassWizardConfigureCopyView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewDataClassWizardConfigureCopyView(
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
        registerChild(ALERT, CCAlertInline.class);
        registerChild(COPY_TIME, CCTextField.class);
        registerChild(EXPIRATION_TIME, CCTextField.class);
        registerChild(COPY_TIME_LABEL, CCLabel.class);
        registerChild(EXPIRATION_TIME_LABEL, CCLabel.class);
        registerChild(MEDIA_POOL_LABEL, CCLabel.class);
        registerChild(SCRATCH_POOL_LABEL, CCLabel.class);
        registerChild(COPY_MIGRATION_TO_LABEL, CCLabel.class);
        registerChild(ENABLE_RECYCLING_LABEL, CCLabel.class);
        registerChild(COPY_TIME_UNIT, CCDropDownMenu.class);
        registerChild(EXPIRATION_TIME_UNIT, CCDropDownMenu.class);
        registerChild(MEDIA_POOL, CCDropDownMenu.class);
        registerChild(SCRATCH_POOL, CCDropDownMenu.class);
        registerChild(COPY_MIGRATE_TO, CCDropDownMenu.class);
        registerChild(NEVER_EXPIRE, CCCheckBox.class);
        registerChild(ENABLE_RECYCLING, CCCheckBox.class);
        registerChild(POOL_INFO, CCHiddenField.class);
        registerChild(SELECTED_SCRATCH_POOL, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering").
            append(name).toString());

        View child;
        if (name.equals(ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(COPY_TIME) ||
                   name.equals(EXPIRATION_TIME)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(COPY_TIME_LABEL) ||
                   name.equals(EXPIRATION_TIME_LABEL) ||
                   name.equals(MEDIA_POOL_LABEL) ||
                   name.equals(SCRATCH_POOL_LABEL) ||
                   name.equals(COPY_MIGRATION_TO_LABEL) ||
                   name.equals(ENABLE_RECYCLING_LABEL)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(COPY_TIME_UNIT) ||
                   name.equals(EXPIRATION_TIME_UNIT) ||
                   name.equals(MEDIA_POOL) ||
                   name.equals(SCRATCH_POOL) ||
                   name.equals(COPY_MIGRATE_TO)) {
            child = new CCDropDownMenu(this, name, null);
        } else if (name.equals(NEVER_EXPIRE) ||
                   name.equals(ENABLE_RECYCLING)) {
            child = new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(POOL_INFO) ||
                   name.equals(SELECTED_SCRATCH_POOL)) {
            child = new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child name ["
                                               + name
                                               + "]");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        TraceUtil.trace3("Entering");

        String url = null;
        if (!prevErr) {
            url = "/jsp/archive/wizards/NewDataClassWizardConfigureCopy.jsp";
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
        String errorString = (String)
            wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (errorString != null) {
            if (errorString.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
                prevErr = true;

                String msgs = (String) wizardModel.getValue(
                    Constants.Wizard.ERROR_MESSAGE);
                int code = Integer.parseInt((String) wizardModel.getValue(
                    Constants.Wizard.ERROR_CODE));
                SamUtil.setErrorAlert(
                    this,
                    NewDataClassWizardConfigureCopyView.ALERT,
                    "NewArchivePolWizard.error.carryover",
                    code,
                    msgs,
                    getServerName(wizardModel));
            }
        }

        populateDropDownMenus(wizardModel);
        populateCopyInformation(wizardModel);

        // set the label to error mode if necessary
        setErrorLabel(wizardModel);


        TraceUtil.trace3("Exiting");
    }

    private void populateDropDownMenus(SamWizardModel wizardModel) {
        // Copy / Expire Time
        OptionList options =
            new OptionList(
                SelectableGroupHelper.Time.labels,
                SelectableGroupHelper.Time.values);
        CCDropDownMenu dropDown = (CCDropDownMenu) getChild(COPY_TIME_UNIT);
        dropDown.setOptions(options);
        dropDown = (CCDropDownMenu) getChild(EXPIRATION_TIME_UNIT);
        dropDown.setOptions(options);

        // Media Pool
        // populate the VSN Pools dropdown
        dropDown = (CCDropDownMenu) getChild(MEDIA_POOL);

        VSNPool [] vsnPools = null;
        try {
            vsnPools = SamUtil.getModel(getServerName(wizardModel)).
                getSamQFSSystemArchiveManager().getAllVSNPools();
        } catch (Exception e) {
            // TODO: Need to file a bug that getAllVSNPools SHOULD NOT
            // throw an exception when catserverd is not running.
            // All we need here is the pool name and the media type,
            // and they are available in archiver.cmd.
            // Need to fix the New Archive Policy Wizard as well if we
            // keep using the New policy wizard for the point product.
        }
        if (vsnPools == null) {
            vsnPools = new VSNPool[0];
        }

        // For Media Pool
        String [] vsnLabels = new String [vsnPools.length];
        String [] vsnValues = new String [vsnPools.length];

        // Prepare for Scratch Pool
        NonSyncStringBuffer allPoolbuf = new NonSyncStringBuffer();

        for (int j = 0; j < vsnPools.length; j++) {
            vsnLabels[j] =
                vsnPools[j].getPoolName() + " (" +
                SamUtil.getMediaTypeString(vsnPools[j].getMediaType()) + ")";
            vsnValues[j] =
                vsnPools[j].getPoolName() + "," + vsnPools[j].getMediaType();

            if (allPoolbuf.length() != 0) {
                allPoolbuf.append("###");
            }
            allPoolbuf.append(vsnPools[j].getPoolName()).append("&").
                append(SamUtil.getMediaTypeString(vsnPools[j].getMediaType())).
                append("&").append(vsnPools[j].getMediaType());
        }

        options = new OptionList(vsnLabels, vsnValues);
        dropDown.setOptions(options);

        // Set the pool information to hidden field.  This is needed for the
        // javascript to populate the scratch pool on the fly
        CCHiddenField PoolInfo = (CCHiddenField) getChild(POOL_INFO);
        PoolInfo.setValue(allPoolbuf.toString());
    }

    private void populateCopyInformation(SamWizardModel wizardModel) {
        // Base on the Copy Number, populate the fields in the page

        // Grab the HashMap that contains copy information from wizardModel
        HashMap copyNumberHashMap =
            (HashMap) wizardModel.getValue(NewDataClassWizardImpl.COPY_HASHMAP);

        // Grab the copy number that this page represents,
        // the copy number is assigned in getPageClass()
        Integer copyNumber = getCopyNumber(wizardModel);

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
                wizardModel.setValue(
                    Constants.Wizard.WIZARD_ERROR,
                    Constants.Wizard.WIZARD_ERROR_YES);
                return;
            }

            wizardModel.setValue(
                NewPolicyWizardImpl.COPY_HASHMAP, copyNumberHashMap);

            resetAllFields();
            return;
        } else {
            myWrapper = info.getCopyWrapper();
        }

        // Grab the wrapper from the CopyInfo datastructure, then grab the
        // ArchiveCopy object to start prepopulating information
        ArchivePolCriteriaCopy criteriaCopy =
            myWrapper.getArchivePolCriteriaCopy();
        ArchiveCopy archiveCopy = myWrapper.getArchiveCopy();
        ArchiveVSNMap archiveVSNMap = archiveCopy.getArchiveVSNMap();

        // Populate Copy Time & Unit
        long aage    = criteriaCopy.getArchiveAge();
        int aageUnit = criteriaCopy.getArchiveAgeUnit();
        ((CCTextField) getChild(COPY_TIME)).setValue(
            aage == -1 ? "" : Long.toString(aage));
        ((CCDropDownMenu) getChild(COPY_TIME_UNIT)).setValue(
            aageUnit == -1 ?
                new Integer(SamQFSSystemModel.TIME_MINUTE).toString() :
                Integer.toString(aageUnit));

        // Expiration Time & Unit
        long uage    = criteriaCopy.getUnarchiveAge();
        int uageUnit = criteriaCopy.getUnarchiveAgeUnit();
        CCTextField expireTime = (CCTextField) getChild(EXPIRATION_TIME);
        expireTime.setValue(uage == -1 ? "" : Long.toString(uage));
        expireTime.setDisabled(uage == -1);
        expireTime.resetStateData();

        CCDropDownMenu unit = (CCDropDownMenu) getChild(EXPIRATION_TIME_UNIT);
        unit.setValue(
            uage == -1 ?
                new Integer(SamQFSSystemModel.TIME_MINUTE).toString() :
                Integer.toString(uageUnit));
        unit.setDisabled(uage == -1);

        ((CCCheckBox) getChild(NEVER_EXPIRE)).
            setValue(Boolean.toString(uage == -1));

        // Media Pool & Scratch Pool
        String poolInfo =
            (String) ((CCHiddenField) getChild(POOL_INFO)).getValue();
        String [] pools = archiveVSNMap.getPoolExpression().split(",");

        if (pools.length != 0) {
            int mediaType = findMediaType(poolInfo, pools[0]);
            ((CCDropDownMenu) getChild(MEDIA_POOL)).setValue(
                pools[0] + "," + mediaType);
            if (pools.length > 1) {
                mediaType = findMediaType(poolInfo, pools[1]);

                // Set selected scratch pool to a hidden field.  The content
                // of the scratch pool drop down is dynamically populated
                // in the client side upon the page is loaded. DO NOT set
                // the selected value directly to the drop down as it WILL BE
                // LOST.
                ((CCHiddenField) getChild(SELECTED_SCRATCH_POOL)).setValue(
                    pools[1] + "," + mediaType);
            }
        }

        // Enable Recycling?
        ((CCCheckBox) getChild(ENABLE_RECYCLING)).
            setValue(Boolean.toString(!archiveCopy.isIgnoreRecycle()));
    }

    private int findMediaType(String poolInfo, String poolName) {
        String [] pools = poolInfo.split("###");
        for (int i = 0; i < pools.length; i++) {
            String [] poolKeys = pools[i].split("&");
            if (poolName.equals(poolKeys[0])) {
                return Integer.parseInt(poolKeys[2]);
            }
        }
        return -1;
    }

    private void resetAllFields() {
        ((CCTextField) getChild(COPY_TIME)).setValue("4");
        ((CCDropDownMenu) getChild(COPY_TIME_UNIT)).setValue(
            new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        ((CCTextField) getChild(EXPIRATION_TIME)).setValue("");
        ((CCTextField) getChild(EXPIRATION_TIME)).setDisabled(true);
        ((CCDropDownMenu) getChild(EXPIRATION_TIME_UNIT)).setValue(
            new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        ((CCDropDownMenu) getChild(EXPIRATION_TIME_UNIT)).setDisabled(true);
        ((CCCheckBox) getChild(NEVER_EXPIRE)).setValue("true");
        ((CCDropDownMenu) getChild(MEDIA_POOL)).setValue("");
        ((CCDropDownMenu) getChild(SCRATCH_POOL)).
            setValue(SelectableGroupHelper.NOVAL);
        ((CCCheckBox) getChild(ENABLE_RECYCLING)).setValue("false");
    }

    /**
     * To retrieve the default wrapper from the back-end
     * @return Default Wrapper object from the back-end
     */
    private ArchiveCopyGUIWrapper getDefaultWrapper() throws SamFSException {
        SamUtil.doPrint("getDefaultWrapper called");
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        ArchiveCopyGUIWrapper wrapper =
            PolicyUtil.getArchiveManager(
                getServerName(wizardModel)).getArchiveCopyGUIWrapper();
        if (wrapper == null) {
            throw new SamFSException(null, -2023);
        } else {
            return wrapper;
        }
    }

    private void setErrorLabel(SamWizardModel wizardModel) {
        String labelName = (String) wizardModel.getValue(
            NewDataClassWizardImpl.VALIDATION_ERROR);
        if (labelName == null || labelName == "") {
            return;
        }

        CCLabel theLabel = (CCLabel) getChild(labelName);
        if (theLabel != null) {
            theLabel.setShowError(true);
        }

        // reset wizardModel field
        wizardModel.setValue(
            NewDataClassWizardImpl.VALIDATION_ERROR, "");
    }

    private Integer getCopyNumber(SamWizardModel wizardModel) {
        return (Integer) wizardModel.getValue(
            NewDataClassWizardImpl.CURRENT_COPY_NUMBER);
    }

    private String getServerName(SamWizardModel wizardModel) {
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
