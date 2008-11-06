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

// ident        $Id: SharedFSFilterBean.java,v 1.3 2008/11/06 00:47:07 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.netstorage.samqfs.web.model.fs.SharedFSFilter;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.Option;
import javax.faces.event.ActionEvent;

public class SharedFSFilterBean {

    private final static String delimitor = "###";

    protected Option [] menuFilterItems = null;
    protected Option [] menuFilterConditions = null;
    protected Option [] menuStatus = null;
    protected String condition1 = null;
    protected String condition2 = null;
    protected String menuContent1 = null;
    protected String menuContent2 = null;
    protected String menuContentValue1 = null;
    protected String menuContentValue2 = null;
    protected String errMsgAtLeast = null;
    protected String errMsgAtMost = null;
    protected String selectedMenuFilterItem1 = null;
    protected String selectedMenuFilterItem2 = null;
    protected String selectedMenuFilterItem3 = null;
    protected String selectedMenuFilterCondition1 = null;
    protected String selectedMenuFilterCondition2 = null;
    protected String selectedMenuFilterCondition3 = null;
    protected String menu1 = null;
    protected String menu2 = null;
    protected String menu3 = null;
    protected String text1 = null;
    protected String text2 = null;
    protected String text3 = null;
    protected String submitValue = null;

    protected String [][] filterCriteria = new String [][] {
        {"SharedFS.client.table.heading.hostname",
         SharedFSFilter.TARGET_HOST_NAME},
        {"SharedFS.client.table.heading.type",
         SharedFSFilter.TARGET_TYPE},
        {"SharedFS.client.table.heading.ipaddress",
         SharedFSFilter.TARGET_IP_ADDRESS},
        {"SharedFS.client.table.heading.status",
         SharedFSFilter.TARGET_STATUS},
    };

    protected String [][] filterCondition = new String [][] {
        {"common.filter.condition.contains",
         SharedFSFilter.COND_CONTAINS},
        {"common.filter.condition.doesnotcontain",
         SharedFSFilter.COND_DOES_NOT_CONTAIN},
        {"common.filter.condition.startswith",
         SharedFSFilter.COND_STARTS_WITH},
        {"common.filter.condition.endswith",
         SharedFSFilter.COND_ENDS_WITH}
    };

    protected String [][] allStatus = new String [][] {
        {"SharedFS.state.ok", SharedFSFilter.STATUS_OK},
        {"SharedFS.state.mounted", SharedFSFilter.STATUS_MOUNTED},
        {"SharedFS.state.accessenabled", SharedFSFilter.STATUS_ACCESS_ENABLED}
    };


    public SharedFSFilterBean() {
        errMsgAtLeast = JSFUtil.getMessage("common.filter.message.atleast");
        errMsgAtMost = JSFUtil.getMessage("common.filter.message.atmost");

        menuFilterItems = new Option[filterCriteria.length];
        for (int i = 0; i < filterCriteria.length; i++) {
            menuFilterItems[i] =
                new Option(
                    filterCriteria[i][1],
                    JSFUtil.getMessage(filterCriteria[i][0]));
        }

        menuFilterConditions = new Option[filterCondition.length];
        for (int i = 0; i < filterCondition.length; i++) {
            menuFilterConditions[i] =
                new Option(
                    filterCondition[i][1],
                    JSFUtil.getMessage(filterCondition[i][0]));
        }

        menuStatus = new Option[allStatus.length];
        for (int i = 0; i < allStatus.length; i++) {
            menuStatus[i] =
                new Option(
                    allStatus[i][1],
                    JSFUtil.getMessage(allStatus[i][0]));
        }

        condition1 =
            JSFUtil.getMessage("common.filter.condition.contains") +
                delimitor +
            JSFUtil.getMessage("common.filter.condition.doesnotcontain") +
                delimitor +
            JSFUtil.getMessage("common.filter.condition.startswith") +
                delimitor +
            JSFUtil.getMessage("common.filter.condition.endswith");

        condition2 =
            JSFUtil.getMessage("common.filter.condition.is") +
                delimitor +
            JSFUtil.getMessage("common.filter.condition.isnot");

        menuContent1 =
            JSFUtil.getMessage("SharedFS.state.ok") +
                delimitor +
            JSFUtil.getMessage("SharedFS.state.mounted") +
                delimitor +
            JSFUtil.getMessage("SharedFS.state.accessenabled") +
                delimitor +
            JSFUtil.getMessage("SharedFS.state.inerror");
        menuContent2 =
            JSFUtil.getMessage("SharedFS.text.mdspmds") +
                delimitor +
            JSFUtil.getMessage("SharedFS.text.client");

        menuContentValue1 =
            SharedFSFilter.STATUS_OK +
                delimitor +
            SharedFSFilter.STATUS_MOUNTED +
                delimitor +
            SharedFSFilter.STATUS_ACCESS_ENABLED +
                delimitor +
            SharedFSFilter.STATUS_IN_ERROR;

        menuContentValue2 =
            SharedFSFilter.TYPE_MDS_PMDS +
                delimitor +
            SharedFSFilter.TYPE_CLIENTS;
    }

    // Action Event Listener

    public void okButtonClicked(ActionEvent event) {
        TraceUtil.trace2("submitted: " + getSubmitValue());

        String param = "";
        String [] inUsed = getSubmitValue().split(",");
        if (inUsed == null || inUsed.length == 0) {
            System.out.println("Invalid onSubmit value from javascript!");
            System.out.println("No filtering is taking place!");
        } else {
            for (int i = 0; i < inUsed.length; i++) {
                if ("false".equals(inUsed[i])) {
                    break;
                }
                if (param.length() > 0) {
                    param += SharedFSFilter.FILTER_DELIMITOR;
                }

                // MAXIMUM three criteria
                if (0 == i) {
                    boolean useMenu =
                        selectedMenuFilterItem1.equals(
                            SharedFSFilter.TARGET_TYPE) ||
                        selectedMenuFilterItem1.equals(
                            SharedFSFilter.TARGET_STATUS);
                    param +=
                        selectedMenuFilterItem1 + SharedFSFilter.LINKER +
                        selectedMenuFilterCondition1 + SharedFSFilter.LINKER +
                        (useMenu? menu1 : text1);
                } else if (1 == i) {
                    boolean useMenu =
                        selectedMenuFilterItem2.equals(
                            SharedFSFilter.TARGET_TYPE) ||
                        selectedMenuFilterItem2.equals(
                            SharedFSFilter.TARGET_STATUS);
                    param +=
                        selectedMenuFilterItem2 + SharedFSFilter.LINKER +
                        selectedMenuFilterCondition2 + SharedFSFilter.LINKER +
                        (useMenu? menu2 : text2);
                } else if (2 == i) {
                    boolean useMenu =
                        selectedMenuFilterItem3.equals(
                            SharedFSFilter.TARGET_TYPE) ||
                        selectedMenuFilterItem3.equals(
                            SharedFSFilter.TARGET_STATUS);
                    param +=
                        selectedMenuFilterItem3 + SharedFSFilter.LINKER +
                        selectedMenuFilterCondition3 + SharedFSFilter.LINKER +
                        (useMenu? menu3 : text3);
                }
            }
        }

        JSFUtil.redirectPage(
            "/faces/jsp/fs/SharedFSClient.jsp",
            SharedFSBean.PARAM_FILTER + "=" + param);
    }

    public Option [] getMenuFilterItems() {
        return menuFilterItems;
    }

    public void setMenuFilterItems(Option[] menuFilterItems) {
        this.menuFilterItems = menuFilterItems;
    }

    public Option[] getMenuFilterConditions() {
        return menuFilterConditions;
    }

    public void setMenuFilterConditions(Option[] menuFilterConditions) {
        this.menuFilterConditions = menuFilterConditions;
    }

    public String[][] getFilterCriteria() {
        return filterCriteria;
    }

    public void setFilterCriteria(String[][] filterCriteria) {
        this.filterCriteria = filterCriteria;
    }

    public Option[] getMenuStatus() {
        return menuStatus;
    }

    public void setMenuStatus(Option[] menuStatus) {
        this.menuStatus = menuStatus;
    }

    public String getMenu1() {
        return menu1;
    }

    public void setMenu1(String menu1) {
        this.menu1 = menu1;
    }

    public String getMenu2() {
        return menu2;
    }

    public void setMenu2(String menu2) {
        this.menu2 = menu2;
    }

    public String getMenu3() {
        return menu3;
    }

    public void setMenu3(String menu3) {
        this.menu3 = menu3;
    }

    public String getSelectedMenuFilterCondition1() {
        return selectedMenuFilterCondition1;
    }

    public void setSelectedMenuFilterCondition1(
        String selectedMenuFilterCondition1) {
        this.selectedMenuFilterCondition1 = selectedMenuFilterCondition1;
    }

    public String getSelectedMenuFilterCondition2() {
        return selectedMenuFilterCondition2;
    }

    public void setSelectedMenuFilterCondition2(
        String selectedMenuFilterCondition2) {
        this.selectedMenuFilterCondition2 = selectedMenuFilterCondition2;
    }

    public String getSelectedMenuFilterCondition3() {
        return selectedMenuFilterCondition3;
    }

    public void setSelectedMenuFilterCondition3(
        String selectedMenuFilterCondition3) {
        this.selectedMenuFilterCondition3 = selectedMenuFilterCondition3;
    }

    public String getSelectedMenuFilterItem1() {
        return selectedMenuFilterItem1;
    }

    public void setSelectedMenuFilterItem1(String selectedMenuFilterItem1) {
        this.selectedMenuFilterItem1 = selectedMenuFilterItem1;
    }

    public String getSelectedMenuFilterItem2() {
        return selectedMenuFilterItem2;
    }

    public void setSelectedMenuFilterItem2(String selectedMenuFilterItem2) {
        this.selectedMenuFilterItem2 = selectedMenuFilterItem2;
    }

    public String getSelectedMenuFilterItem3() {
        return selectedMenuFilterItem3;
    }

    public void setSelectedMenuFilterItem3(String selectedMenuFilterItem3) {
        this.selectedMenuFilterItem3 = selectedMenuFilterItem3;
    }

    public String getText1() {
        return text1;
    }

    public void setText1(String text1) {
        this.text1 = text1;
    }

    public String getText2() {
        return text2;
    }

    public void setText2(String text2) {
        this.text2 = text2;
    }

    public String getText3() {
        return text3;
    }

    public void setText3(String text3) {
        this.text3 = text3;
    }


    public String getCondition1() {
        return condition1;
    }

    public void setCondition1(String condition1) {
        this.condition1 = condition1;
    }

    public String getCondition2() {
        return condition2;
    }

    public void setCondition2(String condition2) {
        this.condition2 = condition2;
    }

    public String getErrMsgAtLeast() {
        return errMsgAtLeast;
    }

    public void setErrMsgAtLeast(String errMsgAtLeast) {
        this.errMsgAtLeast = errMsgAtLeast;
    }

    public String getErrMsgAtMost() {
        return errMsgAtMost;
    }

    public void setErrMsgAtMost(String errMsgAtMost) {
        this.errMsgAtMost = errMsgAtMost;
    }

    public String getMenuContent1() {
        return menuContent1;
    }

    public void setMenuContent1(String menuContent1) {
        this.menuContent1 = menuContent1;
    }

    public String getMenuContentValue1() {
        return menuContentValue1;
    }

    public void setMenuContentValue1(String menuContentValue1) {
        this.menuContentValue1 = menuContentValue1;
    }

    public String getMenuContentValue2() {
        return menuContentValue2;
    }

    public void setMenuContentValue2(String menuContentValue2) {
        this.menuContentValue2 = menuContentValue2;
    }

    public String getMenuContent2() {
        return menuContent2;
    }

    public void setMenuContent2(String menuContent2) {
        this.menuContent2 = menuContent2;
    }

    public String getSubmitValue() {
        return submitValue;
    }

    public void setSubmitValue(String submitValue) {
        this.submitValue = submitValue;
    }

}
