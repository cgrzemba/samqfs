/*
 * recycler.h - recycler volumes of removable media.
 *
 * Structure definitions and function prototypes.
 *
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

/*
 * $Revision: 1.36 $
 */

#ifndef _RECYCLER_H
#define	_RECYCLER_H


/* Solaris headers */
#include <sys/param.h>
#include <string.h>

/* SAM-FS headers */
#include "sam/defaults.h"
#include "sam/param.h"
#include "aml/archiver.h"
#include "aml/shm.h"
#include "sam/fs/ino.h"
#include "pub/rminfo.h"
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"
#include "sam/lint.h"
#include "aml/diskvols.h"

#include "include/recycler_lib.h"

/* Types */

#define	IS_DISK_MEDIA(media) (media == DT_DISK || media == DT_STK5800)
#define	IS_DISK_HONEYCOMB(media) (media == DT_STK5800)
#define	IS_DISK_FILESYS(media) (media == DT_DISK)


/* Parameters */
#define	RY_CMD "recycler.cmd"	/* Name of command file */
#define	RECYCLER_LOCK_FILE SAM_VARIABLE_PATH"/recycler.pid"
#define	IOCTL_NAME "/.ioctl"	/* SAM-FS file name for ioctl() access */
#define	S2ABUFSIZE 128		/* Size of buffer s2a() needs */
#define	RY_CLASS "Recycler"	/* Recycler Class name used by sysevent */

int cannot_recycle;		/* can we continue? */
FILE *log;

#define	TO_TTY	1
#define	TO_SYS	2
#define	TO_FILE	4
#define	TO_ALL	TO_TTY|TO_SYS|TO_FILE

typedef struct {

	/* Information read from command file */

	struct CatalogEntry *chtable;	/* catalog table - catalog entries */
					/* for this robot */
	int	chtable_numof;		/* number of entries in catalog table */
	int	cat_file_size;		/* the byte size of the catalog file */

	/*
	 * Disk volume dictionary elements
	 */
	struct {
		size_t	entries;	/* number of elements in use */
		size_t	alloc;		/* number of allocated elements */
		vsn_t	*key;		/* disk volume name */
		DiskVolumeInfo_t **data; /* disk volume dictionary element */
	} diskvols;

	dev_ent_t	*dev;		/* robot device */
	boolean_t	archset;	/* set TRUE if archive set recycling */
	long		asflags;	/* archive set flags */
	long long	free_up;	/* free up this # of blocks to get */
					/*    below thresh */
	int		high;		/* high threshold of usage in */
					/*   whole robot */
	int		min;		/* min threshold per VSN recycled */
	char		name[MAXPATHLEN]; /* path name of robot's catalog */
	int		needs_work;	/* this robot above HWM */
	long long	total_space;	/* sum of space for all vsns */
	long long	total_capacity;	/* sum of capacity for all vsns */
	int		total_vsns;	/* number of VSNs in this robot */
	fsize_t		total_written;
	fsize_t		total_active;

	/* Options from recycler.cmd */

	long long	dataquantity;
	int		vsncount;
	int		ignore;
	int		obs;
	boolean_t	limit_quantity;
	boolean_t	limit_vsncount;
	int		mail;
	char		*mailaddress;
} ROBOT_TABLE;

/* Mingain defines, defaults determined by media capacity at runtime */
#define	DEFAULT_MIN_GAIN -1
#define	MIN_GAIN_SM_MEDIA 60
#define	MIN_GAIN_LG_MEDIA 90
#define	MIN_GAIN_MEDIA_THRSH 214748364800LL /* 200 GB */

#define	DEFAULT_MIN_OBS  50
#define	DEFAULT_ROBOT_HWM 95
#define	DEFAULT_DATAQUANTITY 1073741824 /* 1 gigabyte */
#define	DEFAULT_VSNCOUNT	1

/*
 * The following defines the number of catalog or disk dictionary entries
 * allocated to archset robots at creation and when necessary during expansion.
 */
#define	CATALOG_CHUNKSIZE	10
#define	DICT_CHUNKSIZE		10


/* Robot table:	*/

int ROBOT_count;		/* Number of robots on this system */
ROBOT_TABLE *ROBOT_table;	/* Robot table */


typedef struct VSN_TABLE {		/* VSN table: */
	float		alpha;		/* accounts for compression */
	int		active_files;	/* count of files in filesystem */
					/*    on VSN */
	unsigned int	blocksize;	/* block size on this vsn */
	long long	capacity;	/* capacity from catalog */

	struct CatalogEntry *chtable;	/* catalog table - catalog entries */
					/*    for robot this VSN in */
	struct CatalogEntry *ce;	/* actual catalog entry for this VSN */
	DiskVolumeSeqnum_t maxSeqnum;	/* max disk archive seqnum to recycle */
	int		disk_archsets;	/* number of archsets belong to */

	long		count;		/* file count */
	dev_ent_t	*dev;		/* assigned robot device */
	int		free;		/* free percentage */
	int		good;		/* good percentage */
	int		junk;		/* junk percentage */
	time_t		label_time;	/* time the VSN was labeled */
	dev_ent_t	*real_dev;	/* real robot device */
	ROBOT_TABLE	*robot;		/* robot table entry for this vsn */
	media_t		media;		/* media type */
	long long	size;		/* total file size, bytes */
	int		slot;		/* the catalog entry for this VSN */
	long long	space;		/* apace from catalog */
	fsize_t		written;	/* amount of data written to disk VSN */
	vsn_t		vsn;		/* volume serial name */
	ushort_t			/* flags */
		duplicate	:1,	/* is duplicated */
		has_active_files:1,	/* has any files in a filesystem */
		has_request_files:1,	/* is named in "request" files */
		has_noarchive_files:1,	/* holds archive images for */
					/*    archive -n files */
		in_robot	:1,	/* is in a robot */
		is_read_only	:1,	/* is flagged in catalog as r/o */
		is_recycling	:1,	/* is marked "recycle" in catalog */
		needs_recycling	:1,	/* needs to be be recycled */
		no_recycle	:1,	/* matches a regexp in a no_recycle */
					/*    section */
		was_reassigned	:1,	/* reassigned from a real to archset */
		multiple_arsets	:1,	/* is in multiple tape archive sets */
		candidate	:1,	/* if candidate to recycle but */
					/*   ignore set */
		log_action	:1;	/* if disk recycling and logging */
					/*    removes */
} VSN_TABLE;

#ifdef HASH_DEBUG
#undef TABLE_INCREMENT
#define	TABLE_INCREMENT	4
#endif

int table_used;		/* Number of VSNs currently in table(s) */
int table_avail;	/* Number of table entries allocated */
struct VSN_TABLE *vsn_table;	/* VSN table */
int *vsn_permute;	/* VSN permutation list, see sort_vsns() */

/* Hash table used for quick vsn table lookup */
HashTable_t *hashTable;

/* Macros */
#define	errtext strerror(errno) ? strerror(errno) : "(unknown error number)"

/* no_recycle, a linked list of regular expressions found in recycler.cmd */
struct no_recycle {
	char *regexp;	/* regular expression for VSNs to not be recycled */
	int  medium;		/* media type */
	struct no_recycle *next; /* next expression */
};
extern struct no_recycle *no_recycle;  /* head of no_recycle list */

boolean_t display_selection_logic;
boolean_t check_expired;
boolean_t display_draining_vsns;
boolean_t suppress_catalog;
boolean_t catalog_summary_only;

/* Functions */
void assign_vsn(int robot_index, VSN_TABLE *VSN);
void dump_hash_stats(void);
void emit(int where, int priority, int msgno, ...);
char *family_name(ROBOT_TABLE *Robot);
VSN_TABLE *Find_VSN(media_t media, char *vsn);
char *id_to_path(char *, sam_id_t);
void Init(void);
void Init_fs(void);
void Init_shm(void);
void MapCatalogs(void);
char *product_id(ROBOT_TABLE *Robot);
void PrintCatalogs(void);
void PrintDiskArchsets(void);
void PrintVsns(int copy);
void Readcmd(void);
char *s2a(unsigned long long);
void ScanCatalogs(void);
char *vendor_id(ROBOT_TABLE *Robot);
int create_robot(int archive_set_index);
void InitTrace(void);
char *GetScript(void);
FILE *Mopen(char *destination);
void Msend(FILE **mail_file, char *destination, char *subject);

/* Prototypes for functions in disk_archive.c */
void AssignDiskVol(int robot, VSN_TABLE *vsn);
void RecycleDiskArchives(void);
int strerror_r(int errnum, char *strerrbuf, size_t buflen);


#define	cemit if (display_selection_logic) emit
#define	CATALOG_LIST_ON() (!suppress_catalog)

/* Shared data */

/* Declaration/initialization macros */
#undef CON_DCL
#undef DCL
#undef IVAL
#if defined(DEC_INIT)
#define	CON_DCL
#define	DCL
#define	IVAL(v) = v
#else /* defined(DEC_INIT) */
#define	CON_DCL extern const
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(DEC_INIT) */

DCL sam_defaults_t *defaults;

/* Flags and common data */
DCL struct sam_fs_info *first_fs;
DCL int num_fs;

/* archiver headers */
#define	ARCHIVER_PRIVATE
#include "aml/archset.h"
#undef ARCHIVER_PRIVATE

struct ArchSetFileHdr *afh;

#endif /* _RECYCLER_H */
