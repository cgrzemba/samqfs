/*
 * itemize.c -  Catalog SAM-FS device.
 *
 *	Description:
 *
 *	    Itemize generates a list of the files cataloged on the
 *	    given optical disk.
 *
 *	Command Syntax:
 *
 *	    itemize  [-file|f fff]  [-owner|u uuu] [-group|g ggg] [-v]
 *			[-2] device
 *	    where:
 *		device	    Specifies the name of the optical disk
 *			    device to catalog.
 *
 *		-file	    Only files with the specified file
 *		-f	    identifier will be listed.
 *
 *		-owner	    Only files with the specified owner
 *		-u	    identifier will be listed.
 *
 *		-group	    Only files with the specified group
 *		-g	    identifier will be listed.
 *
 *		-2	    Two lines per file will be outputed thus
 *			    making more readable output for terminals.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.23 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/*	Include files:							*/

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#define DEC_INIT
#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "aml/odlabels.h"
#include "aml/external_data.h"
#include "sam/devnm.h"
#include "sam/lib.h"
#include "sam/custmsg.h"

#define		SHM_ADDR(a, x)  ((char *)a.shared_memory + (int)(x))


/*	Global tables & pointers:					*/

dev_ptr_tbl_t	*Dev_Tbl;		/* Device pointer table		*/
int		Max_Devices;		/* Maxinum no. of devices	*/
shm_alloc_t	shm_master;		/* Master device table		*/

time_t	Current_Time;			/* Current time			*/

int	CatSpacePercent(struct CatalogEntry *ce);
void	itemize_disk(dev_ent_t *, char *, char *, char *, int);
void	itemize_jukebox(dev_ent_t *);
int	finddev(char *);
extern	char *time_string(time_t, time_t, char *);
extern	char *device_to_nm(int);


int
main(int argc, char **argv)
{
	char		*file	= NULL;	/* File  identifier filter	*/
	char		*owner	= NULL;	/* Owner identifier filter	*/
	char		*group	= NULL;	/* Group identifier filter	*/
	int		twoline	= 0;	/* Two line output flag		*/
	shm_ptr_tbl_t	*shm_ptr_tbl;	/* SM device pointer table	*/
	dev_ent_t	*dev;		/* Device entry			*/
	int		i;

	Current_Time = time(0L);

	CustmsgInit(0, NULL);

	if (argc < 2) {
		fprintf(stderr, catgets(catfd, SET, 13001,
		"Usage: %s %s\n"), program_name,
		"[-file|f f..f] [-owner|u u..u] [-group|g g..g] [-2] eq");
		exit   (-1);
	}


	/* Access master/device shared memory segment.			*/
	shm_master.shmid  = shmget(SHM_MASTER_KEY, 0, 0);
	if (shm_master.shmid < 0)
		error(1, errno,
		    catgets(catfd, SET, 2593,
		    "Unable to find master shared memory segment."),
		    0);

	shm_master.shared_memory  =
	    shmat(shm_master.shmid, (void *)NULL, SHM_RDONLY);
	if (shm_master.shared_memory == (void *)-1)
		error(1, errno,
		    catgets(catfd, SET, 2556,
		    "Unable to attach master shared memory segment"),
		    0);

	shm_ptr_tbl  = (shm_ptr_tbl_t *)shm_master.shared_memory;

	Dev_Tbl	= (dev_ptr_tbl_t *)
	    SHM_ADDR(shm_master,  shm_ptr_tbl->dev_table);
	Max_Devices = Dev_Tbl->max_devices;


	/* Process arguments.						*/

	while (--argc > 0) {
		if (**++argv == '-') {
			if (strcmp (*argv, "-file") == 0) {
				file  = *++argv;
				--argc;
				continue;
			}
			if (strcmp (*argv, "-f") == 0) {
				file  = *++argv;
				--argc;
				continue;
			}
			if (strcmp (*argv, "-owner") == 0) {
				owner = *++argv;
				--argc;
				continue;
			}
			if (strcmp (*argv, "-u") == 0) {
				owner = *++argv;
				--argc;
				continue;
			}
			if (strcmp (*argv, "-group") == 0) {
				group = *++argv;
				--argc;
				continue;
			}
			if (strcmp (*argv, "-g") == 0) {
				group = *++argv;
				--argc;
				continue;
			}
			if (strcmp (*argv, "-2") == 0) {
				twoline = 1;
				continue;
			}
		}
		else
			break;
	}

	if (argc == 0)
		error(1, 0,
		    catgets(catfd, SET, 881, "Device not specified."), 0);

	if ((i = finddev (*argv)) < 0)	/* Find device		*/
		error(1, 0,
		    catgets(catfd, SET, 875, "Device not found (%s)"),
		    *argv);

	dev = (dev_ent_t *)SHM_ADDR(shm_master, Dev_Tbl->d_ent[i]);

	if ((dev->type & DT_CLASS_MASK)  == DT_OPTICAL)
		itemize_disk(dev, file, owner, group, twoline);

	else if (((dev->type & DT_CLASS_MASK) == DT_ROBOT) |
	    (dev->type == DT_HISTORIAN)) {
		if (dev->dt.rb.name[0] == '\0')
			error(1, 0,
			    catgets(catfd, SET, 2874,
			    "VSN catalog not present."),
			    0);
		itemize_jukebox(dev);
	}

	else {
		error(1, 0,
		    catgets(catfd, SET, 861, "Device (%s) cannot be itemized."),
		    *argv);
	}

	return (0);
}


/*
 * ----	itemize_disk - Catalog optical disk.
 */


void
itemize_disk(dev, file, owner, group, twoline)

	dev_ent_t	*dev;		/* Device entry			*/
	char		*file;		/* File  identifier filter	*/
	char		*owner;		/* Owner identifier filter	*/
	char		*group;		/* Group identifier filter	*/
	int		twoline;	/* Two line output flag		*/

{
	uint_t		ptocfwa;	/* First sector of PTOC		*/
	uint_t		ptoclwa;	/* Last  sector of PTOC		*/
	uint_t		sector;		/* Current sector		*/
	uint_t		sectsz;		/* sector size in bytes		*/
	uint16_t	version;
	uint32_t	byte_length;
	ls_bof1_label_t	*l;		/* Label buffer			*/
	int		len;		/* Read length			*/
	offset_t	addr;		/* Disk byte address		*/
	char		*buffer;	/* Sector buffer		*/
	int		disk;		/* Disk file descriptor		*/
	int		i;

	if ((disk = open(dev->name, O_RDONLY)) < 0)
	    error(1, errno, "open(%s)", dev->name);

	sectsz  = dev->sector_size;
	ptocfwa = dev->dt.od.ptoc_fwa;
	ptoclwa = dev->dt.od.ptoc_lwa;
	buffer  = malloc(sectsz);
	l	= (ls_bof1_label_t *)buffer;

	if (buffer == NULL)
		error(1, 0,
		    catgets(catfd, SET, 1652, "Memory exhausted"),
		    0);

	for (sector = ptocfwa; sector < ptoclwa; sector++) {

	    memset(buffer, 0, sectsz);
	    addr = (offset_t)sector * sectsz;

	    if (llseek(disk, addr, SEEK_SET) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 1548, "llseek: sector %.8x (%d)"),
		    sector, sector);
		continue;
	    }

	    errno = 0;
	    len = read(disk, buffer, sectsz);

	    if (errno != 0) {			/* read error	*/
		error(0, errno,
		    catgets(catfd, SET, 2023, "read: sector %.8x (%d)"),
		    sector, sector);
		continue;
	    }

	    if (errno == 0 && len == 0) {	/* empty sector	*/
		error(0, 0,
		    catgets(catfd, SET, 997, "Empty sector %.8x (%d)"),
		    sector, sector);
		continue;
	    }

	    if (len != sectsz) {		/* read short	*/
		error(0, 0,
		    catgets(catfd, SET, 2023, "read: sector %.8x (%d)"),
		    sector, sector);
		error(0, 0,
		    catgets(catfd, SET, 2015, "Read incomplete, len=%x (%d)"),
		    len, len);
		continue;
	    }

	    if (strncmp(l->label_id, "EO", 2) != 0 || l->label_version != 1)
		continue;

	    l->resc = l->rese = l->resf = '\0';
	    for (i = 30; i >= 0; i--) {
		    if (l->file_id[i] != ' ') break;
		    l->file_id[i] = '\0';
	    }
	    for (i = 30; i >= 0; i--) {
		    if (l->owner_id[i] != ' ') break;
		    l->owner_id[i] = '\0';
	    }
	    for (i = 30; i >= 0; i--) {
		    if (l->group_id[i] != ' ') break;
		    l->group_id[i] = '\0';
	    }
	    if (file  != NULL && strcasecmp(file, l->file_id)  != 0)
		    continue;
	    if (owner != NULL && strcasecmp(owner, l->owner_id) != 0)
		    continue;
	    if (group != NULL && strcasecmp(group, l->group_id) != 0)
		    continue;
		BE32toH(&l->byte_length, &byte_length);
		BE16toH(&l->version, &version);
	    if (twoline) {
		printf("%-32s %10u %-12s\n%-32s %10u %-12s\n",
			l->file_id, version, l->owner_id,
			" ", byte_length, l->group_id);
	    } else {
		printf("%-32s %6u %10u %-12s %s\n",
			l->file_id, version, byte_length,
			l->owner_id, l->group_id);
	    }
	}

	close(disk);
}


/*
 * ----	itemize_jukebox - Catalog robot device.
 *
 *
 *	Description:
 *	    Prints the contents of the vsn catalog for the
 *	    selected robotic device.
 *
 *	On entry:
 *	    dev		= Device entry for the robot.
 */


void
itemize_jukebox(dev)

	dev_ent_t	*dev;		/* Device entry			*/

{
	int		pct;		/* Usage percentage		*/
	int		i, n_entries;
	char		time_str[40];	/* Buffer for time formating	*/
	struct CatalogEntry *list;

	if (CatalogInit("itemize") == -1) {
		printf(catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		printf("\n");
		exit(1);
	}

	list = CatalogGetEntriesByLibrary(dev->eq, &n_entries);

	printf(catgets(catfd, SET, 2142,
	    "Robot VSN catalog: eq: %d\tcount: %d\n"),
	    dev->eq, n_entries);
	printf(catgets(catfd, SET, 13016,
	    "slot    access_time  count  use  ty vsn\n"));


	for (i = 0; i < n_entries; i++) {
		struct CatalogEntry *ce;

		ce = &list[i];

	    if (!(ce->CeStatus & CES_inuse)) {
			pct = 0;
		} else {
			pct = CatSpacePercent(ce);
		}

	    printf("%4d    %-12s %5d %3d%% %s %-32s",
			ce->CeSlot,
			time_string(ce->CeMountTime, Current_Time, time_str),
			ce->CeAccess,
			pct,
			ce->CeMtype,
			(ce->CeStatus & CES_labeled) ? ce->CeVsn : "nolabel");

		if (ce->CeStatus & CES_inuse) {
			if ((ce->CeStatus & CES_labeled) &&
			    (*ce->CeVsn == '\0')) {
				printf(catgets(catfd, SET, 2880,
				    " VSN MISSING"));
			}

			if ((!(ce->CeStatus & CES_occupied)) &&
			    *ce->CeVsn != '\0') {
				printf(catgets(catfd, SET, 2365,
				    " SLOT VACANT"));
			}

			if (ce->CeStatus & CES_needs_audit) {
				printf(catgets(catfd, SET, 1737,
				    " NEEDS AUDIT"));
			}

			if (ce->CeStatus & CES_bad_media) {
				printf(catgets(catfd, SET, 1634,
				    " MEDIA ERROR"));
			}
		}

	    printf("\n");
	}
	free(list);
}

/*
 * ----	finddev - Find device.
 *
 *	Description:
 *	    Finddev locates a specified device either by name or by
 *	    ordinal.
 *
 *	On entry:
 *	    name	= The name or the ordinal of the device to search
 *			  for.  If a name is specified, it may be the full
 *			  name or the basename of the device.
 *
 *	Returns:
 *	    The equipment number of the device.  -1 if the device
 *	    is not found, or the ordinal is non-existent.
 *
 *	Externals:
 *	    Dev_Tbl	= Device pointer table.
 *
 */


int
finddev(name)
	char		*name;		/* Name or ordinal of device	*/

{
	dev_ent_t	*dev;
	int		i;

	if (isdigit (*name)) {
	    i = atoi(name);
	    if ((i <= 0) || (i > Max_Devices))
		return (-1);
	    return (Dev_Tbl->d_ent[i] != NULL ? i : -1);
	}

	for (i = 0; i <= Max_Devices; i++) {
	    if (Dev_Tbl->d_ent[i] == NULL)
		continue;
	    dev = (dev_ent_t *)SHM_ADDR(shm_master, Dev_Tbl->d_ent[i]);
	    if (strcmp(name, dev->name) == 0)
		return (i);
	    if (strcmp(name, basename(dev->name)) == 0)
		return (i);
	}

	return (-1);
}


/*
 * Returns percent(cp->capacity - cp->space) of cp->capacity, rounded up.
 * Adjust for labels if magneto-optical.
 */
int
CatSpacePercent(struct CatalogEntry *ce)
{
	double p;
	uint_t c = ce->CeCapacity;
	uint_t s = ce->CeSpace;

	if (is_optical(sam_atomedia(ce->CeMtype))) {
		c = 2*ce->m.CePtocFwa - ce->CeCapacity;
	}

	if (c == 0 || c == s) {
		return (0);
	}

	p = (double)100*(((double)c-(double)s)/(double)c)+0.5;

	return ((int)p);
}
