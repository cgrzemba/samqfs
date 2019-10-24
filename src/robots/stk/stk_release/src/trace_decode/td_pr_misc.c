#ifndef lint
static char SccsId[] = "@(#)td_pr_misc.c        2.2 10/10/01 ";
#endif
/*
 * Copyright (1993, 2013) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_print_misc 
 *
 * Description:
 *      This module contains functions to format a common data type
 *      (ex type, count, RESPONSE_STATUS,...)
 *      and call td_print to print it.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      NONE
 *
 * Implicit Outputs:
 *      NONE
 *
 * Considerations:
 *      NONE
 *
 * Revision History:
 *
 *      M. H. Shum          10-Sep-1993     Original.
 *      S. L. Siao          01-Nov-2001     Added print routines for event 
 *                                          notification which
 *                                          include adding the following 
 *                                          print routines:
 *                                          td_print_reply_type
 *                                          td_print_volume_event_type
 *                                          td_print_resource_type
 *                                          td_print_resource_identifier
 *                                          td_print_resource_event
 *                                          td_print_resource_data_type
 *                                          td_print_category
 *                                          td_print_code
 *                                          td_print_type_hli
 *                                          td_print_sense_key
 *                                          td_print_asc
 *                                          td_print_ascq
 *                                          td_print_type_scsi
 *                                          td_print_type_fsc
 *                                          td_print_type_serial_num
 *                                          td_print_type_lsm
 *                                          td_print_eventClass_list
 *                                          td_print_register_return
 *                                          td_print_registration_id
 *                                          td_print_event_sequence
 *                                          td_print_register_status
 *      S. L. Siao          13-Nov-2001     Added print routines for display 
 *                                          td_print_xml_data,
 *                                          td_print_xml_data_length,
 *                                          td_print_xml_data_data.
 *      S. L. Siao          26-Feb-2002     Added print routines for virtual:
 *                                          td_print_mgmt_clas,
 *                                          td_print_dataset_name,
 *                                          td_print_step_name
 *                                          td_print_job_name,
 *                                          td_print_group_type
 *                                          td_print_group_id,
 *                                          td_print_subpool_name,
 *      S. L. Siao          01-Mar-2002     Changed VOL_MANUALLY_DELETED to
 *                                          VOL_DELETED.
 *      S. L. Siao          26-Mar-2002     Changed td_print_xml_data_data
 *                                          so that proper length used in
 *                                          output.
 *      Wipro (Hemendra)    18-Jun-2004     Support for mount/ dismount events (for CIM)
 *                                          Modified td_print_event_class_type
 *                                          td_print_resource_data_type
 *                                          td_print_reply_type
 *                                          Added td_print_drive_activity_type
 *                                          td_print_drive_activity_data
 *                                          td_print_volume_type
 *                                          Added a constant TIME_STRING_LENGTH
 *      Mitch Black         06-Dec-2004     For AIX, cast the argument to ctime()
 *                                          to be an (int *) rather than (long *).
 *      Mitch Black         13-Dec-2004     Ifdef the previous change for AIX.
 *      Mitch Black         11-Apr-2005     Remove td_rm_space call against value 
 *                                          returned by cl_resource_event.  It is never
 *                                          needed, and can cause a core dump on 
 *                                          some systems (value is a string literal,
 *                                          and program behavior when modifying one of
 *                                          those is undefined- See K&R "String Literals"). 
 *      Joseph Nofi         15-Aug-2011     XAPI support;
 *                                          Added call to td_ustodev to convert CCUU
 *                                          format device addresses. 
 *      Chris Morrison       3-Jan-2013     Add new VOLUME_TYPE enums.
 *
 */


/*
 * header files
 */
#include <stdlib.h>
#include <time.h>
#include "td.h"

#define TIME_STRING_LENGTH  26 /*  Length of a string returned by ctime() system call. */

void
td_print_command(COMMAND *command)
{
    td_print("command", command, sizeof(COMMAND), cl_command(*command));
}

void
td_print_type(TYPE *type)
{
    td_print("type", type, sizeof(TYPE), cl_type(*type));
}

void
td_print_count(unsigned short *count)
{
    td_print("count", count, sizeof(unsigned short), td_ustoa(*count));
}

void
td_print_state(STATE *state)
{
    td_print("state", state, sizeof(STATE), cl_state(*state));
}

void
td_print_status(STATUS *status)
{
    td_print("status", status, sizeof(STATUS), cl_status(*status));
}

void
td_print_freecells(FREECELLS *freecells)
{
    td_print("freecells", freecells, sizeof(long), td_ltoa(*freecells));
}

void
td_print_low_water_mark(unsigned long *low_water_mark)
{
    td_print("low_water_mark", low_water_mark, sizeof(unsigned long),
         td_ultoa(*low_water_mark));
}

void
td_print_high_water_mark(unsigned long *high_water_mark)
{
    td_print("high_water_mark", high_water_mark, sizeof(unsigned long),
         td_ultoa(*high_water_mark));
}

void
td_print_pool_attr(unsigned long *pool_attr)
{
    td_print("pool_attributes", pool_attr, sizeof(unsigned long),
         td_ultoa(*pool_attr));
}

void
td_print_cur_use(unsigned short *cur_use)
{
    td_print("cur_use", cur_use, sizeof(unsigned short),
         td_ustoa(*cur_use));
}

void
td_print_max_use(unsigned short *max_use)
{
    td_print("max_use", max_use, sizeof(unsigned short),
         td_ustoa(*max_use));
}

void
td_print_location(LOCATION *location)
{
    static char *location_str[4] = { "LOCATION_FIRST",
                     "LOCATION_CELL",
                     "LOCATION_DRIVE",
                     "LOCATION_LAST" };

    if (*location > LOCATION_FIRST && *location < LOCATION_LAST)
    td_print("location", location, sizeof(LOCATION), location_str[*location]);
    else
    td_print("location", location, sizeof(LOCATION), "Invalid value");
}

void
td_print_cap_mode(CAP_MODE *cap_mode)
{
    static char *cap_mode_str[5] = { "CAP_MODE_FIRST",
                     "CAP_MODE_AUTOMATIC",
                     "CAP_MODE_MANUAL",
                     "CAP_MODE_SAME",
                     "CAP_MODE_LAST" };

    if (*cap_mode > CAP_MODE_FIRST && *cap_mode < CAP_MODE_LAST)
    td_print("cap_mode", cap_mode, sizeof(CAP_MODE), cap_mode_str[*cap_mode]);
    else
    td_print("cap_mode", cap_mode, sizeof(CAP_MODE), "Invalid value");
}

void
td_print_cap_priority(CAP_PRIORITY *cap_priority)
{
    td_print("cap_priority", cap_priority, sizeof(CAP_PRIORITY),
         td_ctoa(*cap_priority));
}

void
td_print_media_type(MEDIA_TYPE *mtype)
{
    td_print("media_type", mtype, sizeof(MEDIA_TYPE), td_ctoa(*mtype));
}

void
td_print_media_type_list(MEDIA_TYPE *mtype, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
    td_print_media_type(mtype++);
}

void
td_print_drive_type(DRIVE_TYPE *dtype)
{
    td_print("drive_type", dtype, sizeof(DRIVE_TYPE), td_ctoa(*dtype));
}

void
td_print_drive_type_list(DRIVE_TYPE *dtype, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
    td_print_drive_type(dtype++);
}

void
td_print_clean_cart_cap(CLN_CART_CAPABILITY *cln_cart)
{
    static char *clean_cart_str[5] = { "CLN_CART_FIRST",
                       "CLN_CART_NEVER",
                       "CLN_CART_INDETERMINATE",
                       "CLN_CART_ALWAYS",
                       "CLN_CART_LAST" };

    if (*cln_cart > CLN_CART_FIRST && *cln_cart < CLN_CART_LAST)
    td_print("clean capability", cln_cart,
         sizeof(CLN_CART_CAPABILITY), clean_cart_str[*cln_cart]);
    else
    td_print("clean capability", cln_cart,
         sizeof(CLN_CART_CAPABILITY), "Invalid value");
}
    
/*
 * Decode response status
 */
void
td_decode_resp_status(RESPONSE_STATUS *resp_stat)
{
    td_print_status(&resp_stat->status);
    td_print_type(&resp_stat->type);
    td_print("identifier", &resp_stat->identifier, sizeof(resp_stat->identifier),
         td_rm_space(cl_identifier(resp_stat->type, &resp_stat->identifier)));
}

void
td_decode_vol_status(VOLUME_STATUS *vol_status)
{
    td_print_volid(&vol_status->vol_id);
    td_decode_resp_status(&vol_status->status);
}

void
td_decode_vol_status_list(VOLUME_STATUS *vol_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
    td_decode_vol_status(vol_status++);
}

void
td_print_reply_type(EVENT_REPLY_TYPE *event_reply_type)
{
    static char *event_reply_str[10] = { "EVENT_REPLY_FIRST",
                                        "EVENT_REPLY_REGISTER",
                                        "EVENT_REPLY_UNREGISTER",
                                        "EVENT_REPLY_SUPERCEDED",
                                        "EVENT_REPLY_SHUTDOWN",
                                        "EVENT_REPLY_CLIENT_CHECK",
                                        "EVENT_REPLY_RESOURCE",
                                        "EVENT_REPLY_VOLUME",
                                        "EVENT_REPLY_DRIVE_ACTIVITY",
                                        "EVENT_REPLY_LAST" };

    if (*event_reply_type > EVENT_REPLY_FIRST && *event_reply_type < EVENT_REPLY_LAST)
    td_print("event_reply_type", event_reply_type, sizeof(EVENT_REPLY_TYPE),
    event_reply_str[*event_reply_type]);
    else
    td_print("event_reply_type", event_reply_type, sizeof(EVENT_REPLY_TYPE), "Invalid value");
}

void
td_print_volume_event_type(VOL_EVENT_TYPE *vol_event_type)
{
    static char *vol_event_str[8] = { "VOL_FIRST",
                                        "VOL_ENTERED",
                                        "VOL_ADDED",
                                        "VOL_REACTIVATED",
                                        "VOL_EJECTED",
                                        "VOL_DELETED",
                                        "VOL_MARKED_ABSENT",
                                        "VOL_LAST" };

    if (*vol_event_type > VOL_FIRST && *vol_event_type < VOL_LAST)
    td_print("vol_event_type", vol_event_type, sizeof(VOL_EVENT_TYPE),
    vol_event_str[*vol_event_type]);
    else
    td_print("vol_event_type", vol_event_type, sizeof(VOL_EVENT_TYPE), "Invalid value");
}

void
td_print_resource_type(TYPE *type)
{
    td_print("resource_type", type, sizeof(TYPE), cl_type(*type));
}

void
td_print_resource_identifier(TYPE type, IDENTIFIER *identifier)
{
    td_print("resource_identifier", identifier, sizeof(IDENTIFIER),
         td_rm_space(cl_identifier(type, identifier)));
}

void
td_print_resource_event(RESOURCE_EVENT *resource_event)
{

    td_print("resource_event", resource_event, sizeof(RESOURCE_EVENT), 
         cl_resource_event(*resource_event));
         
}

void
td_print_resource_data_type(RESOURCE_DATA_TYPE *resource_data_type)
{
    static char *resource_data_type_str[10] = { "SENSE_TYPE_FIRST",
                               "SENSE_TYPE_NONE",
                               "SENSE_TYPE_HLI",
                               "SENSE_TYPE_SCSI",
                               "SENSE_TYPE_FSC",
                               "RESOURCE_CHANGE_SERIAL_NUM",
                               "RESOURCE_CHANGE_LSM_TYPE",
                               "RESOURCE_CHANGE_DRIVE_TYPE",
                               "DRIVE_ACTIVITY_DATA_TYPE",
                               "SENSE_TYPE_LAST" };

    if (*resource_data_type > SENSE_TYPE_FIRST && *resource_data_type < SENSE_TYPE_LAST)
    if (*resource_data_type == DRIVE_ACTIVITY_DATA_TYPE)
        td_print("drive_activity_data_type", resource_data_type,
         sizeof(RESOURCE_DATA_TYPE), resource_data_type_str[*resource_data_type]);
    else
        td_print("resource_data_type", resource_data_type,
         sizeof(RESOURCE_DATA_TYPE), resource_data_type_str[*resource_data_type]);
    else
    td_print("resource_data_type", resource_data_type,
         sizeof(RESOURCE_DATA_TYPE), "Invalid value");
}

void
td_print_category(int *category)
{
    td_print("category", category, sizeof(int), td_ustoa(*category));
}

void
td_print_code(int *code)
{
    td_print("code", code, sizeof(int), td_ustoa(*code));
}

void
td_print_type_hli(SENSE_HLI *sense_hli)
{
    td_print_category(&sense_hli->category);
    td_print_code(&sense_hli->code);
}

void 
td_print_sense_key(unsigned char *sense_key)
{
    td_print("sense_key", sense_key, sizeof(char), td_htoa(*sense_key));
}

void 
td_print_asc(unsigned char *asc)
{
    td_print("asc", asc, sizeof(char), td_htoa(*asc));
}

void 
td_print_ascq(unsigned char *ascq)
{
    td_print("ascq", ascq, sizeof(char), td_htoa(*ascq)); 
}

void 
td_print_type_scsi(SENSE_SCSI *sense_scsi)
{
    td_print_sense_key(&sense_scsi->sense_key);
    td_print_asc(&sense_scsi->asc);
    td_print_ascq(&sense_scsi->ascq);
}

void 
td_print_type_fsc(SENSE_FSC *sense_fsc)
{
    td_print("fsc", sense_fsc->fsc, sizeof(SENSE_FSC), sense_fsc->fsc);
}

void 
td_print_type_serial_num(SERIAL_NUM *serial_num)
{
    td_print("serial number", serial_num, sizeof(SERIAL_NUM), serial_num->serial_nbr);
}

void 
td_print_type_lsm(LSM_TYPE *lsm_type)
{
    td_print("lsm_type", lsm_type, sizeof(LSM_TYPE), td_ctoa(*lsm_type));
}

void
td_print_register_return(EVENT_CLASS_REGISTER_RETURN *event_class_register_return)
{
    static char *event_class_register_return_str[5] = { "EVENT_REGISTER_FIRST",
                               "EVENT_REGISTER_REGISTERED",
                               "EVENT_REGISTER_UNREGISTERED",
                               "EVENT_REGISTER_INVALID_CLASS",
                               "EVENT_REGISTER_LAST" };

    if (*event_class_register_return > EVENT_REGISTER_FIRST &&
    *event_class_register_return < EVENT_REGISTER_LAST)
    td_print("event_class_register_return", event_class_register_return,
         sizeof(EVENT_CLASS_REGISTER_RETURN), 
         event_class_register_return_str[*event_class_register_return]);
    else
    td_print("event_class_register_return", event_class_register_return,
         sizeof(EVENT_CLASS_REGISTER_RETURN), "Invalid value");
}

void
td_print_registration_id(REGISTRATION_ID *registration_id)
{
    td_print("registration_id", registration_id, sizeof(REGISTRATION_ID),
         registration_id->registration);
}

void
td_print_event_class_type(EVENT_CLASS_TYPE *event_class_type)
{
    static char *event_class_str[5] = { "EVENT_CLASS_FIRST",
                               "EVENT_CLASS_VOLUME",
                               "EVENT_CLASS_RESOURCE",
                           "EVENT_CLASS_DRIVE_ACTIVITY",
                               "EVENT_CLASS_LAST" };
    if (*event_class_type > EVENT_CLASS_FIRST &&
    *event_class_type < EVENT_CLASS_LAST)
    td_print("event_class_type", event_class_type,
         sizeof(EVENT_CLASS_TYPE), 
         event_class_str[*event_class_type]);
    else
    td_print("event_class_type", event_class_type,
         sizeof(EVENT_CLASS_TYPE), "Invalid value");
}

void
td_print_eventClass_list(EVENT_CLASS_TYPE *event_class_type, unsigned short count)
{
    int i;
    for (i=0; i< (int) count; i++, event_class_type++)
    td_print_event_class_type(event_class_type);
}

void
td_print_register_status(EVENT_REGISTER_STATUS *event_register_status, unsigned short count)
{
    int i;
    td_print_registration_id(&event_register_status->registration_id);
    td_print_count(&event_register_status->count);
    for (i=0; i < (int) count; i++) {
    td_print_event_class_type(&event_register_status->register_status[i].event_class);
    td_print_register_return(&event_register_status->register_status[i].register_return);
    }
}
void
td_print_event_sequence(EVENT_SEQUENCE *event_sequence)
{
    td_print("event_sequence", event_sequence, sizeof(EVENT_SEQUENCE), td_ultoa(*event_sequence));
}
void
td_print_xml_data_length(unsigned short *length)
{
    td_print("length", length, sizeof(unsigned short), td_ustoa(*length));
}
void
td_print_xml_data_data(char *xml_data, unsigned short length)
{
    td_print("xml_data", xml_data, length, xml_data);
}
void
td_print_xml_data(DISPLAY_XML_DATA *display_xml_data)
{
    td_print_xml_data_length(&display_xml_data->length);
    td_print_xml_data_data(display_xml_data->xml_data, display_xml_data->length);
}
void
td_print_mgmt_clas(MGMT_CLAS *mgmt_clas)
{
    td_print("mgmt_clas", mgmt_clas, sizeof(MGMT_CLAS), mgmt_clas->mgmt_clas);
}
void
td_print_dataset_name(DATASET_NAME *dataset_name)
{
    td_print("dataset_name", dataset_name, sizeof(DATASET_NAME), 
         dataset_name->dataset_name);
}
void
td_print_step_name(STEP_NAME *step_name)
{
    td_print("step_name", step_name, sizeof(STEP_NAME), step_name->step_name);
}
void
td_print_job_name(JOB_NAME *job_name)
{
    td_print("job_name", job_name, sizeof(JOB_NAME), job_name->job_name);
}
void
td_print_subpool_name(SUBPOOL_NAME *subpool_name)
{
    td_print("subpool_name", subpool_name, sizeof(SUBPOOL_NAME), 
         subpool_name->subpool_name);
}
void
td_print_drive_addr(unsigned short *drive_addr)
{
    td_print("drive_addr", drive_addr, sizeof(unsigned short), 
         td_ustodev(*drive_addr));
}
void
td_print_group_type(GROUP_TYPE *group_type)
{
    static char *group_type_str[3] = { "GROUP_TYPE_FIRST",
                       "GROUP_TYPE_VTSS",
                       "GROUP_TYPE_LAST" };

    if (*group_type > GROUP_TYPE_FIRST && *group_type < GROUP_TYPE_LAST)
    td_print("group_type", group_type, sizeof(GROUP_TYPE), 
         group_type_str[*group_type]);
    else
    td_print("group_type", group_type, sizeof(GROUP_TYPE),
         "Invalid value");
}
void
td_print_group_id(GROUPID *group_id)
{
    td_print("group_id", group_id, sizeof(GROUPID), group_id->groupid);
}
void
td_print_group_info(unsigned short count, GROUPID *group_id)
{
    int i;
    for (i=0; i< (int) count; i++) {
    td_print_group_id(&group_id[i]);
    }
}
void
td_print_subpool_info(unsigned short count, SUBPOOL_NAME *subpool_name)
{
    int i;
    for (i=0; i< (int) count; i++) {
    td_print_subpool_name(&subpool_name[i]);
    }
}
void
td_print_drive_activity_type(TYPE *type) {

    td_print("drive_activity_type", type, sizeof(TYPE), cl_type(*type));
}
void
td_print_volume_type(VOLUME_TYPE *volume_type) {
    static char *volume_type_str[13] = { "VOLUME_TYPE_FIRST",
                    "VOLUME_TYPE_DIAGNOSTIC",
                    "VOLUME_TYPE_STANDARD",
                    "VOLUME_TYPE_DATA",
                    "VOLUME_TYPE_SCRATCH",
                    "VOLUME_TYPE_CLEAN",
                    "VOLUME_TYPE_MVC",
                    "VOLUME_TYPE_VTV",
                    "VOLUME_TYPE_SPENT_CLEANER",
                    "VOLUME_TYPE_MEDIA_ERROR",
                    "VOLUME_TYPE_UNSUPPORTED_MEDIA",
                    "VOLUME_TYPE_C_OR_D",
                    "VOLUME_TYPE_LAST" };

    if (*volume_type > VOLUME_TYPE_FIRST && *volume_type < VOLUME_TYPE_LAST)
        td_print("volume_type", volume_type, sizeof(VOLUME_TYPE),
        volume_type_str[*volume_type]);
    else
        td_print("volume_type", volume_type, sizeof(VOLUME_TYPE), "Invalid value");
}
void
td_print_drive_activity_data(DRIVE_ACTIVITY_DATA *drive_activity_data) {
    char *time_string;

#ifdef AIX
    time_string = ctime((int *)&drive_activity_data->start_time);
#else
    time_string = ctime((long *)&drive_activity_data->start_time);
#endif
    td_print("start_time", &drive_activity_data->start_time, TIME_STRING_LENGTH, time_string);

#ifdef AIX
    time_string = ctime((int *)&drive_activity_data->completion_time);
#else
    time_string = ctime((long *)&drive_activity_data->completion_time);
#endif
    td_print("completion_time", &drive_activity_data->completion_time, TIME_STRING_LENGTH, time_string);

    td_print_volid(&drive_activity_data->vol_id);
    td_print_volume_type(&drive_activity_data->volume_type);
    td_print_driveid(&drive_activity_data->drive_id);
    td_print_poolid(&drive_activity_data->pool_id);
    td_print_cellid(&drive_activity_data->home_location);
}
