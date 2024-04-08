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

// ident	$Id: WizardUtil.java,v 1.13 2008/12/16 00:12:27 am143972 Exp $

package com.sun.netstorage.samqfs.web.wizard;

import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.web.ui.model.CCWizardWindowModel;
import com.sun.web.ui.model.CCWizardWindowModelInterface;
import java.util.ArrayList;
import java.util.Iterator;

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

    /**
     * This utility method inserts an array containing page ids to the main
     * page id array before or after a specified page.
     *
     * @param original - the main array to be added to
     * @param insert - the new array to insert in the original
     * @param page - the page to insert before or after
     * @param before - true to insert before or false to insert after.
     * @return
     */
    public static int [] insertPagesBefore(int [] original,
                                      int [] insert,
                                      int page,
                                      boolean before) {
        int [] result = new int[original.length + insert.length];

        // find the index of page on the original array
        int pageIndex = 0;
        boolean found = false;
        for (int i = 0; !found && i < original.length; i++) {
            if (original[i] == page) {
                pageIndex = i;
                found = true;
            }
        }

        // adjust the uptoIndex to account for whether we are inserting the
        // new array before or after the give page
        int uptoIndex = pageIndex;
        if (before) uptoIndex = pageIndex -1;

        // copy the first half of the array
        int resultIndex = 0;
        for (int i = 0; i <= uptoIndex; i++, resultIndex++) {
            result[resultIndex] = original[i];

        }

        // copy the insert array to the result
        for (int i = 0; i < insert.length; i++, resultIndex++) {
            result[resultIndex] = insert[i];
        }

        // copy the remaining part of the orginal array
        for (int i = uptoIndex + 1; i < original.length; i++, resultIndex++) {
            result[resultIndex] = original[i];

        }

        // if before, then copy the insert array into result
        return result;
    }

    /**
     * remove the given page id from the array of pages provided.
     *
     * @param int pageId - page to remove
     *  NOTE: If pageId is a striped group or object group
     *  page, all occurences of the page will be removed.
     * @param in [] pages - the array to remove the page from
     */
    public static int [] removePage(int pageId, int [] pages) {
        ArrayList<Integer> list =  new ArrayList<Integer>();
        for (int i = 0; i < pages.length; i++) {
            if (pages[i] != pageId)
                list.add(pages[i]);
        }

        int [] newPages = new int[list.size()];
        Iterator<Integer> it = list.iterator();
        int counter = 0;

        // copy the list back to the arrary
        while (it.hasNext()) {
            newPages[counter++] = it.next();
        }

        return newPages;
    }
}
