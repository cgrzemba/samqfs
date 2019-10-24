/* SccsId @(#)db_structs.h	1.2 1/11/94  */
#ifndef _DB_STRUCTS_
#define _DB_STRUCTS_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header file defines system-wide data structures used in database
 *      transactions.  Due to limitations of the embedded SQL preprocessor, it
 *      is necessary to separate definitions required by the SQL preprocessor
 *      because it does not handle all C preprocessor statements nor some C
 *      constructs.
 *
 *      In embedded SQL (.sc) modules, this header file must be included
 *      before defs.h by an "EXEC SQL include" statement.  For example:
 *
 *          EXEC SQL include sqlca;             // SQL communications area
 *          EXEC SQL begin declare section;     // include in order shown
 *              EXEC SQL include "../h/db_defs.h";
 *              EXEC SQL include "../h/identifier.h";
 *              EXEC SQL include "../h/db_structs.h";
 *          EXEC SQL end declare section;
 *
 *          #include "defs.h"
 *          #include "structs.h"
 *
 *      This file also gets included by other header files if _DB_STRUCTS_ is
 *      not yet defined and those header files include a #ifndef/#endif pair
 *      around the #include for this header file.  For example, structs.h
 *      contains:
 *
 *          #ifndef _DB_STRUCTS_
 *          #include "db_structs.h"
 *          #endif  _DB_STRUCTS_
 *
 *      Due to the limitations of the INGRES embedded SQL preprocessor:
 *
 *        o #define definitions must be of the simple form below and must have
 *          have a replacement value.  Not specifying the value will result in
 *          the preprocessor flagging the line as an error.
 *
 *              #define name    value
 *
 *        o Do NOT specify #define values as expressions; they must be numeric
 *          or string constants.  For example:
 *
 *              #define MAX_TROUT   (MAX_RAINBOW + MAX_BROOK)   // WRONG !!
 *
 *          will be flagged as an error by the preprocessor.
 *
 *        o Do NOT include any compiler directive other #define in this header
 *          file.  The preprocessor will flag those lines as errors.  Because of *          this limitation, this header file is excluded from the #ifdef/#endif *          standard to limit header file inclusion.  However this means that
 *          header files including this one should have #ifdef/#endif pairs 
 *          surrounding the #include to avoid generating duplicate definitions.
 *
 *        o Do not include "unsigned" in the data type of a definition.  It is
 *          not supported by the preprocessor and will be flagged as an error.
 *
 * Modified by:
 *
 *      D. F. Reed          22-Sep-1988     Original.
 *      D. A. Beidle        08-Jan-1990     Implement home location feature by
 *                      redefining VOLUME_RECORD declaration.
 *      J. W. Montgomery    03-Mar-1990     Added scratch processing fields to
 *                      VOLUME_RECORD.  Created POOL_RECORD.
 *      D. L. Trachy        23-Apr-1990     Added lock server processing fields
 *                      to VOLUME_RECORD, DRIVE_RECORD.  Created LOCKID_RECORD.
 *      H. I. Grapek        26-Apr-1998     Added set_clean processing field
 *                      and max_use to VOLUME_RECORD
 *      J. A. Wishner       30-May-1990     Added label_attr to VOLUME_RECORD.
 *      D. A. Beidle        27-Jun-1990     Added CAP enhancements support.
 *      D. A. Beidle        06-Jul-1990     Completed CAP enhancements support.
 *      D. L. Trachy        04-Sep-1990     Added CSI_RECORD
 *      J.S. Alexander      17-Jun-1991.    Added new fields to CAP_RECORD to 
 *                      support release 3.0.
 *      D. A. Beidle        27-Sep-1991     Added lots of additional comments
 *                      to explain why we don't make this header file easier
 *                      to use or understand.
 *      Alec Sharp          20-Aug-1992     Added VAC_RECORD.
 *      David A. Myers	    17-Aug-1993     Updated copyright year as per code
 *			review.  Original changes to add media type to volume
 *			record, and drive type to drive record.
 */

/*
 *      Header Files:
 */

#ifndef _IDENTIFIER_
#include "identifier.h"
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */

typedef struct {                        /* ACSTABLE record */
    ACS             acs;
    STATE           acs_state;
} ACS_RECORD;

typedef struct {                        /* LSMTABLE record */
    LSMID           lsm_id;
    STATE           lsm_state;
    STATUS          lsm_status;
    int             lsm_activity;
    PANEL           last_panel;
    ROW             last_row;
    COL             last_col;
    LSM             lsm_ptp_1;
    LSM             lsm_ptp_2;
    LSM             lsm_ptp_3;
    LSM             lsm_ptp_4;
} LSM_RECORD;

typedef struct {                        /* CAPTABLE record */
    CAPID           cap_id;
    STATUS          cap_status;
    CAP_PRIORITY    cap_priority;
    STATE           cap_state;
    CAP_MODE        cap_mode;
    short           cap_size;
} CAP_RECORD;

typedef struct {                        /* CELLTABLE record */
    CELLID          cell_id;
    STATUS          cell_status;
} CELL_RECORD;

typedef struct {                        /* DRIVETABLE record */
    DRIVEID         drive_id;
    STATUS          drive_status;
    STATE           drive_state;
    VOLID           vol_id;
    LOCKID          lock_id;
    long            lock_time;
    DRIVE_TYPE	    drive_type;
} DRIVE_RECORD;

typedef struct {                        /* PORTTABLE record */
    PORTID          port_id;
    STATE           port_state;
    char            port_name[PORT_NAME_SIZE + 1];
} PORT_RECORD;

typedef struct {                        /* VOLUMETABLE record */
    VOLID           vol_id;
    CELLID          cell_id;
    DRIVEID         drive_id;
    VOLUME_TYPE     vol_type;
    LABEL_ATTR      label_attr;
    POOLID          pool_id;
    STATUS          vol_status;
    long            entry_date;
    long            access_date;
    long            access_count;
    long            max_use;
    LOCKID          lock_id;
    long            lock_time;
    MEDIA_TYPE	    media_type;
} VOLUME_RECORD;
 
typedef struct {                  /* VACTABLE record - Volume Access Control */
    VOLID           vol_id;
    USERID          owner_id;
} VAC_RECORD;
                                        /* POOL record */
typedef struct {
    POOLID          pool_id;
    long            low_water_mark;
    long            high_water_mark;
    int             pool_attributes;
} POOL_RECORD;

                                        /* CSI record */
typedef struct {
    char            csi_name[CSI_NAME_LENGTH+1];
} CSI_RECORD;

typedef enum {                          /* audited volume table codes */
    AVT_FIRST = 0,                      /* illegal */
    AVT_FOUND,
    AVT_NORMAL,
    AVT_LAST                            /* illegal */
} AVT_STATUS;

typedef struct {                        /* AVTpid record                     */
    short           audit_pid;          /*   process ID creating record      */
    VOLID           vol_id;             /*   volume to audit (secondary key) */
    AVT_STATUS      avt_status;         /*   status of volume                */
} AVT_RECORD;

typedef struct {
    LOCKID          lock_id;
    USERID          user_id;
} LOCKID_RECORD;

#endif /* _DB_STRUCTS_ */

