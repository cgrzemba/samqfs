/* SccsId @(#)inclds.h	2.0 1/11/94  */
#ifndef _INCLUDES_
#define _INCLUDES_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *     This file enumerates all of the common header files shared with the
 *     ACSLS product.

 *     This file is a TOOLKIT machine dependent header file. It consists of 
 *     two sets of #include statements. One set is used by the "makedepend"
 *     tool, which is peculiar to "make" , a UNIX tool which maintains,
 *     updates, and regenerates libraries and executables. This set has
 *     real file names, the file names that are recognized by the all of
 *     the ACSLS product. The other set is used by whatever compilers are 
 *     used to build object code. This set has names that are aliases for
 *     the header file names, which are defined in acssys.h. The MAKEDEPEND
 *     keyword is the switch that toggles the visability of the sets of
 *     include files. This keyword is defined in the makefiles that are
 *     supplied with the Toolkit.
 *
 *     Both sets of include statements enumerate all of the common header
 *     files shared with ACSLS.
 *
 * Considerations:
 *     If this file is used to build Toolkit executables on computers with
 *     file name length limitations, then it will need to be modified.
 *
 * Modified by:
 *
 *      Ken Stickney        04-Nov-1993    Original 
 */

/*
 *      Header Files:
 */

 /* Here is the ordering of the include statements... keep it! */
 
/*
 * The macro defines for the header files are found in acssys.h.
 * This is needed so that the header file names can be in 6.3 format
 * on the AS400.
*/
 
#ifndef MAKEDEPEND
#include _DB_DEFS_API_HEADER_    /* public definitions */
#include _DEFS_API_HEADER_       /* public definitions */
#include _IDENT_API_HEADER_      /* public identifiers */
#include _IPC_HDR_API_HEADER_    /* appropriate ipc_header structure */
#include _STRUCTS_API_HEADER_    /* public structures */
#include _LMSTRUCTS_API_HEADER_  /* public structures */
#else
/*
 * This is the same set of includes as above, but with the real names,
 * so that makedepend can use them to build up dependency lists.
*/
#include "api/db_defs_api.h"          /* public definitions */
#include "api/defs_api.h"             /* public definitions */
#include "api/ident_api.h"            /* public identifiers */
#include "api/ipc_hdr_api.h"      /* appropriate ipc_header structure */
#include "api/structs_api.h"          /* public structures */
#include "api/lm_structs_api.h"       /* public structures */
#endif

#endif /* #ifndef _INCLUDES_ */

