#ifndef lint
static char SccsId[] = "@(#)td_query_p.c	2.2 2/25/02 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_query_resp
 *
 * Description:
 *      Decode the message content of a query response packet.
 *
 * Return Values:
 *      NONE
 *
 * Implicit Inputs:
 *      td_msg_ptr, td_fix_portion, td_var_portion.
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
 *      S. L. Siao          25-Feb-2002     Added support for query_drive_group
 *                                          and query_subpool_name.
 *      S. L. Siao          01-Mar-2002     Added support for 
 *                                          query_mount_scratch_pinfo.
 *
 */

#include <stdio.h>
#include "csi.h"
#include "td.h"

static void st_decode_status_list(char *, TYPE, unsigned short, VERSION);
static void st_decode_response_list(char *, TYPE);
static void st_decode_qu_srv_status(QU_SRV_STATUS *, unsigned short);
static void st_decode_qu_acs_status(QU_ACS_STATUS *, unsigned short);
static void st_decode_qu_lsm_status(QU_LSM_STATUS *, unsigned short);
static void st_decode_qu_cap_status(QU_CAP_STATUS *, unsigned short);
static void st_decode_v0_qu_cap_status(V0_QU_CAP_STATUS *, unsigned short);
static void st_decode_v1_qu_cap_status(V1_QU_CAP_STATUS *, unsigned short);
static void st_decode_qu_clean_status(QU_CLN_STATUS *, unsigned short);
static void st_decode_v2_qu_clean_status(V2_QU_CLN_STATUS *, unsigned short);
static void st_decode_qu_drive_status(QU_DRV_STATUS *, unsigned short);
static void st_decode_v2_qu_drive_status(V2_QU_DRV_STATUS *, unsigned short);
static void st_decode_qu_mount_status(QU_MNT_STATUS *, unsigned short);
static void st_decode_v2_qu_mount_status(V2_QU_MNT_STATUS *, unsigned short);
static void st_decode_qu_vol_status(QU_VOL_STATUS *, unsigned short);
static void st_decode_v2_qu_vol_status(V2_QU_VOL_STATUS *, unsigned short);
static void st_decode_qu_port_status(QU_PRT_STATUS *, unsigned short);
static void st_decode_qu_req_status(QU_REQ_STATUS *, unsigned short);
static void st_decode_qu_scratch_status(QU_SCR_STATUS *, unsigned short);
static void st_decode_v2_qu_scratch_status(V2_QU_SCR_STATUS *, unsigned short);
static void st_decode_qu_pool_status(QU_POL_STATUS *, unsigned short);
static void st_decode_qu_msc_status(QU_MSC_STATUS *, unsigned short);
static void st_decode_v2_qu_msc_status(V2_QU_MSC_STATUS *, unsigned short);
static void st_decode_qu_media_response(QU_MMI_RESPONSE *);
static void st_decode_req_sum(REQ_SUMMARY *);
static void st_decode_drive_group_response(QU_DRG_RESPONSE *);
static void st_decode_subpool_name_response(QU_SPN_RESPONSE *);

void
td_query_resp(VERSION version)
{
    fputs(td_fix_portion, stdout);

    if (version == VERSION0) {
	CSI_V0_QUERY_RESPONSE *query_ptr;
	
	query_ptr = (CSI_V0_QUERY_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&query_ptr->message_status);
	td_print_type(&query_ptr->type);
	td_print_count(&query_ptr->count);

	if (query_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    st_decode_status_list((char *) &query_ptr->status_response,
				  query_ptr->type, query_ptr->count, version);
	}
    }
    else if (version >= VERSION1 && version <= VERSION3) {
	/* version 1 is the same as version 2 and 3
	   except the status_response */
	   
	CSI_V2_QUERY_RESPONSE *query_ptr;
	
	query_ptr = (CSI_V2_QUERY_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&query_ptr->message_status);
	td_print_type(&query_ptr->type);
	td_print_count(&query_ptr->count);

	if (query_ptr->count > 0) {
	    fputs(td_var_portion, stdout);
	    st_decode_status_list((char *) &query_ptr->status_response,
				  query_ptr->type, query_ptr->count, version);
	}
    }
    else {
	/* version 4 */
	CSI_QUERY_RESPONSE *query_ptr;
	
	query_ptr = (CSI_QUERY_RESPONSE *) td_msg_ptr;
	td_decode_resp_status(&query_ptr->message_status);
	td_print_type(&query_ptr->type);

	st_decode_response_list((char *) &query_ptr->status_response,
				query_ptr->type);
    }
}

/*
 * Decode an array of status for version 0 to version 3.
 */
static void
st_decode_status_list(char *status, TYPE type, unsigned short count,
		      VERSION version)
{
    switch (type) {
      case TYPE_SERVER:
	st_decode_qu_srv_status((QU_SRV_STATUS *) status, count);
	break;
      case TYPE_ACS:
	st_decode_qu_acs_status((QU_ACS_STATUS *) status, count);
	break;
      case TYPE_LSM:
	st_decode_qu_lsm_status((QU_LSM_STATUS *) status, count);
	break;
      case TYPE_CAP:
	if (version == VERSION0)
	    st_decode_v0_qu_cap_status((V0_QU_CAP_STATUS *) status, count);
	else if (version == VERSION1)
	    st_decode_v1_qu_cap_status((V1_QU_CAP_STATUS *) status, count);
        else
	    st_decode_qu_cap_status((QU_CAP_STATUS *) status, count);
	break;
      case TYPE_CLEAN:
	st_decode_v2_qu_clean_status((V2_QU_CLN_STATUS *) status, count);
	break;
      case TYPE_DRIVE:
	st_decode_v2_qu_drive_status((V2_QU_DRV_STATUS *) status, count);
	break;
      case TYPE_MOUNT:
	st_decode_v2_qu_mount_status((V2_QU_MNT_STATUS *) status, count);
	break;
      case TYPE_VOLUME:
	st_decode_v2_qu_vol_status((V2_QU_VOL_STATUS *) status, count);
	break;
      case TYPE_PORT:
	st_decode_qu_port_status((QU_PRT_STATUS *) status, count);
	break;
      case TYPE_REQUEST:
	st_decode_qu_req_status((QU_REQ_STATUS *) status, count);
	break;
      case TYPE_SCRATCH:
	st_decode_v2_qu_scratch_status((V2_QU_SCR_STATUS *) status, count);
	break;
      case TYPE_POOL:
	st_decode_qu_pool_status((QU_POL_STATUS *) status, count);
	break;
      case TYPE_MOUNT_SCRATCH:
	st_decode_v2_qu_msc_status((V2_QU_MSC_STATUS *) status, count);
	break;
    }
}

/*
 * Decode an array of status for version 4.
 */
static void
st_decode_response_list(char *resp, TYPE type)
{
    unsigned short *cnt;

    fputs(td_var_portion, stdout);
  
    switch (type) {
      case TYPE_SERVER:
	st_decode_qu_srv_status(&(((QU_SRV_RESPONSE *) resp)->server_status), 1);
	break;
      case TYPE_ACS:
	cnt = &((QU_ACS_RESPONSE *) resp)->acs_count;
	td_print_count(cnt);
	st_decode_qu_acs_status(((QU_ACS_RESPONSE *) resp)->acs_status, *cnt);
	break;
      case TYPE_LSM:
	cnt = &((QU_LSM_RESPONSE *) resp)->lsm_count;
	td_print_count(cnt);
	st_decode_qu_lsm_status(((QU_LSM_RESPONSE *) resp)->lsm_status, *cnt);
	break;
      case TYPE_CAP:
        cnt = &((QU_CAP_RESPONSE *) resp)->cap_count;
	td_print_count(cnt);
	st_decode_qu_cap_status(((QU_CAP_RESPONSE *) resp)->cap_status, *cnt);
	break;
      case TYPE_CLEAN:
	cnt = &((QU_CLN_RESPONSE *) resp)->volume_count;
	td_print_count(cnt);
	st_decode_qu_clean_status(((QU_CLN_RESPONSE *) resp)->clean_volume_status,
				  *cnt);
	break;
      case TYPE_DRIVE:
	cnt = &((QU_DRV_RESPONSE *) resp)->drive_count;
	td_print_count(cnt);
	st_decode_qu_drive_status(((QU_DRV_RESPONSE *) resp)->drive_status, *cnt);
	break;
      case TYPE_MOUNT:
	cnt = &((QU_MNT_RESPONSE *) resp)->mount_status_count;
	td_print_count(cnt);
	st_decode_qu_mount_status(((QU_MNT_RESPONSE *) resp)->mount_status, *cnt);
	break;
      case TYPE_VOLUME:
	cnt = &((QU_VOL_RESPONSE *) resp)->volume_count;
	td_print_count(cnt);
	st_decode_qu_vol_status(((QU_VOL_RESPONSE *) resp)->volume_status, *cnt);
	break;
      case TYPE_PORT:
	cnt = &((QU_PRT_RESPONSE *) resp)->port_count;
	td_print_count(cnt);
	st_decode_qu_port_status(((QU_PRT_RESPONSE *) resp)->port_status, *cnt);
	break;
      case TYPE_REQUEST:
	cnt = &((QU_REQ_RESPONSE *) resp)->request_count;
	td_print_count(cnt);
	st_decode_qu_req_status(((QU_REQ_RESPONSE *) resp)->request_status, *cnt);
	break;
      case TYPE_SCRATCH:
	cnt = &((QU_SCR_RESPONSE *) resp)->volume_count;
	td_print_count(cnt);
	st_decode_qu_scratch_status(((QU_SCR_RESPONSE *) resp)->scratch_status,
				    *cnt);
	break;
      case TYPE_POOL:
	cnt = &((QU_POL_RESPONSE *) resp)->pool_count;
	td_print_count(cnt);
	st_decode_qu_pool_status(((QU_POL_RESPONSE *) resp)->pool_status, *cnt);
	break;
      case TYPE_MOUNT_SCRATCH_PINFO:
      case TYPE_MOUNT_SCRATCH:
	cnt = &((QU_MSC_RESPONSE *) resp)->msc_status_count;
	td_print_count(cnt);
	st_decode_qu_msc_status(((QU_MSC_RESPONSE *) resp)->mount_scratch_status,
				*cnt);
	break;
      case TYPE_MIXED_MEDIA_INFO:
	st_decode_qu_media_response((QU_MMI_RESPONSE *) resp);
	break;
      case TYPE_DRIVE_GROUP:
	st_decode_drive_group_response((QU_DRG_RESPONSE *) resp);
	break;
      case TYPE_SUBPOOL_NAME:
	st_decode_subpool_name_response((QU_SPN_RESPONSE *) resp);
	break;
    }
}

static void
st_decode_qu_srv_status(QU_SRV_STATUS *srv_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, srv_status++) {
	td_print_state(&srv_status->state);
	td_print_freecells(&srv_status->freecells);
	st_decode_req_sum(&srv_status->requests);
    }
}

static void
st_decode_qu_acs_status(QU_ACS_STATUS *acs_status, unsigned short count)
{
    unsigned short i;
    
    for (i = 0; i < count; i++, acs_status++) {
	td_print_acs(&acs_status->acs_id);
	td_print_state(&acs_status->state);
	td_print_freecells(&acs_status->freecells);
	st_decode_req_sum(&acs_status->requests);
	td_print_status(&acs_status->status);
    }
}

static void
st_decode_qu_lsm_status(QU_LSM_STATUS *lsm_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, lsm_status++) {
	td_print_lsmid(&lsm_status->lsm_id);
	td_print_state(&lsm_status->state);
	td_print_freecells(&lsm_status->freecells);
	st_decode_req_sum(&lsm_status->requests);
	td_print_status(&lsm_status->status);
    }
}

static void
st_decode_v0_qu_cap_status(V0_QU_CAP_STATUS *cap_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, cap_status++) {
	td_print_v0_capid(&cap_status->cap_id);
	td_print_status(&cap_status->status);
    }
}

static void
st_decode_v1_qu_cap_status(V1_QU_CAP_STATUS *cap_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, cap_status++) {
	td_print_v0_capid(&cap_status->cap_id);
	td_print_status(&cap_status->status);
	td_print_cap_priority(&cap_status->cap_priority);
	td_print("cap size", &cap_status->cap_size, sizeof(unsigned short),
		 td_ustoa(cap_status->cap_size));
    }
}

static void
st_decode_qu_cap_status(QU_CAP_STATUS *cap_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, cap_status++) {
	td_print_capid(&cap_status->cap_id);
	td_print_status(&cap_status->status);
	td_print_cap_priority(&cap_status->cap_priority);
	td_print("cap size", &cap_status->cap_size, sizeof(unsigned short),
		 td_ustoa(cap_status->cap_size));
	td_print_state(&cap_status->cap_state);
	td_print_cap_mode(&cap_status->cap_mode);
    }
}

static void
st_decode_qu_clean_status(QU_CLN_STATUS *clean_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, clean_status++) {
	td_print_volid(&clean_status->vol_id);
	td_print_media_type(&clean_status->media_type);
	td_print_cellid(&clean_status->home_location);
	td_print_max_use(&clean_status->max_use);
	td_print_cur_use(&clean_status->current_use);
	td_print_status(&clean_status->status);
    }
}

static void
st_decode_v2_qu_clean_status(V2_QU_CLN_STATUS *clean_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, clean_status++) {
	td_print_volid(&clean_status->vol_id);
	td_print_cellid(&clean_status->home_location);
	td_print_max_use(&clean_status->max_use);
	td_print_cur_use(&clean_status->current_use);
	td_print_status(&clean_status->status);
    }
}

static void
st_decode_qu_drive_status(QU_DRV_STATUS *drive_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, drive_status++) {
	td_print_driveid(&drive_status->drive_id);
	td_print_volid(&drive_status->vol_id);
	td_print_drive_type(&drive_status->drive_type);
	td_print_state(&drive_status->state);
	td_print_status(&drive_status->status);
    }
}

static void
st_decode_v2_qu_drive_status(V2_QU_DRV_STATUS *drive_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, drive_status++) {
	td_print_driveid(&drive_status->drive_id);
	td_print_state(&drive_status->state);
	td_print_volid(&drive_status->vol_id);
	td_print_status(&drive_status->status);
    }
}

static void
st_decode_qu_mount_status(QU_MNT_STATUS *mount_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, mount_status++) {
	td_print_volid(&mount_status->vol_id);
	td_print_status(&mount_status->status);
	td_print_count(&mount_status->drive_count);
	st_decode_qu_drive_status(mount_status->drive_status,
				  mount_status->drive_count);
    }
}

static void
st_decode_v2_qu_mount_status(V2_QU_MNT_STATUS *mount_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, mount_status++) {
	td_print_volid(&mount_status->vol_id);
	td_print_status(&mount_status->status);
	td_print_count(&mount_status->drive_count);
	st_decode_v2_qu_drive_status(mount_status->drive_status,
				  mount_status->drive_count);
    }
}

static void
st_decode_qu_vol_status(QU_VOL_STATUS *vol_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, vol_status++) {
	td_print_volid(&vol_status->vol_id);
	td_print_media_type(&vol_status->media_type);
	td_print_location(&vol_status->location_type);
	if (vol_status->location_type == LOCATION_CELL)
	    td_print_cellid(&vol_status->location.cell_id);
	else if (vol_status->location_type == LOCATION_DRIVE)
	    td_print_driveid(&vol_status->location.drive_id);
	else
	    printf("Invalid location type\n");
	td_print_status(&vol_status->status);
    }
}

static void
st_decode_v2_qu_vol_status(V2_QU_VOL_STATUS *vol_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, vol_status++) {
	td_print_volid(&vol_status->vol_id);
	td_print_location(&vol_status->location_type);
	if (vol_status->location_type == LOCATION_CELL)
	    td_print_cellid(&vol_status->location.cell_id);
	else if (vol_status->location_type == LOCATION_DRIVE)
	    td_print_driveid(&vol_status->location.drive_id);
	else
	    printf("Invalid location type\n");
	td_print_status(&vol_status->status);
    }
}

static void
st_decode_qu_port_status(QU_PRT_STATUS *port_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, port_status++) {
	td_print_portid(&port_status->port_id);
	td_print_state(&port_status->state);
	td_print_status(&port_status->status);
    }
}

static void
st_decode_qu_req_status(QU_REQ_STATUS *req_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, req_status++) {
	td_print_msgid(&req_status->request);
	td_print_command(&req_status->command);
	td_print_status(&req_status->status);
    }
}

static void
st_decode_qu_scratch_status(QU_SCR_STATUS *scratch_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, scratch_status++) {
	td_print_volid(&scratch_status->vol_id);
	td_print_media_type(&scratch_status->media_type);
	td_print_cellid(&scratch_status->home_location);
	td_print_poolid(&scratch_status->pool_id);
	td_print_status(&scratch_status->status);
    }
}

static void
st_decode_v2_qu_scratch_status(V2_QU_SCR_STATUS *scratch_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, scratch_status++) {
	td_print_volid(&scratch_status->vol_id);
	td_print_cellid(&scratch_status->home_location);
	td_print_poolid(&scratch_status->pool_id);
	td_print_status(&scratch_status->status);
    }
}

static void
st_decode_qu_pool_status(QU_POL_STATUS *pool_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, pool_status++) {
	td_print_poolid(&pool_status->pool_id);
	td_print("volume count", &pool_status->volume_count,
		 sizeof(pool_status->volume_count),
		 td_ultoa(pool_status->volume_count));
	td_print_low_water_mark(&pool_status->low_water_mark);
	td_print_high_water_mark(&pool_status->high_water_mark);
	td_print_pool_attr(&pool_status->pool_attributes);
	td_print_status(&pool_status->status);
    }
}

/*
 * decode query mount scratch status
 */

static void
st_decode_qu_msc_status(QU_MSC_STATUS *msc_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, msc_status++) {
	td_print_poolid(&msc_status->pool_id);
	td_print_status(&msc_status->status);
	td_print_count(&msc_status->drive_count);
	st_decode_qu_drive_status(msc_status->drive_list,
				  msc_status->drive_count);
    }
}

static void
st_decode_v2_qu_msc_status(V2_QU_MSC_STATUS *msc_status, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++, msc_status++) {
	td_print_poolid(&msc_status->pool_id);
	td_print_status(&msc_status->status);
	td_print_count(&msc_status->drive_count);
	st_decode_v2_qu_drive_status(msc_status->drive_list,
				  msc_status->drive_count);
    }
}

static void
st_decode_qu_media_response(QU_MMI_RESPONSE *mmi_resp)
{
    unsigned short i;
    QU_MEDIA_TYPE_STATUS *mts;
    QU_DRIVE_TYPE_STATUS *dts;
    
    /* decode MEDIA_TYPE_STATUS */
    td_print_count(&mmi_resp->media_type_count);
    mts = mmi_resp->media_type_status;
    for (i = 0; i < mmi_resp->media_type_count; i++, mts++) {
	td_print_media_type(&mts->media_type);
	td_print("media_type_name", &mts->media_type_name,
		 sizeof(mts->media_type_name), mts->media_type_name);
	td_print_clean_cart_cap(&mts->cleaning_cartridge);
	td_print_count(&mts->compat_count);
	td_print_drive_type_list(mts->compat_drive_types, mts->compat_count);
    }

    /* decode DRIVE_TYPE_STATUS */
    td_print_count(&mmi_resp->drive_type_count);
    dts = mmi_resp->drive_type_status;
    for (i = 0; i < mmi_resp->drive_type_count; i++, dts++) {
	td_print_drive_type(&dts->drive_type);
	td_print("drive_type_name", &dts->drive_type_name,
		 sizeof(dts->drive_type_name), dts->drive_type_name);
	td_print_count(&dts->compat_count);
	td_print_media_type_list(dts->compat_media_types, dts->compat_count);
    }
}

/*
 * Decode REQ_SUMMARY, REQ_SUMMARY is a two dimensional array of commands
 * and disposition for the commands AUDIT, DISMOUNT, MOUNT, ENTER and
 * EJECT.
 */
static void
st_decode_req_sum(REQ_SUMMARY *req)
{
    char *com[MAX_COMMANDS] = { "AUDIT", "DISMOUNT", "MOUNT", "ENTER", "EJECT" };
    char *disp[MAX_DISPOSITIONS] = { "current", "pending" }; 
    char desc[20];
    int  i, j;
    
    for (i = 0; i < MAX_COMMANDS; i++) {
	for (j = 0; j < MAX_DISPOSITIONS; j++) {
	    sprintf(desc, "%s %s", com[i], disp[j]);
	    td_print(desc, &req->requests[i][j], sizeof(req->requests[i][j]),
		     td_ustoa(req->requests[i][j]));
	}
    }	     
}

static void
st_decode_drive_group_response(QU_DRG_RESPONSE *drg_resp)
{
    int i;
    td_print_group_id(&drg_resp->group_id);
    td_print_group_type(&drg_resp->group_type);
    td_print_count(&drg_resp->vir_drv_map_count);
    for (i=0; i< (int) drg_resp->vir_drv_map_count; i++) {
        td_print_driveid(&drg_resp->virt_drv_map[i].drive_id);
	td_print_drive_addr(&drg_resp->virt_drv_map[i].drive_addr);
    }
}

static void
st_decode_subpool_name_response(QU_SPN_RESPONSE *resp)
{
    int i;
    td_print_count(&resp->spn_status_count);
    for (i=0; i < (int) resp->spn_status_count; i++) {
	td_print_subpool_name(&resp->subpl_name_status[i].subpool_name);
	td_print_poolid(&resp->subpl_name_status[i].pool_id);
	td_print_status(&resp->subpl_name_status[i].status);
    }	
}

