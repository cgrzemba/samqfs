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

// ident	$Id: NewWizardArchiveConfigView.java,v 1.14 2008/03/17 14:43:36 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;


public class NewWizardArchiveConfigView extends RequestHandlingViewBase
                                        implements CCWizardPage {
    public static final String PAGE_NAME = "NewWizardArchiveConfigView";
    public static final
        String DEFAULT_URL = "/jsp/fs/NewWizardArchiveConfig.jsp";

    // children
    public static final String ALERT = "Alert";
    public static final String POLICY_TYPE = "policyType";
    public static final String POLICY_TYPE_EXISTING = "policyTypeExisting";
    public static final String POLICY_TYPE_NEW = "policyTypeNew";
    public static final String EXISTING_POLICY_NAME = "existingPolicyName";
    public static final String NEW_POLICY_NAME_LABEL = "policyNameLabel";
    public static final String NEW_POLICY_NAME = "newPolicyName";
    public static final String COPY_ONE_LABEL = "CopyOneLabel";
    public static final String ENABLE_COPY_TWO = "enableCopyTwo";
    public static final String ENABLE_COPY_THREE = "enableCopyThree";
    public static final String ENABLE_COPY_FOUR = "enableCopyFour";
    public static final String ARCHIVE_AGE_LABEL = "archiveAgeLabel";
    public static final String ARCHIVE_AGE_ONE = "archiveAgeOne";
    public static final String ARCHIVE_AGE_ONE_UNIT = "archiveAgeOneUnit";
    public static final String ARCHIVE_AGE_TWO = "archiveAgeTwo";
    public static final String ARCHIVE_AGE_TWO_UNIT = "archiveAgeTwoUnit";
    public static final String ARCHIVE_AGE_THREE = "archiveAgeThree";
    public static final String ARCHIVE_AGE_THREE_UNIT = "archiveAgeThreeUnit";
    public static final String ARCHIVE_AGE_FOUR = "archiveAgeFour";
    public static final String ARCHIVE_AGE_FOUR_UNIT = "archiveAgeFourUnit";
    public static final String MEDIA_LABEL = "mediaLabel";
    public static final String MEDIA_ONE = "mediaOne";
    public static final String MEDIA_TWO = "mediaTwo";
    public static final String MEDIA_THREE = "mediaThree";
    public static final String MEDIA_FOUR = "mediaFour";
    public static final String NO_RELEASE = "noRelease";
    public static final String LOG_FILE_LABEL = "logFileLabel";
    public static final String LOG_FILE = "logFile";
    public static final String ERROR_MSG = "errMsg";
    public static final String H_PTYPE = "ptype";
    public static final String SHOW_COPY_34 = "showCopy3and4";
    public static final String MORE_COPY_STATUS = "copy3and4Status";

    public static final String ERROR_OCCUR = "errorOccur";
    private boolean error = false;

    public NewWizardArchiveConfigView(View parent, Model model) {
        super(parent, PAGE_NAME);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        registerChild(POLICY_TYPE, CCRadioButton.class);
        registerChild(POLICY_TYPE_EXISTING, CCRadioButton.class);
        registerChild(POLICY_TYPE_NEW, CCRadioButton.class);
        registerChild(EXISTING_POLICY_NAME, CCDropDownMenu.class);
        registerChild(NEW_POLICY_NAME_LABEL, CCLabel.class);
        registerChild(NEW_POLICY_NAME, CCTextField.class);
        registerChild(COPY_ONE_LABEL, CCLabel.class);
        registerChild(ENABLE_COPY_TWO, CCCheckBox.class);
        registerChild(ENABLE_COPY_THREE, CCCheckBox.class);
        registerChild(ENABLE_COPY_FOUR, CCCheckBox.class);
        registerChild(ARCHIVE_AGE_LABEL, CCLabel.class);
        registerChild(ARCHIVE_AGE_ONE, CCTextField.class);
        registerChild(ARCHIVE_AGE_ONE_UNIT, CCDropDownMenu.class);
        registerChild(ARCHIVE_AGE_TWO, CCTextField.class);
        registerChild(ARCHIVE_AGE_TWO_UNIT, CCDropDownMenu.class);
        registerChild(ARCHIVE_AGE_THREE, CCTextField.class);
        registerChild(ARCHIVE_AGE_THREE_UNIT, CCDropDownMenu.class);
        registerChild(ARCHIVE_AGE_FOUR, CCTextField.class);
        registerChild(ARCHIVE_AGE_FOUR_UNIT, CCDropDownMenu.class);
        registerChild(MEDIA_LABEL, CCLabel.class);
        registerChild(MEDIA_ONE, CCDropDownMenu.class);
        registerChild(MEDIA_TWO, CCDropDownMenu.class);
        registerChild(MEDIA_THREE, CCDropDownMenu.class);
        registerChild(MEDIA_FOUR, CCDropDownMenu.class);
        registerChild(NO_RELEASE, CCCheckBox.class);
        registerChild(LOG_FILE_LABEL, CCLabel.class);
        registerChild(LOG_FILE, CCTextField.class);
        registerChild(ALERT, CCAlertInline.class);
        registerChild(ERROR_MSG, CCHiddenField.class);
        registerChild(H_PTYPE, CCHiddenField.class);
        registerChild(ERROR_OCCUR, CCHiddenField.class);
        registerChild(SHOW_COPY_34, CCButton.class);
        registerChild(MORE_COPY_STATUS, CCHiddenField.class);
    }

    public View createChild(String name) {
        if (name.equals(SHOW_COPY_34)) {
            return new CCButton(this, name, null);
        } else if (name.indexOf("Label") != -1) {
            return new CCLabel(this, name, null);
        } else if (name.equals(ARCHIVE_AGE_ONE) ||
                   name.equals(ARCHIVE_AGE_TWO) ||
                   name.equals(ARCHIVE_AGE_THREE) ||
                   name.equals(ARCHIVE_AGE_FOUR) ||
                   name.equals(LOG_FILE)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(ERROR_MSG) ||
                   name.equals(H_PTYPE) ||
                   name.equals(MORE_COPY_STATUS)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(ARCHIVE_AGE_ONE_UNIT) ||
                   name.equals(ARCHIVE_AGE_TWO_UNIT) ||
                   name.equals(ARCHIVE_AGE_THREE_UNIT) ||
                   name.equals(ARCHIVE_AGE_FOUR_UNIT) ||
                   name.equals(MEDIA_ONE) ||
                   name.equals(MEDIA_TWO) ||
                   name.equals(MEDIA_THREE) ||
                   name.equals(MEDIA_FOUR) ||
                   name.equals(EXISTING_POLICY_NAME) ||
                   name.equals(NEW_POLICY_NAME)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(NO_RELEASE) ||
                   name.equals(ENABLE_COPY_TWO) ||
                   name.equals(ENABLE_COPY_THREE) ||
                   name.equals(ENABLE_COPY_FOUR)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(POLICY_TYPE)) {
            return new CCRadioButton(this, name, null);
        } else if (name.equals(POLICY_TYPE_EXISTING)) {
            CCRadioButton child =  new CCRadioButton(this, name, null);
            child.setOptions(new OptionList(
                new String [] {"FSWizard.new.archiving.existing"},
                new String [] {"existing"}));
            return child;
        } else if (name.equals(POLICY_TYPE_NEW)) {
            CCRadioButton child =
                new CCRadioButton(this, POLICY_TYPE_EXISTING, null);
            child.setOptions(new OptionList(
               new String [] {"FSWizard.new.archiving.new"},
               new String [] {"new"}));
            return child;
        } else if (name.equals(ERROR_OCCUR)) {
            if (error) {
                return new CCHiddenField(
                    this, name, Constants.Wizard.EXCEPTION);
            } else {
                return new CCHiddenField(this, name, Constants.Wizard.SUCCESS);
            }
        } else {
            throw new IllegalArgumentException("invalid child '" + name +"'");
        }
    }

    private void initializeSelectableLists(String serverName)
        throws SamFSException {
        SamQFSSystemArchiveManager archiveManager =
            SamUtil.getModel(serverName).getSamQFSSystemArchiveManager();
        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        boolean noExistingPolicy = false;

        // existing policies
        String [] existingPolicyName =
            archiveManager.getAllExplicitlySetPolicyNames();
        if (existingPolicyName != null && existingPolicyName.length != 0) {
            ((CCDropDownMenu)getChild(EXISTING_POLICY_NAME))
                .setOptions(new OptionList(existingPolicyName,
                                           existingPolicyName));
            RequestManager.getRequestContext().getRequest()
                .setAttribute("_existing_policies_", "true");
        } else {
            ((CCDropDownMenu)getChild(EXISTING_POLICY_NAME)).setDisabled(true);
            noExistingPolicy = true;
            RequestManager.getRequestContext().getRequest()
                .setAttribute("_existing_policies_", "false");
        }

        // If user has never chosen to create or use an existing policy, and
        // if there is at least an existing policy available in the drop down,
        // default radio button to use existing policy.  Otherwise default
        // to create a new policy
        String policyType = (String) wm.getValue(
                            NewWizardArchiveConfigView.POLICY_TYPE_EXISTING);
        if (policyType == null || policyType.length() == 0) {
            wm.setValue(
                NewWizardArchiveConfigView.POLICY_TYPE_EXISTING,
                noExistingPolicy ?
                    "new" : "existing");
        }

        // new policy names
        // get the current file system name
        String fsName =
            (String)wm.getValue(NewWizardMountView.CHILD_FSNAME_FIELD);
        boolean found = false;
        for (int i = 0; !found && i < existingPolicyName.length; i++) {
            if ("all".equalsIgnoreCase(existingPolicyName[i])) {
                found = true;
            }
        }
        if (found) {
            ((CCDropDownMenu)getChild(NEW_POLICY_NAME))
                .setOptions(new OptionList(new String [] {fsName},
                                           new String [] {fsName}));
        } else {
            ((CCDropDownMenu)getChild(NEW_POLICY_NAME))
                .setOptions(new OptionList(new String [] {fsName, "all"},
                                           new String [] {fsName, "all"}));
        }

        // copy one archive age
        ((CCDropDownMenu)getChild(ARCHIVE_AGE_ONE_UNIT))
            .setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                       SelectableGroupHelper.Time.values));
        // copy two archive age
        ((CCDropDownMenu)getChild(ARCHIVE_AGE_TWO_UNIT))
            .setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                       SelectableGroupHelper.Time.values));

        // copy three archive age
        ((CCDropDownMenu)getChild(ARCHIVE_AGE_THREE_UNIT))
            .setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                       SelectableGroupHelper.Time.values));

        // copy four archive age
        ((CCDropDownMenu)getChild(ARCHIVE_AGE_FOUR_UNIT))
            .setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                       SelectableGroupHelper.Time.values));

        // available vsn pool
        String [][] vsnPool = PolicyUtil.getAllVSNPoolNames(serverName);
        OptionList poolList = new OptionList(vsnPool[0], vsnPool[1]);

        // now append the 'all <media type> vsns' pools to the list
        int [] mediaType = SamUtil.getModel(serverName)
            .getSamQFSSystemMediaManager().getAvailableArchiveMediaTypes();
        String mtLabel = "", mtValue = "";
        for (int i = 0; i < mediaType.length; i++) {
            mtLabel = SamUtil
                .getResourceString("FSWizard.new.archiving.allvsns",
                    new String [] {SamUtil.getMediaTypeString(mediaType[i])});

            mtValue = ".:" + mediaType[i];
            poolList.add(mtLabel, mtValue);
        }

        ((CCDropDownMenu)getChild(MEDIA_ONE)).setOptions(poolList);
        ((CCDropDownMenu)getChild(MEDIA_TWO)).setOptions(poolList);
        ((CCDropDownMenu)getChild(MEDIA_THREE)).setOptions(poolList);
        ((CCDropDownMenu)getChild(MEDIA_FOUR)).setOptions(poolList);
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String serverName = (String)wm.getValue(Constants.Wizard.SERVER_NAME);

        try {
            initializeSelectableLists(serverName);

            String pType = (String) wm.getValue(POLICY_TYPE_EXISTING);
            if (pType == null) pType = "undefined";
            ((CCHiddenField)getChild(H_PTYPE)).setValue(pType);

            String enableCopyTwo = (String)wm.getValue(ENABLE_COPY_TWO);
            if (enableCopyTwo == null || enableCopyTwo.equals("false")) {
                // initially do not check enable Create Copy 2
                wm.setValue(ENABLE_COPY_TWO, Boolean.toString(false));
            } else if ("true".equals(enableCopyTwo)) { // enable more copies
                ((CCButton)getChild(SHOW_COPY_34)).setDisabled(false);
            } else { // disable create more copies
                ((CCButton)getChild(SHOW_COPY_34)).setDisabled(true);
            }

            // determine the correct label for the create more copies button
            if ("visible".equals((String)wm.getValue(MORE_COPY_STATUS))) {
                ((CCButton)getChild(SHOW_COPY_34))
                    .setValue("FSWizard.new.archiving.copy3and4.hide");
            } else {
                ((CCButton)getChild(SHOW_COPY_34))
                    .setValue("FSWizard.new.archiving.copy3and4.show");
            }

            boolean enableCopyTwoBoolean = !"true".equals(enableCopyTwo);
            // disable copy two fields
            ((CCTextField) getChild(ARCHIVE_AGE_TWO)).
                setDisabled(enableCopyTwoBoolean);
            ((CCDropDownMenu) getChild(ARCHIVE_AGE_TWO_UNIT)).
                setDisabled(enableCopyTwoBoolean);
            ((CCDropDownMenu) getChild(MEDIA_TWO)).
                setDisabled(enableCopyTwoBoolean);

            if (enableCopyTwoBoolean) {
                ((CCTextField) getChild(ARCHIVE_AGE_TWO)).resetStateData();
            }

            // reset copy three fields
            String enableCopyThree = (String)wm.getValue(ENABLE_COPY_THREE);
            boolean enableCopyThreeBoolean = !"true".equals(enableCopyThree);
            // disable copy three fields
            ((CCTextField) getChild(ARCHIVE_AGE_THREE)).
                setDisabled(enableCopyThreeBoolean);
            ((CCDropDownMenu) getChild(ARCHIVE_AGE_THREE_UNIT)).
                setDisabled(enableCopyThreeBoolean);
            ((CCDropDownMenu) getChild(MEDIA_THREE)).
                setDisabled(enableCopyThreeBoolean);
            if (enableCopyThreeBoolean) {
                ((CCTextField) getChild(ARCHIVE_AGE_THREE)).resetStateData();
            }

            // disable the 'create copy four' check box iff three is enabled
            ((CCCheckBox)getChild(ENABLE_COPY_FOUR))
                .setDisabled(enableCopyThreeBoolean);

            // reset copy four fields
            String enableCopyFour = (String)wm.getValue(ENABLE_COPY_FOUR);
            boolean enableCopyFourBoolean = !"true".equals(enableCopyFour);
            // disable copy four fields
            ((CCTextField) getChild(ARCHIVE_AGE_FOUR)).
                setDisabled(enableCopyFourBoolean);
            ((CCDropDownMenu) getChild(ARCHIVE_AGE_FOUR_UNIT)).
                setDisabled(enableCopyFourBoolean);
            ((CCDropDownMenu) getChild(MEDIA_FOUR)).
                setDisabled(enableCopyFourBoolean);
            if (enableCopyFourBoolean) {
                ((CCTextField) getChild(ARCHIVE_AGE_FOUR)).resetStateData();
            }

            // if Archive age fields are empty, use system defaults
            String archiveAge = (String) wm.getValue(ARCHIVE_AGE_ONE);
            if (archiveAge == null) {
                wm.setValue(
                    ARCHIVE_AGE_ONE,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE));
                wm.setValue(
                    ARCHIVE_AGE_ONE_UNIT,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT));
            }
            archiveAge = (String) wm.getValue(ARCHIVE_AGE_TWO);
            if (archiveAge == null) {
                wm.setValue(
                    ARCHIVE_AGE_TWO,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE));
                wm.setValue(
                    ARCHIVE_AGE_TWO_UNIT,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT));
            }
            archiveAge = (String) wm.getValue(ARCHIVE_AGE_THREE);
            if (archiveAge == null) {
                wm.setValue(
                    ARCHIVE_AGE_THREE,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE));
                wm.setValue(
                    ARCHIVE_AGE_THREE_UNIT,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT));
            }
            archiveAge = (String) wm.getValue(ARCHIVE_AGE_FOUR);
            if (archiveAge == null) {
                wm.setValue(
                    ARCHIVE_AGE_FOUR,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE));
                wm.setValue(
                    ARCHIVE_AGE_FOUR_UNIT,
                    Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT));
            }

            // determine visibility of the more copies section
            String moreCopyStatus = (String)wm.getValue(MORE_COPY_STATUS);
            if (moreCopyStatus != null && moreCopyStatus.equals("visible")) {
                ((CCHiddenField)getChild(MORE_COPY_STATUS)).setValue("visible");
            } else {
                ((CCHiddenField)getChild(MORE_COPY_STATUS)).setValue("hidden");
            }
        } catch (SamFSException sfe) {
            SamUtil.processException(
                sfe,
                this.getClass(),
                "beginDisplay()",
                "Failed to populate archive information",
                getServerName());
            error = true;
            SamUtil.setErrorAlert(
                this,
                ALERT,
                "FSWizard.new.archiving.error.populate",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                getServerName());
        }

        // set the list of error messages and button labels
        StringBuffer buf = new StringBuffer();

        // toggle copy 3 and 4 labels
        buf.append(SamUtil
                  .getResourceString("FSWizard.new.archiving.copy3and4.show"))
            .append("##")
            .append(SamUtil
                  .getResourceString("FSWizard.new.archiving.copy3and4.hide"))
            .append("##")
            .append(SamUtil
                    .getResourceString("FSWizard.new.archiving.error.pool"))
            .append("##")
            .append(SamUtil
                    .getResourceString("FSWizard.new.archiving.error.age"));

        ((CCHiddenField)getChild(ERROR_MSG)).setValue(buf.toString());
    }

    public String getPageletUrl() {
        if (error) {
            return "/jsp/fs/wizardErrorPage.jsp";
        } else {
            return DEFAULT_URL;
        }
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }

    private boolean hasExistingPolicy() {
        String s = (String)RequestManager.getRequestContext().getRequest()
            .getAttribute("_existing_policies_");

        return "true".equals(s) ? true : false;
    }

    public boolean beginPolicyTypeExistingDisplay(ChildDisplayEvent cde)
        throws ModelControlException {
        return hasExistingPolicy();
    }

    public boolean beginExistingPolicyNameDisplay(ChildDisplayEvent cde)
        throws ModelControlException {
        return hasExistingPolicy();
    }

    public boolean beginPolicyTypeNewDisplay(ChildDisplayEvent cde)
        throws ModelControlException  {
        return hasExistingPolicy();
    }

    public boolean beginPolicyTypeNewLabelDisplay(ChildDisplayEvent cde)
        throws ModelControlException {
        return !hasExistingPolicy();
    }
}
