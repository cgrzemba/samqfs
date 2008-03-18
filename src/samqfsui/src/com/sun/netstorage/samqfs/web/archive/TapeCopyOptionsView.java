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

// ident	$Id: TapeCopyOptionsView.java,v 1.16 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import java.util.List;

public class TapeCopyOptionsView extends CopyOptionsViewBase {
    private static final String PS_XML =
        "/jsp/archive/TapeCopyOptionsPropertySheet.xml";


    public TapeCopyOptionsView(View parent, String name) {
        super(parent, PS_XML, name);
    }

    protected void initializeDropDownMenus() {
        // size drop downs
        for (int i = 0; i < sizeDropDowns.length; i++) {
            CCDropDownMenu dropDown =
                (CCDropDownMenu)getChild(sizeDropDowns[i]);
            dropDown.setOptions(new OptionList(
                SelectableGroupHelper.Sizes.labels,
                SelectableGroupHelper.Sizes.values));
        }

        // reservation method
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(RM_ATTRIBUTE);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.ReservationMethod.labels,
            SelectableGroupHelper.ReservationMethod.values));

        // sort method
        dropDown = (CCDropDownMenu)getChild(SORT_METHOD);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.SortMethod.labels,
            SelectableGroupHelper.SortMethod.values));

        // unarchive time reference
        dropDown = (CCDropDownMenu)getChild(UNARCHIVE_TIME_REF);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.UATimeReference.labels,
            SelectableGroupHelper.UATimeReference.values));

        // offline copy method
        dropDown = (CCDropDownMenu)getChild(OFFLINE_METHOD);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.OfflineCopyMethod.labels,
            SelectableGroupHelper.OfflineCopyMethod.values));

        // start age unit
        dropDown = (CCDropDownMenu)getChild(START_AGE_UNIT);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Times.labels,
            SelectableGroupHelper.Times.values));
    }

    public void loadCopyOptions() throws SamFSException {
        loadCommonCopyOptions();

        // now load the tape specific options
        ArchiveCopy theCopy = getCurrentArchiveCopy();

        // retrieve the server name
        String serverName =
            ((CommonViewBeanBase)getParentViewBean()).getServerName();

        // media type
        ArchiveVSNMap vsnMap = theCopy.getArchiveVSNMap();
        if (vsnMap == null) {
            vsnMap = PolicyUtil.getAllsetsCopyVSNMap(theCopy, serverName);
        }

        CCStaticTextField text = (CCStaticTextField)getChild(MEDIA_TYPE);
        text.setValue(SamUtil.getMediaTypeString(
            vsnMap.getArchiveMediaType()));

        // reservation method
        int reservationMethod = theCopy.getReservationMethod();
        ReservationMethodHelper rmh = new ReservationMethodHelper();
        rmh.setValue(reservationMethod);

        // reserve by policy?
        CCCheckBox checkBox = (CCCheckBox)getChild(RM_POLICY);
        String rmPolicy = rmh.getSet() == rmh.RM_SET ? "true" : "false";
        checkBox.setValue(rmPolicy);

        // reserve by filesystem?
        checkBox = (CCCheckBox)getChild(RM_FILESYSTEM);
        String rmFS = rmh.getFS() == rmh.RM_FS ? "true" : "false";
        checkBox.setValue(rmFS);

        // Fill VSNs?
        checkBox = (CCCheckBox)getChild(FILL_VSNS);
        String fillvsns = theCopy.isFillVSNs() ? "true" : "false";
        checkBox.setValue(fillvsns);

        // reserve by attributes?
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(RM_ATTRIBUTE);
        dropDown.setValue(Integer.toString(rmh.getAttributes()));

        // maximum drives to use
        CCTextField textField = (CCTextField)getChild(DRIVES);
        int maxDrives = theCopy.getDrives();
        if (maxDrives != -1) {
            textField.setValue(Integer.toString(maxDrives));
        }

        // max per drive
        long maxPerDrive = theCopy.getMaxDrives();
        dropDown = (CCDropDownMenu)getChild(MAX_PER_DRIVE_UNIT);
        if (maxPerDrive != -1) {
            textField = (CCTextField)getChild(MAX_PER_DRIVE);
            textField.setValue(Long.toString(maxPerDrive));

            dropDown.setValue(Integer.toString(theCopy.getMaxDrivesUnit()));
        } else {
            dropDown.setValue(Integer.toString(SamQFSSystemModel.SIZE_MB));
        }

        // multi-drive trigger
        long multiDriveTrigger = theCopy.getMinDrives();
        dropDown = (CCDropDownMenu)getChild(DRIVE_TRIGGER_UNIT);
        if (multiDriveTrigger != -1) {
            textField = (CCTextField)getChild(DRIVE_TRIGGER);
            textField.setValue(Long.toString(multiDriveTrigger));

            dropDown.setValue(Integer.toString(theCopy.getMinDrivesUnit()));
        } else {
            dropDown.setValue(Integer.toString(SamQFSSystemModel.SIZE_MB));
        }

        // max size for overflow
        long maxOverflow = theCopy.getOverflowMinSize();
        dropDown = (CCDropDownMenu)getChild(MIN_OVERFLOW_UNIT);
        if (maxOverflow != -1) {
            textField = (CCTextField)getChild(MIN_OVERFLOW);
            textField.setValue(Long.toString(maxOverflow));

            dropDown.setValue(
                Integer.toString(theCopy.getOverflowMinSizeUnit()));
        } else {
            dropDown.setValue(Integer.toString(SamQFSSystemModel.SIZE_MB));
        }

        // recycler data quantity
        long recyclerDataSize = theCopy.getRecycleDataSize();
        dropDown = (CCDropDownMenu)getChild(RECYCLE_SIZE_UNIT);
        if (recyclerDataSize != -1) {
            textField = (CCTextField)getChild(RECYCLE_SIZE);
            textField.setValue(Long.toString(recyclerDataSize));

            dropDown.setValue(
                Integer.toString(theCopy.getRecycleDataSizeUnit()));
        } else {
            dropDown.setValue(Integer.toString(SamQFSSystemModel.SIZE_MB));
        }

        // vsn count
        int vsnCount = theCopy.getMaxVSNCount();
        if (vsnCount != -1) {
            textField = (CCTextField)getChild(MAX_VSN_COUNT);
            textField.setValue(Integer.toString(vsnCount));
        }
    }

    public List validateCopyOptions() throws SamFSException {
        ArchiveCopy theCopy = getCurrentArchiveCopy();

        // validate the common fields first
        List errors = super.validateCommonCopyOptions();

        // now validate the tape specific options

        // reservation method
        ReservationMethodHelper rmh = new ReservationMethodHelper();
        int attributes =
                Integer.parseInt(getDisplayFieldStringValue(RM_ATTRIBUTE));

        if (attributes > 0) {
            rmh.setAttributes(attributes);
        }

        if (getDisplayFieldStringValue(RM_POLICY).equals("true")) {
            rmh.setSet(ReservationMethodHelper.RM_SET);
        }

        if (getDisplayFieldStringValue(RM_FILESYSTEM).equals("true")) {
            rmh.setFS(ReservationMethodHelper.RM_FS);
        }
        theCopy.setReservationMethod(rmh.getValue());

        theCopy.setFillVSNs(
            "true".equals(getDisplayFieldStringValue(FILL_VSNS)));

        // max drives to use
        String maxDriveString = getDisplayFieldStringValue(DRIVES);
        maxDriveString = maxDriveString == null ? "" : maxDriveString.trim();
        if (!maxDriveString.equals("")) {
            CCLabel label = (CCLabel)getChild(DRIVES.concat("Label"));

            try {
                int maxDrives = Integer.parseInt(maxDriveString);
                if (maxDrives > 0) {
                    theCopy.setDrives(maxDrives);
                } else {
                    errors.add("archiving.error.drives");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("archiving.error.drives");
                label.setShowError(true);
            }
        } else {
            theCopy.setDrives(-1);
        }

        // max per drive
        String maxPerDriveString = getDisplayFieldStringValue(MAX_PER_DRIVE);
        maxPerDriveString =
            maxPerDriveString == null ? "" : maxPerDriveString.trim();
        if (!maxPerDriveString.equals("")) {
            CCLabel label = (CCLabel)getChild(MAX_PER_DRIVE.concat("Label"));

            try {
                long maxPerDrive = Long.parseLong(maxPerDriveString);
                int maxPerDriveUnit = Integer.parseInt(
                    getDisplayFieldStringValue(MAX_PER_DRIVE_UNIT));

                if (PolicyUtil.isValidSize(maxPerDrive, maxPerDriveUnit)) {
                    theCopy.setMaxDrives(maxPerDrive);
                    theCopy.setMaxDrivesUnit(maxPerDriveUnit);
                } else {
                    errors.add("archiving.error.drivesmax");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("archiving.error.drivesmax");
                label.setShowError(true);
            }
        } else {
            theCopy.setMaxDrives(-1);
            theCopy.setMaxDrivesUnit(-1);
        }

        // multi-drive trigger
        String driveTriggerString = getDisplayFieldStringValue(DRIVE_TRIGGER);
        driveTriggerString =
            driveTriggerString == null ? "" : driveTriggerString.trim();
        if (!driveTriggerString.equals("")) {
            CCLabel label = (CCLabel)getChild(DRIVE_TRIGGER.concat("Label"));

            try {
                long driveTrigger = Long.parseLong(driveTriggerString);
                int driveTriggerUnit = Integer.parseInt(
                    getDisplayFieldStringValue(DRIVE_TRIGGER_UNIT));

                if (PolicyUtil.isValidSize(driveTrigger, driveTriggerUnit)) {
                    theCopy.setMinDrives(driveTrigger);
                    theCopy.setMinDrivesUnit(driveTriggerUnit);
                } else {
                    errors.add("archiving.error.drivesmin");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("archiving.error.drivesmin");
                label.setShowError(true);
            }
        } else {
            theCopy.setMinDrives(-1);
            theCopy.setMinDrivesUnit(-1);
        }

        // min size for overfow
        String minOverflowString = getDisplayFieldStringValue(MIN_OVERFLOW);
        minOverflowString =
            minOverflowString == null ? "" : minOverflowString.trim();
        if (!minOverflowString.equals("")) {
            CCLabel label = (CCLabel)getChild(MIN_OVERFLOW.concat("Label"));

            try {
                long minOverflow = Long.parseLong(minOverflowString);
                int minOverflowUnit = Integer.parseInt(
                    getDisplayFieldStringValue(MIN_OVERFLOW_UNIT));

                if (PolicyUtil.isValidSize(minOverflow, minOverflowUnit)) {
                    theCopy.setOverflowMinSize(minOverflow);
                    theCopy.setOverflowMinSizeUnit(minOverflowUnit);
                } else {
                    errors.add("archiving.error.overflowmin");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("archiving.error.overflowmin");
                label.setShowError(true);
            }
        } else {
            theCopy.setOverflowMinSize(-1);
            theCopy.setOverflowMinSizeUnit(-1);
        }

        // recycler data quantity
        String dataSizeString = getDisplayFieldStringValue(RECYCLE_SIZE);
        dataSizeString = dataSizeString == null ? "" : dataSizeString.trim();
        if (!dataSizeString.equals("")) {
            CCLabel label = (CCLabel)getChild(RECYCLE_SIZE.concat("Label"));

            try {
                long dataSize = Long.parseLong(dataSizeString);
                int dataSizeUnit = Integer.parseInt(
                    getDisplayFieldStringValue(RECYCLE_SIZE_UNIT));

                if (PolicyUtil.isValidSize(dataSize, dataSizeUnit)) {
                    theCopy.setRecycleDataSize(dataSize);
                    theCopy.setRecycleDataSizeUnit(dataSizeUnit);
                } else {
                    errors.add("archiving.error.recyclequantity");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfew) {
                errors.add("archiving.error.recyclequantity");
                label.setShowError(true);
            }
        } else {
            theCopy.setRecycleDataSize(-1);
            theCopy.setRecycleDataSizeUnit(-1);
        }

        // max vsn count
        String maxVSNString = getDisplayFieldStringValue(MAX_VSN_COUNT);
        maxVSNString = maxVSNString == null ? "" : maxVSNString.trim();
        if (!maxVSNString.equals("")) {
            CCLabel label = (CCLabel)getChild(MAX_VSN_COUNT.concat("Label"));

            try {
                int vsnCount = Integer.parseInt(maxVSNString);

                if (vsnCount <= 0) {
                    errors.add("ArchivePolCopy.error.maxVSNCount");
                    label.setShowError(true);
                } else {
                    theCopy.setMaxVSNCount(vsnCount);
                }
            } catch (NumberFormatException nfe) {
                errors.add("ArchivePolCopy.error.maxVSNCount");
                label.setShowError(true);
            }
        } else {
            theCopy.setMaxVSNCount(-1);
        }

        // if now errors were encountered, persist the copy changes
        if (errors.size() == 0) {
            ArchivePolicy thePolicy = theCopy.getArchivePolicy();

            thePolicy.updatePolicy();
        }

        // return the cumulative list of errors
        return errors;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        initializeDropDownMenus();
    }

    /**
     * imlements the CCpagelet interface
     *
     * @see com.sun.web.ui.common.CCPagelet#getPagelet
     */
    public String getPageletUrl() {
        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        Integer copyMediaType = (Integer)
            parent.getPageSessionAttribute(Constants.Archive.COPY_MEDIA_TYPE);

        if (copyMediaType.intValue() != BaseDevice.MTYPE_DISK &&
            copyMediaType.intValue() != BaseDevice.MTYPE_STK_5800) {
            return "/jsp/archive/TapeCopyOptionsPagelet.jsp";
        } else {
            return "/jsp/archive/BlankPagelet.jsp";
        }
    }

    /**
     * Hide fill vsns if it is not set (CR 6452025)
     */
    public boolean beginFillVSNsDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        try {
            return getCurrentArchiveCopy().isFillVSNs();
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Exception caught while checking fillvsns!");
        }
        return false;
    }

    /**
     * Hide fill vsns label if it is not set (CR 6452025)
     */
    public boolean beginFillVSNsLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        try {
            return getCurrentArchiveCopy().isFillVSNs();
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Exception caught while checking fillvsns!");
        }
        return false;
    }


    // size drop downs
    private static String [] sizeDropDowns = {
        MIN_OVERFLOW_UNIT,
        MAX_PER_DRIVE_UNIT,
        MAX_SIZE_ARCHIVE_UNIT,
        DRIVE_TRIGGER_UNIT,
        START_SIZE_UNIT,
        RECYCLE_SIZE_UNIT};
    }
