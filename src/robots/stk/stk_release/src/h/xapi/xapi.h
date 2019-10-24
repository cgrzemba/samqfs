/** HEADER FILE PROLOGUE *********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      xapi.h                                           */
/** Description:    XAPI client common header file.                  */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720/CDK240  Joseph Nofi     05/11/11                          */
/**     Created for CDK to add XAPI support.                         */
/** I6087283       Joseph Nofi     01/08/13                          */
/**     Fix high CPU utilization in srvlogs and srvtrcs daemons      */
/**     due to unblocked read processing.                            */
/**                                                                  */
/** END PROLOGUE *****************************************************/

#ifndef XAPI_HEADER
#define XAPI_HEADER

#include <semaphore.h>
#include <time.h>
#include <sys/types.h>


/*********************************************************************/
/* XAPI extern function name mapping:                                */
/*********************************************************************/
//#define xapi_attach_xapicvt                  xapi_attach1
//#define xapi_attach_xapicfg                  xapi_attach2
//#define xapi_attach_work_xapireqe            xapi_attach3
//#define xapi_audit                           xapi_audit1
//#define xapi_cancel                          xapi_cancel1
//#define xapi_check_reg                       xapi_check_reg1
//#define xapi_clr_lock.c                      xapi_clr_lock1
//#define xapi_clr_lock_drv.c                  xapi_clr_lock_drv1
//#define xapi_clr_lock_vol.c                  xapi_clr_lock_vol1
//#define xapi_config                          xapi_config1
//#define xapi_config_search_drivelocid        xapi_config_search1
//#define xapi_config_search_hexdevaddr        xapi_config_search2
//#define xapi_config_search_libdrvid          xapi_config_search3
//#define xapi_config_search_vtss              xapi_config_search4
//#define xapi_define_pool                     xapi_define_pool1
//#define xapi_delete_pool                     xapi_delete_pool1
//#define xapi_dismount                        xapi_dismount1
//#define xapi_display                         xapi_display1
//#define xapi_validate_single_drive           xapi_drive1
//#define xapi_validate_drive_range            xapi_drive2
//#define xapi_drive_increment                 xapi_drive3
//#define xapi_drive_list                      xapi_drive_list.c 
//#define xapi_drvtyp                          xapi_drvtyp1
//#define xapi_drvtyp_search_name              xapi_drvtyp_search1
//#define xapi_drvtyp_search_type              xapi_drvtyp_search2
//#define xapi_eject                           xapi_eject1
//#define xapi_eject_response                  xapi_eject2
//#define xapi_enter                           xapi_enter1
//#define xapi_idle                            xapi_idle1
//#define xapi_idle_test                       xapi_idle_test1
//#define xapi_lock                            xapi_lock1
//#define xapi_lock_drv                        xapi_lock_drv1
//#define xapi_lock_init_resp                  xapi_lock_init_resp1
//#define xapi_lock_vol                        xapi_lock_vol1
//#define xapi_main                            xapi_main1
//#define xapi_media                           xapi_media1
//#define xapi_media_search_name               xapi_media_search1
//#define xapi_media_search_type               xapi_media_search2
//#define xapi_mount                           xapi_mount1
//#define xapi_mount_pinfo                     xapi_mount_pinfo1
//#define xapi_mount_scr                       xapi_mount_scr1
//#define xapi_parse_header_trailer            xapi_parse_common1
//#define xapi_parse_loctype_rawvolume         xapi_parse_common2
//#define xapi_parse_rectechs_xmlparse         xapi_parse_common3
//#define xapi_qacs                            xapi_qacs1
//#define xapi_qcap                            xapi_qcap1
//#define xapi_qdrv                            xapi_qdrv1
//#define xapi_qdrv_group                      xapi_qdrv_group1
//#define xapi_qfree                           xapi_qfree1
//#define xapi_qlock                           xapi_qlock1
//#define xapi_qlock_drv                       xapi_qlock_drv1
//#define xapi_qlock_drv_one                   xapi_qlock_drv_one1
//#define xapi_qlock_init_resp                 xapi_qlock_init_resp1
//#define xapi_qlock_vol                       xapi_qlock_vol1
//#define xapi_qlock_vol_one                   xapi_qlock_vol_one1
//#define xapi_qlsm                            xapi_qlsm1
//#define xapi_qmedia                          xapi_qmedia1
//#define xapi_qmnt_one                        xapi_qmnt_one1
//#define xapi_qmnt_pinfo                      xapi_qmnt_pinfo1
//#define xapi_qmnt_scr                        xapi_qmnt_scr1
//#define xapi_qmnt                            xapi_qmnt1
//#define xapi_qpool                           xapi_qpool1
//#define xapi_qrequest                        xapi_qrequest1
//#define xapi_qscr                            xapi_qscr1
//#define xapi_qserver                         xapi_qserver1
//#define xapi_qsubpool                        xapi_qsubpool1
//#define xapi_query                           xapi_query1
//#define xapi_query_init_resp                 xapi_query_init_resp1
//#define xapi_qvol                            xapi_qvol1
//#define xapi_qvol_all                        xapi_qvol_all1
//#define xapi_qvol_one                        xapi_qvol_one1
//#define xapi_register                        xapi_register1
//#define xapi_request_header                  xapi_request_header1
//#define xapi_ack_response                    xapi_response1
//#define xapi_int_response                    xapi_response2
//#define xapi_err_response                    xapi_response3
//#define xapi_fin_response                    xapi_response4
//#define xapi_scrpool                         xapi_scrpool1
//#define xapi_scrpool_search_name             xapi_scrpool_search1
//#define xapi_scrpool_search_index            xapi_scrpool_search2
//#define xapi_scrpool_counts                  xapi_scrpool_counts1
//#define xapi_set_cap                         xapi_set_cap1
//#define xapi_set_clean                       xapi_set_clean1
//#define xapi_set_scr                         xapi_set_scr1
//#define xapi_start                           xapi_start1
//#define xapi_tcp                             xapi_tcp1
//#define xapi_unlock                          xapi_unlock1
//#define xapi_unlock_drv                      xapi_unlock_drv1
//#define xapi_unlock_vol                      xapi_unlock_vol1
//#define xapi_unregister                      xapi_unregister1
//#define xapi_userid                          xapi_userid1
//#define xapi_vary                            xapi_vary1
//#define xapi_venter                          xapi_venter1
//#define xapi_validate_single_volser          xapi_volser1
//#define xapi_validate_volser_range           xapi_volser2
//#define xapi_volser_increment                xapi_volser3
//#define xapi_xeject                          xapi_xeject1


/*********************************************************************/
/* Constants:                                                        */
/*********************************************************************/
#ifndef SETSOCKOPT_ON
    #define SETSOCKOPT_ON       1
#endif

#ifndef SETSOCKOPT_OFF
    #define SETSOCKOPT_OFF      0
#endif

#ifndef RC_SUCCESS
    #define RC_SUCCESS          0
#endif

#ifndef RC_WARNING
    #define RC_WARNING          4
#endif

#ifndef RC_FAILURE
    #define RC_FAILURE          -1
#endif

#ifndef TRUE
    #define TRUE                1
#endif

#ifndef FALSE
    #define FALSE               0
#endif

#ifndef DONOTHING
    #define DONOTHING                  /* Pseudo NOP                 */
#endif


/*********************************************************************/
/* Constants used to populate and access the XAPICVT control table:  */
/*********************************************************************/
#define XAPI_CVT_SHM_KEY       1952675444  /* Shared mem key for     */
/*                                        ...XAPICVT (ASCII "tcvt")  */
#define XAPI_CFG_SHM_KEY       1952671335  /* Shared mem key for     */
/*                                        ...XAPICFG (ASCII "tcfg")  */
#define XAPI_CONVERSION        "XAPI_CONVERSION"  
#define XAPI_PORT              "XAPI_PORT"        
#define XAPI_HOSTNAME          "XAPI_HOSTNAME"    
#define XAPI_TAPEPLEX          "XAPI_TAPEPLEX"    
#define XAPI_SUBSYSTEM         "XAPI_SUBSYSTEM"   
#define XAPI_VERSION           "XAPI_VERSION"     
#define XAPI_USER              "XAPI_USER"        
#define XAPI_GROUP             "XAPI_GROUP" 
#define XAPI_EJECT_TEXT        "XAPI_EJECT_TEXT" 
#define XAPI_DRIVE_LIST        "XAPI_DRIVE_LIST" 
#define XAPI_PORT_DEFAULT      8080               
#define XAPI_VERS_DEFAULT      "710"              
#define XAPI_HOSTNAME_SIZE     128   
#define XAPI_TAPEPLEX_SIZE     8                  
#define XAPI_SUBSYSTEM_SIZE    4                  
#define XAPI_VERSION_SIZE      4                  
#define XAPI_USER_SIZE         8                  
#define XAPI_GROUP_SIZE        8                  
#define XAPI_VTSSNAME_SIZE     8 
#define MAX_ACS_NUMBER         128     /* Max "dummy" ACS number     */
#define XAPI_SUBPOOL_NAME_SIZE 13
#define XAPI_MEDIA_NAME_SIZE   8
#define XAPI_MODEL_NAME_SIZE   8
#define XAPI_RECTECH_NAME_SIZE 8
#define XAPI_VOLSER_SIZE       6
#define XAPI_EJECT_TEXT_SIZE   44
#define XAPI_DEFAULTPOOL       "DEFAULTPOOL"
#define XAPI_DEFAULTUSER       "XAPIUSER"


/*********************************************************************/
/* XAPI xapi_main.c input processFlag values.                        */
/*********************************************************************/
#define XAPI_FORK              0       /* Invoked as forked child    */
#define XAPI_CALL              1       /* Invoked via call           */


/*********************************************************************/
/* XAPI XML constants.                                               */
/* XSTARTTAG are XAPI XML start tag strings.                         */
/* XENDTAG are XAPI XML end tag strings.                             */
/* XNAME are XAPI XML name strings.                                  */
/* XCONTENT are XAPI XML content strings.                            */
/*********************************************************************/
#define XSTARTTAG_command      "<command>"
#define XSTARTTAG_libreply     "<libreply>"
#define XENDTAG_libreply       "</libreply>"
#define XSTARTTAG_libtrans     "<libtrans>"
#define XENDTAG_libtrans       "</libtrans>"

#define XNAME_acs                              "acs"
#define XNAME_acsapi_value                     "acsapi_value"
#define XNAME_acs_data                         "acs_data"
#define XNAME_acs_list                         "acs_list"
#define XNAME_acs_status                       "acs_status"
#define XNAME_adjacent_count                   "adjacent_count"
#define XNAME_advanced_management              "advanced_management"
#define XNAME_all_request                      "all_request"
#define XNAME_append_model_list                "append_model_list"
#define XNAME_cancel_request                   "cancel_request"
#define XNAME_cap                              "cap"
#define XNAME_cap_count                        "cap_count"
#define XNAME_cap_data                         "cap_data"
#define XNAME_cap_jobname_owner                "cap_jobname_owner"
#define XNAME_cap_list                         "cap_list"
#define XNAME_cap_location_data                "cap_location_data"
#define XNAME_cap_type                         "cap_type"
#define XNAME_cdk_drive_status                 "cdk_drive_status"
#define XNAME_cdk_volume_status                "cdk_volume_status"
#define XNAME_cell_count                       "cell_count"
#define XNAME_cells_per_magazine               "cells_per_magazine"
#define XNAME_cleaner_count                    "cleaner_count"
#define XNAME_cleaner_media_indicator          "cleaner_media_indicator"
#define XNAME_client_name                      "client_name"
#define XNAME_client_type                      "client_type"
#define XNAME_clr_drive_lock                   "clr_drive_lock"
#define XNAME_clr_drive_lock_request           "clr_drive_lock_request"
#define XNAME_clr_volume_lock                  "clr_volume_lock"
#define XNAME_clr_volume_lock_request          "clr_volume_lock_request"
#define XNAME_column                           "column"
#define XNAME_column_count                     "column_count"
#define XNAME_command                          "command"
#define XNAME_configuration_token              "configuration_token"
#define XNAME_config_info_request              "config_info_request"
#define XNAME_csv_break_tag                    "csv_break_tag"
#define XNAME_csv_fields                       "csv_fields"
#define XNAME_csv_fixed_flag                   "csv_fixed_flag"
#define XNAME_csv_notitle_flag                 "csv_notitle_flag"
#define XNAME_csv_titles                       "csv_titles"
#define XNAME_date                             "date"
#define XNAME_ddname                           "ddname"
#define XNAME_density                          "density"
#define XNAME_destination_cell                 "destination_cell"
#define XNAME_destination_location             "destination_location"
#define XNAME_device_address                   "device_address"
#define XNAME_detail_request                   "detail_request"
#define XNAME_dismount                         "dismount"
#define XNAME_dismount_data                    "dismount_data"
#define XNAME_dismount_request                 "dismount_request"
#define XNAME_display_server_request           "display_server_request"
#define XNAME_drive_data                       "drive_data"
#define XNAME_drive_group_location             "drive_group_location"
#define XNAME_drive_info                       "drive_info"
#define XNAME_drive_info_format                "drive_info_format"
#define XNAME_drive_library_address            "drive_library_address"
#define XNAME_drive_list                       "drive_list"
#define XNAME_drive_location                   "drive_location"
#define XNAME_drive_location_id                "drive_location_id"
#define XNAME_drive_location_zone              "drive_location_zone"
#define XNAME_drive_name                       "drive_name"
#define XNAME_drive_number                     "drive_number"
#define XNAME_drvtype_info                     "drvtype_info"
#define XNAME_dsname                           "dsname"
#define XNAME_dsn_data                         "dsn_data"
#define XNAME_dual_lmu_config                  "dual_lmu_config"
#define XNAME_eject_data                       "eject_data"
#define XNAME_eject_group                      "eject_group"
#define XNAME_eject_text                       "eject_text"
#define XNAME_eject_vol                        "eject_vol"
#define XNAME_els_version                      "els_version"
#define XNAME_enter                            "enter"
#define XNAME_enter_data                       "enter_data"
#define XNAME_encrypted                        "encrypted"
#define XNAME_error                            "error"
#define XNAME_exceptions                       "exceptions"
#define XNAME_expiration_date                  "expiration_date"
#define XNAME_free_cell_count                  "free_cell_count"
#define XNAME_from_library_address             "from_library_address"
#define XNAME_frozen                           "frozen"
#define XNAME_job_info                         "job_info"
#define XNAME_jobname                          "jobname"
#define XNAME_header                           "header"
#define XNAME_home_cell                        "home_cell"
#define XNAME_host_name                        "host_name"
#define XNAME_invalid_management_class_flag    "invalid_management_class_flag"
#define XNAME_invalid_subpool_name_flag        "invalid_subpool_name_flag"
#define XNAME_label_type                       "label_type"
#define XNAME_library_address                  "library_address"
#define XNAME_library_name                     "library_name"
#define XNAME_libtrans                         "libtrans"
#define XNAME_libreply                         "libreply"
#define XNAME_lock_drive                       "lock_drive"
#define XNAME_lock_drive_request               "lock_drive_request"
#define XNAME_lock_duration                    "lock_duration"
#define XNAME_lock_id                          "lock_id"
#define XNAME_lock_volume                      "lock_volume"
#define XNAME_lock_volume_request              "lock_volume_request"
#define XNAME_locks_pending                    "locks_pending"
#define XNAME_lsm                              "lsm"
#define XNAME_lsm_count                        "lsm_count"
#define XNAME_lsm_data                         "lsm_data"
#define XNAME_lsm_id                           "lsm_id"
#define XNAME_lsm_list                         "lsm_list"
#define XNAME_lsm_location_data                "lsm_location_data"
#define XNAME_max_drive_count                  "max_drive_count"
#define XNAME_magazine_count                   "magazine_count"
#define XNAME_management_class                 "management_class"
#define XNAME_media                            "media"
#define XNAME_media_domain                     "media_domain"
#define XNAME_media_drive_info                 "media_drive_info"
#define XNAME_media_info                       "media_info"
#define XNAME_media_list                       "media_list"
#define XNAME_media_type                       "media_type"
#define XNAME_mode                             "mode"
#define XNAME_model                            "model"
#define XNAME_model_list                       "model_list"
#define XNAME_mount                            "mount"
#define XNAME_mount_data                       "mount_data"
#define XNAME_mount_request                    "mount_request"
#define XNAME_move                             "move"
#define XNAME_move_data                        "move_data"
#define XNAME_no_library_scratch_flag          "no_library_scratch_flag"
#define XNAME_no_scratch_for_lbltype_flag      "no_scratch_for_lbltype_flag"
#define XNAME_panel                            "panel"
#define XNAME_panel_count                      "panel_count"
#define XNAME_panel_data                       "panel_data"
#define XNAME_panel_type                       "panel_type"
#define XNAME_part_id                          "part_id"
#define XNAME_pool_type                        "pool_type"
#define XNAME_priority                         "priority"
#define XNAME_preference_order                 "preference_order"
#define XNAME_program_name                     "program_name"
#define XNAME_qry_drive_lock                   "qry_drive_lock"
#define XNAME_qry_drive_lock_request           "qry_drive_lock_request"
#define XNAME_qry_volume_lock                  "qry_volume_lock"
#define XNAME_qry_volume_lock_request          "qry_volume_lock_request"
#define XNAME_query_acs                        "query_acs"
#define XNAME_query_all_volume_request         "query_all_volume_request"
#define XNAME_query_cap                        "query_cap"
#define XNAME_query_drive_info                 "query_drive_info"
#define XNAME_query_drive_info_request         "query_drive_info_request"
#define XNAME_query_drvtypes                   "query_drvtypes"
#define XNAME_query_lsm                        "query_lsm"
#define XNAME_query_media                      "query_media"
#define XNAME_query_server                     "query_server"
#define XNAME_query_scratch                    "query_scratch"
#define XNAME_query_scratch_mount_request      "query_scratch_mount_request"
#define XNAME_query_scrpool_info               "query_scrpool_info"
#define XNAME_query_scr_mnt_info               "query_scr_mnt_info"
#define XNAME_query_threshold                  "query_threshold"
#define XNAME_query_volume_info                "query_volume_info"
#define XNAME_query_volume_info_request        "query_volume_info_request"
#define XNAME_racf_group_id                    "racf_group_id"
#define XNAME_racf_user_id                     "racf_user_id"
#define XNAME_read_model_list                  "read_model_list"
#define XNAME_read_only                        "read_only"
#define XNAME_reason                           "reason"
#define XNAME_rectech                          "rectech"
#define XNAME_rectech_list                     "rectech_list"
#define XNAME_resident_vtss                    "resident_vtss"
#define XNAME_result                           "result"
#define XNAME_retention_period                 "retention_period"
#define XNAME_rewind_unload                    "rewind_unload"
#define XNAME_row                              "row"
#define XNAME_row_count                        "row_count"
#define XNAME_scratch                          "scratch"
#define XNAME_scratch_count                    "scratch_count"
#define XNAME_scratch_data                     "scratch_data"
#define XNAME_scratch_pool_info                "scratch_pool_info"
#define XNAME_scratch_vol                      "scratch_vol"
#define XNAME_scrpool_request_info             "scrpool_request_info"
#define XNAME_sequence_by_lsm                  "sequence_by_lsm"
#define XNAME_serial_number                    "serial_number"
#define XNAME_server_data                      "server_data"
#define XNAME_server_type                      "server_type"
#define XNAME_service_level                    "service_level"
#define XNAME_source_cell                      "source_cell"
#define XNAME_source_location                  "source_location"
#define XNAME_state                            "state"
#define XNAME_status                           "status"
#define XNAME_stepname                         "stepname"
#define XNAME_subpool_data                     "subpool_data"
#define XNAME_subpool_index                    "subpool_index"
#define XNAME_subpool_name                     "subpool_name"
#define XNAME_subsystem_name                   "subsystem_name"
#define XNAME_subsystem_start_date             "subsystem_start_date"
#define XNAME_subsystem_start_time             "subsystem_start_time"
#define XNAME_tapeplex_name                    "tapeplex_name"
#define XNAME_task_token                       "task_token"
#define XNAME_termination_in_progress          "termination_in_progress"
#define XNAME_time                             "time"
#define XNAME_to_library_address               "to_library_address"
#define XNAME_trace_flag                       "trace_flag"
#define XNAME_unlock_drive                     "unlock_drive"
#define XNAME_unlock_drive_request             "unlock_drive_request"
#define XNAME_unlock_volume                    "unlock_volume"
#define XNAME_unlock_volume_request            "unlock_volume_request"
#define XNAME_unscratch                        "unscratch"
#define XNAME_user_label                       "user_label"
#define XNAME_user_name                        "user_name"
#define XNAME_uui_asynch_flag                  "uui_asynch_flag"
#define XNAME_uui_command_timeout              "uui_command_timeout"
#define XNAME_uui_line_type                    "uui_line_type"
#define XNAME_uui_read_timeout                 "uui_read_timeout"
#define XNAME_uui_reason_code                  "uui_reason_code"
#define XNAME_uui_return_code                  "uui_return_code"
#define XNAME_uui_text                         "uui_text"
#define XNAME_volser                           "volser"
#define XNAME_voltype                          "voltype"
#define XNAME_volume_count                     "volume_count"
#define XNAME_volume_data                      "volume_data"
#define XNAME_volume_list                      "volume_list"
#define XNAME_vol_range                        "vol_range"
#define XNAME_vtcs_available                   "vtcs_available"
#define XNAME_vtss_name                        "vtss_name"
#define XNAME_wait_cap                         "wait_cap"
#define XNAME_wait_flag                        "wait_flag"
#define XNAME_world_wide_name                  "world_wide_name"
#define XNAME_write_model_list                 "write_model_list"
#define XNAME_xml_case                         "xml_case"
#define XNAME_xml_date_format                  "xml_date_format"
#define XNAME_xml_response_flag                "xml_response_flag"

#define XCONTENT_FULL          "FULL"
#define XCONTENT_N             "N"
#define XCONTENT_NO            "NO"
#define XCONTENT_Y             "Y"
#define XCONTENT_YES           "YES"

#define XAPI_TASK_TOKEN_SIZE   16
#define XAPI_DRIVENUMBER_SIZE  4

#define MAX_RMCODE_PLUS_STRING 80      /* Max len of RMCODE strings  */

#define XML_CASE_UPPER         "U"
#define XML_CASE_MIXED         "M"

#define XML_DATE_YYYYMONDD     "0"     /* YYYYMonDD date (default)   */
#define XML_DATE_YYYY_MON_DD   "1"     /* YYYY-Mon-DD date           */
#define XML_DATE_YYYY_MM_DD    "2"     /* YYYY-MM-DD date            */
#define XML_DATE_STCK          "3"     /* STCK format                */


/*********************************************************************/
/* Structures:                                                       */
/*********************************************************************/

/*********************************************************************/
/* LIBDRVID: Library drive ID.                                       */
/*********************************************************************/
struct LIBDRVID
{
    unsigned char      acs;            /* ACS Id                     */
    unsigned char      lsm;            /* LSM Id                     */
    unsigned char      panel;          /* Panel number (0-128)       */
    unsigned char      driveNumber;    /* Drive number (0-255)       */
};


/*********************************************************************/
/* XAPIVRANGE: Represents one volume or volume range in a request    */
/* and keeps track of the processing status for that range.          */
/*********************************************************************/
struct XAPIVRANGE
{
    struct XAPIVRANGE *pNextXapivrange;/* Addr of next XAPIVRANGE    */
    int                numInRange;     /* Number of volsers in range */
    char               firstVol[6];    /* Beginning volser in range  */
    char               lastVol[6];     /* Ending volser in range     */
    char               mask[6];        /* Volser mask                */
    char               currVol[6];     /* Current volser in range    */
    char               firstVolNotFound[6];/* First volser in range  */
    /*                                    ...that was not found      */
    char               lastVolNotFound[6]; /* Last volser in range   */
    /*                                    ...that was not found      */
    char               volIncIx;       /* Index of increment         */
    char               volIncLen;      /* Length of increment        */
    char               volNotFoundFlag;/* Current volser not found   */
    char               rangeCompletedFlag; /* Entire volser range    */
    /*                                    ...was processed flag      */
};


/*********************************************************************/
/* XAPIDRANGE: Represents one drive or drive range in a command      */
/* and keeps track of the processing status for that range.          */
/*********************************************************************/
struct XAPIDRANGE
{
    struct XAPIDRANGE *pNextXapidrange;/* Addr of next XAPIDRANGE    */
    int                numInRange;     /* Number of drives in range  */
    char               firstDrive[16]; /* Beginning drive in range   */
    char               lastDrive[16];  /* Ending drive in range      */
    char               mask[16];       /* Drive mask                 */
    char               currDrive[16];  /* Current drive in range     */
    char               firstDriveNotFound[16]; /* First drive in     */
    /*                                    ...range that was not found*/
    char               lastDriveNotFound[16];  /* Last drive in      */
    /*                                    ...range that was not found*/
    char               driveIncIx;     /* Index of increment         */
    char               driveIncLen;    /* Len of increment           */
    char               driveNotFoundFlag;  /* Current not found drive*/
    char               rangeCompletedFlag; /* Entire drive range     */
    /*                                    ...was processed flag      */
    char               driveFormatFlag;/* Drive format flag          */
#define DRIVE_RCOLON_FORMAT    'R'     /* ...R: loc ID format        */
#define DRIVE_VCOLON_FORMAT    'V'     /* ...V: loc ID format        */
#define DRIVE_CCUU_FORMAT      'X'     /* ...CCUU or CUU name format */
};


/*********************************************************************/
/* XAPIDRLST: Represents one drive in the XAPI_DRIVE_LIST that       */
/* was specified at startup.                                         */
/*                                                                   */
/* The XAPIDRLST is used to match the configuration query response.  */
/* Any matching drive in the XAPIDRLST will be added to the          */
/* XAPICFG configuration drive list.                                 */
/*********************************************************************/
struct XAPIDRLST
{
    char               driveName[4];   /* Drive CCUU address         */
    char               driveLocId[16]; /* Drive location ID          */
    char               configDriveFlag;/* Configuration drive flag   */
    /*                                    ...TRUE: Drive was returned*/
    /*                                    ...in config response      */
    char               _f0[3];         /* Available                  */
};


/*********************************************************************/
/* XMLHDRINFO describes user and policy override information         */
/* is is passed to the HTTP server under the XML <header>            */
/* element (this information is roughly equivalent to the            */
/* SMC PTAPEKEY used by the SMC for tape policy lookup).             */
/*********************************************************************/
struct XMLHDRINFO
{
    char               systemId[8];    /* Originating system id      */
    char               jobname[8];     /* Jobname                    */
    char               stepname[8];    /* Stepname                   */
    char               programName[8]; /* Program name               */
    char               ddname[8];      /* DD name                    */
    char               dsname[44];     /* Dataset name               */
    char               expdt[5];       /* Expiration date            */
    char               retpd[3];       /* Retention period           */
    char               volser[6];      /* 1st volser                 */
    char               volType[8];     /* Voltype: "Specific" or     */
    /*                                    ..."Nonspec" (scratch)     */
};


/*********************************************************************/
/* CDKSTATUS: Translate an XAPI LOCK function status string into     */
/* an ACSAPI status code.                                            */
/*********************************************************************/
struct CDKSTATUS
{
    char               statusString[17];
    int                statusCode;
};


/*********************************************************************/
/* RAWACS: <acs_data> extract data from QUERY ACS XAPI XML           */
/* command responses.                                                */
/*********************************************************************/
struct RAWACS
{
    char               acs[2];         /* ACS Id                     */
    char               lsmCount[4];    /* LSM count                  */
    char               status[16];     /* CONNECTED|DISCONNECTED     */
    char               state[9];       /* ONLINE|OFFLINE             */
    char               freeCellCount[8];   /* Free (empty) cell count*/
    char               scratchCount[7];    /* Scatch volume count    */
    char               dualLmu[3];     /* YES|NO                     */
    char               capCount[3];    /* CAP count                  */
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWLSM: <lsm_data> extract data from QUERY LSM XAPI XML           */
/* command responses.                                                */
/*********************************************************************/
struct RAWLSM
{
    char               acs[2];         /* ACS Id                     */
    char               lsm[2];         /* LSM Id                     */
    char               model[8];       /* LSM model                  */
    char               status[16];     /* CONNECTED|DISCONNECTED     */
    char               state[9];       /* ONLINE|OFFLINE             */
    char               mode[20];       /* Mode                       */
    char               cellCount[4];   /* LSM cell count             */
    char               freeCellCount[8];   /* Free (empty) cell count*/
    char               scratchCount[7];    /* Scratch volume count   */
    char               cleanerCount[4];    /* Cleaner volume count   */
    char               adjacentCount[3];   /* Adjacent LSM count     */
    char               capCount[3];    /* CAP count                  */
    char               panelCount[3];  /* Panel count                */
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWCAP: <cap_data> extract data from QUERY CAP XAPI XML           */
/* command responses.                                                */
/*********************************************************************/
struct RAWCAP
{
    char               acs[2];         /* ACS Id                     */
    char               lsm[2];         /* LSM Id                     */
    char               cap[2];         /* CAP Id                     */
    char               hostName[8];    /* Host name                  */
    char               cellCount[4];   /* CAP cell count             */
    char               priority[1];    /* Priority                   */
    char               mode[20];       /* Mode                       */
    char               status[16];     /* Status                     */
    char               state[9];       /* ONLINE|OFFLINE             */
    char               partId[8];      /* Part Id                    */
    char               rowCount[4];    /* Cap row count              */
    char               columnCount[4]; /* Cap column count           */
    char               magazineCount[4];   /* Magazine count         */
    char               cellsPerMagazine[4];/* Cells per magazine     */
    char               capType[16];        /* Cap type               */
    char               capJobnameOwner[8]; /* Cap jobname owner      */
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWDRIVE: <drive_info> extract data from QUERY_DRIVE_INFO         */
/* XAPI XML command responses.                                       */
/*********************************************************************/
struct RAWDRIVE
{
    char               driveName[4];   /* Drive CCUU address         */
    char               acs[2];         /* ACS Id                     */
    char               lsm[2];         /* LSM Id                     */
    char               panel[2];       /* Panel number               */
    char               driveNumber[2]; /* Drive number               */
    char               volser[6];      /* Volume serial number       */
    char               driveLocId[16]; /* R:aa:ll:ppp:nnn            */
    char               model[8];       /* Single model name          */
    char               rectech[8];     /* Singel rectech name        */
    char               vtssName[8];    /* VTSS name                  */
    char               driveGroup[16]; /* V:vtssname:ccuu            */
    char               serialNumber[20];   /* Drive serial number    */
    char               status[16];     /* MOUNTING|ON DRIVE|DISMOUNT */
    char               state[9];       /* ONLINE|OFFLINE             */
    char               worldWideName[23];
    char               driveLocZone[2];
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWEJECT: <eject_data> extract data from EJECT XAPI XML           */
/* command responses.                                                */
/*********************************************************************/
struct RAWEJECT
{
    char               acs[2];         /* ACS Id                     */
    char               lsm[2];         /* LSM Id                     */
    char               panel[2];       /* Panel number               */
    char               row[2];         /* Row number                 */
    char               column[2];      /* Column number              */
    char               volser[6];      /* Volume serial number       */
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWENTER: <enter_data> extract data from ENTER XAPI XML           */
/* command responses.                                                */
/*********************************************************************/
struct RAWENTER
{
    char               acs[2];         /* ACS Id                     */
    char               lsm[2];         /* LSM Id                     */
    char               panel[2];       /* Panel number               */
    char               row[2];         /* Row number                 */
    char               column[2];      /* Column number              */
    char               volser[6];      /* Volume serial number       */
    char               media[8];       /* Single media name          */
    char               mediaType[1];   /* LMU media type code        */
    char               mediaDomain[1]; /* LMU media domain code      */
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWMOVE: <move_data> extract data from MOVE XAPI XML              */
/* command responses.                                                */
/*********************************************************************/
struct RAWMOVE
{
    char               volser[6];      /* Volume serial number       */
    char               sourceCell[14]; /* AA:LL:PP:RR:CC format      */
    char               destinationCell[14];
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
    /*****************************************************************/
    /* <source_location>                                             */
    /*****************************************************************/
    char               fromAcs[2];
    char               fromLsm[2];
    char               fromPanel[2];
    char               fromRow[2];
    char               fromColumn[2];
    /*****************************************************************/
    /* <target_location>                                             */
    /*****************************************************************/
    char               toAcs[2];
    char               toLsm[2];
    char               toPanel[2];
    char               toRow[2];
    char               toColumn[2];
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWVOLUME: <volume_data> extract data from QUERY VOLUME_INFO      */
/* XAPI XML response.                                                */
/*                                                                   */
/* Also used to extract data from <mount_data>, <dismount_data>,     */
/* and <selected_volume_data> from MOUNT, DISMOUNT, and SELSCR       */
/* XAPI XML responses.                                               */
/*********************************************************************/
struct RAWVOLUME
{
    char               volser[6];      /* Volume serial number       */
    char               status[7];      /* MOUNTED|ERRANT             */
    /*                                    ...otherwise not returned  */
    char               media[8];       /* Single media name          */
    char               mediaType[1];   /* LMU media type code        */
    char               mediaDomain[1]; /* LMU media domain code      */
    char               model[8];       /* Single model name          */
    char               homeCell[14];   /* AA:LL:PP:RR:CC format      */
    char               vtssName[8];    /* VTSS name                  */
    char               residentVtss[8];/* Resident VTSS name         */
    char               denRectName[8]; /* Density rectech name       */
    char               scratch[3];     /* YES|NO                     */
    char               encrypted[3];   /* YES|NO                     */
    char               subpoolType[8]; /* SCRATCH|MVS|EXTERNAL       */
    char               subpoolName[13];/* Subpool name               */
    char               charDevAddr[4]; /* Drive CCUU address         */
    char               driveName[4];   /* Drive CCUU address         */
    char               driveLocId[16]; /* R:AA:LL:PP:DD:ZZ format    */
    char               locType[1];     /* Location type              */
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
    /*****************************************************************/
    /* <library_address>                                             */
    /*****************************************************************/
    char               cellAcs[2];
    char               cellLsm[2];
    char               cellPanel[2];
    char               cellRow[2];
    char               cellColumn[2];
    /*****************************************************************/
    /* <drive_library_address>                                       */
    /*****************************************************************/
    char               driveAcs[2];
    char               driveLsm[2];
    char               drivePanel[2];
    char               driveNumber[2];
    char               driveLocZone[2];
    /*****************************************************************/
    /* The following fields are not represented by XML tag values    */
    /* themselves but are extrapolated from other QUERY VOLUME_INFO  */
    /* information.                                                  */
    /*****************************************************************/
    char               readModelList[MAX_RMCODE_PLUS_STRING + 1];
    char               compositeReadRectech[8];
    char               writeModelList[MAX_RMCODE_PLUS_STRING + 1];
    char               compositeWriteRectech[8];
    char               appendModelList[MAX_RMCODE_PLUS_STRING + 1];
    char               compositeAppendRectech[8];
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWDRONE: Drive list element extract data under the <drive_info>  */
/* tag of the QUERY_SCR_MNT_INFO, and QUERY VOLUME_INFO              */
/* (<drive_info_format>) XAPI XML command responses.                 */
/*                                                                   */
/* Used in RAWDRLST below.                                           */
/*********************************************************************/
struct RAWDRONE
{
    char               driveName[4];   /* Drive CCUU address         */
    char               driveLocId[16]; /* R:aa:ll:ppp:nnn            */
    char               model[8];       /* Single model name          */
    char               state[9];       /* ONLINE|OFFLINE             */
    char               prefOrder[4];   /* Drive preference order     */
    char               scratchCount[7];/* Scratch count (for         */
    /*                                    ...QUERY_SCR_MNT_INFO      */
    /*                                    ...drive list responses)   */ 
};


/*********************************************************************/
/* RAWDRLST: Drive list returned from the QUERY_SCR_MNT_INFO, and    */
/* QUERY VOLUME_INFO (<drive_info_format>) XAPI XML command          */
/* responses.                                                        */
/*********************************************************************/
struct RAWDRLST
{
#define XAPI_MAX_DRLST_COUNT   165     /* Max <drive_data> elements  */
/*                                        ...requested; should be    */
/*                                        ...same as ACSAPI #define  */
/*                                        ...for QU_MAX_DRV_STATUS   */
    int                driveCount;     /* Number of drives in list   */
    int                _f0;            /* Available                  */
    struct RAWDRONE    rawdrone[XAPI_MAX_DRLST_COUNT];               
};


/*********************************************************************/
/* RAWSCRMNT: <scratch_data> extract data from the                   */
/* QUERY_SCR_MNT_INFO XAPI XML command response.                     */
/*********************************************************************/
struct RAWSCRMNT
{
    char               invalidMgmt[3]; /* Invalid mgmt class  YES|NO */
    char               noLibScratch[3];/* No library scratch  YES|NO */
    char               invalidSubpool[3];  /* Invalid subpool YES|NO */
    char               noLblScratch[3];/* No lbltype scratch  YES|NO */
    char               mediaString[10] [9];/* Up to 10 media strings */
    char               modelString[10] [9];/* Up to 10 model strings */
};


/*********************************************************************/
/* RAWSCRATCH: <volume_data> extract data from SCRATCH_VOL and       */
/* UNSCRATCH XAPI XML command responses.                             */
/*********************************************************************/
struct RAWSCRATCH
{
    char               volser[6];      /* Volume serial number       */
    char               result[8];      /* SUCCESS|FAILURE            */
    char               error[8];       /* Hexadecimal error code     */
    char               reason[124];    /* Reason text                */
};


/*********************************************************************/
/* RAWMOUNT: <mount_data> and <dismount_data> extract data from      */
/* MOUNT and DISMOUNT XAPI XML command responses.                    */
/*********************************************************************/
struct RAWMOUNT
{
    struct RAWVOLUME   rawvolume;      /* RAWVOLUME                  */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* RAWQLOCK: <qry_drive_lock> or <qry_volume_lock> extract data      */
/* from QUERY LOCK XAPI XML command responses.                       */
/*********************************************************************/
struct RAWQLOCK
{
    char               queryStatus[16];/* Highest level status       */
    char               lockId[5];      /* LockId (1-32767)           */
    char               lockDuration[10];   /* Duration in seconds    */
    char               userLabel[64];  /* Userid (label) of owner    */
    char               resStatus[16];  /* Individual resource status */
    char               volser[6];      /* Volume serial number       */
    char               driveLocId[16]; /* R:aa:ll:ppp:nnn            */
    char               locksPending[10];   /* Requests waiting       */
    /*****************************************************************/
    /* The following fields are not represented by XML tag values    */
    /* themselves but are extrapolated from other QUERY LOCK         */
    /* information.                                                  */
    /*****************************************************************/
    int                queryRC;         /* Highest level RC          */
    int                resRC;           /* Individual resource RC    */
};


/*********************************************************************/
/* RAWSERVER: <server_data> from QUERY SERVER XAPI XML command       */
/* response.                                                         */
/*********************************************************************/
struct RAWSERVER
{
    char               serviceLevel[4];/* Service level (ex: "0710") */
    char               subsystemName[4];   /* Subsystem name         */
    char               serverType[3]; /* Server type                 */
    char               startDate[9];  /* Subsystem start date        */
    /*                                   ...or HEX CHAR time() value */
    char               startTime[8];  /* Subsystem start date        */
    char               termInProgress[3];  /* YES|NO                 */
    char               vtcsAvailable[3];   /* YES|NO                 */
    char               advManagement[3];   /* YES|NO                 */
};


/*********************************************************************/
/* RAWCOMMON: Common header and trailer extract data for all         */
/* XAPI XML responses.                                               */
/*********************************************************************/
struct RAWCOMMON
{
    char               uuiRC[8];       /* Decimal UUI return code    */
    char               uuiReason[8];   /* Decimal UUI reason code    */
    char               configToken[16];/* Character hexadecimal STCK */
    /*                                    ...configuration token     */
    char               tapeplexName[8];/* Tapeplex (or library) name */
    char               hostName[8];    /* Host name                  */
    char               subsystemName[4];   /* Subsystem name         */
    char               version[5];     /* Version "9.9.9" format     */
    char               serverType[3];  /* HSC|SMC|VLE                */
    char               exceptionText[125]; /* Most significant       */
    /*                                 /* ...<reason> msg string     */
#define MAX_REASON_TEXTS 4
    char               reasonText[4] [125];/* Up to 4 <exception>    */
    /*                                    ...<reason> msg strings    */
    char               xmlDateFormat;  /* Implied XML date format:   */
    /*                                    ...4 possible formats (0-3)*/
    /*                                    ...0 = yyyymondd           */
    /*                                    ...1 = yyyy-mon-dd         */
    /*                                    ...2 = yyyy-mm-dd          */
    /*                                    ...3 = STCK value          */
    char               date[11];       /* Date of request            */
    /*                                    ...if xmlDateFormat = 0-2  */
    char               stckTime[8];    /* Character hexadecimal      */
    /*                                    ...hi-order STCK value     */
    /*                                    ...if xmlDateFormat = 3    */
    char               time[8];        /* Time in hh:mm:ss format    */
    /*                                    ...if xmlDateFormat = 0-2  */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* XAPICOMMON: Essentially the same as the above RAWCOMMON extract   */
/* with the serverName and commRC added, and the version, STCK       */
/* values, configuration token, UUI return and reason codes          */
/* converted to binary, and the message number extracted from        */
/* the exception reason text.                                        */
/*********************************************************************/
struct XAPICOMMON
{
    struct RAWCOMMON   rawcommon;      /* Rawcommon                  */
    unsigned long long driveListTime;  /* Binary configuration token */
    time_t             requestTime;    /* Binary request time        */
    int                normalizedRelease; /* Version in 999 format   */
    /*                                    ...7.1.0 = 710             */
    int                commRC;         /* COMMRESP return code       */
    int                uuiRC;          /* UUI return code            */
    int                uuiReason;      /* UUI reason code            */
    int                exceptionNum;   /* Most significant msg num   */
    int                reasonNum[4];   /* Up to 4 <reason> msg nums  */
    char               serverName[8];  /* Server name                */
    int                _f0[4];         /* Available                  */
};


/*********************************************************************/
/* XAPICFG describes an XAPI config table drive entry.               */
/*                                                                   */
/* Used in XAPICVT below.  NOTE: The XAPICVT.pXapicfg points to the  */
/* 1st of XAPICVT.driveCount XAPICFG entries (which collectively     */
/* are the configuration drive table).                               */
/*********************************************************************/
struct XAPICFG
{
    unsigned short     driveName;      /* CCUU drive address         */
    /*                                    ...(the converted XAPI     */
    /*                                    ...drive_name)             */
    struct LIBDRVID    libdrvid;       /* ACS:LSM:PANEL:DRIVE        */
    unsigned char      acsapiDriveType;/* ACSAPI drive_type code     */
    char               vtssNameString[XAPI_VTSSNAME_SIZE + 1]; 
    char               model[8];       /* Single XAPI model name     */
    char               rectech[8];     /* Single XAPI rectech name   */
    char               driveLocId[16]; /* XAPI drive_location_id     */
    /*                                    ...padded with blanks      */
    int                _f0[2];         /* Available                  */

};


/*********************************************************************/
/* XAPIREQE describes an XAPI request entry in the                   */
/* XAPI request entry table (csi_xapi_control_table).                */
/*                                                                   */
/* The XAPIREQE is a fixed length table embedded within the          */
/* XAPICVT.  It has 64 entries which the arbitrary maximim number    */
/* of active XAPI requests that can be active at any one moment.     */
/*                                                                   */
/* Used in XAPICVT below.                                            */
/*********************************************************************/
struct XAPIREQE
{
    char               requestFlag;    /* Request status flag        */
#define XAPIREQE_FREE          '\x00'  /* ...Entry is free           */
#define XAPIREQE_START         '\x01'  /* ...Request started         */
#define XAPIREQE_END           '\x02'  /* ...Request ended; entry    */
    /*                                    ...can be free'd           */
#define XAPIREQE_CANCEL        '\x04'  /* ...Request cancelled       */
    char               command;        /* CDK command code           */
    /*****************************************************************/
    /* XAPI_SOCKET_NAME_SIZE must be the same as SOCKET_NAME_SIZE in */
    /* h/api/db_defs_api.h                                           */
    /*****************************************************************/
#define XAPI_SOCKET_NAME_SIZE  14
    char               return_socket[XAPI_SOCKET_NAME_SIZE];
    time_t             startTime;      /* Time request was initiated */
    time_t             eventTime;      /* Time of last request event */
    char              *pAcsapiBuffer;  /* Address of ACSAPI request  */
    int                acsapiBufferSize;   /* Size of ACSAPI request */
    int                seqNumber;      /* ACSAPI SEQ_NO (PACKET_ID)  */
    int                packetCount;    /* Packet count               */
    pid_t              xapiPid;        /* Pid of child XAPI thread   */
    int                _f0[4];         /* Available                  */
    /*****************************************************************/
    /* Rather than keep XAPICFG chains and use counts, each XAPI     */
    /* thread will copy the existing XAPICFG table (with access      */
    /* serialized using the XAPICVT.xapicfgLock semaphore).          */
    /*****************************************************************/
    int                xapicfgCount;   /* Drives in XAPICFG table    */
    int                xapicfgSize;    /* Size of XAPICFG table      */
    struct XAPICFG    *pXapicfg;       /* Address of XAPICFG table   */
};


/*********************************************************************/
/* XAPIDRVTYP describes an XAPI-ACSAPI drive (model) table entry in  */
/* the XAPI-ACSAPI drive type conversion table.                      */
/*                                                                   */
/* The XAPIDRVTYP is a fixed length table embedded within the        */
/* XAPICVT.  It has 64 entries which the maximim number of           */
/* single bit entries that can be represented by a 4 byte HSC        */
/* server RMCODE.                                                    */
/*                                                                   */
/* Used in XAPICVT below.                                            */
/*********************************************************************/
struct XAPIDRVTYP
{
#define MAX_COMPATMEDIA        16      /* Max number of compatible   */
    /*                                    ...medias for this drive;  */
    /*                                    ...should be same value as */
    /*                                    ...MM_MAX_COMPAT_TYPES = 16*/
    char               modelNameString[XAPI_MODEL_NAME_SIZE + 1]; 
    unsigned char      acsapiDriveType;    /* ACSAPI drive_type code */
    char               _f0;                /* Available              */
    unsigned char      numCompatMedias;    /* Num compatible medias  */
    /*                                    Compat ACSAPI media_type(s)*/
    unsigned char      compatMediaType[MAX_COMPATMEDIA];              
};


/*********************************************************************/
/* XAPIMEDIA describes an XAPI-ACSAPI media table entry in the       */
/* XAPI-ACSAPI media conversion table.                               */
/*                                                                   */
/* The XAPIMEDIA is a fixed length table embedded within the         */
/* XAPICVT.  It has 64 entries which the maximim number of           */
/* single bit entries that can be represented by a 4 byte HSC        */
/* server RMCODE.                                                    */
/*                                                                   */
/* Used in XAPICVT below.                                            */
/*********************************************************************/
struct XAPIMEDIA
{
#define MAX_COMPATDRIVE        16      /* Max number of compatible   */
    /*                                    ...drives for this media;  */
    /*                                    ...should be same value as */
    /*                                    ...MM_MAX_COMPAT_TYPES = 16*/
    char               mediaNameString[XAPI_MEDIA_NAME_SIZE + 1]; 
    unsigned char      acsapiMediaType;    /* ACSAPI media_type code */
    unsigned char      cleanerMediaFlag;   /* Same values as ACSAPI  */
    /*                                    ...CLN_CART_CAPABILITY     */
    unsigned char      numCompatDrives;    /* Num compatible drives  */
    /*                                    Compat ACSAPI drive_type(s)*/
    unsigned char      compatDriveType[MAX_COMPATDRIVE];              
};


/*********************************************************************/
/* XAPISCRPOOL describes a table entry in the XAPISCRPOOL table      */
/* describing scratch subpools.                                      */
/*                                                                   */
/* The XAPISCRPOOL is a fixed length table embedded within the       */
/* XAPICVT.  It has 256 entries; entry (or subpool index) = 0 is     */
/* for the default subpool; entries 1-255 are for the named          */
/* subpools.                                                         */
/*                                                                   */
/* Used in XAPICVT below.                                            */
/*********************************************************************/
struct XAPISCRPOOL
{
    short              subpoolIndex;   /* Scratch subpool index      */
    /*                                    ...0 = DEFAULTPOOL or      */
    /*                                    ...non subpool             */
    char               subpoolNameString[XAPI_SUBPOOL_NAME_SIZE + 1];
    int                threshold;      /* Threshold (low water mark) */
    int                volumeCount;    /* Volume count               */
    int                scratchCount;   /* Scratch count              */
    char               virtualFlag;    /* Virtual or mixed subpool   */
    char               _f0[3];         /* Available                  */
};


/*********************************************************************/
/* XAPICVT describes the XAPI control table.                         */
/*********************************************************************/
struct XAPICVT
{
#define MAX_XAPIREQE           64      /* Max 64 XAPIREQE; max of    */
    /*                                    ...64 active XAPI requests */
#define MAX_XAPIDRVTYP         64      /* Max 64 MODEL names; 8 byte */
    /*                                    ...RMCODE * 8 bits/byte    */
#define MAX_XAPIMEDIA          64      /* Max 64 MEDIA names; 8 byte */
    /*                                    ...RMCODE * 8 bits/byte    */
#define MAX_XAPISCRPOOL        256     /* Max scratch subpool number */
    /*                                    (0 is for non-subpool)     */
    char               cbHdr[8];       /* Control block eyecatcher   */
#define XAPICVT_ID     "XAPICVT"       /* ..."XAPICVT"               */
    int                cvtShMemSegId;  /* XAPICVT segment ID         */
    int                cfgShMemSegId;  /* XAPICFG segment ID         */
    char               configToken[16];/* Configuration token        */ 
    pid_t              ssiPid;         /* Pid of parent SSI thread   */
    int                requestCount;   /* XAPI request counter       */
    sem_t              xapicfgLock;    /* Semaphore to serialize     */
    /*                                    ...access to XAPICFG       */
    time_t             startTime;      /* Time XAPI first initialized*/
    time_t             configTime;     /* Time config last built     */
    time_t             scrpoolTime;    /* Time scrpool last built    */
#define MAX_SCRPOOL_AGE_SECS   3600    /* Max age (in seconds) before*/
    /*                                    ...XAPISCRPOOL is updated  */
    int                xapicfgCount;   /* Drives in XAPICFG table    */
    int                xapicfgSize;    /* Size of XAPICFG table      */
    struct XAPICFG    *pXapicfg;       /* Address of XAPICFG table   */
    int                logPipeFd;      /* Log pipe descriptor        */
    int                tracePipeFd;    /* Trace pipe descriptor      */
    int                _f0[6];         /* Available                  */
    char               updateConfig;   /* XAPICFG update required    */
    char               updateScrpool;  /* XAPISCRPOOL update required*/
    char               xapidrvtypCount;/* Drive table entry count    */
    char               xapimediaCount; /* Media table entry count    */
    char               status;         /* XAPI status                */
#define XAPI_ACTIVE            0       /* ...XAPI active or start'ed */
#define XAPI_IDLE_PENDING      1       /* ...XAPI idle pending       */
#define XAPI_IDLE              2       /* ...XAPI idle               */
    char               virtualFlag;    /* XAPI virtual drive flag    */
#define VIRTUAL_ENABLED        0x80    /* ...Virtual drives defined  */
    char               ipafFlag;       /* IP address family flag     */
#define XAPI_IPAF_UNSPEC       0       /* ...Default (unspecified)   */
    /*                                    ...otherwise will be either*/
    /*                                    ...AF_INET or AF_INET6     */
    char               _f1;            /* Available                  */
    /*****************************************************************/
    /* The following values are defined using environmental          */
    /* variables as follows:                                         */
    /* XAPI_PORT       : TCP/IP port number of SMC HTTP server on    */
    /*                   remote host.  Default value is 8080.        */
    /* XAPI_HOSTNAME   : Hostname of remote host where SMC HTTP      */
    /*                   server is executing.                        */
    /*                   Value must be specified if the environment  */
    /*                   variable CSI_XAPI_CONVERSION=1              */
    /* XAPI_TAPEPLEX   : TapePlex name of remote HSC server.         */
    /*                   Value must be specified if the environment  */
    /*                   variable CSI_XAPI_CONVERSION=1              */
    /* XAPI_SUBSYSTEM  : MVS subsystem name of remote HSC server.    */
    /*                   Value defaults to 1st 4 characters of       */
    /*                   XAPI_TAPEPLEX.                              */
    /* XAPI_VERSION    : Minimum version of XAPI that is supported.  */
    /*                   Default value is 710.                       */
    /* XAPI_USER       : RACF userid passed as part of XAPI          */
    /*                   transaction for XAPI command authorization. */
    /*                   This is optional.                           */
    /* XAPI_GROUP      : RACF groupid passed as part of XAPI         */
    /*                   transaction for XAPI command authorization. */
    /*                   This is optional.                           */
    /*****************************************************************/
    unsigned short     xapiPort;
    char               xapiHostname[XAPI_HOSTNAME_SIZE + 1];
    char               xapiTapeplex[XAPI_TAPEPLEX_SIZE + 1];
    char               xapiSubsystem[XAPI_SUBSYSTEM_SIZE + 1];
    char               xapiVersion[XAPI_VERSION_SIZE + 1];
    char               xapiUser[XAPI_USER_SIZE + 1];
    char               xapiGroup[XAPI_GROUP_SIZE + 1];
    char               xapiEjectText[XAPI_EJECT_TEXT_SIZE + 1];
    /*****************************************************************/
    /* The XAPIREQE request table.                                   */
    /*****************************************************************/
    struct XAPIREQE    xapireqe[MAX_XAPIREQE];     /* Table entries  */
    /*****************************************************************/
    /* The XAPIDRVTYP table.                                         */
    /*****************************************************************/
    struct XAPIDRVTYP  xapidrvtyp[MAX_XAPIDRVTYP]; /* Table entries  */
    /*****************************************************************/
    /* The XAPIMEDIA table.                                          */
    /*****************************************************************/
    struct XAPIMEDIA   xapimedia[MAX_XAPIMEDIA];   /* Table entries  */
    /*****************************************************************/
    /* The XAPISCRPOOL table.                                        */
    /*****************************************************************/
    struct XAPISCRPOOL xapiscrpool[MAX_XAPISCRPOOL];
};


/*********************************************************************/
/* Macros:                                                           */
/*********************************************************************/

/*********************************************************************/
/* The following macros provide a means of converting an unquoted    */
/* literal to a string.                                              */
/*********************************************************************/
#define MAKE_STRING0(inStrinG) # inStrinG
#define MAKE_STRING(inStrinG) MAKE_STRING0(inStrinG)


/*********************************************************************/
/* The following macro provides a means of formatting a double word  */
/* (dword or long long) in hexadecimal for output.                   */
/*********************************************************************/
#define FORMAT_LONGLONG(inLongLonG, outStrinG) \
do \
{ \
    int long1InT; \
    int long2InT; \
    char longLongChaR[8]; \
    char longStrinG[17]; \
    memcpy(longLongChaR, (char*) &inLongLonG, 8); \
    memcpy((char*) &long1InT, &longLongChaR[0], 4); \
    memcpy((char*) &long2InT, &longLongChaR[4], 4); \
    sprintf(longStrinG, "%08X%08X", long1InT, long2InT); \
    memcpy(outStrinG, longStrinG, sizeof(outStrinG)); \
} while(0)


/*********************************************************************/
/* The following macro accepts two character field as input, the     */
/* first the input string, and the second, which must be at least    */
/* one character longer than the input string, as output, plus the   */
/* length of the original character field.                           */
/* This maco assumes no imbedded blanks in the text before trailing  */
/* blanks.  To strip trailing blanks, where the text string has      */
/* imbedded blanks, use TRUNCATE_BLANKS.                             */
/*********************************************************************/
#define STRIP_TRAILING_BLANKS(pCharS, OutCStrinG, lengtH) \
do \
{ \
    auto int          copyLengtH = lengtH; \
    auto char        *pBlanK; \
    OutCStrinG[copyLengtH] = ' '; \
    memcpy(OutCStrinG, pCharS, copyLengtH); \
    pBlanK = strchr(OutCStrinG, ' '); \
    if (pBlanK != NULL) \
    { \
        *pBlanK = 0; \
    } \
}while(0)


/*********************************************************************/
/* The following macro accepts two character field as input, the     */
/* first the input string, and the second, which must be at least    */
/* one character longer than the input string, as output, plus the   */
/* length of the original character field.                           */
/* This macro can be used instead of STRIP_TRAILING_BLANKS when      */
/* the text contains imbedded blanks.                                */
/*********************************************************************/
#define TRUNCATE_BLANKS(pCharS, OutCStrinG, lengtH) \
do \
{ \
    auto int          iX; \
    memcpy(OutCStrinG, pCharS, lengtH); \
    OutCStrinG[(lengtH)] = 0; \
    for (iX = lengtH-1; iX > 0; iX--) \
    { \
        if (OutCStrinG[iX] > ' ') \
        { \
            OutCStrinG[(iX+1)] = 0; \
            break; \
        } \
    } \
}while(0)


/*********************************************************************/
/* Prototypes:                                                       */
/*********************************************************************/
extern int xapi_ack_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize);

extern struct XAPICFG *xapi_attach_xapicfg(struct XAPICVT *pXapicvt);

extern struct XAPICVT *xapi_attach_xapicvt(void);

extern void xapi_attach_work_xapireqe(struct XAPICVT  *pXapicvt,
                                      struct XAPIREQE *pDummyXapireqe,
                                      char            *pAcsapiBuffer,
                                      int              acsapiBufferSize);

extern int xapi_audit(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern int xapi_cancel(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe);

extern int xapi_check_reg(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe);

extern int xapi_clr_lock(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern int xapi_clr_lock_drv(struct XAPICVT  *pXapicvt,
                             struct XAPIREQE *pXapireqe);

extern int xapi_clr_lock_vol(struct XAPICVT  *pXapicvt,
                             struct XAPIREQE *pXapireqe);

extern int xapi_config(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe);

extern struct XAPICFG *xapi_config_search_hexdevaddr(struct XAPICVT  *pXapicvt,
                                                     struct XAPIREQE *pXapireqe,
                                                     unsigned short   hexDevAddr);

extern struct XAPICFG *xapi_config_search_drivelocid(struct XAPICVT  *pXapicvt,
                                                     struct XAPIREQE *pXapireqe,
                                                     char             driveLocId[16]);

extern struct XAPICFG *xapi_config_search_libdrvid(struct XAPICVT  *pXapicvt,
                                                   struct XAPIREQE *pXapireqe,
                                                   struct LIBDRVID *pLibdrvid);

extern struct XAPICFG *xapi_config_search_vtss(struct XAPICVT  *pXapicvt,
                                               struct XAPIREQE *pXapireqe,
                                               char            *vtssNameString);

extern int xapi_define_pool(struct XAPICVT  *pXapicvt,
                            struct XAPIREQE *pXapireqe);

extern int xapi_delete_pool(struct XAPICVT  *pXapicvt,
                            struct XAPIREQE *pXapireqe);

extern int xapi_dismount(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern int xapi_display(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe);

extern void xapi_drive_increment(struct XAPIDRANGE *pXapidrange);

extern int xapi_drive_list(struct XAPICVT    *pXapicvt,
                           struct XAPIREQE   *pXapireqe,
                           struct XAPIDRLST **ptrXapidrlst,
                           int               *pNumXapidrlst);

extern int xapi_drvtyp(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe);

extern struct XAPIDRVTYP *xapi_drvtyp_search_name(struct XAPICVT  *pXapicvt,
                                                  struct XAPIREQE *pXapireqe,
                                                  char            *modelNameString);

extern struct XAPIDRVTYP *xapi_drvtyp_search_type(struct XAPICVT  *pXapicvt,
                                                  struct XAPIREQE *pXapireqe,
                                                  char             acsapiDriveType);

extern int xapi_eject(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern int xapi_eject_response(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char            *pEjectResponse,
                               void            *pResponseXmlparse);

extern int xapi_enter(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern int xapi_err_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize,
                             int              errorStatus);

extern int xapi_fin_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize);

extern int xapi_idle(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_idle_test(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe,
                          char            *pResponseBuffer,
                          int              responseBufferSize);

extern int xapi_int_response(struct XAPIREQE *pXapireqe,
                             char            *pResponseBuffer,
                             int              responseBufferSize);

extern int xapi_lock(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_lock_drv(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern void xapi_lock_init_resp(struct XAPIREQE *pXapireqe,
                                char            *pLockResponse,
                                int              lockResponseSize);

extern int xapi_lock_vol(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern int xapi_main(char *pAcsapiBuffer,
                     int   acsapiBufferSize,
                     int   xapireqeIndex,
                     char  processFlag);

extern int xapi_media(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern struct XAPIMEDIA *xapi_media_search_name(struct XAPICVT  *pXapicvt,
                                                struct XAPIREQE *pXapireqe,
                                                char            *mediaNameString);

extern struct XAPIMEDIA *xapi_media_search_type(struct XAPICVT  *pXapicvt,
                                                struct XAPIREQE *pXapireqe,
                                                char             acsapiMediaType);

extern int xapi_mount(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern int xapi_mount_pinfo(struct XAPICVT  *pXapicvt,
                            struct XAPIREQE *pXapireqe);

extern int xapi_mount_scr(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe);

extern int xapi_parse_header_trailer(struct XAPICVT    *pXapicvt,
                                     struct XAPIREQE   *pXapireqe,
                                     void              *pResponseXmlparse,
                                     struct XAPICOMMON *pXapicommon);

extern void xapi_parse_loctype_from_rawvolume(struct RAWVOLUME *pRawvolume);

extern void xapi_parse_rectechs_from_xmlparse(void             *pResponseXmlparse,
                                              void             *pStartXmlelem,
                                              struct RAWVOLUME *pRawvolume);

extern int xapi_qacs(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_qcap(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_qdrv(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_qdrv_group(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe);

extern int xapi_qfree(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe,
                      int             *pFreeCellCount);

extern int xapi_qlock(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern int xapi_qlock_drv(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe);

extern int xapi_qlock_drv_one(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              struct RAWQLOCK  *pRawqlock,
                              char              driveLocId[16],
                              int               lockId);

extern void xapi_qlock_init_resp(struct XAPIREQE *pXapireqe,
                                 char            *pQueryResponse,
                                 int              queryResponseSize);

extern int xapi_qlock_vol(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe);

extern int xapi_qlock_vol_one(struct XAPICVT   *pXapicvt,
                              struct XAPIREQE  *pXapireqe,
                              struct RAWQLOCK  *pRawqlock,
                              char              volser[6],
                              int               lockId);

extern int xapi_qmedia(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe);

extern int xapi_qmnt_one(struct XAPICVT   *pXapicvt,
                         struct XAPIREQE  *pXapireqe,
                         struct RAWSCRMNT *pRawscrmnt,
                         struct RAWDRLST  *pRawdrlst,
                         char             *subpoolNameString,
                         char             *mediaNameString,
                         char             *managementClassString);

extern int xapi_qmnt_pinfo(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe);

extern int xapi_qmnt_scr(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern int xapi_qmnt(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_qpool(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern int xapi_qrequest(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern int xapi_qscr(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_qserver(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe);

extern int xapi_qsubpool(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern int xapi_query(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern void xapi_query_init_resp(struct XAPIREQE *pXapireqe,
                                 char            *pQueryResponse,
                                 int              queryResponseSize);

extern int xapi_qvol(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_qvol_all(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern int xapi_qvol_one(struct XAPICVT   *pXapicvt,
                         struct XAPIREQE  *pXapireqe,
                         struct RAWVOLUME *pRawvolume,
                         struct RAWDRLST  *pRawdrlst,
                         char              volser[6]);

extern int xapi_register(struct XAPICVT  *pXapicvt,
                         struct XAPIREQE *pXapireqe);

extern void xapi_request_header(struct XAPICVT    *pXapicvt,
                                struct XAPIREQE   *pXapireqe,
                                struct XMLHDRINFO *pXmlhdrinfo,
                                void              *pRequestXmlparse,
                                char               xmlResponseFlag,
                                char               textResponseFlag,
                                char              *xmlCaseString,
                                char              *xmlDateString);

extern int xapi_scrpool(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe,
                        char             processFlag);

extern int xapi_scrpool_counts(struct XAPICVT  *pXapicvt,
                               struct XAPIREQE *pXapireqe,
                               char             processFlag);

extern struct XAPISCRPOOL *xapi_scrpool_search_index(struct XAPICVT  *pXapicvt,
                                                     struct XAPIREQE *pXapireqe,
                                                     short            subpoolIndex);

extern struct XAPISCRPOOL *xapi_scrpool_search_name(struct XAPICVT  *pXapicvt,
                                                    struct XAPIREQE *pXapireqe,
                                                    char            *subpoolNameString);

extern int xapi_set_cap(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe);

extern int xapi_set_clean(struct XAPICVT  *pXapicvt,
                          struct XAPIREQE *pXapireqe);

extern int xapi_set_scr(struct XAPICVT  *pXapicvt,
                        struct XAPIREQE *pXapireqe);

extern int xapi_start(struct XAPICVT  *pXapicvt,
                      struct XAPIREQE *pXapireqe);

extern int xapi_tcp(struct XAPICVT  *pXapicvt,
                    struct XAPIREQE *pXapireqe,
                    char            *pXapiBuffer,
                    int              xapiBufferSize,
                    int              sendTimeout,
                    int              recvTimeout1st,
                    int              recvTimeoutNon1st,
                    void           **ptrResponseXmlparse);

extern int xapi_unlock(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe);

extern int xapi_unlock_drv(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe);

extern int xapi_unlock_vol(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe);

extern int xapi_unregister(struct XAPICVT  *pXapicvt,
                           struct XAPIREQE *pXapireqe);

extern void xapi_userid(struct XAPICVT *pXapicvt,
                        char           *pMessageHeader,
                        char           *pUseridString);

extern int xapi_validate_single_drive(char  drive[16],
                                      char *driveFormatFlag);

extern int xapi_validate_single_volser(char volser[6]);

extern int xapi_validate_drive_range(struct XAPIDRANGE *pXapidrange);

extern int xapi_validate_volser_range(struct XAPIVRANGE *pXapivrange);

extern int xapi_vary(struct XAPICVT  *pXapicvt,
                     struct XAPIREQE *pXapireqe);

extern int xapi_venter(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe);

extern void xapi_volser_increment(struct XAPIVRANGE *pXapivrange);

extern int xapi_xeject(struct XAPICVT  *pXapicvt,
                       struct XAPIREQE *pXapireqe);


#endif                                           /* XAPI_HEADER      */


