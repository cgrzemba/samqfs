/*
 * id2path.c - Return a path for the given inode.
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
 * id2path derives the full path name for a SAM-FS file given a pointer
 * to it's inode.
 *
 * The parent inode number is used to recursively read the directories
 * in the path to the file until the root inode is reached.  The path
 * components are thus found.
 *
 * The path components and associated inode information are cached to
 * speed up the process.
 *
 */

#pragma ident "$Revision: 1.47 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* I2P_TRACE If defined, turn on DEBUG traces for module */
#if defined(DEBUG)
#define	I2P_TRACE
#endif

/* ANSI C headers. */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/vfs.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "pub/stat.h"
#include "sam/fioctl.h"
#include "sam/lib.h"
#include "sam/uioctl.h"
#include "sam/fs/dirent.h"
#include "sam/fs/ino.h"
#include "sam/fs/sblk.h"

/* Local headers. */
#include "arfind.h"
#include "dir_inode.h"


/* Private data. */

/* Directory cache. */
/* Memory mapped files. */
struct DirCache {
	MappedFile_t Dc;
	size_t	DcSize;			/* Size of all entries */
	int	DcCount;		/* Number of entries */
	int	DcFree;			/* Number of freed entries */
	struct CacheEntry {
		sam_id_t CeId;		/* Inode number */
		sam_id_t CeParId;	/* Parent inode number */
		int	CeName_l;	/* Path component length */
		char	CeName[1];	/* Path component */
	} ClEntry[1];
};

#define	CACHE "dircache"
#define	CACHE_MAGIC 0301032005	/* Directory cache file magic number */

#define	CACHE_INCR (128 * 1024)
#define	CACHE_MAX (32 * CACHE_INCR)
#define	CACHE_MAX_TABLE_COUNT 32

#define	CE_SIZE(ce) (STRUCT_RND(sizeof (struct CacheEntry)) + \
	STRUCT_RND((ce)->CeName_l))

#define	DIR_CACHE_TABLE_INCR 10

static struct DirCache **dirCacheTable;
static int dirCacheTableCount = 0;

static struct DirCache *dirCache = NULL;
static int dirCacheIndex = 0;
static char dirCacheName[] = CACHE"xxxxxxxx";

/*
 * idList -  List of the location of all inode ids in directory caches.
 * Each file in a directory cache is referenced by an entry in the idList.
 * An idList entry "points" to a sam_id_t struct in the directory cache
 * using the index of the DirCache in the dirCacheTable.
 *
 * The idList section is sorted by inode number.  It is searched using
 * a binary search.
 */

/* Inode reference. */
struct IdRef {
	uint_t  IrDcIndex;
	uint_t  IrOffset;
};

static struct IdList {
	MappedFile_t Il;
	size_t	IlSize;			/* Size of all entries */
	int	IlCount;		/* Number of entries */
	struct IdRef IlEntry[1];
} *idList;

#define	IDLIST "idlist_dircache"
#define	IDLIST_MAGIC 0110414112324
#define	IDLIST_INCR 1000
#define	ID_LOC(ir) ((void *) \
	((char *)(void *)dirCacheTable[(ir)->IrDcIndex] + (ir)->IrOffset))


static pthread_mutex_t id2pathMutex = PTHREAD_MUTEX_INITIALIZER;
static struct sam_disk_inode dinode;
static struct sam_ioctl_idstat idstatArgs = {
		{ 0, 0 }, sizeof (dinode), &dinode };

/* Private functions. */
static void cacheAddEntry(sam_id_t id, sam_id_t parent_id, char *name,
    int name_l);
static struct CacheEntry *cacheLookup(sam_id_t id);
static struct DirCache *createDirCache(void);
static struct IdRef *idListAdd(sam_id_t id);
static struct IdRef *idListLookup(sam_id_t id);
static int readDir(sam_id_t parent_id, sam_id_t id);
static void removeDirCache(int dci);


/*
 * Return relative path to file given the inode.
 */
void
IdToPath(
	struct sam_disk_inode *fileInode,	/* File's inode. */
	struct PathBuffer *pb)
{
	struct CacheEntry *ce;
	sam_id_t id;
	sam_id_t parent_id;
	char *path;
	int pathLen;

	/*
	 * Start at the end of the path buffer.
	 */
	PthreadMutexLock(&id2pathMutex);
#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Id2path start inode: %d.%d parent: %d.%d",
	    fileInode->id.ino, fileInode->id.gen,
	    fileInode->parent_id.ino, fileInode->parent_id.gen);
#endif /* defined(I2P_TRACE) */

	path = &pb->PbBuf[PATHBUF_END];
	*path = '\0';
	pb->PbPath = pb->PbEnd = path;
	pathLen = 0;
	id = fileInode->id;
	parent_id = fileInode->parent_id;
	State->AfId2path++;

	while (id.ino != SAM_ROOT_INO && id.ino != 0) {

		ce = cacheLookup(id);
		if (ce == NULL) {
			if (parent_id.ino == 0) {
				/*
				 * Get the inode to find the parent.
				 */
				Trace(TR_DEBUG, "Id2path inode: %d.%d "
				    "find parent", id.ino, id.gen);
				idstatArgs.id = id;
				State->AfId2pathIdstat++;
				if (ioctl(FsFd, F_IDSTAT, &idstatArgs) < 0) {
					if (errno != EBADF) {
						Trace(TR_DEBUGERR,
						    "id2path stat(%d.%d)",
						    id.ino, id.gen);
					}
					path = pb->PbEnd - 1;
					break;
				}
				parent_id = dinode.parent_id;
			}

			/*
			 * Read parent directory which adds all inodes in
			 * directory to the cache.  Guarantees that entry
			 * for id is cached.
			 */
			if (readDir(parent_id, id) == -1) {
				break;
			}

			/*
			 * Lookup inode in directory cache.  Added by readDir().
			 */
			ce = cacheLookup(id);
			if (ce == NULL) {
				path = pb->PbEnd - 1;
				Trace(TR_MISC, "Id2path inode: %d.%d "
				    "cache lookup failed", id.ino, id.gen);
				break;
			}
		} else {
			State->AfId2pathCached++;
		}

		/*
		 * Append path component.
		 */
		pathLen += ce->CeName_l + 1;
		if (pathLen > MAXPATHLEN) {
			Trace(TR_DEBUGERR, "inode %d: pathname too long",
			    fileInode->id.ino);
			path = pb->PbEnd - 1;
			break;
		}
		path -= ce->CeName_l;
		memmove(path, ce->CeName,  ce->CeName_l);
		path--;
		*path = '/';

		/*
		 * Set up to get path component for parent.
		 */
		id = ce->CeParId;
		parent_id.ino = 0;
	}

	*path = '\0';   /* Mark start of path */
	pb->PbPath = path + 1;
#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Id2path done inode: %d.%d (%s)",
	    fileInode->id.ino, fileInode->id.gen, pb->PbPath);
#endif /* defined(I2P_TRACE) */
	PthreadMutexUnlock(&id2pathMutex);
}


/*
 * Return relative path to file given an id.
 */
void
IdToPathId(
	sam_id_t id,
	struct PathBuffer *pb)
{
	static struct sam_perm_inode pinode;

#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Id2path dir inode: %d.%d", id.ino, id.gen);
#endif /* defined(I2P_TRACE) */
	if (GetPinode(id, &pinode) != 0) {
		pb->PbEnd = pb->PbPath;
		*pb->PbPath = '\0';
	} else {
		char	*p;
		int	l;

		/*
		 * Preserve path buffer positioning.
		 */
		p = pb->PbPath;
		IdToPath((struct sam_disk_inode *)&pinode, pb);
		l = Ptrdiff(pb->PbEnd, pb->PbPath);
		memmove(p, pb->PbPath, l + 1);
		pb->PbPath = p;
		pb->PbEnd = p + l;
	}
}


/*
 * Initialize module data.
 */
void
IdToPathInit(void)
{
	static struct DirCache emptyCf;

	/*
	 * Make the dirCache table.
	 */
	dirCacheTableCount = DIR_CACHE_TABLE_INCR;
	SamMalloc(dirCacheTable,
	    dirCacheTableCount * sizeof (struct DirCache *));
	memset(dirCacheTable, 0,
	    dirCacheTableCount * sizeof (struct DirCache *));
	dirCache = &emptyCf;
	dirCache->DcSize = dirCache->Dc.MfLen = CACHE_MAX;

	/*
	 * Create the empty idList 'idlist_dircache'.
	 */
	idList = MapFileCreate(IDLIST, IDLIST_MAGIC,
	    IDLIST_INCR * sizeof (struct IdRef));
	idList->Il.MfValid = 1;
	idList->IlCount = 0;
	idList->IlSize = sizeof (struct IdList);
}


/*
 * Remove inode entry from cache.
 */
void
IdToPathRmInode(
	sam_id_t id)
{
	struct IdRef *ir;

	PthreadMutexLock(&id2pathMutex);
	ir = idListLookup(id);
	if (ir != NULL && ir->IrDcIndex != 0) {
		struct CacheEntry *ce;
		struct DirCache *dc;

		ce = (struct CacheEntry *)ID_LOC(ir);
		if (ce->CeId.gen == id.gen) {
			ce->CeParId.ino = 0;
#if defined(I2P_TRACE)
			Trace(TR_DEBUG,
			    "[%d] Dircache remove inode: %d.%d parent: %d (%s)",
			    ir->IrDcIndex, ce->CeId.ino, ce->CeId.gen,
			    ce->CeParId.ino, ce->CeName);
#endif /* defined(I2P_TRACE) */
			dc = dirCacheTable[ir->IrDcIndex];
			dc->DcFree++;
			ir->IrDcIndex = 0;
			ir->IrOffset = id.ino;
		}
	}
	PthreadMutexUnlock(&id2pathMutex);
}


/*
 * Trace cache.
 */
void
IdToPathTrace(void)
{
	FILE	*st;
	int	dci;

	if ((st = TraceOpen()) == NULL) {
		return;
	}

	fprintf(st, "Dircache table count: %d\n", dirCacheTableCount);

	for (dci = 0; dci < dirCacheTableCount; dci++) {
		struct DirCache *dc;
		char cacheName[] = CACHE"xxxxxxxx";

		if (dirCacheTable[dci] == NULL) {
			continue;
		}
		dc = dirCacheTable[dci];
		snprintf(cacheName, sizeof (cacheName), CACHE"%d", dci);
		fprintf(st,
		    "[%d] Dircache trace (%s) count: %d free: %d size: %d\n",
		    dci, cacheName, dc->DcCount, dc->DcFree, dc->DcSize);
#if defined(I2P_TRACE)
		if (dc->DcCount != 0) {
			char	*cec;
			int	i;

			cec = (char *)&dc->ClEntry[0];
			for (i = 0; i < dc->DcCount; i++) {
				struct CacheEntry *ce;

				ce = (struct CacheEntry *)(void *)cec;
				fprintf(st, "%4d %d.%d %d %s\n",
				    i, ce->CeId.ino, ce->CeId.gen,
				    ce->CeParId.ino, ce->CeName);
				cec += CE_SIZE(ce);
			}
		}
#endif /* defined(I2P_TRACE) */
	}
	TraceClose(-1);
}


/*
 * Return an id for a path.
 */
int
PathToId(
	char *path,	/* Path relative to mount point */
	sam_id_t *id)
{
	static pthread_mutex_t enter = PTHREAD_MUTEX_INITIALIZER;
	static char pathBuf[MAXPATHLEN];
	struct sam_stat sb;

	PthreadMutexLock(&enter);
	snprintf(pathBuf, sizeof (pathBuf), "%s/%s", MntPoint, path);
	if (sam_lstat(pathBuf, &sb, sizeof (sb)) == -1) {
		PthreadMutexUnlock(&enter);
		return (-1);
	}
	id->ino = sb.st_ino;
	id->gen = sb.gen;
	PthreadMutexUnlock(&enter);
	return (0);
}


/* Private functions. */


/*
 * Add a directory entry to the cache.
 */
static void
cacheAddEntry(
	sam_id_t id,
	sam_id_t parent_id,
	char *name,
	int name_l)
{
	struct CacheEntry *ce;
	struct IdRef *ir;
	int	ceSize;

	ir = idListAdd(id);
	if (ir->IrDcIndex != 0) {
		ce = (struct CacheEntry *)ID_LOC(ir);
		if (ce->CeId.gen == id.gen) {
#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "[%d] Dircache add (exists) inode: %d.%d (%s)",
	    dirCacheIndex, ce->CeId.ino, ce->CeId.gen, name);
#endif /* defined(I2P_TRACE) */
			return;
		}
	}

	ceSize = STRUCT_RND(sizeof (struct CacheEntry)) + STRUCT_RND(name_l);
	while (dirCache->DcSize + ceSize > dirCache->Dc.MfLen) {
		if (dirCache->Dc.MfLen == 0 ||
		    dirCache->Dc.MfLen >= CACHE_MAX) {
			dirCache = createDirCache();
		} else {
			dirCache = MapFileGrow(dirCache, CACHE_INCR);
			if (dirCache == NULL) {
				LibFatal(MapFileGrow, dirCacheName);
			}
			dirCacheTable[dirCacheIndex] = dirCache;
#if defined(I2P_TRACE)
			Trace(TR_DEBUG, "[%d] Dircache realloc incr: %d",
			    dirCacheIndex, CACHE_INCR);
#endif /* defined(I2P_TRACE) */
		}
	}

	ce = (struct CacheEntry *)(void *)((char *)dirCache + dirCache->DcSize);
	dirCache->DcCount++;
	dirCache->DcSize += ceSize;
	ce->CeId = id;
	ce->CeParId = parent_id;
	ce->CeName_l = (short)name_l;
	memmove(ce->CeName, name, name_l);
#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "[%d] Dircache add (new) inode: %d.%d (%s)",
	    dirCacheIndex, ce->CeId.ino, ce->CeId.gen, name);
#endif /* defined(I2P_TRACE) */
	ir->IrDcIndex = dirCacheIndex;
	ir->IrOffset = Ptrdiff(ce, dirCache);
}


/*
 * Lookup file in cache.
 */
static struct CacheEntry *
cacheLookup(
	sam_id_t id)
{
	struct IdRef *ir;
	struct CacheEntry *ce;

	ce = NULL;
	ir = idListLookup(id);
	if (ir != NULL && ir->IrDcIndex != 0) {
		ce = (struct CacheEntry *)ID_LOC(ir);
		if (ce->CeId.gen != id.gen) {
#if defined(I2P_TRACE)
			Trace(TR_DEBUG, "[%d] Dircache lookup failed "
			    "inode: %d.%d exist: %d",
			    ir->IrDcIndex, id.ino, id.gen, ce->CeId.gen);
#endif /* defined(I2P_TRACE) */
			ce = NULL;
		}
	}
	return (ce);
}


/*
 * Create a directory cache.
 */
static struct DirCache *
createDirCache(void)
{
	struct DirCache *dc;
	int dci = dirCacheIndex + 1;

#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Create dircache");
#endif /* defined(I2P_TRACE) */

	if (dirCacheTableCount >= CACHE_MAX_TABLE_COUNT) {
		if (dci >= CACHE_MAX_TABLE_COUNT) {
			dci = 0;
		}
		if (dirCacheTable[dci] != NULL) {
			removeDirCache(dci);
		}
	} else if (dci >= dirCacheTableCount) {
		dci = dirCacheTableCount;
		dirCacheTableCount += DIR_CACHE_TABLE_INCR;
		SamRealloc(dirCacheTable,
		    dirCacheTableCount * sizeof (struct DirCache *));
		memset(&dirCacheTable[dci], 0,
		    (dirCacheTableCount - dci) * sizeof (struct DirCache *));
	}

	/*
	 * Initialize an empty cache.
	 */
	snprintf(dirCacheName, sizeof (dirCacheName), CACHE"%d", dci);
	dc = MapFileCreate(dirCacheName, CACHE_MAGIC, CACHE_INCR);
	if (dc == NULL) {
		LibFatal(create, dirCacheName);
	}
	dirCacheIndex = dci;
	dirCacheTable[dirCacheIndex] = dc;
	dc->Dc.MfValid = 1;
	dc->DcCount = 0;
	dc->DcSize = sizeof (struct DirCache);
#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "[%d] Dircache alloc len: %u",
	    dirCacheIndex, dc->Dc.MfLen);
#endif /* defined(I2P_TRACE) */
	return (dc);
}


/*
 * Add an entry to a idList.
 */
static struct IdRef *
idListAdd(
	sam_id_t id)
{
	struct IdRef *ir, *irEnd;
	int i;
	int new;
	int l, u;

	/*
	 * Binary search list.
	 */
	l = 0;
	u = idList->IlCount;
	new = 0;
	while (u > l) {
		sam_ino_t ino;

		i = (l + u) >> 1;	/* Find midpoint */
		ir = &idList->IlEntry[i];
		if (ir->IrDcIndex != 0) {
			sam_id_t *ip;

			ip = ID_LOC(ir);
			ino = ip->ino;
		} else {
			ino = ir->IrOffset;
		}
		if (id.ino == ino) {
			return (ir);
		}
		if (id.ino < ino) {
			u = i;
			new = u;
		} else {
			/* id.ino > ino */
			l = i + 1;
			new = l;
		}
	}

	/*
	 * Need a new entry.
	 */
	while (idList->IlSize + sizeof (struct IdRef) > idList->Il.MfLen) {
		idList =
		    MapFileGrow(idList, IDLIST_INCR * sizeof (struct IdRef));
		if (idList == NULL) {
			LibFatal(MapFileGrow, IDLIST);
		}
#if defined(I2P_TRACE)
		Trace(TR_DEBUG, "Dircache grow idlist from: %d need: %d",
		    idList->Il.MfLen, idList->IlSize + sizeof (struct IdRef));
#endif /* defined(I2P_TRACE) */
	}

	/*
	 * Set location where entry belongs in sorted order.
	 * Move remainder of list up.
	 */
	ir = &idList->IlEntry[new];
	irEnd = &idList->IlEntry[idList->IlCount];
	if (Ptrdiff(irEnd, ir) > 0) {
		memmove(ir+1, ir, Ptrdiff(irEnd, ir));
	}
	idList->IlCount++;
	idList->IlSize += sizeof (struct IdRef);
	ir->IrDcIndex = 0;
	ir->IrOffset = id.ino;
	return (ir);
}


/*
 * Lookup file in idList.
 */
static struct IdRef *
idListLookup(
	sam_id_t id)
{
	struct IdRef *ir;
	int	i;
	int	l, u;

	/*
	 * Binary search list.
	 */
	l = 0;
	u = idList->IlCount;
	while (u > l) {
		sam_ino_t ino;

		i = (l + u) >> 1;	/* Find midpoint */
		ir = &idList->IlEntry[i];
		if (ir->IrDcIndex != 0) {
			sam_id_t *ip;

			ip = ID_LOC(ir);
			ino = ip->ino;
		} else {
			ino = ir->IrOffset;
		}
		if (id.ino == ino) {
			return (ir);
		}
		if (id.ino < ino) {
			u = i;
		} else {
			/* id.ino > ino */
			l = i + 1;
		}
	}
	return (NULL);
}


/*
 * Read directory.
 * Guarantees that the entry with id is in the cache after return.
 */
static int
readDir(
	sam_id_t parent_id,
	sam_id_t id)
{
	static struct DirRead dr;
	struct sam_dirent *dp;
	char id_name[MAXNAMELEN];
	int id_namelen = 0;

	if (OpenDir(parent_id, NULL, &dr) < 0) {
		return (-1);
	}

	/*
	 * Read the directory to find the entry for the required inode.
	 */
#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Read dir start inode: %d.%d",
	    parent_id.ino, parent_id.gen);
#endif /* defined(I2P_TRACE) */

	State->AfId2pathReaddir++;

	while ((dp = GetDirent(&dr)) != NULL) {
		char    *name;

		name = (char *)dp->d_name;
		/* ignore dot and dot-dot */
		if (*name == '.') {
			if (*(name+1) == '\0' ||
			    (*(name+1) == '.' && *(name+2) == '\0')) {
				continue;
			}
		}

		if (dp->d_id.ino == id.ino) {
			strncpy(id_name, name, dp->d_namlen);
			id_namelen = dp->d_namlen;
		} else {
			cacheAddEntry(dp->d_id, parent_id, name, dp->d_namlen);
		}
	}

	/* Guarantee that id's entry is cached */
	if (id_namelen != 0) {
		cacheAddEntry(id, parent_id, id_name, id_namelen);
	}

	if (errno != 0) {
		Trace(TR_ERR, "Read dir error inode: %d.%d errno: %d",
		    parent_id.ino, parent_id.gen, errno);
	}

	if (close(dr.DrFd) < 0) {
		Trace(TR_ERR, "Close dir error inode: %d.%d errno: %d",
		    parent_id.ino, parent_id.gen, errno);
	}

#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Read dir done inode: %d.%d",
	    parent_id.ino, parent_id.gen);
#endif

	return (0);
}


/*
 * Remove a cache by removing references to it from
 * the idlist.
 */
static void
removeDirCache(
	int dci)
{
	static char dcName[] = CACHE"xxxxxxxx";
	int i, j;

	snprintf(dcName, sizeof (dcName), CACHE"%d", dci);

	/*
	 * Eliminate deleted entries and entries for this cache.
	 */
	j = 0;
	for (i = 0; i < idList->IlCount; i++) {
		if (idList->IlEntry[i].IrDcIndex != 0 &&
		    idList->IlEntry[i].IrDcIndex != dci) {
			if (i != j) {
				idList->IlEntry[j] = idList->IlEntry[i];
			}
			j++;
		}
	}

	Trace(TR_DEBUG, "[%d] Dircache remove cache (%s) from: %d to: %d",
	    dci, dcName, idList->IlCount, j);

	idList->IlCount = j;
	idList->IlSize = sizeof (struct IdList) + (j-1) * sizeof (struct IdRef);

	(void) unlink(dcName);
	(void) ArMapFileDetach(dirCacheTable[dci]);
	dirCacheTable[dci] = NULL;
}
