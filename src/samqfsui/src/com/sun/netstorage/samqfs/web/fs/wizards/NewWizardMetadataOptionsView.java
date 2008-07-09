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

// ident	$Id: NewWizardMetadataOptionsView.java,v 1.2 2008/07/09 22:20:57 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.wizard.CCWizardPage;

public class NewWizardMetadataOptionsView extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "NewWizardMetadataOptionsView";

    // child views
    public static final String ALERT = "Alert";
    public static final String METADATA_STORAGE = "metadataStorage";

    public static final String SEPARATE_DEVICE = "separate";
    public static final String SAME_DEVICE = "same";

    public NewWizardMetadataOptionsView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardMetadataOptionsView(View parent,
                                        Model model,
                                        String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");
        setDefaultModel(model);
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        TraceUtil.trace3("Entering");
        registerChild(ALERT, CCAlertInline.class);
        registerChild(METADATA_STORAGE, CCRadioButton.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(METADATA_STORAGE)) {
            CCRadioButton rb = new CCRadioButton(this, name, SAME_DEVICE);
            rb.setOptions(new OptionList(
                new String [] {"FSWizard.new.metadataoptions.separate",
                               "FSWizard.new.metadataoptions.same"},
                new String [] {SEPARATE_DEVICE, SAME_DEVICE}));

            return rb;
        } else {

            throw new IllegalArgumentException("Invalid child '" + "'");
        }
    }

    // implement CCWizardPage
    public String getPageletUrl() {
        return "/jsp/fs/NewWizardMetadataOptions.jsp";
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        populateDefaultValues(wm);
    }

    private void populateDefaultValues(SamWizardModel wm) {
    }
}
