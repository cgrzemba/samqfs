/**********************************************************************
*
*	C Source:		dv_api.c
*	Subsystem:		1
*	Description:	
*	%created_by:	kjs %
*	%date_created:	Tue Dec 20 10:57:48 1994 %
*
**********************************************************************/
#ifndef lint
static char *_csrc = "@(#) %filespec: dv_api.c,2.1.2 %  (%full_filespec: 1,csrc,dv_api.c,2.1.2 %)";
static char SccsId[]= "@(#) %filespec: dv_api.c,2.1.2 %  (%full_filespec: 1,csrc,dv_api.c,2.1.2 %)";
#endif
/*
 * Copyright (1989, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 *
 * Description:
 *
 *   This file contains functions to get values from the DV shared memory.
 *   Overloaded for the client applications to only get stuff from the env.
 *
 *	dv_get_boolean
 *	dv_get_mask
 *	dv_get_number
 *	dv_get_string
 *
 * Revision History:
 *
 *   Alec Sharp 	13-Aug-1993  Original. Consolidated from 6 other
 * 		.c files.
 *   Alec Sharp         11-Oct-1993  Added information in error messages,
 * 		and restructured calls to cl_log_event to be varargs calls.
 *   Alec Sharp		01-Dec-1993  Only report that environment variables
 *              are being used if environment variable DV_SHARED_MEMORY is
 *              not set to FALSE. Added dv_get_count.
 *   Alec Sharp         10-Dec-1993  Made into stubs, a mere shadow of their
 *              former selves.
 *   Howie Grapek	04-Jan-1994  More cleanup... no shared memory, etc
 *   Ken Stickney       23-Dec-1994  Changes for Solaris Port.
 *
 */


/* ----------- Header Files -------------------------------------------- */

#include "flags.h"
#include "system.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>

#include "cl_pub.h"
#include "dv_pub.h"
#include "ml_pub.h"

/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */

/* 
 * the following is in dv/dvi_getenv_names.c ... 
 * Note: for each toolkit release, make sure this is
 * the most current, and up to date.
 * It HAS to match the values in dv_api.h
 */

char *dv_getenv_names[] = {
	"-PLACE_HOLDER-",
	"EVENT_FILE_NUMBER",
	"LOG_PATH",
	"LOG_SIZE",
	"TIME_FORMAT",
	"UNIFORM_CLEAN_USE",
	"AC_CMD_ACCESS",
	"AC_CMD_DEFAULT",
	"AC_VOL_ACCESS",
	"AC_VOL_DEFAULT",
	"AC_LOG_ACCESS",
	"CSI_CONNECT_AGETIME",
	"CSI_RETRY_TIMEOUT",
	"CSI_RETRY_TRIES",
	"CSI_TCP_RPCSERVICE",
	"CSI_UDP_RPCSERVICE",
	"AUTO_CLEAN",
	"AUTO_START",
	"MAX_ACSMT",
	"MAX_ACS_PROCESSES",
	"TRACE_ENTER",
	"TRACE_VOLUME",
	"ACSLS_MIN_VERSION",
	"DI_TRACE_FILE",
	"DI_TRACE_VALUE",
	"LM_RP_TRAIL",
	"ACSLS_ALLOW_ACSPD",
};

/* ----------  Procedure Declarations ---------------------------------- */

static STATUS st_getenv (int tag, enum dv_shm_type type,
			 union dv_shm_value *up_value);


/*--------------------------------------------------------------------------
 *
 * Name:  dv_get_boolean
 *
 * Description:
 *
 *      This functions gets a boolean value from the DV shared memory.
 *      It is passed a typedef that identifies the value to get.
 *
 * Return Values:
 *
 *	STATUS_SUCCESS          Success
 *	STATUS_INVALID_VALUE	The tag is <= TAG_FIRST or >= TAG_LAST
 *	STATUS_INVALID_TYPE	The field is not a boolean field
 *      STATUS_IPC_FAILURE      Failed to reattach to shared memory
 *      STATUS_MISSING_OPTION   We had to use getenv() because no shared
 *                              memory existed, and the environment variable
 *                              did not exist.
 *      STATUS_PROCESS_FAILURE  Process failure such as inability to do malloc
 *      Other                   Error returned from a called function
 *
 * Parameters:
 *
 *  	DV_TAG   tag		The enumerated tag that we want to look up.
 *	                        The tag name after the initial DV_TAG_ is
 *                              the name of the tag as it appears in the DV
 *                              data file.
 *	BOOLEAN *Bpw_bool	Pointer to the boolean value that we will
 *                              fill in from the shared memory value.
 *
 * Implicit Inputs: 	NONE
 *
 * Implicit Outputs:	NONE
 *
 * Considerations:
 *
 *	We use the tag to index directly into shared memory. We can do
 *	this because the first element of the array corresponds to
 *	DV_TAG_FIRST, which sets up the tag as a direct index.
 */


STATUS dv_get_boolean (DV_TAG tag, BOOLEAN *Bpw_bool)
{
    static char          *self = "dv_get_boolean";
    
    STATUS	          status;
    union dv_shm_value    u_value;
    
    
    /* Check the tag enumeration to make sure it's within bounds */
    if (tag <= DV_TAG_FIRST || tag >= DV_TAG_LAST) {
	MLOG((MMSG(523,"%s: Tag %d is out of range\n"), self, tag));
	return (STATUS_INVALID_VALUE);
    }
    
    status = st_getenv ((int)tag, DVI_SHM_BOOLEAN, &u_value);
    if (status == STATUS_SUCCESS)
	*Bpw_bool = u_value.boolean;
    return (status);
}


/*--------------------------------------------------------------------------
 *
 * Name:  dv_get_number
 *
 * Description:
 *
 *      This functions gets a numeric (long integer) value from the
 *      DV shared memory. It is passed a typedef that identifies the
 *      value to get.
 *
 * Return Values:
 *
 *	STATUS_SUCCESS          Success
 *	STATUS_INVALID_VALUE	The tag is <= TAG_FIRST or >= TAG_LAST
 *	STATUS_INVALID_TYPE	The field is not a numeric field
 *      STATUS_IPC_FAILURE      Failed to reattach to shared memory
 *      STATUS_MISSING_OPTION   We had to use getenv() because no shared
 *                              memory existed, and the environment variable
 *                              did not exist.
 *      STATUS_PROCESS_FAILURE  Process failure such as inability to do malloc
 *      Other                   Error returned from a called function
 *
 * Parameters:
 *
 *  	DV_TAG   tag		The enumerated tag that we want to look up.
 *	                        The tag name after the initial DV_TAG_ is
 *                              the name of the tag as it appears in the DV
 *                              data file.
 *	long     *lpw_value	Pointer to the long value that we will
 *                              fill in from the shared memory value.
 *
 * Implicit Inputs: 	NONE
 *
 * Implicit Outputs:	NONE
 *
 * Considerations:
 *
 *	We use the tag to index directly into shared memory. We can do
 *	this because the first element of the array corresponds to
 *	DV_TAG_FIRST, which sets up the tag as a direct index.
 */


STATUS dv_get_number (DV_TAG tag, long *lpw_value)
{
    static char          *self = "dv_get_number";
    
    STATUS	          status;
    union dv_shm_value    u_value;
    
    /* Check the tag enumeration to make sure it's within bounds */
    if (tag <= DV_TAG_FIRST || tag >= DV_TAG_LAST) {
	MLOG((MMSG(523,"%s: Tag %d is out of range\n"), self, tag));
	return (STATUS_INVALID_VALUE);
    }
    
    status = st_getenv ((int)tag, DVI_SHM_NUMBER, &u_value);
    if (status == STATUS_SUCCESS)
	*lpw_value = u_value.number;
    return (status);
    
}



/*--------------------------------------------------------------------------
 *
 * Name:  dv_get_mask
 *
 * Description:
 *
 *      This functions gets a mask value from the DV shared memory.
 *      It is passed a typedef that identifies the value to get.
 *
 * Return Values:
 *
 *	STATUS_SUCCESS          Success
 *	STATUS_INVALID_VALUE	The tag is <= TAG_FIRST or >= TAG_LAST
 *	STATUS_INVALID_TYPE	The field is not a numeric field
 *      STATUS_IPC_FAILURE      Failed to reattach to shared memory
 *      STATUS_MISSING_OPTION   We had to use getenv() because no shared
 *                              memory existed, and the environment variable
 *                              did not exist.
 *      STATUS_PROCESS_FAILURE  Process failure such as inability to do malloc
 *      Other                   Error returned from a called function
 *
 * Parameters:
 *
 *  	DV_TAG   tag		  The enumerated tag that we want to look up.
 *	                          The tag name after the initial DV_TAG_ is
 *                                the name of the tag as it appears in the DV
 *                                data file.
 *	unsigned long *lpw_value  Pointer to the long value that we will
 *                                fill in from the shared memory value.
 *
 * Implicit Inputs: 	NONE
 *
 * Implicit Outputs:	NONE
 *
 * Considerations:
 *
 *	We use the tag to index directly into shared memory. We can do
 *	this because the first element of the array corresponds to
 *	DV_TAG_FIRST, which sets up the tag as a direct index.
 */


STATUS dv_get_mask (DV_TAG tag, unsigned long *lpw_mask)
{
    static char          *self = "dv_get_mask";
    
    STATUS	          status;
    union dv_shm_value    u_value;
    
    /* Check the tag enumeration to make sure it's within bounds */
    if (tag <= DV_TAG_FIRST || tag >= DV_TAG_LAST) {
	MLOG((MMSG(523,"%s: Tag %d is out of range\n"), self, tag));
	return (STATUS_INVALID_VALUE);
    }
    
    
    status = st_getenv ((int)tag, DVI_SHM_MASK, &u_value);
    if (status == STATUS_SUCCESS)
	*lpw_mask = u_value.mask;
    return (status);
}



/*--------------------------------------------------------------------------
 *
 * Name:  dv_get_string
 *
 * Description:
 *
 *      This functions gets a string value from the DV shared memory.
 *      It is passed a typedef that identifies the value to get.
 *
 * Return Values:
 *
 *	STATUS_SUCCESS          Success
 *	STATUS_INVALID_VALUE	The tag is <= TAG_FIRST or >= TAG_LAST
 *	STATUS_INVALID_TYPE     The field is not a string field
 *      STATUS_IPC_FAILURE      Failed to reattach to shared memory
 *      STATUS_MISSING_OPTION   We had to use getenv() because no shared
 *                              memory existed, and the environment variable
 *                              did not exist.
 *      STATUS_PROCESS_FAILURE  Process failure such as inability to do malloc
 *      Other                   Error returned from a called function
 *
 * Parameters:
 *
 *  	DV_TAG   tag		The enumerated tag that we want to look up.
 *	                        The tag name after the initial DV_TAG_ is
 *                              the name of the tag as it appears in the DV
 *                              data file.
 *	char    *cpw_string	Pointer to the string buffer that we will
 *                              fill in from the shared memory value.
 *
 * Implicit Inputs: 	NONE
 *
 * Implicit Outputs:	NONE
 *
 * Considerations:
 *
 *	We use the tag to index directly into shared memory. We can do
 *	this because the first element of the array corresponds to
 *	DV_TAG_FIRST, which sets up the tag as a direct index.
 */


STATUS dv_get_string (DV_TAG tag, char *cpw_string)
{
    static char          *self = "dv_get_string";
    
    STATUS	          status;
    union dv_shm_value    u_value;
    
    /* Check the tag enumeration to make sure it's within bounds */
    if (tag <= DV_TAG_FIRST || tag >= DV_TAG_LAST) {
	MLOG((MMSG(523,"%s: Tag %d is out of range\n"), self, tag));
	return (STATUS_INVALID_VALUE);
    }
    
    status = st_getenv ((int)tag, DVI_SHM_STRING, &u_value);
    if (status == STATUS_SUCCESS)
	strcpy (cpw_string, u_value.string);
    return (status);
}    


/*--------------------------------------------------------------------------
 *
 * Name:  st_getenv
 *
 * Description:
 *
 *   This function gets an environment variable value for a given
 *   tag.  It is used when there are no dynamic variables in shared
 *   memory. Specifically, it is used when cmd_proc is run on a remote
 *   system.
 *
 *   Rather than do an expensive getenv each time, we cache the data
 *   when we get it, so subsequent calls are fairly quick. We use the
 *   same structure as we use in the shared memory, to avoid having to
 *   define yet another set of structures.
 *
 * Return Values:
 *
 *   STATUS_SUCCESS
 *   STATUS_INVALID_TYPE	The passed in type is either unknown or does
 *				not match what we believe is the type of
 *                              the variable.
 *   STATUS_MISSING_OPTION      The environment variable does not exist.
 *                              Similar to a NULL return from getenv.
 *   STATUS_PROCESS_FAILURE     Couldn't malloc the necessary memory
 *
 * Parameters:
 *
 *   tag		The tag we want to look up. We assume that the
 *                      value has been checked and is within range before
 *                      this function is called. Thus, we do no bounds
 *                      checking here. The reason for this is to make sure
 *                      that we don't have any dependencies on dv_tag.h
 *
 *   type		The type of the data. E.g., boolean, string.
 *
 *   up_value		Pointer to the data we will fill in.
 *
 */

static STATUS st_getenv (int tag, enum dv_shm_type type,
			 union dv_shm_value *up_value)
{
    static char           *self = "dv_get: st_getenv";

    static struct dv_shm  *sp_getenv_values;
    
    int  i_size;
    int  i_length;
    char *cp;
    
    
    /* If this is the first time in, create the in memory structure where
       we will store all the values and initialize it. */
    
    if (sp_getenv_values == NULL) {
	i_size = (int) sizeof (*sp_getenv_values) +
	    ( (int) DV_TAG_LAST * sizeof (sp_getenv_values->variable));
	
	if ((sp_getenv_values = (struct dv_shm *)calloc (i_size, 1)) == NULL) {
	    MLOG((MMSG(526,"%s: Failed to calloc %d bytes"), self, i_size));
	    return (STATUS_PROCESS_FAILURE);
	}
    }
    
    
    if (sp_getenv_values->variable[tag].type == DVI_SHM_UNKNOWN) {
	
	/* This is the first time we've tried to get this value,
	   so we need to do a getenv on it. When we've done that,
	   we can store the value in the structure. Note that the
	   structure is organized so that we can index directly in
	   using the tag value */ 
	
	cp = getenv (dv_getenv_names[tag]);
	if (cp == NULL)
	    return (STATUS_MISSING_OPTION);
	
	switch (type) {
	  case DVI_SHM_MASK:
	    sp_getenv_values->variable[tag].u.mask = strtol (cp, NULL, 0);
	    break;
	  case DVI_SHM_NUMBER:
	    sp_getenv_values->variable[tag].u.number = strtol (cp, NULL, 0);
	    break;
	  case DVI_SHM_BOOLEAN:
	    if (strcmp (cp, "TRUE") == 0)
		sp_getenv_values->variable[tag].u.boolean = TRUE;
	    else
		sp_getenv_values->variable[tag].u.boolean = FALSE;
	    break;
	  case DVI_SHM_STRING:
	    i_length = sizeof (sp_getenv_values->variable[tag].u.string);
	    strncpy (sp_getenv_values->variable[tag].u.string, cp, i_length);
	    sp_getenv_values->variable[tag].u.string[i_length - 1] = '\0';
	    /* Log a message if we had to truncate the string */
	    if (strlen (cp) > (size_t)(i_length - 1)) {
		MLOG((MMSG(527,"%s: Value <%s>\n"
			   "of environment variable <%s> is longer than "
			   "allowed length of %d. Truncating to \n<%s>"),
		      self, cp, dv_getenv_names[tag], i_length - 1,
		      sp_getenv_values->variable[tag].u.string));
	    }
	    break;
	  default:
	    MLOG((MMSG(528,"%s: Invalid type %d"), self, type));
	    return (STATUS_INVALID_TYPE);
	}
	
	/* Fill in the type field of the structure */
	sp_getenv_values->variable[tag].type = type;
    }
    
    
    /* At this point, we should have data in the appropriate element
       of the array of structures. Now we can return the value, after
       checking it is of the right type */
    
    if (type != sp_getenv_values->variable[tag].type) {
	MLOG((MMSG(529,"%s: Type mismatch. Parameter=%d, stored=%d"),
	      self, type, sp_getenv_values->variable[tag].type));
	return (STATUS_INVALID_TYPE);
    }
    
    *up_value = sp_getenv_values->variable[tag].u;
    
    return (STATUS_SUCCESS);
}

#if defined (TESTMAIN)

int main(int argc, char *argv[])
{
    BOOLEAN B_auto_clean;
    STATUS  status;

    status = dv_get_boolean (DV_TAG_AUTO_CLEAN, &B_auto_clean);
    if (status == STATUS_SUCCESS)
	printf ("Auto clean = %s\n", B_auto_clean == TRUE ?
		"TRUE" : "FALSE");
    else
	printf ("Failure %d calling dv_get_boolean\n", status);

    return (0);
}

#endif


