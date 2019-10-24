/***SOURCE FILE PROLOGUE**********************************************/
/**                                                                  */
/** Copyright (c) 2011, 2011, Oracle and/or its affiliates.          */
/** All rights reserved.                                             */
/**                                                                  */
/** File Name:      srvvars.c                                        */
/** Description:    The srvcommon global variable set service.       */
/**                                                                  */
/** Special Considerations:                                          */
/**                 Do not attempt to use any TRMSG or TRMEM         */
/**                 facilities from this module as it will cause     */
/**                 a recursive loop.                                */
/**                                                                  */
/** Change History:                                                  */
/**==================================================================*/
/** ISSUE(RELEASE) PROGRAMMER      DATE      ROLLUP INFORMATION      */
/**     DESCRIPTION                                                  */
/**==================================================================*/
/** ELS720         Joseph Nofi     09/15/11                          */
/**     Created for CDK to add XAPI support.                         */
/**                                                                  */
/***END PROLOGUE******************************************************/

/*********************************************************************/
/* Includes:                                                         */
/*********************************************************************/
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "api/defs_api.h"
#include "srvcommon.h"


/*********************************************************************/
/* Global variables:                                                 */
/*********************************************************************/
char  global_cdktrace_flag;
char  global_cdklog_flag;
char  global_srvtrcs_active_flag; 
char  global_srvlogs_active_flag; 
char  global_module_type[8];


/** FUNCTION PROLOGUE ************************************************/
/**                                                                  */
/** Function Name: srvvars                                           */
/** Description:   The srvcommon global variable set service.        */
/**                                                                  */
/** END PROLOGUE *****************************************************/
#undef SELF
#define SELF "srvvars"

extern void srvvars(char *moduleType)
{
#define MAX_PID_SIZE    5
                                        
    int                 i; 
    char                cdktraceValue[MAX_CDKTRACE_TYPE + 1]; 
    char                cdklogValue[MAX_CDKLOG_TYPE + 1]; 
 
    /*****************************************************************/
    /* Set or reset the global_module_type global variable (which    */
    /* is the process type; "SSI", "CSI", "XAPI", "HTTP", etc.).     */
    /* This is also the set to the unix environment variable,        */
    /* CDKTYPE.                                                      */
    /*****************************************************************/
    SETENV(CDKTYPE_VAR, moduleType);
    memset(global_module_type, 0, sizeof(global_module_type));
    memset(global_module_type, ' ', MAX_MODULE_TYPE_SIZE);

    if (strlen(moduleType) > MAX_MODULE_TYPE_SIZE)
    {
        memcpy(global_module_type, 
               moduleType, 
               MAX_MODULE_TYPE_SIZE);
    }
    else
    {
        memcpy(global_module_type, 
               moduleType,
               strlen(moduleType));
    }

    /*****************************************************************/
    /* Set the fast-pass global_cdktrace_flag if the CDKTRACE        */
    /* variable exists, and at least 1 position is set to "1".       */
    /*****************************************************************/
    global_cdktrace_flag = FALSE; 

    if (getenv(CDKTRACE_VAR) != NULL)
    {
        memset(cdktraceValue, 0, sizeof(cdktraceValue)); 
        strncpy(cdktraceValue, getenv(CDKTRACE_VAR), MAX_CDKTRACE_TYPE); 

        for (i = 0; i < MAX_CDKTRACE_TYPE; i++)
        {
            if (cdktraceValue[i] == '1')
            {
                global_cdktrace_flag = 1;
                
                break; 
            }
        } 
    }

    /*****************************************************************/
    /* Set the fast-pass global_cdklog_flag if the CDKLOG            */
    /* variable exists, and at least 1 position is set to "1".       */
    /*****************************************************************/
    global_cdklog_flag = FALSE; 

    if (getenv(CDKLOG_VAR) != NULL)
    {
        memset(cdklogValue, 0, sizeof(cdklogValue)); 
        strncpy(cdklogValue, getenv(CDKLOG_VAR), MAX_CDKLOG_TYPE); 

        for (i = 0; i < MAX_CDKLOG_TYPE; i++)
        {
            if (cdklogValue[i] == '1')
            {
                global_cdklog_flag = 1; 

                break;
            }
        } 
    }

    /*****************************************************************/
    /* At this point, we just assume that the srvtrcs and srvlogs    */
    /* services are active; this means that srvtrcc and srvlogc      */
    /* will attempt to write to their respective pipes (STRCPIPE     */
    /* and SLOGPIPE) which is faster than writing to their           */
    /* output files directly (STRCFILE and SLOGFILE).                */
    /*                                                               */
    /* If srvtrcc or srvlogc detect errors when writing to their     */
    /* respective pipes, then they will reset their respective       */
    /* global_srvtrcs_active_flag or global_srclogs_active_flag      */
    /* and subsequent trace or log output will be directed to the    */
    /* output file directly (which is significantly slower).         */
    /*****************************************************************/
    global_srvtrcs_active_flag = TRUE; 
    global_srvlogs_active_flag = TRUE; 

#ifdef DEBUG

    printf("%s: cdktrace_flag=%d, cdklog_flag=%d, srvtrcs_active=%d, srvlogs_active=%d\n",
           SELF,
           global_cdktrace_flag,
           global_cdklog_flag,
           global_srvtrcs_active_flag,
           global_srvlogs_active_flag);

#endif

    return;
}

