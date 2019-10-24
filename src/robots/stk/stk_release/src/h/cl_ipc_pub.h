/* SccsId @(#)cl_ipc_pub.h	1.2 1/11/94  */
#ifndef _CL_IPC_PUB_
#define _CL_IPC_PUB_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *   This file contains the external interface for the IPC functionality.
 *
 * Modified by:
 *
 *   Alec Sharp		04-Feb-1993	Original.
 */

/* ----------- Header Files -------------------------------------------- */

#ifndef _DB_DEFS_
#include "db_defs.h"
#endif


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

/* ----------- Global and Static Variable Declarations ----------------- */

/* ----------  Procedure Declarations ---------------------------------- */

STATUS cl_ipc_create(char *sock_name);
STATUS cl_ipc_destroy(void);
int    cl_ipc_open(char *sock_name_in, char *sock_name_out);
STATUS cl_ipc_read(char buffer[], int *byte_count);
STATUS cl_ipc_send(char *sock_name, void *buffer, int byte_count,
		   int retry_count);
STATUS cl_ipc_write(char *sock_name, void *buffer, int byte_count);
STATUS cl_ipc_xmit(char *sock_name, void *buffer, int byte_count);

#endif /* _CL_IPC_PUB_ */

