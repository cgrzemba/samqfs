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

// ident	$Id: NewWizardAcceptQFSDefaultsView.java,v 1.1 2008/06/25 23:23:25 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

public class NewWizardAcceptQFSDefaultsView extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "NewWizardAcceptQFSDefaultsView";

    // child views
    public static final String ALERT = "Alert";
    public static final String LABEL = "Label";
    public static final String TEXT = "Text";
    public static final String ACCEPT_CHANGE = "acceptChangeRadioButton";
    public static final String METADATA_STORAGE = "metadataText";
    public static final String ALLOCATION_METHOD = "allocationMethodText";
    public static final String BLOCK_SIZE = "blockSizeText";
    public static final String BLOCKS_PER_DEVICE = "blocksPerDeviceText";
    public static final String DISCOVERY_MESSAGE = "discoveryMessage";

    // values for the accept/change defaults radio button
    public static final String ACCEPT = "accept";
    public static final String CHANGE = "change";

    public NewWizardAcceptQFSDefaultsView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardAcceptQFSDefaultsView(View parent,
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
        registerChild(ACCEPT_CHANGE, CCRadioButton.class);
        registerChild(DISCOVERY_MESSAGE, CCHiddenField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.endsWith(LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.endsWith(TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(ACCEPT_CHANGE)) {
            CCRadioButton rb = new CCRadioButton(this, name, ACCEPT);
            rb.setOptions(new OptionList(
                new String [] {"FSWizard.new.qfsdefaults.accept",
                               "FSWizard.new.qfsdefaults.change"},
                new String [] {ACCEPT, CHANGE}));

           return rb;
        } else if (name.equals(DISCOVERY_MESSAGE)) {
            return new CCHiddenField(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        populateDefaultValues();

        ((CCHiddenField)getChild(DISCOVERY_MESSAGE))
            .setValue(SamUtil.getResourceString("discovery.disk"));
    }

    private void populateDefaultValues() {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        Boolean hpc = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_HPC);
        Boolean hafs = (Boolean)wm.getValue(CreateFSWizardImpl.POPUP_HAFS);

        String blockSizeString = new StringBuffer("64 ")
            .append(SamUtil.getResourceString("common.unit.size.kb"))
            .toString();

        // block size
        ((CCStaticTextField)getChild(BLOCK_SIZE)).setValue(blockSizeString);

        // metadata and data on separate devices
        if (hpc.booleanValue() || hafs.booleanValue()) {
            // metadata storage
            ((CCStaticTextField)getChild(METADATA_STORAGE))
                .setValue("FSWizard.new.blockallocation.mdstorage.separate");

            // allocation method
            ((CCStaticTextField)getChild(ALLOCATION_METHOD))
                .setValue("FSWizard.new.allocationmethod.dual");

            // blocks per device
            if (hafs.booleanValue()) {
                ((CCStaticTextField)getChild(BLOCKS_PER_DEVICE)).setValue("2");
            } else {
                ((CCStaticTextField)getChild(BLOCKS_PER_DEVICE)).setValue("0");
            }
        } else { // metadata and data on the same device(s)
            // metadata storage
            ((CCStaticTextField)getChild(METADATA_STORAGE))
                .setValue("FSWizard.new.blockallocation.mdstorage.same");

            // allocation method
            ((CCStaticTextField)getChild(ALLOCATION_METHOD))
                .setValue("FSWizard.new.allocationmethod.single");

            // blocks per device
            ((CCStaticTextField)getChild(BLOCKS_PER_DEVICE)).setValue("0");
        }
    }

    // implement CCWizardPage
    public String getPageletUrl() {
        return "/jsp/fs/NewWizardAcceptQFSDefaults.jsp";
    }
}
