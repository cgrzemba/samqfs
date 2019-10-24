/* SccsId @(#)dv_pub.h	1.2 1/11/94 (c) 1992-1994 STK */
#ifndef _DV_PUB_
#define _DV_PUB_
/*
 * Copyright (C) 1993,2010, Oracle and/or its affiliates. All rights reserved.
 *
 * Functional Description:
 *
 *   This include file contains definitions, prototypes, etc., that
 *   are needed by external functions calling the dynamic variables
 *   library (libdv.a)
 *
 * Modified by:
 *
 *   Alec Sharp 	11-Jun-1993  Original.
 *   Alec Sharp		04-Nov-1993  Added dv_check
 *   Alec Sharp		01-Dec-1993  Added dv_get_count
 *   Howie Grapek	04-Jan-1993  Toolkit Specifics
 *   Mike Williams      28-May-2010  Moved the end of the NOT_CSC else
 *                                   statement up in the code so that the
 *                                   function prototypes for dv_get_string,
 *                                   dv_get_boolean, dv_get_number are always
 *                                   included.
 */

/* ----------- Header Files -------------------------------------------- */

#ifndef NOT_CSC
#include "dv_api.h"
#else /* NOT_CSC */

#ifndef _DEFS_
#include "defs.h"
#endif /* _DEFS_ */

/* Note: to make it so that external users of the dv_ functions only have
   to include one include file, dv_pub.h, rather than dv_pub.h and dv_tag.h,
   we have gone to great lengths internally to arrange include files so that
   there is not a general inclusion of dv_tag.h. This is because otherwise
   we would have a circular dependency in the dv Makefile. */
   
#ifndef _DV_TAG_
#include "dv_tag.h"
#endif /* _DV_TAG_ */

#ifndef _SBLK_DEFS_
#include "sblk_defs.h"
#endif /* _SBLK_DEFS_ */


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

#define DV_VALUE_LEN  128   /* NOTE: This is also defined in dv_pri.h
			       under the name DVI_VALUE_LEN. If you change
			       one, you MUST change the other. The reason
			       for this is because we had to solve a
			       circular dependency with respect to dv_tag.h
			       and this was part of the solution.
			       
			       Length of string (excluding \0) that
			       contains the value for the tag. When
			       getting a string value, make sure your
			       buffer is at least DV_VALUE_LEN + 1 long */



/* ----------- Global and Static Variable Declarations ----------------- */



/* ----------  Function Prototypes ------------------------------------- */

STATUS 	dv_shm_create (enum dshm_build_flag build_flag, char **cpp_memory);
STATUS 	dv_shm_destroy (void);
int     dv_get_count (void);
STATUS 	dv_get_mask (DV_TAG tag, unsigned long *lpw_mask);
STATUS  dv_check (void);

#endif /* NOT_CSC */
STATUS 	dv_get_number (DV_TAG tag, long *lpw_value);
STATUS 	dv_get_boolean (DV_TAG tag, BOOLEAN *Bpw_bool);
STATUS 	dv_get_string (DV_TAG tag, char *cpw_string);
#endif /* _DV_PUB_ */


