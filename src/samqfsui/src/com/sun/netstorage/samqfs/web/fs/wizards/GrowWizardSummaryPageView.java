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

// ident	$Id: GrowWizardSummaryPageView.java,v 1.16 2008/12/16 00:12:12 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.util.ArrayList;

/**
 * A ContainerView object for the pagelet for summary page of the Grow
 * File System Wizard.
 */
public class GrowWizardSummaryPageView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "GrowWizardSummaryPageView";

    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL_META = "LabelMeta";
    public static final String CHILD_LABEL_DATA = "LabelData";
    public static final String CHILD_DATA_FIELD = "DataField";
    public static final String CHILD_METADATA_FIELD = "MetadataField";
    public static final String CHILD_ALERT = "Alert";
    public static final String NONE_SELECTED = "NoneSelected";

    private boolean previous_error = false;

    // Should be in CCWizardPage ?
    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public GrowWizardSummaryPageView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public GrowWizardSummaryPageView(View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // Child manipulation methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(CHILD_LABEL_META, CCLabel.class);
        registerChild(CHILD_LABEL_DATA, CCLabel.class);
        registerChild(CHILD_DATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_METADATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        registerChild(NONE_SELECTED, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.equals(CHILD_METADATA_FIELD) ||
                   name.equals(CHILD_DATA_FIELD)) {
            return new CCSelectableList(this, name, null);
        } else if (name.startsWith("Label")) {
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child1 = new CCAlertInline(this, name, null);
            child1.setValue(CCAlertInline.TYPE_ERROR);
            return child1;
        } else if (name.equals(NONE_SELECTED)) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "GrowWizardSummaryPageView : Invalid child name ["
                + name + "]");
        }
    }

    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    // CCWizardPage methods
    // ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        String url = null;

        if (!previous_error) {
            url = "/jsp/fs/GrowWizardQFSSummaryPage.jsp";
        } else if (previous_error) {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel)getDefaultModel();

        CCHiddenField noneSelect = (CCHiddenField) getChild(NONE_SELECTED);
        if (isNothingSelected()) {
            SamUtil.setErrorAlert(
                this,
                CHILD_ALERT,
                "FSWizard.grow.noselection.error",
                0,
                "",
                (String) wizardModel.getValue(
                                Constants.Wizard.SERVER_NAME));
            noneSelect.setValue(Boolean.toString(true));
            return;
        } else {
            noneSelect.setValue(Boolean.toString(false));
        }

        // populate device selection list
        ArrayList selectedMetaDevicesList = getSelectedMetaDataDevices();
        String [] metaDataList =
            convertArrayListToArray(selectedMetaDevicesList, false);

        OptionList metaDataOptions =
            new OptionList(metaDataList, metaDataList);
        CCSelectableList selectableMetadataList = ((CCSelectableList)
            getChild(GrowWizardSummaryPageView.CHILD_METADATA_FIELD));
        selectableMetadataList.setOptions(metaDataOptions);
        int metaDataOptionSize = metaDataList.length;
        if (metaDataOptionSize <
            Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE) {
            selectableMetadataList.setSize(metaDataOptionSize);
        } else {
            selectableMetadataList.setSize(
                Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE);
        }

        // populate device selection list
        ArrayList selectedDevicesList = getSelectedDataDevices();
        String [] dataList =
            convertArrayListToArray(selectedDevicesList, false);

        // populate striped group selection list
        ArrayList selectedStripedGroupDevicesList =
                            getSelectedStripedGroupDevices();
        String [] stripedGroupList =
            convertArrayListToArray(selectedStripedGroupDevicesList, true);

        if (dataList.length > 0) {
            OptionList dataOptions = new OptionList(dataList, dataList);
            CCSelectableList selectDataOptions = ((CCSelectableList)
                getChild(GrowWizardSummaryPageView.CHILD_DATA_FIELD));
            selectDataOptions.setOptions(dataOptions);
            int dataOptionsSize = dataList.length;
            if (dataOptionsSize <
                    Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE) {
                selectDataOptions.setSize(dataOptionsSize);
            } else {
                selectDataOptions.setSize(
                    Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE);
            }
        } else if (stripedGroupList.length > 0) {
            OptionList dataOptions =
                new OptionList(stripedGroupList, stripedGroupList);
            CCSelectableList selectDataOptions = ((CCSelectableList)
                getChild(GrowWizardSummaryPageView.CHILD_DATA_FIELD));
            selectDataOptions.setOptions(dataOptions);
            int dataOptionsSize = stripedGroupList.length;
            if (dataOptionsSize <
                    Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE) {
                selectDataOptions.setSize(dataOptionsSize);
            } else {
                selectDataOptions.setSize(
                    Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE);
            }
        }

        String t = (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (t != null && t.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
            String msgs =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
            int code = Integer.parseInt(
                (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
            String errorSummary = "FSWizard.grow.error.step";
            previous_error = true;
            String errorDetails =
                (String) wizardModel.getValue(Constants.Wizard.ERROR_DETAIL);

            if (errorDetails != null) {
                errorSummary = (String)
                    wizardModel.getValue(Constants.Wizard.ERROR_SUMMARY);

                if (errorDetails.equals(Constants.Wizard.ERROR_INLINE_ALERT)) {
                    previous_error = false;
                } else {
                    previous_error = true;
                }
            }

            if (previous_error) {
                SamUtil.setErrorAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    code,
                    msgs,
                    (String) wizardModel.getValue(
                                Constants.Wizard.SERVER_NAME));
            } else {
                SamUtil.setWarningAlert(
                    this,
                    CHILD_ALERT,
                    errorSummary,
                    msgs);
            }
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * Utility method to return an Array of DataDevices
     * from an ArrayList of device paths
     */
    private String [] convertArrayListToArray(
        ArrayList selectedDevicePaths, boolean stripedGroups) {
        if (selectedDevicePaths == null) {
            return new String[0];
        }

        String[] selectedDevices = null;

        if (stripedGroups) {
            StringBuffer buf = new StringBuffer();
            for (int i = 0; i < selectedDevicePaths.size(); i++) {
                buf.append(
                    SamUtil.getResourceString(
                        "FSWizard.new.stripedGroup.deviceListing",
                        new String[] {Integer.toString(i)})).append("<br>");
                ArrayList groupMember = (ArrayList) selectedDevicePaths.get(i);
                groupMember =
                    groupMember == null ? new ArrayList() : groupMember;
                for (int j = 0; j < groupMember.size(); j++) {
                    buf.append(" &nbsp;&nbsp;&nbsp;").
                        append((String) groupMember.get(j)).
                        append("<br>");
                }
            }
            selectedDevices = buf.toString().split("<br>");
        } else {
            selectedDevices = new String[selectedDevicePaths.size()];
            for (int i = 0; i < selectedDevicePaths.size(); i++) {
                selectedDevices[i] = (String) selectedDevicePaths.get(i);
            }
        }

        return selectedDevices;
    }

    private boolean isNothingSelected() {
        return
            getSelectedDataDevices().size() == 0 &&
            getSelectedMetaDataDevices().size() == 0 &&
            getSelectedStripedGroupDevices().size() == 0;
    }

    private ArrayList getSelectedDataDevices() {
        SamWizardModel wizardModel = (SamWizardModel)getDefaultModel();
        ArrayList selectedDataDevicesList =
            (ArrayList) wizardModel.getValue(
                Constants.Wizard.SELECTED_DATADEVICES);
        selectedDataDevicesList =
            selectedDataDevicesList == null ?
                new ArrayList() : selectedDataDevicesList;
        return selectedDataDevicesList;
    }

    private ArrayList getSelectedMetaDataDevices() {
        SamWizardModel wizardModel = (SamWizardModel)getDefaultModel();
        ArrayList selectedMetaDevicesList =
            (ArrayList) wizardModel.getValue(
                Constants.Wizard.SELECTED_METADEVICES);
        selectedMetaDevicesList =
            selectedMetaDevicesList == null ?
                new ArrayList() : selectedMetaDevicesList;
        return selectedMetaDevicesList;
    }

    private ArrayList getSelectedStripedGroupDevices() {
        SamWizardModel wizardModel = (SamWizardModel)getDefaultModel();
        ArrayList selectedStripedGroupDevicesList =
            (ArrayList) wizardModel.getValue(
                Constants.Wizard.SELECTED_STRIPED_GROUP_DEVICES);
        selectedStripedGroupDevicesList =
            selectedStripedGroupDevicesList == null ?
                new ArrayList() : selectedStripedGroupDevicesList;
        return selectedStripedGroupDevicesList;
    }

    public boolean beginLabelMetaDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return getSelectedMetaDataDevices().size() != 0;
    }

    public boolean beginLabelDataDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return getSelectedDataDevices().size() != 0 ||
               getSelectedStripedGroupDevices().size() != 0;
    }

    public boolean beginMetadataFieldDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return getSelectedMetaDataDevices().size() != 0;
    }

    public boolean beginDataFieldDisplay(ChildDisplayEvent event)
        throws ModelControlException {
        return getSelectedDataDevices().size() != 0 ||
               getSelectedStripedGroupDevices().size() != 0;
    }
}
