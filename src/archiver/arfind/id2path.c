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

#pragma ident "$Revision: 1.48 $"

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

/* Directory cache entry */
typedef struct DirCacheEntry {
	sam_id_t	id;		/* Id of directory */
	sam_id_t	parent_id;	/* Parent id of directory */
	int		lastUsed;	/* Time of last usage */
	int		entCount;	/* Number of entries */
	size_t		bufSize;	/* Current size of dirent buffer */
	size_t		bufLen;		/* Length of dirent buffer */
	size_t		ptrBufLen;	/* Length of dirent ptr buffer */
	char		*buf;		/* Dirent buffer */
	sam_dirent_t	**ptrBuf;	/* Dirent pointer array */
} DirCacheEntry_t;

#define	CACHE_LEN_INCR (1000)		/* DirCache length increase */
#define	CACHE_BUF_INCR (8<<10)		/* Entry buffer increase (8 KB) */
#define	MAX_INT 0x7FFFFFFF

static DirCacheEntry_t *dirCache;	/* Table of cache entries */
static int dirCacheCount = 0;		/* Number of active cache entries */
static int dirCacheLen = 0;		/* Alloc'd length of dirCache */
static int dirCacheDel = 0;		/* Number of deleted cache entries */
static int dirCacheTotSize = 0;		/* Total size of dirCache, incl. bufs */

static pthread_mutex_t id2pathMutex = PTHREAD_MUTEX_INITIALIZER;
static struct sam_disk_inode dinode;
static struct sam_ioctl_idstat idstatArgs = {
		{ 0, 0 }, sizeof (dinode), &dinode };

/* Private functions. */
static DirCacheEntry_t *cacheLookup(sam_id_t id);
static int cacheSearch(sam_id_t id);
static void cachePurge(void);
static DirCacheEntry_t *cacheEntryNew(sam_id_t id);
static int cacheEntryPopulate(DirCacheEntry_t *entry);
static sam_dirent_t *cacheEntryLookup(DirCacheEntry_t *entry, sam_id_t id);
static void cacheEntryRemove(DirCacheEntry_t *entry);
static int cmpDirent(const void *v1, const void *v2);

/*
 * Return relative path to file given the inode.
 */
void
IdToPath(
	struct sam_disk_inode *fileInode,	/* File's inode. */
	struct PathBuffer *pb)
{
	DirCacheEntry_t *ce;
	sam_dirent_t *dirent;
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
		ce = cacheLookup(parent_id);
		if (ce == NULL) {
			ce = cacheEntryNew(parent_id);
		}

		dirent = cacheEntryLookup(ce, id);
		if (dirent == NULL) {
			/* (re)populate cache entry */
			if (cacheEntryPopulate(ce) < 0) {
				break;
			}

			dirent = cacheEntryLookup(ce, id);
			if (dirent == NULL) {
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
		pathLen += dirent->d_namlen + 1;
		if (pathLen > MAXPATHLEN) {
			Trace(TR_DEBUGERR, "inode %d: pathname too long",
			    fileInode->id.ino);
			path = pb->PbEnd - 1;
			break;
		}
		path -= dirent->d_namlen;
		memmove(path, (char *)dirent->d_name,  dirent->d_namlen);
		path--;
		*path = '/';

		/* Set up to get path component for parent. */
		id = parent_id;
		if (ce->parent_id.ino == 0) {
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
			ce->parent_id = dinode.parent_id;
		}
		parent_id = ce->parent_id;
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
	dirCacheLen = CACHE_LEN_INCR;
	dirCacheTotSize = dirCacheLen * sizeof (DirCacheEntry_t);
	SamMalloc(dirCache, dirCacheTotSize);
	memset(dirCache, 0, dirCacheTotSize);
}


/*
 * Remove inode entry from cache.
 */
void
IdToPathRmInode(
	sam_id_t id)
{
	DirCacheEntry_t *entry;

	PthreadMutexLock(&id2pathMutex);
	entry = cacheLookup(id);
	if (entry != NULL) {
		cacheEntryRemove(entry);
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

	if ((st = TraceOpen()) == NULL) {
		return;
	}

	fprintf(st, "Dircache dir count: %d\n", dirCacheCount);
	fprintf(st, "Dircache size: %d\n", dirCacheTotSize);

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

/* Create new cache entry for directory with specified id. */
static DirCacheEntry_t *
cacheEntryNew(sam_id_t id) {
	int i;
	DirCacheEntry_t *entry = NULL;

	/* Make sure enough room for one more */
	if (dirCacheCount >= dirCacheLen) {
		dirCacheLen += CACHE_LEN_INCR;
		SamRealloc(dirCache, dirCacheLen * sizeof (DirCacheEntry_t));
		dirCacheTotSize += CACHE_LEN_INCR * sizeof (DirCacheEntry_t);
	}

	/* Search for slot to use and set entry */
	i = cacheSearch(id);
	entry = &dirCache[i];

	/* If not our slot, move everything to the right */
	if (entry->id.ino != id.ino) {
		memmove(entry + 1, entry,
		    (dirCacheCount - i) * sizeof (DirCacheEntry_t));
		dirCacheCount++;
	}

	/* Initialize entry */
	memset(entry, 0, sizeof (DirCacheEntry_t));
	entry->id = id;
	entry->lastUsed = time(NULL);

	return (entry);
}

/* Lookup existing cache entry for directory, NULL if not found */
static DirCacheEntry_t *
cacheLookup(sam_id_t id)
{
	int i;
	DirCacheEntry_t *entry = NULL;

	i = cacheSearch(id);
	if (dirCache[i].id.ino == id.ino &&
	    dirCache[i].id.gen == id.gen) {
		entry = &dirCache[i];
		entry->lastUsed = time(NULL);
	}

	return (entry);
}

/*
 * Searches dirCache array for entry with given id.
 * If found, returns index of entry matching id.
 * If not found, returns index where entry should be inserted.
 */
static int
cacheSearch(sam_id_t id) {
	int i = dirCacheCount / 2;
	int low = 0;
	int high = dirCacheCount;

	/* Binary search dirCache array */
	while (low < high) {
		if (dirCache[i].id.ino < id.ino) {
			low = i + 1;
		} else if (dirCache[i].id.ino > id.ino) {
			high = i;
		} else { /* equal */
			break;
		}
		i = ((uint_t)low + (uint_t)high) / 2;
	}

	return (i);
}

/*
 * Populates the given cache entry with the directory contents.
 *
 * On return the sam_dirent pointers in entry->ptrBuf will be
 * sorted by inode.
 */
static int
cacheEntryPopulate(DirCacheEntry_t *entry)
{
	sam_ioctl_idgetdents_t request;	/* Getdents request */
	char *dirbuf;
	char *endbuf;
	sam_dirent_t *dirp;
	int n;

#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Read dir start inode: %d.%d",
	    entry->id.ino, entry->id.gen);
#endif /* defined(I2P_TRACE) */

	State->AfId2pathReaddir++;

	entry->bufSize = 0;
	entry->entCount = 0;

	request.id = entry->id;
	request.offset = 0;
	request.eof = 0;

	/* Read the directory to populate the cache entry */
	while (!request.eof) {
		if (entry->bufLen - entry->bufSize < MAXNAMELEN) {
			entry->bufLen += CACHE_BUF_INCR;
			SamRealloc(entry->buf, entry->bufLen);
			dirCacheTotSize += CACHE_BUF_INCR;
		}

		dirbuf = entry->buf + entry->bufSize;
		request.dir.ptr = (void *) dirbuf;
		request.size = entry->bufLen - entry->bufSize;

		if ((n = ioctl(FsFd, F_IDGETDENTS, &request)) < 0) {
			return (-1);
		}

		endbuf = dirbuf + n;
		dirp = (sam_dirent_t *)((void *)dirbuf);
		while ((char *)dirp < endbuf) {
			if ((entry->entCount + 1) * sizeof (sam_dirent_t *) >
			    entry->ptrBufLen) {
				entry->ptrBufLen += CACHE_LEN_INCR *
				    sizeof (sam_dirent_t *);
				SamRealloc(entry->ptrBuf, entry->ptrBufLen);
				dirCacheTotSize += CACHE_LEN_INCR *
				    sizeof (sam_dirent_t *);
			}

			entry->ptrBuf[entry->entCount++] = dirp;
			entry->bufSize += SAM_DIRSIZ(dirp);
			dirp = (sam_dirent_t *)((void *)((char *)dirp +
			    SAM_DIRSIZ(dirp)));
		}
	}

	/* Sort directory entry pointers */
	qsort(entry->ptrBuf, entry->entCount,
	    sizeof (sam_dirent_t *), cmpDirent);

	/* Get rid of old entries */
	cachePurge();

	if (errno != 0) {
		Trace(TR_ERR, "Read dir error inode: %d.%d errno: %d",
		    entry->id.ino, entry->id.gen, errno);
	}

#if defined(I2P_TRACE)
	Trace(TR_DEBUG, "Read dir done inode: %d.%d",
	    entry->id.ino, entry->id.gen);
#endif

	return (0);
}

/*
 * Purges the cache of old entries if the size of the cache is
 * larger than the maximum allowed.  If the number of removed
 * entries is large, then this function will remove inactive
 * entries from dirCache.
 */
static void
cachePurge(void)
{
	int i;
	int min_i;
	int min_age;

	/* Remove entries until below max size or only one left */
	while (dirCacheTotSize > State->AfDirCacheSize &&
	    (dirCacheCount - dirCacheDel) > 1) {
		/* Find oldest unused entry and remove it */
		min_i = MAX_INT;
		min_age = MAX_INT;
		for (i = 0; i < dirCacheCount; i++) {
			DirCacheEntry_t *entry = &dirCache[i];
			if (entry->id.gen == -1) {
				continue;
			}

			if (entry->lastUsed < min_age) {
				min_i = i;
				min_age = entry->lastUsed;
			}
		}

		cacheEntryRemove(&dirCache[min_i]);
	}

	/*
	 * Check to see if we need to shrink cache entry array.
	 * This is true if the number of unused entries is greater
	 * than two times the growth increase.
	 */
	if (dirCacheDel + (dirCacheLen - dirCacheCount) >
	    2 * CACHE_LEN_INCR) {
		int numRemoved = 0;
		int hole_start = -1;	/* Where entries to delete begin */
		int good_start = -1;	/* Where entries to keep begin */
		for (i = 0; i < dirCacheCount; i++) {
			if (dirCache[i].id.gen == -1) {
				/*
				 * Copy the current good entries over the
				 * hole left by removed entries.
				 */
				if (hole_start >= 0 && good_start > 0) {
					int hole_len = good_start - hole_start;
					int good_len = i - good_start;
					memmove(&dirCache[hole_start],
					    &dirCache[good_start],
					    good_len *
					    sizeof (DirCacheEntry_t));
					/*
					 * numRemoved depends on whether we
					 * copied more than the size of the
					 * hole.
					 */
					numRemoved += good_len < hole_len ?
					    good_len : hole_len;
					hole_start += good_len;
					good_start = -1;
				} else if (hole_start < 0) {
					hole_start = i;
					good_start = -1;
				}
			} else {
				if (good_start < 0) {
					good_start = i;
				}
			}
		}
		/* Copy remainder of elements */
		if (hole_start >= 0 && good_start > 0) {
			int hole_len = good_start - hole_start;
			int good_len = dirCacheCount - good_start;
			memmove(&dirCache[hole_start], &dirCache[good_start],
			    good_len * sizeof (DirCacheEntry_t));
			/*
			 * Here numRemoved is always the hole size because
			 * we are at the end of the array.
			 */
			numRemoved += hole_len;
		} else if (hole_start >= 0) {
			/* dirCache ended with a hole */
			numRemoved += dirCacheCount - hole_start;
		}

		/* Need to readjust total size for new dirCacheLen */
		dirCacheTotSize -= dirCacheLen * sizeof (DirCacheEntry_t);

		dirCacheLen -= numRemoved;
		/* Round up length to the nearest CACHE_LEN_INCR */
		dirCacheLen += CACHE_LEN_INCR - (dirCacheLen % CACHE_LEN_INCR);

		/* Resize dirCache */
		SamRealloc(dirCache, dirCacheLen * sizeof (DirCacheEntry_t));
		dirCacheTotSize += dirCacheLen * sizeof (DirCacheEntry_t);

		dirCacheCount -= numRemoved;
		dirCacheDel = 0;
	}
}

/* Lookup a directory entry with given id in the provided cache entry. */
static sam_dirent_t *
cacheEntryLookup(DirCacheEntry_t *entry, sam_id_t id)
{
	int i = entry->entCount / 2;
	int low = 0;
	int high = entry->entCount;
	sam_dirent_t *dirent = NULL;

	/* Binary search dirent ptrBuf array */
	while (low < high) {
		if (entry->ptrBuf[i]->d_id.ino < id.ino) {
			low = i + 1;
		} else if (entry->ptrBuf[i]->d_id.ino > id.ino) {
			high = i;
		} else { /* equal */
			if (entry->ptrBuf[i]->d_id.gen == id.gen) {
				dirent = entry->ptrBuf[i];
			}
			break;
		}
		i = ((uint_t)low + (uint_t)high) / 2;
	}

	return (dirent);
}

/*
 * Remove the given cache entry.
 * Frees memory used by entry and marks entry removed
 * by setting id.gen to -1.  The entry remains in dirCache
 * with ino still set for binary search purposes.
 */
static void
cacheEntryRemove(DirCacheEntry_t *entry)
{
	if (entry->buf != NULL) {
		SamFree(entry->buf);
		entry->buf = NULL;
	}
	if (entry->ptrBuf != NULL) {
		SamFree(entry->ptrBuf);
		entry->ptrBuf = NULL;
	}

	dirCacheTotSize -= entry->bufLen;
	dirCacheTotSize -= entry->ptrBufLen;

	/*
	 * Leave ino number for binary search purposes,
	 * Set gen to -1 to mark removed,
	 * Set lastUsed to MAX_INT for LRU defense,
	 * Set everything else to 0
	 */
	entry->id.gen = -1;
	entry->lastUsed = MAX_INT;
	entry->parent_id.ino = 0;
	entry->parent_id.gen = 0;
	entry->entCount = 0;
	entry->bufLen = 0;
	entry->bufSize = 0;
	entry->ptrBufLen = 0;

	dirCacheDel++;
}

/* Comparison function to sort directory entries using qsort */
static int
cmpDirent(const void *v1, const void *v2) {
	sam_dirent_t *d1 = *((sam_dirent_t **)v1);
	sam_dirent_t *d2 = *((sam_dirent_t **)v2);

	if (d1->d_id.ino < d2->d_id.ino) {
		return (-1);
	} else if (d1->d_id.ino > d2->d_id.ino) {
		return (1);
	} else {
		return (0);
	}
}
