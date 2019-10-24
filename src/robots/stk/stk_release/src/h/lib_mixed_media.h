/* SccsId @(#)lib_mixed_media.h	1.2 1/11/94  */
#ifndef _LIB_MIXED_MEDIA_
#define _LIB_MIXED_MEDIA_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Header file for information that is defined by the LMU Interface
 *	but is also needed by the routines that use the mixed media common lib 
 *	routines.  The standard is for everything defined by the LMU to be
 *	in lmu_defs.h which is local to the acslh.  lib_mixed_media.h should
 *	be included by lmu_defs.h and cl_mm_pub.h.
 *     
 *
 * Modified by:
 *
 *      A. W. Steere        17-Jul-1993    Original.
 */

/*
 *      Defines, Typedefs and Structure Definitions:
 */

/* length of the library drive or media type returned from the LMU */
#define LIB_MEDIA_TYPE_LEN		1	/* alphanumeric field */
#define LIB_DRIVE_TYPE_LEN		2	/* numeric field */

/* 
 * library media and drive type field - create name structure that contains 
 * space for the null terminator.  
 */
typedef char LIB_MEDIA_TYPE[LIB_MEDIA_TYPE_LEN + 1];
typedef char LIB_DRIVE_TYPE[LIB_DRIVE_TYPE_LEN + 1];

/* 
 * Drive type 0 (zero) indicates that the LSM is not configured, drive is
 * not installed or the drive is not communicating.
 */
#define MM_UNK_LIB_DRIVE_TYPE		0

/* 
 * Valid range of drive types is LIB_DTYPE_MIN - LIB_DTYPE_MAX.  
 * The field is two digits as it is a numeric field.
 */
#define LIB_DTYPE_MIN		01
#define LIB_DTYPE_MAX		99

#endif /* _LIB_MIXED_MEDIA_ */

