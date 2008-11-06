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

// ident	$Id: ChangeFileAttributesView.java,v 1.13 2008/11/06 00:47:07 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.Archiver;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
import com.sun.netstorage.samqfs.web.archive.CriteriaDetailsViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemFSManager;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;

import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.pagetitle.CCPageTitle;
import java.io.IOException;
import javax.servlet.ServletException;

/**
 * ChangeFileAttributesView - view in File Details Pop Up Window if CHANGE
 * href link is clicked in Archive/Release/Stage Attributes row.
 */
public class ChangeFileAttributesView extends RequestHandlingViewBase
    implements CCPagelet {

    // Page Children
    public static final String PAGE_TITLE = "PageTitle";

    // Main radio button group for Archive/Release/Stage
    public static final String RADIO = "Radio";
    // values
    public static final String RELEASE = "release";
    public static final String STAGE = "stage";

    // Sub-radio button group for Release & Stage
    public static final String SUB_RADIO = "SubRadio";

    // Check box to determine if new attributes are applied recursively
    // to all files in directory (reset to default before applying new flag)
    // This checkbox only shows up in the File Details Pop Up.
    public static final String RECURSIVE = "Recursive";

    // Check box to determine if new attributes are applied to the files that
    // match the criteria and override individual file settings.  The check box
    // only shows up when the pagelet is used in the Criteria Details page
    public static final String OVERRIDE = "Override";



    // Other page components
    public static final String PARTIAL_RELEASE  = "PartialRelease";
    public static final String LABEL = "Label";
    public static final String PARTIAL_RELEASE_SIZE = "PartialReleaseSize";
    public static final String ALERT = "Alert";
    public static final String SUBMIT = "Submit";
    public static final String HELP_TEXT = "HelpText";

    // private models for various components
    private CCPageTitleModel pageTitleModel;

    // keep track of the server name that is transferred from the VB
    private String serverName;

    private boolean directory;

    // Page Session Attributes to keep track of current file attributes in the
    // form of Archiver_Att###Releaser_Att###Stager_Att
    public static final String PSA_FILE_ATT = "psa_file_att";

    // Keep track of the viewbean that uses this view
    public static final short PAGE_FILE_DETAIL = 0;
    public static final short PAGE_CRITERIA_DETAIL = 1;
    private short parentPage = -1;

    // Keep track on the mode of the page (archive, release, or stage)
    // NOTE: This variable is used only when this pagelet is used in the
    // Criteria Details Page.  getPageMode() retrieves the page mode information
    // from the FileDetails view bean because the pagelet can be in different
    // mode within the same pop up when it reloads.  If we pass the pageMode
    // in the ChangeFileAttributesView constructor, the pageMode will not
    // contain the updated information because createChild() in File Details
    // View Bean does not pass the latest
    // -1 == Empty
    // 0  == Change Archive Attribute mode
    // 1  == Change Release Attribute mode
    // 2  == Change Stage   Attribute mode
    private int pageMode = -1;
    public static final int MODE_ARCHIVE = 0;
    public static final int MODE_RELEASE = 1;
    public static final int MODE_STAGE = 2;


    public ChangeFileAttributesView(
        View parent, String name, String serverName,
        boolean directory, short parentPage, int pageMode) {
        super(parent, name);
        TraceUtil.trace3("Entering ChangeFileAttributesView()");
        TraceUtil.trace3("serverName: " + serverName);
        TraceUtil.trace3("parentPage: " + parentPage);
        TraceUtil.trace3("pageMode: " + pageMode);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        this.serverName = serverName;
        this.directory  = directory;
        this.parentPage = parentPage;
        this.pageMode = pageMode;
        pageTitleModel  = createPageTitleModel();

        registerChildren();

        TraceUtil.trace3("Exiting");
    }

    /**
     * registerChildren
     */
    public void registerChildren() {
        TraceUtil.trace3("Entering");
        pageTitleModel.registerChildren(this);
        registerChild(PAGE_TITLE, CCPageTitle.class);
        registerChild(RADIO, CCRadioButton.class);
        registerChild(SUB_RADIO, CCRadioButton.class);
        registerChild(PARTIAL_RELEASE, CCCheckBox.class);
        registerChild(PARTIAL_RELEASE_SIZE, CCTextField.class);
        registerChild(LABEL, CCLabel.class);
        registerChild(ALERT, CCAlertInline.class);
        registerChild(SUBMIT, CCButton.class);
        registerChild(HELP_TEXT, CCStaticTextField.class);
        registerChild(RECURSIVE, CCCheckBox.class);
        registerChild(OVERRIDE, CCCheckBox.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * createChild
     */
    public View createChild(String name) {
        TraceUtil.trace3("Entering");

        View child = null;

        if (name.equals(PAGE_TITLE)) {
            child = new CCPageTitle(this, pageTitleModel, name);
        } else if (pageTitleModel.isChildSupported(name)) {
            // Create child from page title model.
            child = pageTitleModel.createChild(this, name);
        } else if (name.equals(RADIO) ||
                   name.equals(SUB_RADIO)) {
            return new CCRadioButton(this, name, null);
        } else if (name.equals(PARTIAL_RELEASE)) {
            return new CCCheckBox(
                this, name, Boolean.toString(true),
                Boolean.toString(false), false);
        } else if (name.equals(PARTIAL_RELEASE_SIZE)) {
            return new CCTextField(this, name, Integer.toString(8));
        } else if (name.equals(LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(SUBMIT)) {
            return new CCButton(this, name, null);
        } else if (name.equals(HELP_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(RECURSIVE)) {
            return new CCCheckBox(
                this, name, Boolean.toString(true),
                Boolean.toString(false), true);
        } else if (name.equals(OVERRIDE)) {
            return new CCCheckBox(
                this, name, Boolean.toString(true),
                Boolean.toString(false), false);
        } else {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    /**
     * Called when the view is displayed
     * @param evt
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        int pageMode = getPageMode();
        TraceUtil.trace3("Entering beginDisplay: pageMode is " + pageMode);
        if (pageMode != -1) {
            populatePageTitleModel(pageMode);
            // Initialize all radio button groups.
            // Radio button setValues for criteria details page are taking
            // place in Release/StageAttributesView.java
            populateRadioButtonGroup(pageMode);
        }

        // Change release help text if pagelet is used in Criteria Details page
        if (parentPage == PAGE_CRITERIA_DETAIL && pageMode == MODE_RELEASE) {
            ((CCStaticTextField) getChild(HELP_TEXT)).setValue(
                SamUtil.getResourceString(
                "fs.filedetails.releasing.releaseSize.criteriadetails.help"));
        }

        TraceUtil.trace3("Exiting");
    }

    // implement the CCPagelet interface
    /**
     * return the appropriate pagelet jsp
     */
    public String getPageletUrl() {
        if (getPageMode() == -1) {
            return "/jsp/archive/BlankPagelet.jsp";
        } else {
            return "/jsp/fs/ChangeFileAttributesPagelet.jsp";
        }
    }

    /**
     * Create page title model
     */
    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel =
                new CCPageTitleModel(SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
        return  pageTitleModel;
    }

    /**
     * Populate page title model
     */
    private void populatePageTitleModel(int pageMode) {
        // set page title
        switch (pageMode) {
            case MODE_ARCHIVE:
                // This case is used only in the File Details Pop Up
                pageTitleModel.setPageTitleText(
                    "fs.filedetails.editattributes.archive");
                pageTitleModel.setPageTitleHelpMessage(
                    directory ?
                        "fs.filedetails.editattributes.archive.dir.help" :
                        "fs.filedetails.editattributes.archive.help");
                break;
            case MODE_RELEASE:
                // Switch the page title text based on where this pagelet is
                // used
                pageTitleModel.setPageTitleText(
                    "fs.filedetails.editattributes.release");
                pageTitleModel.setPageTitleHelpMessage(
                    parentPage == PAGE_CRITERIA_DETAIL ?
                    "fs.filedetails.editattributes.release.criteriadetails.help"
                        : directory ?
                        "fs.filedetails.editattributes.release.dir.help" :
                        "fs.filedetails.editattributes.release.help");
                break;
            case MODE_STAGE:
                // Switch the page title text based on where this pagelet is
                // used
                pageTitleModel.setPageTitleText(
                    "fs.filedetails.editattributes.stage");
                pageTitleModel.setPageTitleHelpMessage(
                    parentPage == PAGE_CRITERIA_DETAIL ?
                    "fs.filedetails.editattributes.stage.criteriadetails.help"
                    : directory ?
                        "fs.filedetails.editattributes.stage.dir.help" :
                        "fs.filedetails.editattributes.stage.help");
                break;
        }
    }

    /**
     * Populate radio button group contents
     */
    private void populateRadioButtonGroup(int pageMode) {
        TraceUtil.trace3(
            "Entering populateRadioButtonGroup: pageMode is " + pageMode);
        OptionList optionList    = null;
        CCRadioButton myRadio  = (CCRadioButton) getChild(RADIO);
        CCRadioButton subRadio = (CCRadioButton) getChild(SUB_RADIO);

        String [] existingAttArray = getFileAttributes().split("###");

        int currentSetting = -1;
        switch (pageMode) {
            case MODE_ARCHIVE:
                // Archive contains "Never Archive" &
                // "When file matches policy(s) criterias"
                optionList =
                    new OptionList(
                        new String [] {
                            "fs.filedetails.archiving.never",
                            "fs.filedetails.archiving.default"},
                        new String [] {
                            Integer.toString(Archiver.NEVER),
                            Integer.toString(Archiver.DEFAULTS)});
                myRadio.setOptions(optionList);

                // This option only applies to the File Details Pop Up
                // default to archive when file matches policy criterias
                // myRadio.setValue(Integer.toString(Archiver.DEFAULTS));
                myRadio.setValue(existingAttArray[pageMode]);
                myRadio.resetStateData();

                break;
            case MODE_RELEASE:
                // Release contains "Never Release" & "Release"
                optionList =
                    new OptionList(
                        new String [] {
                            "fs.filedetails.releasing.never",
                            "fs.filedetails.releasing.release"},
                        new String [] {
                            Integer.toString(Releaser.NEVER),
                            RELEASE});
                myRadio.setOptions(optionList);

                // default to release when space is required
                subRadio.setOptions(
                    new OptionList(
                        new String [] {
                            "fs.filedetails.releasing.default",
                            "fs.filedetails.releasing.onecopy"},
                        new String [] {
                            Integer.toString(Releaser.RESET_DEFAULTS),
                            Integer.toString(Releaser.WHEN_1)}));

                // Only setValues when this is used in File Details Pop Up
                if (parentPage == PAGE_CRITERIA_DETAIL) {
                    return;
                }

                try {
                    currentSetting =
                        Integer.parseInt(existingAttArray[pageMode]);
                } catch (NumberFormatException numEx) {
                    TraceUtil.trace1("Developer's bug found!");
                }

                if (currentSetting == Releaser.NEVER) {
                    myRadio.setValue(existingAttArray[pageMode]);
                    subRadio.setDisabled(true);
                    ((CCCheckBox) getChild(PARTIAL_RELEASE)).setDisabled(true);
                    ((CCTextField) getChild(
                        PARTIAL_RELEASE_SIZE)).setDisabled(true);
                } else {
                    ((CCCheckBox) getChild(PARTIAL_RELEASE)).setDisabled(false);
                    myRadio.setValue(RELEASE);
                }
                myRadio.resetStateData();

                if (currentSetting != Releaser.NEVER) {
                    subRadio.setValue(existingAttArray[pageMode]);
                    int partialReleaseSize = -1;
                    try {
                        partialReleaseSize =
                            Integer.parseInt(existingAttArray[4]);
                    } catch (NumberFormatException numEx) {
                        TraceUtil.trace1("Developer's bug found!");
                    }

                    if (partialReleaseSize != -1) {
                        ((CCCheckBox) getChild(PARTIAL_RELEASE)).
                            setChecked(true);
                        ((CCCheckBox) getChild(PARTIAL_RELEASE)).
                            resetStateData();
                        ((CCTextField) getChild(PARTIAL_RELEASE_SIZE)).
                            setDisabled(false);
                        ((CCTextField) getChild(PARTIAL_RELEASE_SIZE)).
                            setValue(existingAttArray[4]);
                        ((CCTextField) getChild(PARTIAL_RELEASE_SIZE)).
                            resetStateData();
                    }
                }
                subRadio.resetStateData();

                // populate inline help text for partial release
                ((CCStaticTextField) getChild(HELP_TEXT)).setValue(
                    SamUtil.getResourceString(
                        "fs.filedetails.releasing.releaseSize.help",
                        existingAttArray[3]));

                break;
            case MODE_STAGE:
                // Stage contains "Never Stage" & Stage
                optionList =
                    new OptionList(
                        new String [] {
                            "fs.filedetails.staging.never",
                            "fs.filedetails.staging.stage"},
                        new String [] {
                            Integer.toString(Stager.NEVER),
                            STAGE});
                myRadio.setOptions(optionList);

                // default to stage when a file is accessed
                subRadio.setOptions(
                    new OptionList(
                        new String [] {
                            "fs.filedetails.staging.default",
                            "fs.filedetails.staging.associative"},
                        new String [] {
                            Integer.toString(Stager.RESET_DEFAULTS),
                            Integer.toString(Stager.ASSOCIATIVE)}));

                // Only setValues when this is used in File Details Pop Up
                if (parentPage == PAGE_CRITERIA_DETAIL) {
                    return;
                }

                try {
                    currentSetting =
                        Integer.parseInt(existingAttArray[pageMode]);
                } catch (NumberFormatException numEx) {
                    TraceUtil.trace1("Developer's bug found!");
                }

                if (currentSetting == Stager.NEVER) {
                    myRadio.setValue(existingAttArray[pageMode]);
                    subRadio.setDisabled(true);
                } else {
                    myRadio.setValue(STAGE);
                }
                myRadio.resetStateData();

                if (currentSetting != Releaser.NEVER) {
                    subRadio.setValue(existingAttArray[pageMode]);
                }
                subRadio.resetStateData();
                break;
        }
    }


    public void handleSubmitRequest(RequestInvocationEvent event)
        throws ServletException, IOException, ModelControlException {
        TraceUtil.trace3("Entering");
        TraceUtil.trace3("Entering handleSubmitRequest()");
        int pageMode =
            Integer.parseInt((String) getParentViewBean().getDisplayFieldValue(
            FileDetailsPopupViewBean.PAGE_MODE));
        TraceUtil.trace3("pageMode is " + pageMode);

        String successMsg = null, errorMsg   = null;
        String [] existingAttArray = getFileAttributes().split("###");
        boolean recursive = false;
        TraceUtil.trace3("fileAttributes: " + getFileAttributes());
        SamQFSSystemFSManager fsManager = null;
        String radioValue =
            (String) ((CCRadioButton) getChild(RADIO)).getValue();
        TraceUtil.trace3("radioValue: " + radioValue);
        // If entry is a directory, check if RECURSIVE is checked
        if (directory) {
            CCCheckBox checkBox = (CCCheckBox) getChild(RECURSIVE);
            recursive = "true".equals((String) checkBox.getValue());
        }

        int newOption = -1, existingOption = -1, partialSize = -1;

        try {
            // Check Permission (IE7)
            if (!SecurityManagerFactory.getSecurityManager().
                hasAuthorization(Authorization.FILE_OPERATOR)) {
                throw new SamFSException("common.nopermission");
            }

            try {
                existingOption = Integer.parseInt(existingAttArray[pageMode]);
            } catch (NumberFormatException numEx) {
                throw new SamFSException(
                    "Developer's bug found, incorrect existingOption!");
            }
            fsManager = SamUtil.getModel(serverName).getSamQFSSystemFSManager();

            switch (pageMode) {
                case MODE_ARCHIVE:
                    successMsg = "fs.filedetails.archiving.success";
                    errorMsg   = "fs.filedetails.archiving.failed";

                    try {
                        newOption = Integer.parseInt(radioValue);
                    } catch (NumberFormatException numEx) {
                        // Developer's bug
                        throw new SamFSException(
                            "Invalid radioValue detected!");
                    }
                    break;

                case MODE_RELEASE:
                    successMsg = "fs.filedetails.releasing.success";
                    errorMsg   = "fs.filedetails.releasing.failed";

                    if ("release".equals(radioValue)) {
                        String subRadioValue = (String)
                            ((CCRadioButton) getChild(SUB_RADIO)).getValue();
                        try {
                            newOption = Integer.parseInt(subRadioValue);
                        } catch (NumberFormatException numEx) {
                            // Developer's bug
                            throw new SamFSException(
                                "Invalid subRadioValue detected!", numEx);
                        }
                    } else {
                        newOption = Releaser.NEVER;
                    }

                    CCCheckBox check = (CCCheckBox) getChild(PARTIAL_RELEASE);
                    if ("true".equals(check.getValue())) {
                        String sizeText = (String) ((CCTextField)
                                    getChild(PARTIAL_RELEASE_SIZE)).getValue();
                        try {
                            partialSize = Integer.parseInt(sizeText);
                        } catch (NumberFormatException numEx) {
                            // let partialSize be -1 and generate error
                            // in isValidPartialSize
                        }
                        if (!isValidPartialSize(partialSize)) {
                            ((CCLabel) getChild(LABEL)).setShowError(true);
                            throw new SamFSException(
                                SamUtil.getResourceString(
                                "fs.filedetails.releasing.invalidReleaseSize"));
                        } else {
                            newOption = newOption | Releaser.PARTIAL;
                        }
                    }
                    break;

                case MODE_STAGE:
                    successMsg = "fs.filedetails.staging.success";
                    errorMsg   = "fs.filedetails.staging.failed";

                    if ("stage".equals(radioValue)) {
                        String subRadioValue = (String)
                            ((CCRadioButton) getChild(SUB_RADIO)).getValue();
                        try {
                            newOption = Integer.parseInt(subRadioValue);
                        } catch (NumberFormatException numEx) {
                            // Developer's bug
                            throw new SamFSException(
                                "Invalid subRadioValue detected!", numEx);
                        }
                    } else {
                        newOption = Stager.NEVER;
                    }

                    break;
            }

            fsManager.changeFileAttributes(
                        pageMode,
                        getFileName(),
                        newOption, existingOption, recursive, partialSize);

            SamUtil.setInfoAlert(
                this,
                ALERT,
                "success.summary",
                successMsg,
                serverName);
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to set file attributes! Reason: " + samEx.getMessage());
            SamUtil.processException(
                samEx,
                this.getClass(),
                "handleSubmitRequest()",
                "Failed to set file attributes!",
                serverName);

            // Suppress the following two error codes because the detailed
            // error message makes no sense in this case
            // SE_RELEASE_FILES_FAILED = 30180,
            // SE_ARCHIVE_FILES_FAILED = 30629,
            SamUtil.setErrorAlert(
                this,
                ALERT,
                errorMsg,
                samEx.getSAMerrno(),
                samEx.getSAMerrno() == 30180 || samEx.getSAMerrno() == 30629 ?
                    "" : samEx.getMessage(),
                serverName);
        }

        getParentViewBean().forwardTo(getRequestContext());
        TraceUtil.trace3("Exiting");
    }

    /**
     * The definitions of the page mode can be one of the following:
     * 0 == Change Archive Attribute mode
     * 1 == Change Release Attribute mode
     * 2 == Change Stage   Attribute mode
     * @return the mode of the page
     */
    public int getPageMode() {
        return pageMode;
    }

    public void setPageMode(int pageMode) {
        this.pageMode = pageMode;
    }


    /**
     *  This method is only needed in the File Details Pop Up.
     * @return the name of the file of which user is changing the file attrs.
     */
    private String getFileName() {
        return ((FileDetailsPopupViewBean)
            getParentViewBean()).getFileNameWithPath();
    }

    /**
     *  This method is only needed in the File Details Pop Up.
     * @return the current file attributes of which user is viewing
     */
    private String getFileAttributes() {
        TraceUtil.trace3(
            "Entering getFileAttributes(): parentPage: " + parentPage);
        if (parentPage == PAGE_FILE_DETAIL) {
            return (String) ((FileDetailsPopupViewBean)
                getParentViewBean()).getPageSessionAttribute(PSA_FILE_ATT);
        } else {
            return (String) ((CriteriaDetailsViewBean)
                getParentViewBean()).getPageSessionAttribute(PSA_FILE_ATT);
        }
    }

    /**
     * Helper function of changeFileAttributes
     * Check if the size is valid. Valid ranges from 8 to Max-Partial of the FS
     */
    private boolean isValidPartialSize(int size) {
        String [] existingAttArray = getFileAttributes().split("###");
        int maxPartial = -1;
        try {
            maxPartial = Integer.parseInt(existingAttArray[3]);
        } catch (NumberFormatException numEx) {
            TraceUtil.trace1("NumberFormatException: " + numEx.getMessage() +
                " in isValidPartialSize(int)!");
            return false;
        }

        return !(size < 8 || size > maxPartial);
    }

    /**
     * Hide Recursive check box if entry is not a directory
     */
    public boolean beginRecursiveDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return directory;
    }

    /**
     * Hide Override check box if this pagelet is used for the File Details
     * Pop up.
     * @param event
     * @return boolean to indicate if this component is visible or not
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public boolean beginOverrideDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return parentPage == PAGE_CRITERIA_DETAIL;
    }

    /**
     * This method determines if the managed server has the capability to set
     * multiple release/stage attributes.  This method is used only in the
     * Criteria Details Page.
     * @return if the server can support multiple attributes
     */
    protected boolean isServerPatched() {
        // Boolean to determine if multiple flags are allowed.
        // Multiple Release Attribute flag is allowed in:
        // Version 5.0
        // Version 4.6 with patch 04 (API Version 1.5.9.1 or later)
        try {
            String apiVersion = SamUtil.getAPIVersion(serverName);
            TraceUtil.trace3("isServerPatched: apiVersion: " + apiVersion);
            if (apiVersion.startsWith("1.5")) {
                return
                    SamUtil.isVersionCurrentOrLaterThan(apiVersion, "1.5.9.1");
            } else {
                // This is a 5.0 GUI managing a 5.0 server
                return true;
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1(
                "Failed to determine if the server can support multi-attr!");
            TraceUtil.trace1("Reason: " + samEx.getMessage());
            return false;
        }
    }

    /**
     * Helper method to determine if the attribute that user sets contains
     * multiple flags.  This method is used only in the Criteria Details page.
     * @param attr Attribute of which that needs to be tested
     * @param releaseMode Boolean to define if it is used to check the release
     * or stage attribute
     * @return if the attribute is multi-flagged
     */
    protected boolean isMultiFlag(int attr, boolean releaseMode) {
        if (attr == 0) {
            return false;
        }

        int counter = 0;
        if (releaseMode) {
            // Release
            if ((attr & Criteria.ATTR_RESET_RELEASE_DEFAULT)
                        == Criteria.ATTR_RESET_RELEASE_DEFAULT) {
                counter++;
            }
            if ((attr & Criteria.ATTR_RELEASE_ALWAYS)
                        == Criteria.ATTR_RELEASE_ALWAYS) {
                counter++;
            }
            if ((attr & Criteria.ATTR_RELEASE_NEVER)
                        == Criteria.ATTR_RELEASE_NEVER) {
                counter++;
            }
            if ((attr & Criteria.ATTR_RELEASE_PARTIAL)
                        == Criteria.ATTR_RELEASE_PARTIAL) {
                counter++;
            }
        } else {
            // Stage
            if ((attr & Criteria.ATTR_RESET_STAGE_DEFAULT)
                        == Criteria.ATTR_RESET_STAGE_DEFAULT) {
                counter++;
            }
            if ((attr & Criteria.ATTR_STAGE_ASSOCIATIVE)
                        == Criteria.ATTR_STAGE_ASSOCIATIVE) {
                counter++;
            }
            if ((attr & Criteria.ATTR_STAGE_NEVER)
                        == Criteria.ATTR_STAGE_NEVER) {
                counter++;
            }
        }
        return counter > 1;
    }
}
