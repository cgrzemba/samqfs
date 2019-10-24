static char SccsId[] = "@(#)cl_identifie.c	5.4 10/21/01 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_identifier
 *
 * Description:
 *
 *      Common module that converts an identifier, according to its specified
 *      type, to an equivalent character string counterpart.  If an invalid
 *      type is specified, the string "no format for TYPE_type" is returned.
 *
 *      cl_identifier ensures character string identifiers won't overrun the
 *      formatting buffer by limiting string sizes to IDENTIFIER_SIZE bytes.
 *
 * Return Values:
 *
 *      (char *)pointer to character string.
 *
 * Implicit Inputs:
 *
 *      NONE
 *
 * Implicit Outputs:
 *
 *      NONE
 *
 * Considerations:
 *
 *      NONE
 *
 * Module Test Plan:
 *
 *      NONE
 *
 * Revision History:
 *
 *      D. A. Beidle            25-Jan-1989     Original.
 *      D. F. Reed              03-Feb-1989     Common_lib-ized.
 *      J. W. Montgomery        27-Apr-1990     Added POOL_ID identifier.
 *      D. A. Beidle            14-Jul-1990     Added translations for ANY_ACS
 *                          and ANY_LSM.
 *      D. L. Trachy            27-Oct-1990     Added LOCK_ID identifier
 *      D. A. Beidle            27-Jul-1991     Added support for expanded CAP
 *                          identifier, ANY_CAP and ALL_CAP.  code cleanup.
 *      J. S. Alexander         27-Sep-1991     Relocated IDENTIFIER_SIZE define
 *                          to identifier.h.
 *	D. B. Faremr		01-Feb-1994	support 5 concurent messages
 *	D. B. Faremr		10-Feb-1994	since id_buf is pointer instead
 *						of array, sizeof not usefull
 *      S. L. Siao              31-Oct-2001     Added handling of type hand.
 *      S. L. Siao              05-Dec-2001     Added handling of type ptpid.
 *	Mitch Black		24-Nov-2004	Changed PTP ID to new 
 *						form (master/slave)
 *						Also fixed hand_id.panel_id
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <string.h>
#include <stdio.h>                      /* ANSI-C compatible */

#include "cl_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */
#define MAX_DISTINCT_IDS 5

/*
 *      Global and Static Variable Declarations:
 */

/*
 *      Procedure Type Declarations:
 */


char *
cl_identifier (
    TYPE type,                       /* identifier type */
    IDENTIFIER *id                   /* pointer to identifier to display */
)
{
    int     size;                       /* maximum string length */
					/* array of identifier strings */
    static char id_bufs[MAX_DISTINCT_IDS][IDENTIFIER_SIZE+1];  
    static int next_id = 0;
    char *id_buf;			/* current id pointer */

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_identifier", 2,    /* routine name and parameter count */
                 (unsigned long)type,
                 (unsigned long)id);
#endif /* DEBUG */

    /* set id_buf to next available id_bufs */
    id_buf = id_bufs[next_id];
    ++next_id;
    next_id %= MAX_DISTINCT_IDS;

    switch (type) {
      case TYPE_ACS:
        sprintf(id_buf, "%3d", id->acs_id);
        break;

      case TYPE_CAP:
        if (id->cap_id.lsm_id.acs == ANY_ACS)
            strcpy(id_buf, "  *");      /* generate 3-digit ACS */
        else
            sprintf(id_buf, "%3d", id->cap_id.lsm_id.acs);

        if (id->cap_id.lsm_id.lsm == ANY_LSM)
            strcat(id_buf, ", *");      /* generate 2-digit LSM */
        else
            sprintf(id_buf + strlen(id_buf), ",%2d", id->cap_id.lsm_id.lsm);

        if (id->cap_id.cap == ANY_CAP)
            strcat(id_buf, ",*");       /* generate 1-digit CAP */
        else if (id->cap_id.cap == ALL_CAP)
            strcat(id_buf, "  ");
        else
            sprintf(id_buf + strlen(id_buf), ",%1d", id->cap_id.cap);

        break;

      case TYPE_CELL:
        sprintf(id_buf, "%3d,%2d,%2d,%2d,%2d",
                id->cell_id.panel_id.lsm_id.acs,
                id->cell_id.panel_id.lsm_id.lsm,
                id->cell_id.panel_id.panel,
                id->cell_id.row,
                id->cell_id.col);
        break;

      case TYPE_DRIVE:
        sprintf(id_buf, "%3d,%2d,%2d,%2d",
                id->drive_id.panel_id.lsm_id.acs,
                id->drive_id.panel_id.lsm_id.lsm,
                id->drive_id.panel_id.panel,
                id->drive_id.drive);
        break;

      case TYPE_IPC:
        size = IDENTIFIER_SIZE;
        size = (size < SOCKET_NAME_SIZE) ? size : SOCKET_NAME_SIZE;
        memset(id_buf, 0, IDENTIFIER_SIZE+1);
        strncpy(id_buf, id->socket_name, size);
        break;

      case TYPE_LOCK:
        sprintf(id_buf, "%d",id->lock_id);
        break;

      case TYPE_LSM:
        sprintf(id_buf, "%3d,%2d", id->lsm_id.acs, id->lsm_id.lsm);
        break;

      case TYPE_NONE:
      case TYPE_SERVER:
        memset(id_buf, 0, IDENTIFIER_SIZE+1);
        break;

      case TYPE_PANEL:
        sprintf(id_buf, "%3d,%2d,%2d",
                id->panel_id.lsm_id.acs,
                id->panel_id.lsm_id.lsm,
                id->panel_id.panel);
        break;

      /* Changed to new form PTP ID; master/slave */
      case TYPE_PTP:
        sprintf(id_buf, "%3d,%3d,%2d,%3d,%2d",
                id->ptp_id.acs,
                id->ptp_id.master_lsm,
                id->ptp_id.master_panel,
                id->ptp_id.slave_lsm,
                id->ptp_id.slave_panel);
        break;

      case TYPE_POOL:
        sprintf(id_buf, "%2d", id->pool_id.pool);
        break;

      case TYPE_PORT:
        sprintf(id_buf, "%3d,%2d", id->port_id.acs, id->port_id.port);
        break;

      case TYPE_SUBPANEL:
        sprintf(id_buf, "%3d,%2d,%2d,%2d,%2d,%2d,%2d",
                id->subpanel_id.panel_id.lsm_id.acs,
                id->subpanel_id.panel_id.lsm_id.lsm,
                id->subpanel_id.panel_id.panel,
                id->subpanel_id.begin_row,
                id->subpanel_id.begin_col,
                id->subpanel_id.end_row,
                id->subpanel_id.end_col);
        break;

      case TYPE_VOLUME:
        size = IDENTIFIER_SIZE;
        size = (size < EXTERNAL_LABEL_SIZE) ? size : EXTERNAL_LABEL_SIZE;
        memset(id_buf, 0, IDENTIFIER_SIZE+1);
        strncpy(id_buf, id->vol_id.external_label, size);
        break;

      case TYPE_HAND:
        sprintf(id_buf, "%3d,%2d,%2d,%2d",
                id->hand_id.panel_id.lsm_id.acs,
                id->hand_id.panel_id.lsm_id.lsm,
                id->hand_id.panel_id.panel,
                id->hand_id.hand);
        break;

      default:
        sprintf(id_buf, "no format for TYPE_%s", cl_type(type));
        break;
    }

    return id_buf;
}
