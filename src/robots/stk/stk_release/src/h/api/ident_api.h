/* SccsId @(#)ident_api.h	2.2 10/21/01  */
#ifndef _IDENT_API_H_
#define _IDENT_API_H_
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This header file defines system-wide data structures used in database
 *      transactions and ACSLS subsystem interfaces (i.e., ACSLM programmatic 
 *      interface).  Due to limitations of the embedded SQL preprocessor, it
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
 *      This file also gets included by other header files if _IDENTIFIER_ is
 *      not yet defined and those header files include a #ifndef/#endif pair
 *      around the #include for this header file.  For example, structs.h
 *      contains:
 *
 *          #ifndef _IDENTIFIER_
 *          #include "identifier.h"
 *          #endif  _IDENTIFIER_
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
 *              #define MAX_TROUT   (MAX_RAINBOW + MAX_BROOK)   // WRONG!!
 *
 *          will be flagged as an error by the preprocessor.
 *
 *        o Do NOT include any compiler directive other #define in this header
 *          file.  The preprocessor will flag those lines as errors.  Because of
 *          this limitation, this header file is excluded from the #ifdef/#endif
 *          standard to limit header file inclusion.  However this means that
 *          header files including this one should have #ifdef/#endif pairs 
 *          surrounding the #include to avoid generating duplicate definitions.
 *
 *        o Do not include "unsigned" in the data type of a definition.  It is
 *          not supported by the preprocessor and will be flagged as an error.
 *
 * Modified by:
 *
 *      D. F. Reed          27-Jan-1989     Original.
 *      J. W. Montgomery    06-Mar-1990     Added POOLID and VOLRANGE.
 *      D. L. Trachy        10-Sep-1990     Added lock_id to IDENTIFIER
 *      J. S. Alexander     18-Jun-1991     Added CAPID, and CAP_CELLID.
 *      D. A. Beidle        10-Sep-1991     Removed CAP_CELLID.
 *      H. I. Grapek        23-Sep-1991     Added V0_CAPID and V1_CAPID defs,
 *                      Added V0_CAPID and V1_CAPID to IDENTIFIERS.
 *      J. S. Alexander     27-Sep-1991     Added define IDENTIFIER_SIZE.
 *      D. A. Beidle        27-Sep-1991     Added lots of additional comments
 *                      to explain why we don't make this header file easier
 *                      to use or understand.
 *      E. A. Alongi        26-Oct-1992     Added define ALIGNMENT_PAD_SIZE
 *                      which is used in typedef union IDENTIFIER and other
 *                      source code.
 *	J. Borzuchowski	    05-Aug-1993	    R5.0 Mixed Media--  Added
 *			MEDIA_TYPE and DRIVE_TYPE to IDENTIFER union.
 *      S. Siao             09-Oct-2001     Added Registrationid, handid for
 *                                          event notification.
 *      S. Siao             05-Dec-2001     Added typdef for PTPID for event
 *                                          notification.
 *      S. Siao             23-Jan-2002     Added typdefs for:
 *					     VIRTUAL_TAPE_DRIVE;
 *					     VTDID,
 *					     MGMT_CLAS,
 *					     SUBPOOL_NAME,
 *					     JOB_NAME,
 *					     STEP_NAME,
 *					     DATASET_NAME,
 *					     GROUPID for Virtual support.
 *                                          Also added these types into 
 *                                          the IDENTIFIER union.
 *      S. Siao             06-Feb-2002     Added 1 to array for 
 *                                          registration.
 *      S. Siao             01-Mar-2002     Changed VOL_MANUALLY_DELETED to
 *                                          VOL_DELETED.
 *      S. Siao             26-Apr-2002     In EVENT_REGISTER_STATUS changed
 *                                          MAX_ID to MAX_REGISTER_STATUS 
 *                                          also added definition of MAX_REGISTER_
 *                                          STATUS to 2.
 *      S. Siao             20-May-2002     Changed MAX_REGISTER_STATUS to
 *                                          MAX_EVENT_CLASS_TYPE.
 *      Wipro (Subhash)     28-May-2004     Added data structures needed to
 *                                          support mount/dismount events.
 *      Wipro (Hemendra)    18-Jun-2004     Modified DRIVE_ACTIVITY_DATA
 *						start_time & completion_time are 
 *						made long
 *	Mitch Black         22-Nov-2004     Modified definition of PTPID to match 
 *                                          new ACSLS (SL8500-based) definition.
 *      Mitch Black         24-Nov-2004     Fixed HANDID member to be panel_id 
 *                                              (rather than "panel").
 *	Mitch Black	    29-Dec-2004	    Added new volume events to enum.
 */

/*
 *      Header Files:
 */

/*
 *      Defines, Typedefs and Structure Definitions:
 */

#define IDENTIFIER_SIZE       64       /* length of largest identifier */
#define REGISTRATION_ID_SIZE  32       /* length of registration identifier */

typedef struct {
    ACS             acs;
    LSM             lsm;
} LSMID;

typedef LSMID       V0_CAPID;           /* CAP ID in ACSLM VERSION0 packet  */
typedef LSMID       V1_CAPID;           /* CAP ID in ACSLM VERSION1 packet  */

typedef struct {
    LSMID           lsm_id;
    CAP             cap;
} CAPID;

typedef struct {
    ACS             acs;
    PORT            port;
} PORTID;

typedef struct {
    LSMID           lsm_id;
    PANEL           panel;
} PANELID;

typedef struct {
    ACS      acs;
    LSM      master_lsm;
    PANEL    master_panel;
    LSM      slave_lsm;
    PANEL    slave_panel;
} PTPID;

typedef struct {
    PANELID         panel_id;
    ROW             begin_row;
    COL             begin_col;
    ROW             end_row;
    COL             end_col;
} SUBPANELID;

typedef struct {
    PANELID         panel_id;
    DRIVE           drive;
} DRIVEID;

typedef struct {
    PANELID         panel_id;
    ROW             row;
    COL             col;
} CELLID;

typedef struct {
    char            external_label[EXTERNAL_LABEL_SIZE + 1];
} VOLID;

typedef struct {
    POOL            pool;
} POOLID;

typedef struct {
    VOLID           startvol;
    VOLID           endvol;
} VOLRANGE;

typedef struct {
    PANEL           panel;
    DRIVE           drive;
} VIRTUAL_TAPE_DRIVE;


typedef struct {
    ACS                acs;
    LSM                lsm;
    VIRTUAL_TAPE_DRIVE vtd;
} VTDID;                         /* 4-byte VTD drive id */

typedef struct {
    char            mgmt_clas[MGMT_CLAS_SIZE + 1];
} MGMT_CLAS;

typedef struct {
    char            subpool_name[SUBPOOL_NAME_SIZE + 1];
} SUBPOOL_NAME;

typedef struct {
    char            job_name[JOB_NAME_SIZE + 1];
} JOB_NAME;

typedef struct {
    char            step_name[STEP_NAME_SIZE + 1];
} STEP_NAME;

typedef struct {
    char            dataset_name[DATASET_NAME_SIZE + 1];
} DATASET_NAME;

typedef struct {
    char            groupid[GROUPID_SIZE + 1];
} GROUPID;


typedef struct {
    char            registration[REGISTRATION_ID_SIZE + 1];
} REGISTRATION_ID;

typedef enum {
    EVENT_REGISTER_FIRST = 0,
    EVENT_REGISTER_REGISTERED,
    EVENT_REGISTER_UNREGISTERED,
    EVENT_REGISTER_INVALID_CLASS,
    EVENT_REGISTER_LAST
} EVENT_CLASS_REGISTER_RETURN;

typedef struct {
    EVENT_CLASS_TYPE             event_class;
    EVENT_CLASS_REGISTER_RETURN  register_return;
} REGISTER_STATUS;

typedef enum {
    VOL_FIRST=0,
    VOL_ENTERED,
    VOL_ADDED,
    VOL_REACTIVATED,
    VOL_EJECTED,
    VOL_DELETED,
    VOL_MARKED_ABSENT,
    VOL_OVER_MAX_CLEAN,
    VOL_CLEAN_CART_SPENT,
    VOL_HOME_LSM_CHG,
    VOL_OWNER_CHG,
    VOL_LAST
} VOL_EVENT_TYPE;

typedef struct {
    VOL_EVENT_TYPE               event_type;
    VOLID                        vol_id;
} EVENT_VOLUME_STATUS;

typedef struct {
    PANELID         panel_id;
    HAND            hand;
} HANDID;

#define ALIGNMENT_PAD_SIZE  32       /* used in forcing typedef union
				      * IDENTIFIER on a full word boundary
				      * - for the purposes of dealing with
				      * other architectures; e.g., TANDEM */
     
/*
 *   Warning.  "alignment_size" forces TYPE_NONE identifiers to be unpacked
 *   from network as a predetermined "Sun-defined" size regardless of host
 *   architecture involved on the client end.  If this data structure is
 *   altered, make sure that "alignment_size" is greater than or equal to
 *   the largest size of any other data structures in IDENTIFIER.  The value
 *   ALIGNMENT_PAD_SIZE (which is defined above as 32) is assigned since it is
 *   hopefully large enough and a power of 2, so that the addition of future
 *   identifiers will not cause alignment problems with customer compilers.
 *
 *   Note:  DATASET_NAME was expressly NOT added to the IDENTIFIER union or
 *          the TYPE enum in defs_api.h so as not to cause the
 *          ALIGNMENT_PAD_SIZE to need to be changed.  Changing this pad size
 *          could cause unpredictable results.
 */

typedef union {
    ACS            acs_id;
    V0_CAPID       v0_cap_id;
    V1_CAPID       v1_cap_id;
    CAPID          cap_id;              /* latest CAP identifier format */
    CELLID         cell_id;
    DRIVEID        drive_id;
    LSMID          lsm_id;
    PANELID        panel_id;
    PORTID         port_id;
    SUBPANELID     subpanel_id;
    VOLID          vol_id;
    POOLID         pool_id;
    LOCKID         lock_id;
    char           socket_name[SOCKET_NAME_SIZE];
    long           request;             /* really a MESSAGE_ID  */
    short          lh_error;            /* really a LH_ERR_TYPE */
    MEDIA_TYPE     media_type;          /* cartridge media type */
    DRIVE_TYPE     drive_type;          /* ACSLS drive type     */
    HANDID         hand_id;
    PTPID          ptp_id;
    VTDID          vtd_id;
    SUBPOOL_NAME   subpool_name;
    MGMT_CLAS      mgmt_clas;           /* management class     */
    JOB_NAME       job_name;
    STEP_NAME      step_name;
    GROUPID        groupid;             /* VTSS name             */
    char           alignment_size[ALIGNMENT_PAD_SIZE];  /* alignment for other
							 * architectures */
} IDENTIFIER;

typedef enum {
    SENSE_TYPE_FIRST=0,
    SENSE_TYPE_NONE,
    SENSE_TYPE_HLI,
    SENSE_TYPE_SCSI,
    SENSE_TYPE_FSC,
    RESOURCE_CHANGE_SERIAL_NUM,
    RESOURCE_CHANGE_LSM_TYPE,
    RESOURCE_CHANGE_DRIVE_TYPE,
    DRIVE_ACTIVITY_DATA_TYPE,
    SENSE_TYPE_LAST
} RESOURCE_DATA_TYPE;

typedef struct {
    long          start_time;
    long          completion_time;
    VOLID         vol_id;
    VOLUME_TYPE   volume_type;
    DRIVEID       drive_id;
    POOLID        pool_id;
    CELLID        home_location;
} DRIVE_ACTIVITY_DATA;

typedef union {
    SENSE_HLI            sense_hli;
    SENSE_SCSI           sense_scsi;
    SENSE_FSC            sense_fsc;
    SERIAL_NUM           serial_num;
    LSM_TYPE             lsm_type;
    DRIVE_TYPE           drive_type;
    DRIVE_ACTIVITY_DATA  drive_activity_data;
    char                 resource_align_pad[RESOURCE_ALIGN_PAD_SIZE];
} RESOURCE_DATA;

typedef struct {
    TYPE                         resource_type;
    IDENTIFIER                   resource_identifier;
    RESOURCE_EVENT               resource_event;
    RESOURCE_DATA_TYPE           resource_data_type;
    RESOURCE_DATA                resource_data;
} EVENT_RESOURCE_STATUS;

typedef struct {
    TYPE                event_type;
    RESOURCE_DATA_TYPE  resource_data_type;
    RESOURCE_DATA       resource_data;
} EVENT_DRIVE_STATUS;

#define MAX_EVENT_CLASS_TYPE 3

typedef struct {
   REGISTRATION_ID               registration_id;
   unsigned short                count;
   REGISTER_STATUS               register_status[MAX_EVENT_CLASS_TYPE];
} EVENT_REGISTER_STATUS;

#define EVENT_ALIGN_PAD_SIZE  128

typedef union {
    EVENT_REGISTER_STATUS        event_register_status;
    EVENT_VOLUME_STATUS          event_volume_status;
    EVENT_RESOURCE_STATUS        event_resource_status;
    EVENT_DRIVE_STATUS           event_drive_status;
    char                         event_align_pad[EVENT_ALIGN_PAD_SIZE];
} EVENT;

#endif /* _IDENTIFIER_ */

