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

// ident	$Id: CopyMediaParametersView.java,v 1.14 2008/03/17 14:43:30 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive.wizards;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.Model;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.RequestHandlingViewBase;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.archive.PolicyUtil;
import com.sun.netstorage.samqfs.web.archive.ReservationMethodHelper;
import com.sun.netstorage.samqfs.web.archive.SelectableGroupHelper;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveCopyGUIWrapper;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaCopy;
import com.sun.netstorage.samqfs.web.model.archive.ArchiveVSNMap;
import com.sun.netstorage.samqfs.web.model.archive.VSNPool;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.wizard.SamWizardModel;
import com.sun.web.ui.view.alert.CCAlertInline;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import com.sun.web.ui.view.wizard.CCWizardPage;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;

public class CopyMediaParametersView extends RequestHandlingViewBase
    implements CCWizardPage {
    public static final String PAGE_NAME = "CopyMediaParametersView";

    // this view's children
    public static final String ALERT = "Alert";
    public static final String ARCHIVE_AGE = "archiveAge";
    public static final String ARCHIVE_AGE_LABEL = "archiveAgeLabel";
    public static final String ARCHIVE_AGE_UNIT = "archiveAgeUnit";
    public static final String SELECT_VSN_LABEL = "selectVSNLabel";
    public static final String MEDIA_TYPE_LABEL = "mediaTypeLabel";
    public static final String MEDIA_TYPE = "mediaType";
    public static final String POOL_NAME_LABEL = "poolNameLabel";
    public static final String POOL_NAME = "poolName";
    public static final String RANGE_LABEL = "rangeLabel";
    public static final String FROM = "from";
    public static final String TO_LABEL = "toLabel";
    public static final String TO = "to";
    public static final String LIST_LABEL = "listLabel";
    public static final String LIST = "list";

    // reservation method
    public static final String RM_LABEL = "rmLabel";
    public static final String RM_ATTRIBUTES = "rmAttributes";
    public static final String RM_POLICY = "rmPolicy";
    public static final String RM_FS = "rmFS";
    public static final String RESERVATION_METHOD = "reservation_method_int";

    public static final String SELECTED_VSN_RADIO = "selected_vsn_radio";
    public static final String EXISTING_VSN = "existingvsn";
    public static final String NEW_DISK_VSN  = "newdiskvsn";

    // inline helpe display fields
    public static final String MT_LABEL = "ArchiveMediaTypeText";
    public static final String VSN_INSTRUCTION = "HelpText";
    public static final String FROMTO_HELP = "fromToInlineHelp";
    public static final String LIST_HELP = "listInlineHelp";

    // helper hidden fields
    public static final String ERROR_FIELD = "errorOccur";
    public static final String POOL_MEDIA_TYPES = "poolMediaTypes";
    public static final String ALL_POOLS = "allPools";

    // request-scoped objects
    public static final String DEFAULT_MEDIA_TYPE =
        "_defaultly_selected_media_type_";

    public CopyMediaParametersView(View parent, Model model) {
        this(parent, model, PAGE_NAME);
    }

    public CopyMediaParametersView(View parent, Model model, String pageName) {
        super(parent, pageName);

        setDefaultModel(model);
        registerChildren();
    }

    public void registerChildren() {
        registerChild(ALERT, CCAlertInline.class);
        registerChild(ARCHIVE_AGE, CCTextField.class);
        registerChild(ARCHIVE_AGE_LABEL, CCLabel.class);
        registerChild(ARCHIVE_AGE_UNIT, CCDropDownMenu.class);
        registerChild(SELECT_VSN_LABEL, CCLabel.class);
        registerChild(MEDIA_TYPE_LABEL, CCLabel.class);
        registerChild(MEDIA_TYPE, CCDropDownMenu.class);
        registerChild(POOL_NAME_LABEL, CCLabel.class);
        registerChild(POOL_NAME, CCDropDownMenu.class);
        registerChild(RANGE_LABEL, CCLabel.class);
        registerChild(FROM, CCTextField.class);
        registerChild(TO_LABEL, CCLabel.class);
        registerChild(TO, CCTextField.class);
        registerChild(LIST_LABEL, CCLabel.class);
        registerChild(LIST, CCTextField.class);
        registerChild(RM_ATTRIBUTES, CCDropDownMenu.class);
        registerChild(RM_POLICY, CCCheckBox.class);
        registerChild(RM_FS, CCCheckBox.class);
        registerChild(RM_LABEL, CCLabel.class);
        registerChild(MT_LABEL, CCLabel.class);
        registerChild(VSN_INSTRUCTION, CCStaticTextField.class);
        registerChild(FROMTO_HELP, CCStaticTextField.class);
        registerChild(LIST_HELP, CCStaticTextField.class);
        registerChild(POOL_MEDIA_TYPES, CCHiddenField.class);
        registerChild(ALL_POOLS, CCHiddenField.class);
        registerChild(ERROR_FIELD, CCHiddenField.class);
    }

    public View createChild(String name) {
        if (name.equals(ERROR_FIELD) ||
            name.equals(POOL_MEDIA_TYPES) ||
            name.equals(ALL_POOLS)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(ALERT)) {
            return new CCAlertInline(this, name, null);
        } else if (name.equals(ARCHIVE_AGE_UNIT) ||
                   name.equals(MEDIA_TYPE) ||
                   name.equals(POOL_NAME) ||
                   name.equals(RM_ATTRIBUTES)) {
            return new CCDropDownMenu(this, name, null);
        } else if (name.equals(ARCHIVE_AGE_LABEL) ||
                   name.equals(SELECT_VSN_LABEL) ||
                   name.equals(MEDIA_TYPE_LABEL) ||
                   name.equals(POOL_NAME_LABEL) ||
                   name.equals(RANGE_LABEL) ||
                   name.equals(TO_LABEL) ||
                   name.equals(LIST_LABEL) ||
                   name.equals(RM_LABEL) ||
                   name.equals(MT_LABEL)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(ARCHIVE_AGE) ||
                   name.equals(FROM) ||
                   name.equals(TO) ||
                   name.equals(LIST)) {
            return new CCTextField(this, name, null);
        } else if (name.equals(VSN_INSTRUCTION) ||
                   name.equals(FROMTO_HELP) ||
                   name.equals(LIST_HELP)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(RM_POLICY) ||
                   name.equals(RM_FS)) {
            return new CCCheckBox(this, name, "true", "false", false);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    private void populateDropDownMenus()
        throws SamFSException {

        // archive age units
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(ARCHIVE_AGE_UNIT);
        dropDown.setOptions(new OptionList(SelectableGroupHelper.Time.labels,
                                           SelectableGroupHelper.Time.values));

        String serverName = getServerName();

        // media types -- including disk
        SamQFSSystemModel sysModel = SamUtil.getModel(serverName);
        int [] mediaTypes = sysModel.getSamQFSSystemMediaManager().
            getAvailableArchiveMediaTypes();

        String [] mediaLabels = new String[mediaTypes.length];
        String [] mediaValues = new String[mediaTypes.length];

        for (int i = 0; i < mediaTypes.length; i++) {
            mediaLabels[i] = SamUtil.getMediaTypeString(mediaTypes[i]);
            mediaValues[i] = Integer.toString(mediaTypes[i]);
        }

        dropDown = (CCDropDownMenu)getChild(MEDIA_TYPE);
        dropDown.setOptions(new OptionList(mediaLabels, mediaValues));

        // vsn pools
        VSNPool [] pools = PolicyUtil.getAllVSNPools(serverName);

        // set the vsn pool -> media type hidden fields
        setPoolMediaTypesString(mediaTypes, pools);

        // Pools are populated in initializaDisplayFields due to its
        // dependency on the mediaType

        // reservation method attributes
        dropDown = (CCDropDownMenu)getChild(RM_ATTRIBUTES);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.ReservationMethod.labels,
            SelectableGroupHelper.ReservationMethod.values));

    }

    private void setPoolMediaTypesString(int [] types,
                                         VSNPool [] pool) {
        HashMap map = new HashMap(types.length);
        ArrayList list = new ArrayList();
        NonSyncStringBuffer buffer2 = new NonSyncStringBuffer();

        for (int i = 0; i < pool.length; i++) {
            Integer key = new Integer(pool[i].getMediaType());

            ArrayList value = (ArrayList)map.get(key);

            if (value == null) {
                value = new ArrayList();
                value.add(SelectableGroupHelper.NOVAL);
                map.put(key, value);
            }
            buffer2.append(pool[i].getPoolName()).append(";");
            value.add(pool[i].getPoolName());
        }

        Iterator it1 = map.keySet().iterator();
        NonSyncStringBuffer buffer1 = new NonSyncStringBuffer();
        while (it1.hasNext()) {
            Integer poolMediaType = (Integer)it1.next();
            buffer1.append(poolMediaType).append("=");
            ArrayList poolNames = (ArrayList)map.get(poolMediaType);

            Iterator it2 = poolNames.iterator();
            while (it2.hasNext()) {
                String poolName = (String)it2.next();
                buffer1.append(poolName).append(",");
            }
            buffer1.append(";");
        }

        // now set the values in the hidden fields
        CCHiddenField field = (CCHiddenField)getChild(ALL_POOLS);
        field.setValue(buffer2.toString());

        field = (CCHiddenField)getChild(POOL_MEDIA_TYPES);
        field.setValue(buffer1.toString());
    }

    /**
     * this method must start by checking the wizard model for any preset
     * values
     */
    private void initializeDisplayFields(
        CopyInfo copyInfo, boolean isCopyWizard) throws SamFSException {

        // Grab the wrapper from the CopyInfo datastructure, then grab the
        // ArchiveCopy object to start prepopulating information
        ArchiveCopyGUIWrapper myWrapper = null;
        ArchivePolCriteriaCopy criteriaCopy = null;
        ArchiveCopy archiveCopy = null;
        ArchiveVSNMap archiveVSNMap = null;
        long archiveAge = -1;
        int archiveAgeUnit = -1;
        int mediaType = -1;
        int copyNumber = -1;
        String poolName = null;
        String list = null;
        String rangeTo = null;
        String rangeFrom = null;

        if (copyInfo != null) {
            myWrapper = copyInfo.getCopyWrapper();
            criteriaCopy = myWrapper.getArchivePolCriteriaCopy();
            archiveCopy = myWrapper.getArchiveCopy();
            archiveVSNMap = archiveCopy.getArchiveVSNMap();

            archiveAge = criteriaCopy.getArchiveAge();
            archiveAgeUnit = criteriaCopy.getArchiveAgeUnit();
            mediaType  = archiveVSNMap.getArchiveMediaType();
            poolName = archiveVSNMap.getPoolExpression();
            list = archiveVSNMap.getMapExpression();
            rangeTo = archiveVSNMap.getMapExpressionEndVSN();
            rangeFrom = archiveVSNMap.getMapExpressionStartVSN();
        }

        // Only used if this view is being used in NewCopyWizard
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // Set warning for mix media setup if copy number is 1
        if (getCopyNumber() == 1 || isCopyWizard) {
            ((CCStaticTextField) getChild(VSN_INSTRUCTION)).setValue(
                "NewPolicyWizard.copymediaparameter.help.firstcopy");
        } else {
            ((CCStaticTextField) getChild(VSN_INSTRUCTION)).setValue(
                "NewPolicyWizard.copymediaparameter.help");
        }

        // archive age : default 4 minutes
        if (archiveAge == -1) {
            archiveAge = SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE;
        }

        if (archiveAgeUnit == -1) {
            archiveAgeUnit =
                SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT;
        }

        if (!isCopyWizard) {
            ((CCTextField) getChild(
                ARCHIVE_AGE)).setValue(Long.toString(archiveAge));
            ((CCDropDownMenu) getChild(
                ARCHIVE_AGE_UNIT)).setValue(Integer.toString(archiveAgeUnit));
        } else {
            // Prepopulate Archive Age if it's not set yet
            String archiveAgeStr = (String) wizardModel.getValue(ARCHIVE_AGE);
            if (archiveAgeStr == null || archiveAgeStr.length() == 0) {
                ((CCTextField) getChild(ARCHIVE_AGE)).
                    setValue(Long.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE));
                ((CCDropDownMenu) getChild(ARCHIVE_AGE_UNIT)).
                    setValue(Integer.toString(
                        SamQFSSystemArchiveManager.DEFAULT_ARCHIVE_AGE_UNIT));
            }

        }
        // media type
        if (!isCopyWizard) {
            if (mediaType == -1) {
                mediaType = BaseDevice.MTYPE_DISK;
            }
            ((CCDropDownMenu) getChild(
                MEDIA_TYPE)).setValue(Integer.toString(mediaType));
        } else {
            String mediaTypeStr = (String) wizardModel.getValue(MEDIA_TYPE);
            if (mediaTypeStr != null && mediaTypeStr.length() != 0) {
                mediaType =
                    Integer.parseInt((String) wizardModel.getValue(MEDIA_TYPE));
            } else {
                mediaType = BaseDevice.MTYPE_DISK;
            }
        }

        // pool name
        String [] VSNPoolNames =
            PolicyUtil.getVSNPoolNames(mediaType, getServerName());
        String [] poolNames = new String[VSNPoolNames.length + 1];
        String [] poolValues = new String[VSNPoolNames.length + 1];
        poolNames[0] = SelectableGroupHelper.NOVAL_LABEL;
        poolValues[0] = SelectableGroupHelper.NOVAL;
        for (int i = 0, j = 1; i < VSNPoolNames.length; i++, j++) {
            poolNames[j] = VSNPoolNames[i];
            poolValues[j] = VSNPoolNames[i];
        }

        CCDropDownMenu dropDown = (CCDropDownMenu) getChild(POOL_NAME);
        dropDown.setOptions(new OptionList(poolNames, poolValues));

        if (poolName == null) {
            poolName = SelectableGroupHelper.NOVAL;
        }

        if (!isCopyWizard) {
            dropDown.setValue(poolName);
        }

        // vsn range -- from
        rangeFrom = rangeFrom == null ? "" : rangeFrom.trim();
        ((CCTextField) getChild(FROM)).setValue(rangeFrom);

        // vsn ramge -- to
        rangeTo = rangeTo == null ? "" : rangeTo.trim();
        ((CCTextField) getChild(TO)).setValue(rangeTo);

        // vsn list
        list = list == null ? "" : list.trim();
        ((CCTextField)getChild(LIST)).setValue(list);

        // reservation method
        // don't bother with reservation method if dealing with disk
        if (mediaType != BaseDevice.MTYPE_DISK && !isCopyWizard) {
            ArchiveCopy copy = myWrapper.getArchiveCopy();

            // Reserve
            int reserveMethod = copy.getReservationMethod();

            // default to no reservation method
            String serverName = getServerName();
            ReservationMethodHelper rmh =  new ReservationMethodHelper();

            rmh.setValue(reserveMethod);

            // set the attributes drop down
            CCDropDownMenu rmAttributes =
                (CCDropDownMenu)getChild(RM_ATTRIBUTES);
            if (rmh.getAttributes() == 0) {
                rmAttributes.setValue(SelectableGroupHelper.NOVAL);
            } else {
                rmAttributes.setValue(Integer.toString(rmh.getAttributes()));
            }

            CCCheckBox rmPolicy = (CCCheckBox) getChild(RM_POLICY);
            String policy =  Boolean.toString(rmh.getSet() != 0);
            rmPolicy.setValue(policy);
            rmPolicy.resetStateData();

            // set the fs check box
            CCCheckBox rmFS = (CCCheckBox) getChild(RM_FS);
            String fs = Boolean.toString(rmh.getFS() != 0);
            rmFS.setValue(fs);
            rmFS.resetStateData();

            disableReservationMethod(false);
        } else if (isCopyWizard && mediaType != BaseDevice.MTYPE_DISK) {
            disableReservationMethod(false);
            ((CCCheckBox) getChild(RM_POLICY)).resetStateData();
            ((CCCheckBox) getChild(RM_FS)).resetStateData();
        } else {
            disableReservationMethod(true);
        }
    }

    private void disableReservationMethod(boolean disable) {
        CCDropDownMenu rmAttributes = (CCDropDownMenu)getChild(RM_ATTRIBUTES);
        rmAttributes.setDisabled(disable);

        CCCheckBox checkbox = (CCCheckBox)getChild(RM_POLICY);
        checkbox.setDisabled(disable);

        checkbox = (CCCheckBox)getChild(RM_FS);
        checkbox.setDisabled(disable);

        if (disable) {
            rmAttributes.setValue(Integer.toString(0));
            checkbox.setValue(Boolean.toString(false));
            checkbox.setValue(Boolean.toString(false));
        }
    }

    // begin pagelet display
    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        String serverName = getServerName();

        /**
         * This view is being used by both the NewArchivePolicyWizard and the
         * NewCopyWizard.  In order to populate the fields in this page
         * correctly, we have to pass in the CopyInfo object if this class is
         * being used by the NewArchivePolicyWizard.  This is because this page
         * may be used by multiple copies and thus we need to be careful on
         * presenting the values that users may have already entered.
         *
         * Simply passing null for CopyInfo parameter if this view is being
         * used by the NewCopyWizard.
         */

        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();
        HashMap copyNumberHashMap =
            (HashMap) wizardModel.getValue(NewPolicyWizardImpl.COPY_HASHMAP);
        CopyInfo copyInfo =
            copyNumberHashMap == null ?
                null :getCopyInfoObject(copyNumberHashMap);

        try {
            populateDropDownMenus();

            // if copyNumberHashMap is null, this view is being used in
            // New Copy Wizard
            initializeDisplayFields(copyInfo, copyNumberHashMap == null);
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                     getClass(),
                                     "beingDisplay",
                                     "Error will initializing page",
                                     serverName);

            SamUtil.setErrorAlert(this,
                                  ALERT,
                           "an exception occurred while populating dropdowns",
                                  sfe.getSAMerrno(),
                                  sfe.getMessage(),
                                  serverName);
        }

        // TODO: do this as part of the main process
        Integer mediaType = (Integer)RequestManager.getRequestContext().
            getRequest().getAttribute(DEFAULT_MEDIA_TYPE);

        if (mediaType != null &&
            mediaType.intValue() == BaseDevice.MTYPE_DISK) {
            disableReservationMethod(true);
        }

        // Set label to red if error is detected
        setErrorLabel((SamWizardModel) getDefaultModel(), copyInfo != null);
    }

    // implement the CCWizardPage
    public String getPageletUrl() {
        return "/jsp/archive/wizards/CopyMediaParameters.jsp";
    }

    private String getServerName() {
        SamWizardModel model = (SamWizardModel)getDefaultModel();
        String serverName = (String)
            model.getValue(Constants.PageSessionAttributes.SAMFS_SERVER_NAME);

        return serverName;
    }


    /**
     * Retrieve the Copy Info structure to populate info
     */
    private CopyInfo getCopyInfoObject(HashMap copyNumberHashMap) {
        // Grab the HashMap that contains copy information from wizardModel
        SamWizardModel wizardModel = (SamWizardModel) getDefaultModel();

        // HashMap is null if called in New Copy Wizard
        if (copyNumberHashMap == null) {
            return null;
        }

        // Grab the copy number that this page represents,
        // the copy number is assigned in getPageClass()
        Integer copyNumber =
            (Integer) wizardModel.getValue(Constants.Wizard.COPY_NUMBER);

        // Retrieve the CopyInfo data structure from the HashMap
        CopyInfo info = (CopyInfo) copyNumberHashMap.get(copyNumber);

        return info;
    }

    private int getCopyNumber() {
        Integer copyNumber = (Integer) ((SamWizardModel) getDefaultModel()).
            getValue(Constants.Wizard.COPY_NUMBER);
        return
            copyNumber == null ? -1 : copyNumber.intValue();
    }

    private void setErrorLabel(
        SamWizardModel wizardModel, boolean isPolicyWizard) {
        String labelName = (String) wizardModel.getValue(
            isPolicyWizard ?
                NewPolicyWizardImpl.VALIDATION_ERROR :
                NewCopyWizardImpl.VALIDATION_ERROR);
        if (labelName == null || labelName == "") {
            return;
        }

        CCLabel theLabel = (CCLabel) getChild(labelName);
        if (theLabel != null) {
            theLabel.setShowError(true);
        }

        // reset wizardModel field
        if (isPolicyWizard) {
            wizardModel.setValue(
                NewPolicyWizardImpl.VALIDATION_ERROR, "");
        } else {
            wizardModel.setValue(
                NewCopyWizardImpl.VALIDATION_ERROR, "");
        }
    }
}
