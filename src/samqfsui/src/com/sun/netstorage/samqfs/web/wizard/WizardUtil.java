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

// ident	$Id: WizardUtil.java,v 1.9 2008/05/16 18:39:07 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;

public class WizardUtil {

    /**
     * This factory method will create a CCWizardWindowModel which will
     * be used in the caller (usually a ViewBean) to create a
     * CCWizardWindow child. This method will typically be called in
     * createChild():
     * <pre>
     *
     * ...
     * else if (name.equals(CHILD_WIZARDWINDOW)) {
     *
     *   CCWizardWindowModel wizWinModel = WizardUtil.createModel(
     *   "NewVSNPoolWizard.title",
     *   "com.sun.netstorage.samqfs.web.archive.wizards.NewVSNPoolWizardImpl",
     *   "NewVSNPoolWizardImpl");
     *
     *   CCWizardWindow child = new CCWizardWindow(this, model, name,
     *   "NewVSNPoolWizard.button.label");
     *   return child;
     * }
     * < /pre>
     */
    public static CCWizardWindowModel createModel(
        String title,
        String className,
        String implClass) {

        CCWizardWindowModel wizWinModel = new CCWizardWindowModel();

        // common to all wizards within SAM-FS

        wizWinModel.setValue(
            CCWizardWindowModelInterface.MASTHEAD_SRC,
            Constants.SecondaryMasthead.PRODUCT_NAME_SRC);
        wizWinModel.setValue(
            CCWizardWindowModelInterface.MASTHEAD_ALT,
            Constants.SecondaryMasthead.PRODUCT_NAME_ALT);
        wizWinModel.setValue(
            CCWizardWindowModelInterface.BASENAME,
            Constants.ResourceProperty.BASE_NAME);
        wizWinModel.setValue(
            CCWizardWindowModelInterface.BUNDLEID,
            Constants.Wizard.BUNDLEID);

        // specific to a wizard
        wizWinModel.setValue(CCWizardWindowModelInterface.TITLE, title);
        wizWinModel.setValue(
            CCWizardWindowModelInterface.WIZARD_CLASS_NAME,
            className);
        wizWinModel.setValue(
            CCWizardWindowModelInterface.WIZARD_NAME,
            implClass);

        return wizWinModel;
    }
}
