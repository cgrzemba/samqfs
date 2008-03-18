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

// ident	$Id: Filter.java,v 1.9 2008/03/17 14:43:56 am143972 Exp $

package com.sun.netstorage.samqfs.web.util;

import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;

import java.io.Serializable;
import java.io.UnsupportedEncodingException;
import java.net.URLDecoder;
import java.net.URLEncoder;
import java.text.SimpleDateFormat;
import java.util.StringTokenizer;
import java.util.GregorianCalendar;
import java.util.Properties;

/**
 * class used for filtering of files based on various criteria
 */
public class Filter implements Serializable {

    /**
     * DateCriteria inner class
     */

    public class DateCriteria implements Serializable {
        private int dateRangeType = 0;
        public static final int DATERANGETYPE_INTHELAST = 9;
        public static final int DATERANGETYPE_BETWEEN = 10;
        private int dateValue = 0;
        private int dateUnit = 0;
        public static final int DATEUNIT_DAYS = 11;
        public static final int DATEUNIT_MONTHS = 12;
        private GregorianCalendar beforeDate;
        private GregorianCalendar afterDate;

        private final String KEY_DATE_RANGE_TYPE = "dateRangeType";
        private final String KEY_DATE_VALUE = "dateValue";
        private final String KEY_DATE_UNIT = "dateUnit";
        private final String KEY_BEFORE_DATE = "beforeDate";
        private final String KEY_AFTER_DATE = "afterDate";

        private DateCriteria() {
            dateRangeType = DATERANGETYPE_INTHELAST;
            dateUnit = DATEUNIT_DAYS;
        }

        public GregorianCalendar getBeforeDate() {
            return this.beforeDate;
        }
        public void setBeforeDate(GregorianCalendar date) {
            this.beforeDate = date;
        }
        public GregorianCalendar getAfterDate() {
            return this.afterDate;
        }
        public void setAfterDate(GregorianCalendar date) {
            this.afterDate = date;
        }

        public int getDateRangeType() {
            return this.dateRangeType;
        }

        public void setDateRangeType(int dateRangeType) throws SamFSException {
            if (dateRangeType != DATERANGETYPE_INTHELAST &&
                dateRangeType != DATERANGETYPE_BETWEEN) {
                throw new SamFSException(SamUtil.getResourceString(
                    "FilterSettings.filter.error.invalidDateRangeType",
                    String.valueOf(dateRangeType)));
            }
            this.dateRangeType = dateRangeType;
        }
        public int getDateValue() {
            return this.dateValue;
        }
        public void setDateValue(int dateValue) {
            this.dateValue = dateValue;
        }
        public int getDateUnit() {
            return this.dateUnit;
        }
        public void setDateUnit(int dateUnit) throws SamFSException {
            if (dateUnit != DATEUNIT_DAYS &&
                dateUnit != DATEUNIT_MONTHS) {
                throw new SamFSException(SamUtil.getResourceString(
                    "FilterSettings.filter.error.invalidDateUnit," +
                    String.valueOf(dateUnit)));
            }
            this.dateUnit = dateUnit;
        }


        /**
         * Convenience method for setting the date parameters.  This method sets
         * the filter to essentially say "give me the files whose
         * created|modified|archived dates are within the last x days|months.
         *
         * @param dateValue The number of days|months.  This amount should be a
         *  positive number which will be subtracted from the current date.
         *
         * @param dateUnit The unit that describes value.  Can be days or months
         *
         */
        public void setDatesPrior(int dateValue, int dateUnit)
        throws SamFSException {

            setDateUnit(dateUnit);
            setDateValue(dateValue);
            setDateRangeType(DATERANGETYPE_INTHELAST);

            // Prior means that it is before today
            this.beforeDate = new GregorianCalendar();

            // Translate days or months to an after date
            // Start with today and subtract time.
            this.afterDate = new GregorianCalendar();

            // Negate the value
            if (dateValue > 0) {
                dateValue = dateValue - (2 * dateValue);
            }
            if (dateUnit == DATEUNIT_DAYS) {
                this.afterDate.add(GregorianCalendar.DAY_OF_MONTH, dateValue);
            } else {
                // Months
                this.afterDate.add(GregorianCalendar.MONTH, dateValue);
            }
        }
        /**
         * Convenience method for setting the date parameters.  This method sets
         * the filter to essentially say "give me the files whose
         * created|modified|archived dates are after <date1> and before <date2>.
         * Obviously, date1 must come prior to date2...
         *
         * @param value
         * The number of days|months.  This amount should be a positive number
         * which will be subtracted from the current date.
         *
         * @param unit The unit that describes value.  Can be days or months.
         *
         */
        public void setDatesBetween(GregorianCalendar afterDate,
            GregorianCalendar beforeDate)
            throws SamFSException {

            // Normallize times on the date objects
            afterDate.set(GregorianCalendar.HOUR_OF_DAY, 0);
            afterDate.set(GregorianCalendar.MINUTE, 0);
            afterDate.set(GregorianCalendar.SECOND, 0);
            afterDate.set(GregorianCalendar.MILLISECOND, 0);
            beforeDate.set(GregorianCalendar.HOUR_OF_DAY, 0);
            beforeDate.set(GregorianCalendar.MINUTE, 0);
            beforeDate.set(GregorianCalendar.SECOND, 0);
            beforeDate.set(GregorianCalendar.MILLISECOND, 0);

            // This may seem wierd, but the afterDate must come chronologically
            // before the before date.  We want files dated after the afterDate,
            // but before the beforeDate.  Having them equal is ok.
            if (afterDate.getTimeInMillis() > beforeDate.getTimeInMillis()) {
                throw new SamFSException(SamUtil.getResourceString(
                    "FilterSettings.filter.error.invalidDatesBetween"));
            }

            setDateRangeType(DATERANGETYPE_BETWEEN);
            setAfterDate(afterDate);
            setBeforeDate(beforeDate);
        }

        public String toString() {
            StringBuffer out = new StringBuffer();
            if (this.dateRangeType == DATERANGETYPE_INTHELAST) {
                // "in the last x days" or "in the last x months"
                out.append(SamUtil.getResourceString(
                    "FilterSettings.fileDateRange.inTheLast")).append(" ")
                    .append(this.dateValue).append(" ");
                if (this.dateUnit == DATEUNIT_DAYS) {
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileDateUnit.days"));
                } else {
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileDateUnit.months"));
                }
            } else {
                // "between <afterdate> and <beforedate>"
                SimpleDateFormat sdf = new SimpleDateFormat(SamUtil
                    .getResourceString("FilterSettings.fileDatePattern"));
                out.append(SamUtil.getResourceString(
                    "FilterSettings.fileDateRange.between")).append(" ")
                    .append(sdf.format(this.afterDate.getTime())).append(" ")
                    .append(SamUtil.getResourceString(
                    "FilterSettings.fileDateText")).append(" ")
                    .append(sdf.format(this.beforeDate.getTime()));
            }
            return out.toString();
        }
        public void writePersistString(StringBuffer out) {
            // String will be delimited by \t to differentiate from the
            // delimiters of the filter class
            out.append(KEY_DATE_RANGE_TYPE).append("\t")
            .append(this.dateRangeType).append("\t")
            .append(KEY_DATE_UNIT).append("\t")
            .append(this.dateUnit).append("\t")
            .append(KEY_DATE_VALUE).append("\t")
            .append(this.dateValue).append("\t")
            .append(KEY_AFTER_DATE).append("\t")
            .append(this.afterDate.getTimeInMillis()).append("\t")
            .append(KEY_BEFORE_DATE).append("\t")
            .append(this.beforeDate.getTimeInMillis()).append("\t");
        }

        // Reads the string and populates members from contents of string.
        public void loadPersistString(String persistString) {
            StringTokenizer strTok = new StringTokenizer(persistString, "\t");
            while (strTok.hasMoreTokens()) {
                String name = strTok.nextToken();
                String value = strTok.nextToken();

                try {
                    if (name.equals(KEY_DATE_RANGE_TYPE)) {
                        this.setDateRangeType(Integer.parseInt(value));
                    } else if (name.equals(KEY_DATE_UNIT)) {
                        this.setDateUnit(Integer.parseInt(value));
                    } else if (name.equals(KEY_DATE_VALUE)) {
                        this.setDateValue(Integer.parseInt(value));
                    } else if (name.equals(KEY_AFTER_DATE)) {
                        GregorianCalendar date = new GregorianCalendar();
                        date.setTimeInMillis(Long.parseLong(value));
                        this.setAfterDate(date);
                    } else if (name.equals(KEY_BEFORE_DATE)) {
                        GregorianCalendar date = new GregorianCalendar();
                        date.setTimeInMillis(Long.parseLong(value));
                        this.setBeforeDate(date);
                    }
                } catch (Exception e) {
                    TraceUtil.trace1("Failed to load value \"" + name +
                        "\" from persisted string: " +
                        persistString, e);
                }
            }
        }
    }

    /**
     * SizeCriteria inner class
     */

    public class SizeCriteria implements Serializable {
        private long size;
        /*
         * sizeUnit may be one of the following:
         *  SamQFSSystemModel.SIZE_B, SamQFSSystemModel.SIZE_KB,
         *  SamQFSSystemModel.SIZE_MB, SamQFSSystemModel.SIZE_GB,
         *  SamQFSSystemModel.SIZE_TB, SamQFSSystemModel.SIZE_PB
         */
        private int sizeUnit;

        private final String KEY_SIZE = "size";
        private final String KEY_SIZE_UNIT = "sizeUnit";

        public SizeCriteria() {
            try {
                setSize(0L);
                setSizeUnit(SamQFSSystemModel.SIZE_B);
            } catch (SamFSException e) {
            }
        }
        public SizeCriteria(long size, int sizeUnit) throws SamFSException {
            setSize(size);
            setSizeUnit(sizeUnit);
        }
        public long getSize() {
            return this.size;
        }
        public void setSize(long size) throws SamFSException {
            if (size < 0) {
                throw new SamFSException(SamUtil.getResourceString(
                    "FilterSettings.filter.error.invalidSizeValue"));
            }
            this.size = size;
        }

        public int getSizeUnit() {
            return this.sizeUnit;
        }

        public void setSizeUnit(int sizeUnit) throws SamFSException {
            if (sizeUnit != SamQFSSystemModel.SIZE_B &&
                sizeUnit != SamQFSSystemModel.SIZE_GB &&
                sizeUnit != SamQFSSystemModel.SIZE_KB &&
                sizeUnit != SamQFSSystemModel.SIZE_MB &&
                sizeUnit != SamQFSSystemModel.SIZE_PB &&
                sizeUnit != SamQFSSystemModel.SIZE_TB) {
                throw new SamFSException(SamUtil.getResourceString(
                    "FilterSettings.filter.error.invalidSizeUnit",
                    String.valueOf(sizeUnit)));
            }
            this.sizeUnit = sizeUnit;
        }

        public String toString() {
            StringBuffer out = new StringBuffer()
            .append(this.size).append(" ");

            switch (this.sizeUnit) {
                case SamQFSSystemModel.SIZE_B:
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileSizeUnit.b"));
                    break;
                case SamQFSSystemModel.SIZE_KB:
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileSizeUnit.kb"));
                    break;
                case SamQFSSystemModel.SIZE_MB:
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileSizeUnit.mb"));
                    break;
                case SamQFSSystemModel.SIZE_GB:
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileSizeUnit.gb"));
                    break;
                case SamQFSSystemModel.SIZE_TB:
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileSizeUnit.tb"));
                    break;
                case SamQFSSystemModel.SIZE_PB:
                    out.append(SamUtil.getResourceString(
                        "FilterSettings.fileSizeUnit.pb"));
                    break;
            }
            return out.toString();
        }

        public void writePersistString(StringBuffer out) {
            // String will be delimited by \t to differentiate from the
            // delimiters of the filter class
            out.append(KEY_SIZE).append("\t")
            .append(this.size).append("\t")
            .append(KEY_SIZE_UNIT).append("\t")
            .append(this.sizeUnit).append("\t");
        }

        // Reads the string and populates members from contents of string.
        public void loadPersistString(String persistString) {
            StringTokenizer strTok = new StringTokenizer(persistString, "\t");
            while (strTok.hasMoreTokens()) {
                String name = strTok.nextToken();
                String value = strTok.nextToken();

                try {
                    if (name.equals(KEY_SIZE)) {
                        this.setSize(Long.parseLong(value));
                    } else if (name.equals(KEY_SIZE_UNIT)) {
                        this.setSizeUnit(Integer.parseInt(value));
                    }
                } catch (Exception e) {
                    TraceUtil.trace1("Failed to load value \"" + name +
                        "\" from persisted string: "
                        + persistString, e);
                }
            }
        }
    }  // class Size

    /**
     * Filter outer class
     */

    private String namePattern;
    private SizeCriteria sizeGreater;
    private SizeCriteria sizeLess;
    private DateCriteria dateCreated;
    private DateCriteria dateModified;
    private DateCriteria dateAccessed;
    private String owner;
    private String group;
    private Boolean isDamaged;
    private Boolean isOnline;
    private int version;  // uiVersion is carried around to be used by

    /**
     * VERSION_1:
     *   File name
     *   Size greater
     *   Size less
     *   Date created
     *   Date modified
     *   Date accessed
     *   Owner
     *   Group
     *
     * VERSION_2:
     *   isDamaged
     *   isOnline
     */

    // Version 1 is obsoleted.
    public final static int VERSION_1 = 1;
    public final static int VERSION_2 = 2;
    public final static int VERSION_LATEST = VERSION_2;
    private final static String KEY_VERSION = "version";

    public final static String KEY_FILENAME = "filename";
    public final static String KEY_OWNER = "owner";
    public final static String KEY_GROUP = "group";
    public final static String KEY_MODBEFORE = "modifiedBefore"; // secs
    public final static String KEY_MODAFTER = "modifiedAfter";  // secs
    public final static String KEY_CREATEDBEFORE = "createdBefore"; // secs
    public final static String KEY_CREATEDAFTER  = "createdAfter";  // secs
    public final static String KEY_BIGGERTHAN = "biggerThan";  // bytes
    public final static String KEY_SMALLERTHAN = "smallerThan"; // bytes
    public final static String KEY_DATEMODIFIED = "dateModified";
    public final static String KEY_DATEACCESSED = "dateAccessed";
    public final static String KEY_DATECREATED  = "dateCreated";
    public final static String KEY_SIZEGREATER  = "sizeGreater";
    public final static String KEY_SIZELESS = "sizeLess";
    public final static String KEY_DAMAGED = "damaged";
    public final static String KEY_ONLINE  = "online";

    private final static String CHAR_ENCODING = "UTF-8";

    public Filter() {
        this(VERSION_LATEST);
    }

    public Filter(int version) {
        TraceUtil.initTrace();

        // Set so that all criteria are "unused"
        this.namePattern = null;
        this.sizeGreater = null;
        this.sizeLess = null;
        this.dateCreated = null;
        this.dateModified = null;
        this.dateAccessed = null;
        this.owner = null;
        this.group = null;
        this.isDamaged = null;
        this.isOnline = null;
        this.version = version;
    }

    public Filter(int version, Properties props) throws SamFSException {
        this(version);
        String prop;
        Filter.DateCriteria date;
        Filter.SizeCriteria size;

        if (null != (prop = props.getProperty(KEY_FILENAME))) {
            filterOnNamePattern(true, prop);
        }

        if (null != (prop = props.getProperty(KEY_OWNER))) {
            filterOnOwner(true, prop);
        }
        if (null != (prop = props.getProperty(KEY_GROUP))) {
            filterOnGroup(true, prop);
        }
        if (null != (prop = props.getProperty(KEY_MODBEFORE))) {
            filterOnDateModified(true);
            date = getDateModified();
            date.setBeforeDate(SamQFSUtil.convertTime(
                ConversionUtil.strToLongVal(prop)));
        }
        if (null != (prop = props.getProperty(KEY_MODAFTER))) {
            filterOnDateModified(true);
            date = getDateModified();
            date.setAfterDate(SamQFSUtil.convertTime(
                ConversionUtil.strToLongVal(prop)));
        }
        if (null != (prop = props.getProperty(KEY_CREATEDBEFORE))) {
            filterOnDateCreated(true);
            date = getDateCreated();
            date.setBeforeDate(SamQFSUtil.convertTime(
                ConversionUtil.strToLongVal(prop)));
        }
        if (null != (prop = props.getProperty(KEY_CREATEDAFTER))) {
            filterOnDateCreated(true);
            date = getDateCreated();
            date.setAfterDate(SamQFSUtil.convertTime
                (ConversionUtil.strToLongVal(prop)));
        }
        if (null != (prop = props.getProperty(KEY_BIGGERTHAN))) {
            filterOnSizeGreater(true);
            size = getSizeGreater();
            size.setSize(ConversionUtil.strToLongVal(prop));
        }
        if (null != (prop = props.getProperty(KEY_SMALLERTHAN))) {
            filterOnSizeLess(true);
            size = getSizeLess();
            size.setSize(ConversionUtil.strToLongVal(prop));
        }

        if (null != (prop = props.getProperty(KEY_DAMAGED))) {
            filterOnIsDamaged(true,  prop.equals("1") ? true : false);
        }
        if (null != (prop = props.getProperty(KEY_ONLINE))) {
            filterOnIsOnline(true,  prop.equals("1") ? true : false);
        }

    }

    public int getVersion() { return version; }

    // Returns null if the criteria is not set.
    public String getNamePattern() {
        return this.namePattern;
    }

    // If enabled = false, namePattern is ignored.
    public void filterOnNamePattern(boolean enabled, String namePattern) {
        if (enabled) {
            this.namePattern = namePattern;
        } else {
            this.namePattern = null;
        }
    }

    // Returns null if the criteria is not set.
    public DateCriteria getDateCreated() {
        return this.dateCreated;
    }

    public void filterOnDateCreated(boolean enabled) {
        if (enabled) {
            this.dateCreated = new Filter.DateCriteria();
        } else {
            this.dateCreated = null;
        }
    }

    // Returns null if the criteria is not set.
    public DateCriteria getDateModified() {
        return this.dateModified;
    }

    public void filterOnDateModified(boolean enabled) {
        if (enabled) {
            this.dateModified = new Filter.DateCriteria();
        } else {
            this.dateModified = null;
        }
    }

    // Returns null if the criteria is not set.
    public DateCriteria getDateAccessed() {
        return this.dateAccessed;
    }

    public void filterOnDateAccessed(boolean enabled) {
        if (enabled) {
            this.dateAccessed = new Filter.DateCriteria();
        } else {
            this.dateAccessed = null;
        }
    }

    // Returns null if the criteria is not set.
    public SizeCriteria getSizeGreater() {
        return this.sizeGreater;
    }

    public void filterOnSizeGreater(boolean enabled) {
        if (enabled) {
            this.sizeGreater = new Filter.SizeCriteria();
        } else {
            this.sizeGreater = null;
        }
    }

    // Returns null if the criteria is not set.
    public SizeCriteria getSizeLess() {
        return this.sizeLess;
    }

    public void filterOnSizeLess(boolean enabled) {
        if (enabled) {
            this.sizeLess = new Filter.SizeCriteria();
        } else {
            this.sizeLess = null;
        }
    }

    // Returns null if the criteria is not set.
    public String getOwner() {
        return this.owner;
    }

    // If enabled = false, id is ignored.
    public void filterOnOwner(boolean enabled, String owner) {
        if (enabled) {
            this.owner = owner;
        } else {
            this.owner = null;
        }
    }

    // Returns null if the criteria is not set.
    public String getGroup() {
        return this.group;
    }

    // If enabled = false, id is ignored.
    public void filterOnGroup(boolean enabled, String group) {
        if (enabled) {
            this.group = group;
        } else {
            this.group = null;
        }
    }

    // Returns null if the criteria is not set.
    public Boolean getIsDamaged() {
        return this.isDamaged;
    }

    /**
     * Sets or clears the isDamaged filter criteria.
     *
     * @param filterEnabled When true, the filter criteria will be used.
     * When false, the filter criteria is ignored.
     *
     * @param isDamaged The value of this criteria.  When true,
     * the filter will pass files that are damaged.  When false,
     * the filter will pass files that are not damaged.  If filterEnabled
     * is false, this parameter is ignored.
     */
    public void filterOnIsDamaged(boolean filterEnabled, boolean isDamaged) {
        if (filterEnabled) {
            this.isDamaged = Boolean.valueOf(isDamaged);
        } else {
            this.isDamaged = null;
        }
    }

    // Returns null if the criteria is not set.
    public Boolean getIsOnline() {
        return this.isOnline;
    }

    /**
     * Sets or clears the isOnline filter criteria.
     *
     * @param filterEnabled When true, the filter criteria will be used.
     * When false, the filter criteria is ignored.
     *
     * @param isOnline The value of this criteria.  When true,
     * the filter will pass files that are online.  When false,
     * the filter will pass files that are offline.  If filterEnabled
     * is false, this parameter is ignored.
     */
    public void filterOnIsOnline(boolean filterEnabled, boolean isOnline) {
        if (filterEnabled) {
            this.isOnline = Boolean.valueOf(isOnline);
        } else {
            this.isOnline = null;
        }
    }

    /**
     * encode data in the form expected by the lower layer
     */
    public String toString() {
        StringBuffer s = new StringBuffer();
        if (namePattern != null) {
            s.append(KEY_FILENAME).append('=').append(namePattern).append(',');
        }
        if (owner != null) {
            s.append(KEY_OWNER).append('=').append(owner).append(',');
        }
        if (group != null) {
            s.append(KEY_GROUP).append('=').append(group).append(',');
        }
        if (this.dateModified != null &&
            this.dateModified.getBeforeDate() != null) {
            s.append(KEY_MODBEFORE).append('=')
            .append(String.valueOf(SamQFSUtil.convertTime(
                this.dateModified.getBeforeDate())))
                .append(',');
        }
        if (this.dateModified != null &&
            this.dateModified.getAfterDate() != null) {
            s.append(KEY_MODAFTER).append('=')
            .append(String.valueOf(SamQFSUtil.convertTime(
                this.dateModified.getAfterDate())))
                .append(',');
        }
        if (this.dateCreated != null &&
            this.dateCreated.getBeforeDate() != null) {
            s.append(KEY_CREATEDBEFORE).append('=')
            .append(String.valueOf(SamQFSUtil.convertTime(
                this.dateCreated.getBeforeDate())))
                .append(',');
        }
        if (this.dateCreated != null &&
            this.dateCreated.getAfterDate() != null) {
            s.append(KEY_CREATEDAFTER).append('=')
            .append(String.valueOf(SamQFSUtil.convertTime(
                this.dateCreated.getAfterDate())))
                .append(',');
        }
        if (this.sizeGreater != null) {
            s.append(KEY_BIGGERTHAN).append('=')
            .append(String.valueOf(SamQFSUtil.getSizeInBytes(
                this.sizeGreater.getSize(),
                this.sizeGreater.getSizeUnit())))
                .append(',');
        }
        if (this.sizeLess != null) {
            s.append(KEY_SMALLERTHAN).append('=')
            .append(String.valueOf(SamQFSUtil.getSizeInBytes(
                this.sizeLess.getSize(),
                this.sizeLess.getSizeUnit())))
                .append(',');
        }
        if (this.isDamaged != null) {
            s.append(KEY_DAMAGED).append('=')
            .append(isDamaged.booleanValue() ? "1" : "0")
            .append(',');
        }

        if (this.isOnline != null) {
            s.append(KEY_ONLINE).append('=')
            .append(isOnline.booleanValue() ? "1" : "0")
            .append(',');
        }

        if (s.length() > 0) {
            s.setLength(s.length() - 1); // remove last ','
        }
        return s.toString();

    }

    public String getDisplayString() {

        // Field order should match the order fields are arranged in the
        // popup.
        StringBuffer s = new StringBuffer();
        if (namePattern != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.fileNamePatternLabel")).append(" = ")
                .append(namePattern).append("<br>");
        }
        if (sizeGreater != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.fileSizeGreaterLabel")).append(" ")
                .append(sizeGreater.toString()).append("<br>");
        }
        if (sizeLess != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.fileSizeLessLabel")).append(" ")
                .append(sizeLess.toString()).append("<br>");
        }
        if (dateCreated != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.fileDateLabel")).append(" ")
                .append(SamUtil.getResourceString(
                "FilterSettings.fileDateType.created")).append(" = ")
                .append(dateCreated.toString()).append("<br>");
        }
        if (dateModified != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.fileDateLabel")).append(" ")
                .append(SamUtil.getResourceString(
                "FilterSettings.fileDateType.modified")).append(" = ")
                .append(dateModified.toString()).append("<br>");
        }
        if (dateAccessed != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.fileDateLabel")).append(" ")
                .append(SamUtil.getResourceString(
                "FilterSettings.fileDateType.lastAccessed")).append(" = ")
                .append(dateAccessed.toString()).append("<br>");
        }
        if (owner != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.ownerLabel")).append(" = ")
                .append(owner).append("<br>");
        }
        if (group != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.groupLabel")).append(" = ")
                .append(group).append("<br>");
        }
        if (isDamaged != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.isDamagedLabel")).append(" = ")
                .append(isDamaged.booleanValue()
                ? SamUtil.getResourceString("samqfsui.yes")
                : SamUtil.getResourceString("samqfsui.no"))
                .append("<br>");
        }
        if (isOnline != null) {
            s.append(SamUtil.getResourceString(
                "FilterSettings.isOnlineLabel")).append(" = ")
                .append(isOnline.booleanValue()
                ? SamUtil.getResourceString("samqfsui.yes")
                : SamUtil.getResourceString("samqfsui.no"))
                .append("<br>");
        }
        // Delete trailing "<br>"
        String outStr = s.toString();
        if (s.length() > 1) {
            outStr = outStr.substring(0, outStr.length() - 4);
        }
        return outStr;
    }

    public String writeEncodedString() throws UnsupportedEncodingException {
        // Persisted object is a list of name value pairs delimited by \n.
        // Names and values are also separated by \n, so \n must always appear
        // in pairs.
        StringBuffer out = new StringBuffer();
        String delimiter = "\n";

        out.append(KEY_VERSION).append(delimiter)
        .append(this.version).append(delimiter);
        if (this.namePattern != null) {
            out.append(KEY_FILENAME).append(delimiter)
            .append(this.namePattern).append(delimiter);
        }
        if (this.owner != null) {
            out.append(KEY_OWNER).append(delimiter)
            .append(this.owner).append(delimiter);
        }
        if (this.group != null) {
            out.append(KEY_GROUP).append(delimiter)
            .append(this.group).append(delimiter);
        }
        if (this.isDamaged != null) {
            out.append(KEY_DAMAGED).append(delimiter)
            .append(this.isDamaged.toString()).append(delimiter);
        }
        if (this.isOnline != null) {
            out.append(KEY_ONLINE).append(delimiter)
            .append(this.isOnline.toString()).append(delimiter);
        }
        if (this.dateAccessed != null) {
            out.append(KEY_DATEACCESSED).append(delimiter);
            this.dateAccessed.writePersistString(out);
            out.append(delimiter);
        }
        if (this.dateCreated != null) {
            out.append(KEY_DATECREATED).append(delimiter);
            this.dateCreated.writePersistString(out);
            out.append(delimiter);
        }
        if (this.dateModified != null) {
            out.append(KEY_DATEMODIFIED).append(delimiter);
            this.dateModified.writePersistString(out);
            out.append(delimiter);
        }
        if (this.sizeGreater != null) {
            out.append(KEY_SIZEGREATER).append(delimiter);
            this.sizeGreater.writePersistString(out);
            out.append(delimiter);
        }
        if (this.sizeLess != null) {
            out.append(KEY_SIZELESS).append(delimiter);
            this.sizeLess.writePersistString(out);
            out.append(delimiter);
        }

        return URLEncoder.encode(out.toString(), CHAR_ENCODING);
    }

    public static Filter getFilterFromEncodedString(String persistString)
        throws UnsupportedEncodingException {
        String unencodedStr = URLDecoder.decode(persistString, CHAR_ENCODING);
        StringTokenizer strTok = new StringTokenizer(unencodedStr, "\n");
        Filter filter = new Filter();
        while (strTok.hasMoreTokens()) {
            // get next name value pair
            String name = strTok.nextToken();
            String value = strTok.nextToken();
            if (name.equals(KEY_VERSION)) {
                int version = Filter.VERSION_LATEST;
                try {
                    version = Integer.parseInt(value);
                } catch (NumberFormatException e) {
                }
                filter.version = version;
            } else if (name.equals(KEY_FILENAME)) {
                filter.filterOnNamePattern(true, value);
            } else if (name.equals(KEY_OWNER)) {
                filter.filterOnOwner(true, value);
            } else if (name.equals(KEY_GROUP)) {
                filter.filterOnGroup(true, value);
            } else if (name.equals(KEY_DAMAGED)) {
                filter.filterOnIsDamaged(
                    true, Boolean.valueOf(value).booleanValue());
            } else if (name.equals(KEY_ONLINE)) {
                filter.filterOnIsOnline(
                    true, Boolean.valueOf(value).booleanValue());
            } else if (name.equals(KEY_DATEACCESSED)) {
                filter.filterOnDateAccessed(true);
                filter.getDateAccessed().loadPersistString(value);
            } else if (name.equals(KEY_DATECREATED)) {
                filter.filterOnDateCreated(true);
                filter.getDateCreated().loadPersistString(value);
            } else if (name.equals(KEY_DATEMODIFIED)) {
                filter.filterOnDateModified(true);
                filter.getDateModified().loadPersistString(value);
            } else if (name.equals(KEY_SIZEGREATER)) {
                filter.filterOnSizeGreater(true);
                filter.getSizeGreater().loadPersistString(value);
            } else if (name.equals(KEY_SIZELESS)) {
                filter.filterOnSizeLess(true);
                filter.getSizeLess().loadPersistString(value);
            }
        }
        return filter;
    }
}
