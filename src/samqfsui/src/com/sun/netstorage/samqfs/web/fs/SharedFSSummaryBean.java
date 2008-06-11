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

// ident        $Id: SharedFSSummaryBean.java,v 1.1 2008/06/11 16:58:00 ronaldso Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.data.provider.TableDataProvider;
import com.sun.data.provider.impl.ObjectArrayDataProvider;
import com.sun.netstorage.samqfs.web.model.MemberInfo;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.web.ui.component.Hyperlink;
import com.sun.web.ui.model.Option;
import com.sun.web.ui.model.OptionTitle;

public class SharedFSSummaryBean {

    /** Section IDs definitions. */
    private static final short SECTION_SUMMARY = 0;
    private static final short SECTION_CLIENTS = 1;
    private static final short SECTION_STORAGE_NODES = 2;

    /** Page Menu Options (with storage nodes) */
    protected String [][] menuOptionsWithSN = new String [][] {
        {"SharedFS.operation.editmo", "editmo"},
        {"SharedFS.operation.mount", "mount"},
        {"SharedFS.operation.unmount", "unmount"},
        {"SharedFS.operation.removesn", "removesn"},
        {"SharedFS.operation.removecn", "removecn"}
    };
    /** Page Menu Options (without storage nodes) */
    protected String [][] menuOptions = new String [][] {
        {"SharedFS.operation.editmo", "editmo"},
        {"SharedFS.operation.mount", "mount"},
        {"SharedFS.operation.unmount", "unmount"},
        {"SharedFS.operation.removecn", "removecn"}
    };
    /** Holds value of the file system type text. */
    protected String textType = null;
    /** Holds value of the file system mount point. */
    protected String textMountPoint = null;
    /** Holds value of the file system capacity. */
    protected String textCapacity = null;
    /** Holds value of the file system high water mark. */
    protected String textHWM = null;
    /** Holds value of the file system archiving status. */
    protected String textArchiving = null;
    /** Holds value of the file system potential metadata server number. */
    protected String textPMDS = null;
    /** Holds value of the page title of clients section. */

    protected MemberInfo [] allInfo = null;
    protected MemberInfo [] summaryInfo = null;
    protected MemberInfo [] clientInfo = null;
    protected MemberInfo [] snInfo = null;

    public SharedFSSummaryBean() {
    }

    public Hyperlink [] getBreadCrumbs(){
        Hyperlink [] links = new Hyperlink[2];
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
            } else {
                // shared fs summary
                links[i].setText(JSFUtil.getMessage("SharedFS.title"));
            }
        }

        return links;
    }

    public Option [] getJumpMenuOptions(
        boolean showStorageNodes, boolean mounted) {
        String [][] options =
            showStorageNodes ?
                menuOptionsWithSN :
                menuOptions;
        Option[] jumpMenuOptions = new Option[options.length + 1];
        jumpMenuOptions[0] =
                new OptionTitle(JSFUtil.getMessage("common.dropdown.header"));
        for (int i = 1; i < options.length + 1; i++) {
            jumpMenuOptions[i] =
                    new Option(
                        options[i - 1][1],
                        JSFUtil.getMessage(options[i - 1][0]));
        }

        if (mounted) {
            jumpMenuOptions[2].setDisabled(true);
        } else {
            jumpMenuOptions[3].setDisabled(true);
        }
        return jumpMenuOptions;
    }

    public String getTextArchiving() {
        return textArchiving;
    }

    public String getTextPMDS() {
        return textPMDS;
    }

    public String getTextHWM() {
        return textHWM;
    }

    public String getTextCapacity() {
        return textCapacity;
    }

    public String getTextMountPoint() {
        return textMountPoint;
    }

    public String getTextType() {
        return textType;
    }

    public String getTitleClients() {
        if (clientInfo == null) {
            clientInfo = getInfo(allInfo, SECTION_CLIENTS);
        }
        return
            clientInfo.length > 0 ?
                JSFUtil.getMessage(
                    "SharedFS.title.clients",
                    new String [] {
                            Integer.toString(clientInfo[0].getClients())}) :
                "";
    }

    public TableDataProvider getClientList() {
        if (clientInfo == null) {
            clientInfo = getInfo(allInfo, SECTION_CLIENTS);
        }
        return
            clientInfo == null ?
                new ObjectArrayDataProvider(new MemberInfo[0]):
                new ObjectArrayDataProvider(clientInfo);
    }

    public String getTitleStorageNodes() {
        if (snInfo == null) {
            snInfo = getInfo(allInfo, SECTION_STORAGE_NODES);
        }
        return
            snInfo.length > 0 ?
                JSFUtil.getMessage(
                    "SharedFS.title.sns",
                    new String [] {
                            Integer.toString(snInfo[0].getStorageNodes())}) :
                "";
    }

    public TableDataProvider getStorageNodeList() {
        if (snInfo == null) {
            snInfo = getInfo(allInfo, SECTION_STORAGE_NODES);
        }
        return
            snInfo.length > 0 ?
                new ObjectArrayDataProvider(snInfo) :
            null;
    }

    public void populateSummary(
        MemberInfo [] infos, FileSystem thisFS, boolean showStorageNodes) {

        textType = FSUtil.getFileSystemDescriptionString(thisFS, true);
        textMountPoint =
            thisFS.getState() == FileSystem.MOUNTED ?
                thisFS.getMountPoint() + " " +
                JSFUtil.getMessage("SharedFS.text.mounted") :
            JSFUtil.getMessage("SharedFS.text.notmounted");
        if (thisFS.getArchivingType() == FileSystem.ARCHIVING) {
            textHWM = Integer.toString(thisFS.getMountProperties().getHWM());
            textArchiving = "TODO:";
        }

        allInfo = infos;
        summaryInfo = getInfo(infos, SECTION_SUMMARY);

        textPMDS = summaryInfo.length > 0 ?
            Integer.toString(summaryInfo[0].getPmds()) : "";
    }

    private MemberInfo [] getInfo(MemberInfo [] infos, short sectionId) {
        for (int i = 0; i < infos.length; i++) {
            if (
                (infos[i].isSummary() && sectionId == SECTION_SUMMARY) ||
                (infos[i].isClients() && sectionId == SECTION_CLIENTS) ||
                (infos[i].isStorageNodes() &&
                       sectionId == SECTION_STORAGE_NODES)) {
                return new MemberInfo [] {infos[i]};
            }
        }
        return new MemberInfo[0];
    }
}
