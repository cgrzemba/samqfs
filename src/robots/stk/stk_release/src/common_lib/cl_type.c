static char SccsId[] = "@(#)cl_type.c	5.9 11/2/01 ";
/*
 * Copyright (1988, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_type
 *
 * Description:
 *
 *      Common routine to convert a TYPE enum value to its character
 *      string counterpart.
 *
 * Return Values:
 *
 *      (char *)pointer to character string.
 *      if value is invalid, pointer to string "code %d".
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
 *      All common_lib test programs.
 *
 * Revision History:
 *
 *      D. A. Beidle            25-Jan-1989     Original.
 *      J. W. Montgomery        19-Mar-1990     Added scratch types.
 *      H. I. Grapek            30-Mar-1990     Added clean types.
 *      D. L. Trachy            04-May-1990     Added lock server types.
 *      J. W. Montgomery        18-May-1990     Added TYPE_SV
 *      J. W. Montgomery        24-May-1990     Added TYPE_MT
 *      D. L. Trachy            29-Jun-1990     Added TYPE_IPC_CLEAN
 *      D. A. Beidle            29-Jun-1990     Added TYPE_SET_CAP. Sorted
 *                      table by type to improve maintainability.
 *      D. A. Beidle            27-Jul-1991     Added TYPE_CM plus all the 
 *                      missing types up to this point.
 *      Alec Sharp              26-Aug-1992     Added TYPE_SET_OWNER
 *	Janet Borzuchowski	12-Aug-1993	R5.0 Mixed Media-- Added
 *			TYPE_MIXED_MEDIA_INFO, and TYPE_MEDIA_TYPE.
 *	Emanuel A. Alongi	12-Oct-1993.	Added TYPE_SSI.
 *	Janet Borzuchowski	02-Nov-1993 	R5.0 Mixed Media-- Changed
 *			copyright dates to 1993 (code review changes).
 *	H. Grapek		11-Jan-1994	Library station internal changes
 *	Alec Sharp		02-Mar-1994	R5.0 BR#145 - added TYPE_CONFIG
 *      Scott Siao              19-Oct-2001     6.1 synchronization; added types:
 *                                              type_missing, errant, surr, han, last.
 *      Scott Siao              19-Nov-2001     Added new types for display:
 *                                              TYPE_GETTYPES, TYPE_BUILDOBJECT,
 *                                              TYPE_DISPLAY, TYPE_ERROR.
 *      Scott Siao              22-Jan-2002     Added new types when syncing
 *                                              with ACSLS:
 *                                              TYPE_DISP, TYPE_PTP, ACS_MON
 *                                              removed TYPE_BUILDOBJECT.
 *      Scott Siao              26-Feb-2002     Added new types:
 *						TYPE_DRIVE_GROUP,
 *						TYPE_SUBPOOL_NAME,
 *						TYPE_MOUNT_PINFO,
 *						TYPE_VTDID,
 *						TYPE_MGMT_CLAS,
 *						TYPE_JOB_NAME,
 *						TYPE_STEP_NAME.
 *      Scott Siao              01-Mar-2002     Added new types:
 *                                              TYPE_MOUNT_SCRATCH_PINFO.
 */

/*
 *      Header Files:
 */
#include "flags.h"
#include "system.h"
#include <stdio.h>

#include "cl_pub.h"
#include "ml_pub.h"

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global and Static Variable Declarations:
 */
static struct type_map {
    TYPE        type_value;
    char       *type_string;
} type_table[] = {
    TYPE_ACS,           "ACS",
    TYPE_AUDIT,         "AUDIT",
    TYPE_CAP,           "CAP",
    TYPE_CELL,          "cell",
    TYPE_CP,            "command process",
    TYPE_CSI,           "CSI",
    TYPE_DISMOUNT,      "DISMOUNT",
    TYPE_EJECT,         "EJECT",
    TYPE_EL,            "ACSEL",
    TYPE_ENTER,         "ENTER",
    TYPE_DRIVE,         "drive",
    TYPE_IPC,           "IPC",
    TYPE_LH,            "ACSLH",
    TYPE_LM,            "ACSLM",
    TYPE_LSM,           "LSM",
    TYPE_MOUNT,         "MOUNT",
    TYPE_NONE,          "none",
    TYPE_PANEL,         "panel",
    TYPE_PORT,          "port",
    TYPE_QUERY,         "QUERY",
    TYPE_RECOVERY,      "RECOVERY",
    TYPE_REQUEST,       "ACSSS request",
    TYPE_SA,            "ACSSA",
    TYPE_SERVER,        "storage server",
    TYPE_SUBPANEL,      "subpanel",
    TYPE_VARY,          "VARY",
    TYPE_VOLUME,        "volume",
    TYPE_PD,            "ACSPD",
    TYPE_SET_SCRATCH,   "SET_SCRATCH",
    TYPE_DEFINE_POOL,   "DEFINE_POOL",
    TYPE_DELETE_POOL,   "DELETE_POOL",
    TYPE_SCRATCH,       "scratch",
    TYPE_POOL,          "pool",
    TYPE_MOUNT_SCRATCH, "mount_scratch",
    TYPE_VOLRANGE,      "volume_range",
    TYPE_CLEAN,         "clean",
    TYPE_LOCK_SERVER,   "lock_server",
    TYPE_SET_CLEAN,     "SET_CLEAN",
    TYPE_SV,            "ACSSV",
    TYPE_MT,            "ACSMT",
    TYPE_IPC_CLEAN,     "ipc_clean",
    TYPE_SET_CAP,       "SET_CAP",
    TYPE_LOCK,          "lock",
    TYPE_CO_CSI,        "CSI co-process",
    TYPE_DISK_FULL,     "disk_full",
    TYPE_CM,            "ACSCM",
    TYPE_SET_OWNER,     "SET_OWNER",
    TYPE_MIXED_MEDIA_INFO,	"mixed_media_info",
    TYPE_MEDIA_TYPE,	"media_type",
    TYPE_SSI,		"SSI",
    TYPE_DB_SERVER,	"DB_SERVER",	/* Library Station Internal */
    TYPE_DRIVE_GROUP,   "DRIVE_GROUP",  /* Library Station */
    TYPE_SUBPOOL_NAME,  "SUBPOOL_NAME", /* Library Station */
    TYPE_MOUNT_PINFO,   "MOUNT_PINFO",  /* Library Station */
    TYPE_VTDID,         "VTDID",  /* Library Station */
    TYPE_MGMT_CLAS,     "MGMT_CLAS",  /* Library Station */
    TYPE_JOB_NAME,      "JOB_NAME",  /* Library Station */
    TYPE_STEP_NAME,     "STEP_NAME",  /* Library Station */
    TYPE_MOUNT_SCRATCH_PINFO, "MOUNT_SCRATCH_PINFO",  /* Library Station */
    TYPE_CONFIG,        "CONFIG",
    TYPE_LMU,           "LMU",
    TYPE_SWITCH,        "SWITCH",
    TYPE_MV,            "ACSMV",
    TYPE_ERRV,          "ERRV",
    TYPE_FIN,           "command process exiting", /* Issue 647307 */
    TYPE_CR,            "ACSCR",
    TYPE_MVD,           "DEL_VOL",
    TYPE_MISSING,       "missing",
    TYPE_ERRANT,        "errant",                 /* ACSCR - clean up errant volumes */
    TYPE_SURR,          "ACSSURR",                /* ACSSURR - "epicenter" surrogate */
    TYPE_HAND,          "HAND",                   /* Hand identifier */
    TYPE_GETTYPES,      "GETTYPES",
    TYPE_PTP,           "PTP",
    TYPE_DISP,          "ACSDISP",
    TYPE_CLMON,         "ACSMON",
    TYPE_DISPLAY,       "DISPLAY",
    TYPE_ERROR,         "ERROR",
    TYPE_MON,           "ACSMON",
    TYPE_LAST,          "code %d"
};

static  char   *self = "cl_type";
static  char unknown[32];               /* unknown type buffer */

/*
 *      Procedure Type Declarations:
 */


char *
cl_type (
    TYPE type                   /* type code to convert */
)
{
    int     i;


#ifdef DEBUG
    if TRACE(0) {
        cl_trace("cl_type", 1,          /* routine name and parameter count */
                 (unsigned long)type);
    }
#endif /* DEBUG */
 
    /* verify table contents */
    if ((int)TYPE_LAST != (sizeof(type_table) / sizeof(struct type_map))) {
        MLOGDEBUG(0,(MMSG(159,"%s: Contents of table %s are incorrect"), self,  "type_table"));
    }


    /* Look for the type parameter in the table. */
    for (i = 0; type_table[i].type_value != TYPE_LAST; i++) {
        if (type == type_table[i].type_value)
            return type_table[i].type_string;
    }


    /* untranslatable type */
    sprintf(unknown, type_table[i].type_string, type);
    return unknown;
}
