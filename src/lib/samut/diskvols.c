/*
 * diskvols.c - Disk volume functions.
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.47 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

/* OS headers. */
#ifdef linux
#include <mntent.h>
#else
#include <sys/mnttab.h>
#endif /* linux */

/* SAM-FS headers. */
#ifdef linux
#include <sam/linux_types.h>
#endif /* linux */
#include <sam/custmsg.h>
#include <aml/device.h>
#include <aml/diskvols.h>
#include <sam/lib.h>
#include <aml/sam_rft.h>
#include <sam/sam_trace.h>
#include <sam/sam_malloc.h>
#include <sam/fs/ino_ext.h>
#include <sam/dbfile.h>

#include "../src/fs/include/bswap.h"

/* Private data. */
static pthread_mutex_t diskvolsMutex = PTHREAD_MUTEX_INITIALIZER;
/*
 * We keep a count on the number of connection to each DB.
 * This count is incremented if handle is created successfuly in
 * DiskVolsNewHandle and decremented in DiskVolsDeleteHandle.
 * DiskVolsDeleteHandle will destroy the handle only if the
 * count becomes zero.
 */
static int connections[3] = {0, 0, 0};
static DiskVolsDictionary_t *hdrDict = NULL;
static DiskVolsDictionary_t *vsnDict = NULL;
static DiskVolsDictionary_t *cliDict = NULL;
static upath_t pathname;

/* Private functions. */
static int diskVolsOpen(struct DiskVolsDictionary *dict, int flags);
static int diskVolsGet(struct DiskVolsDictionary *dict, char *vsn,
	struct DiskVolumeInfo **data);
static int diskVolsGetOld(struct DiskVolsDictionary *dict, char *vsn,
	struct DiskVolumeInfo **data, DiskVolumeVersionVal_t val);
static int diskVolsGetVersion(struct DiskVolsDictionary *dict,
	DiskVolumeVersionVal_t **val);
static int diskVolsPut(struct DiskVolsDictionary *dict, char *key,
	struct DiskVolumeInfo *data);
static int diskVolsPutVersion(struct DiskVolsDictionary *dict,
	DiskVolumeVersionVal_t *val);
static int diskVolsDel(struct DiskVolsDictionary *dict, char *key);
static int diskVolsBeginIterator(struct DiskVolsDictionary *dict);
static int diskVolsGetIterator(struct DiskVolsDictionary *dict,  char **vsn,
	struct DiskVolumeInfo **data);
static int diskVolsEndIterator(struct DiskVolsDictionary *dict);
static int diskVolsNumof(struct DiskVolsDictionary *dict, int *numof,
	enum DV_numof type);
static int diskVolsClose(struct DiskVolsDictionary *dict);

static boolean_t isValidSeqnumFile(char *volname, struct DiskVolumeInfo *dv,
	void *sam_rft);
static char *getMountPnt(char *name, char *mountPnt, int mountPntSize);
static int xatoi(char *cp);
static int statPath(SamrftImpl_t *rftd, char *path, struct stat *sb);

#define	IS_HDR(x) ((x)->dbtype == DISKVOLS_HDR_DICT)
#define	IS_VSN(x) ((x)->dbtype == DISKVOLS_VSN_DICT)
#define	IS_CLI(x) ((x)->dbtype == DISKVOLS_CLI_DICT)
#define	IS_INIT(x) ((x) != NULL && (x)->dbfile != NULL &&  \
		(IS_HDR(x) || IS_VSN(x) || IS_CLI(x)))

#ifdef sun
/*
 * Called by daemons to construct a disk volume, VSN or client, dictionary
 * handle.  If successful, the dictionary handle will be saved in static
 * variables.
 */
DiskVolsDictionary_t *
DiskVolsNewHandle(
	char *progname,
	int dbtype,
	int flags)
{
	int ret;
	DiskVolsDictionary_t *dict = NULL;

	pthread_mutex_lock(&diskvolsMutex);
	if (dbtype == DISKVOLS_VSN_DICT) {
		ASSERT(vsnDict == NULL);
		dict = vsnDict;

		if (dict == NULL) {
			ret = DiskVolsInit(&vsnDict, dbtype, progname);
			if (ret == 0) {
				ret = vsnDict->Open(vsnDict, flags);
			}
			if (ret != 0) {
				Trace(TR_DEBUG,
				    "diskvols.config (vsn) initialization "
				    "failed, errno: %d", errno);
				(void) vsnDict->Close(vsnDict);
				(void) DiskVolsDestroy(vsnDict);
				vsnDict = NULL;
			}
			dict = vsnDict;
		}

	} else if (dbtype == DISKVOLS_CLI_DICT) {
		ASSERT(cliDict == NULL);
		dict = cliDict;

		if (dict == NULL) {
			ret = DiskVolsInit(&cliDict, dbtype, progname);
			if (ret == 0) {
				ret = cliDict->Open(cliDict, flags);
			}
			if (ret != 0) {
				Trace(TR_DEBUG,
				    "diskvols.config (cli) initialization "
				    " failed, errno: %d", errno);
				(void) cliDict->Close(cliDict);
				(void) DiskVolsDestroy(cliDict);
				cliDict = NULL;
			}
			dict = cliDict;
		}
	} else if (dbtype == DISKVOLS_HDR_DICT) {
		ASSERT(hdrDict == NULL);
		dict = hdrDict;

		if (dict == NULL) {
			ret = DiskVolsInit(&hdrDict, dbtype, progname);
			if (ret == 0) {
				ret = hdrDict->Open(hdrDict, flags);
			}
			if (ret != 0) {
				Trace(TR_DEBUG,
				    "diskvols.config (hdr) initialization "
				    "failed, errno: %d", errno);
				(void) hdrDict->Close(hdrDict);
				(void) DiskVolsDestroy(hdrDict);
				hdrDict = NULL;
			}
			dict = hdrDict;
		}
	} else {
		dict = NULL;
	}

	if (dict != NULL) {
		connections[(dbtype - 1)]++;
	}
	pthread_mutex_unlock(&diskvolsMutex);

	return (dict);
}

/*
 * Get disk volume, VSN or client, dictionary handle.
 */
DiskVolsDictionary_t *
DiskVolsGetHandle(
	int dbtype)
{
	DiskVolsDictionary_t *dict = NULL;

	pthread_mutex_lock(&diskvolsMutex);
	if (dbtype == DISKVOLS_VSN_DICT) {
		dict = vsnDict;
	} else if (dbtype == DISKVOLS_CLI_DICT) {
		dict = cliDict;
	}
	if (dict != NULL) {
		connections[(dbtype - 1)]++;
	}
	pthread_mutex_unlock(&diskvolsMutex);

	return (dict);
}

/*
 * Release the Dictionary Handle. Decrement the connection count,
 * call DiskVolsDeleteHandle if this is the last connection.
 */
void
DiskVolsRelHandle(int dbtype)
{
	pthread_mutex_lock(&diskvolsMutex);
	if (connections[(dbtype - 1)] > 1) {
		connections[(dbtype - 1)]--;
	} else if (connections[(dbtype - 1)] == 1) {
		/* Last reference. Delete the handle */
		pthread_mutex_unlock(&diskvolsMutex);
		(void) DiskVolsDeleteHandle(dbtype);
		return;
	} else {
		/* Should not happen. Reset connection to zero */
		connections[(dbtype - 1)] = 0;
	}
	pthread_mutex_unlock(&diskvolsMutex);
}


/*
 * Delete disk volume, VSN or client, dictionary handle.
 */
int
DiskVolsDeleteHandle(
	int dbtype)
{
	pthread_mutex_lock(&diskvolsMutex);
	if (connections[(dbtype - 1)] <= 0) {
		connections[(dbtype - 1)] = 0;
		goto out;
	}

	if (--connections[(dbtype - 1)] == 0) {
		if (dbtype == DISKVOLS_VSN_DICT && vsnDict != NULL) {
			(void) vsnDict->Close(vsnDict);
			(void) DiskVolsDestroy(vsnDict);
			vsnDict = NULL;

		} else if (dbtype == DISKVOLS_CLI_DICT && cliDict != NULL) {
			(void) cliDict->Close(cliDict);
			(void) DiskVolsDestroy(cliDict);
			cliDict = NULL;

		} else if (dbtype == DISKVOLS_HDR_DICT && hdrDict != NULL) {
			(void) hdrDict->Close(hdrDict);
			(void) DiskVolsDestroy(hdrDict);
			hdrDict = NULL;
		}
	}
out:
	pthread_mutex_unlock(&diskvolsMutex);

	return (0);
}

/*
 * Initialize dictionary access.
 */
int
DiskVolsInit(
	DiskVolsDictionary_t **dict,
	int dbtype,
	char *progname)
{
	int ret;
	char *homedir;
	char *datadir;
	DBFile_t *dbfile;
	DiskVolsDictionary_t *new;

	homedir = SAM_CATALOG_DIR;
	datadir = SAM_CATALOG_DIR;

	ret = DBFileInit(&dbfile, homedir, datadir, progname);
	if (ret != 0) {
		if (dbfile != NULL) {
			dbfile->Close(dbfile);
			(void) DBFileDestroy(dbfile);
			return (ret);
		}
	}
	new = (DiskVolsDictionary_t *)malloc(sizeof (DiskVolsDictionary_t));
	if (new == NULL) {
		return (-1);
	}
	memset(new, 0, sizeof (DiskVolsDictionary_t));

	new->dbfile = (void *)dbfile;
	new->dbtype = dbtype;

	new->Open = diskVolsOpen;
	new->Get = diskVolsGet;
	new->GetOld = diskVolsGetOld;
	new->GetVersion = diskVolsGetVersion;
	new->Put = diskVolsPut;
	new->PutVersion = diskVolsPutVersion;
	new->Del = diskVolsDel;
	new->Close = diskVolsClose;

	new->BeginIterator = diskVolsBeginIterator;
	new->GetIterator  = diskVolsGetIterator;
	new->EndIterator = diskVolsEndIterator;

	new->Numof = diskVolsNumof;

	*dict = new;

	return (ret);
}

/*
 * Destroy dictionary access.
 */
int
DiskVolsDestroy(
	DiskVolsDictionary_t *dict)
{
	int ret;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	ret = DBFileDestroy(dbfile);
	if (ret == 0) {
		memset(dict, 0, sizeof (DiskVolsDictionary_t));
		free(dict);
	}

	return (ret);
}

/*
 *	Trace db stats.
 */
void
DiskVolsTrace(
	DiskVolsDictionary_t *dict,
	char *srcFile,
	int srcLine)
{
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return;
	}
	DBFileTrace(dbfile, srcFile, srcLine);
}

/*
 * Remove disk volume dictionary.
 */
void
DiskVolsUnlink(void)
{
	upath_t pathname;

	snprintf(pathname, sizeof (pathname), "%s/%s",
	    SAM_CATALOG_DIR, DISKVOLS_FILENAME);

	(void) unlink(pathname);
}

/*
 * Recover disk volume dictionary.
 */
void
DiskVolsRecover(
	char *progname)
{
	char *homedir;
	char *datadir;

	homedir = SAM_CATALOG_DIR;
	datadir = SAM_CATALOG_DIR;

	DBFileRecover(homedir, datadir, progname);
}

/*
 * Return the hostname for a volume.
 */
char *
DiskVolsGetHostname(
	struct DiskVolumeInfo *dv)
{
	if (*dv->DvHost != '\0') {
		return (dv->DvHost);
	}
	return ("localhost");
}

/*
 * Check if disk volume is available.  If volume is labeled, available means
 * diskvols.seqnum file exists.  If not labeled, existence is not checked
 * but if stager, existence is always checked.
 * Set space and capacity.  If offlineFiles flag is set, accumulate size of
 * offline disk archives for capacity (used by recycler).
 */
boolean_t
DiskVolsIsAvail(
	char *volname,
	struct DiskVolumeInfo *dv,
	boolean_t offlineFiles,
	enum DVA_caller caller)
{
	struct statvfs64 buf;
	int ret;
	void *sam_rft;
	fsize_t space;
	fsize_t capacity;
	boolean_t changed;
	boolean_t avail;
	boolean_t valid_seqnum;
	boolean_t check_existence;

	if (DISKVOLS_IS_HONEYCOMB(dv)) {
		return (B_TRUE);
	}

	sam_rft = (void *)SamrftConnect(dv->DvHost);
	if (sam_rft != NULL) {

		ret = 0;
		changed = B_FALSE;	/* modified diskvol dictionary entry */
		valid_seqnum = B_TRUE;
		check_existence = B_FALSE;

		/*
		 * Determine if checking existence of diskvols.seqnum file.
		 */
		if (caller == DVA_stager) {
			check_existence = B_TRUE;

		} else if (dv->DvFlags & DV_labeled) {
			check_existence = B_TRUE;

		} else if ((dv->DvFlags & DV_needs_audit) &&
		    (caller == DVA_archiver || caller == DVA_recycler)) {
			check_existence = B_TRUE;
		}

		if (check_existence == B_TRUE) {
			valid_seqnum = isValidSeqnumFile(volname, dv, sam_rft);
			if ((dv->DvFlags & DV_needs_audit) &&
			    (valid_seqnum == B_TRUE)) {
				dv->DvFlags &= ~DV_needs_audit;
				changed = B_TRUE;
			}
		}

		if (caller != DVA_stager) {
			if (valid_seqnum == B_TRUE) {
				ret = SamrftStatvfs(sam_rft, dv->DvPath,
				    offlineFiles, &buf);
			} else {
				ret = -1;
			}
		}
		SamrftDisconnect(sam_rft);

	} else {
		ret = -1;
		valid_seqnum = B_FALSE;
		SendCustMsg(HERE, 14090, dv->DvHost);
	}
	if (caller == DVA_stager) {
		return (valid_seqnum);
	}

	if (ret == 0) {
		avail = B_TRUE;

		space = (fsize_t)(buf.f_bfree * buf.f_frsize);
		capacity = (fsize_t)(buf.f_blocks * buf.f_frsize);

		if (space != dv->DvSpace || capacity != dv->DvSpace) {
			changed = B_TRUE;
			dv->DvSpace = space;
			dv->DvCapacity = capacity;
		}

	} else {
		dv->DvSpace = dv->DvCapacity = 0;
		return (B_FALSE);
	}

	if (changed == B_TRUE && vsnDict != NULL && caller == DVA_archiver) {
		(void) diskVolsPut(vsnDict, volname, dv);
	}

	return (avail);
}

/*
 * Label volume, write seqnum file.
 */
int
DiskVolsLabel(
	char *volname,
	struct DiskVolumeInfo *dv,
	void *connectedRftd)
{
	int ret;
	SamrftImpl_t *rftd;
	char *hostname;
	char seqnumPath[MAXPATHLEN + 4];
	struct stat pathStat;
	SamrftCreateAttr_t creat;
	int nbytes;
	int size;
	struct DiskVolumeSeqnumFile buf;
	boolean_t localConnect;
	char *filename;

	rftd = (SamrftImpl_t *)connectedRftd;
	hostname = DiskVolsGetHostname(dv);

	localConnect = B_FALSE;
	if (rftd == NULL) {
		rftd = SamrftConnect(hostname);
		if (rftd == NULL) {
			Trace(TR_ERR, "Samrft connection failed '%s'",
			    hostname);
			return (DISKVOLS_PATH_ERROR);
		}
		localConnect = B_TRUE;
	}

	/*
	 * The disk archive directory must exist.
	 */
	ret = statPath(rftd, dv->DvPath, &pathStat);
	if (ret != 0) {
		if (localConnect == B_TRUE) {
			SamrftDisconnect(rftd);
		}
		return (DISKVOLS_PATH_ENOENT);
	}

	filename = DISKVOLS_SEQNUM_FILENAME;
	if (DISKVOLS_IS_HONEYCOMB(dv)) {
		filename = volname;
	}
	snprintf(seqnumPath, sizeof (seqnumPath), "%s/%s.%s",
	    dv->DvPath, filename, DISKVOLS_SEQNUM_SUFFIX);

	ret = SamrftOpen(rftd, seqnumPath, O_RDWR, NULL);
	if (ret != 0) {
		/*
		 * Seqnum file does not exists.  If not labeled, create it.
		 */
		if (dv->DvFlags & DV_labeled) {
			if (localConnect == B_TRUE) {
				SamrftDisconnect(rftd);
			}
			return (DISKVOLS_PATH_UNAVAIL);
		}

		memset(&creat, 0, sizeof (SamrftCreateAttr_t));
		creat.mode = 0600;
		creat.uid = pathStat.st_uid;
		creat.gid = pathStat.st_gid;

		ret = SamrftOpen(rftd, seqnumPath,
		    O_RDWR | O_CREAT | O_EXCL, &creat);
		if (ret != 0) {
			/*
			 * Open failed.
			 */
			Trace(TR_ERR,
			    "Disk archive seqnum file open failed '%s', "
			    "errno= %d", seqnumPath, errno);

			if (localConnect == B_TRUE) {
				SamrftDisconnect(rftd);
			}

			return (DISKVOLS_PATH_ERROR);
		}

		size = sizeof (struct DiskVolumeSeqnumFile);
		memset(&buf, 0, size);
		buf.DsMagic = DISKVOLS_SEQNUM_MAGIC;
		buf.DsVal = -1;

		nbytes = SamrftWrite(rftd, &buf, size);
		if (nbytes != size) {
			Trace(TR_ERR,
			    "Disk archive seqnum file write failed '%s', "
			    "errno= %d", seqnumPath, errno);

			(void) SamrftClose(rftd);
			if (localConnect == B_TRUE) {
				SamrftDisconnect(rftd);
			}
			return (DISKVOLS_PATH_ERROR);
		}

		/*
		 * Disable archiving for the seqnum file.  Ignore errors
		 * since we may be trying to set this attribute on a
		 * diskvols.seqnum file thats not in a SAM-FS file system.
		 */
		(void) SamrftArchiveOp(rftd, seqnumPath, "n");

	}

	(void) SamrftClose(rftd);
	if (localConnect == B_TRUE) {
		SamrftDisconnect(rftd);
	}

	return (0);
}
#endif	/* sun */


/*
 * Generate file name from integer.
 * 256 files per directory.
 * 255 directories per directory (there is no 'd0').
 * E.g. 100      produces f100
 *      9100     produces d35/f140
 *      9863000  produces d150/d127/f88
 */
int
DiskVolsGenFileName(
	DiskVolumeSeqnum_t val,
	char *name,
	int nameSize)
{
	if (val & 0xff00000000000000ULL) {
		snprintf(name, nameSize, "d%d/d%d/d%d/d%d/d%d/d%d/d%d/f%d",
		    (int)(val >> 56) & 0xff, (int)(val >> 48) & 0xff,
		    (int)(val >> 40) & 0xff, (int)(val >> 32) & 0xff,
		    (int)(val >> 24) & 0xff, (int)(val >> 16) & 0xff,
		    (int)(val >> 8)  & 0xff, (int)val & 0xff);
	} else if (val & 0xff000000000000ULL) {
		snprintf(name, nameSize, "d%d/d%d/d%d/d%d/d%d/d%d/f%d",
		    (int)(val >> 48) & 0xff, (int)(val >> 40) & 0xff,
		    (int)(val >> 32) & 0xff, (int)(val >> 24) & 0xff,
		    (int)(val >> 16) & 0xff, (int)(val >> 8) & 0xff,
		    (int)val & 0xff);
	} else if (val & 0xff0000000000ULL) {
		snprintf(name, nameSize, "d%d/d%d/d%d/d%d/d%d/f%d",
		    (int)(val >> 40) & 0xff, (int)(val >> 32) & 0xff,
		    (int)(val >> 24) & 0xff, (int)(val >> 16) & 0xff,
		    (int)(val >> 8)  & 0xff, (int)val & 0xff);
	} else if (val & 0xff00000000ULL) {
		snprintf(name, nameSize, "d%d/d%d/d%d/d%d/f%d",
		    (int)(val >> 32) & 0xff, (int)(val >> 24) & 0xff,
		    (int)(val >> 16) & 0xff, (int)(val >> 8) & 0xff,
		    (int)val & 0xff);
	} else if (val & 0xff000000) {
		snprintf(name, nameSize, "d%d/d%d/d%d/f%d",
		    (int)(val >> 24) & 0xff, (int)(val >> 16) & 0xff,
		    (int)(val >> 8)  & 0xff, (int)val & 0xff);
	} else if (val & 0xff0000) {
		snprintf(name, nameSize, "d%d/d%d/f%d",
		    (int)(val >> 16) & 0xff, (int)(val >> 8) & 0xff,
		    (int)val & 0xff);
	} else if (val & 0xff00) {
		snprintf(name, nameSize, "d%d/f%d",
		    (int)(val >> 8) & 0xff, (int)val & 0xff);
	} else {
		snprintf(name, nameSize, "f%d", (int)val);
	}
	return (0);
}

/*
 * Generate sequence number from file name.
 * (inverse of DiskVolsGenFileName)
 * 256 files per directory.
 * 255 directories per directory (there is no 'd0').
 * E.g. f100           produces   100
 *      d35/f140       produces   9100
 *      d150/d127/f88  produces   9863000
 */
DiskVolumeSeqnum_t
DiskVolsGenSequence(
	char *name)
{
	DiskVolumeSeqnum_t seq;
	int64_t tmp;
	char *token;
	char *endptr;
	char *path;		/* temporary to break up tokens */

	seq = 0;
	path = strdup(name);
	token = strtok(path, "/");
	while (token && *token != 'f') {
		if (*token == 'd') {
			token++;
			tmp = strtoll(token, &endptr, 10);
			if (endptr == token || tmp == 0 || tmp > 255) {
				free(path);
				return (-1);
			}
			seq += tmp;
		} else {
			free(path);
			return (-1);
		}
		token = strtok(NULL, "/");
		seq <<= 8;
	}
	if (token && *token == 'f') {
		token++;
		tmp = strtoll(token, &endptr, 10);
		if (endptr == token || tmp > 255) {
			free(path);
			return (-1);
		}
		seq += tmp;
		free(path);
		return (seq);
	}
	free(path);
	return (-1);
}

#ifdef sun
/*
 * Accumulate size of offline disk archives.
 */
fsize_t
DiskVolsOfflineFiles(
	char *name)
{
	static union sam_di_ino *inodesBuffer = NULL;
	static size_t inodesBufferSize;

	int fd;
	ino_t inodeNumber;
	int bytesRead;
	long long numof;
	fsize_t size;
	upath_t mountPntPath;
	char *mountPnt;

	Trace(TR_DEBUG, "Accum size of offline disk archives volume: %s", name);

	mountPnt = getMountPnt(name, mountPntPath, sizeof (mountPntPath));

	Trace(TR_DEBUG, "\tmount point: %s", TrNullString(mountPnt));

	numof = 0;		/* Number of offline files */
	size = 0;		/* Accumulated size of offline files */

	if (mountPnt != NULL) {

		fd = OpenInodesFile(mountPnt);

		if (fd >= 0) {
			if (inodesBuffer == NULL) {
				inodesBufferSize = INO_BLK_SIZE *
				    INO_BLK_FACTOR;
				SamMalloc(inodesBuffer, inodesBufferSize);
			}

			inodeNumber = 0;

			while ((bytesRead = read(fd, inodesBuffer,
			    inodesBufferSize)) > 0) {

				union sam_di_ino *inodeInBuffer;
				struct sam_disk_inode *dinode;
				int fmode;

				inodeInBuffer = inodesBuffer;

				while (bytesRead > 0) {

					dinode = &inodeInBuffer->inode.di;
					inodeInBuffer++;
					inodeNumber++;
					bytesRead -= sizeof (union sam_di_ino);

					/*
					 * Ignore non-file inodes.
					 */
					if (dinode->mode == 0) {
						continue;
					}

					if (S_ISEXT(dinode->mode)) {
						continue;
					}

					if (dinode->id.ino != inodeNumber) {
						continue;
					}

					if (!(SAM_CHECK_INODE_VERSION(
					    dinode->version))) {
						continue;
					}

					Trace(TR_DEBUG, "inode: %d.%d "
					    "offline: %d size: %lld",
					    dinode->id.ino, dinode->id.gen,
					    dinode->status.b.offline, size);

					fmode = dinode->mode & S_IFMT;
					if ((fmode == S_IFREG) &&
					    dinode->status.b.offline) {

						numof++;
						size += dinode->rm.size;
					}
				}
			}
			close(fd);

		} else {
			Trace(TR_ERR, "Inodes file open failed mountpnt: %s "
			    "errno: %d", mountPnt, errno);
		}
	} else {
		Trace(TR_ERR, "Failed to get mountpnt volume: %s errno: %d",
		    name, errno);
	}

	Trace(TR_DEBUG, "Offline disk archives size: %lld numof: %lld",
	    size, numof);

	return (size);
}

/*
 * Get space used value for the disk volume.  The space used on a
 * disk volume is saved in the persistent disk volume's seqnum file.
 */
int
DiskVolsGetSpaceUsed(
	char *volname,
	struct DiskVolumeInfo *dv,
	void *connectedRftd,
	fsize_t *spaceUsed)
{
	int retval;
	SamrftImpl_t *rftd;
	char *hostname;
	char seqnumPath[MAXPATHLEN + 4];
	int nbytes;
	int size;
	struct DiskVolumeSeqnumFile buf;
	boolean_t localConnect, swapped = B_FALSE;
	char *filename;

	*spaceUsed = 0;

	rftd = (SamrftImpl_t *)connectedRftd;
	hostname = DiskVolsGetHostname(dv);

	localConnect = B_FALSE;
	if (rftd == NULL) {
		rftd = SamrftConnect(hostname);
		if (rftd == NULL) {
			Trace(TR_ERR, "Samrft connection failed '%s'",
			    hostname);
			return (DISKVOLS_PATH_ERROR);
		}
		localConnect = B_TRUE;
	}

	filename = DISKVOLS_SEQNUM_FILENAME;
	if (DISKVOLS_IS_HONEYCOMB(dv)) {
		filename = volname;
	}
	snprintf(seqnumPath, sizeof (seqnumPath), "%s/%s.%s",
	    dv->DvPath, filename, DISKVOLS_SEQNUM_SUFFIX);

	retval = SamrftOpen(rftd, seqnumPath, O_RDWR, NULL);

	if (retval == 0) {
		size = sizeof (struct DiskVolumeSeqnumFile);

		nbytes = SamrftRead(rftd, &buf, size);

		if (nbytes != size) {
			Trace(TR_ERR, "Disk volume seqnum file read failed");
			return (-1);
		}

		if (buf.DsMagic == DISKVOLS_SEQNUM_MAGIC_RE) {
			swapped = B_TRUE;
			sam_bswap8(&buf.DsUsed, 1);
		} else if (buf.DsMagic != DISKVOLS_SEQNUM_MAGIC) {
			Trace(TR_ERR, "Disk volume seqnum file magic number"
			    " invalid");
			return (-1);
		}

		*spaceUsed = buf.DsUsed;

		(void) SamrftClose(rftd);

		if (localConnect == B_TRUE) {
			SamrftDisconnect(rftd);
		}
	} else {
		/*
		 * Seqnum file does not exist.
		 */
		if (localConnect == B_TRUE) {
			SamrftDisconnect(rftd);
		}
		retval = DISKVOLS_PATH_UNAVAIL;
	}

	return (retval);
}

/*
 * Accumulate space used for selected path.
 */
fsize_t
DiskVolsAccumSpaceUsed(
	char *path)
{
	int rval;
	DIR *curdir;
	dirent64_t *entry;
	dirent64_t *entryp;
	char pathname[MAXPATHLEN];
	struct stat64 sbuf;
	fsize_t spaceUsed;

	spaceUsed = 0;

	curdir = opendir(path);
	if (curdir == NULL) {
		return (spaceUsed);
	}

	SamMalloc(entryp, sizeof (dirent64_t) + MAXPATHLEN + 1);

	/*
	 * Walk through all directory entries
	 */
	while ((rval = readdir64_r(curdir, entryp, &entry)) == 0) {
		if (entry == NULL) {
			break;
		}

		if ((strcmp(entry->d_name, ".") == 0) ||
		    (strcmp(entry->d_name, "..") == 0)) {
			continue;
		}

		snprintf(pathname, MAXPATHLEN, "%s/%s", path, entry->d_name);

		rval = stat64(pathname, &sbuf);

		if (rval != 0) {
			continue;	/* ignore files we cannot stat */
		}

		if (S_ISDIR(sbuf.st_mode)) {
			/*
			 * Directory, accumulate space allocated to a directory.
			 */
			spaceUsed += DiskVolsAccumSpaceUsed(pathname);
		}

		if (S_ISREG(sbuf.st_mode)) {
			/*
			 * Regular file, accumulate space allocated for file.
			 */
			spaceUsed += sbuf.st_size;
		}
	}

	SamFree(entryp);
	closedir(curdir);

	Trace(TR_DEBUG, "Path: '%s' size: %lld (%s)",
	    path, sbuf.st_size, StrFromFsize(spaceUsed, 3, NULL, 0));

	return (spaceUsed);
}


/*
 * Generate honeycomb metadata query string.
 */
char *
DiskVolsGenMetadataQuery(
	char *volname,
	DiskVolumeSeqnum_t seqnum,
	char *queryBuf)
{
	if (*volname == '\0') {
		return (NULL);
	}

	if (queryBuf == NULL) {
		SamMalloc(queryBuf, HONEYCOMB_METADATA_QUERYLEN);
	}

	snprintf(queryBuf, HONEYCOMB_METADATA_QUERYLEN,
	    "%s = '%s:%llx'", HONEYCOMB_METADATA_ARCHID, volname, seqnum);

	return (queryBuf);
}


/*
 * Generate honeycomb metadata archive id.
 */
char *
DiskVolsGenMetadataArchiveId(
	char *volname,
	DiskVolumeSeqnum_t seqnum,
	char *archiveId)
{
	if (*volname == '\0') {
		return (NULL);
	}

	if (archiveId == NULL) {
		SamMalloc(archiveId, HONEYCOMB_METADATA_QUERYLEN);
	}

	snprintf(archiveId, HONEYCOMB_METADATA_QUERYLEN,
	    "%s:%llx", volname, seqnum);

	return (archiveId);
}


/*
 * Open disk volume dictionary.
 */
static int
diskVolsOpen(
	DiskVolsDictionary_t *dict,
	int flags)
{
	int dbfile_flags;
	int ret;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	dbfile_flags = 0;
	if (flags & DISKVOLS_CREATE) {
		dbfile_flags |= DBFILE_CREATE;
	}
	if (flags & DISKVOLS_RDONLY) {
		dbfile_flags |= DBFILE_RDONLY;
	}

	if (IS_VSN(dict)) {
		ret = dbfile->Open(dbfile, DISKVOLS_FILENAME,
		    DISKVOLS_VSN_DBNAME, dbfile_flags);
	} else if (IS_CLI(dict)) {
		ret = dbfile->Open(dbfile, DISKVOLS_FILENAME,
		    DISKVOLS_CLI_DBNAME, dbfile_flags);
	} else if (IS_HDR(dict)) {
		ret = dbfile->Open(dbfile, DISKVOLS_FILENAME,
		    DISKVOLS_HDR_DBNAME, dbfile_flags);
	} else {
		ret = -1;
	}

	return (ret);
}

/*
 * Store disk volume in dictionary.
 */
static int
diskVolsPut(
	DiskVolsDictionary_t *dict,
	char *key,
	struct DiskVolumeInfo *data)
{
	int ret;
	size_t data_size;
	vsn_t vsn;
	host_t host;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	data_size = STRUCT_RND(sizeof (struct DiskVolumeInfo) +
	    data->DvPathLen);

	if (IS_VSN(dict)) {
		memset(&vsn, 0, sizeof (vsn_t));
		strncpy((char *)&vsn, key, sizeof (vsn_t));
		ret = dbfile->Put(dbfile, vsn, sizeof (vsn_t),
		    data, data_size, 0);
	} else if (IS_CLI(dict)) {
		memset(&host, 0, sizeof (host_t));
		strncpy((char *)&host, key, sizeof (host_t));
		ret = dbfile->Put(dbfile, host, sizeof (host_t),
		    data, data_size, 0);
	} else {
		ret = -1;
	}

	return (ret);
}

/*
 * Store disk volume version in dictionary.
 */
static int
diskVolsPutVersion(
	DiskVolsDictionary_t *dict,
	DiskVolumeVersionVal_t *val)
{
	int ret;
	DiskVolumeVersionKey_t versionKey;
	size_t data_size;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict) || !IS_HDR(dict)) {
		return (-1);
	}

	data_size = sizeof (DiskVolumeVersionVal_t);

	ASSERT(sizeof (DiskVolumeVersionKey_t) == strlen(DISKVOLS_VERSION_KEY));
	memcpy(versionKey, DISKVOLS_VERSION_KEY, sizeof (DiskVolumeVersionKey_t));

	ret = dbfile->Put(dbfile, versionKey, sizeof (DiskVolumeVersionKey_t),
	    val, data_size, 0);

	return (ret);
}

/*
 * Get disk volume from dictionary.
 */
static int
diskVolsGet(
	DiskVolsDictionary_t *dict,
	char *key,
	struct DiskVolumeInfo **dv)
{
	int ret;
	vsn_t vsn;
	host_t host;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict) || dv == NULL) {
		return (-1);
	}

	*dv = NULL;

	if (IS_VSN(dict)) {
		memset(&vsn, 0, sizeof (vsn_t));
		strncpy((char *)&vsn, key, sizeof (vsn_t));
		ret = dbfile->Get(dbfile, (void *)&vsn, sizeof (vsn_t),
		    (void **)dv);
	} else if (IS_CLI(dict)) {
		memset(&host, 0, sizeof (host_t));
		strncpy((char *)&host, key, sizeof (host_t));
		ret = dbfile->Get(dbfile, (void *)&host, sizeof (host_t),
		    (void **)dv);
	} else {
		ret = -1;
	}

	return (ret);
}

/*
 * Get disk volume from old version of the dictionary.
 */
static int
diskVolsGetOld(
	DiskVolsDictionary_t *dict,
	char *key,
	struct DiskVolumeInfo **dv,
	DiskVolumeVersionVal_t version)
{
	int ret;
	vsn_t vsn;
	DiskVolumeInfo_t *new;
	DiskVolumeInfoR45_t *old;
	size_t size;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict) || dv == NULL) {
		return (-1);
	}

	if (version != DISKVOLS_VERSION_R45) {
		return (-1);
	}

	*dv = NULL;

	if (IS_VSN(dict)) {
		memset(&vsn, 0, sizeof (vsn_t));
		strncpy((char *)&vsn, key, sizeof (vsn_t));
		ret = dbfile->Get(dbfile, (void *)&vsn, sizeof (vsn_t),
		    (void **)&old);
		if (ret == 0) {
			size = STRUCT_RND(sizeof (struct DiskVolumeInfo) +
			    old->DvPathLen);
			new = (struct DiskVolumeInfo *)malloc(size);
			memset(new, 0, size);

			strncpy(new->DvHost, old->DvHost, sizeof (new->DvHost));
			new->DvSpace = old->DvSpace;
			new->DvCapacity = old->DvCapacity;
			new->DvFlags = old->DvFlags;
			new->DvPathLen = old->DvPathLen;
			strncpy(new->DvPath, old->DvPath, new->DvPathLen);

			*dv = new;
		}
	} else {
		ret = -1;
	}

	return (ret);
}

/*
 * Get disk volume version from dictionary.
 */
static int
diskVolsGetVersion(
    DiskVolsDictionary_t *dict,
    DiskVolumeVersionVal_t **val)
{
	int ret;
	DiskVolumeVersionKey_t versionKey;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict) || !IS_HDR(dict)) {
		return (-1);
	}

	ASSERT(sizeof (DiskVolumeVersionKey_t) == strlen(DISKVOLS_VERSION_KEY));
	memcpy(versionKey, DISKVOLS_VERSION_KEY, sizeof (DiskVolumeVersionKey_t));

	ret = dbfile->Get(dbfile, versionKey, sizeof (DiskVolumeVersionKey_t),
	    (void **)val);

	return (ret);
}

/*
 * Delete disk volume from dictionary.
 */
static int
diskVolsDel(
	DiskVolsDictionary_t *dict,
	char *key)
{
	int ret;
	vsn_t vsn;
	host_t host;
	DiskVolumeVersionKey_t version;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	if (IS_VSN(dict)) {
		memset(&vsn, 0, sizeof (vsn_t));
		strncpy((char *)&vsn, key, sizeof (vsn_t));
		ret = dbfile->Del(dbfile, vsn, sizeof (vsn_t));
	} else if (IS_CLI(dict)) {
		memset(&host, 0, sizeof (host_t));
		strncpy((char *)&host, key, sizeof (host_t));
		ret = dbfile->Del(dbfile, host, sizeof (host_t));
	} else if (IS_HDR(dict)) {
		memset(&version, 0, sizeof (DiskVolumeVersionKey_t));
		strncpy((char *)&version, key, sizeof (DiskVolumeVersionKey_t));
		ret = dbfile->Del(dbfile, version,
		    sizeof (DiskVolumeVersionKey_t));
	} else {
		ret = -1;
	}

	return (ret);
}

/*
 * Close disk volume dictionary.
 */
static int
diskVolsClose(
	DiskVolsDictionary_t *dict)
{
	int ret;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	ret = dbfile->Close(dbfile);

	return (ret);
}

/*
 * Begin disk volume iterator.
 */
static int
diskVolsBeginIterator(
	DiskVolsDictionary_t *dict)
{
	int ret;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	ret = dbfile->BeginIterator(dbfile);

	return (ret);
}

/*
 * Get next disk volume from dictionary iterator.
 */
static int
diskVolsGetIterator(
	DiskVolsDictionary_t *dict,
	char **vsn,
	struct DiskVolumeInfo **dv)
{
	int ret;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	if (vsn == NULL || dv == NULL) {
		return (-1);
	}

	*vsn = NULL;
	*dv = NULL;

	ret = dbfile->GetIterator(dbfile, (void **)vsn, (void **)dv);

	return (ret);
}

/*
 * End disk volume iterator.
 */
static int
diskVolsEndIterator(
	DiskVolsDictionary_t *dict)
{
	int ret;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	if (!IS_INIT(dict)) {
		return (-1);
	}

	ret = dbfile->EndIterator(dbfile);

	return (ret);
}

/*
 * Number of disk volumes in dictionary.
 */
static int
diskVolsNumof(
	DiskVolsDictionary_t *dict,
	int *numof,
	enum DV_numof type)
{
	int ret;
	int cnt;
	char *vsn;
	struct DiskVolumeInfo *dv;
	DBFile_t *dbfile = (DBFile_t *)dict->dbfile;

	ret = dbfile->BeginIterator(dbfile);
	cnt = 0;
	while (ret == 0) {
		ret = dbfile->GetIterator(dbfile, (void **)&vsn, (void **)&dv);
		if (ret == 0) {
			if (type == DV_numof_all) {
				cnt++;
			} else if (DISKVOLS_IS_DISK(dv) &&
			    type == DV_numof_disk) {
				cnt++;
			} else if (DISKVOLS_IS_HONEYCOMB(dv) &&
			    type == DV_numof_honeycomb) {
				cnt++;
			}
		}
	}
	ret = dbfile->EndIterator(dbfile);
	*numof = cnt;

	return (ret);
}

/*
 * Check for existence of a valid diskvols.seqnum file.
 * If the 'needs audit' flag is set calculate the space used
 * value for an archive volume and update the sequnum file.  If
 * successful, the caller is responsible for clearing the audit
 * flag.
 */
static boolean_t
isValidSeqnumFile(
	char *volname,
	struct DiskVolumeInfo *dv,
	void *sam_rft)
{
	boolean_t valid_seqnum, swapped = B_FALSE;
	int rval;
	struct DiskVolumeSeqnumFile buf;
	size_t size;
	int nbytes;
	char *filename;
	char *errFunc = NULL;

	valid_seqnum = B_FALSE;

	filename = DISKVOLS_SEQNUM_FILENAME;
	if (DISKVOLS_IS_HONEYCOMB(dv)) {
		filename = volname;
	}
	snprintf(pathname, sizeof (pathname), "%s/%s.%s",
	    dv->DvPath, filename, DISKVOLS_SEQNUM_SUFFIX);

	rval = SamrftOpen(sam_rft, pathname, O_RDWR, NULL);

	if (rval == 0) {
		/*
		 * Read to make sure we found a valid diskvols.seqnum file.
		 */
		size = sizeof (struct DiskVolumeSeqnumFile);
		nbytes = SamrftRead(sam_rft, &buf, size);

		if (nbytes == size) {
			if (buf.DsMagic == DISKVOLS_SEQNUM_MAGIC_RE) {
				swapped = B_TRUE;
			} else if (buf.DsMagic != DISKVOLS_SEQNUM_MAGIC) {
				Trace(TR_ERR, "Disk volume seqnum file magic"
				    " number invalid");
				return (-1);
			}

			/*
			 * Valid seqnum file. Check if auditing.
			 */
			valid_seqnum = B_TRUE;

			if (dv->DvFlags & DV_needs_audit) {
				off64_t off;
				fsize_t spaceUsed;

				(void) SamrftSpaceUsed(sam_rft, dv->DvPath,
				    &spaceUsed);

				if (SamrftFlock(sam_rft, F_WRLCK) < 0) {
					(void) SamrftClose(sam_rft);
					errFunc = "flock";
					goto err;
				}

				buf.DsUsed = spaceUsed;

				/*
				 * Prepare for write, rewind the file.
				 */
				if (SamrftSeek(sam_rft, 0, SEEK_SET,
				    &off) < 0) {
					(void) SamrftClose(sam_rft);
					errFunc = "seek";
					goto err;
				}

				if (swapped) {
					sam_bswap8(&buf.DsUsed, 1);
				}

				nbytes = SamrftWrite(sam_rft, &buf, size);
				if (nbytes != size) {
					(void) SamrftClose(sam_rft);
					errFunc = "write";
					goto err;
				}

				if (SamrftFlock(sam_rft, F_UNLCK) < 0) {
					(void) SamrftClose(sam_rft);
					errFunc = "unlock";
					goto err;
				}
			}
		}
		(void) SamrftClose(sam_rft);
	}

err:
	if (errFunc != NULL) {
		Trace(TR_ERR, "Disk archive seqnum file %s failed '%s', "
		    "errno= %d", errFunc, pathname, errno);
		valid_seqnum = B_FALSE;
	}

	return (valid_seqnum);
}

/*
 * Get mount point name for SAM-FS file system.
 */
static char *
getMountPnt(
	char *name,
	char *mountPnt,
	int mountPntSize)
{
	FILE *fp;
	struct mnttab mnt;
	struct mnttab *mp;
	struct stat buf;
	char *retval = NULL;

	if (stat(name, &buf) == -1) {
		return (retval);
	}

	fp = fopen(MOUNTED, "r");
	if (fp != NULL) {
		mp = &mnt;
		while ((getmntent(fp, mp)) == 0) {
			if (strcmp(mp->mnt_fstype, "samfs") == 0) {
				int dev;
				char *devopt;

				devopt = strstr(mp->mnt_mntopts, "dev=");
				if (devopt) {
					dev = xatoi(devopt + 4);
					if (buf.st_dev == dev) {
						strncpy(mountPnt,
						    mp->mnt_mountp,
						    mountPntSize - 1);
						retval = mountPnt;
						break;
					}
				}
			}
		}
	}
	fclose(fp);

	return (retval);
}
#endif	/* sun */

static int
xatoi(
	char *cp)
{
	int val;

	val = 0;
	while (*cp) {
		if (*cp >= 'a' && *cp <= 'f')
			val = val * 16 + *cp - 'a' + 10;
		else if (*cp >= 'A' && *cp <= 'F')
			val = val * 16 + *cp - 'A' + 10;
		else if (*cp >= '0' && *cp <= '9')
			val = val * 16 + *cp - '0';
		else
			break;
		cp++;
	}
	return (val);
}

#ifdef sun
/*
 * Get status information for file/directory.
 */
static int
statPath(
	SamrftImpl_t *rftd,
	char *path,
	struct stat *sb)
{
	SamrftStatInfo_t buf;
	int ret;

	memset(sb, 0, sizeof (struct stat));

	ret = SamrftStat(rftd, path, &buf);
	if (ret == 0) {
		sb->st_mode = buf.mode;
		sb->st_uid  = buf.uid;
		sb->st_gid  = buf.gid;
	}

	return (ret);
}

/* New in solaris 10.  Called from honeycomb client API library. */
int
strerror_r(int errnum, char *strerrbuf, size_t buflen)
{
	char *buf;
	int ret = 0;

	buf = strerror(errnum);

	if (buflen < (strlen(buf) + 1)) {
		ret = errno = ERANGE;
	} else {
		(void) strcpy(strerrbuf, buf);
	}
	return (ret);
}
#endif	/* sun */
