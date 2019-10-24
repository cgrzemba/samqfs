/* P4_Id - $Id: //depot/acsls6.0/h/api/structs_api.h#1 $ */

#ifndef _STRUCTS_API_
#define _STRUCTS_API_
/*
 * Copyright (2001, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      definitions of specific system data structures.  
 *      most of these structures are common to csi and acslm, 
 *      and are included by both header files.  This file contains
 *	definitions also needed by the ACSAPI.
 *
 * Modified by:
 *
 *	David A. Myers	    16-Sep-1993	    Split file with portion required
 *			    by ACSAPI into api/structs_api.h.  Also changed
 *			    hostid size to 12.
 *	Janet Borzuchowski  17-Sep-1993	    R5.0 Mixed Media-- Reduce 
 *			    QU_MAX_DRV_STATUS from 175 to 165 because the
 *			    query mount and query mount scratch responses are
 *			    over the 4K limit.  To further conserve space, 
 *			    put vol_id before drive_type in struct QU_DRV_STATUS
 *			    to eliminate padding.
 *	J. Borzuchowski	    05-Aug-1993	R5.0 Mixed Media-- 
 *			    Added media type to QU_CLN_STATUS, QU_SCR_STATUS, 
 *			    QU_VOL_STATUS; added drive type to QU_DRV_STATUS.
 *			    Added new select_criteria union member structures 
 *			    QU_ACS_CRITERIA, QU_LSM_CRITERIA, QU_CAP_CRITERIA,
 *			    QU_DRV_CRITERIA, QU_VOL_CRITERIA, QU_REQ_CRITERIA,
 *			    QU_PRT_CRITERIA, QU_POL_CRITERIA, QU_MSC_CRITERIA.
 *			    Added new response_status union member structures 
 *			    QU_SRV_RESPONSE, QU_ACS_RESPONSE, QU_LSM_RESPONSE,
 *			    QU_CAP_RESPONSE, QU_CLN_RESPONSE, QU_DRV_RESPONSE,
 *			    QU_MNT_RESPONSE, QU_VOL_RESPONSE, QU_PRT_RESPONSE,
 *			    QU_REQ_RESPONSE, QU_SCR_RESPONSE, QU_POL_RESPONSE,
 *			    QU_MSC_RESPONSE, QU_MMI_RESPONSE.
 *			    Added status structures for new query mixed 
 *			    media info request, QU_MEDIA_TYPE_STATUS, and
 *			    QU_DRIVE_TYPE_STATUS.
 *	J. Borzuchowski	    23-Aug-1993	R5.0 Mixed Media-- 
 *			    Changed drive_count in QU_MNT_RESPONSE and
 *			    QU_MSC_RESPONSE to mount_status_count and 
 *			    msc_status_count respectively.
 *	J. Borzuchowski     09-Sep-1993 R5.0 Mixed Media-- Changed 
 *                          counts in new Query structures from ints to shorts,
 *                          mistyped on 05-Aug-1993. 
 *	Emanuel A. Alongi   28-Oct-1993	    Moved IPC_HEADER structure and
 *			    related variables to api/ipc_hdr_api.h.
 *	J. Borzuchowski     02-Nov-1993 R5.0 Mixed Media-- Moved comments for
 *			    changes to structs in this file from ../structs.h.
 *	Emanuel A. Alongi   03-Nov-1993     Eliminated media_type array and 
 *			    media_type_count from the QU_MSC_CRITERIA structure;
 *			    the list was replaced with a single media_type.
 *	Howard L. Freeman   03-Apr-1997     Added Query/Switch LMU support(R5.2)
 *      Scott Siao          18-Oct-2001 Synchronized into toolkit.
 *      Scott Siao          12-Nov-2001 Added Display definitions.
 *      Scott Siao          04-Feb-2002 Added QU_SPN_CRITERIA,
 *                                      QU_DRG_CRITERIA,
 *                                      MAX_VTD_MAP,
 *                                      QU_VIRT_DRV_STATUS,
 *                                      QU_VIRT_DRV_MAP,
 *                                      QU_SUBPOOLNAME_STATUS,
 *                                      QU_DRG_RESPONSE,
 *                                      QU_SPN_RESPONSE,
 *                                      QU_MSC_PINFO_CRITERIA definitions
 */

#include "db_structs.h"
#include "defs.h"

/* 
 * maximum number of drive_status entries in mount, mount_scratch  
 * query responses.                 
 */
#define QU_MAX_DRV_STATUS   165         

/* 
 * maximum number of drive_status entires in query drive group 
 * responses.
 */                                                        
#define MAX_VTD_MAP         490 

typedef struct {                        /* message header */
    unsigned short  packet_id;          /*   client-specified */
    COMMAND         command;
    unsigned char   message_options;
    VERSION         version;
    unsigned long   extended_options;
    LOCKID          lock_id;
    ACCESSID        access_id;
    unsigned char   reserved[16];
} MESSAGE_HEADER;

typedef struct {                        /* response status */
    STATUS          status;
    TYPE            type;
    IDENTIFIER      identifier;
} RESPONSE_STATUS;

typedef struct {                        /* volume status */
    VOLID           vol_id;
    RESPONSE_STATUS status;
} VOLUME_STATUS;

typedef struct {                        /* audit ACS identifier status */
    ACS             acs_id;
    RESPONSE_STATUS status;
} AU_ACS_STATUS;

typedef struct {                        /* audit LSM identifier status */
    LSMID           lsm_id;
    RESPONSE_STATUS status;
} AU_LSM_STATUS;

typedef struct {                        /* audit panel identifier status */
    PANELID         panel_id;
    RESPONSE_STATUS status;
} AU_PNL_STATUS;

typedef struct {                        /* audit subpanel ident status */
    SUBPANELID      subpanel_id;
    RESPONSE_STATUS status;
} AU_SUB_STATUS;

typedef struct {                        /* lock request volume status */
    VOLID           vol_id;
    RESPONSE_STATUS status;
} LO_VOL_STATUS;

typedef struct {                        /* lock request drive status */
    DRIVEID         drive_id;
    RESPONSE_STATUS status;
} LO_DRV_STATUS;

typedef enum {                          /* Query request commands */
    AUDIT = 0,
    MOUNT,
    DISMOUNT,
    ENTER,
    EJECT,
    MAX_COMMANDS
} QU_COMMANDS;

typedef enum {                          /* query request dispostions */
    CURRENT = 0,
    PENDING,
    MAX_DISPOSITIONS
} QU_DISPOSITIONS;

typedef struct {                        /* request summary matrix */
    MESSAGE_ID  requests[MAX_COMMANDS][MAX_DISPOSITIONS];
} REQ_SUMMARY;

typedef struct {			/* query ACS request criteria    */
    unsigned short  acs_count;		/*   number of ACS identifiers   */
    ACS             acs_id[MAX_ID];	/*   ACS identifiers             */
} QU_ACS_CRITERIA;

typedef struct {			/* query LSM request criteria    */
    unsigned short  lsm_count;		/*   number of LSM identifiers   */
    LSMID           lsm_id[MAX_ID];	/*   LSM identifiers             */
} QU_LSM_CRITERIA;

typedef struct {			/* query CAP request criteria    */
    unsigned short  cap_count;		/*   number of CAP identifiers   */
    CAPID           cap_id[MAX_ID];	/*   CAP identifiers             */
} QU_CAP_CRITERIA;

typedef struct {			/* query drive request criteria   */
    unsigned short  drive_count;	/*   number of drive identifiers  */
    DRIVEID         drive_id[MAX_ID];	/*   drive identifiers            */
} QU_DRV_CRITERIA;

typedef struct {                    /* query drive group request criteria  */
    GROUP_TYPE      group_type;     /*   group type                        */
    unsigned short  drg_count;      /*   number of drive group identifiers */
    GROUPID         group_id[MAX_DRG];  /*   drive group identifiers       */
} QU_DRG_CRITERIA;

typedef struct {			/* query volume request criteria   */
    unsigned short  volume_count;	/*   number of volume identifiers  */
    VOLID           volume_id[MAX_ID];	/*   volume identifiers            */
} QU_VOL_CRITERIA;

typedef struct {			/* query message request criteria  */
    unsigned short  request_count;	/*   number of request identifiers */
    MESSAGE_ID      request_id[MAX_ID];	/*   request identifiers           */
} QU_REQ_CRITERIA;

typedef struct {			/* query port request criteria     */
    unsigned short  port_count;		/*   number of port identifiers    */
    PORTID          port_id[MAX_ID];	/*   port identifiers              */
} QU_PRT_CRITERIA;

typedef struct {			/* query pool request criteria     */
    unsigned short  pool_count;		/*   number of pool identifiers    */
    POOLID          pool_id[MAX_ID];	/*   pool identifiers              */
} QU_POL_CRITERIA;

typedef struct {			/* query mount scratch criteria    */
    MEDIA_TYPE      media_type;		/*   media type                    */
    unsigned short  pool_count;		/*   number of pool identifiers    */
    POOLID          pool_id[MAX_ID];	/*   pool identifiers              */
} QU_MSC_CRITERIA;

typedef struct {              /* query mount scratch enhanced criteria */
    MEDIA_TYPE      media_type;         /*   media type                */
    unsigned short  pool_count;         /*   number of pool ids        */
    POOLID          pool_id[MAX_ID];    /*   pool identifiers          */
    MGMT_CLAS       mgmt_clas;          /*   management class name     */
} QU_MSC_PINFO_CRITERIA;

typedef struct {			/* query LMU request criteria    */
    unsigned short  lmu_count;		/*   number of LMU identifiers   */
    ACS             lmu_id[MAX_ID];	/*   LMU identifiers             */
} QU_LMU_CRITERIA;

typedef struct {			/*query subpool name request criteria */
    unsigned short	spn_count;	/*number of subpool name identifiers*/
    SUBPOOL_NAME	subpl_name[MAX_SPN]; /*subpool name identifiers */
} QU_SPN_CRITERIA;

typedef struct {                        /* query ACS status */
    ACS             acs_id;             /*   ACS for status              */
    STATE           state;              /*   ACS state                   */
    FREECELLS       freecells;          /*   number of free cells in ACS */
    REQ_SUMMARY     requests;           /*   request summary for ACS     */
    STATUS          status;             /*   ACS status                  */
} QU_ACS_STATUS;

typedef struct {                        /* query clean status */
    VOLID           vol_id;             /*   volume identifier */
    MEDIA_TYPE	    media_type;		/*   media type of cleaning cartridge */
    CELLID          home_location;      /*   cell location of clean cartridge */
    unsigned short  max_use;            /*   number of uses allowed */
    unsigned short  current_use;        /*   current usage level */
    STATUS          status;             /*   status of cleaning cartridge */
} QU_CLN_STATUS;

typedef struct {                        /* query CAP status */
    CAPID           cap_id;             /*   CAP for status            */
    STATUS          status;             /*   CAP status                */
    CAP_PRIORITY    cap_priority;       /*   CAP priority number       */
    unsigned short  cap_size;           /*   number of cells in CAP    */
    STATE           cap_state;          /*   CAP state                 */
    CAP_MODE        cap_mode;           /*   CAP enter processing mode */
} QU_CAP_STATUS;

typedef struct {                        /* query drive status */
    DRIVEID         drive_id;           /*   drive for status              */
    VOLID           vol_id;             /*   volume if STATUS_DRIVE_IN_USE */
    DRIVE_TYPE	    drive_type;		/*   drive type of transport       */
    STATE           state;              /*   drive state                   */
    STATUS          status;             /*   drive status                  */
} QU_DRV_STATUS;

typedef struct {                        /* query virtual drive status   */ 
    VOLID           vol_id;             /*   volume on the drive        */
    STATE           state;              /*   drive state                */
    STATUS          status;             /*   drive status               */
    DRIVEID         drive_id;           /*   drive id                   */
} QU_VIRT_DRV_STATUS;

typedef struct {                        /* query virtual drive mapping  */
    DRIVEID         drive_id;           /*   drive id                   */
    unsigned short  drive_addr;         /*   drive unit address         */
} QU_VIRT_DRV_MAP;

typedef struct {                        /* query LSM status */
    LSMID           lsm_id;             /*   LSM for status              */
    STATE           state;              /*   LSM state                   */
    FREECELLS       freecells;          /*   number of free cells in LSM */
    REQ_SUMMARY     requests;           /*   request summary for LSM     */
    STATUS          status;             /*   LSM status                  */
} QU_LSM_STATUS;

typedef struct {                        /* query MOUNT status */
    VOLID           vol_id;             /*   volume for drive proximity list  */
    STATUS          status;             /*   volume status                    */
    unsigned short  drive_count;        /*   number of drive identifiers      */
    QU_DRV_STATUS   drive_status[QU_MAX_DRV_STATUS];
                                        /*   drive list in proximity order    */
} QU_MNT_STATUS;

typedef struct {                        /* query port status */
    PORTID          port_id;            /*   port for status         */
    STATE           state;              /*   port state              */
    STATUS          status;             /*   port status             */
} QU_PRT_STATUS;

typedef struct {                        /* query request status */
    MESSAGE_ID      request;            /*   request for status            */
    COMMAND         command;            /*   command from request_packet   */
    STATUS          status;             /*   request status                */
} QU_REQ_STATUS;

typedef struct {                        /* query server status */
    STATE           state;              /*   ACSLM state                     */
    FREECELLS       freecells;          /*   number of free cells in library */
    REQ_SUMMARY     requests;           /*   request summary for library     */
} QU_SRV_STATUS;

typedef struct {                        /* query volume status */
    VOLID           vol_id;             /*   volume for status            */
    MEDIA_TYPE	    media_type;		/*   media type of volume         */
    LOCATION        location_type;      /*   LOCATION_CELL or LOCATION_DRIVE */
    union {                             /*   current location of volume   */
        CELLID      cell_id;            /*     if STATUS_VOLUME_HOME      */
        DRIVEID     drive_id;           /*     if STATUS_VOLUME_IN_DRIVE  */
    } location;                         /*     undefined if none of above */
    STATUS          status;             /*   volume status                */
} QU_VOL_STATUS;

typedef struct {                        /* query scratch status */
    VOLID           vol_id;
    MEDIA_TYPE	    media_type;		/* media type of scratch volume   */
    CELLID          home_location;
    POOLID          pool_id;
    STATUS          status;
} QU_SCR_STATUS;

typedef struct {                        /* query pool status */
    POOLID          pool_id;
    unsigned long   volume_count;
    unsigned long   low_water_mark;
    unsigned long   high_water_mark;
    unsigned long   pool_attributes;
    STATUS          status;
} QU_POL_STATUS;

typedef struct {                        /* query subpool naume status */
    SUBPOOL_NAME    subpool_name;       /*   subpool name             */
    POOLID          pool_id;            /*   pool id                  */
    STATUS          status;             /*   pool status              */
} QU_SUBPOOL_NAME_STATUS;

typedef struct {                        /* query mount_scratch status */
    POOLID          pool_id;
    STATUS          status;
    unsigned short  drive_count;
    QU_DRV_STATUS   drive_list[QU_MAX_DRV_STATUS];
} QU_MSC_STATUS;

typedef struct {			/* query media type status for mm_info*/
  MEDIA_TYPE                    media_type;
  char                          media_type_name[MEDIA_TYPE_NAME_LEN + 1];
  CLN_CART_CAPABILITY           cleaning_cartridge;
  int                           max_cleaning_usage;
  unsigned short                compat_count;
  DRIVE_TYPE                    compat_drive_types[MM_MAX_COMPAT_TYPES];
} QU_MEDIA_TYPE_STATUS;

typedef struct {			/* query drive type status for mm_info*/
  DRIVE_TYPE                    drive_type;
  char                          drive_type_name[DRIVE_TYPE_NAME_LEN + 1];
  unsigned short                compat_count;
  MEDIA_TYPE                    compat_media_types[MM_MAX_COMPAT_TYPES];
} QU_DRIVE_TYPE_STATUS;

typedef struct {			/* query server response         */
    QU_SRV_STATUS   server_status;	/*   server status               */
} QU_SRV_RESPONSE;

typedef struct {			/* query ACS response            */
    unsigned short  acs_count;		/*   number of ACS identifiers   */
    QU_ACS_STATUS   acs_status[MAX_ID];	/*   ACS statuses                */
} QU_ACS_RESPONSE;

typedef struct {			/* query LSM response            */
    unsigned short  lsm_count;		/*   number of LSM statuses      */
    QU_LSM_STATUS   lsm_status[MAX_ID];	/*   LSM statuses                */
} QU_LSM_RESPONSE;

typedef struct {			/* query CAP response            */
    unsigned short  cap_count;		/*   number of CAP statuses      */
    QU_CAP_STATUS   cap_status[MAX_ID];	/*   CAP statuses                */
} QU_CAP_RESPONSE;

typedef struct {			/* query clean response           */
    unsigned short  volume_count;	/*   number of clean vol statuses */
    QU_CLN_STATUS   clean_volume_status[MAX_ID]; /* clean volume statuses */
} QU_CLN_RESPONSE;

typedef struct {			/* query drive response           */
    unsigned short  drive_count;	/*   number of drive statuses     */
    QU_DRV_STATUS   drive_status[MAX_ID];	/*   drive statuses       */
} QU_DRV_RESPONSE;

typedef struct {                        /* query drive group response     */
    GROUPID         group_id;           /*   drive group id               */
    GROUP_TYPE      group_type;         /*   drive group type             */
    unsigned short  vir_drv_map_count;  /*   number of drive statuses     */
    QU_VIRT_DRV_MAP virt_drv_map[MAX_VTD_MAP]; /* drive statuses          */
} QU_DRG_RESPONSE;

typedef struct {			/* query mount response           */
    unsigned short  mount_status_count;	/*   number of mount statuses     */
    QU_MNT_STATUS   mount_status[MAX_ID];	/*   mount statuses       */
} QU_MNT_RESPONSE;

typedef struct {			/* query volume response          */
    unsigned short  volume_count;	/*   number of volume statuses    */
    QU_VOL_STATUS   volume_status[MAX_ID];	/*   volume statuses      */
} QU_VOL_RESPONSE;

typedef struct {			/* query port response            */
    unsigned short  port_count;		/*   number of port statuses      */
    QU_PRT_STATUS   port_status[MAX_ID];	/*   port statuses        */
} QU_PRT_RESPONSE;

typedef struct {			/* query message response         */
    unsigned short  request_count;	/*   number of request statuses   */
    QU_REQ_STATUS   request_status[MAX_ID];	/*   request statuses     */
} QU_REQ_RESPONSE;

typedef struct {			/* query scratch response         */
    unsigned short  volume_count;	/*   number of scratch statuses   */
    QU_SCR_STATUS   scratch_status[MAX_ID];	/*   scratch statuses     */
} QU_SCR_RESPONSE;

typedef struct {			/* query pool response            */
    unsigned short  pool_count;		/*   number of pool statuses      */
    QU_POL_STATUS   pool_status[MAX_ID];	/*   pool statuses        */
} QU_POL_RESPONSE;

typedef struct {                        /* query subpool name response    */
    unsigned short  spn_status_count;   /*   number of subpl name statuses*/
    QU_SUBPOOL_NAME_STATUS subpl_name_status[MAX_ID]; /* subpl name status */
} QU_SPN_RESPONSE;

typedef struct {			/* query mount scratch response    */
    unsigned short  msc_status_count;	/*   number of mount scr statuses  */
    QU_MSC_STATUS   mount_scratch_status[MAX_ID];/*   msc statuses         */
} QU_MSC_RESPONSE;

typedef struct {			/* query mixed media info response */
    unsigned short          media_type_count; 	    /*number of media types*/
    QU_MEDIA_TYPE_STATUS    media_type_status[MAX_ID];	/* media types     */
    unsigned short          drive_type_count;	    /*number of drive types*/
    QU_DRIVE_TYPE_STATUS    drive_type_status[MAX_ID];	/* drive types     */
} QU_MMI_RESPONSE;

typedef struct {                        /* query_lock volume status */
    VOLID           vol_id;
    LOCKID          lock_id;
    unsigned long   lock_duration;
    unsigned int    locks_pending;
    USERID          user_id;
    STATUS          status;
} QL_VOL_STATUS; 

typedef struct {                        /* query_lock drive status */
    DRIVEID         drive_id;
    LOCKID          lock_id;
    unsigned long   lock_duration;
    unsigned int    locks_pending;
    USERID          user_id;
    STATUS          status;
} QL_DRV_STATUS;

typedef struct {                        /* vary ACS status */
    ACS             acs_id;
    RESPONSE_STATUS status;
} VA_ACS_STATUS;

typedef struct {                        /* vary drive status */
    DRIVEID         drive_id;
    RESPONSE_STATUS status;
} VA_DRV_STATUS;

typedef struct {                        /* vary LSM status */
    LSMID           lsm_id;
    RESPONSE_STATUS status;
} VA_LSM_STATUS;

typedef struct {                        /* vary CAP status */
    CAPID           cap_id;
    RESPONSE_STATUS status;
} VA_CAP_STATUS;

typedef struct {                        /* vary port status */
    PORTID          port_id;
    RESPONSE_STATUS status;
} VA_PRT_STATUS;

typedef struct {
    PORT_RECORD     prt_record;
    ROLE            role;                  /* lmu role */
    int             compat_level;          /* Host/LMU Compat Level */
    STATE           lmu_port_diag;
} LMU_PORT_RECORD;

typedef struct {                           /* query LMU status */
    STATUS          status;                /* status of ACS id */
    ACS             acs_id;                /* ACS for status */
    STATE           state;                 /* ACS state */
    int             prt_count;             /* Number of prt records present */
    STATUS          standby_status;        /* Master's status of Standby */
    STATUS          master_status;         /* Master's status */
    MODE            mode;                  /* lmu mode */
    LMU_PORT_RECORD lmu_record[MAX_PORTS]; /* Port information */
} QU_LMU_STATUS;

typedef struct {			   /* query LMU response */
    unsigned short  lmu_count;		   /* number of LMU identifiers */
    QU_LMU_STATUS   lmu_status[MAX_ID];   /* LMU statuses */
} QU_LMU_RESPONSE;

typedef struct {                           /* query LMU status */
    STATUS          status;                /* status of ACS id */
    ACS             lmu_id;                /* ACS for status */
} SW_LMU_STATUS;

/* The size of status, type and identifier should be encorperated in the 
   size of RESPONE_STATUS, but in the present toolkit environment that 
   struct is not accessable from here so it is broken down to the elements 
   of the structure. The 2nd TYPE is for the type used in the display
   request/response and the last 4 is for the difference between 
   CSI_MSGBUF and IPC_HEADER. The sizeof onsigned short is the length
   field in the DISP_XML structure*/


#define MAX_XML_DATA_SIZE (MAX_MESSAGE_SIZE-                   \
		       (                          \
			  (sizeof(IPC_HEADER) +4) \
		         + sizeof(MESSAGE_HEADER) \
		         + (sizeof(STATUS)        \
		         + sizeof(TYPE)           \
		         + sizeof(IDENTIFIER))    \
			 + sizeof(TYPE)           \
			 + sizeof(unsigned short)))

typedef struct {
    unsigned short     length;
    char               xml_data[MAX_XML_DATA_SIZE];
} DISPLAY_XML_DATA;

#endif /* _STRUCTS_API_ */
