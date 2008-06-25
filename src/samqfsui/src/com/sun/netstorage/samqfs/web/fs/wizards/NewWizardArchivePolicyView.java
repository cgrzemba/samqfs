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

// ident	$Id: NewWizardArchivePolicyView.java,v 1.1 2008/06/25 23:23:25 kilemba Exp $

package com.sun.netstorage.samqfs.web.fs.wizards;

import com.iplanet.jato.model.Model;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.wizard.CCWizardPage;

public class NewWizardArchivePolicyView extends RequestHandlingViewBase
    implements CCWizardPage {

    public static final String PAGE_NAME = "NewWizardArchivePolicyView";

    // child views
    public static final String ALERT = "Alert";

    public NewWizardArchivePolicyView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public NewWizardArchivePolicyView(View parent, Model model, String name) {

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
        TraceUtil.trace3("Exiting");
    }

    public View createChild(String name) {
        if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else {
            throw new IllegalArgumentException("Invalid child '" + "'");
        }
    }

    // implement CCWizardPage
    public String getPageletUrl() {
        return "/jsp/fs/NewWizardArchivePolicy.jsp";
    }
}
