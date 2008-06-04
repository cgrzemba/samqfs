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

// ident	$Id: BaseDevice.java,v 1.20 2008/06/04 18:10:13 ronaldso Exp $

package com.sun.netstorage.samqfs.web.model.media;


public interface BaseDevice {

    // device states
    public static final int ON = 0;
    public static final int OFF = 1;
    public static final int DOWN = 2;
    public static final int UNAVAILABLE = 3;
    public static final int IDLE = 4;
    public static final int READONLY = 5;
    public static final int NOALLOC = 6;

    // media types
    public static final int MTYPE_SONY_AIT = 101;
    public static final int MTYPE_AMPEX_DST310 = 102;
    public static final int MTYPE_STK_SD3 = 103;
    public static final int MTYPE_DAT = 104;
    public static final int MTYPE_FUJITSU_M8100 = 105;
    public static final int MTYPE_IBM_3570 = 106;
    public static final int MTYPE_IBM_3580_LTO = 107;
    public static final int MTYPE_DLT = 108;
    public static final int MTYPE_STK_9490 = 109;
    public static final int MTYPE_STK_9840 = 110;
    public static final int MTYPE_SONY_DTF = 111;
    public static final int MTYPE_METRUM_VHS = 112;
    public static final int MTYPE_EXABYTE_MAMMOTH2 = 113;
    public static final int MTYPE_EOD = 114;
    public static final int MTYPE_WOD = 115;
    public static final int MTYPE_HISTORIAN = 116;

    public static final int MTYPE_IBM_3590 = 122;
    public static final int MTYPE_STK_T9940 = 123;
    public static final int MTYPE_STK_3480 = 124;
    public static final int MTYPE_EXABYTE_8MM = 125;
    public static final int MTYPE_12WOD = 126;
    public static final int MTYPE_OPTICAL = 127;
    public static final int MTYPE_TAPE = 128;
    public static final int MTYPE_ROBOT = 129;
    public static final int MTYPE_HP_L_SERIES = 130;
    public static final int MTYPE_STK_97xx = 132;
    public static final int MTYPE_SSTG_L_SERIES = 134;
    public static final int MTYPE_DT_QUANTUMC4 = 135;
    public static final int MTYPE_DT_TITAN = 136;
    public static final int MTYPE_HP_SL48 = 138;


    // for network attached libraries
    public static final int MTYPE_STK_ACSLS = 117;
    public static final int MTYPE_ADIC_DAS = 118;
    public static final int MTYPE_SONY_PETASITE = 119;
    public static final int MTYPE_FUJ_LMF = 120;
    public static final int MTYPE_IBM_3494 = 121;
    public static final int MTYPE_STK_SL3000 = 139;
    public static final int MTYPE_LT270_250 = 140;

    // for disk archiving
    public static final int MTYPE_DISK = 133;
    public static final int MTYPE_STK_5800 = 137;

    // getters

    public String getDevicePath();

    // setDevicePath is used only when adding ACSLS Library from Version 4.5.
    // Device Path is no longer returned from media discovery and presentation
    // layer needs to set the device path so the C-layer knows where to write
    // the parameter file on the library.
    public void setDevicePath(String devicePath);


    public int getEquipOrdinal();

    public void setEquipOrdinal(int equipOrdinal);


    public int getEquipType();


    public String getFamilySetName();


    public int getFamilySetEquipOrdinal();


    public int getState();


    public void setState(int state);


    public String getAdditionalParamFilePath();

    /**
     * Retrieve the log path of this device (Library/Drive)
     * @since 4.6
     * @return log path of the device
     */
    public String getLogPath();

    /**
     * Set the log path of this device (Library/Drive)
     * @since 4.6
     * @param logPath - log path of the device
     */
    public void setLogPath(String logPath);

    /**
     * Retrieve the last modification time of log path of this device
     * @since 4.6
     * @return the last modification time of log path of this device
     */
    public long getLogModTime();

    /**
     * Set the last modification time of log path of this device
     * @since 4.6
     * @param logModTime - the last modification time of log path of this device
     */
    public void setLogModTime(long logModTime);
}
