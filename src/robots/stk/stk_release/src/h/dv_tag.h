/* SccsId @(#)dv_tag.h	1.2 11/11/93 (c) 1992-1993 STK */
#ifndef _DV_TAG_
#define _DV_TAG_
/*
 *                    (C) Copyright (1992-1993)
 *                  Storage Technology Corporation
 *                        All Rights Reserved
 *
 * Functional Description:
 *
 *      This header file contains enum declarations for the
 *      dynamic variables supported by Release 5.0 of the ACSLS.
 *
 *
 * Considerations:
 *
 *    Created from dv_config.dat on Fri Oct 29 01:07:28 1993
 *    This file will have to be re-created by STK ACSLS whenever
 *    new dynamic variables are supported by the ACSLS server code.
 *
 *
 *
 * Modified by:
 *
 *   Ken Stickney        10/06/93       Original
 *
 */

typedef enum dv_tag {
	DV_TAG_FIRST = 0,
	DV_TAG_EVENT_FILE_NUMBER,
	DV_TAG_LOG_PATH,
	DV_TAG_LOG_SIZE,
	DV_TAG_TIME_FORMAT,
	DV_TAG_UNIFORM_CLEAN_USE,
	DV_TAG_AC_CMD_ACCESS,
	DV_TAG_AC_CMD_DEFAULT,
	DV_TAG_AC_VOL_ACCESS,
	DV_TAG_AC_VOL_DEFAULT,
	DV_TAG_AC_LOG_ACCESS,
	DV_TAG_CSI_CONNECT_AGETIME,
	DV_TAG_CSI_RETRY_TIMEOUT,
	DV_TAG_CSI_RETRY_TRIES,
	DV_TAG_CSI_TCP_RPCSERVICE,
	DV_TAG_CSI_UDP_RPCSERVICE,
	DV_TAG_AUTO_CLEAN,
	DV_TAG_AUTO_START,
	DV_TAG_MAX_ACSMT,
	DV_TAG_MAX_ACS_PROCESSES,
	DV_TAG_TRACE_ENTER,
	DV_TAG_TRACE_VOLUME,
	DV_TAG_ACSLS_MIN_VERSION,
	DV_TAG_DI_TRACE_FILE,
	DV_TAG_DI_TRACE_VALUE,
	DV_TAG_LM_RP_TRAIL,
	DV_TAG_ACSLS_ALLOW_ACSPD,
	DV_TAG_LAST
} DV_TAG;

#endif

