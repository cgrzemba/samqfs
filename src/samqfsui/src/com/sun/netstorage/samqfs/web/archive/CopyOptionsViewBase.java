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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

// ident	$Id: CopyOptionsViewBase.java,v 1.16 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.common.CCPagelet;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCTextField;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;

public abstract class CopyOptionsViewBase extends RequestHandlingViewBase
    implements CopyOptions, CCPagelet {
    protected CCPropertySheetModel model = null;

    // the archive copy whose options we are editing
    // NOTE: this variable should never be referenced directly, instead call
    // this.getCurrentArchiveCopy
    protected ArchiveCopy _theCopy = null;

    protected CopyOptionsViewBase(View parent,
                                  String psFileName,
                                  String name) {
        super(parent, name);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // create the propertysheet model
        createModel(psFileName);

        setDefaultModel(model);

        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    /** register this view's children */
    public void registerChildren() {
        PropertySheetUtil.registerChildren(this, model);
    }

    /** create a named child view of this model */
    public View createChild(String name) {
        if (PropertySheetUtil.isChildSupported(model, name)) {
            return PropertySheetUtil.createChild(this, model, name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    /**
     * create an instance of the property sheet model
     */
    public void createModel(String xmlFile) {
        model = PropertySheetUtil.createModel(xmlFile);
    }

    /**
     * implements the
     * @see com.sun.web.ui.common.CCPagelet#getPageletUrl method
     *
     * Each of the subclasses must implement this method and return the
     * appropriate JSP
     */
    public abstract String getPageletUrl();

    /**
     * implement this method to load the already set copy options
     */
    public abstract void loadCopyOptions() throws SamFSException;

    /**
     * implement this method to respond to user input
     */
    public abstract List validateCopyOptions() throws SamFSException;

    /**
     * retrieve the current copy
     *
     */
    protected ArchiveCopy getCurrentArchiveCopy() throws SamFSException {
        TraceUtil.trace3("Entering");
        if (this._theCopy == null) {
            CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
            String serverName = parent.getServerName();
            String policyName = (String)
                parent.getPageSessionAttribute(Constants.Archive.POLICY_NAME);
            Integer copyNumber = (Integer)
                parent.getPageSessionAttribute(Constants.Archive.COPY_NUMBER);

            SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
            ArchivePolicy thePolicy = sysModel.
                getSamQFSSystemArchiveManager().getArchivePolicy(policyName);
            this._theCopy = thePolicy.getArchiveCopy(copyNumber.intValue());
        }

        TraceUtil.trace3("Exiting");
        return this._theCopy;
    }

    /**
     *
     * used to initialize the fields that are common between the Disk and Tape
     * Archiving Copies
     */
    protected void loadCommonCopyOptions() throws SamFSException {
        TraceUtil.trace3("Entering");

        // retrieve the relevant copy object
        ArchiveCopy theCopy = getCurrentArchiveCopy();

        // sort method
        int sortMethod = theCopy.getArchiveSortMethod();
        if (sortMethod == -1) {
            sortMethod = ArchivePolicy.SM_NONE;
        }
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(SORT_METHOD);
        dropDown.setValue(Integer.toString(sortMethod));

        // unarchive time reference
        int unarchiveTimeRef = theCopy.getUnarchiveTimeReference();
        if (unarchiveTimeRef == -1) {
            unarchiveTimeRef = ArchivePolicy.UNARCH_TIME_REF_ACCESS;
        }
        dropDown = (CCDropDownMenu)getChild(UNARCHIVE_TIME_REF);
        dropDown.setValue(Integer.toString(unarchiveTimeRef));

		// join  method
        String joinMethod = theCopy.getJoinMethod() == ArchivePolicy.JOIN_PATH
                            ? "true" : "false";
        CCCheckBox checkBox = (CCCheckBox)getChild(JOIN_METHOD);
        checkBox.setValue(joinMethod);

        // offline copy method
        int offlineMethod = theCopy.getOfflineCopyMethod();
        if (offlineMethod == -1) {
            offlineMethod = ArchivePolicy.OC_NONE;
        }
        dropDown = (CCDropDownMenu)getChild(OFFLINE_METHOD);
        dropDown.setValue(Integer.toString(offlineMethod));

        // maximum size for archive
        long maxSizeForArchive = theCopy.getArchiveMaxSize();
        CCTextField textField;
        dropDown = (CCDropDownMenu)getChild(MAX_SIZE_ARCHIVE_UNIT);
        if (maxSizeForArchive != -1) {
            textField = (CCTextField)getChild(MAX_SIZE_ARCHIVE);
            textField.setValue(Long.toString(maxSizeForArchive));

            dropDown.setValue(
                Integer.toString(theCopy.getArchiveMaxSizeUnit()));
        } else {
            dropDown.setValue(Integer.toString(SamQFSSystemModel.SIZE_MB));
        }

        // set buffer size
        int bufferSize = theCopy.getBufferSize();
        if (bufferSize != -1) {
            textField = (CCTextField)getChild(BUFFER_SIZE);
            textField.setValue(Integer.toString(bufferSize));
        }

        // lock buffer?
        String lockBuffer = theCopy.isBufferLocked() ? "true" : "false";
        checkBox = (CCCheckBox)getChild(LOCK_BUFFER);
        checkBox.setValue(lockBuffer);

        // start age
        long startAge = theCopy.getStartAge();
        dropDown = (CCDropDownMenu)getChild(START_AGE_UNIT);
        if (startAge != -1) {
            textField = (CCTextField)getChild(START_AGE);
            textField.setValue(Long.toString(startAge));

            dropDown.setValue(Integer.toString(theCopy.getStartAgeUnit()));
        } else {
            dropDown.setValue(Integer.toString(SamQFSSystemModel.TIME_MINUTE));
        }

        // start count
        int startCount = theCopy.getStartCount();
        if (startCount != -1) {
            textField = (CCTextField)getChild(START_COUNT);
            textField.setValue(Integer.toString(startCount));
        }

        // start size
        long startSize = theCopy.getStartSize();
        dropDown = (CCDropDownMenu)getChild(START_SIZE_UNIT);
        if (startSize != -1) {
            textField = (CCTextField)getChild(START_SIZE);
            textField.setValue(Long.toString(startSize));

            dropDown.setValue(Integer.toString(theCopy.getStartSizeUnit()));
        } else {
            dropDown.setValue(Integer.toString(SamQFSSystemModel.SIZE_MB));
        }

        // disable recycling for this policy?
        String disableRecycler = theCopy.isIgnoreRecycle() ? "true" : "false";
        checkBox = (CCCheckBox)getChild(DISABLE_RECYCLER);
        checkBox.setValue(disableRecycler);

        // high water mark
        int highWaterMark = theCopy.getRecycleHWM();
        if (highWaterMark != -1) {
            textField = (CCTextField)getChild(HIGH_WATER_MARK);
            textField.setValue(Integer.toString(highWaterMark));
        }

        // notification address
        textField = (CCTextField)getChild(EMAIL_ADDRESS);
        textField.setValue(theCopy.getNotificationAddress());

        // min vsn gain
        int minGain = theCopy.getMinGain();
        if (minGain != -1) {
            textField = (CCTextField)getChild(MIN_GAIN);
            textField.setValue(Integer.toString(minGain));
        }

        TraceUtil.trace3("Exiting");
    }

    /**
     * used to validate the fields that are common between the Disk and Tape
     * archiving copies
     */
    protected List validateCommonCopyOptions() throws SamFSException {
        TraceUtil.trace3("Entering");

        List errors = new ArrayList();

        ArchiveCopy theCopy = getCurrentArchiveCopy();

        // sort method
        int sortMethod =
            Integer.parseInt(getDisplayFieldStringValue(SORT_METHOD));
        theCopy.setArchiveSortMethod(sortMethod);

        // unarchive time reference
        int unarchiveTimeRef =
            Integer.parseInt(getDisplayFieldStringValue(UNARCHIVE_TIME_REF));
        theCopy.setUnarchiveTimeReference(unarchiveTimeRef);

        // join method
        int joinMethod = "true".equals(getDisplayFieldStringValue(JOIN_METHOD))
            ? ArchivePolicy.JOIN_PATH : ArchivePolicy.NO_JOIN;
        theCopy.setJoinMethod(joinMethod);

        // offline copy method
        int offlineCopyMethod =
            Integer.parseInt(getDisplayFieldStringValue(OFFLINE_METHOD));
        theCopy.setOfflineCopyMethod(offlineCopyMethod);

        // maximum size for archive
        String maxSizeString = getDisplayFieldStringValue(MAX_SIZE_ARCHIVE);
        maxSizeString = maxSizeString == null ? "" : maxSizeString.trim();

        if (!maxSizeString.equals("")) {
            CCLabel label =
                (CCLabel)getChild(MAX_SIZE_ARCHIVE.concat("Label"));

            try {
                long maxSize = Long.parseLong(maxSizeString);
                int maxSizeUnit = Integer.parseInt(
                    getDisplayFieldStringValue(MAX_SIZE_ARCHIVE_UNIT));

                if (PolicyUtil.isValidSize(maxSize, maxSizeUnit)) {
                    theCopy.setArchiveMaxSize(maxSize);
                    theCopy.setArchiveMaxSizeUnit(maxSizeUnit);
                } else {
                    errors.add("archiving.error.archivemax");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("archiving.error.archivemax");
                label.setShowError(true);
            }
        } else {
            theCopy.setArchiveMaxSize(-1);
            theCopy.setArchiveMaxSizeUnit(-1);
        }

        // memory buffer size
        String bufferSizeString = getDisplayFieldStringValue(BUFFER_SIZE);
        bufferSizeString =
            bufferSizeString == null ? "" : bufferSizeString.trim();

        if (!bufferSizeString.equals("")) {
            CCLabel label = (CCLabel)getChild(BUFFER_SIZE.concat("Label"));

            try {
                int bufferSize = Integer.parseInt(bufferSizeString);

                if (bufferSize < 2 || bufferSize > 1024) {
                    errors.add("ArchivePolCopy.error.buffSize");
                    label.setShowError(true);
                } else {
                    theCopy.setBufferSize(bufferSize);
                }
            } catch (NumberFormatException nfe) {
                errors.add("ArchivePolCopy.error.buffSize");
                label.setShowError(true);
            }
        } else {
            theCopy.setBufferSize(-1);
        }

        // lock buffer ?
        String lb = getDisplayFieldStringValue(LOCK_BUFFER);
        boolean lockBuffer = "true".equals(lb) ? true : false;
        theCopy.setBufferLocked(lockBuffer);

        // start age
        String startAgeString = getDisplayFieldStringValue(START_AGE);
        startAgeString = startAgeString == null ? "" : startAgeString.trim();

        if (!startAgeString.equals("")) {
            CCLabel label = (CCLabel)getChild(START_AGE.concat("Label"));

            try {
                long startAge = Long.parseLong(startAgeString);
                int startAgeUnit = Integer.parseInt(
                    getDisplayFieldStringValue(START_AGE_UNIT));

                if (PolicyUtil.isValidTime(startAge, startAgeUnit)) {
                    theCopy.setStartAge(startAge);
                    theCopy.setStartAgeUnit(startAgeUnit);
                } else {
                    errors.add("archiving.error.startage");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("archiving.error.startage");
                label.setShowError(true);
            }
        } else {
            theCopy.setStartAge(-1);
            theCopy.setStartAgeUnit(-1);
        }

        // start count
        String startCountString = getDisplayFieldStringValue(START_COUNT);
        startCountString =
            startCountString == null ? "" : startCountString.trim();

        if (!startCountString.equals("")) {
            CCLabel label = (CCLabel)getChild(START_COUNT.concat("Label"));

            try {
                int startCount = Integer.parseInt(startCountString);

                if (startCount > 0) {
                    theCopy.setStartCount(startCount);
                } else {
                    errors.add("ArchivePolCopy.error.startCount");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("ArchivePolCopy.error.startCount");
                label.setShowError(true);
            }
        } else {
           theCopy.setStartCount(-1);
        }

        // start size
        String startSizeString = getDisplayFieldStringValue(START_SIZE);
        startSizeString = startSizeString == null ? "" : startSizeString.trim();

        if (!startSizeString.equals("")) {
            CCLabel label = (CCLabel)getChild(START_SIZE.concat("Label"));

            try {
                long startSize = Long.parseLong(startSizeString);
                int startSizeUnit = Integer.parseInt(
                    getDisplayFieldStringValue(START_SIZE_UNIT));

                if (PolicyUtil.isValidSize(startSize, startSizeUnit)) {
                    theCopy.setStartSize(startSize);
                    theCopy.setStartSizeUnit(startSizeUnit);
                } else {
                    errors.add("archiving.error.startsize");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("archiving.error.startsize");
                label.setShowError(true);
            }
        } else {
            theCopy.setStartSize(-1);
            theCopy.setStartSizeUnit(-1);
        }

        // disable recycler ?
        boolean disableRecycler = "true".equals(
            getDisplayFieldStringValue(DISABLE_RECYCLER)) ? true : false;
        theCopy.setIgnoreRecycle(disableRecycler);

        // mail recycler information
        String email = getDisplayFieldStringValue(EMAIL_ADDRESS);
        email = email == null ? "" : email.trim();

        if (!email.equals("")) {
            CCLabel label = (CCLabel)getChild(EMAIL_ADDRESS.concat("Label"));

            if (email.indexOf("@") != -1) {
                theCopy.setNotificationAddress(email);
            } else {
                errors.add("ArchivePolCopy.error.mailAddress");
                label.setShowError(true);
            }
        } else {
            theCopy.setNotificationAddress("");
        }

        // highwater mark
        String hwString = getDisplayFieldStringValue(HIGH_WATER_MARK);
        hwString = hwString == null ? "" : hwString.trim();

        if (!hwString.equals("")) {
            CCLabel label = (CCLabel)getChild(HIGH_WATER_MARK.concat("Label"));

            try {
                int hw = Integer.parseInt(hwString);
                if (hw > 0 && hw <= 100) {
                    theCopy.setRecycleHWM(hw);
                } else {
                    errors.add("ArchivePolCopy.error.recycleHwm");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("ArchivePolCopy.error.recycleHwm");
                label.setShowError(true);
            }
        } else {
            theCopy.setRecycleHWM(-1);
        }

        // min gain
        String minGainString = getDisplayFieldStringValue(MIN_GAIN);
        minGainString = minGainString == null ? "" : minGainString.trim();

        if (!minGainString.equals("")) {
            CCLabel label = (CCLabel)getChild(MIN_GAIN.concat("Label"));

            try {
                int minGain = Integer.parseInt(minGainString);
                if (minGain > 0 && minGain <= 100) {
                    theCopy.setMinGain(minGain);
                } else {
                    errors.add("ArchivePolCopy.error.minimumGain");
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add("ArchivePolCopy.error.minimumGain");
                label.setShowError(true);
            }
        } else {
            theCopy.setMinGain(-1);
        }

        TraceUtil.trace3("Exiting");
        // return the list of errors
        return errors;
    }

    public void printErrorMessages(List errors) {
        TraceUtil.trace3("Entering");

        NonSyncStringBuffer buffer = new NonSyncStringBuffer();
        Iterator it = errors.iterator();

        CommonViewBeanBase parent = (CommonViewBeanBase)getParentViewBean();
        String serverName = parent.getServerName();

        buffer.append("<ul>");
        while (it.hasNext()) {
            String error = (String)it.next();
            buffer.append("<li>");
            buffer.append(SamUtil.getResourceString(error)).append("<br>");
            buffer.append("</li>");
        }
        buffer.append("</ul>");

        SamUtil.setErrorAlert(parent,
                              parent.CHILD_COMMON_ALERT,
                              "ArchivePolCopy.error.save",
                              -2026,
                              buffer.toString(),
                              serverName);

        TraceUtil.trace3("Exiting");
    }

    /**
     * Hide fill vsns if it is not set
     */
    public boolean beginJoinMethodDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        try {
            return getCurrentArchiveCopy().getJoinMethod()
                                        == ArchivePolicy.JOIN_PATH;
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Exception caught while checking joinPath!");
        }
        return false;
    }

    /**
     * Hide fill vsns label if it is not set
     */
    public boolean beginJoinMethodLabelDisplay(ChildDisplayEvent event)
        throws ModelControlException {

        try {
            return getCurrentArchiveCopy().getJoinMethod()
                                        == ArchivePolicy.JOIN_PATH;
        } catch (SamFSException samEx) {
            TraceUtil.trace1("Exception caught while checking joinPath!");
        }
        return false;
    }
}
