/* SccsId @(#)cl_smem_pub.h	1.2 1/11/94  */
#ifndef _CL_SMEM_PUB_
#define _CL_SMEM_PUB_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *   This file contains the external interface definition for the general
 *   purpose functions which handle shared memory and semaphores.
 *
 *   Any application specific functions or data structures should be in
 *   sblk_defs.h
 *
 * Modified by:
 *
 *   Alec Sharp		04-Feb-1993	Original
 *   Alec Sharp		20-Jul-1993	Added prototypes for cl_dshm_ functions.
 *   Alec Sharp		02-Aug-1993	Added prototypes for cl_sem_check and
 * 					cl_smem_check.
 *   Alec Sharp		01-Nov-1993	Added cl_dshm_check.
 */

/* ----------- Header Files -------------------------------------------- */

#ifndef _SBLK_DEFS_
#include "sblk_defs.h"
#endif


/* ----------- Defines, Typedefs and Structure Definitions ------------- */

enum ipc_check {
    IPC_CHECK_KEY,
    IPC_CHECK_ID
};


/* ----------- Global and Static Variable Declarations ----------------- */

/* ----------  Function Prototypes ------------------------------------- */

/* Semaphore functions */
int     cl_sem_attach(int semaphore_key);
int     cl_sem_create(int semaphore_key);
STATUS  cl_sem_check (enum ipc_check type, int semaphore);
BOOLEAN cl_sem_destroy(int semaphore_id);
STATUS  cl_sem_lock(int semaphore_id);
BOOLEAN cl_sem_unlock(int semaphore_id);

/* Dynamic shared memory functions */
STATUS cl_dshm_attach (int i_dshm_key, struct dshm_id *p_dshm_id,
		       char **cppw_shared_ptr, char *cp_name);
STATUS cl_dshm_build (enum dshm_build_flag build_flag,
		      int i_dshm_key, int size,
		      struct dshm_id *p_dshm_id, char **pp_shared_mem,
		      STATUS (*build_func)(char *, void *),
		      void *p_func_param, char *name);
STATUS cl_dshm_check (int i_key, char *cp_name);
STATUS cl_dshm_destroy (struct dshm_id *p_dshm_id);

/* Shared memory functions */
int     cl_smem_attach(int shared_key, int size, char **shared);
STATUS  cl_smem_check (enum ipc_check type, int shared_mem);
int     cl_smem_create(int shared_key, int total_size, char **shared);
BOOLEAN cl_smem_destroy(int shmid);


#endif /* _CL_SMEM_PUB_ */

