/*  SccsId      @(#)cl_mm_pri.h	5.3 12/2/93  */
#ifndef _CL_MM_PRI_
#define _CL_MM_PRI_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Private Mixed Media Config File Common Library definitions.   
 *
 * Modified by:
 *
 *      A. W. Steere        17-Jul-93           Original.
 *      A. W. Steere        09-Nov-93           moved MM_UNK_*_TYPE_NAME to
 *						cl_mm_pub.h, include cl_mm_pub.h
 *	J. Borzuchowski	    24-Nov-93		R5.0 Mixed Media-- Moved 
 *			   			MM_MAX_MEDIA_TYPES and 
 *						MM_MAX_DRIVE_TYPES to defs.h.
 */

/*
 *      Header Files:
 */

#ifndef _CL_MM_PUB_
#include "cl_mm_pub.h" 
#endif


/*
 *      Defines, Typedefs and Structure Definitions:
 */

/*
 *      Global Variable Declarations:
 */

/* 
 * Location of the ACSLS internal mixed media configuration files, (no user
 * changes to the contents of the files in the directory).  
 */
#define MM_INTERNAL_FILES		"data/internal/mixed_media"

/* Names of the internal files */
#define	MEDIA_TYPES_FILE		"media_types.dat"
#define	MEDIA_COMPAT_FILE		"media_compatibility.dat"
#define	MEDIA_CLEAN_FILE		"media_cleaning.dat"
#define	DRIVE_TYPES_FILE		"drive_types.dat"

/* Location for the user changeable mixed media configuration files */
#define MM_EXTERNAL_FILES		"data/external/mixed_media"

/* Names of the external files */
#define	SCRATCH_PREFERENCES_FILE	"scratch_preferences.dat"

/* Globals to allow, realloc scheme similar to cl_pnl.c */
extern int Mm_max_media_types;
extern int Mm_max_drive_types;

/* Pointers to the malloc'd media and drive type information */ 
extern MEDIA_TYPE_INFO *Mm_media_info_ptr;
extern DRIVE_TYPE_INFO *Mm_drive_info_ptr;

/* define strings for cl_message */
#define MM_DRIVE_TYPE		"drive type"
#define MM_LIB_DRIVE_TYPE	"library drive type"
#define MM_DRIVE_TYPE_NAME	"drive type name"

#define MM_MEDIA_TYPE		"media type"
#define MM_LIB_MEDIA_TYPE	"library media type"
#define MM_MEDIA_TYPE_NAME	"media type name"
#define MM_CLEAN_CART		"cleaning cartridge type"
#define MM_MAX_USE		"max usage"


/* define a common structure, used by cl_mm_clean.c and ut_mm_test.c */
typedef struct  {
	CLN_CART_CAPABILITY     val;
	char *name;
} MM_CLN_CAPAB;

extern MM_CLN_CAPAB mm_cln_capab[];	/* located in cl_defs.c */

/*
 *      Procedure Type Declarations:
 */
extern STATUS cl_mm_init( void );
extern STATUS cl_mm_media_types( void );
extern STATUS cl_mm_drive_types( void );
extern STATUS cl_mm_compat( void );
extern STATUS cl_mm_clean( void );
extern STATUS cl_mm_scr_pref( void );

#endif /* _CL_MM_PRI_ */
