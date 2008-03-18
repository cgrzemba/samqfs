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

// ident	$Id: NewCopySummary.java,v 1.15 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for summary page.
 *
 */
public class NewCopySummary extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewCopySummary";

    // children (label)
    public static final String ARCHIVE_AGE_LABEL = "ArchiveAgeLabel";
    public static final String ARCHIVE_TYPE_LABEL = "ArchiveTypeLabel";
    public static final String MEDIA_TYPE_LABEL = "MediaTypeLabel";
    public static final String VSN_POOL_NAME_LABEL = "VSNPoolNameLabel";
    public static final String SPECIFY_VSN_LABEL = "SpecifyVSNLabel";
    public static final String RESERVE_LABEL = "ReserveLabel";
    public static final String OFFLINE_COPY_LABEL = "OfflineCopyLabel";
    public static final String MAX_DRIVE_LABEL = "MaxDriveLabel";
    public static final String MIN_DRIVE_LABEL = "MinDriveLabel";
    public static final String SAVE_METHOD_LABEL = "SaveMethodLabel";
    public static final String DISK_VSN_LABEL = "DiskVSNLabel";
    public static final
        String DISK_ARCHIVE_PATH_LABEL = "DiskArchivePathLabel";
    public static final String START_AGE_LABEL = "StartAgeLabel";
    public static final String START_COUNT_LABEL = "StartCountLabel";
    public static final String START_SIZE_LABEL = "StartSizeLabel";
    public static final String RECYCLE_HWM_LABEL = "RecycleHWMLabel";

    // children (statictext)
    public static final String ARCHIVE_AGE = "ArchiveAge";
    public static final String ARCHIVE_TYPE = "ArchiveType";
    public static final String MEDIA_TYPE = "MediaType";
    public static final String VSN_POOL_NAME = "VSNPoolName";
    public static final String SPECIFY_VSN = "SpecifyVSN";
    public static final String RESERVE = "Reserve";
    public static final String OFFLINE_COPY = "OfflineCopy";
    public static final String MAX_DRIVE = "MaxDrive";
    public static final String MIN_DRIVE = "MinDrive";
    public static final String SAVE_METHOD = "SaveMethod";
    public static final String DISK_VSN = "DiskVSN";
    public static final String DISK_ARCHIVE_PATH = "DiskArchivePath";
    public static final String START_AGE = "StartAge";
    public static final String START_COUNT = "StartCount";
    public static final String START_SIZE = "StartSize";
    public static final String RECYCLE_HWM = "RecycleHWM";

    public NewCopySummary(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewCopySummary(View parent, Model model, String name) {
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
        registerChild(ARCHIVE_AGE_LABEL, CCLabel.class);
        registerChild(ARCHIVE_TYPE_LABEL, CCLabel.class);
        registerChild(MEDIA_TYPE_LABEL, CCLabel.class);
        registerChild(VSN_POOL_NAME_LABEL, CCLabel.class);
        registerChild(SPECIFY_VSN_LABEL, CCLabel.class);
        registerChild(RESERVE_LABEL, CCLabel.class);
        registerChild(OFFLINE_COPY_LABEL, CCLabel.class);
        registerChild(MAX_DRIVE_LABEL, CCLabel.class);
        registerChild(MIN_DRIVE_LABEL, CCLabel.class);
        registerChild(SAVE_METHOD_LABEL, CCLabel.class);
        registerChild(DISK_VSN_LABEL, CCLabel.class);
        registerChild(DISK_ARCHIVE_PATH_LABEL, CCLabel.class);
        registerChild(START_AGE_LABEL, CCLabel.class);
        registerChild(START_COUNT_LABEL, CCLabel.class);
        registerChild(START_SIZE_LABEL, CCLabel.class);
        registerChild(RECYCLE_HWM_LABEL, CCLabel.class);
        registerChild(ARCHIVE_AGE, CCStaticTextField.class);
        registerChild(ARCHIVE_TYPE, CCStaticTextField.class);
        registerChild(MEDIA_TYPE, CCStaticTextField.class);
        registerChild(VSN_POOL_NAME, CCStaticTextField.class);
        registerChild(SPECIFY_VSN, CCStaticTextField.class);
        registerChild(RESERVE, CCStaticTextField.class);
        registerChild(OFFLINE_COPY, CCStaticTextField.class);
        registerChild(MAX_DRIVE, CCStaticTextField.class);
        registerChild(MIN_DRIVE, CCStaticTextField.class);
        registerChild(SAVE_METHOD, CCStaticTextField.class);
        registerChild(DISK_VSN, CCStaticTextField.class);
        registerChild(DISK_ARCHIVE_PATH, CCStaticTextField.class);
        registerChild(START_AGE, CCStaticTextField.class);
        registerChild(START_COUNT, CCStaticTextField.class);
        registerChild(START_SIZE, CCStaticTextField.class);
        registerChild(RECYCLE_HWM, CCStaticTextField.class);
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.equals(ARCHIVE_AGE_LABEL) ||
            name.equals(ARCHIVE_TYPE_LABEL) ||
            name.equals(MEDIA_TYPE_LABEL) ||
            name.equals(VSN_POOL_NAME_LABEL) ||
            name.equals(SPECIFY_VSN_LABEL) ||
            name.equals(RESERVE_LABEL) ||
            name.equals(OFFLINE_COPY_LABEL) ||
            name.equals(MAX_DRIVE_LABEL) ||
            name.equals(MIN_DRIVE_LABEL) ||
            name.equals(SAVE_METHOD_LABEL) ||
            name.equals(DISK_VSN_LABEL) ||
            name.equals(DISK_ARCHIVE_PATH_LABEL) ||
            name.equals(START_AGE_LABEL) ||
            name.equals(START_COUNT_LABEL) ||
            name.equals(START_SIZE_LABEL) ||
            name.equals(RECYCLE_HWM_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(ARCHIVE_AGE) ||
            name.equals(ARCHIVE_TYPE) ||
            name.equals(MEDIA_TYPE) ||
            name.equals(VSN_POOL_NAME) ||
            name.equals(SPECIFY_VSN) ||
            name.equals(RESERVE) ||
            name.equals(OFFLINE_COPY) ||
            name.equals(MAX_DRIVE) ||
            name.equals(MIN_DRIVE) ||
            name.equals(SAVE_METHOD) ||
            name.equals(DISK_VSN) ||
            name.equals(DISK_ARCHIVE_PATH) ||
            name.equals(START_AGE) ||
            name.equals(START_COUNT) ||
            name.equals(START_SIZE) ||
            name.equals(RECYCLE_HWM)) {
            return new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "invalid child '" + name + "'");
        }
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        SamWizardModel wm = (SamWizardModel) getDefaultModel();
        String url;

        if (((String) wm.getValue(NewCopyWizardImpl.MEDIA_TYPE)).
            equals(NewCopyWizardImpl.TAPE)) {
            url = "/jsp/archive/NewCopyTapeSummary.jsp";
        } else {
            url = "/jsp/archive/NewCopyDiskSummary.jsp";
        }
        return url;
    }

    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);

        SamWizardModel wm = (SamWizardModel) getDefaultModel();

        String error = (String) wm.getValue(Constants.Wizard.WIZARD_ERROR);
        if (error != null && error.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
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
        }
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
