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

// ident	$Id: DriveLimitsTiledView.java,v 1.14 2008/12/16 00:10:55 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.archive.DriveDirective;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.util.CommonTiledViewBase;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.view.html.CCTextField;

/**
 * This class is the tiledview class for DriveLimitsTable
 */

public class DriveLimitsTiledView extends CommonTiledViewBase {

    public DriveLimitsTiledView(
        View parent, CCActionTableModel model, String name) {
        super(parent, model, name);
    }


    public boolean beginMaxDrivesForStageDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        TraceUtil.trace3("Entering");
        int index = model.getRowIndex();

        try {
            String maxDrives = getMaxDrivesEditableValue("stage", index);
            if (!maxDrives.equals("-1")) {
                ((CCTextField)getChild("MaxDrivesForStage", index)).
                    setValue(maxDrives);
            }
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginMaxDrivesForStageDisplay",
                "Failed to retrieve maximum drives for stage",
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "beginMaxDrivesForStageDisplay",
                "Failed to retrieve maximum drives for stage",
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                -2017, // samerrno not available
                ex.getMessage(),
                getServerName());
        }
        TraceUtil.trace3("Exiting");
        return true;
    }


    public boolean beginMaxDrivesForArchiveDisplay(ChildDisplayEvent event)
		throws ModelControlException {

        TraceUtil.trace3("Entering");
        int index = model.getRowIndex();
        CCTextField driver =
            (CCTextField)getChild("MaxDrivesForArchive", index);
        try {
            String maxDrives = getMaxDrivesEditableValue("archive", index);
            if (!maxDrives.equals("-1"))
                driver.setValue(maxDrives);
        } catch (SamFSException smfex) {
            SamUtil.processException(
                smfex,
                this.getClass(),
                "beginMaxDrivesForStageDisplay",
                "Failed to retrieve maximum drives for stage",
                getServerName());

            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                "ArchiveSetupViewBean.error.failedPopulate",
                smfex.getSAMerrno(),
                smfex.getMessage(),
                getServerName());

        } catch (Exception ex) {

            SamUtil.processException(ex,
                                     this.getClass(),
                                     "beginDriverCountDisplay",
                                     "Failed to set the driver count",
                                     getServerName());

            SamUtil.setErrorAlert(getParentViewBean(),
                                  CommonViewBeanBase.CHILD_COMMON_ALERT,
                                  "ArchiveSetupViewBean.error.failedPopulate",
                                  -2017,
                                  ex.getMessage(),
                                  getServerName());
        }

        TraceUtil.trace3("Exiting");
        return true;
    }

    private String getMaxDrivesEditableValue(String type, int index)
		throws SamFSException {

        TraceUtil.trace3("Entering");
        DriveDirective[] drives = null;

        if (type.equals("archive")) {
            GlobalArchiveDirective globalDir =
                ((ArchiveSetUpViewBean)getParentViewBean()).
                getGlobalDirective();

            if (globalDir == null)
                throw new SamFSException(null, -2015);

            drives = globalDir.getDriveDirectives();

        } else if (type.equals("stage")) {
            drives = ((ArchiveSetUpViewBean)getParentViewBean()).
                getStageDriveDirectives();

        } /* not a valid type */

        if (drives == null)
            throw new SamFSException(null, -2015);

        if (drives.length == 0)
            return "";

        DriveDirective drive = drives[index];
        String value = Integer.toString(drive.getCount());
        value = (value != null) ? value.trim() : "";
        TraceUtil.trace3("Exiting");
        return value;
    }


    private String getServerName() {
        return (String)
            ((CommonViewBeanBase)getParentViewBean()).getServerName();
    }
}
