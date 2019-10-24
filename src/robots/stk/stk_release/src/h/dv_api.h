/* SccsId @(#)dv_api.h	1.2 1/11/94 (c) 1992-1994 STK */
#ifndef _DV_API_
#define _DV_API_
/*
 *                    (C) Copyright (1992-1993)
 *                  Storage Technology Corporation
 *                        All Rights Reserved
 *
 * Functional Description:
 *
 *      This header file contains enum declarations for the
 *      dynamic variables supported by Release 5.0 of the ACSLS.
 *	needed for the TOOLKIT.
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
 *   Ken Stickney       10/06/93       	Original
 *   Howie Grapek	04-Jan-1994	Mods made to make dv_api.c compile.
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

#define DVI_NAME_LEN   	32
#define DVI_VALUE_LEN  128  /* NOTE: This is also defined in dv_pub.h
			       under the name DV_VALUE_LEN. If you change
			       one, you MUST change the other. The reason
			       for this is because we had to solve a
			       circular dependency with respect to dv_tag.h
			       and this was part of the solution.
			       
			       Length of string (excluding \0) that
			       contains the value for the tag. When
			       getting a string value, make sure your
			       buffer is at least DVI_VALUE_LEN + 1 long */

/* Shared memory structure. Note that the array of length 1 is basically
   a placeholder. The actual array will be larger. The number of real
   elements in the array will be DV_TAG_LAST - 1. Array element 0 is not
   counted in the element count because it is never used. We index
   directly into the array using the DV_TAG value. */

enum dv_shm_type {
    DVI_SHM_UNKNOWN = 0,   /* Must be zero */
    DVI_SHM_BOOLEAN,
    DVI_SHM_MASK,
    DVI_SHM_NUMBER,
    DVI_SHM_STRING
};

union dv_shm_value {
    BOOLEAN        boolean;
    unsigned long  mask;
    long           number;
    char           string[DVI_VALUE_LEN + 1];
};


/*
 * From ../h/sblk_defs.h
 *
 *  All dynamic shared memory blocks are required to have the following
 *  header. I.e., most of the shared memory can be as they want it, but
 *  it must start with the following structure. 
 */

struct dshm_hdr {
    BOOLEAN reattach;     /* Does the process need to reattach?  */
    BOOLEAN built;        /* Has the data been rebuilt?          */
    time_t  timestamp;
};

struct dv_shm {
    struct dshm_hdr header;   /* Common header for dynamic shared memory */
    int                    i_count;   /* Number of variables in shared memory
					 excluding the unused element 0 */
    struct {
	char               ca_name[DVI_NAME_LEN + 1];
	enum dv_shm_type   type;
	union dv_shm_value u;
    } variable[1];         /* Element 0 is not used */
};

#endif

