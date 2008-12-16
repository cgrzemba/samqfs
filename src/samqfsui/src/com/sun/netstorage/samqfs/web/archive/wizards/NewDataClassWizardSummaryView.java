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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: NewDataClassWizardSummaryView.java,v 1.13 2008/12/16 00:12:08 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.util.HashMap;

/**
 * A ContainerView object for the pagelet for the New Data Class Summary
 * page.
 *
 */
public class NewDataClassWizardSummaryView extends RequestHandlingViewBase
    implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "NewDataClassWizardSummaryView";

    // Child view names (i.e. display fields).
    public static final String LABEL = "Label";
    public static final String DATA_CLASS_NAME = "DataClassName";
    public static final String DESCRIPTION = "Description";
    public static final String POLICY_NAME = "PolicyName";
    public static final String POLICY_DESC = "PolicyDescription";
    public static final String STARTING_DIR = "StartingDir";
    public static final String NUM_COPIES = "NumCopies";
    public static final String MIN_SIZE_LABEL = "MinSize";
    public static final String MIN_SIZE = "MinSize";
    public static final String MAX_SIZE = "MaxSize";
    public static final String NAME_PATTERN = "NamePattern";
    public static final String OWNER = "Owner";
    public static final String GROUP = "Group";
    public static final String ALERT = "Alert";
    public static final String ACCESS_AGE = "AccessAge";
    public static final String COPY_INFO = "CopyInfo";

    private boolean previousError;

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public NewDataClassWizardSummaryView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewDataClassWizardSummaryView(
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
        registerChild(LABEL, CCLabel.class);
        registerChild(STARTING_DIR, CCStaticTextField.class);
        registerChild(DATA_CLASS_NAME, CCStaticTextField.class);
        registerChild(DESCRIPTION, CCStaticTextField.class);
        registerChild(POLICY_NAME, CCStaticTextField.class);
        registerChild(POLICY_DESC, CCStaticTextField.class);
        registerChild(NUM_COPIES, CCStaticTextField.class);
        registerChild(MIN_SIZE, CCStaticTextField.class);
        registerChild(MAX_SIZE, CCStaticTextField.class);
        registerChild(NAME_PATTERN, CCStaticTextField.class);
        registerChild(OWNER, CCStaticTextField.class);
        registerChild(GROUP, CCStaticTextField.class);
        registerChild(ALERT, CCAlertInline.class);
        registerChild(ACCESS_AGE, CCStaticTextField.class);
        registerChild(COPY_INFO, CCStaticTextField.class);
        TraceUtil.trace3("Exiting");
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        TraceUtil.trace3(new NonSyncStringBuffer("Entering: name is ").
            append(name).toString());

        View child = null;
        if (name.equals(ALERT)) {
           child = new CCAlertInline(this, name, null);

        } else if (name.equals(LABEL)) {
            child = new CCLabel(this, name, null);

        } else if (name.equals(POLICY_NAME) ||
            name.equals(POLICY_DESC) ||
            name.equals(DESCRIPTION) ||
            name.equals(STARTING_DIR) ||
            name.equals(NUM_COPIES) ||
            name.equals(MIN_SIZE) ||
            name.equals(MAX_SIZE) ||
            name.equals(NAME_PATTERN) ||
            name.equals(OWNER) ||
            name.equals(GROUP) ||
            name.equals(ACCESS_AGE) ||
            name.equals(COPY_INFO) ||
            name.equals(DATA_CLASS_NAME)) {
            child = new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "NewDataClassWizardSummaryView : Invalid child name [" +
                name + "]");
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
            url = "/jsp/archive/wizards/NewDataClassWizardSummary.jsp";
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
        showPreviousError(wizardModel);

        constructCopyInfoString(wizardModel);
        TraceUtil.trace3("Exiting");
    }

    private void showPreviousError(SamWizardModel wizardModel) {
        String error =
            (String) wizardModel.getValue(Constants.Wizard.WIZARD_ERROR);
        if (error != null) {
            if (error.equals(Constants.Wizard.WIZARD_ERROR_YES)) {
                previousError = true;

                String msgs = (String)
                    wizardModel.getValue(Constants.Wizard.ERROR_MESSAGE);
                int code = Integer.parseInt(
                    (String) wizardModel.getValue(Constants.Wizard.ERROR_CODE));
                SamUtil.setErrorAlert(
                    this,
                NewDataClassWizardSummaryView.ALERT,
                    "NewArchivePolWizard.error.carryover",
                    code,
                    msgs,
                    getServerName());
            }
        }
    }

    /**
     * To construct the copy information string in the summary page
     */
    private void constructCopyInfoString(SamWizardModel wizardModel) {
        int totalCopies =
            ((Integer) wizardModel.getValue(
                NewDataClassWizardImpl.TOTAL_COPIES)).intValue();
        if (totalCopies == 0) {
            return;
        }
        // Grab the HashMap that contains copy information from wizardModel
        HashMap copyNumberHashMap =
            (HashMap) wizardModel.getValue(NewDataClassWizardImpl.COPY_HASHMAP);

        NonSyncStringBuffer buf = new NonSyncStringBuffer();

        for (int i = 0; i < totalCopies; i++) {
            // Retrieve the CopyInfo data structure from the HashMap
            CopyInfo info =
                (CopyInfo) copyNumberHashMap.get(new Integer(i + 1));
            if (info == null) {
                // won't happen
                continue;
            }
            ArchiveCopyGUIWrapper myWrapper = info.getCopyWrapper();
            if (myWrapper == null) {
                // won't happen
                continue;
            }
            ArchivePolCriteriaCopy copy =
                myWrapper.getArchivePolCriteriaCopy();
            ArchiveCopy archiveCopy = myWrapper.getArchiveCopy();
            ArchiveVSNMap vsnMap = archiveCopy.getArchiveVSNMap();

            buf.append("<b>").append(
                SamUtil.getResourceString(
                    "archiving.copynumber", Integer.toString(i + 1)))
                .append(": </b>")
                .append(copy.getArchiveAge())
                .append(" ").append(
                SamUtil.getTimeUnitL10NString(copy.getArchiveAgeUnit()));

            // Comma and Media Type
            buf.append(", ");

            int mediaType =  vsnMap != null ? vsnMap.getArchiveMediaType() : -1;
            String mediaTypeString;
            if (mediaType < 0) {
                mediaTypeString =
                    SamUtil.getResourceString("common.mediatype.unknown");
            } else if (mediaType == BaseDevice.MTYPE_DISK) {
                mediaTypeString =
                    SamUtil.getResourceString("common.mediatype.disk");
            } else {
                mediaTypeString = SamUtil.getMediaTypeString(mediaType);
            }

            buf.append(mediaTypeString);
            if (i % 2 == 1) {
                buf.append("<br>");
            } else {
                buf.append("&nbsp;&nbsp;&nbsp;");
            }
        }
        ((CCStaticTextField) getChild(COPY_INFO)).setValue(buf.toString());
    }

    private String getServerName() {
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        String serverName = (String) wizardModel.getValue(
            Constants.PageSessionAttributes.SAMFS_SERVER_NAME);
        return serverName == null ? "" : serverName;
    }
}
