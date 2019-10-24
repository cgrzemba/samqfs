/* SccsId @(#)db_defs.h	1.2 1/11/94  */
#ifndef _DB_DEFS_
#define _DB_DEFS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header file defines system-wide data definitions used in database
 *      transactions and ACSLS subsystem interfaces (i.e., ACSLM programmatic 
 *      interface).
 *
 * Modified by:
 *
 *      D. F. Reed          06-Oct-1988     Original.
 *
 *      many lines deleted....
 *
 *      D. A. Beidle        18-Oct-1991.    Define MIN_PRIORITY as lowest
 *                      non-zero CAP priority.
 *      D. A. Beidle        14-Apr-1992     BR#475, BR#487 - Changed definition
 *                      of DB_DEADLOCK to -49900 which is the generic deadlock
 *                      error code returned by Ingres.
 *      Alec Sharp          22-May-1992     U4-MR1,MR2,MR6,MR11.  Added
 *                      REQUEST_PRIORITY, PCAP_PRIORITY, PCAP_SIZE, PANEL_TYPE.
 *                      Changed values of DATA_BASE, ANY_XXX, SAME_XXX,
 *                      ALL_XXX, MAX_LSM, MAX_ROW. Note that there are no
 *                      parentheses around the -1 and -2 values because the
 *                      Ingres preprocessor has a problem with the parentheses.
 *	C. N. Hobbie	    14-Jul-1992	    Added symbolic constants for
 *			the storage structure parameters of the various
 *			database tables.
 *      D. A. Myers         24-Aug-1992     Added definitions for volume
 *                      access table (VAC).
 *      Alec Sharp          04-Oct-1992     Added STATUS_COMMAND_ACCESS_DENIED,
 *                      STATUS_VOLUME_ACCESS_DENIED, STATUS_OWNER_NOT_FOUND
 *      E. A. Alongi    changed STATUS_RPC_FAILURE #define to an equivalent of
 *			STATUS_NI_FAILURE.
 *      David A. Myers	    08-Jun-1993	    Added MEDIA_TYPE and DRIVE_TYPE
 *			typedefs and corresponding defines.
 *      T. Z. Khawaja	    13-Jul-1993	    Added STATUS_INVALID_DRIVE_TYPE 
 *			and STATUS_INVALID_MEDIA_TYPE
 *	C. A. Paul	    02-Aug-1993	    Added
 *			STATUS_INCOMPATIBLE_MEDIA_TYPE.
 *      David A. Myers	    17-Aug-1993	    Updated copyright year as per code
 *			review.
 *	David Farmer	    25-Aug-1993	    changed char to SIGNED char 
 *					    where negtive values are used.
 *      Alec Sharp          07-Sep-1993     R5.0 HOSTID structure
 *	David A. Myers	    16-Sep-1993	    Split file with portion required
 *			by ACSAPI into api/db_defs_api.h
 */

/*
 *      Header Files:
 */
#ifndef _DB_DEFS_API_
#include "api/db_defs_api.h"
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */

#define DATA_BASE       "lib5"          /* data base name */

/* Negative values are used to represent panel types which are not explicitly
 * specified by the LMU.  These are the ACS4400 inner panels.  There are three
 * types of inner panels.  This leads to MIN_PANEL_TYPE being defined as -3.
 */
#define MIN_PANEL_TYPE	-3

#define MIN_TAPE_USAGE  0

/* Volume table storage structure */

#define VOLUME_TABLE_FILLFACTOR		20
#define VOLUME_TABLE_INDEXFILL		20
#define VOLUME_TABLE_MAXINDEXFILL	24

/* Volume access control table storage structure */

#define VAC_TABLE_FILLFACTOR		26
#define VAC_TABLE_INDEXFILL		26
#define VAC_TABLE_MAXINDEXFILL		31


/* Cell table storage structure */

#define CELL_TABLE_FILLFACTOR		46
#define CELL_TABLE_INDEXFILL		20
#define CELL_TABLE_MAXINDEXFILL		24


/* Audit table storage structure */

#define AUDIT_TABLE_FILLFACTOR		32
#define AUDIT_TABLE_INDEXFILL		40
#define AUDIT_TABLE_MAXINDEXFILL	46


/* LSM table storage structure */

#define LSM_TABLE_FILLFACTOR		1


/* ACS table storage structure */

#define ACS_TABLE_FILLFACTOR		1


/* Drive table storage structure */

#define DRIVE_TABLE_FILLFACTOR		1


/* Lock ID table storage structure */

#define LOCKID_TABLE_MINPAGES		280
#define LOCKID_TABLE_FILLFACTOR		25


/* CAP table storage structure */

#define CAP_TABLE_FILLFACTOR		1


/* Pool table storage structure */

#define POOL_TABLE_FILLFACTOR		20
#define POOL_TABLE_INDEXFILL		20
#define POOL_TABLE_MAXINDEXFILL		24


/* CSI table storage structure */

#define CSI_TABLE_FILLFACTOR		1


/* Port table storage structure */

#define PORT_TABLE_FILLFACTOR		1


/* Panel table storage structure */

#define PANEL_TABLE_FILLFACTOR		20
#define PANEL_TABLE_INDEXFILL		20
#define PANEL_TABLE_MAXINDEXFILL	24



#endif /* _DB_DEFS_ */

