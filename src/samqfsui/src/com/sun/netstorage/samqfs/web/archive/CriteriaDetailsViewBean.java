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

// ident	$Id: CriteriaDetailsViewBean.java,v 1.24 2008/11/05 20:24:48 ronaldso Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.RequestManager;
import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.web.fs.FSArchivePoliciesViewBean;
import com.sun.netstorage.samqfs.web.fs.FSDetailsViewBean;
import com.sun.netstorage.samqfs.web.fs.FSSummaryViewBean;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.fs.FileSystem;
import com.sun.netstorage.samqfs.web.util.Authorization;
import com.sun.netstorage.samqfs.web.util.BreadCrumbUtil;
import com.sun.netstorage.samqfs.web.util.CommonViewBeanBase;
import com.sun.netstorage.samqfs.web.util.Constants;
import com.sun.netstorage.samqfs.web.util.PageInfo;
import com.sun.netstorage.samqfs.web.util.PageTitleUtil;
import com.sun.netstorage.samqfs.web.util.PropertySheetUtil;
import com.sun.netstorage.samqfs.web.util.SamUtil;
import com.sun.netstorage.samqfs.web.util.SecurityManagerFactory;
import com.sun.netstorage.samqfs.web.util.TraceUtil;
import com.sun.web.ui.model.CCActionTableModel;
import com.sun.web.ui.model.CCBreadCrumbsModel;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCStaticTextField;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import javax.servlet.ServletContext;
import javax.servlet.ServletException;

public class CriteriaDetailsViewBean extends CommonViewBeanBase {
    public static final String DETAILS_VIEW = "CriteriaDetailsView";

    private static final String PAGE_NAME = "CriteriaDetails";
    private static final String DEFAULT_URL =
        "/jsp/archive/CriteriaDetails.jsp";

    // breadcrumbing children
    public static final String BREADCRUMB = "BreadCrumb";

    public static final String POLICY_SUMMARY_HREF = "PolicySummaryHref";
    public static final String POLICY_DETAILS_HREF = "PolicyDetailsHref";
    public static final String CRITERIA_DETAILS_HREF = "CriteriaDetailsHref";
    public static final String FS_SUMMARY_HREF = "FileSystemSummaryHref";
    public static final String FS_DETAILS_HREF = "FileSystemDetailsHref";
    public static final String FS_ARCHIVEPOL_HREF = "FSArchivePolicyHref";

    // children
    private static final String MESSAGE = "message";
    public static final String GLOBAL_CRITERIA_TEXT = "globalCriteriaText";

    // apply fs popup helper fields
    public static final String PS_ATTRIBUTES = "psAttributes";
    public static final String FS_LIST = "fsList";
    public static final String FS_NAME = "fsname";

    // field to keep track of is fs is removeable or not
    public static final String FS_DELETABLE = "fsDeletable";
    public static final String FS_DELETE_CONFIRMATION = "fsDeleteConfirmation";

    public static final String SERVER_NAME = "ServerName";

    // property sheet children
    public static final String STARTING_DIR = "StartingDir";
    public static final String NAME_PATTERN = "NamePattern";
    public static final String MIN_SIZE = "MinimumSize";
    public static final String MIN_SIZE_UNITS = "MinimumSizeUnits";
    public static final String MAX_SIZE = "MaximumSize";
    public static final String MAX_SIZE_UNITS = "MaximumSizeUnits";
    public static final String OWNER = "Owner";
    public static final String GROUP = "Group";
    public static final String ACCESS_AGE = "AccessAge";
    public static final String ACCESS_AGE_UNITS = "AccessAgeUnits";

    // table models
    private Map models = null;
    private CCPropertySheetModel psModel = null;
    private CCPageTitleModel ptModel = null;

    private String dupPolName = null, dupCriteriaName = null;

    public CriteriaDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        initializeTableModels();
        psModel = PropertySheetUtil.createModel(
            "/jsp/archive/CriteriaDetailsPropertySheet.xml");
        ptModel = PageTitleUtil.createModel(
            "/jsp/archive/CriteriaDetailsPageTitle.xml");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        super.registerChildren();
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(POLICY_SUMMARY_HREF, CCHref.class);
        registerChild(POLICY_DETAILS_HREF, CCHref.class);
        registerChild(CRITERIA_DETAILS_HREF, CCHref.class);
        registerChild(FS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_DETAILS_HREF, CCHref.class);
        registerChild(FS_ARCHIVEPOL_HREF, CCHref.class);
        registerChild(FS_NAME, CCHiddenField.class);
        registerChild(FS_DELETABLE, CCHiddenField.class);
        registerChild(PS_ATTRIBUTES, CCHiddenField.class);
        registerChild(FS_LIST, CCHiddenField.class);
        registerChild(FS_DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(MESSAGE, CCStaticTextField.class);
        registerChild(GLOBAL_CRITERIA_TEXT, CCStaticTextField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        PropertySheetUtil.registerChildren(this, psModel);
        registerChild(DETAILS_VIEW, CriteriaDetailsView.class);
        PageTitleUtil.registerChildren(this, ptModel);
    }

    public View createChild(String name) {
        if (name.equals(MESSAGE)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            CCBreadCrumbsModel bcModel =
                new CCBreadCrumbsModel("CriteriaDetails.title");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (name.equals(POLICY_SUMMARY_HREF) ||
                   name.equals(POLICY_DETAILS_HREF) ||
                   name.equals(CRITERIA_DETAILS_HREF) ||
                   name.equals(FS_SUMMARY_HREF) ||
                   name.equals(FS_DETAILS_HREF) ||
                   name.equals(FS_ARCHIVEPOL_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(FS_NAME) ||
                   name.equals(FS_DELETABLE) ||
                   name.equals(FS_DELETE_CONFIRMATION) ||
                   name.equals(PS_ATTRIBUTES) ||
                   name.equals(FS_LIST)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, getServerName());
        } else if (name.equals(GLOBAL_CRITERIA_TEXT)) {
            return new CCStaticTextField(this, name, null);
        } else if (name.equals(DETAILS_VIEW)) {
            return new CriteriaDetailsView(this, models, name);
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (PropertySheetUtil.isChildSupported(psModel, name)) {
            return PropertySheetUtil.createChild(this, psModel, name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    private void initializeTableModels() {
        models = new HashMap(2);
        ServletContext sc =
            RequestManager.getRequestContext().getServletContext();

        // copy settings table
        CCActionTableModel model = new CCActionTableModel(
            sc, "/jsp/archive/CriteriaDetailsCopyTable.xml");
        models.put(CriteriaDetailsView.COPY_TABLE, model);

        // fs using criteria table
        model = new CCActionTableModel(
            sc, "/jsp/archive/CriteriaDetailsFSTable.xml");
        models.put(CriteriaDetailsView.FS_TABLE, model);
    }

    private void initializeDropDownMenus() {
        // minimum size drop down
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(MIN_SIZE_UNITS);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Sizes.labels,
            SelectableGroupHelper.Sizes.values));

        // msximum size drop down
        dropDown = (CCDropDownMenu)getChild(MAX_SIZE_UNITS);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Sizes.labels,
            SelectableGroupHelper.Sizes.values));

        // access age
        dropDown = (CCDropDownMenu)getChild(ACCESS_AGE_UNITS);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.Times.labels,
            SelectableGroupHelper.Times.values));
    }

    /**
     * loads the saved property sheet part of the policy criteria
     */
    private void loadPolicyMatchCriteria() throws SamFSException {
        // retrieve the model first
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        // check duplicate
        String duplicate = (String) getPageSessionAttribute("DUPLICATE");
        if (duplicate != null && duplicate.equals("true")) {
            // resync the policy object model
            sysModel.getSamQFSSystemArchiveManager().getAllArchivePolicies();
        }

        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer criteriaNumber = (Integer)
            getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        // the server name
        ArchivePolicy thePolicy = sysModel.
            getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

        if (thePolicy == null)
            throw new SamFSException(null, -2000);

        ArchivePolCriteria criteria =
            thePolicy.getArchivePolCriteria(criteriaNumber.intValue());

        if (criteria == null)
            throw new SamFSException(null, 2020);

        ArchivePolCriteriaProp property =
            criteria.getArchivePolCriteriaProperties();

        // starting dir
        CCTextField field = (CCTextField)getChild(STARTING_DIR);
        field.setValue(property.getStartingDir());

        // name pattern
        field = (CCTextField)getChild(NAME_PATTERN);
        field.setValue(property.getNamePattern());

        // minimum size value skip -1's
        if (property.getMinSize() >= 0) {
            field = (CCTextField)getChild(MIN_SIZE);
            field.setValue((new Long(property.getMinSize())).toString());
        }

        // minimum size units
        CCDropDownMenu dropDown = (CCDropDownMenu)getChild(MIN_SIZE_UNITS);
        dropDown.setValue((new Integer(property.getMinSizeUnit())).toString());

        // maximum size value skip -1's
        if (property.getMaxSize() >= 0) {
            field = (CCTextField)getChild(MAX_SIZE);
            field.setValue((new Long(property.getMaxSize())).toString());
        }

        // maximum size units
        dropDown = (CCDropDownMenu)getChild(MAX_SIZE_UNITS);
        dropDown.setValue((new Integer(property.getMaxSizeUnit())).toString());

        if (property.getAccessAge() > 0) {
            field = (CCTextField)getChild(ACCESS_AGE);
            field.setValue((new Long(property.getAccessAge())).toString());
        }

        dropDown = (CCDropDownMenu)getChild(ACCESS_AGE_UNITS);
        dropDown.setValue(Integer.toString(property.getAccessAgeUnit()));

        // owner
        field = (CCTextField)getChild(OWNER);
        field.setValue(property.getOwner());

        // group
        field = (CCTextField)getChild(GROUP);
        field.setValue(property.getGroup());

        // releasing & staging only make sense when dealing w/ custom policies
        if (thePolicy.getPolicyType() == ArSet.AR_SET_TYPE_GENERAL) {
            CriteriaDetailsView view =
                (CriteriaDetailsView)getChild(DETAILS_VIEW);
            StageAttributesView stageView =
                (StageAttributesView) view.getChild(view.STAGE_ATTR_VIEW);
            // staging
            stageView.populateStageAttributes(property);

            // releasing
            ReleaseAttributesView releaseView =
                (ReleaseAttributesView) view.getChild(view.RELEASE_ATTR_VIEW);
            releaseView.populateReleaseAttributes(property);
        }
    }

    private List savePolicyMatchCriteria(ArchivePolCriteria criteria) {
        // track all the errors in this section
        List errors = new ArrayList();

        ArchivePolCriteriaProp property =
            criteria.getArchivePolCriteriaProperties();

        String serverName = getServerName();

        // validate starting directory
        String sd = getDisplayFieldStringValue(STARTING_DIR);
        sd = sd != null ? sd.trim() : "";
        CCLabel label = (CCLabel)getChild(STARTING_DIR.concat("Label"));

        if (sd.equals("")) {
            errors.add(SamUtil.getResourceString(
                "NewArchivePolWizard.page1.errMsg3"));
            label.setShowError(true);
        } else if (sd.indexOf(' ') != -1) {
            errors.add(SamUtil.getResourceString(
                "NewArchivePolWizard.page1.errMsg7"));
            label.setShowError(true);
        } else if (sd.startsWith("/")) {
            errors.add(SamUtil.getResourceString(
                "NewArchivePolWizard.page1.errMsg10"));
            label.setShowError(true);
        } else {
            // starting is valid, save it
            property.setStartingDir(sd);
        }

        // validate min size
        String minSize = getDisplayFieldStringValue(MIN_SIZE);
        String minSizeUnit = getDisplayFieldStringValue(MIN_SIZE_UNITS);
        label = (CCLabel)getChild(MIN_SIZE.concat("Label"));

        minSize = minSize != null ? minSize.trim() : "";
        boolean minEmpty = false, minValid = false;
        long min = -1;
        int minu = -1;

        if (!minSize.equals("")) {
            try {
                min = Long.parseLong(minSize);
                if (min <= 0) {
                    errors.add(SamUtil.getResourceString(
                        "archiving.error.minsize"));
                    label.setShowError(true);
                } else {
                    minu = Integer.parseInt(minSizeUnit);
                    if (PolicyUtil.isOverFlow(min, minu)) {
                        errors.add(SamUtil.getResourceString(
                            "archiving.error.minsize"));
                        label.setShowError(true);
                    } else {
                        minValid = true;
                    }
                }
            } catch (NumberFormatException nfe) {
                errors.add(SamUtil.getResourceString(
                    "archiving.error.minsize"));
                label.setShowError(true);
            }
        } else {
            minEmpty = true;
        }

        // validate max size
        String maxSize = getDisplayFieldStringValue(MAX_SIZE);
        String maxSizeUnit = getDisplayFieldStringValue(MAX_SIZE_UNITS);
        label = (CCLabel)getChild(MAX_SIZE.concat("Label"));

        maxSize = maxSize != null ? maxSize.trim() : "";
        boolean maxEmpty = false, maxValid = false;
        long max = -1;
        int maxu = -1;

        if (!maxSize.equals("")) {
            try {
                max = Long.parseLong(maxSize);
                if (max <= 0) {
                    errors.add(SamUtil.getResourceString(
                        "archiving.error.maxsize"));
                    label.setShowError(true);
                } else {
                    maxu = Integer.parseInt(maxSizeUnit);
                    if (PolicyUtil.isOverFlow(max, maxu)) {
                        errors.add(SamUtil.getResourceString(
                            "archiving.error.maxsize"));
                        label.setShowError(true);
                    } else {
                        maxValid = true;
                    }
                }
            } catch (NumberFormatException nfe) {
                errors.add(SamUtil.getResourceString(
                    "archiving.error.maxsize"));
                label.setShowError(true);
            }
        } else {
            maxEmpty = true;
        }

        // if both min and max are present, check that max is greater than min
        boolean bothValid = false;
        if (minValid && maxValid) {
            bothValid = PolicyUtil.isMaxGreaterThanMin(min, minu, max, maxu);
            if (!bothValid) {
                errors.add(SamUtil.getResourceString(
                    "NewArchivePolWizard.page2.errMsgMinMaxSize"));
                label.setShowError(true);
            }
        }

        // finally set the values
        if (minEmpty && maxValid) {
            property.setMinSize(-1);
            property.setMinSizeUnit(-1);
            property.setMaxSize(max);
            property.setMaxSizeUnit(maxu);
        } else if (minValid && maxEmpty) {
            property.setMinSize(min);
            property.setMinSizeUnit(minu);
            property.setMaxSize(-1);
            property.setMaxSizeUnit(-1);
        } else if (minEmpty && maxEmpty) {
            property.setMinSize(-1);
            property.setMinSizeUnit(-1);
            property.setMaxSize(-1);
            property.setMaxSizeUnit(-1);
        } else if (minValid && maxValid) {
            if (bothValid) {
                property.setMinSize(min);
                property.setMinSizeUnit(minu);
                property.setMaxSize(max);
                property.setMaxSizeUnit(maxu);
            }
        }

        boolean valid = false;
        // validate name pattern
        String namePattern = getDisplayFieldStringValue(NAME_PATTERN);
        namePattern = namePattern != null ? namePattern.trim() : "";
        label = (CCLabel)getChild(NAME_PATTERN.concat("Label"));

        if (!namePattern.equals("")) {
            valid = true;
            if (namePattern.indexOf(' ') != -1) {
                errors.add(SamUtil.getResourceString(
                    "NewArchivePolWizard.page2.errNamePattern"));
                valid = false;
                label.setShowError(true);
            }

            if (!PolicyUtil.isValidNamePattern(namePattern)) {
                errors.add(SamUtil.getResourceString(
                    "NewArchivePolWizard.page2.errNamePattern"));
                valid = false;
                label.setShowError(true);
            }

            if (valid) {
                property.setNamePattern(namePattern);
            }
        } else {
            property.setNamePattern(namePattern);
        }

        // validate owner
        String owner = getDisplayFieldStringValue(OWNER);
        owner = owner != null ? owner.trim() : "";
        label = (CCLabel)getChild(OWNER.concat("Label"));

        if (!owner.equals("")) {
            if (owner.indexOf(' ') != -1 ||
                !PolicyUtil.isUserValid(owner, serverName)) {
                errors.add(SamUtil.getResourceString(
                    "NewArchivePolWizard.page2.errOwner"));
                label.setShowError(true);
            } else {
                property.setOwner(owner);
            }
        } else {
            property.setOwner(owner);
        }

        // validate group
        String group = getDisplayFieldStringValue(GROUP);
        group = group != null ? group.trim() : "";
        label = (CCLabel)getChild(GROUP.concat("Label"));

        if (!group.equals("")) {
            valid = true;
            if (group.indexOf(' ') != -1) {
                errors.add(SamUtil.getResourceString(
                    "NewArchivePolWizard.page2.errGroup"));
                valid = false;
                label.setShowError(true);
            }
            if (!PolicyUtil.isGroupValid(group, serverName)) {
                errors.add(SamUtil.getResourceString(
                    "NewArchivePolWizard.page2.errGroupExist"));
                valid = false;
                label.setShowError(true);
            }

            if (valid) {
                property.setGroup(group);
            }
        } else {
            property.setGroup(group);
        }

        // access age
        String ageString = getDisplayFieldStringValue(ACCESS_AGE);
        ageString = ageString == null ? "" : ageString.trim();

        if (!ageString.equals("")) {
            label = (CCLabel)getChild(ACCESS_AGE.concat("Label"));

            try {
                long age = Long.parseLong(ageString);
                int ageUnit = Integer.parseInt(
                    getDisplayFieldStringValue(ACCESS_AGE_UNITS));

                if (PolicyUtil.isValidTime(age, ageUnit)) {
                    property.setAccessAge(age);
                    property.setAccessAgeUnit(ageUnit);
                } else {
                    errors.add(
                       SamUtil.getResourceString("archiving.accessage.error"));
                    label.setShowError(true);
                }
            } catch (NumberFormatException nfe) {
                errors.add(
                    SamUtil.getResourceString("archiving.accessage.error"));
                label.setShowError(true);
            }
        } else {
            property.setAccessAge(-1);
            property.setAccessAgeUnit(-1);
        }

        // validate staging and releasing only policy != no_archive
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        if (!policyName.equals(ArchivePolicy.POLICY_NAME_NOARCHIVE)) {
            CriteriaDetailsView view =
                (CriteriaDetailsView) getChild(DETAILS_VIEW);
            ReleaseAttributesView releaseView =
                (ReleaseAttributesView) view.getChild(view.RELEASE_ATTR_VIEW);
            StageAttributesView stageView =
                (StageAttributesView) view.getChild(view.STAGE_ATTR_VIEW);
            String errorMsg = releaseView.saveReleaseSettings(property);
            if (errorMsg != null) {
                errors.add(errorMsg);
                label = (CCLabel) releaseView.getChild(releaseView.LABEL);
                label.setShowError(true);
            }
            errorMsg = stageView.saveStageSettings(property);
            if (errorMsg != null) {
                errors.add(errorMsg);
            }
        }

        return errors;
    }

    private List saveCopySettings(ArchivePolCriteria criteria) {
        // retrieve the table from the view
        CriteriaDetailsView theView =
            (CriteriaDetailsView)getChild(DETAILS_VIEW);

        return theView.saveCopySettings(criteria);
    }

    private void populateTableModels() throws SamFSException {
        CriteriaDetailsView view = (CriteriaDetailsView)getChild(DETAILS_VIEW);
        view.populateTableModels();
    }
    /**
     * retrieve the ArchivePolicy
     */
    public ArchivePolicy getCurrentPolicy() throws SamFSException {
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        ArchivePolicy thePolicy = sysModel.
            getSamQFSSystemArchiveManager().getArchivePolicy(policyName);

        if (thePolicy == null)
            throw new SamFSException(null, -2000);

        return thePolicy;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        initializeDropDownMenus();

        // the servername
        String serverName = getServerName();

        // initialize the criteria parameters
        try {
            ArchivePolicy thePolicy = getCurrentPolicy();

            switch (thePolicy.getPolicyType()) {
                case ArSet.AR_SET_TYPE_GENERAL:
                    loadPolicyMatchCriteria();
                    populateTableModels();
                    break;
                case ArSet.AR_SET_TYPE_NO_ARCHIVE:
                    loadPolicyMatchCriteria();
                    populateTableModels();

                    // hide the stager/releaser section
                    psModel.setVisible("StagerReleaser", false);
                    break;
                case ArSet.AR_SET_TYPE_ALLSETS_PSEUDO:
                case ArSet.AR_SET_TYPE_DEFAULT:

                // TODO: Figure out what we have to do for explicit default
                // and not assigned policies
                case ArSet.AR_SET_TYPE_UNASSIGNED:
                case ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT:

                default:
            }
        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                    this.getClass(),
                                    "loadPolicyMatchCriteria",
                                    "Unload to load the policy match criteria",
                                    serverName);
            SamUtil.setErrorAlert(
                this,
                CHILD_COMMON_ALERT,
                "-2002",
                sfe.getSAMerrno(),
                sfe.getMessage(),
                getServerName());
        }

        // set page title string
        String policyName =
            (String)getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer criteriaNumber =
            (Integer)getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        String key = "archiving.criteria.details.pagetitle";
        String [] values = {policyName, criteriaNumber.toString()};
        ptModel.setPageTitleText(SamUtil.getResourceString(key, values));

        // disable save button if no write permission
        if (!SecurityManagerFactory.getSecurityManager().
            hasAuthorization(Authorization.CONFIG)) {

            ((CCButton)getChild("Save")).setDisabled(true);
        }

        // set delete confirmation messages
        CCHiddenField field = (CCHiddenField)getChild(FS_DELETE_CONFIRMATION);
        field.setValue(
            SamUtil.getResourceString("archiving.fs.delete.confirm"));

        // save the server name for the apply criteria popup
        StringBuffer buf = new StringBuffer();
        buf.append(getServerName()).append("-_-")
           .append(policyName).append("-_-")
           .append(criteriaNumber.toString());

        field = (CCHiddenField)getChild(PS_ATTRIBUTES);
        field.setValue(buf.toString());

        TraceUtil.trace3("Exiting");
    }

    public void handleSaveRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        // effective cancel for now
        boolean valid = false;

        // get server name
        String serverName = getServerName();
        Integer criteriaNumber = (Integer)
            getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);
        int index = criteriaNumber.intValue();
        try {
            ArchivePolicy thePolicy = getCurrentPolicy();

            ArchivePolCriteria criteria =
                thePolicy.getArchivePolCriteria(index);

            List errors = new ArrayList();
            switch (thePolicy.getPolicyType()) {
                case ArSet.AR_SET_TYPE_GENERAL:
                    errors.addAll(savePolicyMatchCriteria(criteria));
                    errors.addAll(saveCopySettings(criteria));
                    break;
                case ArSet.AR_SET_TYPE_DEFAULT:
                    errors.addAll(saveCopySettings(criteria));
                    break;
                case ArSet.AR_SET_TYPE_NO_ARCHIVE:
                    errors.addAll(savePolicyMatchCriteria(criteria));
                    break;
                case ArSet.AR_SET_TYPE_ALLSETS_PSEUDO:

                // TODO: What is the behavior if policy type is explicit
                // default or unassigned???
                case ArSet.AR_SET_TYPE_EXPLICIT_DEFAULT:
                case ArSet.AR_SET_TYPE_UNASSIGNED:

                default:
            } // end switch

            // see if there were any validation errors
            if (errors.size() > 0) {
                StringBuffer buffer = new StringBuffer();
                Iterator it = errors.iterator();

                buffer.append("<ul>");
                while (it.hasNext()) {
                    String err = (String)it.next();
                    buffer.append("<li>");
                    buffer.append(err).append("<br>");
                    buffer.append("</li>");
                }
                buffer.append("</ul>");

                SamUtil.setErrorAlert(this,
                                 CHILD_COMMON_ALERT,
                                 "archiving.criteriadetails.validation.error",
                                 -2022,
                                 buffer.toString(),
                                serverName);
            } else {
                FileSystem [] fs = criteria.getFileSystemsForCriteria();
                int fssize = fs.length;
                String[] fsName = new String[fssize];
                for (int i = 0; i < fssize; i++) {
                    fsName[i] = fs[i].getName();
                }

                SamQFSSystemArchiveManager archiveManager = SamUtil.
                    getModel(serverName).getSamQFSSystemArchiveManager();

                ArrayList resultList = archiveManager.
                     isDuplicateCriteria(criteria, fsName, true);
                String duplicate = (String) resultList.get(0);
                if (duplicate.equals("true")) {
                    dupCriteriaName = (String) resultList.get(1);
                    dupPolName = (String) resultList.get(2);
                    setPageSessionAttribute("DUPLICATE", "true");
                    throw new SamFSException(null, -2025);
                } else {
                    TraceUtil.trace3("it is not a dup criteria");
                    valid = true;
                    thePolicy.updatePolicy();
                }
            }
        } catch (SamFSWarnings sfw) {
            valid = false;
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "handleSaveRequest",
                                     "Unable to save policy criteria",
                                    serverName);

            SamUtil.setWarningAlert(this,
                                    CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException sme) {
            valid = false;
            SamUtil.processException(sme,
                                     this.getClass(),
                                     "handleSaveRequest",
                                     "Unable to save policy criteria",
                                    serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error",
                                  sme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                serverName);
        } catch (SamFSException sfe) {
            valid = false;
            // set duplciate into pagesession
            if (sfe.getSAMerrno() == 30136) {
                setPageSessionAttribute("DUPLICATE", "true");
            }
            // process exception
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleSaveRequest",
                                     "Unable to save policy criteria",
                                    serverName);

            // update confirmation alert
            String [] temp1 = {Integer.toString(index)};
            String [] temp2 =
                {SamUtil.getResourceString("archiving.criterianumber", temp1)};
            if (sfe.getSAMerrno() != -2025) {
                SamUtil.setErrorAlert(this,
                                      CHILD_COMMON_ALERT,
                                      SamUtil.getResourceString(
                                      "archiving.criteria.save.failure", temp2),
                                      sfe.getSAMerrno(),
                                      sfe.getMessage(),
                                    serverName);
            } else {
                 SamUtil.setErrorAlert(this,
                                      CHILD_COMMON_ALERT,
                                      SamUtil.getResourceString(
                                      "archiving.criteria.save.failure", temp2),
                                      sfe.getSAMerrno(),
                                      SamUtil.getResourceString(
                                        "archiving.criteria.save.duplicate",
                                        new String[] {
                                            dupCriteriaName, dupPolName }),
                                                serverName);
            }
        }

        // recycle the page or foward to Policy Details page
        if (valid) {
            forwardToPreviousPage(true);
        } else {
            forwardTo(getRequestContext());
        }

        TraceUtil.trace3("Exiting");
    }

    public void handleCancelRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        // the server name
        String serverName = getServerName();
        try {
            SamUtil.getModel(serverName).
                getSamQFSSystemArchiveManager().getAllArchivePolicies();
        } catch (SamFSWarnings sfw) {
            SamUtil.processException(sfw,
                                     this.getClass(),
                                     "handleCancelRequest",
                                     "Unable to cancel policy criteria",
                                    serverName);

            SamUtil.setWarningAlert(this,
                                   CHILD_COMMON_ALERT,
                                    "ArchiveConfig.error",
                                    "ArchiveConfig.warning.detail");
        } catch (SamFSMultiMsgException sme) {
            SamUtil.processException(sme,
                                     this.getClass(),
                                     "handleCancelRequest",
                                     "Unable to cancel policy criteria",
                                    serverName);

            SamUtil.setErrorAlert(this,
                                  CHILD_COMMON_ALERT,
                                  "ArchiveConfig.error",
                                  sme.getSAMerrno(),
                                  "ArchiveConfig.error.detail",
                                  serverName);
        } catch (SamFSException sfe) {
            // process exception
            SamUtil.processException(sfe,
                                     this.getClass(),
                                     "handleCancelRequest",
                                     "Unable to cancel policy criteria",
                                    serverName);
        }

        forwardToPreviousPage(false);
        TraceUtil.trace3("Exiting");
    }

    // handler for successful save/cancel clicks
    public void forwardToPreviousPage(boolean save) {
        TraceUtil.trace3("Entering");
        // retrieve the target path
        Integer [] links = (Integer [])
            getPageSessionAttribute(Constants.SessionAttributes.PAGE_PATH);
        Integer [] paths = BreadCrumbUtil.getBreadCrumbDisplay(links);
        int index = paths[paths.length -1].intValue();

        String targetCmd =
            PageInfo.getPageInfo().getPagePath(index).getCommandField();

        // two ways we could have got to this page
        // PolicyDetails or FS-Details
        CommonViewBeanBase target = null;
        if ("PolicyDetailsHref".equals(targetCmd)) {
            target =
                (CommonViewBeanBase)getViewBean(PolicyDetailsViewBean.class);
        } else if ("FSArchivePolicyHref".equals(targetCmd)) {
            target = (CommonViewBeanBase)
                getViewBean(FSArchivePoliciesViewBean.class);
        }

        // if save was clicked, update alert message
        if (save) {
            Integer ci = (Integer)
                getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

            SamUtil.setInfoAlert(target,
                                 CommonViewBeanBase.CHILD_COMMON_ALERT,
                                 "success.summary",
                                 SamUtil.getResourceString(
                                    "archiving.criteria.save.success",
                                    new String [] {
                                    SamUtil.getResourceString(
                                        "archiving.criterianumber",
                                        new String [] {ci.toString()})}),
                                 getServerName());
        }

        // href
        String hrefValue = Integer.toString(
            BreadCrumbUtil.inPagePath(paths, index, paths.length-1));

        // back track to the right page
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), hrefValue);

        forwardTo(target);

        TraceUtil.trace3("Exiting");
    }

    // handle breadcrumb to the policy summary page
    public void handlePolicySummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(PolicySummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the policy details summary page - incase we loop
    // back here
    public void handlePolicyDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(POLICY_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(PolicyDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the criteria details summary page - incase we loop
    // back here
    public void handleCriteriaDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(CRITERIA_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(CriteriaDetailsViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the filesystem page
    public void handleFileSystemSummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSSummaryViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the filesystem deatils page
    public void handleFileSystemDetailsHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_DETAILS_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSDetailsViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    // handle breadcrumb to the fs archive policies
    public void handleFSArchivePolicyHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(FS_ARCHIVEPOL_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(FSArchivePoliciesViewBean.class);

        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }
}
