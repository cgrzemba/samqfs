/* SccsId @(#)sblk_pub.h	1.2 1/11/94  */
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
 */

/*
 *      Header Files:
 */

#include <time.h>

#ifndef _DEFS_
#include "defs.h"
#endif


typedef int  COUNT_TYPE;

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

