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

// ident        $Id: SharedFSTabBean.java,v 1.1 2008/06/11 16:58:00 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.web.ui.component.Tab;
import com.sun.web.ui.component.TabSet;

public class SharedFSTabBean {

    public static final String TAB_SUMMARY = "summary";
    public static final String TAB_CLIENT = "client";
    public static final String TAB_STORAGE_NODE = "storagenode";

    private static final String [][] tabsInfo = new String [][] {
        {TAB_SUMMARY, "SharedFS.tab.summary",
                    "/faces/jsp/fs/SharedFSSummary.jsp"},
        {TAB_CLIENT, "SharedFS.tab.clients",
                    "/faces/jsp/fs/SharedFSClient.jsp"},
        {TAB_STORAGE_NODE, "SharedFS.tab.sn",
                    "/faces/jsp/fs/SharedFSStorageNode.jsp"}
    };

    public SharedFSTabBean() {
    }

    public TabSet getMyTabSet(boolean showStorageNodes) {
        TabSet tabSet = new TabSet();
        tabSet.setId("SharedFSTabSet");

        for (int i = 0; i < tabsInfo.length; i++) {
            if (tabsInfo[i][0].equals(tabsInfo[2][0]) && !showStorageNodes) {
                break;
            }
            Tab tab = new Tab(JSFUtil.getMessage(tabsInfo[i][1]));
            tab.setId(tabsInfo[i][0]);
            tab.setUrl(tabsInfo[i][2]);
            tabSet.getChildren().add(tab);
        }

        return tabSet;
    }
}
