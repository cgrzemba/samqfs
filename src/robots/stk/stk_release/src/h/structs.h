/* SccsId @(#)structs.h	1.2 1/11/94  */
#ifndef _STRUCTS_
#define _STRUCTS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      definitions of specific system data structures.  
 *      most of these structures are common to csi and acslm, 
 *      and are included by both header files.
 *
 * Considerations:
 *
 *      Because identifer.h and db_structs.h may be included within an SQL
 *      declared of embedded SQL modules, these header files are conditionally
 *      included here.  Due to the limitations of the SQL preprocessor, the
 *      conditionally included header files cannot conditionally exclude
 *      themselves so it must be done here.
 *
 * Modified by:
 *
 *      D. F. Reed              22-Sep-1988     Original.
 *      J. A. Wishner           06-Apr-1990     Added QU_CLN_STATUS for R2.
 *      J. A. Wishner           10-Apr-1990     Added LO_VOL_STATUS,
 *                          LO_DRV_STATUS, QL_VOL_STATUS, QL_DRV_STATUS,
 *                          QU_EXT_CAP_STATUS
 *      J. W. Montgomery        27-Jun-1990     Added Version 0 Message_header.
 *      D. A. Beidle            27-Jun-1990     Added cap_size to
 *                          QU_EXT_CAP_STATUS
 *      J. W. Montgomery        11-Jul-1990     Added requestor_type to
 *                          IPC_HEADER.
 *      J. S. Alexander         18-Jun-1991     Added QU_EXT2_CAP_STATUS and 
 *                          VA_CAP_STATUS, removed V0_MESSAGE_HEADER.
 *      H. I. Grapek            23-Sep-1991     
 *                          - Changed name of QU_CAP_STATUS 
 *                              to QU_V0_CAP_STATUS
 *                          - Changed name of QU_EXT_CAP_STATUS 
 *                              to QU_V1_CAP_STATUS
 *                          - Changed name of QU_EXT2_CAP_STATUS
 *                              to QU_CAP_STATUS
 *                          - Modified QU_V0_CAP_STATUS and QU_V1_CAP_STATUS 
 *                              structures to use V0_CAPID and V1_CAPID 
 *                              respectively.
 *                          - Moved QU_V0_CAP_STATUS to v0_structs.h.
 *                          - Moved QU_V1_CAP_STATUS to v1_structs.h.
 *      D. A> Beidle            27-Sep-1991     Added comments on the
 *                          necessity of conditionally including identifier.h
 *                          and db_structs.h
 *	J. Borzuchowski	    	05-Aug-1993	R5.0 Mixed Media-- 
 *			    Added media type to QU_CLN_STATUS, QU_SCR_STATUS, 
 *			    QU_VOL_STATUS; added drive type to QU_DRV_STATUS.
 *			    Added new select_criteria union member structures 
 *			    QU_ACS_CRITERIA, QU_LSM_CRITERIA, QU_CAP_CRITERIA,
 *			    QU_DRV_CRITERIA, QU_VOL_CRITERIA, QU_REQ_CRITERIA,
 *			    QU_PRT_CRITERIA, QU_POL_CRITERIA, QU_MCS_CRITERIA.
 *			    Added new response_status union member structures 
 *			    QU_SRV_RESPONSE, QU_ACS_RESPONSE, QU_LSM_RESPONSE,
 *			    QU_CAP_RESPONSE, QU_CLN_RESPONSE, QU_DRV_RESPONSE,
 *			    QU_MNT_RESPONSE, QU_VOL_RESPONSE, QU_PRT_RESPONSE,
 *			    QU_REQ_RESPONSE, QU_SCR_RESPONSE, QU_POL_RESPONSE,
 *			    QU_MSC_RESPONSE, QU_MMI_RESPONSE.
 *			    Added status structures for new query mixed 
 *			    media info request, QU_MEDIA_TYPE_STATUS, and
 *			    QU_DRIVE_TYPE_STATUS.
 *
 *	J. Borzuchowski	    	23-Aug-1993	R5.0 Mixed Media-- 
 *			    Changed drive_count in QU_MNT_RESPONSE and
 *			    QU_MSC_RESPONSE to mount_status_count and 
 *			    msc_status_count respectively.
 *      Alec Sharp              26-Aug-1993     R5.0 Removed cl_mm_pub.h
 *      Alec Sharp              07-Sep-1993     R5.0 Added HOSTID to
 *                          ipc_header structure.
 *	J. Borzuchowski         09-Sep-1993     R5.0 Mixed Media-- Changed 
 *                          counts in new Query structures from ints to shorts,
 *                          mistyped on 05-Aug-1993. 
 *	David A. Myers	    16-Sep-1993	    Split file with portion required
 *			    by ACSAPI into api/structs_api.h.  Also changed
 *			    hostid size to 12.
 */

/*
 *      Header Files:
 */

#ifndef _DEFS_
#include "defs.h"
#endif

#ifndef _IDENTIFIER_
#include "identifier.h"
#endif
  
#ifndef _IPC_HDR_API_
#include "api/ipc_hdr_api.h"
#endif

#ifndef _STRUCTS_API_
#include "api/structs_api.h"
#endif
  
typedef struct {                        /* event log message */
    IPC_HEADER      ipc_header;
    LOG_OPTION      log_options;
    char            event_message[MAX_MESSAGE_SIZE];
} EVENT_LOG_MESSAGE;

typedef struct {                        /* unsolicited message */
    IPC_HEADER      ipc_header;
    MESSAGE_HEADER  message_header;
    RESPONSE_STATUS message_status;
    unsigned short  error;
} UNSOLICITED_MESSAGE;

#endif /* _STRUCTS_ */

