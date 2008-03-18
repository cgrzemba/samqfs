/*
 * diskvols.h - Disk archiving table definitions.
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

#if !defined(_AML_DISKVOLS_H)
#define	_AML_DISKVOLS_H

#pragma ident "$Revision: 1.26 $"

#define	DISKVOLS_FILENAME "diskvols.db"	/* Disk volume database names */
#define	DISKVOLS_VSN_DBNAME "vsn"
#define	DISKVOLS_CLI_DBNAME "cli"
#define	DISKVOLS_HDR_DBNAME "hdr"

/* Disk volume database version key and value. */
#define	DISKVOLS_VERSION_KEY	"version "	/* Key is eight chars */
#define	DISKVOLS_VERSION_VAL	460
#define	DISKVOLS_VERSION_R45	450

/*
 *	Common flags.
 */
#define	DISKVOLS_VSN_DICT 1			/* Disk volumes dictionary */
#define	DISKVOLS_CLI_DICT 2			/* Trusted clients dictionary */
#define	DISKVOLS_HDR_DICT 3			/* Dictionary header */

#define	DISKVOLS_SEQNUM_FILENAME "diskvols"
#define	DISKVOLS_SEQNUM_SUFFIX	"seqnum"

#define	DISKVOLS_SEQNUM_MAGIC	0x000045C2
#define	DISKVOLS_SEQNUM_MAGIC_RE 0xC2450000	/* Reverse-endian magic num */
#define	DISKVOLS_SEQNUM_MAX_VALUE 4294967295U

/* Disk volume library function error return codes. */
#define	DISKVOLS_PATH_ERROR	(-1)
#define	DISKVOLS_PATH_UNAVAIL	(-2)
#define	DISKVOLS_PATH_ENOENT	(-3)

/* Honeycomb configuration resource name */
#define	HONEYCOMB_RESOURCE_NAME	"stk5800"

/* Honeycomb device name */
#define	HONEYCOMB_DEVNAME		"StorageTek 5800"

/* Honeycomb SAM-FS metadata names */
#define	HONEYCOMB_NUM_METADATA		3
#define	HONEYCOMB_METADATA_ARCHID	"com.sun.samfs.archiveId"
#define	HONEYCOMB_METADATA_FILENAME	"com.sun.samfs.fileName"
#define	HONEYCOMB_METADATA_MODTIME	"com.sun.samfs.modTime"

/* Honeycomb metadata name for size of an object */
#define	HONEYCOMB_METADATA_OBJECT_SIZE	"system.object_size"

/* Max size for honeycomb metadata query string */
#define	HONEYCOMB_METADATA_QUERYLEN	256

/* Max length of honeycomb metadata string type */
#define	HONEYCOMB_METADATA_STRINGLEN	512

/* Honeycomb default blocksize */
#define	HONEYCOMB_DEFAULT_BLOCKSIZE	16372

/* Max number of open honeycomb session per daemon */
#define	HONEYCOMB_MAX_OPEN_SESSIONS 10

/* Definition of disk volume flags. */
enum {
	DV_labeled	= 1 << 0,	/* Volume label, seqnum file created */
	DV_remote	= 1 << 1,	/* Volume is defined on a remote host */
	DV_unavail	= 1 << 2,	/* User set volume unavailable */
	DV_read_only	= 1 << 3,	/* User set volume read only */
	DV_bad_media	= 1 << 4,	/* Volume is unusable */
	DV_needs_audit	= 1 << 5,	/* Need audit to calculate space used */
	DV_archfull	= 1 << 6	/* Archiver found volume full */
};
#define	DISKVOLS_IS_HONEYCOMB(x) ((x->DvMedia == DT_STK5800) ? B_TRUE : B_FALSE)
#define	DISKVOLS_IS_DISK(x) ((x->DvMedia == DT_DISK) ? B_TRUE : B_FALSE)

/* Definition of DiskVolsIsAvail() caller. */
enum DVA_caller {
	DVA_archiver	= 1 << 0,	/* Archiver */
	DVA_recycler	= 1 << 1,	/* Recycler */
	DVA_stager	= 1 << 2,	/* Stager */
	DVA_other	= 1 << 3	/* Other */
};

/* Definition for numof() types to count. */
enum DV_numof {
	DV_numof_all		= 1 << 0,	/* All */
	DV_numof_disk		= 1 << 1,	/* Disk */
	DV_numof_honeycomb	= 1 << 2	/* Honeycomb */
};

/* Definitions of open flags. */
enum {
	DISKVOLS_CREATE		= 1 << 0,	/* Create db */
	DISKVOLS_RDONLY		= 1 << 1	/* Read only */
};

typedef struct DiskVolumeInfo DiskVolumeInfo_t;
typedef struct DiskVolumeInfoR45 DiskVolumeInfoR45_t;
typedef int64_t DiskVolumeSeqnum_t;
typedef int DiskVolumeVersionVal_t;
typedef char DiskVolumeVersionKey_t[8];

/*
 * Disk volume information.
 */
struct DiskVolumeInfo {
	media_t		DvMedia;	/* Media type */
	host_t		DvHost;		/* If remote, host name */
	fsize_t		DvSpace;	/* Space available */
	fsize_t		DvCapacity;	/* Total space allocated */
	host_t		DvAddr;		/* Addr/host name of Honeycomb server */
	int		DvPort;		/* Optional Honeycomb server port */
	uint_t		DvFlags;	/* Volume attribute flags */
	long long	unused1;
	long long	unused2;
	long long	unused3;
	long long	unused4;
	int		DvPathLen;	/* Length of path name */
	char		DvPath[1];	/* Path name */
};

/*
 * Disk volume information changed in 4.6.
 * For upgrade, define the previous disk volume information.
 * Read only.
 */
struct DiskVolumeInfoR45 {
	host_t		DvHost;		/* If remote, host name */
	fsize_t		DvSpace;	/* Space available */
	fsize_t		DvCapacity;	/* Total space allocated */
	uint_t		DvFlags;	/* Volume attribute flags */
	int		DvPathLen;	/* Length of path name */
	char		DvPath[1];	/* Path name */
};


/*
 * Disk volume sequence number file.
 */
struct DiskVolumeSeqnumFile {
	uint32_t	DsMagic;
	char		pad[4];			/* AMD packing */
	DiskVolumeSeqnum_t	DsVal;		/* Current sequence number */
	uint_t		DsFlags;		/* Attribute flags */
	int		unused1;
	DiskVolumeSeqnum_t	DsRecycle;	/* Recycle sequence number */
	fsize_t 	DsUsed;			/* Space used */
	long long	unused3;
	long long	unused4;
};

typedef struct DiskVolsDictionary {
	void		*dbfile;	/* Database handle */
	int		dbtype;		/* Database type */

	int (*Open)(struct DiskVolsDictionary *, int);
	int (*Get)(struct DiskVolsDictionary *, char *,
		struct DiskVolumeInfo **);
	int (*GetOld)(struct DiskVolsDictionary *, char *,
		struct DiskVolumeInfo **, DiskVolumeVersionVal_t);
	int (*GetVersion)(struct DiskVolsDictionary *,
		DiskVolumeVersionVal_t **);
	int (*Put)(struct DiskVolsDictionary *, char *,
		struct DiskVolumeInfo *);
	int (*PutVersion)(struct DiskVolsDictionary *,
		DiskVolumeVersionVal_t *);
	int (*Del)(struct DiskVolsDictionary *, char *);
	int (*Close)(struct DiskVolsDictionary *);

	int (*BeginIterator)(struct DiskVolsDictionary *);
	int (*GetIterator)(struct DiskVolsDictionary *, char **,
		struct DiskVolumeInfo **);
	int (*EndIterator)(struct DiskVolsDictionary *);

	int (*Numof)(struct DiskVolsDictionary *, int *, enum DV_numof);
} DiskVolsDictionary_t;


/*
 * Define prototypes.
 */
struct DiskVolsDictionary *DiskVolsNewHandle(char *progname, int dbtype,
    int flags);
struct DiskVolsDictionary *DiskVolsGetHandle(int dbtype);
int DiskVolsDeleteHandle(int dbtype);

int DiskVolsInit(struct DiskVolsDictionary **dict, int dbytype,
    char *progname);
int DiskVolsDestroy(struct DiskVolsDictionary *dict);
void DiskVolsTrace(struct DiskVolsDictionary *dict, char *srcFile, int srcLine);

/*
 * Convenience functions.
 */
void DiskVolsUnlink();
void DiskVolsRecover();
char *DiskVolsGetHostname(struct DiskVolumeInfo *dv);
boolean_t DiskVolsIsAvail(char *volname, struct DiskVolumeInfo *dv,
	boolean_t offlineFiles, enum DVA_caller caller);
int DiskVolsLabel(char *volname, struct DiskVolumeInfo *dv, void *rftd);
int DiskVolsGenFileName(DiskVolumeSeqnum_t val, char *name, int nameSize);
DiskVolumeSeqnum_t DiskVolsGenSequence(char *name);
fsize_t DiskVolsOfflineFiles(char *path);
fsize_t DiskVolsAccumSpaceUsed(char *path);
int DiskVolsGetSpaceUsed(char *volname, struct DiskVolumeInfo *dv,
	void *connectedRftd, fsize_t *spaceUsed);
char *DiskVolsGenMetadataQuery(char *volname, DiskVolumeSeqnum_t seqnum,
	char *queryBuf);
char *DiskVolsGenMetadataArchiveId(char *volname, DiskVolumeSeqnum_t seqnum,
	char *archiveId);

#endif /* !defined(_AML_DISKVOLS_H) */
