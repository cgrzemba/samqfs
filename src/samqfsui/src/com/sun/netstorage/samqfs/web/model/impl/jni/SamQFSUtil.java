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

// ident	$Id: SamQFSUtil.java,v 1.55 2008/03/17 14:43:46 am143972 Exp $

package com.sun.netstorage.samqfs.web.model.impl.jni;

import com.sun.netstorage.samqfs.mgmt.BaseDev;
import com.sun.netstorage.samqfs.mgmt.Ctx;
import com.sun.netstorage.samqfs.mgmt.SamFSException;
import com.sun.netstorage.samqfs.mgmt.arc.ArFSDirective;
import com.sun.netstorage.samqfs.mgmt.arc.CopyParams;
import com.sun.netstorage.samqfs.mgmt.arc.Criteria;
import com.sun.netstorage.samqfs.mgmt.arc.DiskVol;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemMediaManager;
import com.sun.netstorage.samqfs.web.model.SamQFSSystemModel;
import com.sun.netstorage.samqfs.web.model.archive.ArchivePolicy;
import com.sun.netstorage.samqfs.web.model.archive.GlobalArchiveDirective;
import com.sun.netstorage.samqfs.web.model.media.BaseDevice;
import com.sun.netstorage.samqfs.web.model.media.DiskCache;
import com.sun.netstorage.samqfs.web.model.media.Library;
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.GregorianCalendar;
import java.util.StringTokenizer;


public class SamQFSUtil {
    /* bytes in a SIZE_<unit> */
    private static final long SIZE_KB = (long) 1024;
    private static final long SIZE_MB = (long) 1024 * SIZE_KB;
    private static final long SIZE_GB = (long) 1024 * SIZE_MB;
    private static final long SIZE_TB = (long) 1024 * SIZE_GB;
    private static final long SIZE_PB = (long) 1024 * SIZE_TB;

    private static final String TIME_UNITS = "smhdw"; // sec, min, hour, day
    private static final int[]  TIME_BASE  = { 60, 60, 24, 7 };

    private static final String SIZE_UNITS = "BKMGTP";
    private static final BigInteger SIZE_BASE = new BigInteger("1024");


    public static String createVSNExpressionFromStrings(String[] strs) {
        String exp = null;

        if ((strs != null) && (strs.length > 0)) {
            StringBuffer buf = new StringBuffer(strs[0]);
            for (int i = 1; i < strs.length; i++)
                buf.append("," + strs[i]);
            exp = buf.toString();
        }

        return exp;
    }

    public static String[] getStringsFromCommaStream(String str) {
        String [] result = null;

        if (str != null && str.length() > 0) {
            result = str.split(",");
        }

        return result;
    }


    public static String getCriteriaName(String nameWithDot) {
        String name = null;

        if (nameWithDot != null) {
            StringTokenizer st = new StringTokenizer(nameWithDot, ".");
            name = st.nextToken(); // the first token is the name
        }

        return name;

    }


    /**
     * Parse the input value to extract the number that follows the period.
     * @param nameWithDot Input value to be parsed.  Expected format is:
     *  "copyName.X", where X = 1, 2, 3, or 4
     * @return -1 on error, 1, 2, 3, or 4 on success
     */
    public static int getCopyNumber(String nameWithDot) {

        int copyNo = -1;
        if (nameWithDot != null) {
            int index = nameWithDot.lastIndexOf('.');
            if (index != -1) {
                try {
                    copyNo = Integer.parseInt(nameWithDot.substring(index + 1));
                } catch (NumberFormatException e) {
                }
                if (copyNo < 1 || copyNo > 4) {
                    copyNo = -1;
                }
            }
        }

        return copyNo;
    }

    public static String createExpression(String startVSN, String endVSN) {

        String s = null;
        if ((isValidString(startVSN)) && (!isValidString(endVSN))) {
            s = startVSN;
        } else if ((!isValidString(startVSN)) && (isValidString(endVSN))) {
            s = endVSN;
        } else if ((isValidString(startVSN)) && (isValidString(endVSN))) {
            s = RE.convertToREsString(startVSN, endVSN);
            if (!isValidString(s)) {
                // situation: startVSN: xyz000, endVSN: xyzm00
                s = startVSN + " " + endVSN;
            }
        }

        return s;

    }


    /**
     *  Convert srcSize from srcUnit to targetUnit, ie from MB to GB.
     *
     *  @param srcSize original value
     *  @param srcUnit One of the following:
     *   SamQFSSystemModel.SIZE_B, SamQFSSystemModel.SIZE_KB,
     *   SamQFSSystemModel.SIZE_MB, SamQFSSystemModel.SIZE_GB,
     *   SamQFSSystemModel.SIZE_TB, SamQFSSystemModel.SIZE_PB
     *  @param targetUnit One of the following:
     *   SamQFSSystemModel.SIZE_KB,
     *   SamQFSSystemModel.SIZE_MB, SamQFSSystemModel.SIZE_GB,
     *   SamQFSSystemModel.SIZE_TB, SamQFSSystemModel.SIZE_PB
     *  @return srcSize converted to targetUnit
     */
    public static long convertSize(long srcSize,
                                   int srcUnit, int targetUnit) {

        long sizeInBytes = getSizeInBytes(srcSize, srcUnit);
        long targetSize = -1;
        if (sizeInBytes >= 0) {
            switch (targetUnit) {

            case SamQFSSystemModel.SIZE_KB:
                targetSize = sizeInBytes / SamQFSUtil.SIZE_KB;
                break;

            case SamQFSSystemModel.SIZE_MB:
                targetSize =  sizeInBytes / SamQFSUtil.SIZE_MB;
                break;

            case SamQFSSystemModel.SIZE_GB:
                targetSize = sizeInBytes / SamQFSUtil.SIZE_GB;
                break;

            case SamQFSSystemModel.SIZE_TB:
                targetSize = sizeInBytes / SamQFSUtil.SIZE_TB;
                break;

            case SamQFSSystemModel.SIZE_PB:
                targetSize = sizeInBytes / SamQFSUtil.SIZE_PB;
                break;

            }
        }

        return targetSize;

    }


    /**
     *  Convert srcSize from srcUnit to bytes.
     *
     *  @param srcSize original value
     *  @param srcUnit One of the following:
     *   SamQFSSystemModel.SIZE_B, SamQFSSystemModel.SIZE_KB,
     *   SamQFSSystemModel.SIZE_MB, SamQFSSystemModel.SIZE_GB,
     *   SamQFSSystemModel.SIZE_TB, SamQFSSystemModel.SIZE_PB
     *  @return srcSize converted to bytes.
     */
    public static long getSizeInBytes(long srcSize, int srcUnit) {

        long size = -1;

        switch (srcUnit) {

        case SamQFSSystemModel.SIZE_B:
            size = srcSize;
            break;

        case SamQFSSystemModel.SIZE_KB:
            size = srcSize * SamQFSUtil.SIZE_KB;
            break;

        case SamQFSSystemModel.SIZE_MB:
            size =  srcSize * SamQFSUtil.SIZE_MB;
            break;

        case SamQFSSystemModel.SIZE_GB:
            size = srcSize * SamQFSUtil.SIZE_GB;
            break;

        case SamQFSSystemModel.SIZE_TB:
            size = srcSize * SamQFSUtil.SIZE_TB;
            break;

        case SamQFSSystemModel.SIZE_PB:
            size =  srcSize * SamQFSUtil.SIZE_PB;
            break;

        }

        return size;

    }


    public static int getEquipTypeInteger(String equipType) {

        int type = -1;

        if (isValidString(equipType)) {

            if ((equipType.equals("md")) || (equipType.equals("MD")))
                type = DiskCache.MD;

            else if ((equipType.equals("mr")) || (equipType.equals("MR")))
                type = DiskCache.MR;

            else if ((equipType.equals("mm")) || (equipType.equals("MM")))
                type = DiskCache.METADATA;

            else if ((equipType.equals("at")) || (equipType.equals("AT")))
                type = BaseDevice.MTYPE_SONY_AIT;

            else if ((equipType.equals("d2")) || (equipType.equals("D2")))
                type = BaseDevice.MTYPE_AMPEX_DST310;

            else if ((equipType.equals("d3")) || (equipType.equals("D3")))
                type = BaseDevice.MTYPE_STK_SD3;

            else if ((equipType.equals("dt")) || (equipType.equals("DT")))
                type = BaseDevice.MTYPE_DAT;

            else if ((equipType.equals("fd")) || (equipType.equals("FD")))
                type = BaseDevice.MTYPE_FUJITSU_M8100;

            else if ((equipType.equals("i7")) || (equipType.equals("I7")))
                type = BaseDevice.MTYPE_IBM_3570;

            else if ((equipType.equals("li")) || (equipType.equals("LI")))
                type = BaseDevice.MTYPE_IBM_3580_LTO;

            else if ((equipType.equals("lt")) || (equipType.equals("LT")))
                type = BaseDevice.MTYPE_DLT;

            else if ((equipType.equals("se")) || (equipType.equals("SE")))
                type = BaseDevice.MTYPE_STK_9490;

            else if ((equipType.equals("sg")) || (equipType.equals("SG")))
                type = BaseDevice.MTYPE_STK_9840;

            else if ((equipType.equals("so")) || (equipType.equals("SO")))
                type = BaseDevice.MTYPE_SONY_DTF;

            else if ((equipType.equals("vt")) || (equipType.equals("VT")))
                type = BaseDevice.MTYPE_METRUM_VHS;

            else if ((equipType.equals("xm")) || (equipType.equals("XM")))
                type = BaseDevice.MTYPE_EXABYTE_MAMMOTH2;

            else if ((equipType.equals("mo")) || (equipType.equals("MO")))
                type = BaseDevice.MTYPE_EOD;

            else if ((equipType.equals("wo")) || (equipType.equals("WO")))
                type = BaseDevice.MTYPE_WOD;

            else if ((equipType.equals("ib")) || (equipType.equals("IB")))
                type = BaseDevice.MTYPE_IBM_3590;

            else if ((equipType.equals("sf")) || (equipType.equals("SF")))
                type = BaseDevice.MTYPE_STK_T9940;

            else if ((equipType.equals("st")) || (equipType.equals("ST")))
                type = BaseDevice.MTYPE_STK_3480;

            else if ((equipType.equals("xt")) || (equipType.equals("XT")))
                type = BaseDevice.MTYPE_EXABYTE_8MM;

            else if ((equipType.equals("o2")) || (equipType.equals("O2")))
                type = BaseDevice.MTYPE_12WOD;

            else if ((equipType.equals("od")) || (equipType.equals("OD")))
                type = BaseDevice.MTYPE_OPTICAL;

            else if ((equipType.equals("tp")) || (equipType.equals("TP")))
                type = BaseDevice.MTYPE_TAPE;

            else if ((equipType.equals("hy")) || (equipType.equals("HY")))
                type = BaseDevice.MTYPE_HISTORIAN;

            else if ((equipType.equals("rb")) || (equipType.equals("RB")))
                type = BaseDevice.MTYPE_ROBOT;

            else if ((equipType.equals("hc")) || (equipType.equals("HC")))
                type = BaseDevice.MTYPE_HP_L_SERIES;

            else if ((equipType.equals("gr")) || (equipType.equals("GR")))
                type = BaseDevice.MTYPE_ADIC_DAS;

            else if ((equipType.equals("fj")) || (equipType.equals("FJ")))
                type = BaseDevice.MTYPE_FUJ_LMF;

            else if ((equipType.equals("im")) || (equipType.equals("IM")))
                type = BaseDevice.MTYPE_IBM_3494;

            else if ((equipType.equals("pe")) || (equipType.equals("PE")))
                type = BaseDevice.MTYPE_SONY_PETASITE;

            else if ((equipType.equals("sk")) || (equipType.equals("SK")))
                type = BaseDevice.MTYPE_STK_ACSLS;

            else if ((equipType.equals("s9")) || (equipType.equals("S9")))
                type = BaseDevice.MTYPE_STK_97xx;

            else if ((equipType.equals("dk")) || (equipType.equals("DK")))
                type = BaseDevice.MTYPE_DISK;

            else if ((equipType.equals("al")) || (equipType.equals("AL")))
                type = BaseDevice.MTYPE_SSTG_L_SERIES;

            else if ((equipType.equals("c4")) || (equipType.equals("C4")))
                type = BaseDevice.MTYPE_DT_QUANTUMC4;

            else if ((equipType.equals("ti")) || (equipType.equals("TI")))
                type = BaseDevice.MTYPE_DT_TITAN;
            else if ((equipType.equals("cb")) || (equipType.equals("CB")))
                type = BaseDevice.MTYPE_STK_5800;
            else if ((equipType.equals("h4")) || (equipType.equals("H4")))
                type = BaseDevice.MTYPE_HP_SL48;

        }

        return type;

    }


    public static int getMediaTypeInteger(String mediaType) {

        return getEquipTypeInteger(mediaType);

    }


    public static String getMediaTypeString(int mediaType) {

        String s = new String();

        switch (mediaType) {
        case DiskCache.MD:
            s = "md";
            break;
        case DiskCache.MR:
            s = "mr";
            break;
        case DiskCache.METADATA:
            s = "mm";
            break;
        case BaseDevice.MTYPE_SONY_AIT:
            s = "at";
            break;
        case BaseDevice.MTYPE_AMPEX_DST310:
            s = "d2";
            break;
        case BaseDevice.MTYPE_STK_SD3:
            s = "d3";
            break;
        case BaseDevice.MTYPE_DAT:
            s = "dt";
            break;
        case BaseDevice.MTYPE_FUJITSU_M8100:
            s = "fd";
            break;
        case BaseDevice.MTYPE_IBM_3570:
            s = "i7";
            break;
        case BaseDevice.MTYPE_IBM_3580_LTO:
            s = "li";
            break;
        case BaseDevice.MTYPE_DLT:
            s = "lt";
            break;
        case BaseDevice.MTYPE_STK_9490:
            s = "se";
            break;
        case BaseDevice.MTYPE_STK_9840:
            s = "sg";
            break;
        case BaseDevice.MTYPE_SONY_DTF:
            s = "so";
            break;
        case BaseDevice.MTYPE_METRUM_VHS:
            s = "vt";
            break;
        case BaseDevice.MTYPE_EXABYTE_MAMMOTH2:
            s = "xm";
            break;
        case BaseDevice.MTYPE_EOD:
            s = "mo";
            break;
        case BaseDevice.MTYPE_WOD:
            s = "wo";
            break;
        case BaseDevice.MTYPE_IBM_3590:
            s = "ib";
            break;
        case BaseDevice.MTYPE_STK_T9940:
            s = "sf";
            break;
        case BaseDevice.MTYPE_STK_3480:
            s = "st";
            break;
        case BaseDevice.MTYPE_EXABYTE_8MM:
            s = "xt";
            break;
        case BaseDevice.MTYPE_12WOD:
            s = "o2";
            break;
        case BaseDevice.MTYPE_OPTICAL:
            s = "od";
            break;
        case BaseDevice.MTYPE_TAPE:
            s = "tp";
            break;
        case BaseDevice.MTYPE_ROBOT:
            s = "rb";
            break;
        case BaseDevice.MTYPE_HISTORIAN:
            s = "hy";
            break;
        case BaseDevice.MTYPE_IBM_3494:
            s = "im";
            break;
        case BaseDevice.MTYPE_FUJ_LMF:
            s = "fj";
            break;
        case BaseDevice.MTYPE_SONY_PETASITE:
            s = "pe";
            break;
        case BaseDevice.MTYPE_ADIC_DAS:
            s = "gr";
            break;
        case BaseDevice.MTYPE_STK_ACSLS:
            s = "sk";
            break;
        case BaseDevice.MTYPE_STK_97xx:
            s = "s9";
            break;
        case BaseDevice.MTYPE_HP_L_SERIES:
            s = "hc";
            break;
        case BaseDevice.MTYPE_DISK:
            s = "dk";
            break;
        case BaseDevice.MTYPE_DT_QUANTUMC4:
            s = "c4";
            break;
        case BaseDevice.MTYPE_DT_TITAN:
            s = "ti";
            break;
        case BaseDevice.MTYPE_STK_5800:
            s = "cb";
            break;
        case BaseDevice.MTYPE_HP_SL48:
            s = "h4";
            break;
        default:
        }

        return s;
    }


    public static long convertToSecond(long value, int unit) {

        long ret = value;

        switch (unit) {
        case SamQFSSystemModel.TIME_MINUTE:
            ret = value * 60;
            break;
        case SamQFSSystemModel.TIME_HOUR:
            ret = value * 60 * 60;
            break;
        case SamQFSSystemModel.TIME_DAY:
            ret = value * 60 * 60 * 24;
            break;
        case SamQFSSystemModel.TIME_WEEK:
            ret = value * 60 * 60 * 24 * 7;
            break;
        }

        return ret;

    }

    public static int convertStageAttribFromJni(char jniVal) {

        int type = -1;

        if (jniVal == Criteria.NEVER_STAGE)
            type = ArchivePolicy.STAGE_NEVER;
        else if (jniVal == Criteria.ASSOCIATIVE_STAGE)
            type = ArchivePolicy.STAGE_ASSOCIATIVE;
        else if (jniVal == Criteria.SET_DEFAULT_STAGE)
            type = ArchivePolicy.STAGE_DEFAULTS;
        else if (jniVal == Criteria.STAGE_NOT_DEFINED)
            type = ArchivePolicy.STAGE_NO_OPTION_SET;

        return type;

    }


    public static char convertStageAttribToJni(long logicVal) {

        char c = ' ';

        if (logicVal == ArchivePolicy.STAGE_NEVER)
            c = Criteria.NEVER_STAGE;
        else if (logicVal == ArchivePolicy.STAGE_ASSOCIATIVE)
            c = Criteria.ASSOCIATIVE_STAGE;
        else if (logicVal == ArchivePolicy.STAGE_DEFAULTS)
            c = Criteria.SET_DEFAULT_STAGE;
        else if (logicVal == ArchivePolicy.STAGE_NO_OPTION_SET)
            c = Criteria.STAGE_NOT_DEFINED;

        return c;

    }


    public static int convertReleaseAttribFromJni(char jniVal) {

        int type = -1;

        if (jniVal == Criteria.NEVER_RELEASE)
            type = ArchivePolicy.RELEASE_NEVER;
        else if (jniVal == Criteria.PARTIAL_RELEASE)
            type = ArchivePolicy.RELEASE_PARTIAL;
        else if (jniVal == Criteria.ALWAYS_RELEASE)
            type = ArchivePolicy.RELEASE_AFTER_ONE;
        else if (jniVal == Criteria.SET_DEFAULT_RELEASE)
            type = ArchivePolicy.RELEASE_DEFAULTS;
        else if (jniVal == Criteria.RELEASE_NOT_DEFINED)
            type = ArchivePolicy.RELEASE_NO_OPTION_SET;

        return type;
    }

    public static char convertReleaseAttribToJni(long logicVal) {

        char c = ' ';

        if (logicVal == ArchivePolicy.RELEASE_NEVER)
            c = Criteria.NEVER_RELEASE;
        else if (logicVal == ArchivePolicy.RELEASE_PARTIAL)
            c = Criteria.PARTIAL_RELEASE;
        else if (logicVal == ArchivePolicy.RELEASE_DEFAULTS)
            c = Criteria.SET_DEFAULT_STAGE;
        else if (logicVal == ArchivePolicy.RELEASE_AFTER_ONE)
            c = Criteria.ALWAYS_RELEASE;
        else if (logicVal == ArchivePolicy.RELEASE_NO_OPTION_SET)
            c = Criteria.RELEASE_NOT_DEFINED;

        return c;

    }

    public static int convertARSortMethod(int jniMethod) {

        int type = -1;

        switch (jniMethod) {
        case CopyParams.SM_NOT_SET:
            type = ArchivePolicy.SM_NOT_SET;
            break;
        case CopyParams.SM_NONE:
            type = ArchivePolicy.SM_NONE;
            break;
        case CopyParams.SM_AGE:
            type = ArchivePolicy.SM_AGE;
            break;
        case CopyParams.SM_PATH:
            type = ArchivePolicy.SM_PATH;
            break;
        case CopyParams.SM_PRIORITY:
            type = ArchivePolicy.SM_PRIORITY;
            break;
        case CopyParams.SM_SIZE:
            type = ArchivePolicy.SM_SIZE;
            break;
        }

        return type;

    }


    public static int convertARSortMethodJni(int uiMethod) {

        int type = -1;

        switch (uiMethod) {
        case ArchivePolicy.SM_NOT_SET:
            type = CopyParams.SM_NOT_SET;
            break;
        case ArchivePolicy.SM_NONE:
            type = CopyParams.SM_NONE;
            break;
        case ArchivePolicy.SM_AGE:
            type = CopyParams.SM_AGE;
            break;
        case ArchivePolicy.SM_PATH:
            type = CopyParams.SM_PATH;
            break;
        case ArchivePolicy.SM_PRIORITY:
            type = CopyParams.SM_PRIORITY;
            break;
        case ArchivePolicy.SM_SIZE:
            type = CopyParams.SM_SIZE;
            break;
        }

        return type;

    }


    public static int convertAROfflineCopyMethod(int jniMethod) {

        int type = -1;

        switch (jniMethod) {
        case CopyParams.OC_NOT_SET:
            type = ArchivePolicy.OC_NOT_SET;
            break;
        case CopyParams.OC_NONE:
            type = ArchivePolicy.OC_NONE;
            break;
        case CopyParams.OC_DIRECT:
            type = ArchivePolicy.OC_DIRECT;
            break;
        case CopyParams.OC_STAGEAHEAD:
            type = ArchivePolicy.OC_STAGEAHEAD;
            break;
        case CopyParams.OC_STAGEALL:
            type = ArchivePolicy.OC_STAGEALL;
            break;
        }

        return type;

    }


    public static int convertAROfflineCopyMethodJni(int uiMethod) {

        int type = -1;

        switch (uiMethod) {
        case ArchivePolicy.OC_NOT_SET:
            type = CopyParams.OC_NOT_SET;
            break;
        case ArchivePolicy.OC_NONE:
            type = CopyParams.OC_NONE;
            break;
        case ArchivePolicy.OC_DIRECT:
            type = CopyParams.OC_DIRECT;
            break;
        case ArchivePolicy.OC_STAGEAHEAD:
            type = CopyParams.OC_STAGEAHEAD;
            break;
        case ArchivePolicy.OC_STAGEALL:
            type = CopyParams.OC_STAGEALL;
            break;
        }

        return type;

    }


    public static int convertARJoinMethod(int jniMethod) {

        int type = -1;

        switch (jniMethod) {
        case CopyParams.JOIN_NOT_SET:
            type = ArchivePolicy.JOIN_NOT_SET;
            break;
        case CopyParams.NO_JOIN:
            type = ArchivePolicy.NO_JOIN;
            break;
        case CopyParams.JOIN_PATH:
            type = ArchivePolicy.JOIN_PATH;
            break;
        }

        return type;

    }


    public static int convertARJoinMethodJni(int uiMethod) {

        int type = -1;

        switch (uiMethod) {
        case ArchivePolicy.JOIN_NOT_SET:
            type = CopyParams.JOIN_NOT_SET;
            break;
        case ArchivePolicy.NO_JOIN:
            type = CopyParams.NO_JOIN;
            break;
        case ArchivePolicy.JOIN_PATH:
            type = CopyParams.JOIN_PATH;
            break;
        }

        return type;

    }


    public static GregorianCalendar convertTime(long time) {

        GregorianCalendar calendar = null;

        if (time > 0) {
            Date date = new Date(time * 1000);
            calendar = new GregorianCalendar();
            calendar.setTime(date);
        }

        return calendar;

    }


    public static long convertTime(GregorianCalendar time) {

         return (time.getTime().getTime())/1000;

    }


    public static boolean isValidString(String str) {

        return (str != null && str.length() != 0);

    }


    public static int convertStateToJni(int uiState) {

        int jniState = -1;

        switch (uiState) {

        case BaseDevice.ON:
            jniState = BaseDev.DEV_ON;
            break;

        case BaseDevice.OFF:
            jniState = BaseDev.DEV_OFF;
            break;

        case BaseDevice.DOWN:
            jniState = BaseDev.DEV_DOWN;
            break;

        case BaseDevice.UNAVAILABLE:
            jniState = BaseDev.DEV_UNAVAIL;
            break;

        case BaseDevice.IDLE:
            jniState = BaseDev.DEV_IDLE;
            break;

        case BaseDevice.READONLY:
            jniState = BaseDev.DEV_RO;
            break;

        }

        return jniState;

    }


    public static int convertStateToUI(int jniState) {

        int uiState = -1;

        switch (jniState) {

        case BaseDev.DEV_ON:
            uiState = BaseDevice.ON;
            break;

        case BaseDev.DEV_OFF:
            uiState = BaseDevice.OFF;
            break;

        case BaseDev.DEV_DOWN:
            uiState = BaseDevice.DOWN;
            break;

        case BaseDev.DEV_UNAVAIL:
            uiState = BaseDevice.UNAVAILABLE;
            break;

        case BaseDev.DEV_IDLE:
            uiState = BaseDevice.IDLE;
            break;

        case BaseDev.DEV_RO:
            uiState = BaseDevice.READONLY;
            break;

        }

        return uiState;

    }


    public static int getDriverTypeFromEQType(String eqType) {

        int type = Library.SAMST;

        if (isValidString(eqType)) {
            if ((eqType.equals("sk")) || (eqType.equals("SK")))
                type = Library.ACSLS;
            else if ((eqType.equals("gr")) || (eqType.equals("GR")))
                type = Library.ADIC_GRAU;
            else if ((eqType.equals("fj")) || (eqType.equals("FJ")))
                type = Library.FUJITSU_LMF;
            else if ((eqType.equals("im")) || (eqType.equals("IM")))
                type = Library.IBM_3494;
            else if ((eqType.equals("pe")) || (eqType.equals("PE")))
                type = Library.SONY;
        }

        return type;

    }


    public static int getLogicScanType(short jniType) {

        int type = -1;

        if (jniType == ArFSDirective.EM_NOT_SET)
            type = GlobalArchiveDirective.SCAN_NOT_SET;
        else if (jniType == ArFSDirective.EM_SCAN)
            type = GlobalArchiveDirective.CONTINUOUS_SCAN;
        else if (jniType == ArFSDirective.EM_SCANDIRS)
            type = GlobalArchiveDirective.SCAN_DIRS;
        else if (jniType == ArFSDirective.EM_SCANINODES)
            type = GlobalArchiveDirective.SCAN_INODES;
        else if (jniType == ArFSDirective.EM_NOSCAN)
            type = GlobalArchiveDirective.NO_SCAN;

        return type;

    }


    public static short getJniScanType(int logicType) {

        short type = ArFSDirective.EM_NOT_SET;

        switch (logicType) {
        case GlobalArchiveDirective.CONTINUOUS_SCAN:
            type = ArFSDirective.EM_SCAN;
            break;
        case GlobalArchiveDirective.SCAN_DIRS:
            type = ArFSDirective.EM_SCANDIRS;
            break;
        case GlobalArchiveDirective.SCAN_INODES:
            type = ArFSDirective.EM_SCANINODES;
            break;
        case  GlobalArchiveDirective.NO_SCAN:
            type = ArFSDirective.EM_NOSCAN;
            break;
        }

        return type;

    }


    public static String dateTime(GregorianCalendar dateTime) {

        String dateTimeString = new String();

        if (dateTime != null) {

            int month = dateTime.get(Calendar.MONTH) + 1;
            int day = dateTime.get(Calendar.DAY_OF_MONTH);
            int year = dateTime.get(Calendar.YEAR);
            int hour = dateTime.get(Calendar.HOUR);
            int minute = dateTime.get(Calendar.MINUTE);
            int second = dateTime.get(Calendar.SECOND);
            String am_pm = new String();
            if (dateTime.get(Calendar.AM_PM) == Calendar.PM)
                am_pm = "PM";
            else if (dateTime.get(Calendar.AM_PM) == Calendar.AM)
                am_pm = "AM";

            String fillerMin = new String();
            if (minute < 10)
                fillerMin = "0";

            String fillerSec = new String();
            if (second < 10)
                fillerSec = "0";

            dateTimeString =  month + "/" + day + "/" + year + ", "
                + hour + ":" + fillerMin + minute + ":" + fillerSec + second
                + " " + am_pm;

        }

        return dateTimeString;

    }

    public static String fsizeToString(String fsize)
        throws NumberFormatException {

        int length = fsize.length();
        char unit = fsize.charAt(length - 1);

        if (Character.isDigit(unit)) {
            unit = 'b'; // default unit
            length++;
        }

        BigInteger size = new BigInteger(fsize.substring(0, length - 1));

        switch (unit) {
            case 'b':
            case 'B': // bytes

                break;

            case 'k':
            case 'K': // kilobytes

                size = size.multiply(new BigInteger(
                                            new Long(SIZE_KB).toString()));

                break;

            case 'm':
            case 'M': // megabytes

                size = size.multiply(new BigInteger(
                                            new Long(SIZE_MB).toString()));

                break;

            case 'g':
            case 'G': // gigabytes

                size = size.multiply(new BigInteger(
                                            new Long(SIZE_GB).toString()));

                break;

            case 't':
            case 'T': // terabytes

                size = size.multiply(new BigInteger(
                                            new Long(SIZE_TB).toString()));

                break;

            case 'p':
            case 'P': // petabytes

                size = size.multiply(new BigInteger(
                                            new Long(SIZE_PB).toString()));

                break;

            default:
                throw new NumberFormatException("Invalid file size: "
                                                + fsize);

            }

        return (size.toString());
    }


    public static String stringToFsize(String bytes)
        throws NumberFormatException {

            // if params is 0, direct return 0 and 'byte' as unit
            if (bytes.equals("0"))
                return "0B";

            BigInteger size = new BigInteger(bytes);
            int i, count = SIZE_UNITS.length() -1;

            for (i = 0; i < count; i++) {
                if (size.mod(SIZE_BASE).compareTo(BigInteger.ZERO) != 0) {
                    break;
                }
                size = size.divide(SIZE_BASE);
            }

            StringBuffer interval =
                new StringBuffer(size.toString()).append(SIZE_UNITS.charAt(i));

        return (interval.toString());

    }


    public static String getUnitString(int unit) {

        String ret = "b";

        switch (unit) {
        case SamQFSSystemModel.SIZE_KB:
            ret = "k";
            break;
        case SamQFSSystemModel.SIZE_MB:
            ret = "m";
            break;
        case SamQFSSystemModel.SIZE_GB:
            ret = "g";
            break;
        case SamQFSSystemModel.SIZE_TB:
            ret = "t";
            break;
        case SamQFSSystemModel.SIZE_PB:
            ret = "p";
            break;
        }

        return ret;

    }


    public static String getTimeUnitString(int unit) {

        String ret = "s";

        switch (unit) {
        case SamQFSSystemModel.TIME_MINUTE:
            ret = "m";
            break;
        case SamQFSSystemModel.TIME_HOUR:
            ret = "h";
            break;
        case SamQFSSystemModel.TIME_DAY:
            ret = "d";
            break;
        case SamQFSSystemModel.TIME_WEEK:
            ret = "w";
            break;
        case SamQFSSystemModel.TIME_YEAR:
            ret = "y";
            break;
        case SamQFSSystemModel.TIME_MONTHS:
            ret = "M";
            break;
        case SamQFSSystemModel.TIME_DAY_OF_MONTH:
            ret = "D";
            break;
        case SamQFSSystemModel.TIME_DAY_OF_WEEK:
            ret = "W";
            break;
        }

        return ret;

    }


    public static long getLongVal(String sizeWithUnit) {

        long val = -1;
        if ((isValidString(sizeWithUnit)) && (sizeWithUnit.length() > 1)) {
            String str = sizeWithUnit.substring(0, sizeWithUnit.length() - 1);
            try {
                val = (new Long(str)).longValue();
            } catch (NumberFormatException nfe) {
                nfe.printStackTrace();
            }
        }

        return val;

    }


    public static int getIntegerVal(String sizeWithUnit) {

        int val = -1;
        if ((isValidString(sizeWithUnit)) && (sizeWithUnit.length() > 1)) {
            String str = sizeWithUnit.substring(0, sizeWithUnit.length() - 1);
            try {
                val = (new Integer(str)).intValue();
            } catch (NumberFormatException nfe) {
                nfe.printStackTrace();
            }
        }

        return val;

    }


    public static int getSizeUnitInteger(String sizeWithUnit) {

        int val = -1;
        if ((isValidString(sizeWithUnit)) && (sizeWithUnit.length() > 0)) {
            char c = sizeWithUnit.charAt(sizeWithUnit.length() - 1);
            switch (c) {
            case 'b': case 'B':
                val = SamQFSSystemModel.SIZE_B;
                break;
            case 'k': case 'K':
                val = SamQFSSystemModel.SIZE_KB;
                break;
            case 'm': case 'M':
                val = SamQFSSystemModel.SIZE_MB;
                break;
            case 'g': case 'G':
                val = SamQFSSystemModel.SIZE_GB;
                break;
            case 't': case 'T':
                val = SamQFSSystemModel.SIZE_TB;
                break;
            case 'p': case 'P':
                val = SamQFSSystemModel.SIZE_PB;
                break;
            }
        }
        return val;

    }


    public static long getLongValSecond(String timeWithUnit) {

        long val = -1;
        if ((isValidString(timeWithUnit)) && (timeWithUnit.length() > 1)) {
            String str = timeWithUnit.substring(0, timeWithUnit.length() - 1);
            try {
                val = (new Long(str)).longValue();
            } catch (NumberFormatException nfe) {
                nfe.printStackTrace();
            }
        }

        return val;

    }


    public static int getTimeUnitInteger(String timeWithUnit) {

        int val = -1;
        if ((isValidString(timeWithUnit)) && (timeWithUnit.length() > 0)) {
            char c = timeWithUnit.charAt(timeWithUnit.length() - 1);
            switch (c) {
                case 's': case 'S':
                    val = SamQFSSystemModel.TIME_SECOND;
                    break;
                case 'm':
                    val = SamQFSSystemModel.TIME_MINUTE;
                    break;
                case 'h': case 'H':
                    val = SamQFSSystemModel.TIME_HOUR;
                    break;
                case 'd':
                    val = SamQFSSystemModel.TIME_DAY;
                    break;
                case 'w':
                    val = SamQFSSystemModel.TIME_WEEK;
                    break;
                case 'y':
                case 'Y':
                    val = SamQFSSystemModel.TIME_YEAR;
                    break;
                case 'M':
                    val = SamQFSSystemModel.TIME_MONTHS;
                    break;
                case 'D':
                    val = SamQFSSystemModel.TIME_DAY_OF_MONTH;
                    break;
                case 'W':
                    val = SamQFSSystemModel.TIME_DAY_OF_WEEK;
                    break;
            }
        }
        return val;

    }

    public static String longToInterval(long seconds)
        throws NumberFormatException {
        if (seconds < 0) {
            throw new NumberFormatException(
                "Invalid time interval: " + seconds);
        }

        if (seconds == 0) {
            return "0s";
        }

        int i;

        for (i = 0; i < TIME_BASE.length; i++) {
            if ((seconds % TIME_BASE[i]) != 0) {
                break;
            }

            seconds /= TIME_BASE[i];
        }

        StringBuffer interval =
            new StringBuffer().append(seconds).append(TIME_UNITS.charAt(i));

        return (interval.toString());
    }


    // the variable "host" has been made null on purpose.
    // revisit this issue later if needed
    public static void createDiskVolInfo(String dvsn, String host,
                                         String path, Ctx ctx)
        throws SamFSException {

        if (isValidString(dvsn)) {
            DiskVol vol = null;
            try {
                vol = DiskVol.get(ctx, dvsn);
            } catch (SamFSException se) {
                // exception is thrown if it didn't exist
            }

            if (vol == null) {
                DiskVol.add(ctx, new DiskVol(dvsn, null, path));

            } else {
                // Nothing needs to be done because same information exists
            }
        }

    }

    public static void removeDiskVolInfo(String dvsn, Ctx ctx)
        throws SamFSException {

        if (isValidString(dvsn)) {
            DiskVol vol = null;
            try {
                vol = DiskVol.get(ctx, dvsn);
            } catch (SamFSException se) {}
            if (vol != null)
                vol.remove(ctx, dvsn);
        }
    }

    public static boolean isDifferentDiskVolInfoPresent(String dvsn,
                                                        String host,
                                                        String path,
                                                        Ctx ctx) {

        boolean present = false;

        if (isValidString(dvsn)) {

            DiskVol vol = null;
            try {
                vol = DiskVol.get(ctx, dvsn);
            } catch (SamFSException se) {
                // exception is thrown if it didn't exist
                vol = null;
            }

            if (vol != null) {

                // check whether same path
                String volPath = vol.getPath();
                if ((volPath != null) && (!volPath.equals(path)))
                    present = true;

                if (!present) {
                    if (((vol.getHost() != null) &&
                         (!vol.getHost().equals(host))) ||
                        ((host != null) && (!host.equals(vol.getHost()))))
                        present = true;
                }
            }

        }

        return present;

    }


    public static int[] getRemovableMediaStatusIntegers(String status) {

        ArrayList list = new ArrayList();
        if (isValidString(status)) {
            for (int i = 0; i < status.length(); i++) {
                char c = status.charAt(i);
                if ((c == 'R') && (i == 5)) {
                    c = 'Z';
                }
                int statusInt =
                    getRemovableMediaStatusInteger(c);
                if (statusInt != -1) {
                    list.add(new Integer(statusInt));
                }
            }
        }

        int[] statusIntArray = new int[list.size()];
        for (int i = 0; i < list.size(); i++) {
            statusIntArray[i] = ((Integer) list.get(i)).intValue();
        }

        return statusIntArray;

    }


    private static int getRemovableMediaStatusInteger(char c) {

        int status = -1;

        switch (c) {

            case 's':
                status = SamQFSSystemMediaManager.RM_STATUS_MEDIA_SCAN;
                break;
            case 'm':
                status =
                    SamQFSSystemMediaManager.RM_STATUS_AUTO_LIB_OPERATIONAL;
                break;
            case 'M':
                status = SamQFSSystemMediaManager.RM_STATUS_MAINT_MODE;
                break;
            case 'E':
                status =
                    SamQFSSystemMediaManager.RM_STATUS_UNRECOVERABLE_ERR_SCAN;
                break;
            case 'a':
                status = SamQFSSystemMediaManager.RM_STATUS_DEVICE_AUDIT;
                break;
            case 'l':
                status = SamQFSSystemMediaManager.RM_STATUS_LABEL_PRESENT;
                break;
            case 'N':
                status = SamQFSSystemMediaManager.RM_STATUS_FOREIGH_MEDIA;
                break;
            case 'L':
                status = SamQFSSystemMediaManager.RM_STATUS_LABELLING_ON;
                break;
            case 'I':
                status = SamQFSSystemMediaManager.RM_STATUS_WAIT_FOR_IDLE;
                break;
            case 'A':
                status =
                    SamQFSSystemMediaManager.RM_STATUS_NEEDS_OPERATOR_ATTENTION;
                break;
            case 'C':
                status = SamQFSSystemMediaManager.RM_STATUS_NEEDS_CLEANING;
                break;
            case 'U':
                status = SamQFSSystemMediaManager.RM_STATUS_UNLOAD_REQUESTED;
                break;
            case 'Z':
                status = SamQFSSystemMediaManager.RM_STATUS_RESERVED_DEVICE;
                break;
            case 'w':
                status =
                    SamQFSSystemMediaManager.RM_STATUS_PROCESS_WRITING_MEDIA;
                break;
            case 'o':
                status = SamQFSSystemMediaManager.RM_STATUS_DEV_OPEN;
                break;
            case 'P':
                status = SamQFSSystemMediaManager.RM_STATUS_TAPE_POSITIONING;
                break;
            case 'F':
                status =
                    SamQFSSystemMediaManager.
                        RM_STATUS_AUTO_LIB_ALL_SLOTS_OCCUPIED;
                break;
            case 'R':
                status = SamQFSSystemMediaManager.RM_STATUS_DEV_READONLY_READY;
                break;
            case 'r':
                status = SamQFSSystemMediaManager.RM_STATUS_DEV_SPUN_UP_READY;
                break;
            case 'p':
                status = SamQFSSystemMediaManager.RM_STATUS_DEV_PRESENT;
                break;
            case 'W':
                status = SamQFSSystemMediaManager.RM_STATUS_WRITE_PROTECTED;
                break;

        }

        return status;

    }

    public static void doPrint(String str) {
    }

    public static String arr2Str(Object[] arr) {
        String s = "[";
        if (arr != null)
            for (int i = 0; i < arr.length; i++) {
                s += arr[i];
                if (i != arr.length - 1)
                   s += ",";
            }
        return (s + "]");
    }

}
