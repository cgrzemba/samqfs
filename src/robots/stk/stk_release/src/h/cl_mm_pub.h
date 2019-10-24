/* SccsId @(#)cl_mm_pub.h	1.2 1/11/94  */
#ifndef _CL_MM_PUB_
#define _CL_MM_PUB_
/*
 * Copyright (1993, 2011) Oracle and/or its affiliates.  All rights reserved.
 *
 * Functional Description:
 *
 *      Public interface into the common lib mixed media routines.
 *      
 *
 * Modified by:
 *
 *      A. W. Steere	    17-Jul-93		Original.
 *	A. W. Steere	    30-Jul-93		R5.0 updated QU_MMI_RESPONSE 
 *	J. Borzuchowski	    10-Aug-93		R5.0 removed QU_MMI_RESPONSE,
 *				QU_MEDIA_TYPE_STATUS, and QU_DRIVE_TYPE_STATUS,
 *				since they are found in header structs.h.  
 *				These structures were included here until
 *				structs.h was updated and returned.
 *				Moved declaration of cl_mm_info to cl_pub.h.
 *				There was a header contention problem.
 *	J. Borzuchowski	    11-Aug-93		R5.0 moved CLN_CART_CAPABILITY,
 *				MM_MAX_COMPAT_TYPES, MEDIA_TYPE_NAME_LEN, and
 *				DRIVE_TYPE_NAME_LEN to defs.h.  Added include
 *				for header defs.h.
 *	A. W. Steere	    09-Nov-93		added MM_UNK_*_TYPE_NAME 
 */

/*
 *      Header Files:
 */

#ifndef _LIB_MIXED_MEDIA_
#include "lib_mixed_media.h"            /* LMU Interface definitions */
#endif

#ifndef _DEFS_
#include "defs.h"
#endif

/*
 *      Defines, Typedefs and Structure Definitions:
 */


/* Define an unknown media type name */
#define MM_UNK_MEDIA_TYPE_NAME          "unknown"

/* Define an unknown drive type name */
#define MM_UNK_DRIVE_TYPE_NAME          "unknown"


/*
 * media and drive type name - create name structure that contains space
 * for the null terminator.
 */
typedef char MEDIA_TYPE_NAME[MEDIA_TYPE_NAME_LEN + 1];
typedef char DRIVE_TYPE_NAME[DRIVE_TYPE_NAME_LEN + 1];

/* 
 * The mixed media common lib media routines take two arguments.  The first 
 * argument is the media type, library media type, or the name of the media.
 * type.  The second argument is the address of a pointer to the 
 * MEDIA_TYPE_INFO structure.  The MEDIA_TYPE_INFO structure is filled in 
 * by the mixed media common lib routines which contain all the 
 * pertinent information for this media type.
 *
 * The media_type is the ACSLS media type code used by the CSC and ACSLS.
 * The lib_media_type is the code used by the LMU's.  The lib_media_type
 * is described in the LMU Interface Guide, Level 10 and Level 6.
 * The media_type_name is the name used by the cmd_proc and CSC.
 * The cleaning cartridge element tells if the media type can be a cleaning
 * cartridge, never can be a cleaning cartridge, or the LMU can't tell if 
 * the cartridge is a cleaning cartridge.  The max_cleaning_usage tells
 * how many times a cartridge can be used to clean a drive before retiring
 * the tape.  The last two elements indicate what drive types are compatible
 * with this media type.
 *
 * The MEDIA_TYPE (char) is used as an index into the array of MEDIA_TYPE_INFO.
 */
typedef struct {
    MEDIA_TYPE		media_type;
    LIB_MEDIA_TYPE	lib_media_type;
    MEDIA_TYPE_NAME	media_type_name;
    CLN_CART_CAPABILITY	cleaning_cartridge;
    unsigned short	max_cleaning_usage;	/* only used by enter */
    unsigned short	compat_count;		/* # compatible drive types */
    DRIVE_TYPE		compat_drive_types[MM_MAX_COMPAT_TYPES];
} MEDIA_TYPE_INFO;

/* 
 * The mixed media common lib drive routines take two arguments.  The 
 * first argument is the drive type, library drive type, or the name of the 
 * drive type.  The second argument is the address of a pointer to the 
 * DRIVE_TYPE_INFO structure.  The DRIVE_TYPE_INFO structure is filled in
 * by the mixed media common lib routines which contain all the 
 * pertinent information for this drive type.
 *
 * The drive_type is the ACSLS drive type code used by the CSC and ACSLS.
 * The lib_drive_type is the code used by the LMU's.  The lib_drive_type
 * is described in the LMU Interface Guide, Level 10 and Level 6.
 * The cleaning cartridge element tells if the drive type can be a cleaning
 * cartridge, never can be a cleaning cartridge, or the LMU can't tell if 
 * the cartridge is a cleaning cartridge.  Compat_count and 
 * compat_media_types[] indicate what media types are compatible with
 * this drive.  This information is repeated for convenience of the 
 * calling routines.  The preferred_count and preferred_media_types[]
 * contain the customer's server specific preferences for scratch tape
 * selection.  
 *
 * The DRIVE_TYPE (char) is used as an index into the array of DRIVE_TYPE_INFO.
 */
typedef struct {
    DRIVE_TYPE		drive_type;
    LIB_DRIVE_TYPE	lib_drive_type;
    DRIVE_TYPE_NAME	drive_type_name;
    unsigned short	compat_count;		/* # compatible media types */
    MEDIA_TYPE 		compat_media_types[MM_MAX_COMPAT_TYPES];
     
     /* 
      * list of preferred media types when choosing a scratch tape.
      * This list is on a per server basis. 
      */
    unsigned short	preferred_count;	/* # preferred media types */
    MEDIA_TYPE		preferred_media_types[MM_MAX_COMPAT_TYPES];
} DRIVE_TYPE_INFO;

/*
 *      Procedure Type Declarations:
 */
extern STATUS cl_drv_type(DRIVE_TYPE drive_type, 
	DRIVE_TYPE_INFO **drive_type_info);
extern STATUS cl_drv_type_lib(LIB_DRIVE_TYPE drive_type, 
	DRIVE_TYPE_INFO **drive_type_info);
extern STATUS cl_drv_type_name(DRIVE_TYPE_NAME drive_type, 
	DRIVE_TYPE_INFO **drive_type_info);

extern STATUS cl_mt_info(MEDIA_TYPE media_type, 
	MEDIA_TYPE_INFO **media_type_info);
extern STATUS cl_mt_lib(LIB_MEDIA_TYPE media_type, 
	MEDIA_TYPE_INFO **media_type_info);
extern STATUS cl_mt_name(MEDIA_TYPE_NAME media_type, 
	MEDIA_TYPE_INFO **media_type_info);

#endif /* _CL_MM_PUB_ */

