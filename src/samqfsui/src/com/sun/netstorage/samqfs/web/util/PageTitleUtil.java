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

// ident	$Id: PageTitleUtil.java,v 1.10 2008/03/17 14:43:57 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.ContainerView;
import com.iplanet.jato.view.ContainerViewBase;
import com.iplanet.jato.view.View;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.pagetitle.CCPageTitle;

/*
 * This is the utility Class to help to create the PageTitle component.
 */

public class PageTitleUtil {

    public static final String CHILD_PAGE_TITLE = "PageTitle";
    public static final String CHILD_PAGEVIEW_MENU = "PageViewMenu";
    public static final String CHILD_PAGEVIEW_MENUHREF = "PageViewMenuHref";
    public static final
        String CHILD_PAGEACTION_MENUHREF = "PageActionsMenuHref";
    /**
     * Constructiot
     */
    public PageTitleUtil() {
    }

    /**
     * Create the model
     * @param xml file
     */
    public static CCPageTitleModel createModel(String xmlFile) {
        // Construct a page title model using XML string.
        CCPageTitleModel pageTitleModel = new CCPageTitleModel(
            RequestManager.getRequestContext().getServletContext(),
            xmlFile);
        return pageTitleModel;
    }

    /**
     * check the child component is valid
     * @param CCPageTitleModel model
     * @param child component name
     */
    public static boolean isChildSupported(
        CCPageTitleModel model, String name) {

        if (name.equals(CHILD_PAGE_TITLE)
            || name.equals(CHILD_PAGEVIEW_MENUHREF)
            || name.equals(CHILD_PAGEACTION_MENUHREF)) {
            return true;
        } else if (model != null && model.isChildSupported(name)) {
            return true;
        } else {
            return false;
        }
    }

    /**
     * Register child component
     * @param viewbean
     * @param CCPageTitleModel model
     */
    public static void registerChildren(
        ContainerViewBase containerView, CCPageTitleModel model) {
        containerView.registerChild(CHILD_PAGE_TITLE, CCPageTitle.class);
        containerView.registerChild(CHILD_PAGEVIEW_MENUHREF, CCHref.class);
        containerView.registerChild(CHILD_PAGEACTION_MENUHREF, CCHref.class);
        model.registerChildren(containerView);
    }

    /**
     * Create the child component
     * @param ContainerView
     * @param CCPageTitleModel model
     * @param Child component name
     */
    public static View createChild(
        ContainerView view,
        CCPageTitleModel model,
        String name) {

        // Page title child
        if (name.equals(CHILD_PAGE_TITLE)) {
            CCPageTitle child = new CCPageTitle(view, model, name);
            return child;
        // jump href
        } else if (name.equals(CHILD_PAGEVIEW_MENUHREF)
            || name.equals(CHILD_PAGEACTION_MENUHREF)) {
            // HREFs
            CCHref child = new CCHref(view, name, null);
            return child;
        } else if (model.isChildSupported(name)) {
            // Create child from page title model.
            return model.createChild(view, name);
        } else {
            throw new IllegalArgumentException(
            "Invalid child name [" + name + "]");
        }
    }
}
