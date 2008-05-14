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

// ident	$Id: GrowWizardMethodView.java,v 1.1 2008/05/14 20:20:01 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.wizard.CCWizardPage;

/**
 * A ContainerView object for the pagelet for select grow device type of
 * the Grow File System Wizard.
 *
 */
public class GrowWizardMethodView
    extends RequestHandlingViewBase implements CCWizardPage {

    // The "logical" name for this page.
    public static final String PAGE_NAME = "GrowWizardMethodView";

    public static final String LABEL_METHOD = "LabelMethod";
    public static final String CHECKBOX_META = "CheckBoxMeta";
    public static final String CHECKBOX_DATA = "CheckBoxData";

    /**
     * Construct an instance with the specified properties.
     * A constructor of this form is required
     *
     * @param parent The parent view of this object.
     * @param name This view's name.
     */
    public GrowWizardMethodView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public GrowWizardMethodView(
        View parent, Model model, String name) {
        super(parent, name);
        TraceUtil.initTrace();
        setDefaultModel(model);
        registerChildren();
    }

    /**
     * Register each child view.
     */
    protected void registerChildren() {
        registerChild(LABEL_METHOD, CCLabel.class);
        registerChild(CHECKBOX_META, CCCheckBox.class);
        registerChild(CHECKBOX_DATA, CCCheckBox.class);
    }

    /**
     * Instantiate each child view.
     */
    protected View createChild(String name) {
        if (name.startsWith("Label")) {
            return new CCLabel(this, name, null);
        } else if (name.startsWith("CheckBox")) {
            return new CCCheckBox(
                this, name,
                Boolean.toString(true), Boolean.toString(false), false);
        } else {
            throw new IllegalArgumentException(
                "GrowWizardMethodView : Invalid child name [" +
                    name + "]");
        }
    }

    /**
     * Get the pagelet to use for the rendering of this instance.
     *
     * @return The pagelet to use for the rendering of this instance.
     */
    public String getPageletUrl() {
        return "/jsp/fs/GrowWizardMethod.jsp";
    }
}
