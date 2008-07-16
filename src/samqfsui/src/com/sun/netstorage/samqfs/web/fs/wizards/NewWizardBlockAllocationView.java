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

// ident	$Id: NewWizardBlockAllocationView.java,v 1.3 2008/07/16 21:55:56 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;

public class NewWizardBlockAllocationView extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "NewWizardBlockAllocationView";

    // child views
    public static final String ALERT = "Alert";
    public static final String LABEL = "Label";
    public static final String ALLOCATION_METHOD = "allocationMethodRadio";
    public static final String ALLOCATION_METHOD_TEXT = "allocationMethodText";
    public static final String BLOCK_SIZE = "blockSizeText";
    public static final String BLOCK_SIZE_DROPDOWN = "blockSizeDropDown";
    public static final String BLOCK_SIZE_UNIT = "blockSizeUnit";
    public static final String BLOCKS_PER_DEVICE = "blocksPerDeviceText";
    public static final String STRIPED_GROUPS = "stripedGroupText";

    // key for the final block size used by the wizard model
    public static final String BLOCK_SIZE_KB = "block_size_in_kb";

    // allocation method values
    public static final String SINGLE = "single";
    public static final String DUAL = "dual";
    public static final String STRIPED = "striped";

    public NewWizardBlockAllocationView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardBlockAllocationView(View parent, Model model, String name) {
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
        registerChild(ALLOCATION_METHOD, CCRadioButton.class);
        registerChild(BLOCK_SIZE, CCTextField.class);
        registerChild(BLOCK_SIZE_DROPDOWN, CCDropDownMenu.class);
        registerChild(BLOCK_SIZE_UNIT, CCDropDownMenu.class);
        registerChild(BLOCKS_PER_DEVICE, CCTextField.class);
        registerChild(STRIPED_GROUPS, CCTextField.class);
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(ALLOCATION_METHOD)) {
            CCRadioButton rb = new CCRadioButton(this, name, SINGLE);
            rb.setOptions(new OptionList(
                new String [] {"FSWizard.new.allocationmethod.single",
                               "FSWizard.new.allocationmethod.dual",
                               "FSWizard.new.allocationmethod.striped"},
                new String [] {SINGLE, DUAL, STRIPED}));

            return rb;
        } else if (name.endsWith(LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(BLOCK_SIZE) ||
            name.equals(BLOCKS_PER_DEVICE) ||
            name.equals(STRIPED_GROUPS)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(BLOCK_SIZE_UNIT) ||
            name.equals(BLOCK_SIZE_DROPDOWN)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(ALLOCATION_METHOD_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + "'");
        }
    }

    public void beginDisplay(DisplayEvent evt)throws ModelControlException {
        // set block size units drop down options
        CCDropDownMenu menu = (CCDropDownMenu)getChild(BLOCK_SIZE_UNIT);
        menu.setOptions(new OptionList(
            new String [] {"common.unit.size.kb", "common.unit.size.mb"},
            new String [] {"1", "2"}));

        menu = (CCDropDownMenu)getChild(BLOCK_SIZE_DROPDOWN);
        menu.setOptions(new OptionList(
            new String [] {"samqfsui.fs.wizards.new.DAUPage.option.16",
                           "samqfsui.fs.wizards.new.DAUPage.option.32",
                           "samqfsui.fs.wizards.new.DAUPage.option.64"},
            new String [] {"16", "32", "64"}));
        SamWizardModel wm = (SamWizardModel)getDefaultModel();

        // testing ...
        CCTextField textField = (CCTextField)getChild(BLOCKS_PER_DEVICE);
        textField.setExtraHtml("style=\"display:\"");

        // if separate metadata device, hide striped group information as well
        // as block size text and unit by default
        if (isSeparateMetadata()) {
            String displayNone = "style=\"display:none\"";
            ((CCLabel)getChild("stripedGroupLabel")).setExtraHtml(displayNone);
            ((CCTextField)getChild(STRIPED_GROUPS)).setExtraHtml(displayNone);
            ((CCTextField)getChild(BLOCK_SIZE)).setExtraHtml(displayNone);
            ((CCDropDownMenu)getChild(BLOCK_SIZE_UNIT)).setExtraHtml(displayNone);
        }
            
    }

    // implement CCWizardPage
    public String getPageletUrl() {
        return "/jsp/fs/NewWizardBlockAllocation.jsp";
    }

    private void populateDefaultValues(SamWizardModel wm) {
    }

    private boolean isSeparateMetadata() {
        SamWizardModel wm = (SamWizardModel)getDefaultModel();
        String mdOptions = (String)
            wm.getValue(NewWizardMetadataOptionsView.METADATA_STORAGE);
        
        return NewWizardMetadataOptionsView.SEPARATE_DEVICE.equals(mdOptions);
    }


    // only display the following fields when separate data/metadata is
    // selected
    public boolean beginAllocationMethodRadioDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return isSeparateMetadata();
    }

    public boolean beginStripedGroupLabelDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return isSeparateMetadata();
    }

    public boolean beginStripedGroupTextDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return isSeparateMetadata();
    }

    public boolean beginAllocationMethodTextDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return !isSeparateMetadata();
    }
 
    public boolean beginBlockSizeTextDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return isSeparateMetadata();
    }

    public boolean beginBlockSizeUnitDisplay(ChildDisplayEvent evt)
        throws ModelControlException {
        return isSeparateMetadata();
    }
}
