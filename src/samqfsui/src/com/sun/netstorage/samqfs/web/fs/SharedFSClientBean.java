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

// ident        $Id: SharedFSClientBean.java,v 1.9 2008/10/02 03:00:24 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.data.provider.TableDataProvider;
import com.sun.data.provider.impl.ObjectArrayDataProvider;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.model.SharedHostInfo;
import com.sun.netstorage.samqfs.web.model.fs.SharedFSFilter;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.web.ui.component.Hyperlink;
import com.sun.web.ui.model.Option;
import com.sun.web.ui.model.OptionTitle;


public class SharedFSClientBean {

    /** Page Menu Options */
    protected String [][] menuOptions = new String [][] {
        {"SharedFS.operation.editmo", "editmo"},
        {"SharedFS.operation.mount", "mount"},
        {"SharedFS.operation.unmount", "unmount"},
        {"SharedFS.operation.enableaccess", "enableaccess"},
        {"SharedFS.operation.disableaccess", "disableaccess"}
    };

    // Basic Filter Menu
    protected String [][] filterOptions = new String [][] {
        {"SharedFS.filter.all", SharedFSFilter.FILTER_NONE},
        {"SharedFS.state.ok", SharedFSFilter.FILTER_OK},
        {"SharedFS.state.unmounted", SharedFSFilter.FILTER_UNMOUNTED},
        {"SharedFS.state.accessdisabled", SharedFSFilter.FILTER_DISABLED},
        {"SharedFS.state.inerror", SharedFSFilter.FILTER_IN_ERROR},
        {"SharedFS.filter.custom", SharedFSFilter.FILTER_CUSTOM}
    };

    protected boolean archive = false;
    private SharedHostInfo [] infos = null;
    private SharedFSFilter [] filters = null;

    public SharedFSClientBean() {
    }

    public Hyperlink [] getBreadCrumbs(String fsName){
        Hyperlink [] links = new Hyperlink[3];
        for (int i = 0; i < links.length; i++){
            links[i] = new Hyperlink();
            links[i].setImmediate(true);
            links[i].setId("breadcrumbid" + i);
            if (i == 0){
                // Back to FS Summary
                links[i].setText(JSFUtil.getMessage("FSSummary.title"));
                links[i].setUrl(
                    "/fs/FSSummary?" +
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME + "=" +
                    JSFUtil.getServerName());
            } else if (i == 1){
                // Back to Shared FS Summary
                links[i].setText(JSFUtil.getMessage("SharedFS.pagetitle"));
                links[i].setUrl(
                    "SharedFSSummary.jsp?" +
                     Constants.PageSessionAttributes.SAMFS_SERVER_NAME + "=" +
                     JSFUtil.getServerName() + "&" +
                     Constants.PageSessionAttributes.FILE_SYSTEM_NAME + "=" +
                     fsName);
            } else {
                // client / sn page
                links[i].setText(
                    JSFUtil.getMessage("SharedFS.tab.clients"));
            }
        }

        return links;
    }

    public void populate(
        SharedHostInfo [] infos,
        boolean showArchive, SharedFSFilter [] filters) {
        this.archive = showArchive;
        this.infos = infos;
        this.filters = filters;
    }

    public TableDataProvider getClientList() {
        TraceUtil.trace3("client: getClientList: info is " +
            (infos != null ? "not " : "") + "null!");
        return
            infos == null?
                new ObjectArrayDataProvider(new SharedHostInfo[0]) :
                new ObjectArrayDataProvider(infos);
    }

    public Option [] getJumpMenuOptions() {
        Option[] jumpMenuOptions = new Option[menuOptions.length + 1];
        jumpMenuOptions[0] =
                new OptionTitle(JSFUtil.getMessage("common.dropdown.header"));
        for (int i = 1; i < menuOptions.length + 1; i++) {
            jumpMenuOptions[i] =
                new Option(
                    menuOptions[i - 1][1],
                    JSFUtil.getMessage(menuOptions[i - 1][0]));
        }

        return jumpMenuOptions;
    }

    public Option [] getFilterMenuOptions() {
        Option[] filterMenuOptions = new Option[filterOptions.length];
        for (int i = 0; i < filterOptions.length; i++) {
            filterMenuOptions[i] =
                new Option(
                    filterOptions[i][1],
                    JSFUtil.getMessage(filterOptions[i][0]));
        }

        return filterMenuOptions;
    }

    public String getClientTableTitle() {
        if (filters.length == 1) {
            String criteria = filters[0].toString();
            String optionSelected = null;
            if (SharedFSFilter.FILTER_OK.equals(criteria)) {
                optionSelected = filterOptions[1][0];
            } else if (SharedFSFilter.FILTER_UNMOUNTED.equals(criteria)) {
                optionSelected = filterOptions[2][0];
            } else if (SharedFSFilter.FILTER_DISABLED.equals(criteria)) {
                optionSelected = filterOptions[3][0];
            } else if (SharedFSFilter.FILTER_IN_ERROR.equals(criteria)) {
                optionSelected = filterOptions[4][0];
            } else {
                // custom filter with one criterion
                return JSFUtil.getMessage(
                    "SharedFS.title.table.clients.filtered",
                    new String [] {
                        JSFUtil.getMessage(filterOptions[5][0])});
            }

            // for preset criteria
            return JSFUtil.getMessage(
                "SharedFS.title.table.clients.filtered",
                new String [] {
                    JSFUtil.getMessage(optionSelected)});
        } else if (filters.length > 1) {
            return JSFUtil.getMessage(
                "SharedFS.title.table.clients.filtered",
                new String [] {
                    JSFUtil.getMessage(filterOptions[5][0])});
        }

        // Otherwise return the generic title
        return JSFUtil.getMessage("SharedFS.title.table.clients");
    }

    public String getClientTableFilterSelectedOption() {
        if (filters.length == 1) {
            return filters[0].toString();
        } else {
            return OptionTitle.NONESELECTED;
        }
    }

    public String getPageTitle(String fsName) {
        return JSFUtil.getMessage(
                "SharedFS.pagetitle.client",
                new String [] {fsName});
    }
}
