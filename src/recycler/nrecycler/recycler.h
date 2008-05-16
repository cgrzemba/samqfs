/*
 * recycler.h - Recycler definitions.
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

#if !defined(RECYCLER_H)
#define	RECYCLER_H

#define	RECYCLER_DIRNAME	"nrecycler"
#define	RECYCLER_CMD_FILNAME	"nrecycler.cmd"
#define	RECYCLER_SCRIPT_FILNAME "nrecycler.sh"
#define	RECYCLER_LOCK_FILNAME	"nrecycler.lock"

#define	RECYCLER_HOMEDIR	SAM_VARIABLE_PATH"/"RECYCLER_DIRNAME
#define	RECYCLER_LOCK_PATH	SAM_VARIABLE_PATH"/"RECYCLER_LOCK_FILNAME
#define	RECYCLER_SCRIPT_PATH	SAM_SCRIPT_PATH"/"RECYCLER_SCRIPT_FILNAME

#define	DAT_FILE_SUFFIX		".SUNWsamfs"
#define	DAT_FILE_MAGIC		0x2659250
#define	DAT_FILE_VERSION	0x060701			/* YYMMDD */

/* Parameters for media table. */
#define	TABLE_INCREMENT 	1000

/* Parameters for media hash table. */
#define	HASH_MULTIPLIER		10
#define	HASH_SLOP		13
#define	HASH_INCREMENT		(TABLE_INCREMENT * HASH_MULTIPLIER)
#define	HASH_INITIAL_SIZE	(HASH_INCREMENT + HASH_SLOP)
#define	HASH_EMPTY		-1

/* Rounding for disk volume seqnum bit map */
#define	BITMAP_CHUNK		2048

/*
 * Maximum number of sequence numbers per filesystem/dump to sort in one
 * pass.  A larger number will use more memory but result in fewer scans.
 */
#define	SEQNUM_CHUNK		100000

/* Number of file system csd table entries to allocate. */
#define	CSD_TABLE_INCREMENT 	100

#define	INODES_IN_PROGRESS	500000

/* Maximum number of threads to create for accumulating inodes. */
#define	CREW_MAXSIZE	8

/*
 * Additional trace debug flags to be enabled when code is under development.
 */
#if 0
#define	TR_samdev (TR_debug)
#endif
#define	TR_samdev (TR_none)
#define	TR_SAMDEV TR_samdev, _SrcFile, __LINE__


/* Macros for manipulating a bit mask used as flags. */
#define	GET_FLAG(flags, bit)	(((flags & bit) != 0) ? B_TRUE : B_FALSE)
#define	SET_FLAG(flags, bit)	(flags |= bit)
#define	CLEAR_FLAG(flags, bit)	(flags &= ~bit)

/* Macro for setting global error which stop nrecycler from completing. */
#define	CANNOT_RECYCLE() CannotRecycle = B_TRUE;

#define	FATAL_EXIT() 			\
	Log(20402);			\
	exit(EXIT_FATAL);

/*
 * Media entry.
 */
typedef struct MediaEntry {
	media_t 	me_type;	/* media type */
	vsn_t 		me_name;	/* volume name */
	/*
	 * Removable media information.
	 */
	dev_ent_t	*me_dev;	/* samfs dev_t entry for library */
	uint32_t	me_slot;	/* storage slot in library */
	uint16_t	me_part;	/* storage partition in library */
	time_t		me_label;	/* time volume was labeled */

	ushort_t	me_flags;	/* media state flags */
	int		me_files;	/* count of files archived to */
					/*   this media */
	pthread_mutex_t	me_mutex;	/* protect access to active file */
					/*    count and disk volume bit map */
	DiskVolumeSeqnum_t me_maxseqnum; /* max sequence num for disk volume */
	size_t		me_mapsize;	/* size of bit map in each disk */
					/*    vsn entry */
	ulong_t		*me_bitmap;	/* disk volume sequence number */
					/*    bit map */
} MediaEntry_t;

/*
 * Media entry flags.
 */
enum {
	ME_noexist =	1 << 0,		/* VSN does not exist in SAM. */
					/* The nrecycler may discover media */
					/* that is not defined in current */
					/* catalog/dictionary. */
	ME_ignore =	1 << 1,		/* Ignore this VSN for recycling */
	ME_recycle =	1 << 2		/* VSN is recycling */
};


/*
 * Media table.
 */
typedef struct MediaTable {
	char		*mt_name;		/* table name for debugging */
	int		*mt_hashTable;
	int		mt_hashSize;
	int		mt_tableUsed;
	int		mt_tableAvail;
	int		*mt_hashPermute;
	ushort_t	mt_flags;
	pthread_mutex_t	mt_mutex;
	MediaEntry_t	*mt_data;		/* media entries */
	/*
	 * Set flag if only accumulating sequence number for disk archives.
	 * Removable media is accumulated in first pass over the inodes.
	 */
	boolean_t	mt_diskArchive;	/* only accumulated disk archives */
	/*
	 *	Applies to bitmaps for all disk volume entries.
	 */
	int		mt_mapchunk;	/* number of sequence number entries */
	DiskVolumeSeqnum_t mt_mapmin;	/* min sequence number in bitmap */
} MediaTable_t;

/*
 * Media table flags.
 */
enum {
	MT_diskarchive =	1 << 0	/* disk archiving configured */
};


/*
 * Samfs dump entry.
 */
typedef struct CsdEntry {
	char		*ce_path;	/* path to samfs dump file */
	boolean_t	ce_exists;	/* set true if valid dat file exists */
	boolean_t	ce_skip;	/* set true if skipping file */
	char		*ce_datPath;	/* path to dat file */
	int		ce_fd;		/* open file descriptor for dat file */
	MediaTable_t	*ce_table;	/* media table generated for dat file */
} CsdEntry_t;

/*
 * Samfs dump directory.  A directory may contain multiple samfs dump
 * files.
 */
typedef struct CsdDir {
	size_t		cd_count;	/* count of samfs dump files in */
					/*    directory */
	size_t		cd_alloc;
	CsdEntry_t	cd_entry[1];	/* dump entries for directory */
} CsdDir_t;

/*
 * All samfs dump directories.
 */
typedef struct CsdTable {
	size_t		ct_count;	/* number of dump directories */
	CsdDir_t	**ct_data;	/* entry for each dump directory */
} CsdTable_t;

/*
 * Samfs dump file descriptor
 */
typedef struct CsdFildes {
	int	flags;		/* file descriptor flags */
	gzFile	inf;		/* zlib inflate/decompression descriptor */
} CsdFildes_t;

/*
 * File descriptor flags.
 */
enum {
	CSD_none =	1 << 0,		/* none */
	CSD_inflate =	1 << 1		/* zlib inflate on read */
};

/*
 * Samfs dump dat file is arranged as follows:
 * struct datfile_header
 *   struct datfile_table
 *     struct vsn_entries
 *   struct datfile_table
 *     struct vsn_entries
 */
typedef struct DatHeader {
	int	dh_magic;	/* dat file magic number */
	int	dh_version;	/* dat file version */
	time_t	dh_ctime;	/* time of dump file's last status change */
	/*
	 * PID of the process writing to this file.  If 0, then i/o has
	 * completed and file is valid.
	 */
	pid_t	dh_pid;
} DatHeader_t;

typedef struct DatTable {
	int		dt_count;	/* number of vsn entries */
	int		dt_mapchunk;	/* number of sequence number entries */
	DiskVolumeSeqnum_t dt_mapmin;	/* min and max sequence number in bit */
} DatTable_t;

typedef struct ScanArgs {
	int		pass;
	void	*data;
} ScanArgs_t;

/*
 * Queued items of work for the crew. One is queued by
 * crew_start, and each worker may queue additional items.
 */
typedef struct WorkItem {
	struct WorkItem	*wi_next;	/* next work item */
	void	*(*wi_func)(void *);	/* work function */
	void	*wi_arg;		/* arguments to work function */
} WorkItem_t;

/*
 * One of these is initialized for each worker thread in the
 * crew.
 */
typedef struct Worker {
	pthread_t	wo_thread;	/* thread id for worker */
	struct Crew	*wo_crew;	/* pointer to crew */
} Worker_t;

/*
 * The external "handle" for a work crew. Contains the
 * crew synchronization state and staging area.
 */
typedef struct Crew {
	int		cr_count;	/* count of work items */
	WorkItem_t	*cr_first;	/* first and last work item */
	WorkItem_t	*cr_last;

	pthread_mutex_t	cr_mutex;	/* mutex for crew data */
	pthread_cond_t	cr_done;	/* wait for crew done */
	pthread_cond_t	cr_go;		/* wait for work */

	int		cr_size;	/* size of worker array */
	Worker_t	cr_workers[CREW_MAXSIZE]; /* crew worker threads */
} Crew_t;

typedef struct log_tag {
	FILE *file;
} log_t;

/*
 * Define prototypes in cmd.c
 */
void CmdReadfile();
char *CmdGetLogfilePath();
char *CmdGetDumpPath(int idx);
char *CmdGetScriptPath();

/*
 * Define prototypes in crew.c
 */
int CrewCreate(Crew_t *crew, int size);
void CrewCleanup(Crew_t *crew);

/*
 * Define prototypes in csd.c
 */
CsdDir_t *CsdInit(char *dirname);
void CsdScan(Crew_t *crew, CsdTable_t *fsdump_table, int num_dumps, int pass,
	void *(*worker)(void *arg));
void *CsdAccumulate(void *arg);
void CsdSetSeqnum(CsdTable_t *fsdump_table, DiskVolumeSeqnum_t min,
	boolean_t diskArchive);
void CsdCleanup(CsdTable_t *fsdump_table);
void CsdAssembleName(char *path, char d_name[1], char *buf, size_t buflen);

/*
 * Define prototypes in dat.c
 */
void DatInit(CsdDir_t *fsdump_dir);
int DatAccumulate(CsdEntry_t *csd);
int DatWriteHeader(CsdEntry_t *csd, pid_t pid);
int DatWriteTable(CsdEntry_t *csd);

/*
 * Define prototypes in device.c
 */
void DeviceInit();
dev_ent_t *DeviceGetHead();
char *DeviceGetFamilyName(dev_ent_t *dev);
void DeviceGetMediaType(dev_ent_t *dev, mtype_t *mtype);
equ_t DeviceGetEq(dev_ent_t *dev);
char *DeviceGetTypeMnemonic(dev_ent_t *dev);

/*
 * Define prototypes in fs.c
 */
struct sam_fs_info *FsInit(int *numFs);
void FsScan(Crew_t *crew, struct sam_fs_info *firstFs, int numFs,
	int pass, void *(*worker)(void *arg));
int FsInodeHandle(union sam_di_ino *inode, MediaTable_t *table,
						int pass);
void *FsAccumulate(void *arg);
void FsCleanup(struct sam_fs_info *firstFs, int numFs);

/*
 * Define prototypes in log.c
 */
int LogOpen(char *path);
void Log(int msgnum, ...);
void LogClose();

/*
 * Define prototypes in media.c
 */
int MediaInit(MediaTable_t *table, char *name);
MediaEntry_t *MediaFind(MediaTable_t *table, media_t media, char *vsn);
DiskVolumeSeqnum_t MediaGetSeqnum(MediaTable_t *table);
void MediaSetSeqnum(MediaTable_t *table, DiskVolumeSeqnum_t min,
	boolean_t diskArchive);
void MediaDebug(MediaTable_t *table);

#endif /* RECYCLER_H */
