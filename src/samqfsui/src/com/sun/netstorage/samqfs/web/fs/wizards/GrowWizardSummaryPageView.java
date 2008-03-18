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

// ident	$Id: GrowWizardSummaryPageView.java,v 1.13 2008/03/17 14:43:36 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCSelectableList;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for summary page of the Grow
 * File System Wizard.
 */
public class GrowWizardSummaryPageView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "GrowWizardSummaryPageView";

    // Child view names (i.e. display fields).
    public static final String CHILD_LABEL = "Label";
    public static final String CHILD_DATA_FIELD = "DataField";
    public static final String CHILD_METADATA_FIELD = "MetadataField";
    public static final String CHILD_ALERT = "Alert";

    private boolean previous_error = false;
    private String fsType = GrowWizardImpl.FSTYPE_FS;

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
        registerChild(CHILD_LABEL, CCLabel.class);
        registerChild(CHILD_DATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_METADATA_FIELD, CCSelectableList.class);
        registerChild(CHILD_ALERT, CCAlertInline.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.equals(CHILD_METADATA_FIELD) ||
                   name.equals(CHILD_DATA_FIELD)) {
            return new CCSelectableList(this, name, null);
        } else if (name.equals(CHILD_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(CHILD_ALERT)) {
            CCAlertInline child1 = new CCAlertInline(this, name, null);
            child1.setValue(CCAlertInline.TYPE_ERROR);
            return child1;
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
        TraceUtil.trace3("Entering");
        String url = null;

        if (!previous_error) {
            if (fsType.equals(GrowWizardImpl.FSTYPE_QFS)) {
                url = "/jsp/fs/GrowWizardQFSSummaryPage.jsp";
            } else {
                url = "/jsp/fs/GrowWizardFSSummaryPage.jsp";
            }
        } else if (previous_error) {
            url = "/jsp/fs/wizardErrorPage.jsp";
        }
        TraceUtil.trace3("Exiting");
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel)getDefaultModel();
        fsType = (String) wizardModel.getValue(GrowWizardImpl.FSTYPE_KEY);

        // populate device selection list
        if (fsType.equals(GrowWizardImpl.FSTYPE_QFS)) {
            String metaDataDevice = (String) wizardModel.getWizardValue(
                GrowWizardSummaryPageView.CHILD_METADATA_FIELD);
            String[] metaDataList = metaDataDevice.split("<br>");
            for (int i = 0; i < metaDataList.length; i ++) {
                int index = metaDataList[i].lastIndexOf('/');
                String pathString = metaDataList[i].substring(index + 1);
                metaDataList[i] = pathString;
            }
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
        }

        String dataDevice = (String) wizardModel.getWizardValue(
            GrowWizardSummaryPageView.CHILD_DATA_FIELD);
        String[] dataList = dataDevice.split("<br>");
        for (int i = 0; i < dataList.length; i ++) {
            int index = dataList[i].lastIndexOf('/');
            String pathString = dataList[i].substring(index + 1);
            dataList[i] = pathString;
        }

        OptionList dataOptions = new OptionList(dataList, dataList);
        CCSelectableList selectDataOptions = ((CCSelectableList)
            getChild(GrowWizardSummaryPageView.CHILD_DATA_FIELD));
        selectDataOptions.setOptions(dataOptions);
        int dataOptionsSize = dataList.length;
        if (dataOptionsSize < Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE) {
            selectDataOptions.setSize(dataOptionsSize);
        } else {
            selectDataOptions.setSize(
                Constants.Wizard.DEVICE_SELECTION_LIST_MAX_SIZE);
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
}
