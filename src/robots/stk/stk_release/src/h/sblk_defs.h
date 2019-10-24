/* SccsId @(#)sblk_defs.h	1.2 1/11/94  */
#ifndef _SBLK_DEFS_
#define _SBLK_DEFS_
/*
 * Copyright (1990, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Defines the functions and defintions used by the shared memory
 *      block routines in the common_lib.
 *      
 * 
 * Modified by:
 *
 *      D. L. Trachy            20-Jun-1990     Original.
 *
 *      D. A. Beidle            26-Nov-1991.    Increased BLK_NUM_LARGE to 32.
 *          Declare small_preamble structure as an external in header file per
 *          coding guidelines.  Upgrade code to standards.
 *      Alec Sharp              03-Jun-1992.    Added shared memory structures
 *          and defines for use with the cl_last_ functions.
 *      Alec Sharp              03-Oct-1992     Added key for access control
 *          shared memory. 
 *      Alec Sharp	  	13-Apr-1993	R4.0 BR#442, F24678
 *	    Don't keep the pointers to the shared memory blocks in the
 *          SMEM_LIB shared memory block. Keep the pointers in a local
 *          structure, SMEM_LIB_PTRS.
 *      Alec Sharp		19-Jul-1993     R5.0.  Added Dynamic Shared
 *          memory structures and enumerations. Added keys for Access Control
 *          client list shared memory blocks.
 */

/*
 *      Header Files:
 */

#include <time.h>

#ifndef _DEFS_
#include "defs.h"
#endif

/* --------------- IPC Shared Memory things -------------------------*/

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/* ---------- Shared Memory Keys ---------------------------
 * NOTE: ACSLS Development has reserved the shared memory keys in the range
 * 50,000 - 59,999 for ACSLS product use.
 *
 * NOTE: This is the old way of doing shared memory keys. The modern way
 * is to use the ftok function (see dv_ functions for an example).
 */

/* Key numbers used for shared memory and semaphore */
#define SHARED_KEY      ((key_t)50000)

/*
 * Keys for last_ shared memory blocks.
 * If the following environment variable exists and is numeric, its value
 * will be used as the key for the library shared memory.
 * If the environment variable does not exist, or the value is not numeric or
 * is less than or equal to zero, we will use the #define value.
 * Whichever way we go, the value will be successively incremented for
 * the other shared memory blocks.
 */

#define SMEM_LIB_ENV_KEY "ACSLS_LIB_SHM_KEY"

#define SMEM_LIB_KEY     ((key_t) 51000)    /* Also use 51001, 51002, 51003 */

/* Key for Access Control shared memory */

#define SMEM_AC_KEY      ((key_t) 52000)
#define SMEM_AC_RPC_KEY  ((key_t) 52001)
#define SMEM_AC_ADI_KEY  ((key_t) 52002)
#define SMEM_AC_LU62_KEY ((key_t) 52003)

/* ------------- End Shared Memory Keys ------------------------- */



/* Possible conditions of a shared block */
typedef enum {
        BLOCK_WRITTEN,
        BLOCK_AVAILABLE
} USAGE_STATUS;

/* preamble in front of each shared block */
struct block_preamble {
        int                     block_size;
        USAGE_STATUS            usage_status;
        int                     pid;
        time_t                  access_time;
        int                     process_count;
} ;


/*  two sizes of messages supported and the number of blocks 
    alloicated for that size */
#define         BLK_SIZE_SMALL                  512
#define         BLK_NUM_SMALL                   128  
#define         BLK_SIZE_LARGE                  4096
#define         BLK_NUM_LARGE                   32  

/* number of small messages any one process can use */
#define         BLK_SMALL_PER                   (BLK_NUM_SMALL/2)

/* number of large messages one process can use */
#define         BLK_LARGE_PER                   (BLK_NUM_LARGE/2)

/* small block defintion, preamble followed by data */
struct sh_small_block {
        struct block_preamble   bp;
        char   data [BLK_SIZE_SMALL]; 
} ;

/* large block defintion, preamble followed by data */
struct sh_large_block {
        struct block_preamble   bp;
        char   data [BLK_SIZE_LARGE]; 
} ;

/* total shared memory needed */
#define         BLK_TOT_MEM_USED   ((sizeof(struct sh_large_block) * BLK_NUM_LARGE) + (sizeof(struct sh_small_block) * BLK_NUM_SMALL))

/* time to wait before checking for dead blocks */
#define         BLK_CLEANUP_WAIT    (600)       

/* cleanup time before message is considered dead */
#define         BLK_CLEANUP_TIME    ((time_t)(1800))    


typedef struct sh_large_block   CL_SHM_LARGE;
typedef struct block_preamble   CL_SHM_PREAMBLE;
typedef struct sh_small_block   CL_SHM_SMALL;


/*
 *      Global and Static Variable Declarations:
 */
extern  CL_SHM_LARGE   *large_preamble; /* ptr to 1st large block preamble  */
extern  int             semaphore_id;   /* semaphore ID                     */
extern  int             shared_id;      /* shared memory segment ID         */
extern  CL_SHM_SMALL   *small_preamble; /* ptr to 1st small block preamble  */


/* ----------------- Dynamic Shared Memory things ------------------------ */

/* Dynamic shared memory consists of two parts - shared memory and a
   semaphore. The shared memory should have a common structure as its
   first part. Upon checking that structure, accessing functions determine
   whether the shared memory is out of date and they need to reattach.

   The functions that manipulate shared memory are:
      cl_dshm_build   - create shared memory and semaphore and fill
                        the shared memory
      cl_dshm_attach  - attach to shared memory if okay to do so
      cl_dshm_destroy - destroy shared memory and semaphore

   All dynamic shared memory blocks are required to have the following
   header. I.e., most of the shared memory can be as they want it, but
   it must start with the following structure. */

struct dshm_hdr {
    BOOLEAN reattach;     /* Does the process need to reattach?  */
    BOOLEAN built;        /* Has the data been rebuilt?          */
    time_t  timestamp;
};


/* This structure keeps information about the IDs of the dynamic shared
   memory and the semaphore. The application using dynamic shared memory
   must declare one of these structures.  */

struct dshm_id {
    int     semaphore;	  /* Semaphore ID     */
    int     shared_mem;   /* Shared memory ID */
};


/* This enumeration tells whether we are creating the shared memory for
   the first time, or rebuilding it. The difference is that first time
   creation must also create the semaphore, while rebuilding must leave
   the existing semaphore intact. */

enum dshm_build_flag {
    DSHM_FIRST_TIME,
    DSHM_REBUILD
};




/* ------------------- Last Numbers Shared Memory things -----------------*/


/* ------------------ #defines ----------------*/

/* For the shared memory keys, see the section above. */

/*
 * Value for unset memory. If the value in shared memory is equal to
 * this value, we go to the database for the number, then update
 * the shared memory with the value from the database.
 */

#define SMEM_UNSET INT_MAX          /* Must be appropriate for the typedef
			             * of COUNT_TYPE, see below.           */
#define SMEM_NOCOUNT (INT_MAX - 1)  /* Used with panels that have no rows,
				     * column, or drives.                  */


/*----------------- typedefs -----------------------
 * If you change these, bear in mind that the Sun compiler starts int's
 * on 4 byte boundaries so you may not be saving the space you think
 * you are saving.
 */

typedef int  COUNT_TYPE;
typedef int  INDEX_TYPE;


/* ------------ Shared memory structures ----------
 *
 *
 *   The blocks are laid out as follows.
 *
 *	
 *	Key = SMEM_LIB_KEY
 *
 *	This is a single structure which contains information about the
 *	library.  It currently contains the number of ACSs in the library.
 *	
 *	Key = SMEM_ACS_KEY
 *
 *	This is an array of structs, one element for each ACS in the
 *	library.  The element contains the number of LSMs for that
 *	particular ACS, and the index where the LSMs for this ACS start
 *	in the LSM block.
 *
 *	Key = SMEM_LSM_KEY
 *
 *	This is an array of structs, one element for each LSM.  The
 *	element contains the number of CAPs and panels for that particular
 *      LSM, and the index where the panels for this LSM start in the
 *	panel block.  It is organized as such (for example, in the case
 *      of ACS 0 with 3 LSMs and ACS 1 with 2 LSMs):
 *
 *	   Element 0      ACS 0    LSM 0
 *	   Element 1      ACS 0    LSM 1
 *	   Element 2      ACS 0    LSM 2
 *	   Element 3      ACS 1    LSM 0
 *	   Element 4      ACS 1    LSM 1
 *	      etc...
 *
 *	Key = SMEM_PNL_KEY
 *
 *	This is an array of structures, one element for each panel.
 *	Each element contains the counts of rows, columns, and drives.
 *	It is organized as such (for example, in the case of one ACS
 *      with 2 LSMs, both of which have 3 panels):
 *
 *	   Element 0      ACS 0    LSM 0    Panel 0
 *	   Element 1      ACS 0    LSM 0    Panel 1
 *	   Element 2      ACS 0    LSM 0    Panel 2
 *	   Element 3      ACS 0    LSM 1    Panel 0
 *	   Element 4      ACS 0    LSM 1    Panel 1
 *	   Element 5      ACS 0    LSM 1    Panel 2
 *	      etc...
 *
 *   ---------------------------------------------------------
 *
 *   The actual last, or maximum value for CAPs in an LSM, and rows,
 *   columns, and drives in a panel, are filled in on a demand basis.
 *   This routine initializes them to "SMEM_UNSET", and when the cl_
 *   routines ask for the last value, if the value is SMEM_UNSET, the
 *   cl_routine will get the value from the database, and fill it the
 *   shared memory block with this value.
 */


/*
 * The ACS shared memory contains an array of SMEM_ACS structures,
 * one element for each ACS in the library.  The index field, lsm_index,
 * is the index into the LSM array where the information for this
 * ACS starts.
 */

typedef struct smem_acs {
    COUNT_TYPE   lsm_count;
    INDEX_TYPE   lsm_index;
} SMEM_ACS;


/*
 * The LSM shared memory contains an array of SMEM_LSM structures,
 * one element for each LSM in the library.  The index field, pnl_index,
 * is the index into the panel array where the information for this
 * LSM starts.
 */

typedef struct smem_lsm {
    COUNT_TYPE   cap_count;
    COUNT_TYPE   pnl_count;
    INDEX_TYPE   pnl_index;
} SMEM_LSM;


/*
 * The PNL shared memory contains an array of SMEM_PNL structures,
 * one element for each panel in the library.
 */

typedef struct smem_pnl {
    COUNT_TYPE   row_count;
    COUNT_TYPE   col_count;
    COUNT_TYPE   drv_count;
} SMEM_PNL;


/*
 * The library shared memory contains the number of ACSs in the library.
 */

typedef struct smem_lib {
    COUNT_TYPE   acs_count;
} SMEM_LIB;

/*
   This structure is not a shared memory structure. It is a structure
   used by functions which access the above shared memory structures,
   and contains a pointer to each of the shared memory structures.
 */

typedef struct smem_lib_ptrs {
    SMEM_LIB    *p_lib;
    SMEM_ACS    *p_acs;
    SMEM_LSM    *p_lsm;
    SMEM_PNL    *p_pnl;
} SMEM_LIB_PTRS;


/* ------------- Function prototypes --------------------- */

BOOLEAN cl_sblk_attach(void);
BOOLEAN cl_sblk_available(void);
int cl_sblk_cleanup(void);
BOOLEAN cl_sblk_create(void);
BOOLEAN cl_sblk_destroy(void);
BOOLEAN cl_sblk_read(char *packet, int message_number, int *message_size);
BOOLEAN cl_sblk_remove(int message_number);
int cl_sblk_write(void *message, int message_count, int process_count);


#endif /* _SBLK_DEFS_ */

