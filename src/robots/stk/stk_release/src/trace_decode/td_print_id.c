#ifndef lint
static char SccsId[] = "@(#)td_print_id.c	2.0 1/10/94 ";
#endif
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *      td_print_id 
 *
 * Description:
 *      This module contains functions to format an id
 *      (ex lsmid, capid)
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
 *
 */


/*
 * header files
 */
#include <string.h>
#include "td.h"

/*
 * global variables
 */
static char idbuf[SIZEOFIDBUF];

void
td_print_volid(VOLID *volid)
{
    td_print("volume_id", volid, sizeof(VOLID),
	     cl_identifier(TYPE_VOLUME, (IDENTIFIER *) volid));
}

void
td_print_volid_list(VOLID *volid, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_volid(volid++);
}

void
td_print_volrange(VOLRANGE *volrange)
{
    strcpy(idbuf,
	   cl_identifier(TYPE_VOLUME, (IDENTIFIER *) &volrange->startvol)); 
    strcat(idbuf, "-");
    strcat(idbuf,
	   cl_identifier(TYPE_VOLUME, (IDENTIFIER *) &volrange->endvol)); 

    td_print("volume_range", volrange, sizeof(VOLRANGE), idbuf);
}

void
td_print_volrange_list(VOLRANGE *volrange, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_volrange(volrange++);
}

void
td_print_driveid(DRIVEID *driveid)
{
    td_print("drive_id", driveid, sizeof(DRIVEID),
	     td_rm_space(cl_identifier(TYPE_DRIVE, (IDENTIFIER *) driveid)));
}

void
td_print_driveid_list(DRIVEID *driveid, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_driveid(driveid++);
}

void
td_print_msgid(MESSAGE_ID *msg)
{
    td_print("message_id", msg, sizeof(MESSAGE_ID), td_ustoa(*msg));
}			 

void
td_print_msgid_list(MESSAGE_ID *msg, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_msgid(msg++);
}

void
td_print_acs(ACS *acs)
{
    td_print("acs_id", acs, sizeof(ACS),
	     td_rm_space(cl_identifier(TYPE_ACS, (IDENTIFIER *) acs)));
}

void
td_print_acs_list(ACS *acs, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_acs(acs++);
}

void
td_print_lsmid(LSMID *lsm)
{
    td_print("lsm_id", lsm, sizeof(LSMID),
	     td_rm_space(cl_identifier(TYPE_LSM, (IDENTIFIER *) lsm)));
}

void
td_print_lsmid_list(LSMID *lsm, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_lsmid(lsm++);
} 

void
td_print_v0_capid(V0_CAPID *capid)
{
    /* version 0 capid is the same as lsm id */
    td_print("cap_id", capid, sizeof(V0_CAPID),
	     td_rm_space(cl_identifier(TYPE_LSM, (IDENTIFIER *) capid)));
}

void
td_print_v0_capid_list(V0_CAPID *capid, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_v0_capid(capid++);
} 

void
td_print_capid(CAPID *capid)
{
    td_print("cap_id", capid, sizeof(CAPID),
	     td_rm_space(cl_identifier(TYPE_CAP, (IDENTIFIER *) capid)));
}

void
td_print_capid_list(CAPID *capid, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_capid(capid++);
} 

void
td_print_cellid(CELLID *cellid)
{
    td_print("cell_id", cellid, sizeof(CELLID),
	     td_rm_space(cl_identifier(TYPE_CELL, (IDENTIFIER *) cellid)));
}

void
td_print_portid(PORTID* portid)
{
    td_print("port_id", portid, sizeof(PORTID),
	     td_rm_space(cl_identifier(TYPE_PORT, (IDENTIFIER *) portid)));
}

void
td_print_portid_list(PORTID *portid, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_portid(portid++);
} 

void
td_print_poolid(POOLID* poolid)
{
    td_print("pool_id", poolid, sizeof(POOLID),
	     td_rm_space(cl_identifier(TYPE_POOL, (IDENTIFIER *) poolid)));
}

void
td_print_poolid_list(POOLID *poolid, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_poolid(poolid++);
} 

void
td_print_panelid(PANELID *panel)
{
    td_print("panel_id", panel, sizeof(PANELID),
	     td_rm_space(cl_identifier(TYPE_PANEL, (IDENTIFIER *) panel)));
}

void
td_print_panelid_list(PANELID *panel, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_panelid(panel++);
}

void
td_print_subpanelid(SUBPANELID *subpanel)
{
    td_print("subpanel_id", subpanel, sizeof(SUBPANELID),
	     td_rm_space(cl_identifier(TYPE_SUBPANEL, (IDENTIFIER *) subpanel)));
}

void
td_print_subpanelid_list(SUBPANELID *subpanel, unsigned short count)
{
    unsigned short i;

    for (i = 0; i < count; i++)
        td_print_subpanelid(subpanel++);
}

void
td_print_lockid(LOCKID *lockid)
{
    td_print("lock_id", lockid, sizeof(LOCKID),
	     cl_identifier(TYPE_LOCK, (IDENTIFIER *) lockid));
}

void
td_print_userid(USERID *userid)
{
    td_print("user_id", userid, sizeof(USERID), userid->user_label);
}

void
td_print_id_list(IDENTIFIER *id, TYPE type, unsigned short count, VERSION ver)
{
    switch(type) {
      case TYPE_ACS:
	td_print_acs_list((ACS *)id, count);
	break;
      case TYPE_LSM:
	td_print_lsmid_list((LSMID *)id, count);
	break;
      case TYPE_CAP:
	if (ver <= VERSION1)
	    td_print_v0_capid_list((V0_CAPID *)id, count);
	else
	    td_print_capid_list((CAPID *)id, count);
	break;
      case TYPE_DRIVE:
	td_print_driveid_list((DRIVEID *)id, count);
	break;
      case TYPE_VOLUME:
      case TYPE_CLEAN:
      case TYPE_MOUNT:
	td_print_volid_list((VOLID *)id, count);
	break;
      case TYPE_REQUEST:
	td_print_msgid_list((MESSAGE_ID *)id, count);
	break;
      case TYPE_PORT:
	td_print_portid_list((PORTID *)id, count);
	break;
      case TYPE_POOL:
      case TYPE_SCRATCH:
      case TYPE_MOUNT_SCRATCH:
	td_print_poolid_list((POOLID *)id, count);
	break;
      case TYPE_PANEL:
	td_print_panelid_list((PANELID *)id, count);
	break;
      case TYPE_SUBPANEL:
	td_print_subpanelid_list((SUBPANELID *)id, count);
	break;
      case TYPE_MEDIA_TYPE:
	td_print_media_type_list((MEDIA_TYPE *)id, count);
	break;
      default:
	printf("Invalid type : %d\n", type);
    }
}















