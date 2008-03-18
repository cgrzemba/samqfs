/*
 * id2path.c - Return a path for the given inode.
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

/*
 *	id2path derives the full path name for a SAM-FS file given a pointer
 *	to it's inode.
 *
 *	The parent inode number is used to recursively read the directories
 *	in the path to the file until the root inode is reached.  The path
 *	components are thus found.
 *
 *	The path components and associated inode information are cached to
 *	speed up the process.
 *
 *	The character pointer returned points to a static array.
 *	The function is intended to be used repeatedly.
 *
 */

#pragma ident "$Revision: 1.4 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* SNAP		If defined, turn on table snap code */

/* ANSI C headers. */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "pub/stat.h"

/* Local headers. */
#include "common.h"
#include "dir_inode.h"

/* Types. */
typedef struct PathInfo_s {
	sam_id_t	id;
	sam_id_t	parent_id;
	char		*PathComponent;
} PathInfo;

/* Private data. */
static struct sam_disk_inode dinode;
static char pathString[MAXPATHLEN + 4];

/* Path information caching. */

typedef struct PathCacheEntry_s {
	PathInfo info;			/* The data being cached */
	struct PathCacheEntry_s	*next;	/* Next entry in the cache list */
	short	refcount;		/* Number of references to entry */
} PathCacheEntry;

/*
 * The path information cache is a series of linked lists.
 */
static PathCacheEntry *pathCache;	/* The cache */
static PathCacheEntry *pathCacheFree;	/* The free entry list */
static int pathCacheNumof = 50;

/*
 * The hash table contains pointers to linked lists of cache entries.
 * The table is indexed by the lower n bits of the inode number.
 */
#define	HASHTABLENUMOF 16
#define	HASHMASK (HASHTABLENUMOF - 1)
static PathCacheEntry *hashTable[HASHTABLENUMOF];

/* Performance statistics. */
static int pathComponentLookups;
static int cacheHits;
static int cacheEntries;
static int cachePurges;

/* Private functions. */
static void addPathInfo(PathInfo *pi);
static char *idPathComponent(void);
static PathInfo *lookupPathInfo(sam_id_t *id);
static void purgeCache(void);
#if defined(SNAP)
void snapPathCache(void);
#endif /* defined(SNAP) */


/*
 * Return relative path to file given the inode.
 */
static char *
IdToPath(
	struct sam_disk_inode *fileInode)	/* File's inode. */
{
	char	*path;
	int	pathLen;

	dinode.id = fileInode->id;
	dinode.parent_id = fileInode->parent_id;
	dinode.mode = fileInode->mode;
	dinode.status.bits = fileInode->status.bits;

	/*
	 * Start at the end of the path buffer.
	 */
	path = &pathString[MAXPATHLEN];
	*path = '\0';
	pathLen = 0;
	while (dinode.id.ino != SAM_ROOT_INO && dinode.id.ino != 0) {
		char	*pathComponent;
		size_t	pathComponentLength;

		pathComponent = idPathComponent();
		if (pathComponent == NULL) {
			path = &pathString[MAXPATHLEN - 1];
			break;
		}
		/*
		 * Add path component and '/' to beginning of path.
		 */
		pathComponentLength = strlen(pathComponent);
		pathLen += pathComponentLength + 1;
		if (pathLen > MAXPATHLEN) {
			Trace(TR_DEBUGERR, "inode %d: pathname too long",
			    fileInode->id.ino);
			path = &pathString[MAXPATHLEN - 1];
			break;
		}
		path -= pathComponentLength;
		memmove(path, pathComponent, pathComponentLength);
		path--;
		*path = '/';
	}
	*path = '\0';   /* Mark start of path */
	return (path + 1);
}


/*
 * Return relative path to file given an id.
 */
char *
IdToPathId(
	sam_id_t id)
{
	if (GetDinode(id, &dinode) != 0) {
		return ("");
	}
	return (IdToPath(&dinode));
}


/*
 * Initialize module data.
 */
void
IdToPathInit(void)
{
	PathCacheEntry	*entry;
	PathCacheEntry	*limit;
	size_t			size;

	if (pathCache != NULL) {
		return;
	}
	/*
	 * Initialize cache.
	 */
	size = pathCacheNumof * sizeof (PathCacheEntry);
	SamMalloc(pathCache, size);
	memset(pathCache, 0, size);
	limit = &pathCache[pathCacheNumof];
	pathCacheFree = pathCache;
	for (entry = pathCache; entry < limit - 1; entry++) {
		entry->next = entry + 1;
	}
	FsFd = -1;
}


/*
 * Remove directory entry from cache.
 */
void
IdToPathRmCacheEntry(
	sam_id_t id)
{
	PathCacheEntry *entry, *prev;
	int	tableIndex;

	tableIndex = HASHMASK & id.ino;
	entry = hashTable[tableIndex];
	prev = NULL;
	while (entry != NULL) {
		PathCacheEntry *next;

		next = entry->next;
		if (entry->info.id.ino == id.ino &&
		    entry->info.id.gen == id.gen) {

			SamFree(entry->info.PathComponent);
			entry->info.PathComponent = NULL;
			entry->refcount = 0;
			if (prev == NULL) {
				hashTable[tableIndex] = entry->next;
			} else {
				prev->next = entry->next;
			}
			cacheEntries--;
			entry->next = pathCacheFree;
			pathCacheFree = entry;
			break;
		}
		prev = entry;
		entry = next;
	}
}


/* Private functions. */


/*
 * Add path information to cache.
 */
static void
addPathInfo(
	PathInfo *p)
{
	PathCacheEntry *entry;

	/*
	 * Check for entry in cache and set the list entry linking variables.
	 */
	if (lookupPathInfo(&p->id) != NULL) {
		return;
	}
	while (pathCacheFree == NULL) {
		purgeCache();
		/*
		 * After a cache purge, the info has to be looked up again.
		 */
		if (lookupPathInfo(&p->id) != NULL) {
			return;
		}
	}
	entry = pathCacheFree;
	pathCacheFree = entry->next;

	/*
	 * Link new entry into chain.
	 */
	entry->next = hashTable[HASHMASK & p->id.ino];
	hashTable[HASHMASK & p->id.ino] = entry;

	/*
	 * Add new entry.
	 */
	SamStrdup(entry->info.PathComponent, p->PathComponent);
	entry->info.id = p->id;
	entry->info.parent_id = p->parent_id;
	entry->refcount = 100;
	cacheEntries++;
}


/*
 * Get the path component from parent directory for the id.
 */
static char *
idPathComponent(void)
{
	static struct DirRead dr;
	struct sam_dirent *dp;
	PathInfo *ip;
	sam_id_t id;
	sam_id_t parent_id;
	boolean_t isDirectory;
	char	*pathComponent;

	id = dinode.id;
	isDirectory = S_ISDIR(dinode.mode);
	pathComponentLookups++;
	if ((ip = lookupPathInfo(&id)) != NULL) {
		cacheHits++;

		/*
		 * Make up the parts of the parent inode that we need.
		 */
		dinode.id = ip->parent_id;
		dinode.parent_id.ino = 0;
		dinode.mode = S_IFDIR;
		return (ip->PathComponent);
	}

	if (dinode.parent_id.ino == 0) {
#if defined(SNAP)
		Trace(TR_ARDEBUG, "Parent of %ld not in cache", dinode.id.ino);
#endif /* defined(SNAP) */
		if (GetDinode(dinode.id, &dinode) < 0) {
			if (errno != EBADF && errno != ENOENT) {
				Trace(TR_DEBUGERR, "stat(%d.%d)",
				    dinode.id.ino, dinode.id.gen);
			}
			return (NULL);
		}
	}

	/*
	 * Open the parent directory.
	 * dinode will now contain the inode information of the parent.
	 */
	parent_id = dinode.parent_id;
	if (OpenDir(dinode.parent_id, &dinode, &dr) < 0) {
		return (NULL);
	}

	/*
	 * Read the directory to find the entry for the required inode.
	 */
	pathComponent = NULL;
	while ((dp = GetDirent(&dr)) != NULL) {
		if (dp->d_id.ino == id.ino && dp->d_id.gen == id.gen) {
			pathComponent = (char *)dp->d_name;
			/*
			 * Cache a directory entry.
			 */
			if (isDirectory) {
				PathInfo direntPathInfo;

				direntPathInfo.id = dp->d_id;
				direntPathInfo.parent_id = parent_id;
				direntPathInfo.PathComponent = pathComponent;
				addPathInfo(&direntPathInfo);
			}
			break;
		}
	}
	if (errno != 0) {
		Trace(TR_DEBUGERR, "readdir(%d)", dinode.parent_id.ino);
	}
	if (close(dr.DrFd) < 0) {
		Trace(TR_DEBUGERR, "closedir(%d)", dinode.parent_id.ino);
	}
	return (pathComponent);
}


/*
 * Lookup inode in cache.
 */
static PathInfo *
lookupPathInfo(
	sam_id_t *id)
{
	PathCacheEntry *entry;

	entry = hashTable[HASHMASK & id->ino];
	while (entry != NULL) {
		if (entry->info.id.ino == id->ino &&
		    entry->info.id.gen == id->gen) {
			entry->refcount++;
			return (&entry->info);
		}
		entry = entry->next;
	}
	return (NULL);
}


/*
 * Remove lesser referenced entries from the cache.
 */
static void
purgeCache(void)
{
	PathCacheEntry	*entry;
	PathCacheEntry	*limit;
	int	entriesFreed;
	int	minCount;
	int	tableIndex;

	/*
	 * Use the minimum refcount as a criteria to delete an entry.
	 */
	minCount = INT_MAX;
	limit = &pathCache[pathCacheNumof];
	for (entry = pathCache; entry < limit; entry++) {
		if (entry->refcount == 0) {
			continue;
		}
		if (entry->refcount < minCount) {
			minCount = entry->refcount;
		}
	}
	cachePurges++;

	/*
	 * Go through cache and remove entries.
	 */
	entriesFreed = 0;
	for (tableIndex = 0; tableIndex < HASHTABLENUMOF; tableIndex++) {
		PathCacheEntry *prev;

		entry = hashTable[tableIndex];
		if (entry == NULL) {
			continue;
		}
		prev = NULL;
		while (entry != NULL) {
			PathCacheEntry *next;

			next = entry->next;
			if (entry->refcount <= minCount) {
				entriesFreed++;
				SamFree(entry->info.PathComponent);
				entry->info.PathComponent = NULL;
				entry->refcount = 0;
				if (prev == NULL) {
					hashTable[tableIndex] = entry->next;
				} else {
					prev->next = entry->next;
				}
				cacheEntries--;
				entry->next = pathCacheFree;
				pathCacheFree = entry;
			} else {
				/* Kind of age the refcount. */
				entry->refcount = (entry->refcount / 4) + 1;
				prev = entry;
			}
			entry = next;
		}
	}

#if defined(SNAP)
	Trace(TR_ARDEBUG, "Cache purged %d entries freed, minCount = %d",
	    entriesFreed, minCount);
#endif /* defined(SNAP) */
}


#if defined(SNAP)
/*
 * Print the cache contents.
 */
void
SnapPathCache(void)
{
	int n;

	for (n = 0; n < HashTableNumof; n++) {
		fprintf(stderr, "%2d - ", n);
		if (HashTable[n] == NULL) {
			fprintf(stderr, "NULL");
		} else {
			fprintf(stderr, "%4d", HashTable[n] - PathCache);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "Free ");
	if (PathCacheFree == NULL) {
		fprintf(stderr, "NULL");
	} else {
		fprintf(stderr, "%4d", PathCacheFree - PathCache);
	}
	fprintf(stderr, "\n");

	for (n = 0; n < PathCacheNumof; n++) {
		PathCacheEntry *entry;

		entry = &PathCache[n];
		if (entry->refcount != -1) {
			fprintf(stderr, "%4d - ", n);
			if (entry->next == NULL) {
				fprintf(stderr, "NULL");
			} else {
				fprintf(stderr, "%4d", entry->next - PathCache);
			}
			fprintf(stderr, " (%d): ", entry->refcount);
			fprintf(stderr, "%ld (%ld) %s\n",
			    entry->info.id.ino, entry->info.parent_id.ino,
			    (entry->info.PathComponent != NULL) ?
			    entry->info.PathComponent : "NULL");
		}
	}
	fprintf(stderr,
	    "Lookups %d, Cache hits %d Cache entries %d Cache purges %d\n",
	    pathComponentLookups, cacheHits, cacheEntries, cachePurges);
}
#endif /* defined(SNAP) */
