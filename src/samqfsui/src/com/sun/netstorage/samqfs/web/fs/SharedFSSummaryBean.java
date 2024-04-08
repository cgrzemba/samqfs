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

// ident        $Id: SharedFSSummaryBean.java,v 1.8 2008/12/16 00:12:11 am143972 Exp $

package com.sun.netstorage.samqfs.web.fs;

import com.sun.data.provider.TableDataProvider;
import com.sun.data.provider.impl.ObjectArrayDataProvider;
import com.sun.netstorage.samqfs.web.model.MemberInfo;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Capacity;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.JSFUtil;
import com.sun.web.ui.component.Hyperlink;
import com.sun.web.ui.model.Option;
import com.sun.web.ui.model.OptionTitle;

public class SharedFSSummaryBean {

    public static final String PAGE_NAME = "SharedFSSummary";

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
    protected String imageUsage = null;
    protected String textCapacity = null;
    /** Holds value of the file system high water mark. */
    protected String textHWM = null;
    /** Holds value of the file system potential metadata server number. */
    protected String textPMDS = null;
    /** Holds value of the page title of clients section. */

    protected MemberInfo [] allInfo = null;
    protected MemberInfo [] mdsInfo = null;
    protected MemberInfo [] clientInfo = null;
    protected MemberInfo [] snInfo = null;

    public SharedFSSummaryBean() {
    }

    public Hyperlink [] getBreadCrumbs() {
        Hyperlink [] links = new Hyperlink[2];
        for (int i = 0; i < links.length; i++) {
            links[i] = new Hyperlink();
            links[i].setImmediate(true);
            links[i].setId("breadcrumbid" + i);
            if (i == 0) {
                // Back to FS Summary
                links[i].setText(JSFUtil.getMessage("FSSummary.title"));
                links[i].setUrl(
                    "/fs/FSSummary?" +
                    Constants.PageSessionAttributes.SAMFS_SERVER_NAME + "=" +
                    JSFUtil.getServerName());
            } else {
                // shared fs summary
                links[i].setText(JSFUtil.getMessage("SharedFS.pagetitle"));
            }
        }

        return links;
    }

    public Option [] getJumpMenuOptions(boolean mounted) {
        String [][] options = menuOptions;
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

    public String getTextHWM() {
        return textHWM;
    }

    public String getTextCapacity() {
        return textCapacity;
    }

    public String getImageUsage() {
        return imageUsage;
    }

    public String getTextMountPoint() {
        return textMountPoint;
    }

    public String getTextType() {
        return textType;
    }

    public String getTitleClients() {
        if (clientInfo == null) {
            clientInfo = getInfo(allInfo);
        }
        return
            JSFUtil.getMessage(
                "SharedFS.title.clients",
                new String [] {
                    clientInfo.length > 0 ?
                        Integer.toString(clientInfo[0].getTotal()) :
                    Integer.toString(0)});
    }

    public TableDataProvider getClientList() {
        if (clientInfo == null) {
            clientInfo = getInfo(allInfo);
        }

        return
            clientInfo == null ?
                new ObjectArrayDataProvider(new MemberInfo[0]):
                new ObjectArrayDataProvider(clientInfo);
    }

    public void populateSummary(MemberInfo [] infos, FileSystem thisFS) {

        allInfo = infos;

        int hwm = thisFS.getMountProperties().getHWM();
        boolean archive = thisFS.getArchivingType() == FileSystem.ARCHIVING;

        if (archive) {
            textHWM = Integer.toString(hwm);
        }

        textType = FSUtil.getFileSystemDescriptionString(thisFS, true);

        if (thisFS.getState() == FileSystem.MOUNTED) {
            textMountPoint = thisFS.getMountPoint() + " " +
                JSFUtil.getMessage("SharedFS.text.mounted");
            textCapacity =
                "(" +
                Capacity.newCapacityInJSF(
                    thisFS.getCapacity(), SamQFSSystemModel.SIZE_KB) +
                ")";
            int consumed = thisFS.getConsumedSpacePercentage();
            if (archive && consumed >= hwm) {
                // have hwm and usage exceeds hwm
                imageUsage =
                    Constants.Image.JSF_RED_USAGE_BAR_DIR + consumed + ".gif";
            } else {
                // have no hwm, or have hwm but usage does not exceed hwm
                imageUsage =
                    Constants.Image.JSF_USAGE_BAR_DIR + consumed + ".gif";
            }
        } else {
            textMountPoint = JSFUtil.getMessage("SharedFS.text.notmounted");
            imageUsage = Constants.Image.JSF_ICON_BLANK;
            textCapacity = "";
        }
    }

    private MemberInfo [] getInfo(MemberInfo [] infos) {
        int ok = 0, off = 0, unmounted = 0, error = 0, total = 0;

        for (int i = 0; i < infos.length; i++) {
            if (infos[i].isClients()) {
                total += infos[i].getClients();
            } else if (infos[i].isPmds()) {
                total += infos[i].getPmds();
            } else {
                continue;
            }
            ok += infos[i].getOk();
            off += infos[i].getOff();
            unmounted += infos[i].getUnmounted();
            error += infos[i].getError();
        }

        MemberInfo info = new MemberInfo(ok, unmounted, off, error, total);
        return new MemberInfo [] {info};
    }
}
