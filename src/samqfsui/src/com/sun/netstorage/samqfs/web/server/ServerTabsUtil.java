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

// ident	$Id: ServerTabsUtil.java,v 1.7 2008/12/16 00:12:25 am143972 Exp $

package com.sun.netstorage.samqfs.web.server;

import com.iplanet.jato.view.ViewBean;
import com.iplanet.jato.view.ViewBeanBase;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.sun.web.ui.model.CCNavNode;
import com.sun.web.ui.model.CCTabsModel;

public class ServerTabsUtil {

    /**
     * Constant Values for the Application Tabs
     */
    public static final int SERVER_SELECTION_TAB = 0;
    public static final int SITE_INFORMATION_TAB = 1;

    /**
     * static variable for all first level tabs
     */
    private static CCNavNode serverSelectionTab =
        new CCNavNode(
            ServerTabsUtil.SERVER_SELECTION_TAB,
            "serverSelectionTab",
            "serverSelectionTabToolTip",
            "serverSelectionTabStatus");

    private static CCNavNode siteInformationTab =
        new CCNavNode(
            ServerTabsUtil.SITE_INFORMATION_TAB,
            "siteInformationTab",
            "siteInformationTabToolTip",
            "siteInformationTabStatus");

    /**
     * The requisite default constructor
     */
    public ServerTabsUtil() {
    }

    /**
     * Creates the TabsModel and set the selectedTab
     * @param selectedTab id
     */
    public static CCTabsModel createTabsModel(int selectedTab) {
        CCTabsModel tabsModel = new CCTabsModel();
        tabsModel.removeAllNodes();
        populateTabsModel(selectedTab, tabsModel);
        return tabsModel;
    }

    /**
     * Populate the set of tabs and set the selected tab
     * @param: selectedTab: The tab to set as the selected tab
     */
    private static void populateTabsModel(
        int selectedTab, CCTabsModel tabsModel) {

        // Add the tabs
        tabsModel.addNode(serverSelectionTab);
        tabsModel.addNode(siteInformationTab);

        /*
         * Set the selected tab
         */
        tabsModel.setSelectedNode(selectedTab);
        return;
    }

    /**
     * Handles the Click Events during the tab navigation
     *
     * @param ViewBean: viewbean class
     * @param RequestInvocationEvent: Event
     * @param id: selected tab id
     */
    public static boolean handleTabClickRequest(
        ViewBeanBase viewBean,
        RequestInvocationEvent event,
        int id) {

        ViewBean targetView = null;
        boolean handled = false;

        switch (id) {
            case ServerTabsUtil.SERVER_SELECTION_TAB:
                targetView =
                    viewBean.getViewBean(ServerSelectionViewBean.class);
                handled = true;
                break;

            case ServerTabsUtil.SITE_INFORMATION_TAB:
                targetView =
                    viewBean.getViewBean(SiteInformationViewBean.class);
                handled = true;
                break;

            default:
                return handled;
        }

        targetView.forwardTo(viewBean.getRequestContext());
        return handled;
    }
}
