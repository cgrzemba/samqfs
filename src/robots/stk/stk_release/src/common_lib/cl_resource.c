static char P4_Id[]="cl_resource.c";

/*
 * Copyright (2002, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Name:
 *
 *      cl_resource_event
 *
 * Description:
 *
 *      Common module that converts a resource identifier,
 *      to an equivalent character string counterpart.  If an invalid
 *      resource_event_value is specified, the string "UNKNOWN_RESOURCE_EVENT" is returned.
 *
 *      cl_resource_event ensures character string resources won't overrun the
 *      formatting buffer by limiting string sizes to EVENT_SIZE bytes.
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
 *      S. L. Siao              19-Oct-2001     Original.
 *      S. L. Siao              04-Dec-2001     Changed RESOURCE_LMU_NOW_MASTER
 *                                              to RESOURCE_LMU_NEW_MASTER.
 *      S. L. Siao              07-Feb-2002     Changed 
 *                                              RESOURCE_LMU_SWITCHOVER_COMPLETE
 *                                              to 
 *                                              RESOURCE_LMU_RECOVERY_COMPLETE.
 *      S. L. Siao              23-Apr-2002     Added RESOURCE_DRIVE_ADDED and 
 *                                              RESOURCE_DRIVE_REMOVED as per code
 *                                              review. Cleaned up Description.
 *	Mitch Black		24-Jan-2004	Added Dynamic config resource
 *						events.  Changed name of table.
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

/*
 *      Global and Static Variable Declarations:
 */

static struct resource_event_map {
    RESOURCE_EVENT resource_event_value;
    char           *resource_event_string;
    } resource_event_table[] = {
       RESOURCE_ONLINE,                   "RESOURCE_ONLINE",
       RESOURCE_OFFLINE,                  "RESOURCE_OFFLINE",
       RESOURCE_OPERATIVE,                "RESOURCE_OPERATIVE",
       RESOURCE_INOPERATIVE,              "RESOURCE_INOPERATIVE",
       RESOURCE_MAINT_REQUIRED,           "RESOURCE_MAINT_REQUIRED",
       RESOURCE_UNIT_ATTENTION,           "RESOURCE_UNIT_ATTENTION",
       RESOURCE_HARDWARE_ERROR,           "RESOURCE_HARDWARE_ERROR",
       RESOURCE_DEGRADED_MODE,            "RESOURCE_DEGRADED_MODE",
       RESOURCE_SERIAL_NUM_CHG,           "RESOURCE_SERIAL_NUM_CHG",
       RESOURCE_DIAGNOSTIC,               "RESOURCE_DIAGNOSTIC",
       RESOURCE_SERV_CONFIG_MISMATCH,     "RESOURCE_SERV_CONFIG_MISMATCH",
       RESOURCE_SERV_CONFIG_CHANGE,       "RESOURCE_SERV_CONFIG_CHANGE",
       RESOURCE_SERV_START,               "RESOURCE_SERV_START",
       RESOURCE_SERV_IDLE,                "RESOURCE_SERV_IDLE",
       RESOURCE_SERV_IDLE_PENDING,        "RESOURCE_SERV_IDLE_PENDING",
       RESOURCE_SERV_FAILURE,             "RESOURCE_SERV_FAILURE",
       RESOURCE_SERV_LOG_FAILED,          "RESOURCE_SERV_LOG_FAILED",
       RESOURCE_SERV_LOG_FILLED,          "RESOURCE_SERV_LOG_FILLED",
       RESOURCE_LMU_NEW_MASTER,           "RESOURCE_LMU_NEW_MASTER",
       RESOURCE_LMU_STBY_COMM,            "RESOURCE_LMU_STBY_COMM",
       RESOURCE_LMU_STBY_NOT_COMM,        "RESOURCE_LMU_STBY_NOT_COMM",
       RESOURCE_LMU_RECOVERY_COMPLETE,    "RESOURCE_LMU_RECOVERY_COMPLETE",
       RESOURCE_CAP_DOOR_OPEN,            "RESOURCE_CAP_DOOR_OPEN",
       RESOURCE_CAP_DOOR_CLOSED,          "RESOURCE_CAP_DOOR_CLOSED",
       RESOURCE_CARTRIDGES_IN_CAP,        "RESOURCE_CARTRIDGES_IN_CAP",
       RESOURCE_CAP_ENTER_START,          "RESOURCE_CAP_ENTER_START",
       RESOURCE_CAP_ENTER_END,            "RESOURCE_CAP_ENTER_END",
       RESOURCE_CAP_REMOVE_CARTRIDGES,    "RESOURCE_CAP_REMOVE_CARTRIDGES",
       RESOURCE_NO_CAP_AVAILABLE,         "RESOURCE_NO_CAP_AVAILABLE",
       RESOURCE_CAP_INSERT_MAGAZINES,     "RESOURCE_CAP_INSERT_MAGAZINES",
       RESOURCE_CAP_INPUT_CARTRIDGES,     "RESOURCE_CAP_INPUT_CARTRIDGES",
       RESOURCE_LSM_DOOR_OPENED,          "RESOURCE_LSM_DOOR_OPENED",
       RESOURCE_LSM_DOOR_CLOSED,          "RESOURCE_LSM_DOOR_CLOSED",
       RESOURCE_LSM_RECOVERY_INCOMPLETE,  "RESOURCE_LSM_RECOVERY_INCOMPLETE",
       RESOURCE_LSM_TYPE_CHG,             "RESOURCE_LSM_TYPE_CHG",
       RESOURCE_LSM_ADDED,                "RESOURCE_LSM_ADDED",
       RESOURCE_LSM_CONFIG_CHANGE,        "RESOURCE_LSM_CONFIG_CHANGE",
       RESOURCE_LSM_REMOVED,              "RESOURCE_LSM_REMOVED",
       RESOURCE_DRIVE_CLEAN_REQUEST,      "RESOURCE_DRIVE_CLEAN_REQUEST",
       RESOURCE_DRIVE_CLEANED,            "RESOURCE_DRIVE_CLEANED",
       RESOURCE_DRIVE_TYPE_CHG,           "RESOURCE_DRIVE_TYPE_CHG",
       RESOURCE_DRIVE_ADDED,              "RESOURCE_DRIVE_ADDED",
       RESOURCE_DRIVE_REMOVED,            "RESOURCE_DRIVE_REMOVED",
       RESOURCE_POOL_HIGHWATER,           "RESOURCE_POOL_HIGHWATER",
       RESOURCE_POOL_LOWWATER,            "RESOURCE_POOL_LOWWATER",
       RESOURCE_ACS_ADDED,                "RESOURCE_ACS_ADDED",
       RESOURCE_ACS_CONFIG_CHANGE,        "RESOURCE_ACS_CONFIG_CHANGE",
       RESOURCE_ACS_REMOVED,              "RESOURCE_ACS_REMOVED",
       RESOURCE_ACS_PORTS_CHANGE,         "RESOURCE_ACS_PORTS_CHANGE",
       RESOURCE_CAP_ADDED,                "RESOURCE_CAP_ADDED",
       RESOURCE_CAP_CONFIG_CHANGE,        "RESOURCE_CAP_CONFIG_CHANGE",
       RESOURCE_CAP_REMOVED,              "RESOURCE_CAP_REMOVED"
   };

   static char unknown[32];
/*
 *      Procedure Type Declarations:
 */


char *cl_resource_event( RESOURCE_EVENT resource_event_value)
{
    int i;

#ifdef DEBUG
    if TRACE(0)
        cl_trace("cl_resource", 1,    /* routine name and parameter count */
                 (RESOURCE_EVENT) resource_event_value);
#endif /* DEBUG */
    
    for (i = 0; i < (sizeof(resource_event_table)/sizeof(struct resource_event_map)); i++)
	if (resource_event_value == resource_event_table[i].resource_event_value) break;

    /* did we find it? */
    if (i >= (sizeof(resource_event_table)/sizeof(struct resource_event_map))) {
	sprintf( unknown, "UNKNOWN_RESOURCE_EVENT (%d)", resource_event_value);
	return(unknown);
    }

    return(resource_event_table[i].resource_event_string);

}
