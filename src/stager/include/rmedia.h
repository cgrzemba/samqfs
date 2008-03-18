/*
 * media.h - removable media definitions
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#if !defined(RMEDIA_H)
#define	RMEDIA_H

#pragma ident "$Revision: 1.29 $"

#include "aml/device.h"
#include "aml/stager_defs.h"

/* Enums. */

/*
 * VSN information flags.
 */
enum {
	VI_LOADED =	1 << 0,		/* vsn loaded in drive */
	VI_RESERVED =	1 << 1,		/* reserved */
	VI_UNAVAIL =	1 << 2,		/* unavailable */
	VI_UNUSABLE =	1 << 3,		/* not usable for staging */
	VI_BADMEDIA =	1 << 4		/* bad media */
};

/*
 * Drive information flags.
 */
enum {
	DI_AVAIL =	1 << 0,		/* drive is available for scheduling */
	DI_BUSY =	1 << 1		/* drive is busy */
};

/*
 * Library information flags.
 */
enum {
	LI_AVAIL =	1 << 0,		/* library is avail for scheduling */
	LI_MANUAL =	1 << 1,		/* manual drive */
	LI_REMOTE =	1 << 2,		/* remote */
	LI_HISTORIAN =	1 << 3,		/* historian */
	LI_DISK	=	1 << 4,		/* disk */
	LI_OFF =	1 << 5,		/* device is off */
	LI_THIRDPARTY =	1 << 6,		/* third party */
	LI_HONEYCOMB =	1 << 7 		/* honeycomb */
};

/*
 * Media characteristic flags.
 */
enum {
	MC_DEFAULT =	1 << 0		/* default */
};

#define	IS_DISKARCHIVE_MEDIA(x)	(x == DT_DISK || x == DT_STK5800)
#define	IS_RMARCHIVE_MEDIA(x)	!(IS_DISKARCHIVE_MEDIA(x))

#define	IS_VSN_LOADED(x)	(x->flags & VI_LOADED)
#define	IS_VSN_UNUSABLE(x)	(x->flags & VI_UNUSABLE)
#define	IS_VSN_BADMEDIA(x)	(x->flags & VI_BADMEDIA)
#define	IS_VSN_UNAVAIL(x)	(x->flags & VI_UNAVAIL)

#define	SET_VSN_LOADED(x)	(x->flags |= VI_LOADED)
#define	SET_VSN_UNUSABLE(x)	(x->flags |= VI_UNUSABLE)
#define	SET_VSN_BADMEDIA(x)	(x->flags |= VI_BADMEDIA)
#define	SET_VSN_UNAVAIL(x)	(x->flags |= VI_UNAVAIL)

/*
 * Removable media drive.
 * Each drive is referenced by this structure.
 */
typedef struct DriveInfo {
	ushort_t	flags;
	uname_t    	name;		/* name for display */
	int		lib;		/* library, offset in table */
	dev_ent_t  	*device;	/* device entry */
	VsnInfo_t	vi;		/* loaded VSN info */
} DriveInfo_t;

#define	IS_DRIVE_AVAIL(x)	(x->flags & DI_AVAIL)
#define	SET_DRIVE_AVAIL(x)	(x->flags |= DI_AVAIL)
#define	CLEAR_DRIVE_AVAIL(x)	(x->flags &= ~DI_AVAIL)

#define	IS_DRIVE_BUSY(x)	(x->flags & DI_BUSY)
#define	SET_DRIVE_BUSY(x)	(x->flags |= DI_BUSY)
#define	CLEAR_DRIVE_BUSY(x)	(x->flags &= ~DI_BUSY)

/*
 * Removable media library.
 * Each removable media library (robot or manually mounted drive) is
 * referenced by this structure.
 */
typedef struct LibraryInfo {
	ushort_t	flags;
	uname_t		name;		/* name for display */
	int		eq;		/* equipment number */
	int		num_drives;	/* number of drives in library */
	int		num_allowed_drives;	/* number of drives allowed */
						/*   to use */
	int		num_avail_drives;	/* number of drives available */
						/*   to use */
	dev_ent_t  	*device;	/* device entry */
	int		manual;		/* drive table index for manual drive */
} LibraryInfo_t;

#define	IS_LIB_MANUAL(x)	(x->flags & LI_MANUAL)
#define	IS_LIB_REMOTE(x)	(x->flags & LI_REMOTE)
#define	IS_LIB_HISTORIAN(x)	(x->flags & LI_HISTORIAN)
#define	IS_LIB_DISK(x)		(x->flags & LI_DISK)
#define	IS_LIB_OFF(x)		(x->flags & LI_OFF)
#define	IS_LIB_AVAIL(x)		(x->flags & LI_AVAIL)
#define	IS_LIB_THIRDPARTY(x)	(x->flags & LI_THIRDPARTY)
#define	IS_LIB_HONEYCOMB(x)	(x->flags & LI_HONEYCOMB)

#define	SET_LIB_AVAIL(x)	(x->flags |= LI_AVAIL)
#define	SET_LIB_MANUAL(x)	(x->flags |= LI_MANUAL)
#define	SET_LIB_HISTORIAN(x)	(x->flags |= LI_HISTORIAN)
#define	SET_LIB_REMOTE(x)	(x->flags |= LI_REMOTE)
#define	SET_LIB_DISK(x)		(x->flags |= LI_DISK)
#define	SET_LIB_THIRDPARTY(x)	(x->flags |= LI_THIRDPARTY)
#define	SET_LIB_HONEYCOMB(x)	(x->flags |= LI_HONEYCOMB)

/*
 * Removable media characteristics table.
 * Used to hold media dependent characteristics information.
 */
typedef struct MediaCharsInfo {
	mtype_t		name;		/* media name */
	ushort_t	flags;
	media_t		type;		/* media type */
	int		drives;		/* num of drives that can use media */
	int		bufsize;	/* size of stage buffer * device */
					/*    mau size */
	boolean_t	lockbuf;	/* lock buffer */
} MediaCharsInfo_t;

/* Functions */
int InitMedia();
int IfDrivesAvail();
int GetNumAllDrives();
void ReconfigCatalog();
boolean_t FoundThirdParty();
boolean_t IsCatalogAvail();

LibraryInfo_t *GetLibraries(int *numLibraries);
int IsLibraryDriveFree(int lib);
int GetNumLibraryDrives(int lib);
boolean_t IsLibraryOff(int lib);

DriveInfo_t *GetDrives(int *numDrives);
int IsDriveFree(int drive);
int GetEqOrdinal(int drive);

VsnInfo_t *FindVsn(vsn_t vsn, media_t media);
boolean_t IsVsnAvail(VsnInfo_t *vi, boolean_t *attended);

void MakeMediaCharsTable();
int GetMediaCharsBufsize(media_t type, boolean_t *lockbuf);
void SetMediaCharsBufsize(char *name, int bufsize, boolean_t lockbuf);

#endif /* RMEDIA_H */
