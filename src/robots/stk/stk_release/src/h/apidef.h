#ifndef _APIDEF_H_
#define _APIDEF_H_ 1
 
/* static char    SccsId[] = "@(#) %full_name:     h/incl/apidef/2.1.2 %"; */
/*
 *                    (C) Copyright (1992-1993)
 *                  Storage Technology Corporation
 *                        All Rights Reserved
 *
 * Functional Description:
 *
 *      This header file contains all necessary #defines
 *      for StorageTek and/or third-party use of
 *      the ACSLS API.
 *
 * Considerations:
 *
 *      Today, this header file is comprised of SEVERAL header
 *      files from the ACSLS product base.... it is intended
 *      to have this header file to be used in the product
 *      at a later date to remove duplicate effort.
 *
 * Modified by:
 *
 *   Ken Stickney         11/06/93       Original.
 *   Ken Stickney         06/23/94       Moved drive and media type defines
 *                                       from acsapi_pvt.h.
 *   Ken Stickney         08/31/94       Added RT_NONE, for return from 
 *                                       acs_response() when status is
 *                                       STATUS_PENDING in polling mode.
 *                                       Fix for BR#37.
 */

/* 
 * Definitions needed for the ACSAPI *
 */
typedef unsigned short SEQ_NO;
typedef MESSAGE_ID     REQ_ID;


/* packet version user wishes to implement */
#define ACSAPI_PACKET_VERSION "ACSAPI_PACKET_VERSION"

/* the name of the acslm test executable */ 
#define TEST_ACSLM "t_acslm"

/* the number of packet versions the toolkit supports */
#define NUM_RECENT_VERSIONS 3

/* media and drive types */
#define DRIVE_TYPE_4480 0
#define MEDIA_TYPE_3480 0

/* ACSAPI Response Types */
typedef  enum {
    RT_FIRST,
    RT_ACKNOWLEDGE,
    RT_INTERMEDIATE,
    RT_NONE,
    RT_FINAL,
    RT_LAST
} ACS_RESPONSE_TYPE;
#endif

