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

// ident	$Id: NewCopyDiskOptions.java,v 1.19 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for Copy Disk Option Page.
 *
 */
public class NewCopyDiskOptions extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewCopyDiskOptions";

    public static final String CHILD_RECYCLE_HWM_TEXT = "RecycleHwmText";
    public static final String CHILD_RECYCLE_HWM_TEXTFIELD =
        "RecycleHwmTextField";
    public static final String CHILD_MIN_GAIN_TEXT = "MinGainText";
    public static final String CHILD_MIN_GAIN_TEXTFIELD = "MinGainTextField";

    public static final String CHILD_START_AGE_TEXT = "StartAgeText";
    public static final String CHILD_START_AGE_TEXTFIELD =
        "StartAgeTextField";
    public static final String CHILD_START_AGE_DROPDOWN = "StartAgeDropDown";
    public static final String CHILD_START_COUNT_TEXT = "StartCountText";
    public static final String CHILD_START_COUNT_TEXTFIELD =
        "StartCountTextField";
    public static final String CHILD_START_SIZE_TEXT = "StartSizeText";
    public static final String CHILD_START_SIZE_TEXTFIELD =
        "StartSizeTextField";
    public static final String CHILD_START_SIZE_DROPDOWN = "StartSizeDropDown";
    public static final String CHILD_OFFLINE_COPY_TEXT  = "OfflineCopyText";
    public static final
        String CHILD_OFFLINE_COPY_DROPDOWN = "OfflineCopyDropDown";
    public static final String CHILD_MAIL_ADDRESS_TEXT  = "mailAddressLabel";
    public static final String CHILD_MAIL_ADDRESS = "mailAddress";
    public static final String CHILD_IGNORE_RECYCLING_TEXT =
        "IgnoreRecyclingText";
    public static final String CHILD_IGNORE_RECYCLING_CHECKBOX =
        "IgnoreRecyclingCheckBox";

    public static final String CHILD_ALERT = "Alert";
    public static final String CHILD_ERROR = "errorOccur";

    // Page Title Attributes and Components.
    private static CCPageTitleModel pageTitleModel = null;

    private boolean previousError;
    private boolean error;


    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewCopyDiskOptions(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewCopyDiskOptions(View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);
        pageTitleModel = createPageTitleModel();
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_OFFLINE_COPY_TEXT, CCLabel.class);
        registerChild(CHILD_OFFLINE_COPY_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_RECYCLE_HWM_TEXT, CCLabel.class);
        registerChild(CHILD_RECYCLE_HWM_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_MIN_GAIN_TEXT, CCLabel.class);
        registerChild(CHILD_MIN_GAIN_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_IGNORE_RECYCLING_TEXT, CCLabel.class);
        registerChild(CHILD_IGNORE_RECYCLING_CHECKBOX, CCCheckBox.class);
        registerChild(CHILD_START_AGE_TEXT, CCLabel.class);
        registerChild(CHILD_START_AGE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_START_AGE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_START_COUNT_TEXT, CCLabel.class);
        registerChild(CHILD_START_COUNT_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_START_SIZE_TEXT, CCLabel.class);
        registerChild(CHILD_START_SIZE_TEXTFIELD, CCTextField.class);
        registerChild(CHILD_START_SIZE_DROPDOWN, CCDropDownMenu.class);
        registerChild(CHILD_MAIL_ADDRESS_TEXT, CCLabel.class);
        registerChild(CHILD_MAIL_ADDRESS, CCTextField.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(CHILD_ERROR, CCHiddenField.class);
        PageTitleUtil.registerChildren(this, pageTitleModel);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3("Entering");

        View child;
        if (name.equals(CHILD_ALERT)) {
            child = new CCAlertInline(this, name, null);
        } else if (name.equals(CHILD_ERROR)) {
            child = new CCHiddenField(this, name, null);
        } else if (name.equals(CHILD_MAIL_ADDRESS_TEXT) ||
            name.equals(CHILD_OFFLINE_COPY_TEXT) ||
            name.equals(CHILD_RECYCLE_HWM_TEXT) ||
            name.equals(CHILD_MIN_GAIN_TEXT) ||
            name.equals(CHILD_START_AGE_TEXT) ||
            name.equals(CHILD_START_COUNT_TEXT) ||
            name.equals(CHILD_START_SIZE_TEXT) ||
            name.equals(CHILD_IGNORE_RECYCLING_TEXT)) {
            child = new CCLabel(this, name, null);
        } else if (name.equals(CHILD_MAIL_ADDRESS) ||
            name.equals(CHILD_RECYCLE_HWM_TEXTFIELD) ||
            name.equals(CHILD_MIN_GAIN_TEXTFIELD) ||
            name.equals(CHILD_START_AGE_TEXTFIELD) ||
            name.equals(CHILD_START_COUNT_TEXTFIELD) ||
            name.equals(CHILD_START_SIZE_TEXTFIELD)) {
            child = new CCTextField(this, name, null);
        } else if (name.equals(CHILD_IGNORE_RECYCLING_CHECKBOX)) {
            child = new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(CHILD_START_AGE_DROPDOWN)) {
            CCDropDownMenu myChild =  new CCDropDownMenu(this, name, null);
            OptionList ageOptions =
                new OptionList(
                    SelectableGroupHelper.Time.labels,
                    SelectableGroupHelper.Time.values);
            myChild.setOptions(ageOptions);
            child = myChild;
        } else if (name.equals(CHILD_START_SIZE_DROPDOWN)) {
            CCDropDownMenu myChild =  new CCDropDownMenu(this, name, null);
            OptionList sizeOptions = new OptionList(
                SelectableGroupHelper.Size.labels,
                SelectableGroupHelper.Size.values);
            myChild.setOptions(sizeOptions);
            child = myChild;
        } else if (name.equals(CHILD_OFFLINE_COPY_DROPDOWN)) {
            CCDropDownMenu myChild = new CCDropDownMenu(this, name, null);
            OptionList offlineCopyOptions =
                new OptionList(
                    SelectableGroupHelper.OfflineCopyMethod.labels,
                    SelectableGroupHelper.OfflineCopyMethod.values);
            myChild.setOptions(offlineCopyOptions);
            child = myChild;
        // PageTitle Child
        } else if (PageTitleUtil.isChildSupported(pageTitleModel, name)) {
            child = PageTitleUtil.createChild(this, pageTitleModel, name);
        } else {
            throw new IllegalArgumentException(
                "NewCopyDiskOptions : Invalid child name [" + name + "]");
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
        if (!previousError) {
            url = "/jsp/archive/NewCopyDiskOptions.jsp";
        } else {
            url = "/jsp/util/WizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    private CCPageTitleModel createPageTitleModel() {
        TraceUtil.trace3("Entering");
        if (pageTitleModel == null) {
            pageTitleModel = new CCPageTitleModel(
                SamUtil.createBlankPageTitleXML());
        }
        TraceUtil.trace3("Exiting");
        return pageTitleModel;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wm = (SamWizardModel) getDefaultModel();

        if (!showPreviousError(wm)) {
            try {
                prePopulateFields(wm);
            } catch (SamFSException samEx) {
                SamUtil.processException(
                    samEx,
                    this.getClass(),
                    "populateTableModels",
                    "Failed to pre-populate input fields",
                    getServerName());
                SamUtil.setErrorAlert(this,
                    NewCopyDiskOptions.CHILD_ALERT,
                    "NewArchivePolWizard.page4.failedPopulate",
                    samEx.getSAMerrno(),
                    samEx.getMessage(),
                    getServerName());
            }
        }

        // Set label to red if error is detected
        setErrorLabel(wm);

        TraceUtil.trace3("Exiting");
    }

    private boolean showPreviousError(SamWizardModel wm) {
        // see if any errors occcurred from previous page
        String error = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);

        if (error != null && error.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            previousError = true;
            String msgs =
                (String) wm.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wm.getValue(Constants.Wizard.ERROR_CODE));
            SamUtil.setErrorAlert(this,
                NewCopyTapeOptions.CHILD_ALERT,
                "NewArchivePolWizard.error.carryover",
                code,
                msgs,
                getServerName());
            return true;
        } else {
            return false;
        }
    }

    private void prePopulateFields(SamWizardModel wm) throws SamFSException {
        ArchiveCopy copy = ((ArchiveCopyGUIWrapper)
            wm.getValue(NewCopyWizardImpl.COPY_GUIWRAPPER)).getArchiveCopy();

        // Offline Copy
        int offlineCopyMethod = copy.getOfflineCopyMethod();
        if (offlineCopyMethod != -1) {
            ((CCDropDownMenu) getChild(CHILD_OFFLINE_COPY_DROPDOWN)).
                setValue(Integer.toString(offlineCopyMethod));
        } else {
            ((CCDropDownMenu) getChild(CHILD_OFFLINE_COPY_DROPDOWN)).
                setValue(SelectableGroupHelper.NOVAL);
        }

        // Start Age
        long startAge = copy.getStartAge();
        int  startAgeUnit = copy.getStartAgeUnit();
        if (startAge != -1 && startAgeUnit != -1) {
            ((CCTextField) getChild(CHILD_START_AGE_TEXTFIELD)).
                setValue(Long.toString(startAge));
            ((CCDropDownMenu) getChild(CHILD_START_AGE_DROPDOWN)).
                setValue(Integer.toString(startAgeUnit));
        } else {
            ((CCDropDownMenu) getChild(CHILD_START_AGE_DROPDOWN)).setValue(
                new Integer(SamQFSSystemModel.TIME_MINUTE).toString());
        }

        // Start Count
        int startCount = copy.getStartCount();
        if (startCount != -1) {
            ((CCTextField) getChild(CHILD_START_COUNT_TEXTFIELD)).
                setValue(Long.toString(startCount));
        }

        // Start Size
        long startSize = copy.getStartSize();
        int  startSizeUnit = copy.getStartSizeUnit();
        if (startSize != -1 && startSizeUnit != -1) {
            ((CCTextField) getChild(CHILD_START_SIZE_TEXTFIELD)).
                setValue(Long.toString(startSize));
            ((CCDropDownMenu) getChild(CHILD_START_SIZE_DROPDOWN)).
                setValue(Integer.toString(startSizeUnit));
        } else {
            ((CCDropDownMenu) getChild(CHILD_START_SIZE_DROPDOWN)).
                setValue(new Integer(SamQFSSystemModel.SIZE_MB).toString());
        }

        // Recycle HWM
        int recycleHWM = copy.getRecycleHWM();
        if (recycleHWM != -1) {
            ((CCTextField) getChild(CHILD_RECYCLE_HWM_TEXTFIELD)).
                setValue(Long.toString(recycleHWM));
        }

        // Ignore Recycling
        boolean isIgnore = copy.isIgnoreRecycle();
        ((CCCheckBox) getChild(CHILD_IGNORE_RECYCLING_CHECKBOX)).
            setValue(Boolean.toString(isIgnore));

        // Mailing Address
        String mailingAddress = copy.getNotificationAddress();
        if (mailingAddress != null) {
            ((CCTextField) getChild(CHILD_MAIL_ADDRESS)).
                setValue(mailingAddress);
        }

        // Minimum Gain (%)
        int minGain = copy.getMinGain();
        if (minGain != -1) {
            ((CCTextField) getChild(CHILD_MIN_GAIN_TEXTFIELD)).
                setValue(Integer.toString(minGain));
        }
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }

    private void setErrorLabel(SamWizardModel wizardModel) {
        String labelName = (String) wizardModel.getValue(
            NewCopyWizardImpl.VALIDATION_ERROR);
        if (labelName == null || labelName == "") {
            return;
        }

        CCLabel theLabel = (CCLabel) getChild(labelName);
        if (theLabel != null) {
            theLabel.setShowError(true);
        }

        // reset wizardModel field
        wizardModel.setValue(
            NewCopyWizardImpl.VALIDATION_ERROR, "");
    }
}
