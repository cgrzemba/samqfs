/* SccsId @(#)flags.h	1.2 1/11/94  */
#ifndef _FLAGS_
#define _FLAGS_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      This include file contains special flags that are used when
 *      building the product. For example:
 *
 *      	#defines that should be defined across the entire
 *		product, such as IPC_SHARED.
 *
 *		Information that you temporarily want for debug purposes.
 *
 *      	Platform specific #defines, such as:
 *                 #ifdef BULL
 *                    #define ABC 3
 *                 #endif
 *
 *      This file should be the VERY FIRST include file in the source
 *      files, before both the system and project files.
 *
 * Modified by:
 *
 *      Alec Sharp          27-Apr-1993	    Original
 */

#ifdef NOT_CSC
#define IPC_SHARED 
#endif /* NOT_CSC */


#endif /* _FLAGS_ */

