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

// ident	$Id: ChangeFileAttributesView.java,v 1.11 2008/10/01 22:43:32 ronaldso Exp $

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
import com.sun.netstorage.samqfs.mgmt.rel.Releaser;
import com.sun.netstorage.samqfs.mgmt.stg.Stager;
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

    // Sub-radio button group for Release & Stage
    public static final String SUB_RADIO = "SubRadio";

    // Check box to determine if new attributes are applied recursively
    // to all files in directory (reset to default before applying new flag)
    public static final String RECURSIVE  = "Recursive";

    // Other page components
    public static final String PARTIAL_RELEASE  = "PartialRelease";
    public static final String LABEL = "Label";
    public static final String PARTIAL_RELEASE_SIZE = "PartialReleaseSize";
    public static final String ALERT  = "Alert";
    public static final String SUBMIT = "Submit";
    public static final String HELP_TEXT = "HelpText";


    // private models for various components
    private CCPageTitleModel pageTitleModel;

    // keep track of the server name that is transferred from the VB
    private String serverName;

    private boolean directory;


    public ChangeFileAttributesView(
        View parent, String name, String serverName, boolean directory) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        this.serverName = serverName;
        this.directory  = directory;
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
        } else {
            // Error if get here
            throw new IllegalArgumentException("Invalid Child '" + name + "'");
        }

        TraceUtil.trace3("Exiting");
        return (View) child;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");

        int pageMode = getPageMode();
        if (getPageMode() != -1) {
            populatePageTitleModel(pageMode);
            populateRadioButtonGroup(pageMode);
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
            case SamQFSSystemFSManager.ARCHIVE:
                pageTitleModel.setPageTitleText(
                    "fs.filedetails.editattributes.archive");
                pageTitleModel.setPageTitleHelpMessage(
                    directory ?
                        "fs.filedetails.editattributes.archive.dir.help" :
                        "fs.filedetails.editattributes.archive.help");
                break;
            case SamQFSSystemFSManager.RELEASE:
                pageTitleModel.setPageTitleText(
                    "fs.filedetails.editattributes.release");
                pageTitleModel.setPageTitleHelpMessage(
                    directory ?
                        "fs.filedetails.editattributes.release.dir.help" :
                        "fs.filedetails.editattributes.release.help");
                break;
            case SamQFSSystemFSManager.STAGE:
                pageTitleModel.setPageTitleText(
                    "fs.filedetails.editattributes.stage");
                pageTitleModel.setPageTitleHelpMessage(
                    directory ?
                        "fs.filedetails.editattributes.stage.dir.help" :
                        "fs.filedetails.editattributes.stage.help");
                break;
        }
    }

    /**
     * Populate radio button group contents
     */
    private void populateRadioButtonGroup(int pageMode) {
        OptionList optionList    = null;
        CCRadioButton myRadio  = (CCRadioButton) getChild(RADIO);
        CCRadioButton subRadio = (CCRadioButton) getChild(SUB_RADIO);

        String [] existingAttArray = getFileAttributes().split("###");

        int currentSetting = -1;
        switch (pageMode) {
            case SamQFSSystemFSManager.ARCHIVE:
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

                // default to archive when file matches policy criterias
                // myRadio.setValue(Integer.toString(Archiver.DEFAULTS));
                myRadio.setValue(existingAttArray[pageMode]);
                myRadio.resetStateData();

                break;
            case SamQFSSystemFSManager.RELEASE:
                // Release contains "Never Release" & "Release"
                ((CCCheckBox) getChild(PARTIAL_RELEASE)).setDisabled(false);

                optionList =
                    new OptionList(
                        new String [] {
                            "fs.filedetails.releasing.never",
                            "fs.filedetails.releasing.release"},
                        new String [] {
                            Integer.toString(Releaser.NEVER),
                            "release"});
                myRadio.setOptions(optionList);

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
                    myRadio.setValue("release");
                }
                myRadio.resetStateData();

                // default to release when space is required
                subRadio.setOptions(
                    new OptionList(
                        new String [] {
                            "fs.filedetails.releasing.default",
                            "fs.filedetails.releasing.onecopy"},
                        new String [] {
                            Integer.toString(Releaser.RESET_DEFAULTS),
                            Integer.toString(Releaser.WHEN_1)}));

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
            case SamQFSSystemFSManager.STAGE:
                // Stage contains "Never Stage" & Stage
                optionList =
                    new OptionList(
                        new String [] {
                            "fs.filedetails.staging.never",
                            "fs.filedetails.staging.stage"},
                        new String [] {
                            Integer.toString(Releaser.NEVER),
                            "stage"});
                myRadio.setOptions(optionList);

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
                    myRadio.setValue("stage");
                }
                myRadio.resetStateData();

                // default to stage when a file is accessed
                subRadio.setOptions(
                    new OptionList(
                        new String [] {
                            "fs.filedetails.staging.default",
                            "fs.filedetails.staging.associative"},
                        new String [] {
                            Integer.toString(Stager.RESET_DEFAULTS),
                            Integer.toString(Stager.ASSOCIATIVE)}));

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

        int pageMode = getPageMode();

        String successMsg = null, errorMsg   = null;
        String [] existingAttArray = getFileAttributes().split("###");
        boolean recursive = false;

        SamQFSSystemFSManager fsManager = null;
        String radioValue =
            (String) ((CCRadioButton) getChild(RADIO)).getValue();

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
                case SamQFSSystemFSManager.ARCHIVE:
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

                case SamQFSSystemFSManager.RELEASE:
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
                                "Invalid subRadioValue detected!");
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

                case SamQFSSystemFSManager.STAGE:
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
                                "Invalid subRadioValue detected!");
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

    private int getPageMode() {
        return ((FileDetailsPopupViewBean)
            getParentViewBean()).getPageMode();
    }

    private String getFileName() {
        return ((FileDetailsPopupViewBean)
            getParentViewBean()).getFileNameWithPath();
    }

    private String getFileAttributes() {
        return (String) ((FileDetailsPopupViewBean)
            getParentViewBean()).getPageSessionAttribute(
            FileDetailsPopupViewBean.PSA_FILE_ATT);

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
            // error
            TraceUtil.trace1("NumberFormatException: " + numEx.getMessage() +
                " in isValidPartialSize(int)!");
        }

        return !(size < 8 || size > maxPartial || size % 8 != 0);
    }

    /**
     * Hide Recursive check box if entry is not a directory
     */
    public boolean beginRecursiveDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return directory;
    }

}
