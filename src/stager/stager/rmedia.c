/*
 * rmedia.c - provide access to samfs removable media
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at pkg/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.60 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/mtio.h>

/* Solaris headers. */

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/shm.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/devnm.h"
#include "aml/tar.h"
#include "sam/defaults.h"
#include "aml/diskvols.h"
#include "sam/lib.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"
#include "aml/stager.h"
#include "aml/stager_defs.h"
#include "pub/mig.h"
#if defined(lint)
#include "sam/lint.h"
#endif /* defined(lint) */

/* Local headers. */
#include "stager_lib.h"
#include "stager_config.h"
#include "stager_shared.h"
#include "rmedia.h"

#include "stager.h"
#include "thirdparty.h"

extern shm_alloc_t master_shm;
extern shm_ptr_tbl_t *shm_ptr_tbl;

/* Private data. */

/*
 * Removable media library table.
 */
static struct {
	int	entries;		/* number of entries in table */
	LibraryInfo_t *data;
} libraryTable = { 0, NULL };

static boolean_t removableMediaFound = B_FALSE;

/*
 * Removable media drive table.
 */
static struct {
	int	entries;		/* number of entries in table */
	DriveInfo_t *data;
} driveTable = { 0, NULL };

/*
 * Manual drives.
 */
static int manualDrives;
static boolean_t foundThirdParty = B_FALSE;

/*
 *	Media parameters table.
 */
static struct {
	int 	entries;
	MediaParamsInfo_t *data;
} mediaParamsTable = { 0, NULL };

/*
 * Holds all VSNs in the catalog.
 */
static struct {
	boolean_t	catalog;	/* set TRUE if catalog initialized */
	struct CatalogEntry **cetable;	/* all inuse catalog entries */
	int		entries;	/* number of entries in table */
	VsnInfo_t	*data;
} vsnTable = { B_FALSE, NULL, 0, NULL };

/* Private functions. */
static void initLibraryTable();
static void initVsnTable();

static int setDriveState(boolean_t check);
static void getManualVsn(DriveInfo_t *drive, VsnInfo_t *vi);
static void getLibraryVsn(struct CatalogEntry *ce, VsnInfo_t *vi);
static void getDiskLibraryVsn(int idx, media_t media, VsnInfo_t *vi);
static void getDriveVsn(DriveInfo_t *drive);
static void setLibraryDrivesAllowed(char *name, int count);
static int initCatalog();
static boolean_t isHistorian(int lib);
static boolean_t isSamAttended();

/*
 * Initialize removable media for staging requests.
 */
int
InitMedia(void)
{
	int i;
	int num_cfg_drives;
	sam_stager_drives_t *cfg_drives;

	initLibraryTable();

	/*
	 * If configured, set number of drives on a robot to
	 * use for staging.
	 */
	num_cfg_drives = GetCfgNumDrives();
	cfg_drives = GetCfgDrives();

	for (i = 0; i < num_cfg_drives; i++) {
		setLibraryDrivesAllowed((char *)cfg_drives[i].robot,
		    cfg_drives[i].count);
	}

	Trace(TR_DEBUG, "Removable media initialized");
	return (0);
}


/*
 * Return number of drives in all libraries.
 */
int
GetNumAllDrives(void)
{
	int i;
	LibraryInfo_t *entry;
	int num_drives = 0;

	for (i = 0; i < libraryTable.entries; i++) {
		entry = &libraryTable.data[i];
		/*
		 * Skip historian and third party.
		 */
		if (IS_LIB_HISTORIAN(entry) || IS_LIB_THIRDPARTY(entry))
			continue;

		num_drives +=  entry->li_numDrives;
	}

	return (num_drives);
}

/*
 * Check if specified drive is free.
 */
int
IsDriveFree(
	int drive)
{
	DriveInfo_t *entry;

	(void) setDriveState(B_TRUE);

	entry = &driveTable.data[drive];
	return (IS_DRIVE_AVAIL(entry) && (IS_DRIVE_BUSY(entry) == 0));
}

/*
 * Return TRUE if any drives are available
 * in any of the libraries.
 */
int
IfDrivesAvail(void)
{
	int i;
	int avail;
	LibraryInfo_t *lib;

	ReconfigLock();		/* wait on reconfig */

	(void) setDriveState(B_TRUE);

	avail = 0;
	for (i = 0; i < libraryTable.entries; i++) {
		lib = &libraryTable.data[i];

		/*
		 * If not historian or third party, add number of
		 * drives available.
		 */
		if (!IS_LIB_HISTORIAN(lib) && !IS_LIB_THIRDPARTY(lib)) {
			avail += lib->li_numAvailDrives;
		}
	}
	ReconfigUnlock();		/* allow reconfig */

	return (avail != 0);
}

/*
 * Return number of drives to available or allowed use in specified
 * library, which ever value is smaller.
 */
int
GetNumLibraryDrives(
	int lib)
{
	int num_drives = 0;

	ReconfigLock();		/* wait on reconfig */

	if (libraryTable.data[lib].li_numAvailDrives <
	    libraryTable.data[lib].li_numAllowedDrives) {

		num_drives = libraryTable.data[lib].li_numAvailDrives;

	} else  {

		num_drives = libraryTable.data[lib].li_numAllowedDrives;
	}
	ReconfigUnlock();		/* allow reconfig */

	return (num_drives);
}

/*
 * Check if there is a drive free/available in specified library.
 */
int
IsLibraryDriveFree(
	int lib)
{
	int avail = FALSE;
	int i;
	DriveInfo_t *drive;

	ReconfigLock();		/* wait on reconfig */

	for (i = 0; i < driveTable.entries; i++) {
		drive = &driveTable.data[i];
		if (drive->dr_lib == lib && IS_DRIVE_AVAIL(drive)) {
			avail = TRUE;
			break;
		}
	}
	ReconfigUnlock();		 /* allow reconfig */

	return (avail);
}

/*
 * Find requested VSN info structure in the VSN table.
 */
VsnInfo_t *
FindVsn(
	vsn_t vsn,
	media_t media)
{
	VsnInfo_t *vi = NULL;
	LibraryInfo_t *lib;
	int i;
	boolean_t first;

	first = B_TRUE;		/* first search */
	ReconfigLock();		/* wait on reconfig */

	if (vsnTable.catalog == B_TRUE &&
	    removableMediaFound == B_TRUE && CatalogSync() > 0) {
		Trace(TR_MISC, "Catalog invalid");
		vsnTable.catalog = B_FALSE;
		CatalogTerm();		/* remove mapped catalog table */
	}

	if (vsnTable.catalog == B_FALSE) {
		if (initCatalog() == 0) {
			Trace(TR_MISC, "Catalog initialized");
		}
		initVsnTable();
	}

retrySearch:

	for (i = 0; i < vsnTable.entries; i++) {

		lib = &libraryTable.data[vsnTable.data[i].lib];
		if (IS_LIB_MANUAL(lib)) {
			/*
			 * Get VSN from manual drive.
			 */
			getManualVsn(&driveTable.data[lib->li_manual],
			    &vsnTable.data[i]);

		} else if (IS_LIB_DISK(lib)) {
			/*
			 * Get disk archiving VSN.
			 */
			getDiskLibraryVsn(vsnTable.data[i].disk_idx, DT_DISK,
			    &vsnTable.data[i]);

		} else if (IS_LIB_HONEYCOMB(lib)) {
			/*
			 * Get honeycomb archiving VSN.
			 */
			getDiskLibraryVsn(vsnTable.data[i].disk_idx,
			    DT_STK5800, &vsnTable.data[i]);

		} else {
			/*
			 * Get VSN managed in library.
			 */
			if ((strcmp(vsn, vsnTable.data[i].ce->CeVsn) == 0) &&
			    (media ==
			    sam_atomedia(vsnTable.data[i].ce->CeMtype))) {
				getLibraryVsn(vsnTable.data[i].ce,
				    &vsnTable.data[i]);
			}
		}

		if ((strcmp(vsn, vsnTable.data[i].vsn) == 0) &&
		    (media == vsnTable.data[i].media)) {
			/*
			 * Found VSN.
			 */
			vi = &vsnTable.data[i];
			if (vi->ce != NULL) {
				/*
				 * VSN in library.  Check if VSN has moved
				 * .ie imported or exported.
				 */
				struct CatalogEntry *ce;
				LibraryInfo_t *lib;

				ce = vi->ce;
				lib = &libraryTable.data[vi->lib];

				if (ce->CeEq != lib->li_eq) {
					int i;

					for (i = 0; i < libraryTable.entries;
					    i++) {
						lib = &libraryTable.data[i];
						if (ce->CeEq == lib->li_eq) {
							vi->lib = i;
							break;
						}
					}
				}
			}

			/*
			 * If disk volume, check existence of the
			 * the diskvols.seqnum.
			 */
			if (media == DT_DISK) {
				extern char *program_name;
				struct DiskVolumeInfo *dv;
				DiskVolsDictionary_t *diskvols;
				boolean_t avail = B_FALSE;

				diskvols = DiskVolsNewHandle(program_name,
				    DISKVOLS_VSN_DICT, DISKVOLS_RDONLY);
				if (diskvols != NULL) {
					(void) diskvols->Get(diskvols, vsn,
					    &dv);
					if (dv != NULL) {
						if (DiskVolsIsAvail(vsn, dv,
						    B_FALSE, DVA_stager)
						    == B_TRUE) {
							avail = B_TRUE;
						}
					}
				}
				(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
				if (avail == B_FALSE) {
					vi = NULL;
				}
			}
			break;
		}
	}

	/*
	 * If a removable media volume was not found, check if it exists in
	 * catserver's catalog.  There are changes (.eg import, export,
	 * relabel) that may result in the stager vsntable being out of date.
	 * If found in the catalog, reinitialize the vsntable and search again.
	 */
	if (vi == NULL && IS_RMARCHIVE_MEDIA(media) && first == B_TRUE) {
		struct CatalogEntry ced, *ce;

		first = B_FALSE;
		ce = CatalogGetCeByMedia(sam_mediatoa(media), vsn, &ced);

		if (ce != NULL) {
			/*
			 * Volume found in catserver's memory mapped file.
			 * Reinitialize the vsntable and search again.
			 */
			initVsnTable();
			goto retrySearch;
		}
	}

	ReconfigUnlock();	/* allow reconfig */

	return (vi);
}

/*
 * Check availability of VSN.
 */
boolean_t
IsVsnAvail(
	VsnInfo_t *vi,
	boolean_t *attended)
{
	boolean_t avail = B_TRUE;
	*attended = B_FALSE;

	if (vi == NULL) {
		avail = B_FALSE;
		*attended = isSamAttended();

	} else {
		if (IS_VSN_UNAVAIL(vi) || isHistorian(vi->lib)) {
			avail = B_FALSE;
			if (IS_VSN_UNAVAIL(vi) == 0) {
				*attended = isSamAttended();
			}
		}
	}
	return (avail);
}

/*
 * Get removable media drive table.
 */
DriveInfo_t *
GetDrives(
	int *numDrives)
{
	*numDrives = driveTable.entries;
	return (driveTable.data);
}

/*
 * Get removable media library table.
 */
LibraryInfo_t *
GetLibraries(
	int *numLibraries)
{
	*numLibraries = libraryTable.entries;
	return (libraryTable.data);
}

/*
 * Get equipment ordinal for specified drive.
 */
int
GetEqOrdinal(
	int drive)
{
	int eq = 0;

	dev_ent_t *dev;
	DriveInfo_t *entry = &driveTable.data[drive];
	dev = entry->dr_device;
	if (dev != NULL) {
		eq = dev->eq;
	}
	return (eq);
}

/*
 * Return TRUE if library is off or library is on but no drives are
 * available.
 */
boolean_t
IsLibraryOff(
	int lib)
{
	LibraryInfo_t *entry = &libraryTable.data[lib];

	return (IS_LIB_OFF(entry) ||
	    (IS_LIB_AVAIL(entry) && entry->li_numAvailDrives == 0));
}

/*
 * Reconfigure catalog.
 */
void
ReconfigCatalog(void)
{
	CatalogTerm();
	vsnTable.catalog = B_FALSE;
}


boolean_t
FoundThirdParty(void)
{
	return (foundThirdParty);
}

/*
 * Returns TRUE if removable media catalog is available.
 */
boolean_t
IsCatalogAvail(void)
{
	return (vsnTable.catalog);
}

/*
 * Set number of drives allowed to use for staging from
 * specified library.
 */
static void
setLibraryDrivesAllowed(
	char *name,
	int count)
{
	int i;
	LibraryInfo_t *lib;

	for (i = 0; i < libraryTable.entries; i++) {
		lib = &libraryTable.data[i];
		if (strcmp(name, lib->li_name) == 0) {
			if (count < lib->li_numAllowedDrives) {
				lib->li_numAllowedDrives = count;
			}
			break;
		}
	}
}

/*
 * Initialize table of media libraries for stager.  This function
 * builds tables for the libraries, removable media drives and manual
 * drives from shared memory.
 */
static void
initLibraryTable(void)
{
	extern char *program_name;
	int jmap;
	dev_ent_t *dev;
	size_t size;
	LibraryInfo_t *lib;
	DriveInfo_t *drive;
	int ldx;
	int ddx;
	dev_ent_t *dev_head;
	int numDiskVols;
	int numHoneycombVols;
	DiskVolsDictionary_t *diskvols;
	boolean_t found_historian = B_FALSE;

	/*
	 * If table already exists, free current table
	 * and re-initialize.
	 */
	if (libraryTable.data != NULL) {
		SamFree(libraryTable.data);
		SamFree(driveTable.data);

		libraryTable.data = NULL;
		driveTable.data = NULL;
	}

	if (vsnTable.data != NULL) {
		SamFree(vsnTable.data);

		vsnTable.data = NULL;
		if (vsnTable.catalog == B_TRUE) {
			CatalogTerm();
			vsnTable.catalog = B_FALSE;
		}
	}

	libraryTable.entries = 0;
	driveTable.entries = 0;
	vsnTable.entries = 0;

	dev_head = GetDevices(B_FALSE);

	for (dev = dev_head; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {

		if (dev->equ_type == DT_HISTORIAN) {
			libraryTable.entries++;
			found_historian = B_TRUE;

		} else if (IS_THIRD_PARTY(dev)) {
			libraryTable.entries++;
			foundThirdParty = B_TRUE;

		} else if (IS_ROBOT(dev) || dev->equ_type == DT_PSEUDO_SC) {
			dev_ent_t *find;

			libraryTable.entries++;

			/*
			 * Find all drives belonging to this library.
			 */
			for (find = dev_head; find != NULL;
			    find = (struct dev_ent *)SHM_REF_ADDR(find->next)) {
				if (find->eq != dev->eq) {
					if (find->fseq == dev->eq) {
						driveTable.entries++;
					}
				}
			}

		} else if (dev->fseq == 0) {
			/*
			 * Manual drive.
			 */
			libraryTable.entries++;
			driveTable.entries++;
			manualDrives++;
		}
	}

	Trace(TR_MISC, "Removable media libraries: %d drives: %d",
	    found_historian ? libraryTable.entries - 1 : libraryTable.entries,
	    driveTable.entries);

	/*
	 * If defined, allocate a library table for disk archiving.
	 * The disk archiving library will have a disk drive defined
	 * for each vsn.
	 */
	numDiskVols = 0;
	numHoneycombVols = 0;

	diskvols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT, 0400);
	if (diskvols != NULL) {
		(void) diskvols->Numof(diskvols, &numDiskVols, DV_numof_disk);
		(void) diskvols->Numof(diskvols, &numHoneycombVols,
		    DV_numof_honeycomb);
		(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
	}

	if (numDiskVols > 0) {
		libraryTable.entries++;
		driveTable.entries += numDiskVols * STREAMS_PER_DISKVOL;
		Trace(TR_MISC, "Disk volumes: %d", numDiskVols);
	}

	if (numHoneycombVols > 0) {
		libraryTable.entries++;
		driveTable.entries += numHoneycombVols;
		Trace(TR_MISC, "Honeycomb volumes: %d", numHoneycombVols);
	}

	/*
	 * Issue message if no removable media or disk archive volumes
	 * were found.
	 */
	if (libraryTable.entries == 0 ||
	    (found_historian && libraryTable.entries == 1)) {
		SendCustMsg(HERE, 19007);
	}


	/*
	 * Allocate library and drive tables and enter data.  If the
	 * table count is zero, allocate one table entry anyway.
	 */

	size = (libraryTable.entries ? libraryTable.entries : 1)
	    * sizeof (LibraryInfo_t);
	SamMalloc(libraryTable.data, size);
	(void) memset(libraryTable.data, 0, size);

	size = (driveTable.entries ? driveTable.entries : 1)
	    * sizeof (DriveInfo_t);
	SamMalloc(driveTable.data, size);
	(void) memset(driveTable.data, 0, size);

	lib = libraryTable.data;
	drive = driveTable.data;
	ldx = 0;
	ddx = 0;

	for (dev = dev_head; dev != NULL;
	    dev = (struct dev_ent *)SHM_REF_ADDR(dev->next)) {

		lib->li_flags = 0;
		lib->li_device = dev;

		if (dev->type == DT_HISTORIAN) {
			(void) strncpy(lib->li_name, "Historian",
			    sizeof (lib->li_name));
			SET_LIB_HISTORIAN(lib);
			lib->li_eq = dev->eq;
			lib->li_numDrives = manualDrives;

		} else if (IS_THIRD_PARTY(dev)) {
			(void) strncpy(lib->li_name, "ThirdParty",
			    sizeof (lib->li_name));
			SET_LIB_THIRDPARTY(lib);
			lib->li_eq = dev->eq;
			(void) InitMigration(dev);

		} else if (IS_ROBOT(dev) || dev->equ_type == DT_PSEUDO_SC) {
			dev_ent_t *find;

			memcpy(lib->li_name, dev->set, sizeof (lib->li_name));
			lib->li_eq = dev->eq;

			/* Check if remote sam client */
			if (dev->equ_type == DT_PSEUDO_SC) {
				SET_LIB_REMOTE(lib);
			}

			/*
			 * Find all drives belonging to this library.
			 */
			find = dev_head;	/* Head of dev entries */
			for (find = dev_head; find != NULL;
			    find = (struct dev_ent *)SHM_REF_ADDR(find->next)) {
				if (find->eq != dev->eq) {
					if (find->fseq == dev->eq) {
						lib->li_numDrives++;

						sprintf(drive->dr_name, "%s%d",
						    sam_mediatoa(find->type),
						    find->eq);
						drive->dr_device = find;
						drive->dr_lib = ldx;

						/*
						 * Increment to next drive
						 * table entry.
						 */
						drive++;
						ddx++;
					}
				}
			}

		} else if (dev->fseq == 0) {
			/*
			 * Manual drive.
			 */
			sprintf(lib->li_name, "%s%d", sam_mediatoa(dev->type),
			    dev->eq);
			SET_LIB_MANUAL(lib);
			lib->li_numDrives = 1;
			lib->li_manual = ddx;
			lib->li_eq = dev->eq;

			sprintf(drive->dr_name, "%s%d", sam_mediatoa(dev->type),
			    dev->eq);
			drive->dr_device = dev;
			drive->dr_lib = ldx;

			/* Increment to next drive table entry. */
			drive++;
			ddx++;

		} else {
			/* Not a library, continue to next dev entry. */
			continue;
		}

		/*
		 * Set number of drives allowed.  Increment to next
		 * library table entry.
		 */
		lib->li_numAllowedDrives = lib->li_numDrives;
		lib++;
		ldx++;
	}

	if (numDiskVols > 0) {
		char *mtype;

		mtype = sam_mediatoa(DT_DISK);
		(void) strncpy(lib->li_name, mtype, sizeof (lib->li_name));
		SET_LIB_DISK(lib);
		SET_LIB_AVAIL(lib);
		lib->li_numDrives = numDiskVols * STREAMS_PER_DISKVOL;
		lib->li_numAllowedDrives = numDiskVols * STREAMS_PER_DISKVOL;
		lib->li_numAvailDrives = numDiskVols * STREAMS_PER_DISKVOL;

		/*
		 * Define drives belonging to disk archive library.
		 */
		for (jmap = 0; jmap < numDiskVols * STREAMS_PER_DISKVOL;
		    jmap++) {

			sprintf(drive->dr_name, "%s%d", mtype, jmap);
			drive->dr_lib = ldx;
			SET_DRIVE_AVAIL(drive);

			/* Increment to next drive table entry. */
			drive++;
			ddx++;
		}

		lib++;
		ldx++;
	}

	if (numHoneycombVols > 0) {
		char *mtype;

		mtype = sam_mediatoa(DT_STK5800);
		(void) strncpy(lib->li_name, mtype, sizeof (lib->li_name));
		SET_LIB_HONEYCOMB(lib);
		SET_LIB_AVAIL(lib);
		lib->li_numDrives = numHoneycombVols;
		lib->li_numAllowedDrives = numHoneycombVols;
		lib->li_numAvailDrives = numHoneycombVols;

		/*
		 * Define drives belonging to honeycomb archive library.
		 */
		for (jmap = 0; jmap < numHoneycombVols; jmap++) {

			sprintf(drive->dr_name, "%s%d", mtype, jmap);
			drive->dr_lib = ldx;
			SET_DRIVE_AVAIL(drive);

			/* Increment to next drive table entry. */
			drive++;
			ddx++;
		}
	}

	(void) setDriveState(B_FALSE);		/* get current drive state */
}

/*
 * Make parameters table for media.  This table is created from
 * information in dev_nm2mt[].
 */
void
MakeMediaParamsTable(void)
{
	size_t size;
	int i = 0;
	extern sam_defaults_t *Defaults;

	if (mediaParamsTable.data != NULL) {
		SamFree(mediaParamsTable.data);

		mediaParamsTable.data = NULL;
		mediaParamsTable.entries = 0;
	}

	while (dev_nm2mt[i].nm != NULL) {
		mediaParamsTable.entries++;
		i++;
	}

	size = mediaParamsTable.entries * sizeof (MediaParamsInfo_t);
	SamMalloc(mediaParamsTable.data, size);
	(void) memset(mediaParamsTable.data, 0, size);

	for (i = 0; i < mediaParamsTable.entries; i++) {
		dev_nm_t *dev;
		MediaParamsInfo_t *mp;

		mp = &mediaParamsTable.data[i];
		dev = &dev_nm2mt[i];

		(void) strcpy(mp->mp_name, dev->nm);
		mp->mp_type = dev->dt;
		mp->mp_bufsize = STAGER_DEFAULT_MC_BUFSIZE;

		if (mp->mp_type == DT_OPTICAL) {
			mp->mp_type = Defaults->optical;
			mp->mp_flags |= MC_DEFAULT;
		}
		if (mp->mp_type == DT_TAPE) {
			mp->mp_type = Defaults->tape;
			mp->mp_flags |= MC_DEFAULT;
		}
	}
}

/*
 * Set buffer size for media type.
 */
void
SetMediaParamsBufsize(
	char *name,
	int bufsize,
	boolean_t lockbuf)
{
	int i;
	media_t type;

	type = sam_atomedia(name);

	for (i = 0; i < mediaParamsTable.entries; i++) {
		MediaParamsInfo_t *mp;

		mp = &mediaParamsTable.data[i];

		/*
		 * Check for matching media type.  Need to change actual
		 * media classs and, if applicable, the default media class.
		 */
		if (mp->mp_type == type) {
			mp->mp_bufsize = bufsize;
			mp->mp_lockbuf = lockbuf;
		}
	}
}

/*
 * Get buffer size and lock for media type.
 */
int
GetMediaParamsBufsize(
	media_t type,
	boolean_t *lockbuf)
{
	int i;
	int bufsize = 0;

	for (i = 0; i < mediaParamsTable.entries; i++) {
		MediaParamsInfo_t *mp;

		mp = &mediaParamsTable.data[i];
		if (mp->mp_type == type) {
			bufsize = mp->mp_bufsize;
			*lockbuf = mp->mp_lockbuf;
			break;
		}
	}
	return (bufsize);
}

/*
 * Initialize removable media catalog for staging requests.
 */
int
initCatalog(void)
{
	extern char *program_name;
	LibraryInfo_t *lib;
	int i;
	int rc = 0;

	/*
	 * Check for existence of removable media libraries and don't
	 * attempt to initialize catalog server if no libraries are found
	 * (.ie only disk/honeycomb archiving has been configured).
	 */
	removableMediaFound = B_FALSE;
	for (i = 0; i < libraryTable.entries; i++) {
		lib = &libraryTable.data[i];
		if ((lib->li_flags & (LI_DISK | LI_HONEYCOMB)) == 0) {
			removableMediaFound = B_TRUE;
			break;
		}
	}

	if (removableMediaFound && CatalogInit(program_name) == -1) {
		Trace(TR_ERR, "Catalog initialization failed");
		rc = -1;
	} else {
		vsnTable.catalog = B_TRUE;
	}
	return (rc);
}

/*
 * Initialize table of vsns for stager.  This function builds a vsn
 * table from the catalogs for all libraries.
 */
static void
initVsnTable(void)
{
	extern char *program_name;
	int i, j;
	int vin;
	size_t size;
	LibraryInfo_t *lib;

	struct CatalogEntry *cetable;
	int numCatalogEntries;
	dev_ent_t *dev;
	int num_mids;
	int numDiskVols;
	int numHoneycombVols;
	DiskVolsDictionary_t *diskvols;

	/*
	 * If it exists, free current table.
	 */
	if (vsnTable.data != NULL) {

		SamFree(vsnTable.data);

		vsnTable.data = NULL;
		vsnTable.entries = 0;

		for (i = 0; i < libraryTable.entries; i++) {
			if (vsnTable.cetable[i] != NULL) {
				free(vsnTable.cetable[i]);
				vsnTable.cetable[i] = NULL;
			}
		}
		SamFree(vsnTable.cetable);
		vsnTable.cetable = NULL;
	}

	numDiskVols = 0;
	numHoneycombVols = 0;

	/*
	 * Compute size of vsn table and allocate it.
	 */
	for (i = 0; i < libraryTable.entries; i++) {
		lib = &libraryTable.data[i];
		if (IS_LIB_MANUAL(lib)) {
			vsnTable.entries++;

		} else if (IS_LIB_DISK(lib)) {
			diskvols = DiskVolsNewHandle(program_name,
			    DISKVOLS_VSN_DICT, DISKVOLS_RDONLY);
			if (diskvols != NULL) {
				(void) diskvols->Numof(diskvols, &numDiskVols,
				    DV_numof_disk);
				vsnTable.entries += numDiskVols;
				(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
			}
		} else if (IS_LIB_HONEYCOMB(lib)) {
			diskvols = DiskVolsNewHandle(program_name,
			    DISKVOLS_VSN_DICT, DISKVOLS_RDONLY);
			if (diskvols != NULL) {
				(void) diskvols->Numof(diskvols,
				    &numHoneycombVols, DV_numof_honeycomb);
				vsnTable.entries += numHoneycombVols;
				(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
			}

		} else {
			if (vsnTable.catalog) {
				dev = lib->li_device;
				cetable = CatalogGetEntriesByLibrary(dev->eq,
				    &numCatalogEntries);
				vsnTable.entries += numCatalogEntries;
				if (cetable != NULL) {
					free(cetable);
				}
			}
		}
	}

	if (vsnTable.entries == 0) {
		return;
	}

	size = vsnTable.entries * sizeof (VsnInfo_t);
	SamMalloc(vsnTable.data, size);
	(void) memset(vsnTable.data, 0, size);

	size = libraryTable.entries * sizeof (struct CatalogEntry *);
	SamMalloc(vsnTable.cetable, size);
	(void) memset(vsnTable.cetable, 0, size);

	/*
	 * Examine each library.
	 */
	vin = 0;	/* VSN table index */

	for (i = 0; i < libraryTable.entries; i++) {

		dev = libraryTable.data[i].li_device;

		lib = &libraryTable.data[i];
		if (IS_LIB_MANUAL(lib)) {
			/*
			 * Manual drive.
			 */
			vsnTable.data[vin].lib = i;
			vsnTable.data[vin].ce = NULL;
			vin++;

		} else if (IS_LIB_DISK(lib)) {
			/*
			 * Disk archiving.
			 */
			num_mids = numDiskVols;
			for (j = 0; j < num_mids; j++) {
				vsnTable.data[vin].lib = i;
				vsnTable.data[vin].disk_idx = j;
				vin++;
			}

		} else if (IS_LIB_HONEYCOMB(lib)) {
			/*
			 * Honeycomb archiving.
			 */
			num_mids = numHoneycombVols;
			for (j = 0; j < num_mids; j++) {
				vsnTable.data[vin].lib = i;
				vsnTable.data[vin].disk_idx = j;
				vin++;
			}

		} else {
			/*
			 * Get count of VSNs in robot's catalog.
			 */
			numCatalogEntries = 0;
			if (vsnTable.catalog) {
				cetable = CatalogGetEntriesByLibrary(dev->eq,
				    &numCatalogEntries);
				vsnTable.cetable[i] = cetable;
			}

			if (cetable != NULL && numCatalogEntries > 0) {

				for (j = 0; j < numCatalogEntries; j++) {
					vsnTable.data[vin].lib = i;
					vsnTable.data[vin].ce = &cetable[j];
					vin++;
				}
			}
		}
	}
}

/*
 * Status drives and set state accordingly.  Returns
 * TRUE if a library state has changed.
 */
static int
setDriveState(
	boolean_t check)
{
	int changed = 0;

	int i;
	int manual_drives;
	int avail_drives;
	LibraryInfo_t *lib;
	DriveInfo_t *drive;
	dev_ent_t *dev;
	dev_ent_t *ent;
	int flags;

	dev_ent_t *dev_head;

	dev_head = GetDevices(check);

	manual_drives = 0;
	drive = driveTable.data;

	for (i = 0; i < libraryTable.entries; i++) {

		lib = &libraryTable.data[i];
		if (lib->li_flags & (LI_DISK | LI_HONEYCOMB))
			continue;

		dev = lib->li_device;
		avail_drives = 0;
		flags = 0;

		if (IS_LIB_HISTORIAN(lib)) {

			lib->li_numAllowedDrives = manual_drives;
			lib->li_numAvailDrives = manual_drives;
			avail_drives = manual_drives;

		} else if (IS_LIB_THIRDPARTY(lib)) {

			lib->li_numAllowedDrives = 1;
			lib->li_numAvailDrives = 1;
			avail_drives = 1;

			if (dev->state == DEV_ON) {
				flags = LI_AVAIL;
			} else {
				flags = LI_OFF;
			}

		} else if (IS_LIB_MANUAL(lib)) {

			/*
			 * Manual drive is available if drive is on and ready.
			 */

			drive->dr_flags = 0;
			if (dev->state == DEV_ON && dev->status.b.ready) {
				flags = LI_AVAIL;
				SET_DRIVE_AVAIL(drive);
				if (dev->status.b.opened) {
					SET_DRIVE_BUSY(drive);
				}
				getDriveVsn(drive);
				avail_drives++;

			} else {
				CLEAR_DRIVE_AVAIL(drive);
				(void) memset(&drive->dr_vi, 0,
				    sizeof (VsnInfo_t));
			}
			if (IS_LIB_MANUAL(lib))
				manual_drives++;
			drive++;

		} else {

			/*
			 * Robot.
			 */

			if (dev == NULL) {
				/*
				 * This shouldn't happen.
				 */
				continue;
			}

			if ((dev->state == DEV_ON) &&
			    (dev->eq == 0 || dev->status.b.ready) &&
			    (dev->status.b.audit == 0)) {

				flags = LI_AVAIL; /* mark library available */
			} else if (dev->state == DEV_OFF) {
				flags = LI_OFF;		/* library is off */
			}

			/*
			 * Check each drive belonging to this robot.
			 */
			for (ent = dev_head; ent != NULL;
			    ent = (struct dev_ent *)SHM_REF_ADDR(ent->next)) {

				if (ent->eq == dev->eq) {
					continue;
				}
				if (ent->fseq != dev->eq) {
					continue;
				}

				/*
				 * Drive is available if library is available
				 * drive is on.
				 */
				drive->dr_flags = 0;
				if ((flags & LI_AVAIL) &&
				    ent->state == DEV_ON) {
					SET_DRIVE_AVAIL(drive);
					avail_drives++;
					if (ent->status.b.ready) {
						getDriveVsn(drive);
						if (dev->status.b.opened) {
							SET_DRIVE_BUSY(drive);
						}
					}
				} else {
					CLEAR_DRIVE_AVAIL(drive);
					(void) memset(&drive->dr_vi, 0,
					    sizeof (VsnInfo_t));
				}
				drive++;
			}
		}

		if ((lib->li_flags & (LI_AVAIL | LI_OFF)) != flags ||
		    lib->li_numAvailDrives != avail_drives) {

			/*
			 * Library state has changed.
			 */

			lib->li_flags &= ~(LI_AVAIL | LI_OFF);
			lib->li_flags |= flags;
			lib->li_numAvailDrives = avail_drives;
			Trace(TR_MISC, "Library %d state change: 0x%x %d",
			    lib->li_eq, lib->li_flags, avail_drives);
			changed = TRUE;
		}
	}
	return (changed);
}

/*
 * Get VSN information for a manual drive.
 */
static void
getManualVsn(
	DriveInfo_t *drive,
	VsnInfo_t *vi)
{
	dev_ent_t *dev;
	int i;

	dev = drive->dr_device;

	if (dev->status.b.ready == 0 ||
	    dev->status.b.present == 0 ||
	    dev->status.b.labeled == 0) {

		*vi->vsn = '\0';
		SET_VSN_UNUSABLE(vi);

	} else {

		memmove(vi->vsn, dev->vsn, sizeof (vi->vsn));
		vi->flags = 0;
		vi->media = dev->type;
		for (i = 0; i < driveTable.entries; i++) {
			if (driveTable.data[i].dr_lib == vi->lib &&
			    strcmp(driveTable.data[i].dr_vi.vsn,
			    vi->vsn) == 0) {

				SET_VSN_LOADED(vi);	/* mark loaded */
				vi->drive = i;		/* set drive */
				break;
			}
		}
		vi->ce = NULL;
	}
}

/*
 * Get VSN managed in library's catalog at specified slot.
 */
static void
getLibraryVsn(
	struct CatalogEntry *ce,
	VsnInfo_t *vi)
{
	if (ce != NULL && ce->CeVsn[0] != '\0') {

		if (CatalogGetCeByMedia(ce->CeMtype, ce->CeVsn, ce) != NULL) {

			memmove(vi->vsn, ce->CeVsn, sizeof (vsn_t));
			vi->flags = 0;
			vi->media = sam_atomedia(ce->CeMtype);

			if (ce->CeStatus & CES_bad_media) {
				SET_VSN_BADMEDIA(vi);
			}

			if (ce->CeStatus & CES_unavail) {
				SET_VSN_UNAVAIL(vi);
			}

			/*
			 * If VSN is usable, see if its loaded.
			 */
			if (!IS_VSN_UNUSABLE(vi)) {
				int i;
				DriveInfo_t *dr;

				for (i = 0; i < driveTable.entries; i++) {
					dr = &driveTable.data[i];

					if (dr->dr_lib == vi->lib &&
					    dr->dr_vi.vsn[0] != '\0' &&
					    strcmp(dr->dr_vi.vsn,
					    vi->vsn) == 0) {

						/* mark loaded */
						SET_VSN_LOADED(vi);

						vi->drive = i;	/* set drive */
						break;
					}
				}
			}
		}
	}
}

/*
 * Get VSN managed in disk archiving library.
 */
static void
getDiskLibraryVsn(
	int idx,
	media_t media,
	VsnInfo_t *vi)
{
	extern char *program_name;
	int i;
	DiskVolsDictionary_t *diskvols;
	DiskVolumeInfo_t *dv;
	boolean_t honeycomb;

	honeycomb = B_FALSE;
	if (media == DT_STK5800) {
		honeycomb = B_TRUE;
	}

	diskvols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT,
	    DISKVOLS_RDONLY);
	if (diskvols == NULL) {
		return;
	}

	if (vi->vsn == NULL || vi->vsn[0] == '\0') {
		int vin;
		char *volName;

		vin = 0;
		(void) diskvols->BeginIterator(diskvols);

		while (diskvols->GetIterator(diskvols, &volName, &dv) == 0) {

			if ((honeycomb && DISKVOLS_IS_HONEYCOMB(dv)) ||
			    (!honeycomb && !DISKVOLS_IS_HONEYCOMB(dv))) {
				if (vin == idx) {
					strncpy(vi->vsn, volName,
					    sizeof (vi->vsn));
					vi->flags = 0;
					vi->media = media;
					if (dv->DvFlags & DV_unavail) {
						SET_VSN_UNAVAIL(vi);
					}
					break;
				}
				vin++;
			}
		}
		(void) diskvols->EndIterator(diskvols);

	} else {
		/*
		 * Refresh disk volume flags.
		 */
		(void) diskvols->Get(diskvols, vi->vsn, &dv);
		vi->flags = 0;
		if (dv == NULL || (dv != NULL && (dv->DvFlags & DV_unavail))) {
			SET_VSN_UNAVAIL(vi);
		}
	}
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);

	/*
	 *	If VSN is usable, see if its loaded.
	 */
	if (!IS_VSN_UNUSABLE(vi)) {
		for (i = 0; i < driveTable.entries; i++) {
			if (driveTable.data[i].dr_lib == vi->lib &&
			    strcmp(driveTable.data[i].dr_vi.vsn,
			    vi->vsn) == 0) {

				SET_VSN_LOADED(vi);	/* mark loaded */
				vi->drive = i;		/* set drive */
				break;
			}
		}
	}
}

/*
 * Get VSN information for a drive.
 */
static void
getDriveVsn(
	DriveInfo_t *drive)
{
	VsnInfo_t *vi;
	dev_ent_t *dev;
	int i;

	vi = &drive->dr_vi;
	dev = drive->dr_device;

	if (dev->status.b.ready == 0 ||
	    dev->status.b.present == 0 ||
	    dev->status.b.labeled == 0) {

		*vi->vsn = '\0';
		SET_VSN_UNUSABLE(vi);

	} else {

		memmove(vi->vsn, dev->vsn, sizeof (vi->vsn));
		vi->flags = 0;
		vi->media = dev->type;
		vi->lib = drive->dr_lib;
		for (i = 0; i < driveTable.entries; i++) {
			if (driveTable.data[i].dr_lib == vi->lib &&
			    strcmp(driveTable.data[i].dr_vi.vsn,
			    vi->vsn) == 0) {

				SET_VSN_LOADED(vi);	/* mark loaded */
				vi->drive = i;		/* set drive */
				break;
			}
		}
		vi->ce = NULL;
	}
}

/*
 * Check if historian.
 */
static boolean_t
isHistorian(
	int lib)
{
	LibraryInfo_t *entry = &libraryTable.data[lib];

	return (IS_LIB_HISTORIAN(entry));
}

/*
 * Check if Sam is attended.  If the attended=no directive is
 * set in defaults.conf, it declares that no operator is available
 * to handle load requests.
 */
static boolean_t
isSamAttended(void)
{
	extern sam_defaults_t *Defaults;
	boolean_t attended = B_TRUE;

	if ((Defaults->flags & DF_ATTENDED) == FALSE) {
		attended = B_FALSE;
	}
	return (attended);
}
