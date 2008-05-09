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

// ident	$Id: SamUtil.java,v 1.119 2008/05/09 21:08:57 kilemba Exp $

package com.sun.netstorage.samqfs.web.util;

import com.iplanet.jato.RequestContext;
import com.iplanet.jato.RequestManager;
import com.iplanet.jato.view.ContainerView;
import com.sun.netstorage.samqfs.mgmt.SamFSAccessDeniedException;
import com.sun.netstorage.samqfs.mgmt.SamFSCommException;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.SamFSMultiHostException;
import com.sun.netstorage.samqfs.mgmt.media.CatEntry;
import com.sun.netstorage.samqfs.web.model.SamQFSAppModel;
import com.sun.netstorage.samqfs.web.model.SamQFSFactory;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemAdminManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.admin.Schedule;
import com.sun.netstorage.samqfs.web.model.admin.ScheduleTaskID;
import com.sun.netstorage.samqfs.web.model.alarm.Alarm;
import com.sun.netstorage.samqfs.web.model.fs.RecoveryPointSchedule;
import com.sun.netstorage.samqfs.web.model.impl.jni.SamQFSUtil;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.Library;
import com.sun.netstorage.samqfs.web.server.ServerUtil;
import com.sun.web.ui.common.CCI18N;
import com.sun.web.ui.view.alert.CCAlertInline;
import java.text.DateFormat;
import java.text.MessageFormat;
import java.util.Arrays;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.StringTokenizer;
import java.util.TreeSet;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import javax.faces.context.ExternalContext;
import javax.faces.context.FacesContext;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.servlet.http.HttpSession;


public class SamUtil {

    /**
     * determine if the given server is a cluster node
     *
     * @param String serverName - name of the server
     * @return boolean - <code>true</code> if the given server is a cluster
     *  node, otherwise, <code>false</code>
     * @throws SamFSException
     */
    public static boolean isClusterNode(String serverName)
        throws SamFSException {
        return getModel(serverName).isClusterNode();
    }

    /**
     * Get the system model of a SAM-QFS server
     * @param serverName - name of the SAM-QFS server
     * @return return the system model of a SAM-QFS server
     */
    public static SamQFSSystemModel getModel(String serverName)
        throws SamFSException {

        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
        SamQFSSystemModel model = appModel.getSamQFSSystemModel(serverName);

        if (model.isDown()) {
            if (model.isAccessDenied()) {
                throw new SamFSException(null, -2803);
            } else {
                throw new SamFSException(null, -2800);
            }
        }

        return model;
    }

    /**
     * Get the API version number of a SAM-QFS server
     * @param serverName - name of the SAM-QFS server
     * @return return the API version number of a SAM-QFS server
     */
    public static String getAPIVersion(String serverName)
        throws SamFSException {
        // construct version key
        String key = new StringBuffer().append(
            Constants.SessionAttributes.API_VERSION_PREFIX).append(".").append(
            serverName).toString();

        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        // Always fetch version number from sysModel
        // DO NOT check if the key already contains a value and simply
        // return this value, this will cause a caching problem.
        SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
        SamQFSSystemModel model = appModel.getSamQFSSystemModel(serverName);

        if (model.isDown()) {
            throw new SamFSException(null, -2800);
        }
        String version = model.getServerAPIVersion();
        session.setAttribute(key, version);
        return version;
    }

    /**
     * Version returned from the api is of the major.minor format
     * Versions can be in 1.3.1 etc. So version should not be expected to be
     * floating number
     * Variable to hold samfs server api version number
     * api 1.0 = samfs ui 4.1
     * api 1.1 = samfs ui 4.2
     * api 1.2 = samfs ui 4.3
     * api 1.3 = samfs ui 4.4
     * api 1.3.1 = samfs ui 4.4  this is used for internal release updates
     * api 1.4 = samfs ui 4.5
     *
     */
    public static String getAPIVersionFromUIVersion(String uiVersion) {
        if (uiVersion.equals("4.1")) {
            return "1.0";
        } else if (uiVersion.equals("4.2")) {
            return "1.1";
        } else if (uiVersion.equals("4.3")) {
            return "1.2";
        } else if (uiVersion.equals("4.4")) {
            return "1.3";
        } else if (uiVersion.equals("4.5")) {
            return "1.4";
        } else if (uiVersion.equals("4.6")) {
            return "1.5";
        } else if (uiVersion.equals("5.0")) {
            return "1.6";
        } else {
            return "";
        }
    }

    /**
     * @param version The version in question.
     * @param referenceVersion The version you're comparing against.
     * @return True if version is the same version as reference vesion or is
     * a more recent version, otherwise false.
     */
    public static boolean isVersionCurrentOrLaterThan(String version,
                                                      String referenceVersion) {
        return version.compareTo(referenceVersion) >= 0;
    }

    // Method to retrive corresponding library driver
    public static String getLibraryDriverString(int dValue) {
        String libraryDriverString = null;

        switch (dValue) {
            case Library.SAMST:
                libraryDriverString = getResourceString("library.driver.samst");
                break;

            case Library.ACSLS:
                libraryDriverString = getResourceString("library.driver.acsls");
                break;

            case Library.ADIC_GRAU:
                libraryDriverString = getResourceString("library.driver.adic");
                break;

            case Library.FUJITSU_LMF:
                libraryDriverString = getResourceString("library.driver.fuji");
                break;

            case Library.IBM_3494:
                libraryDriverString = getResourceString("library.driver.ibm");
                break;

            case Library.SONY:
                libraryDriverString = getResourceString("library.driver.sony");
                break;

            default:
                libraryDriverString = getResourceString("library.driver.other");
                break;
        }

        return (libraryDriverString);
    }

    // Method to retrive corresponding library states
    public static String getStateString(int sValue) {
        String libraryStateString = null;

        switch (sValue) {
            case BaseDevice.ON:
                libraryStateString =
                    getResourceString("LibrarySummary.status1");
                break;

            case BaseDevice.OFF:
                libraryStateString =
                    getResourceString("LibrarySummary.status2");
                break;

            case BaseDevice.DOWN:
                libraryStateString =
                    getResourceString("LibrarySummary.status3");
                break;

            case BaseDevice.UNAVAILABLE:
                libraryStateString =
                    getResourceString("LibrarySummary.status4");
                break;

            case BaseDevice.IDLE:
                libraryStateString =
                    getResourceString("LibrarySummary.status5");
                break;

            case BaseDevice.READONLY:
                libraryStateString =
                    getResourceString("LibrarySummary.status6");
                break;

            default:
                libraryStateString =
                    getResourceString("LibrarySummary.status7");
                break;
        }

        return (libraryStateString);
    }

    // Method to retrieve corresponding media type names
    public static String getMediaTypeString(int mValue) {
        String mediaTypeString = null;

        switch (mValue) {
            case BaseDevice.MTYPE_SONY_AIT:
                mediaTypeString = getResourceString("media.type.sony");
                break;

            case BaseDevice.MTYPE_AMPEX_DST310:
                mediaTypeString = getResourceString("media.type.ampex");
                break;

            case BaseDevice.MTYPE_STK_SD3:
                mediaTypeString = getResourceString("media.type.stk");
                break;

            case BaseDevice.MTYPE_DAT:
                mediaTypeString = getResourceString("media.type.dat");
                break;

            case BaseDevice.MTYPE_FUJITSU_M8100:
                mediaTypeString = getResourceString("media.type.fuji");
                break;

            case BaseDevice.MTYPE_IBM_3570:
                mediaTypeString = getResourceString("media.type.ibm3570");
                break;

            case BaseDevice.MTYPE_IBM_3580_LTO:
                mediaTypeString = getResourceString("media.type.ibm3580");
                break;

            case BaseDevice.MTYPE_DLT:
                mediaTypeString = getResourceString("media.type.dlt");
                break;

            case BaseDevice.MTYPE_STK_9490:
                mediaTypeString = getResourceString("media.type.stk9490");
                break;

            case BaseDevice.MTYPE_STK_9840:
                mediaTypeString = getResourceString("media.type.stk9840");
                break;

            case BaseDevice.MTYPE_SONY_DTF:
                mediaTypeString = getResourceString("media.type.sonydtf");
                break;

            case BaseDevice.MTYPE_METRUM_VHS:
                mediaTypeString = getResourceString("media.type.metrum");
                break;

            case BaseDevice.MTYPE_EXABYTE_MAMMOTH2:
                mediaTypeString = getResourceString("media.type.exabytem");
                break;

            case BaseDevice.MTYPE_EOD:
                mediaTypeString = getResourceString("media.type.eod");
                break;

            case BaseDevice.MTYPE_WOD:
                mediaTypeString = getResourceString("media.type.wod");
                break;

            case BaseDevice.MTYPE_HISTORIAN:
                mediaTypeString = getResourceString("media.type.historian");
                break;

            case BaseDevice.MTYPE_IBM_3590:
                mediaTypeString = getResourceString("media.type.ibm3590");
                break;

            case BaseDevice.MTYPE_STK_T9940:
                mediaTypeString = getResourceString("media.type.stk9940");
                break;

            case BaseDevice.MTYPE_STK_3480:
                mediaTypeString = getResourceString("media.type.stk3480");
                break;

            case BaseDevice.MTYPE_EXABYTE_8MM:
                mediaTypeString = getResourceString("media.type.exabyte8");
                break;

            case BaseDevice.MTYPE_12WOD:
                mediaTypeString = getResourceString("media.type.12wod");
                break;

            case BaseDevice.MTYPE_OPTICAL:
                mediaTypeString = getResourceString("media.type.optical");
                break;

            case BaseDevice.MTYPE_TAPE:
                mediaTypeString = getResourceString("media.type.tape");
                break;

            case BaseDevice.MTYPE_ROBOT:
                mediaTypeString = getResourceString("media.type.robot");
                break;

            case BaseDevice.MTYPE_HP_L_SERIES:
                mediaTypeString = getResourceString("media.type.hp");
                break;

            case BaseDevice.MTYPE_STK_ACSLS:
                mediaTypeString = getResourceString("media.type.stkacsls");
                break;

            case BaseDevice.MTYPE_ADIC_DAS:
                mediaTypeString = getResourceString("media.type.adicdas");
                break;

            case BaseDevice.MTYPE_FUJ_LMF:
                mediaTypeString = getResourceString("media.type.fujilmf");
                break;

            case BaseDevice.MTYPE_IBM_3494:
                mediaTypeString = getResourceString("media.type.ibm3494");
                break;

            case BaseDevice.MTYPE_SONY_PETASITE:
                mediaTypeString = getResourceString("media.type.sonypetasite");
                break;

            case BaseDevice.MTYPE_STK_97xx:
                mediaTypeString = getResourceString("media.type.stk97");
                break;

            case BaseDevice.MTYPE_DISK:
                mediaTypeString = getResourceString("media.type.disk");
                break;

            case BaseDevice.MTYPE_SSTG_L_SERIES:
                mediaTypeString =
                    getResourceString("media.type.sstorage_l_series");
                break;

            case BaseDevice.MTYPE_DT_QUANTUMC4:
                mediaTypeString = getResourceString("media.type.quantumc4");
                break;

            case BaseDevice.MTYPE_DT_TITAN:
                mediaTypeString =
                    getResourceString("media.type.titanium");
                break;
            case BaseDevice.MTYPE_STK_5800:
                mediaTypeString = getResourceString("media.type.honeycomb");
                break;
            case BaseDevice.MTYPE_HP_SL48:
                mediaTypeString = getResourceString("media.type.hp.sl48");
                break;
            default:
                mediaTypeString = getResourceString("media.type.unknown");
                break;

        }
        return (mediaTypeString);
    }

    // return the media type description when given the 2 char mType
    public static String getMediaTypeString(String mType) {

        return getMediaTypeString(SamQFSUtil.getEquipTypeInteger(mType));
    }

    // Method to return corresponding media type int
    public static int getMediaType(String mValue) {
        int mediaType = -1;
        if (mValue.equals(getResourceString("media.type.sony"))) {
            mediaType = BaseDevice.MTYPE_SONY_AIT;
        } else if (mValue.equals(getResourceString("media.type.ampex"))) {
            mediaType = BaseDevice.MTYPE_AMPEX_DST310;
        } else if (mValue.equals(getResourceString("media.type.stk"))) {
            mediaType = BaseDevice.MTYPE_STK_SD3;
        } else if (mValue.equals(getResourceString("media.type.dat"))) {
            mediaType = BaseDevice.MTYPE_DAT;
        } else if (mValue.equals(getResourceString("media.type.fuji"))) {
            mediaType = BaseDevice.MTYPE_FUJITSU_M8100;
        } else if (mValue.equals(getResourceString("media.type.ibm3570"))) {
            mediaType = BaseDevice.MTYPE_IBM_3570;
        } else if (mValue.equals(getResourceString("media.type.ibm3580"))) {
            mediaType = BaseDevice.MTYPE_IBM_3580_LTO;
        } else if (mValue.equals(getResourceString("media.type.dlt"))) {
            mediaType = BaseDevice.MTYPE_DLT;
        } else if (mValue.equals(getResourceString("media.type.stk9490"))) {
            mediaType = BaseDevice.MTYPE_STK_9490;
        } else if (mValue.equals(getResourceString("media.type.stk9840"))) {
            mediaType = BaseDevice.MTYPE_STK_9840;
        } else if (mValue.equals(getResourceString("media.type.sonydtf"))) {
            mediaType = BaseDevice.MTYPE_SONY_DTF;
        } else if (mValue.equals(getResourceString("media.type.metrum"))) {
            mediaType = BaseDevice.MTYPE_METRUM_VHS;
        } else if (mValue.equals(getResourceString("media.type.exabytem"))) {
            mediaType = BaseDevice.MTYPE_EXABYTE_MAMMOTH2;
        } else if (mValue.equals(getResourceString("media.type.eod"))) {
            mediaType = BaseDevice.MTYPE_EOD;
        } else if (mValue.equals(getResourceString("media.type.wod"))) {
            mediaType = BaseDevice.MTYPE_WOD;
        } else if (mValue.equals(getResourceString("media.type.historian"))) {
            mediaType = BaseDevice.MTYPE_HISTORIAN;
        } else if (mValue.equals(getResourceString("media.type.ibm3590"))) {
            mediaType = BaseDevice.MTYPE_IBM_3590;
        } else if (mValue.equals(getResourceString("media.type.stk9940"))) {
            mediaType = BaseDevice.MTYPE_STK_T9940;
        } else if (mValue.equals(getResourceString("media.type.stk3480"))) {
            mediaType = BaseDevice.MTYPE_STK_3480;
        } else if (mValue.equals(getResourceString("media.type.exabyte8"))) {
            mediaType = BaseDevice.MTYPE_EXABYTE_8MM;
        } else if (mValue.equals(getResourceString("media.type.12wod"))) {
            mediaType = BaseDevice.MTYPE_12WOD;
        } else if (mValue.equals(getResourceString("media.type.optical"))) {
            mediaType = BaseDevice.MTYPE_OPTICAL;
        } else if (mValue.equals(getResourceString("media.type.tape"))) {
            mediaType = BaseDevice.MTYPE_TAPE;
        } else if (mValue.equals(getResourceString("media.type.robot"))) {
            mediaType = BaseDevice.MTYPE_ROBOT;
        } else if (mValue.equals(getResourceString("media.type.hp"))) {
            mediaType = BaseDevice.MTYPE_HP_L_SERIES;
        } else if (mValue.equals(getResourceString("media.type.stkacsls"))) {
            mediaType = BaseDevice.MTYPE_STK_ACSLS;
        } else if (mValue.equals(getResourceString("media.type.adicdas"))) {
            mediaType = BaseDevice.MTYPE_ADIC_DAS;
        } else if (mValue.equals(getResourceString("media.type.fujilmf"))) {
            mediaType = BaseDevice.MTYPE_FUJ_LMF;
        } else if (mValue.equals(getResourceString("media.type.ibm3494"))) {
            mediaType = BaseDevice.MTYPE_IBM_3494;
        } else if (mValue.equals(
            getResourceString("media.type.sonypetasite"))) {
            mediaType = BaseDevice.MTYPE_SONY_PETASITE;
        } else if (mValue.equals(getResourceString("media.type.stk97"))) {
            mediaType = BaseDevice.MTYPE_STK_97xx;
        } else if (mValue.equals(getResourceString("media.type.disk"))) {
            mediaType = BaseDevice.MTYPE_DISK;
        } else if (mValue.equals(
            getResourceString("media.type.sstorage_l_series"))) {
            mediaType = BaseDevice.MTYPE_SSTG_L_SERIES;
        } else if (mValue.equals(getResourceString("media.type.quantumc4"))) {
            mediaType = BaseDevice.MTYPE_DT_QUANTUMC4;
        } else if (mValue.equals(
            getResourceString("media.type.titanium"))) {
            mediaType = BaseDevice.MTYPE_DT_TITAN;
        } else if (mValue.equals(getResourceString("media.type.honeycomb"))) {
            mediaType = BaseDevice.MTYPE_STK_5800;
        } else if (mValue.equals(getResourceString("media.type.hp.sl48"))) {
            mediaType = BaseDevice.MTYPE_HP_SL48;
        }

        return (mediaType);
    }

    // method to retrieve the unit time string (localized) from integer
    public static String getTimeUnitL10NString(int unit) {
        switch (unit) {
            case SamQFSSystemModel.TIME_WEEK:
                return SamUtil.getResourceString(
                    "common.unit.time.weeks");
            case SamQFSSystemModel.TIME_DAY:
                return SamUtil.getResourceString(
                    "common.unit.time.days");
            case SamQFSSystemModel.TIME_HOUR:
                return SamUtil.getResourceString(
                    "common.unit.time.hours");
            case SamQFSSystemModel.TIME_MINUTE:
                return SamUtil.getResourceString(
                    "common.unit.time.minutes");
            case SamQFSSystemModel.TIME_SECOND:
                return SamUtil.getResourceString(
                    "common.unit.time.seconds");
        case SamQFSSystemModel.TIME_MONTHS:
            return SamUtil.getResourceString(
                    "common.unit.time.months");
        case SamQFSSystemModel.TIME_DAY_OF_MONTH:
            return SamUtil.getResourceString(
                    "common.unit.time.dayofmonth");
        case SamQFSSystemModel.TIME_DAY_OF_WEEK:
            return SamUtil.getResourceString(
                    "common.unit.time.dayofweek");
        case SamQFSSystemModel.TIME_YEAR:
            return SamUtil.getResourceString("common.unit.time.years");
            default:
                return "";
        }
    }
    // method to retrieve the unit size String (localized) from integer
    public static String getSizeUnitL10NString(int unit) {
        switch (unit) {
            case SamQFSSystemModel.SIZE_B:
                return SamUtil.getResourceString(
                    "AdminSetup.qutity1");
            case SamQFSSystemModel.SIZE_KB:
                return SamUtil.getResourceString(
                    "AdminSetup.qutity2");
            case SamQFSSystemModel.SIZE_MB:
                return SamUtil.getResourceString(
                    "AdminSetup.qutity3");
            case SamQFSSystemModel.SIZE_GB:
                return SamUtil.getResourceString(
                    "AdminSetup.qutity4");
            case SamQFSSystemModel.SIZE_TB:
                return SamUtil.getResourceString(
                    "AdminSetup.qutity5");
            case SamQFSSystemModel.SIZE_PB:
                return SamUtil.getResourceString(
                    "AdminSetup.qutity6");
            default:
                return "";
        }
    }

    // method to retrieve the unit size String (localized) from integer
    public static String getDurationL10NString(boolean plural, int unit) {
        switch (unit) {
            case TimeConvertor.UNIT_MILLI_SEC:
                return
                    SamUtil.getResourceString(
                        plural ?
                            "common.unit.time.msec" :
                            "common.unit.time.msec.single");
            case TimeConvertor.UNIT_SEC:
                return
                    SamUtil.getResourceString(
                        plural ?
                            "common.unit.time.sec" :
                            "common.unit.time.sec.single");
            case TimeConvertor.UNIT_MIN:
                return
                    SamUtil.getResourceString(
                        plural ?
                            "common.unit.time.min" :
                            "common.unit.time.min.single");
            case TimeConvertor.UNIT_HR:
                return
                    SamUtil.getResourceString(
                        plural ?
                            "common.unit.time.hr" :
                            "common.unit.time.hr.single");
            case TimeConvertor.UNIT_DAY:
                return
                    SamUtil.getResourceString(
                        plural ?
                            "common.unit.time.day" :
                            "common.unit.time.day.single");
            default:
                return "";
        }
    }

    // method to retrieve the unit size String from integer
    public static String getSizeUnitString(int unit) {
        switch (unit) {
            case SamQFSSystemModel.SIZE_B:
                return "b";
            case SamQFSSystemModel.SIZE_KB:
                return "kb";
            case SamQFSSystemModel.SIZE_MB:
                return "mb";
            case SamQFSSystemModel.SIZE_GB:
                return "gb";
            case SamQFSSystemModel.SIZE_TB:
                return "tb";
            case SamQFSSystemModel.SIZE_PB:
                return "pb";
            default:
                return "";
        }
    }

    // method to retrieve the size unit from string to integet
    public static int getSizeUnit(String unitString) {
        int unit = -1;
        if (unitString.equals("b")) {
            unit = SamQFSSystemModel.SIZE_B;
        } else if (unitString.equals("kb")) {
            unit = SamQFSSystemModel.SIZE_KB;
        } else if (unitString.equals("mb")) {
            unit = SamQFSSystemModel.SIZE_MB;
        } else if (unitString.equals("gb")) {
            unit = SamQFSSystemModel.SIZE_GB;
        } else if (unitString.equals("tb")) {
            unit = SamQFSSystemModel.SIZE_TB;
        } else if (unitString.equals("pb")) {
            unit = SamQFSSystemModel.SIZE_PB;
        }

        return unit;
    }

    // Method to convert a Calendar object to a string
    public static String getTimeString(Calendar calendar) {
        DateFormat formatter = getTimeFormat();
        String localedString = "";

        if (calendar != null) {
            localedString = formatter.format(calendar.getTime());
        }
        return localedString;
    }

    public static String getTimeString(long timeInSecs) {
        GregorianCalendar time = new GregorianCalendar();
        time.setTimeInMillis(timeInSecs * 1000);

        return getTimeString(time);
    }

    public static String getTimeString(Date date) {
        return getTimeFormat().format(date);
    }

    /**
     * Returns a DateFormat object for formatting date/times using the locale
     * @return
     */
    public static DateFormat getTimeFormat() {
        return DateFormat.getDateTimeInstance(DateFormat.LONG,
                                              DateFormat.MEDIUM,
                                              getCurrentRequest().getLocale());
    }

    /**
     *  Method to return the Alarm type and Alarm count
     *  return a String[] alarm to hold the information of
     *  alarmType, alarm[0], value: Critical/Major/Minor
     *  alarmCount, alarm[1], the number of most critical alarm.
     */
    public static int [] getAlarmInfo(Alarm[] alarms)
        throws SamFSException {
        int criticalCount = 0, majorCount = 0, minorCount = 0;

        int [] result = new int[2];
        if (alarms != null) {
            for (int j = 0; j < alarms.length; j++) {
                int severity = alarms[j].getSeverity();
                switch (severity) {
                    case Alarm.CRITICAL:
                        criticalCount++;
                        break;
                    case Alarm.MAJOR:
                        majorCount++;
                        break;
                    case Alarm.MINOR:
                        minorCount++;
                        break;
                }
            }

            if (criticalCount != 0) {
                result[0] = ServerUtil.ALARM_CRITICAL;
                result[1] = criticalCount;
            } else if (majorCount != 0) {
                result[0] = ServerUtil.ALARM_MAJOR;
                result[1] = majorCount;
            } else if (minorCount != 0) {
                result[0] = ServerUtil.ALARM_MINOR;
                result[1] = minorCount;
            }
        }

        return result;
    }

    /**
     * Populate summary and detail information of an error alert in a page
     * @param view - the containerview of which the alert is set
     * @param varName -
     * @param errMsg -
     * @param origErrorCode -
     * @param origErrorMsg -
     * @param serverName - the name of the server of which is operating on
     * @return return none
     */
    public static void setErrorAlert(
        ContainerView view,
        String varName,
        String errMsg,
        int origErrorCode,
        String origErrorMsg,
        String serverName) {

        CCAlertInline alert = (CCAlertInline) view.getChild(varName);

        boolean alertInfoPresent = false;
        boolean alertErrorPresent = false;
        String detailMsg = "";

        // see if there is already a successful alert message in the
        // CCAlert component, but need to check if there is a message
        // in there as well as Lockhart defaults over to
        // CCAlertInline.TYPE_INFO when calling getValue()

        String alertValue = (String) alert.getValue();
        detailMsg = alert.getDetail();

        if (alertValue != null) {
            if ((alertValue.equals(CCAlertInline.TYPE_INFO) ||
                 alertValue.equals(CCAlertInline.TYPE_WARNING))
                 && detailMsg != null) {
                TraceUtil.trace3("VALUE IS CCAlertInline.TYPE_INFO");
                alertInfoPresent = true;
                alert.setSummary("success.summary");
            }

            // stack up the errors
            if (alertValue.equals(CCAlertInline.TYPE_ERROR) &&
                detailMsg != null) {
                alertErrorPresent = true;
            }
        }

        alert.setValue(CCAlertInline.TYPE_ERROR);

        if (!alertInfoPresent) {
            alert.setSummary(errMsg);
        }

        String displayErrMsg = "";

        if (origErrorMsg == null)  {
            // GUI defined error message, so the alert 'detail'
            // will be based on the origErrorCode

            // using these numbers right now just for testing
            if (origErrorCode <= -1000 && origErrorCode >= -3000)  {
                // Set -2800 (isDown is true) to the same error message
                // shown in Error ViewBean
                // Set -2803 (isAccessDenied is true) to the same error message
                // shown in Error ViewBean
                if (origErrorCode == -2800) {
                    displayErrMsg = getServerDownMessage();
                } else if (origErrorCode == -2803) {
                    displayErrMsg = getAccessDeniedMessage();
                } else {
                    displayErrMsg = getResourceStringError(
                        Integer.toString(origErrorCode)).toString();
                }
            } else {
                displayErrMsg = "";
            }
        } else {
            // just set the detail to the message that comes back
            // TODO: need to check if this is a logic error
            displayErrMsg = origErrorMsg;
        }

        // Override Error messages
        // 30806 => Timeout (Long/Hung API)
        // 30807 => Network Down

        if (origErrorCode == 30806) {
            doPrint("Error code in setErrorAlert is 30806");
            displayErrMsg = getResourceStringError("-2801");
        } else if (origErrorCode == 30807) {
            doPrint("Error code in setErrorAlert is 30807");
            displayErrMsg = getResourceStringError("-2802");
        }

        SamUtil.doPrint(new StringBuffer().append(
            "displayErrMsg is ").append(displayErrMsg).toString());
        SamUtil.doPrint(new StringBuffer().append(
            "detailMsg is ").append(detailMsg).toString());

        if (alertInfoPresent) {
            alert.setValue(CCAlertInline.TYPE_WARNING);

            // see if detailMsg already contains part of displayErrMsg,
            // only display the detailMsg
            if (detailMsg.indexOf(displayErrMsg) != -1) {
                TraceUtil.trace2(
                    "Not concatenating infoPresent in setErrorAlert");
                alert.setDetail(detailMsg);  // don't concatenate
            } else {
                TraceUtil.trace2("Concatenating infoPresent in setErrorAlert");
                alert.setDetail(detailMsg.concat("<br>").concat(displayErrMsg));
            }
        } else if (alertErrorPresent && origErrorCode != -2800 &&
            origErrorCode != -2400) {
            // see if detailMsg already contains displayErrMsg
            // if it does, then don't concatenate displayErrMsg

            if (detailMsg.indexOf(displayErrMsg) != -1) {
                doPrint("Not concatenating errorPresent in setErrorAlert");
                alert.setDetail(detailMsg);  // don't concatenate
            } else {
                TraceUtil.trace2("Concatenating errorPresent in setErrorAlert");
                alert.setDetail(displayErrMsg.concat("<br>").concat(detailMsg));
            }
        } else if (origErrorCode == -2800) {
            alert.setDetail("<br>" + displayErrMsg);

        } else if (origErrorCode == -2400) {
            alert.setDetail(displayErrMsg,
                new String[] {SamUtil.getResourceString("masthead.altText")});

        } else {
            alert.setDetail(displayErrMsg);
        }

        // Check if the detail string contains the downMessage
        // provide Server Name while calling setDetail to avoid users from
        // seeing {0} that supposed to be replaced by the server name
        String detailStr = alert.getDetail();

        if (detailStr.indexOf(getServerDownMessage()) != -1 ||
            detailStr.indexOf(getAccessDeniedMessage()) != -1) {
            alert.setDetail(detailStr, new String[] { serverName });
        }
    }

    /**
     * Populate summary and detail information of an info alert in a page
     * @param view - the containerview of which the alert is set
     * @param alertChildName - alert child to display message
     * @param summaryMsg -
     * @param detailMsg -
     * @param serverName - the name of the server of which is operating on
     * @return return none
     */

    public static void setInfoAlert(
        ContainerView view,
        String alertChildName,
        String summaryMsg,
        String detailMsg,
        String serverName) {

        TraceUtil.trace3("Entering setInfoAlert");
        // see if there is an error
        CCAlertInline alert = (CCAlertInline) view.getChild(alertChildName);

        boolean errorAlertPresent = false;
        String prevDetailMsg = "";
        String downMessage = getServerDownMessage();

        // see if there is already a successful alert message in the
        // CCAlert component, but need to check if there is a message
        // in there as well as Lockhart defaults over to
        // CCAlertInline.TYPE_INFO when calling getValue()

        String alertValue = (String) alert.getValue();
        prevDetailMsg = alert.getDetail();

        doPrint(new StringBuffer().append(
            "detailMsg is ").append(detailMsg).toString());
        doPrint(new StringBuffer().append(
            "prevDetailMsg is ").append(prevDetailMsg).toString());

        if (alertValue != null) {
            if (alertValue.equals(CCAlertInline.TYPE_ERROR) &&
                 prevDetailMsg != null) {
                TraceUtil.trace3("FOUND AN ERROR ALERT");
                errorAlertPresent = true;
                // get the detail message
                alert.setValue(CCAlertInline.TYPE_WARNING);
                alert.setSummary("success.summary");
            }
        }

        if (!errorAlertPresent) {
            doPrint("setInfoAlert: no error is present");
            alert.setValue(CCAlertInline.TYPE_INFO);
            alert.setSummary(summaryMsg);
            alert.setDetail(detailMsg);
        } else {
            doPrint("setInfoAlert: error is present");
            if (prevDetailMsg.indexOf(detailMsg) != -1) {
                doPrint("Not concatenating in setInfoAlert");
                alert.setDetail(prevDetailMsg);  // don't concatenate
            } else {
                doPrint("Concatenating in setInfoAlert");
                alert.setDetail(detailMsg.concat("<br>").concat(prevDetailMsg));
            }
        }

        // Check if the detail string contains the downMessage
        // provide Server Name while calling setDetail to avoid users from
        // seeing {0} that supposed to be replaced by the server name
        String detailStr = alert.getDetail();

        if (detailStr.indexOf(downMessage) != -1) {
            alert.setDetail(detailStr, new String[] { serverName });
        }
    }

    public static void setWarningAlert(
        ContainerView view,
        String varName,
        String summaryMsg,
        String detailMsg) {

        TraceUtil.trace3("Entering setWarningAlert");
        CCAlertInline alert = (CCAlertInline) view.getChild(varName);

        boolean errorAlertPresent = false;
        String detailErrMsg = "";

        // see if there is already a error alert message in the
        // CCAlert component, but need to check if there is a message
        // in there as well as Lockhart defaults over to
        // CCAlertInline.TYPE_INFO when calling getValue()

        String alertValue = (String) alert.getValue();
        detailErrMsg = alert.getDetail();
        if (alertValue != null) {
            if (alertValue.equals(CCAlertInline.TYPE_ERROR) &&
                detailErrMsg != null) {
                TraceUtil.trace3("FOUND AN ERROR ALERT");
                errorAlertPresent = true;
            }
        }
        alert.setValue(CCAlertInline.TYPE_WARNING);
        alert.setSummary(summaryMsg);

        String warningValue = getResourceString(detailMsg);
        String displayMsg = "";
        if (warningValue != null && !warningValue.equals("")) {
            displayMsg = warningValue;
        } else {
            displayMsg = detailMsg;
        }

        if (!errorAlertPresent)  {
            alert.setDetail(displayMsg);
        } else {
            if (detailErrMsg.indexOf(displayMsg) != -1) {
                alert.setDetail(detailErrMsg);
            } else {
                alert.setDetail(displayMsg.concat("<br>").concat(detailErrMsg));
            }
        }
    }



    public static CCI18N getCCI18NObj() {
        HttpServletRequest httprq = getCurrentRequest();
        CCI18N i18n = (CCI18N) httprq.getAttribute(Constants.I18N.I18N_OBJECT);

        if (i18n == null) {
            i18n = new CCI18N(httprq,
                              getCurrentResponse(),
                Constants.ResourceProperty.BASE_NAME, null, httprq.getLocale());
            httprq.setAttribute(Constants.I18N.I18N_OBJECT, i18n);
        }

        return i18n;
    }

    public static CCI18N getCCI18NObj(String BaseName, String bundleId) {
        HttpServletRequest httprq = getCurrentRequest();
        CCI18N i18n = (CCI18N) httprq.getAttribute(
                                        Constants.I18N.I18N_OBJECT + bundleId);

        if (i18n == null) {
            i18n = new CCI18N(httprq,
                              getCurrentResponse(),
                              BaseName,
                              bundleId,
                              null);
            httprq.setAttribute(Constants.I18N.I18N_OBJECT + bundleId, i18n);
        }
        return i18n;
    }

    public static String getLockhartTagString(String key) {
        CCI18N i18n = getCCI18NObj(CCI18N.TAGS_BUNDLE, CCI18N.TAGS_BUNDLE_ID);
        return i18n.getMessage(key);
    }

    public static String getResourceStringError(String errorCode) {
        return getResourceString(errorCode);
    }

    /**
     * Gets a localized string from a resource bundle.
     */
    public static String getResourceString(String aString) {
        CCI18N i18n = getCCI18NObj();
        String localedString = "";

        if (i18n != null) {
            localedString = i18n.getMessage(aString);
        }
        return localedString;
    }

    /**
     * Gets a localized string from a resource bundle and inserts the
     * arguments.  Arguments are localized as well.
     */
    public static String getResourceString(
        String key, String[] arguments) {

        CCI18N i18n = getCCI18NObj();
        if (i18n == null) {
            TraceUtil.trace1("c18n object is NULL!");
            return "";
        }

        // First check if the detailInfo is itself a key in the resource bundle
        // If yes, call getResourceStringWithoutL10NArgs.
        for (int i = 0; i < arguments.length; i++) {
            if (!(getResourceString(arguments[i]).equals(arguments[i]))) {
                return getResourceStringWithoutL10NArgs(key, arguments);
            }
        }

        return i18n.getMessage(key, arguments);
    }

    /**
     * Gets a localized string from a resource bundle and inserts the
     * argument.  Argument is localized as well.
     */
    public static String getResourceString(String key, String value) {
        String [] params = null;

        if (value != null && value.length() > 0) {
            params =  new String [] {value};
        } else {
            return getResourceString(key);
        }

        return getResourceString(key, params);
    }

    /**
     * Gets a localized string from a resource bundle and inserts the
     * arguments.  Arguments are NOT localized.
     */
    public static String getResourceStringWithoutL10NArgs(
                                        String key, String[] arguments) {

        CCI18N i18n = getCCI18NObj();
        String localedString = "";

        if (i18n != null) {
            String pattern = i18n.getMessage(key);
            Object [] args = new Object[arguments.length];
            for (int i = 0; i < arguments.length; i++) {
                args[i] = arguments[i];
            }
            localedString = MessageFormat.format(pattern, args);
        }
        return localedString;
    }

    public static void doPrint(String printString) {
        TraceUtil.trace2(printString);
    }

    /**
     * Test if the string contains spaces in between characters
     * return true if it doesn't
     * Pre-req: testString has to be trimmed before passed in here
     */
    public static boolean isValidString(String testString) {
        return testString.indexOf(" ") == -1 ? true : false;
    }

    /**
     * Test if the string is a valid VSN String
     * return true if it is, otherwise false
     */
    public static boolean isValidVSNString(String vsnString) {
        if (vsnString != null) {
            vsnString = vsnString.trim();
        } else {
            return false;
        }

        if (!isValidString(vsnString)) {
            // contain space(s)
            return false;
        } else if (vsnString.length() > 6) {
            // length greater than 6
            return false;
        } else {
            for (int i = 0; i < vsnString.length(); i++) {
                if (!isVSNDigit(vsnString.charAt(i))) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Process the Exceptions that are generated inside the application
     * @param ex - the exception that needs to be processed
     * @param clazz -
     * @param methodName - Name of the method that generates this exception
     * @param message - Back up message when ex.getMessage() returns null
     * @param serverName - the name of the server of which is operating on
     * @return return none
     */
    public static void processException(
        Exception ex,
        Class clazz,
        String methodName,
        String message,
        String serverName) {

        // when the exception is regarding to RPC connection issues,
        // it needs to be handled peacefully.
        if (ex instanceof SamFSException) {
            handleSamFSException((SamFSException)ex, serverName);
        }

        if (ex.getMessage() != null) {
            TraceUtil.trace1(new StringBuffer().append(
                "Exception occurred: ").append(ex.getMessage()).toString());
            LogUtil.error(clazz, methodName, ex.getMessage());
        } else {
            TraceUtil.trace1(new StringBuffer().append(
                "Exception occurred: ").append(message).toString());
            LogUtil.error(clazz, methodName, message);
        }
    }

    /**
     * Helper function of isValidVSNString
     */
    private static boolean isVSNDigit(char c) {
        return (((c >= '0') && (c <= '9')) ||
         ((c >= 'A') && (c <= 'Z')) || (c == '!') ||
         (c == '"') || (c == '%') || (c == '&') ||
         (c == '\'') || (c == '(') || (c == ')') ||
         (c == '*') || (c == '+') || (c == ',') ||
         (c == '-') || (c == '.') || (c == '/') ||
         (c == ':') || (c == ';') || (c == '<') ||
         (c == '=') || (c == '>') || (c == '?') || (c == '_'));
    }

    /**
     * Test if the string is a valid String which contains no special characters
     * return true if it is, otherwise false
     */
    public static boolean isValidNonSpecialCharString(String inputString) {
        if (!isValidString(inputString)) {
            // contain space(s)
            return false;
        } else {
            for (int i = 0; i < inputString.length(); i++) {
                if (isSpecialDigit(inputString.charAt(i))) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Test if the string is a valid String which contains no special characters
     * (EXCLUDE colon, which is valid in access list string)
     * return true if it is, otherwise false
     */
    public static boolean isValidAccessListString(String inputString) {
        if (!isValidString(inputString)) {
            // contain space(s)
            return false;
        } else {
            for (int i = 0; i < inputString.length(); i++) {
                if (isInvalidAccessListChar(inputString.charAt(i))) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Test if the string is a valid email address
     * return true if it is, otherwise false
     */
    public static boolean isValidEmailAddress(String inputString) {
        // Check null
        if (inputString == null) {
            TraceUtil.trace3("isValidEmailAddress: null");
            return false;
        } else {
            inputString = inputString.trim();
        }

        // Checks if email addresses contains spaces or commas
        if (!isValidString(inputString) || inputString.indexOf(",") != -1) {
            TraceUtil.trace3("isValidEmailAddress: spaces or commas");
            return false;
        }

        // Checks for email addresses starting with inappropriate symbols like
        // dots or @ signs, or www.
        Pattern p = Pattern.compile("^\\.|^\\@|^www\\.");
        Matcher m = p.matcher(inputString);
        if (m.find()) {
            TraceUtil.trace3("isValidEmailAddress: start with . or @ or www.");
            return false;
        }

        // Checks if email address contains illegal characters
        for (int i = 0; i < inputString.length(); i++) {
            if (isInvalidEmailAddressChar(inputString.charAt(i))) {
                return false;
            }
        }

        // Checks if the email address contains @.  It is legal to simply type
        // in the username without the domain name.  Or user can use the typical
        // email format name@domain.com or name@domain
        if (inputString.endsWith("@") || inputString.endsWith(".")) {
            TraceUtil.trace3("isValidEmailAddress: ends with @.");
            return false;
        } else if (inputString.indexOf("@") != -1) {
            // Check if input string has 2 or more @
            if (inputString.indexOf("@") != inputString.lastIndexOf("@")) {
                TraceUtil.trace3("isValidEmailAddress: 2 or more @ found!");
                return false;
            }

            Pattern p1 = Pattern.compile(".+@.+\\.[a-z|A-Z|0-9]+");
            Pattern p2 = Pattern.compile(".+@[a-z|A-Z|0-9]+");
            Matcher m1 = p1.matcher(inputString);
            Matcher m2 = p2.matcher(inputString);

            // error if string does not match both patterns
            if (!m1.find() && !m2.find()) {
                TraceUtil.trace3(
                    "isValidEmailAddress: does not match abc@company.com!");
                return false;
            }

            // Check if the email address user name and domain name starts with
            // non alphanumerics e.g. --@--.com
            String [] nameArray = inputString.split("@");
            for (int i = 0; i < nameArray.length; i++) {
                if (nameArray[i].length() > 1) {
                    char startChar = nameArray[i].charAt(0);
                    boolean error =
                        isInvalidEmailAddressChar(startChar) ||
                        startChar == '-';
                    if (error) {
                        TraceUtil.trace3(
                            "isValidEmailAddress: illegal first character!");
                    }
                }
            }
        }

        return true;
    }

    /**
     * Helper function of isInvalidNonSpecialCharString
     */
    private static boolean isSpecialDigit(char c) {
        return ((c == '#') || (c == '$') ||
         (c == '!') || (c == '\\') || (c == '@') ||
         (c == '"') || (c == '%') || (c == '&') ||
         (c == '\'') || (c == '(') || (c == ')') ||
         (c == '*') || (c == '+') || (c == ',') ||
         (c == ':') || (c == ';') || (c == '<') ||
         (c == '=') || (c == '>') || (c == '?'));
    }

    /**
     * Helper function of isInvalidAccessListString
     */
    private static boolean isInvalidAccessListChar(char c) {
        return ((c == '#') || (c == '$') ||
         (c == '!') || (c == '\\') || (c == '@') ||
         (c == '"') || (c == '%') || (c == '&') ||
         (c == '\'') || (c == '(') || (c == ')') ||
         (c == '*') || (c == '+') || (c == ',') ||
         (c == ';') || (c == '<') || (c == '/') ||
         (c == '=') || (c == '>') || (c == '?'));
    }

    /**
     * Helper function of isValidEmailAddress
     */
    private static boolean isInvalidEmailAddressChar(char c) {
        return ((c == '#') || (c == '$') ||
         (c == '!') || (c == '\\') ||
         (c == '"') || (c == '%') || (c == '&') ||
         (c == '\'') || (c == '(') || (c == ')') ||
         (c == '*') || (c == '+') || (c == ',') ||
         (c == ';') || (c == '<') || (c == '/') ||
         (c == '=') || (c == '>') || (c == '?') ||
         (c == '`') || (c == '~') || (c == ':'));
    }


    /**
     * Process the Exceptions that are generated inside the application
     * @param ex - the exception that needs to be processed
     * @param clazz -
     * @param methodName - Name of the method that generates this exception
     * @param serverName - the name of the server of which is operating on
     * @return return none
     */
    public static void handleSamFSException(
        SamFSException samEx, String serverName) {
        // this is in a single try-catch block by design.
        // this method is for handling two particular type of exceptions.
        // if exception occurs while handling exceptions, there is not
        // much that can be done about it.

        doPrint(new StringBuffer().append(
            "Entering: handleSamFSException(): errCode is: ").append(
            samEx.getSAMerrno()).toString());

        try {
            SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();

            doPrint(new StringBuffer().append("serverName is ").
                append(serverName).toString());

            SamQFSSystemModel model = appModel.getSamQFSSystemModel(serverName);

            // If the exception is a type of the communication or
            // NotInit Exception, or its errCode is -2800 (SamUtil.getModel())
            // Do the appropriate thing
            // No need to do anything for SamFSTimeoutException

            if (samEx instanceof SamFSCommException ||
                samEx.getSAMerrno() == -2800) {

                doPrint("Exception is a type of SamFSComm");
                doPrint("or simply marked as DOWN");

                // 30804 is a type of SamFSCommException but it does not need
                // to destroy and establish a new connection.  All it needs is
                // to reinitialize the connection.

                if (model != null) {
                    if (samEx.getSAMerrno() == 30804) {
                        doPrint("Attempt to reinitialize ...");
                        model.reinitialize();

                    } else if (samEx instanceof SamFSCommException ||
                        samEx.getSAMerrno() == -2800) {
                        if (!(samEx instanceof SamFSAccessDeniedException)) {
                            doPrint("Attempt to reconnect ...");
                            model.reconnect();
                        }
                    }
                }
            }
        } catch (Exception e) {
            // not much can be done at this point
        }
    }


    public static boolean isValidLetterOrDigitString(String s) {
        char [] c = s.toCharArray();

        for (int i = 0; i < c.length; i++) {
            if (!Character.isLetterOrDigit(c[i]) && c[i] != '_') {
                return false;
            }
        }
        return true;
    }

    public static String concatErrMsg(String [] messages) {
        String retMessage = "";
        if (messages == null) {
            return "";
        }
        for (int i = 0; i < messages.length; i++) {
            retMessage = retMessage.concat(messages[i]).concat("<br>");
        }
        return retMessage;
    }

    public static String getMultiMsgInfoError() {
        return new StringBuffer()
            .append(getResourceString("ArchiveConfig.info.save"))
            .append("\n")
            .append(getResourceString("ArchiveConfig.error")).toString();
    }

    /**
     * replaceSpaceWithUnderscore(String)
     *
     * Usage:
     *  Helper function for Add Library Wizard
     *  Library name cannot contain space(s)
     */

    public static String replaceSpaceWithUnderscore(String myString) {
        // Trim leading and trailing spaces first
        myString = myString.trim();

        // Check to see if there are spaces in the string,
        // replace each space with a "_" (underscore)

        myString = myString.replace(' ', '_');
        return myString;
    }

    public static String processJobDescription(String [] s) {
        boolean flag = false;
        String result = "";
        String comma  = ",";
        for (int i = 0; i < s.length; i++) {
            if (s[i] != null && !s[i].equals("")) {
                if (i == 0) {
                    flag = true;
                    result = s[i];
                } else {
                    if (flag) {
                        result = result.concat(comma).concat(s[i]);
                    } else {
                        result = result.concat(s[i]);
                    }
                    flag = true;
                }
            }
        }
        return result;
    }

    public static String createBlankPageTitleXML() {
        return new StringBuffer().append(
            "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n" +
            "<!DOCTYPE pagetitle SYSTEM \"tags/dtd/pagetitle.dtd\">\n" +
            "\n").append("<pagetitle>\n").append("</pagetitle>\n").toString();
    }


    public static String handleMultiHostException(
        SamFSMultiHostException multiEx) {
        String err_msg = "";
        doPrint(new StringBuffer().append(
            "Entering: handleMultiHostException(): errCode is: ").append(
            multiEx.getSAMerrno()).toString());

        err_msg += getResourceString(multiEx.getMessage());
        SamFSException[] exceptions = multiEx.getExceptions();
        if (exceptions == null || exceptions.length == 0) {
            return err_msg;
        }
        err_msg += "\n<br>\n<br>";
        String[] hosts = multiEx.getHostNames();

        for (int i = 0; i < exceptions.length; i++) {
            err_msg += hosts[i] + ": " +
                getResourceString(exceptions[i].getMessage()) + "\n<br>";
            SamUtil.doPrint(new StringBuffer().
                append("exception hostName is ").
                append(hosts[i]).toString());
            SamUtil.doPrint(new StringBuffer().
                append("exception info is ").
                append(exceptions[i]).toString());
            if (multiEx instanceof SamFSException) {
                handleSamFSException((SamFSException)multiEx, hosts[i]);
            }
            try {
                SamQFSAppModel appModel = SamQFSFactory.getSamQFSAppModel();
                SamQFSSystemModel model =
                   appModel.getSamQFSSystemModel(hosts[i]);

                // If the exception is a type of the communication or
                // its errCode is -2800 (SamUtil.getModel())
                // Do the appropriate thing
                // No need to do anything for SamFSTimeoutException

                if (exceptions[i] instanceof SamFSCommException ||
                    exceptions[i].getSAMerrno() == -2800) {

                    SamUtil.doPrint(
                        "Exception is a type of SamFSComm ");
                    SamUtil.doPrint("or simply marked as DOWN");

                    // 30804 is a type of SamFSCommException but
                    // it does not need
                    // to destroy and establish a new connection.
                    // All it needs is
                    // to reinitialize the connection.

                    if (model != null) {
                        if (exceptions[i].getSAMerrno() == 30804) {
                                SamUtil.doPrint("Attempt to reinitialize ...");
                                model.reinitialize();

                        } else if (exceptions[i]
                            instanceof SamFSCommException ||
                            exceptions[i].getSAMerrno() == -2800) {
                                SamUtil.doPrint("Attempt to reconnect ...");
                                model.reconnect();
                        }
                    }
                }
            } catch (Exception ex) {
                              // not much can be done at this point
            }
        }
        return err_msg;
    }

    public static boolean isValidFSNameString(String inputString) {
        if (!isValidString(inputString)) {
            // contain space(s)
            return false;
        } else {
            char [] c = inputString.toCharArray();
            if (Character.isDigit(c[0])) {
                return false;
            }
            for (int i = 0; i < c.length; i++) {
                if (!Character.isLetterOrDigit(c[i])) {
                    if (c[i] != '_' && c[i] != '.' && c[i] != '-') {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    /**
     * a utility function to test tbe well-formedness of a path.
     * NOTE: this method doesn't actually check whether the path exists or not,
     * it merely tests its well-formedness
     *
     * @param path - the candidate path
     * @return boolean - true if path is well-formed, otherwise false
     */
    public static boolean isWellFormedPath(String path) {
        if (path == null || path.length() == 0)
            return false;
        // trim any spaces
        path = path.trim();
        // break the path down into its component directories and test their
        // individual well-formedness
        StringTokenizer st = new StringTokenizer(path, "/");
        while (st.hasMoreTokens()) {
            String subdir = st.nextToken();
            if (!isValidFSNameString(subdir))
                return false;
        }
        // if we get this far, the path is well-formed
        return true;
    }

    /**
     * Concatenates two strings together to build a file path.  If addToPath
     * does not end in a "/", than one is concatenated.  fileOrDir is then
     * added to addToPath.  Note that the addToPath parameter object
     * is modified, but the fileOrDir object is not.
     *
     *
     * @param addToPath Path you want to add to.
     * @param fileOrDir Path you want to add.
     *
     */
    public static void buildPath(StringBuffer addToPath,
                                 StringBuffer fileOrDir) {
        if (addToPath == null || fileOrDir == null) {
            return;
        }
        if (addToPath.charAt(addToPath.length() - 1) != '/') {
            addToPath.append("/");
        }
        addToPath.append(fileOrDir);
        return;
    }

    /**
     * Concatenates two strings together to build a file path.  If addToPath
     * does not end in a "/", than one is concatenated.  fileOrDir is then
     * added to addToPath.  The resulting string is returned.
     *
     *
     * @param addToPath Path you want to add to.
     * @param fileOrDir Path you want to add.
     *
     * @return Returns the new path.
     */
    public static String buildPath(String addToPath, String fileOrDir) {
        if (addToPath == null || fileOrDir == null) {
            return null;
        }
        String retPath = addToPath;
        if (addToPath.charAt(addToPath.length() - 1) != '/') {
            retPath = retPath + "/";
        }
        retPath = retPath + fileOrDir;
        return retPath;
    }

    /**
     * Get the system type of a SAM-QFS server
     * return: SystemLicense.QFS / SystemLicense.SAMFS / SystemLicense.SAMQFS
     */
    public static short getSystemType(String hostName) {

        short license = SamQFSSystemModel.SAMQFS;
        try {
            ServerInfo serverInfo = getServerInfo(hostName);
            if (serverInfo != null) {
                license = serverInfo.getServerLicenseType();
            }
        } catch (SamFSException samEx) {
            TraceUtil.trace1(new StringBuffer(
                "Exception caught while retreiving system type. Reason: ").
                append(samEx.getMessage()).toString());
        }

        return license;
    }

    /**
     * Swap i386 string with AMD64-x86
     */
    public static String swapArchString(String archString) {
        if (archString.equals("i386")) {
            return getResourceString("samqfsui.x86");
        } else {
            // if it's sparc, return it
            return archString;
        }
    }

    /**
     * Retrieve appropriate server down String based on server version
     */
    public static String getServerDownMessage() {
        return getResourceStringError(
            "ErrorHandle.alertElementFailedDetail2");
    }


    /**
     * Retreive appropriate access denied error message based on server version
     */
    public static String getAccessDeniedMessage() {
        return getResourceStringError(
            "ErrorHandle.accessDeniedDetail");
    }

    /**
     * retrieve ServerInfo array from session
     * If serverName is empty, retrieve server name from session for 4.3 servers
     * If server is not in the table, create an entry and insert to the table
     * If the hashtable is not found in session, create it and insert the
     * server info into the hashtable.
     */
    public static ServerInfo getServerInfo(String serverName)
        throws SamFSException {
        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        Hashtable serverTable = getServerTable();

        ServerInfo serverInfo = (ServerInfo) serverTable.get(serverName);
        if (serverInfo == null) {
            serverInfo = createServerInfoObject(serverName);
            serverTable.put(serverName, serverInfo);

            // Update the hashtable in session when needed
            session.setAttribute(
                Constants.SessionAttributes.SAMFS_SERVER_INFO, serverTable);
        }

        return serverInfo;
    }

    /**
     * Helper function to retrieve server table
     */
    public static Hashtable getServerTable() {
        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        Hashtable serverTable = (Hashtable) session.getAttribute(
            Constants.SessionAttributes.SAMFS_SERVER_INFO);
        if (serverTable == null) {
            serverTable = new Hashtable();
            session.setAttribute(
                Constants.SessionAttributes.SAMFS_SERVER_INFO, serverTable);
        }
        return serverTable;
    }

    /**
     * Helper function to create a serverInfo object given the serverName
     */
    private static ServerInfo createServerInfoObject(String serverName)
        throws SamFSException {
        ServerInfo serverInfo = new ServerInfo(serverName);
        SamQFSSystemModel model = getModel(serverName);
        serverInfo.setSamfsServerAPIVersion(model.getServerAPIVersion());
        serverInfo.setServerLicenseType(model.getLicenseType());
        return serverInfo;
    }

    /**
     * Converts a delimited list of items to a tree set.
     *
     * @param theString If null, the method returns an empty tree set.
     * @param delimiter
     */
    public static TreeSet delimitedStringToTreeSet(String theString,
                                                   String delimiter) {
        if (theString == null || theString == "") {
            return new TreeSet();
        }
        if (delimiter == null) {
            delimiter = "";
        }

        String [] strArray = theString.split(delimiter);
        return new TreeSet(Arrays.asList(strArray));
    }

    // Convert an array of strings to one string.
    public static String array2String(String[] strArr, String delim) {
        StringBuffer result = new StringBuffer();
        if (strArr.length > 0) {
            result.append(strArr[0]);
            for (int i = 1; i < strArr.length; i++) {
                result.append("\n");
                result.append(strArr[i]);
            }
        }
        return result.toString();
    }

    /**
     * Make the text red in color.  Mainly used in the monitoring module.
     * The statictextfield must have escape sets to false.
     */
    public static String makeRed(String inputStr) {
        if (inputStr == null || inputStr.length() == 0) {
            return "";
        } else {
            return "<font color='#FF0000'><b>".
                        concat(inputStr).concat("</b></font>");
        }
    }

    /**
     * Construct Image Bar
     * percentage has to be in between 0 to 100
     */
    public static String getImageBar(boolean red, int percentage) {
        String color =
            red ?
                Constants.Image.RED_USAGE_BAR_DIR :
                Constants.Image.USAGE_BAR_DIR;
        return color.concat(Integer.toString(percentage)).concat(".gif");
    }

    /**
     * Return current time string (Used in Monitoring console)
     */
    public static String getCurrentTimeString() {
        return getTimeString(new GregorianCalendar());
    }

    public static String getStatusString(long status) {
        StringBuffer buf  = new StringBuffer();
        StringBuffer newLine = new StringBuffer("<br>");

        if ((status & CatEntry.CES_bad_media) == CatEntry.CES_bad_media) {
            buf.append(SamUtil.getResourceString("EditVSN.damagemedia"));
        }
        if ((status & CatEntry.CES_dupvsn) == CatEntry.CES_dupvsn) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.duplicatevsn"));
        }
        if ((status & CatEntry.CES_read_only) == CatEntry.CES_read_only) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.readonly"));
        }
        if ((status & CatEntry.CES_writeprotect) == CatEntry.CES_writeprotect) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.writeprotected"));
        }
        if ((status & CatEntry.CES_non_sam) == CatEntry.CES_non_sam) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.foreignmedia"));
        }
        if ((status & CatEntry.CES_recycle) == CatEntry.CES_recycle) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.recycle"));
        }
        if ((status & CatEntry.CES_archfull) == CatEntry.CES_archfull) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.volumefull"));
        }
        if ((status & CatEntry.CES_unavail) == CatEntry.CES_unavail) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.unavailable"));
        }
        if ((status & CatEntry.CES_needs_audit) == CatEntry.CES_needs_audit) {
            if (buf.length() > 0) buf.append(newLine);
            buf.append(SamUtil.getResourceString("EditVSN.needaudit"));
        }

        return buf.toString();
    }

    /**
     * To return the RecoveryPointSchedule object for a file system
     * @param sysModel - System Model of the server
     * @param fsName   - the file system name of which its recovery point
     *  schedule is about to return
     * @throws SamFSException - when any error situations occur
     * @return - The Recovery Point Schedule object of the given file system
     * @since 4.6
     */
    public static RecoveryPointSchedule getRecoveryPointSchedule(
        SamQFSSystemModel sysModel, String fsName) throws SamFSException {
        SamQFSSystemAdminManager adminManager =
            sysModel.getSamQFSSystemAdminManager();

        Schedule [] schedules = adminManager.getSpecificTasks(
                                    ScheduleTaskID.SNAPSHOT, fsName);
        if (schedules == null || schedules.length == 0) {
            return null;
        } else {
            // There is only one snapshot schedule allowed per file system
            return (RecoveryPointSchedule) schedules[0];
        }
    }

    /**
     * To construct a string that describes the time interval that the schedule
     * is going to execute.
     *
     * Repeat every {0} {1}
     * This method is used only for 4.6+ servers.
     */
    public static String getSchedulePeriodicString(long period, int unit) {
        return getResourceString(
            "common.schedule.repeatmessage",
            new String [] {
                Long.toString(period),
                getTimeUnitL10NString(unit)});
    }

    /**
     * @param serverName The server in which the file system is last selected
     * @param fsName The file system name of which is last selected
     * @return success or not
     *
     * To set the last selected file system name by the user.
     * The information is going to be saved in the session in a HashMap.
     * The key of the hash map is the serverName, while the value is the
     * last selected file system name.
     *
     * The session attribute label is
     * Constants.SessionAttribute.LAST_SELECTED_FS_NAME
     *
     * This method returns a boolean.  True means attribute is saved
     * successfully, while false means the setter fails.
     */
    public static boolean setLastUsedFSName(String serverName, String fsName) {
        if (serverName == null || fsName == null ||
            serverName.length() == 0 || fsName.length() == 0) {
            return false;
        }
        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        HashMap myMap = (HashMap) session.getAttribute(
                            Constants.SessionAttributes.LAST_SELECTED_FS_NAME);
        if (myMap == null) {
            myMap = new HashMap();
        }
        myMap.put(serverName, fsName);
        session.setAttribute(
            Constants.SessionAttributes.LAST_SELECTED_FS_NAME, myMap);
        return true;
    }

    /**
     * @serverName Server of which the last file system is selected
     * @return the last used file system name of a given server
     *
     * To get the last selected file system name by the user.
     * Return the file system name that is used by the user when managing
     * a particular server.  Return null if no file system has been selected.
     */
    public static String getLastUsedFSName(String serverName) {
        if (serverName == null || serverName.length() == 0) {
            return null;
        }
        HttpSession session =
            RequestManager.getRequestContext().getRequest().getSession();
        HashMap myMap = (HashMap) session.getAttribute(
                            Constants.SessionAttributes.LAST_SELECTED_FS_NAME);
        if (myMap == null) {
            myMap = new HashMap();
            session.setAttribute(
                Constants.SessionAttributes.LAST_SELECTED_FS_NAME, myMap);
            return null;
        }
        return (String) myMap.get(serverName);
    }

    /**
     * Retrieve the <code>HttpServletRequest</code> object for the request
     * currently being processed. NOTE: To get JATO and JSF based pages to
     * cohabit, it will be necessary to change all methods that use the JATO
     * utililty class <code>RequestManager</code>.
     * 
     * @return - <code>HttpServletRequest</code>
     */
    public static HttpServletRequest getCurrentRequest() {
        HttpServletRequest request = null;
        
        // check if the request is coming from a JSF page
        FacesContext fcontext = FacesContext.getCurrentInstance();
        if (fcontext != null) {
            ExternalContext econtext = fcontext.getExternalContext();
            if (econtext != null) {
                request = (HttpServletRequest)econtext.getRequest();
            }
        }
        
        if (request == null) { // must be a JATO request
            RequestContext cxt = RequestManager.getRequestContext();
            if (cxt != null) {
                request = cxt.getRequest();
            }
        }
        return request;
    }
    
    /** 
     * Get the <code>HttpServletResponse</code> object for the current request.
     * This is necessary allow both JATO and JSF-based pages to co-exist and
     * utilize the same utility methods in this class.
     */
    public static HttpServletResponse getCurrentResponse() {
        HttpServletResponse response = null;
        
        // is this JSF response?
        FacesContext fcontext = FacesContext.getCurrentInstance();
        if (fcontext != null) {
            ExternalContext econtext = fcontext.getExternalContext();
            
            if (econtext != null) {
                response = (HttpServletResponse)econtext.getResponse();
            }
        }
        
        if (response == null) { // must be a JATO response 
            RequestContext rcontext = RequestManager.getRequestContext();
            if (rcontext != null) {
                response = RequestManager.getResponse();
            }
        }
        
        return response;
    }
}
