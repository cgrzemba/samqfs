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

// ident	$Id: FilterSettingsViewBean.java,v 1.10 2008/03/17 14:43:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.view.View;
import com.iplanet.jato.view.ViewBase;
import com.iplanet.jato.view.event.ChildDisplayEvent;
import com.iplanet.jato.view.event.DisplayEvent;
import com.iplanet.jato.view.event.RequestInvocationEvent;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.web.ui.taglib.header.CCHtmlHeaderTag;

import java.io.UnsupportedEncodingException;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.GregorianCalendar;

/**
 *  This class is the view bean for the Preferences page
 */

public class FilterSettingsViewBean extends CommonPopupBase {

    // Page information...
    private static final String PAGE_NAME = "FilterSettings";
    private static final String DEFAULT_DISPLAY_URL_ROOT =
        "/jsp/util/FilterSettings.jsp";

    public static final String CHILD_HEADER = "pageHeader";
    public static final String CHILD_FILE_NAME_PATTERN = "fileNamePatternValue";
    public static final String CHILD_FILE_SIZE_GREATER_VALUE =
        "fileSizeGreaterValue";
    public static final String CHILD_FILE_SIZE_LESS_VALUE =
        "fileSizeLessValue";
    public static final String CHILD_FILE_SIZE_GREATER_UNIT =
        "fileSizeGreaterUnit";
    public static final String CHILD_FILE_SIZE_LESS_UNIT =
        "fileSizeLessUnit";
    public static final String CHILD_FILE_DATE_TYPE = "fileDateType";
    public static final String CHILD_FILE_DATE_RANGE = "fileDateRange";
    public static final String CHILD_FILE_DATE_NUM = "fileDateNum";
    public static final String CHILD_FILE_DATE_UNIT = "fileDateUnit";
    public static final String CHILD_FILE_DATE_START = "fileDateStart";
    public static final String CHILD_FILE_DATE_END = "fileDateEnd";
    public static final String CHILD_OWNER = "ownerValue";
    public static final String CHILD_GROUP = "groupValue";
    public static final String CHILD_ISDAMAGED = "isDamagedValue";
    public static final String CHILD_ISDAMAGED_PROP = "isDamaged";
    public static final String CHILD_ISONLINE = "isOnlineValue";
    public static final String CHILD_ISONLINE_PROP = "isOnline";
    public static final String CHILD_RETURN_VALUE = "returnValue";
    public static final String CHILD_SUBMIT_NOW = "submitNow";

    // Drop down menu option values, not strings to be localized
    public static final String FILE_SIZE_UNIT_KB = "kb";
    public static final String FILE_SIZE_UNIT_MB = "mb";
    public static final String FILE_SIZE_UNIT_GB = "gb";
    public static final String FILE_DATE_TYPE_BLANK = "blank";
    public static final String FILE_DATE_TYPE_CREATED = "created";
    public static final String FILE_DATE_TYPE_MODIFIED = "modified";
    public static final String FILE_DATE_TYPE_ACCESSED = "lastAccessed";
    public static final String FILE_DATE_RANGE_INTHELAST = "inTheLast";
    public static final String FILE_DATE_RANGE_BETWEEN = "between";
    public static final String FILE_DATE_UNIT_DAYS = "days";
    public static final String FILE_DATE_UNIT_MONTHS = "months";
    public static final String BOOLEAN_YES = "yes";
    public static final String BOOLEAN_NO = "no";

    // Page session attribute used when refreshing the popup as after
    // failed data validation
    public static final String FILTER_OBJECT = "filterObject";

    private Filter filter;
    private FsmVersion version45;

    /**
     * Constructor
     */
    public FilterSettingsViewBean() {
        super(PAGE_NAME, DEFAULT_DISPLAY_URL_ROOT);
        TraceUtil.initTrace();
        TraceUtil.trace3("Entering");

        TraceUtil.trace3("Exiting");
    }

    public String getPageTitleXmlFile() {
        return "/jsp/util/FilterSettingsPageTitle.xml";
    }

    public String getPropSheetXmlFile() {
        return "/jsp/util/FilterSettingsPropSheet.xml";
    }

    protected View createChild(String name) {
        TraceUtil.trace3("Entering");
        View child = null;
        if (name.equals(CHILD_HEADER)) {
            // There is no view for the header.  Just return a base view so
            // the base impl does not throw an exception.
            child = new ViewBase();
        } else {
            child = super.createChild(name);
        }

        return child;
    }

    /**
     * Called as notification that the JSP has begun its display processing
     * @param event The DisplayEvent
     */
    public void beginDisplay(DisplayEvent event) {
        TraceUtil.trace3("Entering");
        super.beginDisplay(event);
        // build URL for refresh, including URL parameters and set as default.

        loadFilterSettings();

        TraceUtil.trace3("Exiting");
    }

    public boolean beginChildDisplay(ChildDisplayEvent event) {
        super.endDisplay(event);

        // Set the browser title bar to be the same as the page title.
        if (event.getChildName().equals(CHILD_HEADER)) {
            CCHtmlHeaderTag tag = (CCHtmlHeaderTag) event.getSource();
            tag.setPageTitle((String)getPageSessionAttribute(PAGE_TITLE_TEXT));
        }

        return true;
    }

    public void loadFilterSettings() {
        // Check to see if there is a filter object passed by the popup
        // itself on refresh
        this.filter = (Filter) getPageSessionAttribute(FILTER_OBJECT);

        if (this.filter == null) {
            // Check for filter object passed in by parent.
            String filterStr = this.getLoadValue();
            if (filterStr != null && filterStr.length() > 0) {
                try {
                    this.filter = Filter.getFilterFromEncodedString(filterStr);
                    setPageSessionAttribute(FILTER_OBJECT, filter);
                } catch (UnsupportedEncodingException e) {
                    TraceUtil.trace1(
                        "Unable to build filter object from string.", e);
                }
            }
        }
        if (this.filter == null) {
            // Caller should send in at least an empty filter object with the
            // version info.  But they didn't, so default to most recent.
            this.filter = new Filter(Filter.VERSION_LATEST);
            setPageSessionAttribute(FILTER_OBJECT, filter);
        }

        // Name
        setDisplayFieldValue(CHILD_FILE_NAME_PATTERN, filter.getNamePattern());

        // Size greater
        Filter.SizeCriteria size = filter.getSizeGreater();
        if (size != null) {
            setDisplayFieldValue(CHILD_FILE_SIZE_GREATER_VALUE,
                String.valueOf(size.getSize()));
            if (size.getSizeUnit() == SamQFSSystemModel.SIZE_MB) {
                setDisplayFieldValue(CHILD_FILE_SIZE_GREATER_UNIT,
                    FILE_SIZE_UNIT_MB);
            } else if (size.getSizeUnit() == SamQFSSystemModel.SIZE_GB) {
                setDisplayFieldValue(CHILD_FILE_SIZE_GREATER_UNIT,
                    FILE_SIZE_UNIT_GB);
            } else {
                setDisplayFieldValue(CHILD_FILE_SIZE_GREATER_UNIT,
                    FILE_SIZE_UNIT_KB);
            }
        }

        // Size less
        size = filter.getSizeLess();
        if (size != null) {
            setDisplayFieldValue(CHILD_FILE_SIZE_LESS_VALUE,
                String.valueOf(size.getSize()));
            if (size.getSizeUnit() == SamQFSSystemModel.SIZE_MB) {
                setDisplayFieldValue(CHILD_FILE_SIZE_LESS_UNIT,
                    FILE_SIZE_UNIT_MB);
            } else if (size.getSizeUnit() == SamQFSSystemModel.SIZE_GB) {
                setDisplayFieldValue(CHILD_FILE_SIZE_LESS_UNIT,
                    FILE_SIZE_UNIT_GB);
            } else {
                setDisplayFieldValue(CHILD_FILE_SIZE_LESS_UNIT,
                    FILE_SIZE_UNIT_KB);
            }
        }

        // File Date
        Filter.DateCriteria date;
        if ((date = filter.getDateCreated()) != null) {
            setDisplayFieldValue(CHILD_FILE_DATE_TYPE, FILE_DATE_TYPE_CREATED);
        } else if ((date = filter.getDateModified()) != null) {
            setDisplayFieldValue(CHILD_FILE_DATE_TYPE, FILE_DATE_TYPE_MODIFIED);
        } else if ((date = filter.getDateAccessed()) != null) {
            setDisplayFieldValue(CHILD_FILE_DATE_TYPE, FILE_DATE_TYPE_ACCESSED);
        }
        if (date != null) {
            // Date Range
            if (date.getDateRangeType() ==
                Filter.DateCriteria.DATERANGETYPE_INTHELAST) {
                setDisplayFieldValue(CHILD_FILE_DATE_RANGE,
                    FILE_DATE_RANGE_INTHELAST);
                setDisplayFieldValue(CHILD_FILE_DATE_NUM,
                    String.valueOf(date.getDateValue()));
                if (date.getDateUnit() == Filter.DateCriteria.DATEUNIT_DAYS) {
                    setDisplayFieldValue(CHILD_FILE_DATE_UNIT,
                        FILE_DATE_UNIT_DAYS);
                } else {
                    setDisplayFieldValue(CHILD_FILE_DATE_UNIT,
                        FILE_DATE_UNIT_MONTHS);
                }
            } else {
                setDisplayFieldValue(CHILD_FILE_DATE_RANGE,
                    FILE_DATE_RANGE_BETWEEN);
                String datePattern = SamUtil.getResourceString(
                    "FilterSettings.fileDatePattern");
                SimpleDateFormat sdf = new SimpleDateFormat(datePattern);
                // Be careful, these dates may not be set if the user made an
                // error on a previous display.
                String startDate = "";
                if (date.getAfterDate() != null) {
                    startDate = sdf.format(date.getAfterDate().getTime());
                }
                String endDate = "";
                if (date.getBeforeDate() != null) {
                    endDate = sdf.format(date.getBeforeDate().getTime());
                }
                setDisplayFieldValue(CHILD_FILE_DATE_START, startDate);
                setDisplayFieldValue(CHILD_FILE_DATE_END, endDate);
            }
        } // date

        // Owner & group
        if (filter.getOwner() != null) {
            setDisplayFieldValue(CHILD_OWNER, filter.getOwner());
        }
        if (filter.getGroup() != null) {
            setDisplayFieldValue(CHILD_GROUP, filter.getGroup());
        }

        // isDamaged & isOnline
        if (filter.getVersion() >= Filter.VERSION_2) {
            if (filter.getIsDamaged() != null) {
                setDisplayFieldValue(CHILD_ISDAMAGED,
                    filter.getIsDamaged().booleanValue()
                    ? BOOLEAN_YES
                    : BOOLEAN_NO);
            }
            if (filter.getIsOnline() != null) {
                setDisplayFieldValue(CHILD_ISONLINE,
                    filter.getIsOnline().booleanValue()
                    ? BOOLEAN_YES
                    : BOOLEAN_NO);
            }
        } else {
            propertySheetModel.setVisible(CHILD_ISDAMAGED_PROP, false);
            propertySheetModel.setVisible(CHILD_ISONLINE_PROP, false);
        }
    }

    public void handleOKButtonRequest(RequestInvocationEvent event) {
        TraceUtil.trace3("Entering");

        try {
            storeFilterSettings();
        } catch (SamFSException ex) {
            SamUtil.processException(
                ex,
                this.getClass(),
                "handleOKButtonRequest()",
                "Failed to save the filter settings.Most likely a user error.",
                "");
            SamUtil.setErrorAlert(
                this,
                ALERT,
                "FilterSettings.error.cantSave",
                ex.getSAMerrno(),
                ex.getMessage(),
                "");
        }


        this.forwardTo();

        TraceUtil.trace3("Exiting");
    }

    public boolean storeFilterSettings() throws SamFSException {

        // Get filter object to determine version
        Filter filter = (Filter) getPageSessionAttribute(FILTER_OBJECT);
        int version = Filter.VERSION_LATEST;
        // Not having an object in page session is likely an error condition.
        if (filter != null) {
            version = filter.getVersion();
            // Got the version, next nuke the local filter object.
        }
        filter = new Filter(version);

        // In general, as errors, occur, add to error string, and keep going.
        // We want to save other good settings, even if some settings are bad.
        StringBuffer errors = new StringBuffer();

        // Name pattern
        String namePattern =
            getDisplayFieldStringValue(CHILD_FILE_NAME_PATTERN);
        if (namePattern == null || namePattern.length() == 0) {
            filter.filterOnNamePattern(false, null);
        } else {
            filter.filterOnNamePattern(true, namePattern);
        }

        // Sizes
        for (int i = 0; i < 2; i++) {
            String value = "";
            String unitFieldName = "";
            Filter.SizeCriteria size = null;
            switch (i) {
                case 0:
                    value = getDisplayFieldStringValue(
                        CHILD_FILE_SIZE_GREATER_VALUE);
                    if (value == null || value.length() == 0) {
                        filter.filterOnSizeGreater(false);
                        continue;
                    } else {
                        filter.filterOnSizeGreater(true);
                    }
                    size = filter.getSizeGreater();
                    unitFieldName = CHILD_FILE_SIZE_GREATER_UNIT;
                    break;

                case 1:
                    value = getDisplayFieldStringValue(
                        CHILD_FILE_SIZE_LESS_VALUE);
                    if (value == null || value.length() == 0) {
                        filter.filterOnSizeLess(false);
                        continue;
                    } else {
                        filter.filterOnSizeLess(true);
                    }
                    size = filter.getSizeLess();
                    unitFieldName = CHILD_FILE_SIZE_LESS_UNIT;
                    break;

                default:
                    // Developer error
                    break;
            }

            long sizeLong = 0L;
            try {
                sizeLong = Long.parseLong(value);
                if (sizeLong < 0) {
                    addError("FilterSettings.error.badSize", errors);
                }
            } catch (NumberFormatException e) {
                addError("FilterSettings.error.badSize", errors);
            }
            size.setSize(sizeLong);

            value = getDisplayFieldStringValue(unitFieldName);
            if (value.equals(FILE_SIZE_UNIT_MB)) {
                size.setSizeUnit(SamQFSSystemModel.SIZE_MB);
            } else if (value.equals(FILE_SIZE_UNIT_GB)) {
                size.setSizeUnit(SamQFSSystemModel.SIZE_GB);
            } else {
                // Default is KB
                size.setSizeUnit(SamQFSSystemModel.SIZE_KB);
            }
        } // for

        // Validate greater than size relative to less than size.
        if (filter.getSizeGreater() != null && filter.getSizeLess() != null) {
            Capacity capGreater = new Capacity(
                filter.getSizeGreater().getSize(),
                filter.getSizeGreater().getSizeUnit());
            Capacity capLess = new Capacity(
                filter.getSizeLess().getSize(),
                filter.getSizeLess().getSizeUnit());

            // The "greater than" number cannot be greater than
            // the "less than" number.  e.g. NOT x > 5 && x < 4
            if (capGreater.compareTo(capLess) > 0) {
                // Can't do this
                addError("FilterSettings.error.badSizeCombo", errors);
            }
        }

        // File date type
        // To determine if the date criteria is to be used, look at
        // the "num" value for "in the last" dates or the date values
        // for "between" dates
        String temp   = getDisplayFieldStringValue(CHILD_FILE_DATE_TYPE);
        Filter.DateCriteria date = null;
        if (temp.equals(FILE_DATE_TYPE_CREATED)) {
            filter.filterOnDateCreated(true);
            date = filter.getDateCreated();
        } else if (temp.equals(FILE_DATE_TYPE_MODIFIED)) {
            filter.filterOnDateModified(true);
            date = filter.getDateModified();
        } else if (temp.equals(FILE_DATE_TYPE_ACCESSED)) {
            filter.filterOnDateAccessed(true);
            date = filter.getDateAccessed();
        }

        if (date != null) {
            // Get Range type
            temp = getDisplayFieldStringValue(CHILD_FILE_DATE_RANGE);
            if (temp.equals(FILE_DATE_RANGE_INTHELAST)) {
                int fileDateNum;
                try {
                    fileDateNum = Integer.parseInt(
                        getDisplayFieldStringValue(CHILD_FILE_DATE_NUM));
                    if (fileDateNum < 0) {
                        addError("FilterSettings.error.badDateNum", errors);
                    }
                } catch (NumberFormatException e) {
                    addError("FilterSettings.error.badDateNum", errors);
                    fileDateNum = 0;
                }
                temp = getDisplayFieldStringValue(
                    CHILD_FILE_DATE_UNIT);
                if (temp.equals(FILE_DATE_UNIT_DAYS)) {
                    date.setDatesPrior(fileDateNum,
                        Filter.DateCriteria.DATEUNIT_DAYS);
                } else {
                    date.setDatesPrior(fileDateNum,
                        Filter.DateCriteria.DATEUNIT_MONTHS);
                }
            } else {
                // Range "between"

                // The date will be interpreted using the pattern described
                // in the resource bundle.
                String datePattern = SamUtil.getResourceString(
                    "FilterSettings.fileDatePattern");
                SimpleDateFormat sdf = new SimpleDateFormat(datePattern);
                String startDateStr = getDisplayFieldStringValue(
                    CHILD_FILE_DATE_START);
                String endDateStr = getDisplayFieldStringValue(
                    CHILD_FILE_DATE_END);
                Date startDate;
                Date endDate;
                try {
                    startDate = sdf.parse(startDateStr);
                    endDate = sdf.parse(endDateStr);
                    GregorianCalendar startGregDate = new GregorianCalendar();
                    GregorianCalendar endGregDate = new GregorianCalendar();
                    startGregDate.setTime(startDate);
                    endGregDate.setTime(endDate);
                    // Make the time on the start date be at the start of the
                    // day.  The time on the end date should be the end of
                    // the day.
                    startGregDate.set(GregorianCalendar.HOUR_OF_DAY, 0);
                    startGregDate.set(GregorianCalendar.MINUTE, 0);
                    startGregDate.set(GregorianCalendar.SECOND, 0);
                    startGregDate.set(GregorianCalendar.MILLISECOND, 0);
                    endGregDate.set(GregorianCalendar.HOUR_OF_DAY, 23);
                    endGregDate.set(GregorianCalendar.MINUTE, 59);
                    endGregDate.set(GregorianCalendar.SECOND, 59);
                    endGregDate.set(GregorianCalendar.MILLISECOND, 999);
                    // Ensure start date is prior to end date
                    if (startGregDate.getTimeInMillis() >
                        endGregDate.getTimeInMillis()) {
                        addError("FilterSettings.error.badDateOrder", errors);
                        // At least set the other values so that they don't get
                        // cleared upon redisplay
                        date.setDateRangeType(
                            Filter.DateCriteria.DATERANGETYPE_BETWEEN);
                    } else {
                        date.setDatesBetween(startGregDate, endGregDate);
                    }
                } catch (ParseException e) {
                    String dateFormatHelp = SamUtil.getResourceString(
                        "FilterSettings.fileDateHelp");
                    String alertStr = SamUtil.getResourceString(
                        "FilterSettings.error.badDate",
                        dateFormatHelp);
                    addError(alertStr, errors);
                    // At least set the other values so that they don't get
                    // cleared upon redisplay
                    date.setDateRangeType(
                        Filter.DateCriteria.DATERANGETYPE_BETWEEN);
                }
            }
        }

        // Owner.  Owner can be blank for no filter
        String owner = getDisplayFieldStringValue(CHILD_OWNER);
        if (owner == null || owner.length() == 0) {
            filter.filterOnOwner(false, null);
        } else {
            filter.filterOnOwner(true, owner);
        }

        // Group.  Group can be blank for all groups
        String group = getDisplayFieldStringValue(CHILD_GROUP);
        if (group == null || group.length() == 0) {
            filter.filterOnGroup(false,  null);
        } else {
            filter.filterOnGroup(true, group);
        }

        // IsDamaged & isOnline
        if (version >= Filter.VERSION_2) {
            String isDamaged = getDisplayFieldStringValue(CHILD_ISDAMAGED);
            if (isDamaged.equals(BOOLEAN_YES)) {
                filter.filterOnIsDamaged(true, true);
            } else if (isDamaged.equals(BOOLEAN_NO)) {
                filter.filterOnIsDamaged(true, false);
            } else {
                filter.filterOnIsDamaged(false, false);
            }

            String isOnline = getDisplayFieldStringValue(CHILD_ISONLINE);
            if (isOnline.equals(BOOLEAN_YES)) {
                filter.filterOnIsOnline(true, true);
            } else if (isOnline.equals(BOOLEAN_NO)) {
                filter.filterOnIsOnline(true, false);
            } else {
                filter.filterOnIsOnline(false, false);
            }
        }

        if (errors.length() > 0) {
            // User entry errors
            SamFSException e = new SamFSException(errors.toString());
            SamUtil.setErrorAlert(
                this,
                ALERT,
                "FilterSettings.error.cantSave",
                e.getSAMerrno(),
                e.getMessage(),
                "");

            // Store in page context for page reload
            setPageSessionAttribute(FILTER_OBJECT, filter);

            return false;
        } else {
            // everything ok.  Store in returnvalue hidden object for return to
            // parent page.  Enter "true" into submitNow object so the
            // javascript knows to return the value and close the popup.
            try {
                setDisplayFieldValue(CHILD_RETURN_VALUE,
                    filter.writeEncodedString());
                setDisplayFieldValue(CHILD_SUBMIT_NOW, "true");
            } catch (UnsupportedEncodingException e) {
                SamFSException same = new SamFSException(e.getMessage());
                SamUtil.setErrorAlert(
                    this,
                    ALERT,
                    "FilterSettings.error.cantSave",
                    same.getSAMerrno(),
                    same.getMessage(),
                    "");
            }

            return true;
        }
    }


    private void addError(String newError, StringBuffer errorBuf) {
        if (errorBuf.length() > 0) {
            errorBuf.append(SamUtil.getResourceString(
                "FilterSettings.error.sentenceTerminator"));
        }
        errorBuf.append(SamUtil.getResourceString(newError));
    }
}
