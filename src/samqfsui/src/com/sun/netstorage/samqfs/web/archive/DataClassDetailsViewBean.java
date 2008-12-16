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

// ident	$Id: DataClassDetailsViewBean.java,v 1.21 2008/12/16 00:10:54 am143972 Exp $

package com.sun.netstorage.samqfs.web.archive;

import com.iplanet.jato.model.ModelControlException;
import com.iplanet.jato.util.NonSyncStringBuffer;
import com.iplanet.jato.view.View;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;
import com.iplanet.jato.view.html.OptionList;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiMsgException;
import com.sun.netstorage.samqfs.mgmt.SamFSWarnings;
import com.sun.netstorage.samqfs.mgmt.arc.ArSet;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemArchiveManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteria;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolCriteriaProp;
import com.sun.netstorage.samqfs.web.model.archive.DataClassAttributes;
import com.sun.netstorage.samqfs.web.model.archive.PeriodicAudit;
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
import com.sun.web.ui.model.CCDateTimeModel;
import com.sun.web.ui.model.CCDateTimeModelInterface;
import com.sun.web.ui.model.CCPageTitleModel;
import com.sun.web.ui.model.CCPropertySheetModel;
import com.sun.web.ui.view.breadcrumb.CCBreadCrumbs;
import com.sun.web.ui.view.datetime.CCDateTimeWindow;
import com.sun.web.ui.view.html.CCButton;
import com.sun.web.ui.view.html.CCCheckBox;
import com.sun.web.ui.view.html.CCDropDownMenu;
import com.sun.web.ui.view.html.CCHiddenField;
import com.sun.web.ui.view.html.CCHref;
import com.sun.web.ui.view.html.CCLabel;
import com.sun.web.ui.view.html.CCRadioButton;
import com.sun.web.ui.view.html.CCTextField;
import java.io.IOException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import javax.servlet.ServletException;

public class DataClassDetailsViewBean extends CommonViewBeanBase {
    private static final String PAGE_NAME = "DataClassDetails";
    private static final String DEFAULT_URL =
        "/jsp/archive/DataClassDetails.jsp";

    // breadcrumbing children
    public static final String BREADCRUMB = "BreadCrumb";

    public static final String DATACLASS_SUMMARY_HREF = "DataClassSummaryHref";

    // children
    private static final String PAGE_TITLE = "PageTitle";

    // apply fs popup helper fields
    public static final String PS_ATTRIBUTES = "psAttributes";
    public static final String FS_LIST = "fsList";
    public static final String FS_NAME = "fsname";
    private static final String DUMP_PATH = "dumpPath";
    private static final String APPLYFS_HREF = "ApplyCriteriaHref";

    // field to keep track of is fs is removeable or not
    public static final String FS_DELETABLE = "fsDeletable";
    public static final String FS_DELETE_CONFIRMATION = "fsDeleteConfirmation";

    // field to keep track of date/time value (Calendar Pop Up is not supported
    // by the Property Sheet Tag
    public static final String AFTER_DATE_HIDDEN_FIELD = "afterDateHiddenField";

    public static final String SERVER_NAME = "ServerName";

    // property sheet children
    public static final String STARTING_DIR = "StartingDir";
    public static final String NAME_PATTERN = "NamePattern";
    public static final String NAME_PATTERN_DROP_DOWN = "NamePatternDropDown";
    public static final String MIN_SIZE = "MinimumSize";
    public static final String MIN_SIZE_UNITS = "MinimumSizeUnits";
    public static final String MAX_SIZE = "MaximumSize";
    public static final String MAX_SIZE_UNITS = "MaximumSizeUnits";
    public static final String OWNER = "Owner";
    public static final String GROUP = "Group";
    public static final String ACCESS_AGE = "AccessAge";
    public static final String ACCESS_AGE_UNITS = "AccessAgeUnits";
    public static final String DESCRIPTION = "classDescription";

    // Date Time Component
    public static final String AFTER_DATE = "IncludeDate";
    private CCDateTimeModel dateTimeModel = null;

    // class attributes
    public static final String EXPIRATION_TIME = "absolute_expiration_time";
    public static final String EXPT_TYPE = "expirationTimeType";
    public static final String DURATION = "relative_expiration_time";
    public static final String DURATION_UNIT = "relative_expiration_time_unit";

    public static final String PERIODIC_AUDIT = "periodicaudit";
    public static final String AUDIT_PERIOD = "auditperiod";
    public static final String AUDIT_PERIOD_UNIT = "auditperiodunit";
    public static final String AUTO_WORM = "autoworm";
    public static final String AUTO_DELETE = "autodelete";
    public static final String DEDUP = "dedup";
    public static final String BITBYBIT = "bitbybit";

    // logging
    public static final String LOG_AUDIT = "log_data_audit";
    public static final String LOG_DEDUP = "log_deduplication";
    public static final String LOG_AUTOWORM = "log_autoworm";
    public static final String LOG_AUTODELETION = "log_autodeletion";

    // table models
    private Map models = null;
    private CCPropertySheetModel psModel = null;
    private CCPageTitleModel ptModel = null;
    private CCActionTableModel model = null;

    private String dupPolName = null, dupCriteriaName = null;

    public DataClassDetailsViewBean() {
        super(PAGE_NAME, DEFAULT_URL);

        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        // initializeTableModels();
        psModel = PropertySheetUtil.createModel(
            "/jsp/archive/DataClassDetailsPropertySheet.xml");
        ptModel = PageTitleUtil.createModel(
            "/jsp/archive/DataClassDetailsPageTitle.xml");
        registerChildren();
        TraceUtil.trace3("Exiting");
    }

    public void registerChildren() {
        super.registerChildren();
        registerChild(BREADCRUMB, CCBreadCrumbs.class);
        registerChild(AFTER_DATE_HIDDEN_FIELD, CCHiddenField.class);
        registerChild(APPLYFS_HREF, CCHref.class);
        registerChild(DATACLASS_SUMMARY_HREF, CCHref.class);
        registerChild(FS_NAME, CCHiddenField.class);
        registerChild(DUMP_PATH, CCHiddenField.class);
        registerChild(FS_DELETABLE, CCHiddenField.class);
        registerChild(PS_ATTRIBUTES, CCHiddenField.class);
        registerChild(FS_LIST, CCHiddenField.class);
        registerChild(FS_DELETE_CONFIRMATION, CCHiddenField.class);
        registerChild(SERVER_NAME, CCHiddenField.class);
        registerChild(AFTER_DATE, CCDateTimeWindow.class);
        registerChild(EXPIRATION_TIME, CCDateTimeWindow.class);
        registerChild(EXPT_TYPE, CCRadioButton.class);
        PropertySheetUtil.registerChildren(this, psModel);
        // registerChild(DETAILS_VIEW, DataClassDetailsView.class);
        PageTitleUtil.registerChildren(this, ptModel);
    }

    public View createChild(String name) {
        if (name.equals(DESCRIPTION)) {
            return new CCLabel(this, name, null);
        } else if (name.equals(BREADCRUMB)) {
            CCBreadCrumbsModel bcModel =
                new CCBreadCrumbsModel(
                    "archiving.dataclass.details.headertitle");
            BreadCrumbUtil.createBreadCrumbs(this, name, bcModel);
            return new CCBreadCrumbs(this, bcModel, name);
        } else if (name.equals(APPLYFS_HREF) ||
                   name.equals(DATACLASS_SUMMARY_HREF)) {
            return new CCHref(this, name, null);
        } else if (name.equals(FS_NAME) ||
                   name.equals(DUMP_PATH) ||
                   name.equals(FS_DELETABLE) ||
                   name.equals(FS_DELETE_CONFIRMATION) ||
                   name.equals(PS_ATTRIBUTES) ||
                   name.equals(AFTER_DATE_HIDDEN_FIELD) ||
                   name.equals(FS_LIST)) {
            return new CCHiddenField(this, name, null);
        } else if (name.equals(SERVER_NAME)) {
            return new CCHiddenField(this, name, getServerName());
        } else if (PageTitleUtil.isChildSupported(ptModel, name)) {
            return PageTitleUtil.createChild(this, ptModel, name);
        } else if (name.equals(EXPT_TYPE)) {
            return new CCRadioButton(this, name, null);
        } else if (name.equals(EXPIRATION_TIME)) {
            CCDateTimeModel m = new CCDateTimeModel();
            m.setType(CCDateTimeModel.FOR_DATE_SELECTION);
            m.setStartDateLabel(SamUtil.getResourceString(
                "archiving.dataclass.expirationtime"));
            return new CCDateTimeWindow(this, m, name);
        } else if (name.equals(AFTER_DATE)) {
            CCDateTimeModel m = new CCDateTimeModel();
            m.setType(CCDateTimeModel.FOR_DATE_SELECTION);
            m.setStartDateLabel(SamUtil.getResourceString(
                "archiving.dataclass.date.startlabel"));
            CCDateTimeWindow child = new CCDateTimeWindow(this, m, name);
            return child;
        } else if (super.isChildSupported(name)) {
            return super.createChild(name);
        } else if (PropertySheetUtil.isChildSupported(psModel, name)) {
            return PropertySheetUtil.createChild(this, psModel, name);
        } else {
            throw new IllegalArgumentException("invalid child '" + name + "'");
        }
    }

    private void initializeDropDownMenus() {
        // Name Pattern drop down
        CCDropDownMenu dropDown =
            (CCDropDownMenu)getChild(NAME_PATTERN_DROP_DOWN);
        dropDown.setOptions(new OptionList(
            SelectableGroupHelper.namePattern.labels,
            SelectableGroupHelper.namePattern.values));

        // minimum size drop down
        dropDown = (CCDropDownMenu)getChild(MIN_SIZE_UNITS);
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
    private void loadFileMatchCriteria(ArchivePolCriteria criteria)
        throws SamFSException {
        // retrieve the model first
        SamQFSSystemModel sysModel = SamUtil.getModel(getServerName());

        // check duplicate
        String duplicate = (String) getPageSessionAttribute("DUPLICATE");
        if (duplicate != null && duplicate.equals("true")) {
            // resync the policy object model
            sysModel.getSamQFSSystemArchiveManager().getAllArchivePolicies();
        }

        ArchivePolCriteriaProp property =
            criteria.getArchivePolCriteriaProperties();

        // set the description
        ((CCLabel)getChild(DESCRIPTION)).setValue(property.getDescription());

        // starting dir
        CCTextField field = (CCTextField)getChild(STARTING_DIR);
        field.setValue(property.getStartingDir());

        // name pattern
        field = (CCTextField)getChild(NAME_PATTERN);
        field.setValue(property.getNamePattern());

        CCDropDownMenu menu = (CCDropDownMenu)getChild(NAME_PATTERN_DROP_DOWN);
        menu.setValue(Integer.toString(property.getNamePatternType()));

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

        // After Date
        String afterDate = property.getAfterDate();
        afterDate = afterDate == null ? "" : afterDate;

        CCDateTimeWindow dateTime = (CCDateTimeWindow) getChild(AFTER_DATE);
        CCDateTimeModelInterface model = dateTime.getModel();
        model.setType(CCDateTimeModel.FOR_DATE_SELECTION);
        model.setStartDateLabel(SamUtil.getResourceString(
            "archiving.dataclass.date.startlabel"));

        if (!afterDate.equals("")) {
            Date myDate;
            try {
                myDate = new SimpleDateFormat("yyyy-MM-dd").parse(afterDate);
                String dateString = new SimpleDateFormat(
                    dateTime.getDateFormatPattern()).format(myDate);
                ((CCTextField)dateTime.getChild("textField"))
                    .setValue(dateString);
            } catch (NullPointerException nullEx) {
                // This should not happen.  Value comes back from core should be
                // valid
                TraceUtil.trace1("Exception caught while converting dates!");
            } catch (ParseException parseEx) {
                // Same
                TraceUtil.trace1("Exception caught while converting dates!");
            }
        }

    }

    private void loadDataClassAttributes(ArchivePolCriteria criteria) {
        DataClassAttributes attributes = criteria
            .getArchivePolCriteriaProperties().getDataClassAttributes();

        // auto worm
        ((CCCheckBox)getChild(AUTO_WORM)).setValue(
            attributes.isAutoWormEnabled() ? "true" : "false");

        // expiration time type
        String temp =
            attributes.isAbsoluteExpirationTime() ? "date" : "duration";
        ((CCRadioButton)getChild(EXPT_TYPE)).setValue(temp);

        // expiration time
        if (attributes.isAbsoluteExpirationTime()) { // absolute expiration
            CCDateTimeWindow dtWindow =
                (CCDateTimeWindow)getChild(EXPIRATION_TIME);

            temp = new SimpleDateFormat(dtWindow.getDateFormatPattern())
                .format(attributes.getAbsoluteExpirationTime());

            // set the date string on the calendar text field directly
            ((CCTextField)dtWindow.getChild("textField"))
                .setValue(temp);
        } else { // relative expiration
            temp = attributes.getRelativeExpirationTime() != -1 ?
                Long.toString(attributes.getRelativeExpirationTime()) : "";
            ((CCTextField)getChild(DURATION)).setValue(temp);

            temp = Integer
                .toString(attributes.getRelativeExpirationTimeUnit());
            ((CCDropDownMenu)getChild(DURATION_UNIT)).setValue(temp);
        }

        // auto delete
        ((CCCheckBox)getChild(DEDUP)).setValue(
            attributes.isAutoDeleteEnabled() ? "true" : "false");

        // dedup
        ((CCCheckBox)getChild(DEDUP)).setValue(
            attributes.isDedupEnabled() ? "true" : "false");

        // bitbybit
        ((CCCheckBox)getChild(BITBYBIT)).setValue(
            attributes.isDedupEnabled() ? "true" : "false");

        // periodic audit
        ((CCDropDownMenu)getChild(PERIODIC_AUDIT))
            .setValue(attributes.getPeriodicAudit().getStringValue());

        // temp
        temp = attributes.getAuditPeriod() != -1 ?
            Long.toString(attributes.getAuditPeriod()) : "";
        ((CCTextField)getChild(AUDIT_PERIOD)).setValue(temp);
        temp = Integer.toString(attributes.getAuditPeriodUnit());
        ((CCDropDownMenu)getChild(AUDIT_PERIOD_UNIT)).setValue(temp);

        // logging
        ((CCCheckBox)getChild(LOG_AUDIT)).setValue(
            attributes.isLogDataAuditEnabled() ? "true" : "false");
        ((CCCheckBox)getChild(LOG_DEDUP)).setValue(
            attributes.isLogDeduplicationEnabled() ? "true" : "false");
        ((CCCheckBox)getChild(LOG_AUTOWORM)).setValue(
            attributes.isLogAutoWormEnabled() ? "true" : "false");
        ((CCCheckBox)getChild(LOG_AUTODELETION)).setValue(
            attributes.isLogAutoDeletionEnabled() ? "true" : "false");
    }

    private List saveFileMatchCriteria(ArchivePolCriteria criteria) {
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

        String namePatternType =
            getDisplayFieldStringValue(NAME_PATTERN_DROP_DOWN);

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
                property.setNamePatternType(Integer.parseInt(namePatternType));
            }
        } else {
            property.setNamePattern(namePattern);
            property.setNamePatternType(Criteria.REGEXP);
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

        // After Date
        CCDateTimeWindow dateTime = (CCDateTimeWindow) getChild(AFTER_DATE);
        boolean validDate = false;

        // Must invoke validateDataInput() before retrieving model values!
        // Lockhart limitation
        if (dateTime.validateDataInput()) {
            CCDateTimeModelInterface model = dateTime.getModel();
            Date afterDate = model.getStartDateTime();
            if (afterDate != null) {
                String dateString =
                    new SimpleDateFormat("yyyy-MM-dd").format(afterDate);
                property.setAfterDate(dateString);
                validDate = true;
            }
        }

        // Reset if user enters a wrong date.
        // Don't set error message here because there is no way to figure out
        // if the user enters a wrong value, or user leaves the field blank.
        // I can't seem to find out how to validate that. >.<
        if (!validDate) {
            property.setAfterDate("");
        }

        return errors;
    }

    private List saveDataClassAttributes(ArchivePolCriteria criteria) {
        List errors = new ArrayList();
        DataClassAttributes attributes = criteria
            .getArchivePolCriteriaProperties().getDataClassAttributes();

        // autoworm
        String temp = getDisplayFieldStringValue(AUTO_WORM);
        attributes.setAutoWormEnabled(temp.equals("true"));

        // expiration time type
        if ("date".equals(getDisplayFieldStringValue(EXPT_TYPE))) { // abs exp
            CCDateTimeWindow dtWindow =
                (CCDateTimeWindow)getChild(EXPIRATION_TIME);
            if (dtWindow.validateDataInput()) {
                attributes.setAbsoluteExpirationTime(
                    dtWindow.getModel().getStartDateTime());
                attributes.setAbsoluteExpirationEnabled(true);
            } else { // invalid date entered
                errors.add(SamUtil.getResourceString(
                    "archiving.classattributes.invalid.date"));
            }
        } else { // relative expiration time
            temp = getDisplayFieldStringValue(DURATION);
            if (temp != null) {
            try {
                long duration = Long.parseLong(temp);
                int unit = Integer
                    .parseInt(getDisplayFieldStringValue(DURATION_UNIT));

                attributes.setRelativeExpirationTime(duration);
                attributes.setRelativeExpirationTimeUnit(unit);
                attributes.setAbsoluteExpirationEnabled(false);
            } catch (NumberFormatException nfe) {
                errors.add(SamUtil.getResourceString(
                    "archiving.classattributes.duration.nan"));
            }
            } else {
                errors.add(SamUtil.getResourceString(
                    "archiving.classattributes.duration.null"));
            }
        }

        // auto deletion
        attributes.setAutoDeleteEnabled(
            "true".equals(getDisplayFieldStringValue(AUTO_DELETE)));

        // dedup
        boolean dedup = "true".equals(getDisplayFieldStringValue(DEDUP));
        attributes.setDedupEnabled(dedup);
        if (dedup) {
            attributes.setBitbybitEnabled(
                "true".equals(getDisplayFieldStringValue(BITBYBIT)));
        } else {
            attributes.setBitbybitEnabled(false);
        }

        // periodic audit
        temp = getDisplayFieldStringValue(PERIODIC_AUDIT);
        PeriodicAudit periodicAudit = null;
        if (temp.equals(PeriodicAudit.NONE.getStringValue())) {
            periodicAudit = PeriodicAudit.NONE;
        } else if (temp.equals(PeriodicAudit.DISK.getStringValue())) {
            periodicAudit = PeriodicAudit.DISK;
        } else {
            periodicAudit = PeriodicAudit.ALL;
        }

        if (!periodicAudit.equals(PeriodicAudit.NONE)) { // set audit period
            temp = getDisplayFieldStringValue(AUDIT_PERIOD);
            if (temp != null && temp.length() > 0) {
            try {
                long period = Long.parseLong(temp);
                int unit = Integer
                    .parseInt(getDisplayFieldStringValue(AUDIT_PERIOD_UNIT));

                attributes.setAuditPeriod(period);
                attributes.setAuditPeriodUnit(unit);
            } catch (NumberFormatException nfe) {
                errors.add(SamUtil.getResourceString(
                    "archiving.classattributes.auditperiod.nan"));
            }

            }
        } else { // blank out audit period
            // TODO: should audit period be blanked out here?
        }
        attributes.setPeriodicAudit(periodicAudit);

        // logging
        attributes.setLogDataAuditEnabled(
            "true".equals(getDisplayFieldStringValue(LOG_AUDIT)));
        attributes.setLogDeduplicationEnabled(
            "true".equals(getDisplayFieldStringValue(LOG_DEDUP)));
        attributes.setLogAutoWormEnabled(
            "true".equals(getDisplayFieldStringValue(LOG_AUTOWORM)));
        attributes.setLogAutoDeletionEnabled(
            "true".equals(getDisplayFieldStringValue(LOG_AUTODELETION)));

        return errors;
    }

    public void beginDisplay(DisplayEvent evt) throws ModelControlException {
        TraceUtil.trace3("Entering");
        initializeDropDownMenus();

        // the servername
        String serverName = getServerName();
        String policyName =
            (String) getPageSessionAttribute(Constants.Archive.POLICY_NAME);

        // the classname
        String className = "";
        Integer criteriaNumber = (Integer)
                getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

        // initialize the criteria parameters
        try {
            ArchivePolCriteria criteria = getCurrentCriteria();
            className =
                criteria.getArchivePolCriteriaProperties().getClassName();

            loadFileMatchCriteria(criteria);

            if (criteria.getArchivePolicy().getPolicyType() !=
                ArSet.AR_SET_TYPE_NO_ARCHIVE) {
                loadDataClassAttributes(criteria);
            } else {// if no archive hide class attributes
                psModel.setVisible("classAttributes", false);
                psModel.setVisible("logging", false);
            }
            // populateTableModels();

        } catch (SamFSException sfe) {
            SamUtil.processException(sfe,
                                    this.getClass(),
                                    "loadFileMatchCriteria",
                                    "Unload to load the policy match criteria",
                                    serverName);
            SamUtil.setErrorAlert(
                getParentViewBean(),
                CommonViewBeanBase.CHILD_COMMON_ALERT,
                SamUtil.getResourceString(
                    "archiving.dataclass.details.populate.failure"),
                sfe.getSAMerrno(),
                sfe.getMessage(),
                serverName);

        }

        ptModel.setPageTitleText(
            SamUtil.getResourceString(
                "archiving.dataclass.details.pagetitle", className));

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
        NonSyncStringBuffer buf = new NonSyncStringBuffer();
        buf.append(getServerName()).append("-_-")
           .append(policyName).append("-_-")
           .append(criteriaNumber.toString());

        field = (CCHiddenField)getChild(PS_ATTRIBUTES);
        field.setValue(buf.toString());

        // dataclass attributes
        CCRadioButton rb = (CCRadioButton)getChild(EXPT_TYPE);
        rb.setOptions(new OptionList(new String [] {"Date", "Duration"},
                                     new String [] {"date", "duration"}));

        // set the expiration time units
        OptionList timeUnits = new OptionList(
            new String[] {"common.unit.time.days",
                          "common.unit.time.weeks",
                          "common.unit.time.years"},
            new String[] {Integer.toString(SamQFSSystemModel.TIME_DAY),
                          Integer.toString(SamQFSSystemModel.TIME_WEEK),
                          Integer.toString(SamQFSSystemModel.TIME_YEAR)});

        // duration units
        ((CCDropDownMenu)getChild(DURATION_UNIT)).setOptions(timeUnits);
        // audit period nits
        ((CCDropDownMenu)getChild(AUDIT_PERIOD_UNIT)).setOptions(timeUnits);

        // periodic copy auditing
        ((CCDropDownMenu)getChild(PERIODIC_AUDIT)).setOptions(
            new OptionList(new String[] {
                 PeriodicAudit.NONE.getStringKey(),
                 PeriodicAudit.DISK.getStringKey(),
                 PeriodicAudit.ALL.getStringKey()},
            new String [] {
                PeriodicAudit.NONE.getStringValue(),
                PeriodicAudit.DISK.getStringValue(),
                PeriodicAudit.ALL.getStringValue()}));

        TraceUtil.trace3("Exiting");
    }

    public void handleSaveRequest(RequestInvocationEvent rie)
        throws ServletException, IOException {
        TraceUtil.trace3("Entering");

        // effective cancel for now
        boolean valid = false;

        // get server name
        String serverName = getServerName();

        try {
            ArchivePolCriteria criteria = getCurrentCriteria();

            List errors = new ArrayList();
            errors.addAll(saveFileMatchCriteria(criteria));

            if (criteria.getArchivePolicy().getPolicyType() !=
                ArSet.AR_SET_TYPE_NO_ARCHIVE) {
                errors.addAll(saveDataClassAttributes(criteria));
            }

            // see if there were any validation errors
            if (errors.size() > 0) {
                NonSyncStringBuffer buffer = new NonSyncStringBuffer();
                Iterator it = errors.iterator();

                buffer.append("<ul>");
                while (it.hasNext()) {
                    String err = (String)it.next();
                    buffer.append("<li>");
                    buffer.append(err).append("<br>");
                    buffer.append("</li>");
                }
                buffer.append("</ul>");

                SamUtil.setErrorAlert(
                    this,
                    CHILD_COMMON_ALERT,
                    "archiving.dataclass.details.validateerror",
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

                SamQFSSystemArchiveManager archiveManager =
                    SamUtil.getModel(serverName).
                        getSamQFSSystemArchiveManager();

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
                    criteria.getArchivePolicy().updatePolicy();
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
            int index = ((Integer) getPageSessionAttribute(
                Constants.Archive.CRITERIA_NUMBER)).intValue();
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
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(DataClassSummaryViewBean.class);

        // if save was clicked, update alert message
        if (save) {
            Integer ci = (Integer)
                getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);

            SamUtil.setInfoAlert(
                target,
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

    // handle breadcrumb to the data class summary page
    public void handleDataClassSummaryHrefRequest(RequestInvocationEvent evt)
        throws ServletException, IOException {
        String s = (String)getDisplayFieldValue(DATACLASS_SUMMARY_HREF);
        CommonViewBeanBase target =
            (CommonViewBeanBase)getViewBean(DataClassSummaryViewBean.class);

        // breadcrumb
        BreadCrumbUtil.breadCrumbPathBackward(this,
            PageInfo.getPageInfo().getPageNumber(target.getName()), s);

        forwardTo(target);
    }

    private ArchivePolCriteria getCurrentCriteria() throws SamFSException {
        String policyName =
            (String) getPageSessionAttribute(Constants.Archive.POLICY_NAME);
        Integer criteriaNumber = (Integer)
                getPageSessionAttribute(Constants.Archive.CRITERIA_NUMBER);
        return
            SamUtil.getModel(getServerName()).getSamQFSSystemArchiveManager().
                getDataClassByIndex(policyName, criteriaNumber.intValue());
    }
}
