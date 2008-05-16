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

// ident	$Id: CopyMediaValidator.java,v 1.18 2008/05/16 18:38:52 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.util.NonSyncStringBuffer;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.ReservationMethodHelper;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.model.wizard.WizardEvent;

/**
 * this is a common validation utility for the Copy Media page of both the
 * New Archive Policy Wizard (page 3) and the New Copy Wizard (page 1)
 */
public class CopyMediaValidator {
    // public keys for archive type
    public static final String COPY_TYPE = "copy_archive_type";
    public static final String AT_DISK = "disk";
    public static final String AT_TAPE = "tape";

    // prefix to key used to hold copy new disk vns params
    public static final String NDV_KEY = "new_disk_vsn_key.";

    private SamWizardModel model;
    private WizardEvent event;
    private ArchiveCopyGUIWrapper wrapper;
    private int copyNumber;

    // values from this page will need to be displayed in the summary page,
    // since this class is used to validate two different wizards, we need
    // to store the variables here temporarily before moving them to the
    // right summary page name-value binding.
    private String summaryArchiveAge;
    private String summaryArchiveType;
    private String summaryDiskVSNName;
    private String summaryDiskVSNPath;
    private String summaryDiskVSNHost;
    private String summaryMediaType;
    private String summaryVSNPoolName;
    private String summarySpecifiedVSNs;
    private String summaryRMString;

    // important for determining page flow
    private int selectedMediaType;

    public CopyMediaValidator(SamWizardModel model,
                              WizardEvent event,
                              ArchiveCopyGUIWrapper wrapper) {
        this(model, event, wrapper, -1);
    }

    public CopyMediaValidator(SamWizardModel model,
                              WizardEvent event,
                              ArchiveCopyGUIWrapper wrapper,
                              int copyNumber) {
        this.model = model;
        this.event = event;
        this.wrapper = wrapper;
        this.copyNumber = copyNumber;
    }

    public boolean validate() {
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        String serverName = (String)
            model.getValue(Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

        // starting by saving the radio button selection
        String radioValue = CopyMediaParametersView.EXISTING_VSN;
        model.setValue(CopyMediaParametersView.SELECTED_VSN_RADIO, radioValue);

        String archiveAge = (String)
            model.getValue(CopyMediaParametersView.ARCHIVE_AGE);

        // verify that age is provided
        if (archiveAge == null || (archiveAge.trim()).length() == 0) {
            setJavascriptErrorMessage(
                event,
                CopyMediaParametersView.ARCHIVE_AGE_LABEL,
                "NewArchivePolWizard.page3.errArchiveAgeNegInt",
                true);
            return false;
        }

        // verify age is numeric
        long age = -1;

        try {
            age = Long.parseLong(archiveAge);
            if (age < 0) {
                setJavascriptErrorMessage(
                    event,
                    CopyMediaParametersView.ARCHIVE_AGE_LABEL,
                    "NewArchivePolWizard.page3.errArchiveAgeNegInt",
                    true);
                return false;
            } else {
                // --> save to safe age
                wrapper.getArchivePolCriteriaCopy().setArchiveAge(age);
            }
        } catch (NumberFormatException nfe) {
            setJavascriptErrorMessage(
                event,
                CopyMediaParametersView.ARCHIVE_AGE_LABEL,
                "NewArchivePolWizard.page3.errArchiveAge",
                true);
            return false;
        }

        String ageUnitStr =
            (String)model.getValue(CopyMediaParametersView.ARCHIVE_AGE_UNIT);
        if (SelectableGroupHelper.NOVAL.equals(ageUnitStr)) {
            setJavascriptErrorMessage(
                event,
                CopyMediaParametersView.ARCHIVE_AGE_LABEL,
                "NewArchivePolWizard.page3.errArchiveAgeDropDown",
                true);
            return false;
        }

        // safe to save archive age units
        int ageUnit = Integer.parseInt(ageUnitStr);
        wrapper.getArchivePolCriteriaCopy().setArchiveAgeUnit(ageUnit);

        // persist archive age for summary page
        NonSyncStringBuffer temp = new NonSyncStringBuffer();
        temp.append(age)
            .append(" ")
            .append(PolicyUtil.getTimeUnitAsString(ageUnit));
        summaryArchiveAge = temp.toString();

        // retrieve the vsn map for this copy
        ArchiveVSNMap vsnMap = wrapper.getArchiveCopy().getArchiveVSNMap();

        // temporary value to hold media type ...
        int mType = BaseDevice.MTYPE_DISK;
        // if (useVSNs.equals(CopyMediaParametersView.EXISTING_VSN)) {
        // use existing vsn's

        // media type
        String mediaType =
            (String)model.getValue(CopyMediaParametersView.MEDIA_TYPE);

        if (mediaType.equals(SelectableGroupHelper.NOVAL)) {
            setJavascriptErrorMessage(
                event,
                CopyMediaParametersView.MEDIA_TYPE_LABEL,
                "NewArchivePolWizard.page3.errMediaType",
                true);
            return false;
        }

        boolean hasPoolName = false;
        boolean hasRange = false;
        boolean hasList = false;

        mType = Integer.parseInt(mediaType);

        // set media type
        vsnMap.setArchiveMediaType(mType);

        // pool name
        String poolName =
            (String)model.getValue(CopyMediaParametersView.POOL_NAME);
        if (!poolName.equals(SelectableGroupHelper.NOVAL)) {
            // save vsn pool name
            vsnMap.setPoolExpression(poolName);

            summaryVSNPoolName = poolName;
            hasPoolName = true;
        }

        // range
        String startVSN =
            (String)model.getValue(CopyMediaParametersView.FROM);
        startVSN = startVSN == null ? "" : startVSN.trim();

        String endVSN = (String)model.getValue(CopyMediaParametersView.TO);
        endVSN = endVSN == null ? "" : endVSN.trim();

        if (!startVSN.equals("") && !endVSN.equals("")) {
            if (mType != BaseDevice.MTYPE_DISK) {
                if (!SamUtil.isValidVSNString(startVSN)) {
                    setJavascriptErrorMessage(
                        event,
                        CopyMediaParametersView.RANGE_LABEL,
                        "NewArchivePolWizard.page3.errStartInvalidVSN",
                        true);
                    return false;
                }

                if (!SamUtil.isValidVSNString(endVSN)) {
                    setJavascriptErrorMessage(
                        event,
                        CopyMediaParametersView.TO_LABEL,
                        "NewArchivePolWizard.page3.errEndInvalidVSN",
                        true);
                     return false;
                }
            }

            // if we get this far, the range is fine
            vsnMap.setMapExpressionStartVSN(startVSN);
            vsnMap.setMapExpressionEndVSN(endVSN);

            hasRange = true;
        } else {
            // blank out the map to catch forward->backward->forward
            // navigtaion
            vsnMap.setMapExpressionStartVSN("");
            vsnMap.setMapExpressionEndVSN("");
        }

        // check for valid start - end combinations
        if (!startVSN.equals("") && endVSN.equals("")) {
            setJavascriptErrorMessage(
                event,
                CopyMediaParametersView.TO_LABEL,
                "NewArchivePolWizard.page3.errEndVSN",
                true);
            return false;
        } else if (startVSN.equals("") && !endVSN.equals("")) {
            setJavascriptErrorMessage(
                event,
                CopyMediaParametersView.RANGE_LABEL,
                "NewArchivePolWizard.page3.errStartVSN",
                true);
            return false;
        }

        // list
        String list = (String)model.getValue(CopyMediaParametersView.LIST);
        list = list == null ? "" : list.trim();

        if (!list.equals("")) {
            vsnMap.setMapExpression(list);
            hasList = true;
        } else {
            vsnMap.setMapExpression("");
        }

        // save the expression for the summary page
        summarySpecifiedVSNs =
            getSpecifiedVSNString(startVSN, endVSN, list);

        // verify that we have one of pool name, range, or list
        if (!hasPoolName && !hasRange && !hasList) {
            setJavascriptErrorMessage(
                event,
                CopyMediaParametersView.RANGE_LABEL,
                "NewArchivePolWizard.page3.errMediaTypeParam",
                true);
            return false;
        }

        // don't bother with reservatino method if using disk
        if (mType != BaseDevice.MTYPE_DISK) {
            ReservationMethodHelper rmh = new ReservationMethodHelper();

            // attributes
            String rmAttributes = (String)
                model.getValue(CopyMediaParametersView.RM_ATTRIBUTES);
            if (!rmAttributes.equals(SelectableGroupHelper.NOVAL)) {
                rmh.setAttributes(Integer.parseInt(rmAttributes));
            }

            // policy
            String rmPolicy =
                (String)model.getValue(CopyMediaParametersView.RM_POLICY);
            if ("true".equals(rmPolicy)) {
                rmh.setSet(ReservationMethodHelper.RM_SET);
            }

            // fs
            String rmFS =
                (String)model.getValue(CopyMediaParametersView.RM_FS);
            if ("true".equals(rmFS)) {
                rmh.setFS(ReservationMethodHelper.RM_FS);
            }
            int reservationMethod = rmh.getValue();

            wrapper.getArchiveCopy().
                setReservationMethod(reservationMethod);
            model.setValue(CopyMediaParametersView.RESERVATION_METHOD,
                           new Integer(reservationMethod));

            // now save the string version of reservation method for the
            // summary page
            summaryRMString = rmh.toString();
        }

        // for use
        selectedMediaType = mType;

        TraceUtil.trace3("Exiting");
        // if we get here, everything worked fine
        return true;
    }

    /**
     * translate the vsn range and a vsn list to one string
     */
    private String getSpecifiedVSNString(String startVSN,
                                       String endVSN,
                                       String list) {

        NonSyncStringBuffer returnStringBuffer = new NonSyncStringBuffer();

        if ((startVSN.length() != 0) && (endVSN.length() != 0)) {
            returnStringBuffer.append(
                SamUtil.getResourceString(
                    "NewPolicyWizard.specifyVSN",
                    new String[] {startVSN, endVSN}));
        }

        if (list.length() != 0) {
            if (returnStringBuffer.length() != 0) {
                returnStringBuffer.append(", ");
            }
            return returnStringBuffer.append(list).toString();
        } else {
            return returnStringBuffer.toString();
        }
    }

    /**
     * setJavascriptErrorMessage(wizardEvent, labelName, message, setLabel)
     * labelName = Name of the label of which you want to highlight
     * message   = Javascript message in pop up
     * setLabel  = boolean set to false for previous button
     */
    private void setJavascriptErrorMessage(
        WizardEvent event, String labelName, String message, boolean setLabel) {
        event.setSeverity(WizardEvent.ACKNOWLEDGE);
        event.setErrorMessage(message);
        if (setLabel) {
            model.setValue(NewPolicyWizardImpl.VALIDATION_ERROR, labelName);
        }
    }

    /**
     * getter methods for the summary pages. These methods are called from the
     * specific impl classes to set summary page values : NewPolicyWizardImpl
     * and NewCopyWizardImpl
     *
     */

    public String getSummaryArchiveAge() {
        return this.summaryArchiveAge;
    }

    public String getSummaryArchiveType() {
        return this.summaryArchiveType;
    }

    public String getSummaryDiskVSNName() {
        return this.summaryDiskVSNName;
    }

    public String getSummaryDiskVSNPath() {
        return this.summaryDiskVSNPath;
    }

    public String getSummaryDiskVSNHost() {
        return this.summaryDiskVSNHost;
    }

    public String getSummaryMediaType() {
        return this.summaryMediaType;
    }

    public String getSummaryVSNPoolName() {
        return this.summaryVSNPoolName;
    }

    public String getSummarySpecifiedVSNs() {
        return this.summarySpecifiedVSNs;
    }

    public String getSummaryRMString() {
        return this.summaryRMString;
    }

    public int getMediaType() {
        return this.selectedMediaType;
    }
}
