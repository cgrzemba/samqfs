/*
 * archset.h - Archive set table definitions.
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

#if !defined(_AML_ARCHSET_H)
#define	_AML_ARCHSET_H

#pragma ident "$Revision: 1.36 $"

/* Macros. */
#define	ARCHSETS_MAGIC 01222230524	/* Archive sets file magic number */
#define	ARCHSETS_VERSION 80410		/* Archive sets file version (YMMDD) */

#define	ALL_SETS "allsets"		/* Name of defaults archive set */
#define	NO_ARCHIVE "no_archive"		/* Name of "no_archive" archive set */
#define	PR_MAX FLT_MAX			/* Maximum priority */
#define	PR_MIN (-3.4E+38F)		/* Minimum priority */
					/* (bigger than -FLT_MAX) */
#define	PR_FMT "%-.6G"			/* Priority printf() format */

/* Copy number from Archive Set name */
#define	AS_COPY(n) (*(strchr(n, '.')+1)-'1')

/* Types. */
typedef char OwnerName_t[49];	/* Owner name for VSN reservation */
typedef float Priority_t;	/* Scheduling priority */
typedef char set_name_t[33];	/* Archive set name 29 characters and ".nR" */
typedef uint_t vsndesc_t;	/* VSN descriptor */

/*
 * Definition of archive request join methods.
 * The files in an archive request in arfind will be joined together into
 * archive files using one of the join methods.
 * The default is to join files into archive files that are less than
 * archmax in size.
 */
enum JoinMethods { JM_none,
	JM_path,			/* Join by path */
	JM_max
};

/* Definition of offline copy methods. */
enum OfflineCopyMethods {
	OC_none,
	OC_direct,			/* Copy from drive to drive */
	OC_ahead,			/* Stage ahead one archive file */
	OC_all,				/* Stage all files before archiving */
	OC_max
};

/* Definition of reserve VSN methods.  */
#define	RM_none  0
#define	RM_set   0x0001		/* Use archive set */
#define	RM_owner 0x0002		/* Use file owner */
#define	RM_fs    0x0004		/* Use file system */
#define	RM_dir   0x0008		/* Owner is directory name */
#define	RM_user  0x0010		/* Owner is user id */
#define	RM_group 0x0020		/* Owner is group id */

/*
 * Definition of sort methods.
 * The order of files in an archive file is controlled by the sort method.
 * The default is no sorting.
 */
enum SortMethods { SM_none,
	SM_age,				/* Ascending by age */
	SM_path,			/* Ascending by path */
	SM_priority,			/* Descending by priority */
	SM_size,			/* Ascending by size */
	SM_rage,			/* Descending by age */
	SM_rpath,			/* Descending by path */
	SM_rpriority,			/* Ascending by priority */
	SM_rsize,			/* Descending by size */
	SM_max
};

/* Structures. */

/*
 * Archive sets.
 * The archive set definitions are an array of structures.
 * Entry 0 is for 'allsets'.
 * Entries 1 - 4 are for 'allsets.1' - 'allsets.4'.
 */
struct ArchSet {
	set_name_t	AsName;		/* Archive set name */
	fsize_t		AsArchmax;	/* Target maximum size of */
					/* archive file */
	fsize_t		AsDrivemax;	/* Maximum size for multiple drives */
	fsize_t		AsDrivemin;	/* Minimum size for multiple drives */
	fsize_t		AsOvflmin;	/* Minimum archive file size to */
					/* overflow volume */
	fsize_t		AsStartSize;	/* Size of ArchReq to start archiving */
	fsize_t		AsFillvsnsmin;	/* Min space for fillvsns volumes */

	int		AsBufsize;	/* Buffer size * device blksize */
	int		AsStartCount;	/* Files in ArchReq to */
					/* start archiving */
	uint32_t	AsFlags;
	uint32_t	AsCflags;	/* Command processing flags */
	uint32_t	AsEflags;	/* Execution control flags */
	uint32_t	AsQueueTime; 	/* Queue time limit */
	uint_t		AsStartAge;	/* Age of ArchReq to start archiving */
	enum OfflineCopyMethods
			AsOlcm;		/* Offline copy method */
	enum JoinMethods
			AsJoin;		/* File join method for archive file */
	enum SortMethods
			AsSort;		/* File sort method for archive file */
	short		AsDrives;	/* Maximum number of drives to use */
	short		AsRearchStageCopy; /* Copy to stage from when */
						/* rearchiving */
	short		AsReserve;	/* VSN reserve methods */
	mtype_t		AsMtype;	/* Media type to use */
	vsndesc_t	AsVsnDesc;	/* VSN descriptor */

	/* Priorities */
	uint32_t	AsPrFlags;	/* Priority flags */
	Priority_t	AsPrAge;	/* Archive age */
	Priority_t	AsPrArch_im;	/* Archive immediate */
	Priority_t	AsPrArch_ld;	/* Archive VSN loaded */
	Priority_t	AsPrArch_ovfl;	/* Multiple archive VSNs */
	Priority_t	AsPrC1;		/* Copy 1 */
	Priority_t	AsPrC2;		/* Copy 2 */
	Priority_t	AsPrC3;		/* Copy 3 */
	Priority_t	AsPrC4;		/* Copy 4 */
	Priority_t	AsPrCopies;	/* Copies made */
	Priority_t	AsPrOffline;	/* Files off line */
	Priority_t	AsPrQueuewait;	/* Queue wait */
	Priority_t	AsPrRearch;	/* Rearchive */
	Priority_t	AsPrReqrel;	/* Required for release */
	Priority_t	AsPrSize;	/* File size */
	Priority_t	AsPrStage_ld;	/* Stage VSN loaded */
	Priority_t	AsPrStage_ovfl;	/* Multiple stage VSNs */


	/* Recycling */
	uint32_t	AsRyFlags;	/* Recycling flags */
	fsize_t		AsRyDataquantity; /* Max amount of data to */
						/* mark rearch */
	int		AsRyHwm;	/* High-water-mark value */
	char		AsRyMailaddr[32]; /* Mail address */
	int		AsRyMingain;	/* Min-gain value */
	int		AsRyVsncount;	/* Max number of VSNs to rearch */
	int		AsRyMinobs;	/* Min-obsolete value for disk arch */

	/* Debugging */
	uint32_t	AsDbgFlags;	/* Debugging flags */
	uint_t		AsDbgSimDelay;	/* Simulated archiving */
					/* interblock delay */
	uint_t		AsDbgSimEod;	/* Percentage of space to */
					/* simulate EOD */
};

/* Definition of flags. */
/* AsCflags - only used during archiver.cmd processing. */
#define	AC_defVSNs	0x00000001	/* VSNs assigned by default */
#define	AC_needVSNs	0x00000002	/* Archive set needs VSNs */
#define	AC_reArch	0x00000004	/* Rearchive Archive Set */

/* AsEflags - used for execution control */
#define	AE_directio	0x00000001	/* Use directio */
#define	AE_fillvsns	0x00000002	/* Fill vsns */
#define	AE_lockbuf	0x00000004	/* Lock archive buffers */
#define	AE_tapenonstop	0x00000008	/* Use tapenonstop */
#define	AE_unarchage	0x00000010	/* Unarchage uses modify time */

/* AsFlags - used during archiver.cmd processing and execution. */
#define	AS_archmax	0x00000001	/* -archmax */
#define	AS_bufsize	0x00000002	/* -bufsize */
#define	AS_directio	0x00000004	/* -directio */
#define	AS_disk_archive	0x00000008	/* Disk archive */
#define	AS_drivemax	0x00000010	/* -drivemax */
#define	AS_drivemin	0x00000020	/* -drivemin */
#define	AS_drives	0x00000040	/* -drives */
#define	AS_fillvsns	0x00000080	/* -fillvsns */
#define	AS_join		0x00000100	/* -join */
#define	AS_lockbuf	0x00000200	/* -lock */
#define	AS_olcm		0x00000400	/* -offline_copy */
#define	AS_ovflmin	0x00000800	/* -ovflmin */
#define	AS_priority	0x00001000	/* -priority */
#define	AS_queue_time	0x00002000	/* -queue_time_limit */
#define	AS_rearchstc	0x00004000	/* -rearch_stage_copy */
#define	AS_reserve	0x00008000	/* -reserve */
#define	AS_rsort	0x00010000	/* -rsort */
#define	AS_sort		0x00020000	/* -sort */
#define	AS_startage	0x00040000	/* -startage */
#define	AS_startcount	0x00080000	/* -startcount */
#define	AS_startsize	0x00100000	/* -startsize */
#define	AS_tapenonstop	0x00200000	/* -tapenonstop */
#define	AS_unarchage	0x00400000	/* -unarchage */
#define	AS_honeycomb	0x00800000	/* Honeycomb archive */

#define	AS_rmonly (AS_fillvsns | AS_ovflmin | AS_reserve | AS_tapenonstop)
#define	AS_diskArchSet	(AS_disk_archive | AS_honeycomb)

/* AsPrFlags - priority definition flags. */
#define	ASPR_age	0x00000001	/* Archive age */
#define	ASPR_archi	0x00000002	/* Archive immediate */
#define	ASPR_arch_ld	0x00000004	/* Archive VSN loaded */
#define	ASPR_arch_ovfl	0x00000008	/* Multiple archive VSNs */
#define	ASPR_c1		0x00000010	/* Copy 1 */
#define	ASPR_c2		0x00000020	/* Copy 2 */
#define	ASPR_c3		0x00000040	/* Copy 3 */
#define	ASPR_c4		0x00000080	/* Copy 4 */
#define	ASPR_copies	0x00000100	/* Copies made */
#define	ASPR_offline	0x00000200	/* Files off line */
#define	ASPR_queuewait	0x00000400	/* Queue wait */
#define	ASPR_rearch	0x00000800	/* Rearchive */
#define	ASPR_reqrel	0x00001000	/* Required for release */
#define	ASPR_size	0x00002000	/* File size */
#define	ASPR_stage_ld	0x00004000	/* Stage VSN loaded */
#define	ASPR_stage_ovfl	0x00008000	/* Multiple stage VSNs */

/* Recycle definition flags. */
#define	ASRY_dataquantity 0x00000001	/* -recycle_dataquantity */
#define	ASRY_hwm	0x00000002	/* -recycle_hwm */
#define	ASRY_ignore	0x00000004	/* -recycle_ignore */
#define	ASRY_mailaddr	0x00000008	/* -recycle_mail address */
#define	ASRY_mingain	0x00000010	/* -recycle_mingain */
#define	ASRY_vsncount	0x00000020	/* -recycle_vsncount */
#define	ASRY_minobs	0x00000040	/* -recycle_minobs */

/* Debugging definition flags. */
/* Disabled in archiver.cmd processing if DEBUG is not defined. */
#define	ASDBG_simdelay	0x00000001	/* -simdelay */
#define	ASDBG_simeod	0x00000002	/* -simeod */
#define	ASDBG_simread	0x00000004	/* -simread */
#define	ASDBG_tstovfl	0x00000008	/* -tstovfl */

/*
 * Definition of VSN descriptor.
 * For execution, VsnDesc can be the only descriptor or the offset to a list
 * of descriptors.  Many archive sets have only one VSN descriptor.
 * The most significant bits identify which table the descriptor is in.
 */
#define	VD_mask  0xF0000000
#define	VD_none  0x00000000
#define	VD_list  0x10000000	/* descriptor list */
#define	VD_exp   0x20000000	/* regular expression */
#define	VD_pool  0x30000000	/* pool */


/* Functions. */
struct ArchSet *FindArchSet(char *name);
struct ArchSet *ArchSetAttach(int mode);
fsize_t GetArchmax(struct ArchSet *as, char *mtype);
struct MediaParamsEntry *MediaParamsGetEntry(mtype_t mtype);

#if defined(ARCHIVER_PRIVATE)
/* Archive sets file. */
struct ArchSetFileHdr {
	MappedFile_t	As;
	int		AsVersion;	/* Version */
	int		ArchSetTable;	/* Offset to Archive Set Table */
	int		ArchSetNumof;	/* Number of entries */
	int		VsnExpTable;	/* Offset to VSN expressions table */
	int		VsnExpSize;
	int		VsnListTable;	/* Offset to VSN list table */
	int		VsnListSize;
	int		VsnPoolTable;	/* Offset to VSN pool table */
	int		VsnPoolSize;
	int		AsMediaParams;	/* Offset to mmedia parameters */
} *ArchSetFile;


/*
 * Vsn expression.
 * Vsn expressions are forward linked together.  They form a "literal" pool.
 */
struct VsnExp {
	int	VeExplen;	/* Length of regular expression */
	char	VeExpbuf[1];	/* regular expression (dynamic array) */
};


/*
 * VSN descriptor list table.
 */
struct VsnList {
	int		VlNumof;	/* Number of descriptors */
	vsndesc_t	VlDesc[1];	/* VSN descriptors (dynamic array) */
};

/*
 * VSN pool table entry.
 * Each VSN pool is described in the VSN pool table.
 */
struct VsnPool {
	char	VpName[17];	/* Name of pool */
	mtype_t	VpMtype;	/* Media type */
	int	VpNumof;	/* Number of VSN expressions */
	int	VpVsnExp[1];	/* Offsets in VsnExp table (dynamic array) */
};

/*
 * Media parameters.
 * Media dependent characteristics.
 * The first entry is used for non-specified media - default.
 * The remaining entries parallel devnm_t.
 */
struct MediaParams {
	int	MpCount;

	/* sam-arcopy timeout values */
	int	MpReadTimeout;		/* Reading file being archived */
	int	MpRequestTimeout;	/* Media request */
	int	MpStageTimeout;		/* Staging file being archived */
	struct MediaParamsEntry {
		mtype_t	MpMtype;	/* Media type name */
		media_t	MpType;		/* Media type */
		uint_t	MpFlags;
		fsize_t	MpArchmax;	/* Target max size of archive file */
		fsize_t	MpOvflmin;	/* Min size file for volume overflow */
		int	MpBufsize;	/* Size of archive buffer * device */
					/* blksize */
		int	MpTimeout;	/* Timeout for writing file being */
					/* archived */
	} MpEntry[1];			/* First entry of 'MpCount' entries */
};

/* Flags */
#define	MP_avail	0x01	/* Media is available */
#define	MP_default	0x02	/* Site default media */
#define	MP_volovfl	0x04	/* Volume overflow allowed */
#define	MP_refer	0x08	/* Media was referenced */
#define	MP_lockbuf	0x10	/* Archive buffer mlocked */

/* Timeout codes */
enum Timeouts { TO_none,
	TO_read,			/* Reading file being archived */
	TO_request,			/* Media request */
	TO_stage,			/* Staging file being archived */
	TO_write,			/* Write to media */
	TO_max
};

/* Archive set table */
DCL struct ArchSet *ArchSetTable IVAL(NULL);
DCL int ArchSetNumof IVAL(0);

/* VSN information table */
DCL struct VsnExp *VsnExpTable IVAL(NULL);
DCL size_t VsnExpSize IVAL(0);

/* VSN descriptor lists table */
DCL struct VsnList *VsnListTable IVAL(NULL);
DCL size_t VsnListSize IVAL(0);

/* VSN pool table */
DCL struct VsnPool *VsnPoolTable IVAL(NULL);
DCL int VsnPoolSize IVAL(0);

/* Media parameters */
DCL struct MediaParams *MediaParams IVAL(NULL);

#if defined(NEED_ARCHSET_NAMES)
#include "sam/setfield.h"

/* Archive Set parameter names. */

static struct EnumEntry Joins[] = {
	{ "none",	JM_none },
	{ "path",	JM_path },
	{ NULL }
};

static struct EnumEntry OfflineCopies[] = {
	{ "none",	OC_none },
	{ "direct",	OC_direct },
	{ "stageahead",	OC_ahead },
	{ "stageall",	OC_all },
	{ NULL }
};

static struct {
	char	*RsName;
	int	RsValue;
} Reserves[] = {
	{ "none",	RM_none },
	{ "set",	RM_set },
	{ "dir",	RM_dir },
	{ "user",	RM_user },
	{ "group",	RM_group },
	{ "fs",		RM_fs },
	{ "" }
};

static struct EnumEntry Rsorts[] = {
	{ "none",	SM_none },
	{ "age",	SM_rage },
	{ "path",	SM_rpath },
	{ "priority", SM_rpriority },
	{ "size",	SM_rsize },
	{ NULL }
};

static struct EnumEntry Sorts[] = {
	{ "none",	SM_none },
	{ "age",	SM_age },
	{ "path",	SM_path },
	{ "priority", SM_priority },
	{ "size",	SM_size },
	{ NULL }
};

static struct EnumEntry Timeouts[] = {
	{ "read", TO_read },
	{ "request", TO_request },
	{ "stage", TO_stage },
	{ "write", TO_write },
	{ NULL }
};

#endif /* defined(NEED_ARCHSET_NAMES) */

#endif /* defined(ARCHIVER_PRIVATE) */

#endif /* !defined(_AML_ARCHSET_H) */
