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

// ident	$Id: NewCopySummary.java,v 1.18 2008/12/16 00:12:08 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
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
    public static final String LABEL_ARCHIVE_AGE = "LabelArchiveAge";
    public static final String LABEL_MEDIA_TYPE = "LabelMediaType";
    public static final String LABEL_INCLUDE_VOLS = "LabelIncludeVolumes";
    public static final String LABEL_OTHER = "LabelOther";

    // children (statictext)
    public static final String TEXT_ARCHIVE_AGE = "TextArchiveAge";
    public static final String TEXT_MEDIA_TYPE = "TextMediaType";
    public static final String TEXT_INCLUDE_VOLS = "TextIncludeVolumes";
    public static final String TEXT_OTHER = "TextOther";

    public NewCopySummary(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewCopySummary(View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        setDefaultModel(model);
        registerChildren();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(LABEL_ARCHIVE_AGE, CCLabel.class);
        registerChild(LABEL_MEDIA_TYPE, CCLabel.class);
        registerChild(LABEL_INCLUDE_VOLS, CCLabel.class);
        registerChild(TEXT_ARCHIVE_AGE, CCStaticTextField.class);
        registerChild(TEXT_MEDIA_TYPE, CCStaticTextField.class);
        registerChild(TEXT_INCLUDE_VOLS, CCStaticTextField.class);
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.startsWith("Label")) {
            return new CCLabel(this, name, null);
        } else if (name.startsWith("Text")) {
            return new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException(
                "invalid child '" + name + "'");
        }
    }

    /**
     * Called when page is displayed
     * @param event
     * @throws com.iplanet.jato.model.ModelControlException
     */
    public void beginDisplay(DisplayEvent event) throws ModelControlException {
        super.beginDisplay(event);

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // Archive Age
        long archiveAge = Long.parseLong((String)
            wizardModel.getValue(CopyParamsView.ARCHIVE_AGE));
        int ageUnitStr = Integer.parseInt((String)
            wizardModel.getValue(CopyParamsView.ARCHIVE_AGE_UNIT));
        ((CCStaticTextField) getChild(TEXT_ARCHIVE_AGE)).setValue(
            archiveAge + " " + SamUtil.getTimeUnitL10NString(ageUnitStr));

        // Media Type
        int mediaType = Integer.parseInt((String)
            wizardModel.getValue(NewCopyWizardImpl.MEDIA_TYPE));
        ((CCStaticTextField) getChild(TEXT_MEDIA_TYPE)).setValue(
            SamUtil.getMediaTypeString(mediaType));

        // Pool Name / Expressions
        String expression = (String)
            wizardModel.getValue(NewCopyWizardImpl.MEDIA_EXPRESSION);
        String newPoolName = getPoolName();

        CCStaticTextField text =
            (CCStaticTextField) getChild(TEXT_INCLUDE_VOLS);

        if (newPoolName.length() == 0) {
            String archiveMenu = (String) wizardModel.getValue(
                                    CopyParamsView.ARCHIVE_MEDIA_MENU);
            if (archiveMenu.equals(CopyParamsView.MENU_CREATE_EXP)) {
                // Create new expression
                text.setValue(
                    SamUtil.getResourceString(
                        "archiving.copy.newexp",
                        expression));
            } else if (archiveMenu.startsWith(CopyParamsView.MENU_ANY_VOL)) {
                // Use any available volumes
                text.setValue(
                    SamUtil.getResourceString(
                        "archiving.copy.mediaparam.anyvol",
                        SamUtil.getMediaTypeString(mediaType)));
            } else {
                String [] info = archiveMenu.split(",");
                // Use existing pool
                text.setValue(
                    SamUtil.getResourceString(
                        "archiving.copy.existpool",
                        info[0]));
            }
        } else {
            text.setValue(
                SamUtil.getResourceString(
                    "archiving.copy.newpool",
                    new String [] {
                        newPoolName,
                        expression}));
        }

        // Other Info such as volume reservation
        boolean reserved =
            Boolean.valueOf((String) wizardModel.getValue(
                    CopyParamsView.RESERVED)).booleanValue();
        ((CCStaticTextField) getChild(TEXT_OTHER)).setValue(
            reserved ?
                SamUtil.getResourceString("archiving.copy.mediaparam.reserved")
                : "");
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        return "/jsp/archive/wizards/NewCopySummary.jsp";
    }

    private String getPoolName() {
        String poolName = (String) ((SamWizardModel) getDefaultModel()).
                                    getValue(NewCopyWizardImpl.NEW_POOL_NAME);
        poolName = poolName == null ? "" : poolName;
        return poolName;
    }
}
