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
#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <sam/types.h>
#include <sam/lib.h>
#include <sam/uioctl.h>
#include <sam/fs/ino.h>
#include <sam/fs/dirent.h>
#include <sam/sam_db.h>
#include <sam/sam_malloc.h>
#include <sam/sam_trace.h>

#include <errmsg.h>

#include "util.h"

/*
 * Max no. of objects/directory
 * e.g., max no. identical hard
 * links/directory
 */
#define	MAX_OBJS	10

void free_path(sam_db_path_t *, int);
void free_link(sam_db_link_t *, int);

int idstat(unsigned int, unsigned int, struct sam_perm_inode *);
int idmva(unsigned int, unsigned int, struct sam_perm_inode *,
		struct sam_vsn_section **);
char *idfind(sam_id_t, sam_id_t);
int idfindall(sam_id_t, sam_id_t, char **, int);

char *find_id(sam_id_t, sam_id_t);
int find_all_ids(sam_id_t, sam_id_t, char **, int);

int verify_inode(struct sam_perm_inode *, sam_db_inode_t *);
void delete(sam_id_t, sam_id_t, sam_db_inode_t *, sam_db_path_t *, int, time_t);

my_ulonglong update_inode(struct sam_perm_inode *, sam_db_inode_t *);
my_ulonglong update_path(sam_db_path_t *, char *, char *);
my_ulonglong update_link(sam_db_link_t *, char *);
my_ulonglong update_archive(struct sam_perm_inode *, struct sam_vsn_section *,
		int, sam_db_archive_t *, my_ulonglong);
my_ulonglong update_archive_status(struct sam_perm_inode *, sam_db_archive_t *,
		my_ulonglong);

char *sam_db_id_to_path(sam_id_t, char *);
my_ulonglong sam_db_delete_inode(sam_db_inode_t *, time_t);
my_ulonglong sam_db_delete_path(sam_db_path_t *, time_t);


/*
 * 	update - Discovery update.
 *
 *	Description:
 *	    Update queries the database and with the the file's current
 *	    inode information, and updates the database accordingly.
 *	    The update is "discovery" in nature in that all apsects of
 *	    the database is verifed and updated regardless of the nature
 *	    of the triggering event.
 *
 *	    Not handled thus far is rename of a directory in that the
 *	    path table needs to update all matching path entries and
 *	    a rename(2) which moves the object from one directory to an
 *	    other.
 *
 *	On Entry:
 *	    id		Inode/generation number to update.
 *	    parent	Parent inode/generation number.
 *	    level	Not used at this time.
 *	    event_time	Event time.
 *	    mount_point	SAM-FS mount point.
 *
 *	On Return:
 *	    Database updated.
 */
void Update(
	sam_id_t id,		/* Inode to update */
	sam_id_t parent,	/* Parent inode */
	/* LINTED E_FUNC_ARG_UNUSED */
	int level,
	time_t event_time,	/* Event time */
	char *mount_point)	/* SAM-FS mount point */
{
	unsigned int ino; /* Inode number for query */
	unsigned int gen; /* Generation number for query */

	sam_db_path_t *T_path, *p;		/* Path table */
	sam_db_inode_t *T_inode, *t;	/* Inode table */
	sam_db_archive_t *T_archive;	/* Archive table */
	sam_db_link_t *T_link; 		/* Link table */
	my_ulonglong n_path; 		/* No. of items in path table */
	my_ulonglong n_inode; 		/* No. of items in path table */
	my_ulonglong n_archive; 	/* No. of items in archive table */
	my_ulonglong n_link; 		/* No. of items in link table	*/
	/* LINTED E_FUNC_SET_NOT_USED */
	my_ulonglong n_rows; 		/* No. of rows returned */
	int n_obj; /* Number of objects in list	*/
	my_ulonglong n;
	struct sam_perm_inode inode; /* Inode image */
	struct sam_vsn_section *vsnp; /* Multivolume archive VSN list */
	int n_mva; 	/* Number of mv archive VSNs */
	int vn; 	/* VSN list offset */
	int copy; 	/* Archive copy number */
	char *path; /* File path */
	char *obj; 	/* Object (file) name */
	char **objs; /* Object (file) name list	*/
	char *link; /* Symbolic link value */
	char *fullpath; /* Full path: /mp/path/obj */
	int i;

	ino = id.ino;
	gen = id.gen;
	T_inode = NULL;
	T_path = NULL;
	T_archive = NULL;
	T_link = NULL;
	n_path = n_archive = n_link = n_obj = 0;
	objs = NULL;
	path = NULL;
	obj = NULL;

	n_inode = sam_db_query_inode(&T_inode, ino, gen);

	/*
	 * Temporary fix for t not initialized problem
	 * and having inode mismatch
	 */
	t = NULL;

	if (gen != 0) {
		if (n_inode == 0) {
			goto new;
		}
		if (n_inode > 1) {
			Trace(TR_ERR, "more than 1 inode entry for "
			    "ino=%u.%u", ino, gen);
		}
	}

	for (n = i = 0; n < n_inode; n++) {
		t = T_inode + n;
		if (!t->deleted) {
			break;
		}
		if (t->gen > i) {
			i = t->gen;
		}
	}

	if (n == n_inode) { /* If all entries deleted	*/
		Trace(TR_DEBUG, "no active entry for ino=%u.%u", ino, gen);

		if (gen == 0) { /* Try next generation	*/
			/* gen = i + 1;	*/
			t = NULL;
			goto new;
		}

		t = T_inode; /* Use deleted entry	*/
	}

	if (gen == 0) {
		gen = t->gen;
	}

	n_path = sam_db_query_path(&T_path, ino, gen);
	n_archive = sam_db_query_archive(&T_archive, ino, gen);

	if (t->type != FTYPE_REG && n_path > 1) {
		Trace(TR_ERR, "more than 1 path entry for "
		    "non-regular file, ino=%u.%u", ino, gen);
	}

	if (t->type == FTYPE_LINK) {
		n_link = sam_db_query_link(&T_link, ino, gen);
		if (n_link > 1) {
			Trace(TR_ERR, "more than 1 link entry for "
			    "ino=%u.%u", ino, gen);
		}
	}

new:
	i = idstat(ino, gen, &inode);

	if (i < 0) {
		if (errno == ENOENT && gen != 0) {
			if (n_path != 0) {
				delete(id, parent, t, T_path,
				    n_path, event_time);
			} else {
				Trace(TR_ERR, "activity lost for "
				    "ino=%u.%u", ino, gen);
			}
			goto exit;
		}
		Trace(TR_ERR, "can't stat ino=%u.%u", ino, gen);
		goto exit;
	}

	if (gen == 0) {
		gen = inode.di.id.gen;
	}

	if (t != NULL && verify_inode(&inode, t)) {
		Trace(TR_ERR, "inode error ino=%u.%u", ino, gen);
		return;
	}

	if (parent.ino == 0) {
		parent = inode.di.parent_id;
	}

	path = sam_db_id_to_path(parent, mount_point);

	if (path == NULL) {
		Trace(TR_ERR, "can't resolve path for ino=%u.%u", ino, gen);
		goto exit;
	}

	SamMalloc(objs, MAX_OBJS * sizeof (char *));
	n_obj = find_all_ids(parent, id, objs, MAX_OBJS);

	if (n_obj == 0) {
		Trace(TR_ERR, "can't find obj ino=%u.%u "
		    "in parent ino=%u.%u", id.ino, id.gen,
		    parent.ino, parent.gen);
		goto exit;
	}

	if (t == NULL) {
		if (T_inode != NULL) {
			SamFree(T_inode);
		}
		T_inode = t = sam_db_new_inode(&inode);
		n_inode = 1;
		n_rows = sam_db_insert_inode(t);
	} else {
		n_rows = update_inode(&inode, t);
	}

	/*
	 * 	This complex block of code addresses the situation where
	 *	it is possible to have a multiple hard links to the same
	 *	file (i.e., inode) with in the same directory.
	 *
	 *	The question remains as to whether to update/create all
	 *	differences found or to just handle the first one found.
	 *	Persumably later events will adjust the remaining differences.
	 *
	 *	The first half of the main IF is the situation where no
	 *	hard links exist and is simply a creation step.
	 *
	 *	The second half eliminates from the path table and the objects
	 *	list, those items which are not in the parent directory as
	 *	identified by path, or there exist a match of objects in the
	 *	two tables.  After the first FOR loop, we are left with a
	 *  residue of un-matched objects which have resulted from
	 *  either a rename(2) or creat(2).
	 *
	 *	Not correctly addressed is the situation where the results of
	 *	a rename(2) from an another directory into a new directory
	 *	where there already exists an image of the file renamed.
	 *	THis will result in the creation of a new path entry.
	 */

	if (n_path == 0) {
		if (n_obj != 0) {
			obj = objs[0];
			T_path = sam_db_new_path(&inode, path, obj);
			n_path = 1;
			n_rows = sam_db_insert_path(T_path);
		}
	} else {
		if (n_obj != 0) {
			for (i = 0; i < n_path; i++) { /* Pass 1	*/
				p = T_path + i;
				if (strcmp(p->path, path) != 0) {
					p->flag = -1;
					continue;
				}
				for (n = 0; n < n_obj; n++) {
					if (strcmp(p->obj, objs[n]) != 0)
						continue;
					p->flag = 1;
					SamFree(objs[n]);
					objs[n] = NULL;
					break;
				}
			}

			for (n = i = 0; n < n_obj; n++) { /* Pass 2	*/
				if (objs[n] == NULL) {
					continue;
				}
				for (; i < n_path; i++) {
					p = T_path + i;
					if (p->flag == 0) {
						break;
					}
				}
				if (i == n_path) {
					free_path(T_path, n_path);
					T_path = sam_db_new_path(&inode,
					    path, objs[n]);
					n_path = 1;
					n_rows = sam_db_insert_path(T_path);
				} else {
					n_rows = update_path(p, NULL, objs[n]);
				}
				break;
			}

			for (n = 0; n < n_obj; n++) {
				if (objs[n] != NULL) {
					SamFree(objs[n]);
				}
			}
			SamFree(objs);
		}
	}

	if (t->type == FTYPE_LINK) {
		SamMalloc(fullpath, strlen(mount_point) +
		    strlen(path) + strlen(obj) + 4);
		strcpy(fullpath, mount_point);
		strcat(fullpath, "/");
		strcat(fullpath, path);
		strcat(fullpath, obj);
		SamMalloc(link, inode.di.psize.symlink + 4);
		if (readlink(fullpath, link, inode.di.psize.symlink+4) < 0) {
			Trace(TR_ERR, "readlink fail: %s, "
			    "ino=%u.%u", fullpath, ino, gen);
		}
		if (n_link == 0) {
			T_link = sam_db_new_link(&inode, link);
			n_link = 1;
			sam_db_insert_link(T_link);
		} else {
			(void) update_link(T_link, link);
		}
		SamFree(fullpath);
		SamFree(link);
	}

	n_mva = idmva(ino, gen, &inode, &vsnp);

	if (n_mva == -2) {
		Trace(TR_ERR, "inode format version < 2, "
		    "ino=%u.%u", ino, gen);
	}
	if (n_mva < 0) {
		Trace(TR_ERR, "can't get mva ino=%u.%u", ino, gen);
		goto exit;
	}

	for (copy = vn = 0; copy < MAX_ARCHIVE; copy++) {
		if (!(inode.di.arch_status & (1<<copy))) {
			continue;
		}
		if (inode.ar.image[copy].n_vsns > 1 &&
		    vn + inode.ar.image[copy].n_vsns > n_mva) {
			Trace(TR_ERR, "insufficent VSNs in MVA list, "
			    "ino=%u.%u c=%d", ino, gen, copy+1);
			continue;
		}

		n_rows = update_archive(&inode, &vsnp[vn],
		    copy+1, T_archive, n_archive);

		if (inode.ar.image[copy].n_vsns > 1) {
			vn += inode.ar.image[copy].n_vsns;
		}
	}

	n_rows = update_archive_status(&inode, T_archive, n_archive);

exit:
	if (path != NULL) {
		SamFree(path);
	}

	if (obj != NULL) {
		SamFree(obj);
	}

	if (T_inode != NULL) {
		SamFree(T_inode);
	}
	if (T_path != NULL) {
		free_path(T_path, n_path);
	}
	if (T_link != NULL) {
		free_link(T_link, n_link);
	}
	if (T_archive != NULL) {
		SamFree(T_archive);
	}
}

/*
 * delete - Process File Delete.
 *
 *	Description:
 *	    Process file delete.  Mark path entry as deleted and if all
 *	    path's deleted, mark inode as deleted.
 *
 *	On Entry:
 *	    id		Inode/generation number of deleted file.
 *	    parent	Inode/generation number of parent directory.
 *	    T_inode	Inode database entry.
 *	    T_path	Path(s) database entry table.
 *	    n_path	Number of entries in T_path.
 *	    delete_time	Deletion time.
 */
void
delete(
	sam_id_t id,
	sam_id_t parent,
	sam_db_inode_t *T_inode,
	sam_db_path_t *T_path,
	int n_path,
	time_t delete_time)
{
	sam_db_path_t *p;
	sam_db_path_t *T_parent;
	my_ulonglong n;
	int i, l;

	T_parent = NULL;

	/* If single entry with no parent information, mark deleted	*/
	p = T_path;
	if (parent.ino == 0 && n_path == 1) {
		goto del;
	}

	n = sam_db_query_path(&T_parent, parent.ino, parent.gen);

	if (n == 0) {
		Trace(TR_ERR, "No path entry for parent, ino=%u.%u",
		    parent.ino,	parent.gen);
		goto del;
	}

	if (n > 1) {
		Trace(TR_ERR, "Multiple path entries for parent, ino=%u.%u",
		    parent.ino, parent.gen);
	}

	for (n = 0; n < n_path; n++) { /* Search path table	*/
		p = T_path + n;
		i = strlen(T_parent->path);
		if (strncmp(p->path, T_parent->path, i) != 0) {
			continue;
		}
		l = strlen(T_parent->obj);
		if (strncmp(p->path+i, T_parent->obj, l) != 0) {
			continue;
		}
		if (strcmp(p->path+i+l, "/") == 0) {
			break;
		}
	}

	SamFree(T_parent);

	if (n == n_path) { /* If not found	*/
		Trace(TR_ERR, "No path matching parent, ino=%u.%u",
		    id.ino, id.gen);
		return;
	}

del:
	p->deleted = 1;
	p->delete_time = delete_time;
	(void) sam_db_delete_path(p, delete_time);
	for (n = 0; n < n_path; n++) {
		p = T_path + n;
		if (!p->deleted) {
			break;
		}
	}

	/* If all path entries deleted	*/
	if (n == n_path) {
		(void) sam_db_delete_inode(T_inode, delete_time);
	}
}

/*
 * free_path - Free Path Table.
 *
 *	On Entry:
 *	    t		Path table.
 *	    n		Number of entries in table.
 */
void
free_path(
	sam_db_path_t *t,	/* Path table */
	int n)			/* Number of items in table */
{
	int i;

	if (t == NULL) {
		return;
	}

	for (i = 0; i < n; i++) {
		SamFree((t+i)->path);
		SamFree((t+i)->obj);
		SamFree((t+i)->initial_path);
		SamFree((t+i)->initial_obj);
	}

	SamFree(t);
}

/*
 * free_link - Free Link Table.
 *
 *	On Entry:
 *	    t		Link table.
 *	    n		Number of entries in table.
 */
void free_link(
	sam_db_link_t *t,	/* Path table */
	int n) 			/* Number of items in table */
{
	int i;

	if (t == NULL) {
		return;
	}

	for (i = 0; i < n; i++) {
		SamFree((t+i)->link);
	}

	SamFree(t);
}

/*
 * 	idstat - Issue File Status Request.
 *
 *	Description:
 *	    Get file inode.
 *
 *	On Entry:
 *	    ino		Inode number.
 *	    gen		Generation number.
 *
 *	On Return:
 *	    inode	Inode image.
 *
 *	Returns:
 *	    Zero if no errors, -1 if error.
 */
int
idstat(
	unsigned int ino,	/* Inode number for query */
	unsigned int gen, /* Generation number for query */
	struct sam_perm_inode *inode) /* Perminant inode */
{
	struct sam_ioctl_idstat request; /* Stat request */

	request.id.ino = ino;
	request.id.gen = gen;
	request.size = sizeof (struct sam_perm_inode);
	request.dp.ptr = (void *)inode;

	if (ioctl(SAMDB_fd, F_IDSTAT, &request) < 0) {
		return (-1);
	}
	return (0);
}

/*
 * 	idmva - Issue Multivolume Archive Request.
 *
 *	Description:
 *	    Get multivolume archive information.
 *
 *	On Entry:
 *	    ino		Inode number.
 *	    gen		Generation number.
 *	    inode	Inode image.
 *
 *	On Return:
 *	    vsnp	VSN table pointer (dynamicly allocated).
 *			NULL if no VSNs returend.
 *
 *	Returns:
 *	    Number of VSNs in multivolume archives. -1 if error.
 *	    -2 if inode version < V2.
 */
int
idmva(
	unsigned int ino,		/* Inode number for query */
	unsigned int gen,		/* Generation number for query */
	struct sam_perm_inode *inode,	/* Perminant inode */
	struct sam_vsn_section **vsnp)	/* VSN table */
{
	struct sam_ioctl_idmva request; /* Multi-volume archive request	*/
	int n_mva; /* Number of VSNs */
	int i;

	*vsnp = NULL;
	n_mva = 0;

	for (i = 0; i < MAX_ARCHIVE; i++) {
		if (inode->ar.image[i].n_vsns > 1) {
			n_mva += inode->ar.image[i].n_vsns;
		}
	}

	if (n_mva == 0) {
		return (0);
	}

	if (inode->di.version < SAM_INODE_VERS_2) {
		errno = ENOTSUP;
		return (-2);
	}

	SamMalloc(*vsnp, n_mva * sizeof (struct sam_vsn_section));
	request.id.ino = ino;
	request.id.gen = gen;
	request.size = n_mva * sizeof (struct sam_vsn_section);
	request.buf.ptr = (void *)*vsnp;

	for (i = 0; i < MAX_ARCHIVE; i++) {
		request.aid[i].ino = request.aid[i].gen = 0;
	}

	if (ioctl(SAMDB_fd, F_IDMVA, &request) < 0) {
		return (-1);
	}
	return (n_mva);
}

/*
 * 	idfind - Find Inode in Directory using F_IDGETDENTS Request.
 *
 *	Description:
 *	    Search the given directory for the specified inode number
 *	    and return entry name. F_IDGETDENTS ioctl(2) request is used.
 *
 *	On Entry:
 *	    dir		Inode/generation number of directory to search.
 *	    id		Inode/generation number of file to find.
 *
 *	Returns:
 *	    Entry name, NULL if id not found.
 */
char *
idfind(
	sam_id_t dir,	/* Inode number of directory */
	sam_id_t id)	/* Inode number to locate */
{
	sam_ioctl_idgetdents_t request;	/* Getdents request		*/
	struct sam_dirent *dirp;
	char *dirbuf; 	/* Dirent buffer */
	char *limit; 	/* End of dirent buffer */
	char *name; 	/* Object name (returned) */
	int n;

	name = NULL;
	request.id = dir;
	request.size = 1000 * sizeof (struct sam_dirent);
	SamMalloc(dirbuf, request.size);
	request.dir.ptr = (void *) dirbuf;
	request.offset = 0;
	request.eof = 0;

	while (!request.eof) {
		if ((n = ioctl(SAMDB_fd, F_IDGETDENTS, &request)) < 0) {
			name = (char *)-1;
			goto exit;
		}

		dirp = (struct sam_dirent *)((void *)dirbuf);
		limit = dirbuf + n;

		while ((char *)dirp < limit) {
			if (dirp->d_fmt != 0 && dirp->d_id.ino == id.ino &&
			    dirp->d_id.gen == id.gen) {
				name = xstrdup2((char *)dirp->d_name,
				    (int)dirp->d_namlen);
				goto exit;
			}
			dirp = (struct sam_dirent *)((void *)((char *)dirp +
			    SAM_DIRSIZ(dirp)));
		}
	}

exit:
	SamFree(dirbuf);
	return (name);
}

char *
find_id(
	sam_id_t parent,	/* Directory ino/gen numbers */
	sam_id_t id)		/* Item to find */
{
	char *obj; /* Entry name */

	obj = idfind(parent, id);

	if (obj == NULL) {
		Trace(TR_ERR, "can't find inode, ino=%u.%u", id.ino, id.gen);
		return (NULL);
	}

	if (obj == (char *)-1) {
		Trace(TR_ERR, "can't idgetdents, ino=%u.%u",
		    parent.ino, parent.gen);
		return (NULL);
	}

	return (obj);
}

/*
 * 	idfindall - Find All Occurances of Inode in Directory with F_IDGETDENTS.
 *
 *	Description:
 *	    Search the given directory for all occurances of the specified
 *	    inode number and return a list of entry name(s).  F_IDGETDENTS
 *	    ioctl(2) request is used.
 *
 *	On Entry:
 *	    dir		Inode/generation number of directory to search.
 *	    id		Inode/generation number of file to find.
 *	    objs	Entry names list.  Entry names are dynamicly
 *			allocated.
 *	    n_obj	Maximum number of entries in list.
 *
 *	Returns:
 *	    Number of entries found, -1 if error.
 */
int
idfindall(
	sam_id_t dir,	/* Inode number of directory */
	sam_id_t id, 	/* Inode number to locate */
	char **objs,	/* Objects found */
	int n_obj)		/* Maximum number of objects */
{
	sam_ioctl_idgetdents_t request;	/* Getdents request */
	struct sam_dirent *dirp;
	char *dirbuf; 	/* Dirent buffer */
	char *limit; 	/* End of dirent buffer */
	int found; 		/* Number of matches found */
	int n;

	found = 0;
	request.id = dir;
	request.size = 1000 * sizeof (struct sam_dirent);
	SamMalloc(dirbuf, request.size);
	request.dir.ptr = (void *)dirbuf;
	request.offset = 0;
	request.eof = 0;

	while (!request.eof) {
		if ((n = ioctl(SAMDB_fd, F_IDGETDENTS, &request)) < 0) {
			found = -1;
			goto exit;
		}

		dirp = (struct sam_dirent *)((void *)dirbuf);
		limit = dirbuf + n;

		while ((char *)dirp < limit) {
			if (dirp->d_fmt != 0 && dirp->d_id.ino == id.ino &&
			    dirp->d_id.gen == id.gen) {
				if (found >= n_obj) {
					goto exit;
				}
				/* List full */
				objs[found++] = xstrdup2((char *)dirp->d_name,
				    (int)dirp->d_namlen);
			}
			dirp = (struct sam_dirent *)((void *)((char *)dirp +
			    SAM_DIRSIZ(dirp)));
		}
	}

exit:
	SamFree(dirbuf);
	return (found);
}

/*
 * 	find_all_ids - Find All Occurances of Inode for Parent Inode.
 *
 *	Description:
 *	    Search the given parent for all occurances of the specified
 *	    inode number and return a list of entry name(s).
 *
 *	On Entry:
 *	    parent	Parent inode/generation number of directory to search.
 *	    id		Inode/generation number of file to find.
 *	    objs	Entry names list.  Entry names are dynamicly
 *			allocated.
 *	    n_obj	Maximum number of entries in list.
 *
 *	Returns:
 *	    Number of entries found.
 */
int
find_all_ids(
	sam_id_t parent,	/* Directory ino/gen numbers */
	sam_id_t id, 		/* Item to find */
	char **objs, 		/* Object(s) list */
	int n_obj)
{
	int n;

	for (n = 0; n < n_obj; n++) {
		objs[n] = NULL;
	}

	n = idfindall(parent, id, objs, n_obj);

	if (n == 0) {
		Trace(TR_ERR, "can't find inode, ino=%u.%u", id.ino, id.gen);
		return (0);
	}

	if (n == -1) {
		Trace(TR_ERR, "can't idgetdents, ino=%u.%u",
		    parent.ino, parent.gen);
		return (0);
	}

	return (n);
}

/*
 * 	verify_inode - Verify Inode Matches.
 *
 *	Description;
 *	    Verify that inode/generation numbers and file type match
 *	    for a given inode image.
 *
 *	On Entry:
 *	    inode	Inode image.
 *	    t		Expected inode information.
 *
 *	Returns:
 *	    Zero if match,
 *	    1 if inode/generation number mismatch,
 *	    2 if file type mismatch.
 */
int
verify_inode(
	struct sam_perm_inode *inode,	/* Perminant inode */
	sam_db_inode_t *t)		/* Inode db entry */
{
	sam_db_ftype ftype;	/* File type */

	if (inode->di.id.ino != t->ino ||
	    inode->di.id.gen != t->gen) {
		Trace(TR_ERR, "Inode mismatch inode=%u.%u db=%u.%u",
		    inode->di.id.ino, inode->di.id.gen, t->ino, t->gen);
		return (1);
	}

	ftype = sam_db_get_ftype(inode);

	if (ftype != t->type) {
		Trace(TR_ERR, "Inode type mismatch for "
		    "ino=%u.%u inode=%d db=%d", t->ino,
		    t->gen, ftype, t->type);
		return (2);
	}

	return (0);
}

/*
 * 	sam_db_delete_inode - Mark Inode as Deleted.
 *
 *	On Entry:
 *	    t		Inode to mark deleted.
 *	    delete_time	Time inode was deleted.
 *
 *	Returns:
 *	    Number of entries updated (i.e, 0 if already current,
 *	    1 if database updated). -1 if error.
 */
my_ulonglong
sam_db_delete_inode(
	sam_db_inode_t *t,		/* Inode db entry */
	time_t delete_time)	/* Deletion time */
{
	unsigned int ino;	/* Inode number for query */
	unsigned int gen;	/* Generation number for query */
	my_ulonglong rc;	/* Row count */
	char *q;

	ino = t->ino;
	gen = t->gen;

	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, " SET");

	sprintf(q, " deleted='1', delete_time='%lu'", delete_time);
	q = strend(q);

	sprintf(q, " WHERE ino=%u AND gen=%u;", t->ino, t->gen);
	q = strend(q);
	*q = '\0';

	if (SAMDB_Verbose) {
		printf("QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail: %s", mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc == 0) {
		Trace(TR_ERR, "inode delete: no rows affected "
		    "ino=%u.%u", ino, gen);
	}

	if (rc > 1) {
		Trace(TR_ERR, "inode delete: multiple rows affected "
		    "ino=%u.%u", ino, gen);
	}

	return (rc);
}

/*
 * 	sam_db_delete_path - Mark Path as Deleted.
 *
 *	On Entry:
 *	    t		Inode to mark deleted.
 *	    delete_time	Time inode was deleted.
 *
 *	Returns:
 *	    Number of entries updated (i.e, 0 if already current,
 *	    1 if database updated). -1 if error.
 */
my_ulonglong
sam_db_delete_path(
	sam_db_path_t *t,		/* Inode db entry */
	time_t delete_time)	/* Deletion time */
{
	unsigned int ino;	/* Inode number for query */
	unsigned int gen;	/* Generation number for query */
	my_ulonglong rc;	/* Row count */
	char *q;

	ino = t->ino;
	gen = t->gen;

	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, " SET");

	sprintf(q, " deleted='1', delete_time='%lu'", delete_time);
	q = strend(q);

	sprintf(q, " WHERE ino=%u AND gen=%u AND path='", t->ino, t->gen);
	q = strend(q);

	q += mysql_real_escape_string(SAMDB_conn, q, t->path, strlen(t->path));
	q = strend(q);

	q = strmov(q, "' AND obj='");
	q += mysql_real_escape_string(SAMDB_conn, q, t->obj, strlen(t->obj));
	q = strend(q);
	q = strmov(q, "';");
	*q = '\0';

	if (SAMDB_Verbose) {
		printf("QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail: %s", mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc == 0) {
		Trace(TR_ERR, "path delete: no rows affected"
		    " ino=%u.%u", ino, gen);
	}

	if (rc > 1) {
		Trace(TR_ERR, "path delete: multipe rows affected"
		    " ino=%u.%u", ino, gen);
	}

	return (rc);
}

/*
 * 	update_inode - Update Inode Information.
 *
 *	Description:
 *	    Update database to reflect inode information.
 *
 *	On Entry:
 *	    inode	Inode image.
 *	    t		Link database entry table.
 *
 *	Returns:
 *	    Number of inode records updated, -1 if error.
 */
my_ulonglong
update_inode(
	struct sam_perm_inode *inode,	/* Perminant inode */
	sam_db_inode_t *t)		/* Inode db entry */
{
	unsigned int ino;	/* Inode number for query */
	unsigned int gen; 	/* Generation number for query */
	my_ulonglong rc; 	/* Row count */
	char *q;
	int k;

	ino = t->ino;
	gen = t->gen;

	k = 0;
	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, " SET");

	if (t->size != inode->di.rm.size) {
		k++;
		sprintf(q, " size='%llu'", inode->di.rm.size);
		q = strend(q);
	}

	if (t->create_time != inode->di.creation_time) {
		if (k++ != 0) {
			q = strmov(q, ",");
		}
		sprintf(q, " create_time='%u'", inode->di.creation_time);
		q = strend(q);
		Trace(TR_ERR, "create time changing for ino=%u.%u", ino, gen);
	}

	if (t->modify_time != inode->di.modify_time.tv_sec) {
		if (k++ != 0) {
			q = strmov(q, ",");
		}
		sprintf(q, " modify_time='%u'", inode->di.modify_time.tv_sec);
		q = strend(q);
	}

	if (t->uid != inode->di.uid) {
		if (k++ != 0) {
			q = strmov(q, ",");
		}
		sprintf(q, " uid='%lu'", inode->di.uid);
		q = strend(q);
	}

	if (t->gid != inode->di.gid) {
		if (k++ != 0) {
			q = strmov(q, ",");
		}
		sprintf(q, " gid='%lu'", inode->di.gid);
		q = strend(q);
	}

	/* If nothing to update	*/
	if (k == 0) {
		if (SAMDB_Verbose) {
			Trace(TR_ERR, "inode entry already current ino=%u.%u",
			    ino, gen);
		}
		return (0);
	}

	sprintf(q, " WHERE ino=%u AND gen=%u;", t->ino, t->gen);
	q = strend(q);
	*q = '\0';

	if (SAMDB_Verbose) {
		printf("QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail: %s", mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc == 0) {
		Trace(TR_ERR, "inode update: no rows affected "
		    "ino=%u.%u", ino, gen);
	}

	if (rc > 1) {
		Trace(TR_ERR, "inode update: multipe rows affected "
		    "ino=%u.%u", ino, gen);
	}

	return (rc);
}

/*
 * 	update_path - Update Path Information.
 *
 *	Description:
 *	    Update database to reflect path information.  If path is
 *	    NULL, only the object (file) name is changing.
 *
 *	On Entry:
 *	    t		Link database entry table.
 *	    path	New path name, NULL if path unchanged.
 *	    obj		New object name.
 *
 *	Returns:
 *	    Number of path records updated, -1 if error.
 */
my_ulonglong
update_path(
	sam_db_path_t *t,	/* Inode db entry */
	char *path,		/* New path name */
	char *obj)		/* New object (file) name */
{
	unsigned int ino;	/* Inode number for query */
	unsigned int gen;	/* Generation number for query */
	my_ulonglong rc;	/* Row count */
	char *q;

	ino = t->ino;
	gen = t->gen;

	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, " SET ");
	if (path != NULL) {
		q = strmov(q, "path='");
		q += mysql_real_escape_string(SAMDB_conn, q,
		    path, strlen(path));
		q = strmov(q, "', ");
	}
	q = strmov(q, "obj='");
	q += mysql_real_escape_string(SAMDB_conn, q, obj, strlen(obj));

	sprintf(q, "' WHERE ino=%u AND gen=%u AND path='", t->ino, t->gen);
	q = strend(q);

	q += mysql_real_escape_string(SAMDB_conn, q, t->path, strlen(t->path));
	q = strmov(q, "' AND obj='");
	q += mysql_real_escape_string(SAMDB_conn, q, t->obj, strlen(t->obj));
	q = strmov(q, "';");
	*q = '\0';

	if (SAMDB_Verbose) {
		printf("QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int) (q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail: %s", mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc == 0) {
		Trace(TR_ERR, "path update: no rows affected "
		    "ino=%u.%u", ino, gen);
	}

	if (rc > 1) {
		Trace(TR_ERR, "path update: multipe rows affected "
		    "ino=%u.%u", ino, gen);
	}

	return (rc);
}

/*
 * 	update_link - Update Link Information.
 *
 *	Description:
 *	    Update database to reflect link information.
 *
 *	On Entry:
 *	    t		Link database entry table.
 *
 *	Returns:
 *	    Number of link records updated, -1 if error.
 */
my_ulonglong
update_link(
	sam_db_link_t *t, 		/* Link db entry */
	char *link)			/* Symbolic link value */
{
	unsigned int ino;	/* Inode number for query */
	unsigned int gen; 	/* Generation number for query */
	my_ulonglong rc; 	/* Row count */
	char *q;

	ino = t->ino;
	gen = t->gen;

	if (strcmp(t->link, link) == 0) {
		Trace(TR_ERR, "link entry already current "
		    "ino=%u.%u", ino, gen);
		return (0);
	}

	q = strmov(SAMDB_qbuf, "UPDATE ");
	q = strmov(q, T_SAM_LINK);
	q = strmov(q, " SET link='");
	q += mysql_real_escape_string(SAMDB_conn, q, link, strlen(link));

	sprintf(q, "' WHERE ino=%u AND gen=%u;", t->ino, t->gen);
	q = strend(q);
	*q = '\0';

	if (SAMDB_Verbose) {
		printf("QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		Trace(TR_ERR, "select fail: %s", mysql_error(SAMDB_conn));
		return ((my_ulonglong)-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc == 0) {
		Trace(TR_ERR, "link update: no rows affected "
		    "ino=%u.%u", ino, gen);
	}

	if (rc > 1) {
		Trace(TR_ERR, "link update: multipe rows affected "
		    "ino=%u.%u", ino, gen);
	}

	return (rc);
}

/*
 * 	update_archive - Update Archive Information.
 *
 *	Description:
 *	    Update database to reflect archive information for the
 *	    given archive copy.
 *
 *	On Entry:
 *	    inode	Inode image.
 *	    vsnp	Multivolume archive VSN informaiton.
 *	    copy	Archive copy.
 *	    t		Archive database entry table.
 *	    n		Number of entries in archive table.
 *
 *	Returns:
 *	    Number of archive records added if no errors, -1 if error.
 */
my_ulonglong
update_archive(
	struct sam_perm_inode *inode,	/* Perminant inode */
	sam_vsn_section_t *vsnp, /* Multivolume archive VSN list */
	int copy, /* Copy number */
	sam_db_archive_t *t, /* Archive db entry */
	my_ulonglong n) /* Number of archive entries	*/
{
	sam_db_archive_t *v; /* Current archive entry	*/
	sam_db_archive_t new; /* New archive entry		*/
	char *media; /* Media type			*/
	char *vsn; /* VSN				*/
	int vsn_id; /* VSN identifier		*/
	int nv; /* Number of VSNs this archive	*/
	int i, k, s;

	media = sam_mediatoa(inode->di.media[copy-1]);
	nv = inode->ar.image[copy-1].n_vsns;

	for (s = k = 0; s < nv; s++) {
		vsn = nv > 1 ? vsnp[s].vsn : inode->ar.image[copy-1].vsn;
		vsn_id = sam_db_find_vsn(copy, media, vsn);

		if (vsn_id <= 0) { /* If VSN not in cache	*/
			vsn_id = sam_db_insert_vsn(media, vsn);
			if (vsn_id <= 0) {
				return ((my_ulonglong) -1);
			}
			sam_db_cache_vsn(vsn_id, media, vsn);
		}

		for (i = 0; i < n; i++) { /* Search archive table	*/
			v = t + i;
			if (v->copy != copy || v->seq != s+1 ||
			    v->vsn_id != vsn_id) {
				continue;
			}

			if (v->create_time !=
			    inode->ar.image[copy-1].creation_time) {
				Trace(TR_ERR, "archive entry with creation"
				    " time mismatch, ino=%u.%u, c=%d,"
				    " s=%d, vsn=%s:%s", v->ino, v->gen,
				    s, copy, media, vsn);
			}
			break;
		}

		if (i < n) {
			continue;
		}

		new.ino = inode->di.id.ino;
		new.gen = inode->di.id.gen;
		new.copy = copy;
		new.seq = s+1;
		new.vsn_id = vsn_id;
		new.size = nv > 1 ? vsnp[s].length : inode->di.rm.size;
		new.create_time = inode->ar.image[copy-1].creation_time;
		new.modify_time = inode->di.ar_flags[copy-1] & AR_stale ?
		    new.create_time	: inode->di.modify_time.tv_sec;

		if (sam_db_insert_archive(&new,
		    (time_t)inode->di.modify_time.tv_sec) >= 0) {
			k++;
		}
	}

	return ((my_ulonglong) k);
}

/*
 * 	update_archive_status - Update Archive Status.
 *
 *	Description:
 *	    Update database to identify archive entries as either
 *	    stale or current.
 *
 *	On Entry:
 *	    inode	Inode image.
 *	    t		Archive database entry table.
 *	    n		Number of entries in archive table.
 *
 *	Returns:
 *	    Number of entries updated, , -1 if errors.
 */
my_ulonglong
update_archive_status(
	struct sam_perm_inode *inode, /* Perminant inode */
	sam_db_archive_t *t, 	/* Archive db entry */
	my_ulonglong n) 	/* Number of archive entries */
{
	my_ulonglong rc; 	/* Row count */
	my_ulonglong tc; 	/* Total row count */
	sam_db_archive_t *v; 	/* Current archive entry */
	int stale; 			/* Stale status */
	char *q;
	int i;

	tc = 0;

	for (i = 0; i < n; i++) {
		v = t + i;
		stale = v->modify_time !=
		    inode->di.modify_time.tv_sec ? 1 : 0;
		if (v->stale == stale) {
			continue;
		}

		q = strmov(SAMDB_qbuf, "UPDATE ");
		q = strmov(q, T_SAM_ARCHIVE);
		q = strmov(q, " SET");
		sprintf(q, " stale='%d'", stale);
		q = strend(q);
		sprintf(q, " WHERE ino=%u AND gen=%u AND copy=%d AND seq=%d"
		    " AND vsn_id=%d", t->ino, t->gen, t->copy,
		    t->seq, t->vsn_id);
		q = strend(q);
		q = strmov(q, ";");
		*q = '\0';

		if (SAMDB_Verbose) {
			Trace(TR_DEBUG, "QUERY: %s\n", SAMDB_qbuf);
		}

		if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
		    (unsigned int)(q-SAMDB_qbuf))) {
			Trace(TR_ERR, "select fail: %s",
			    mysql_error(SAMDB_conn));
			return ((my_ulonglong)-1);
		}

		rc = mysql_affected_rows(SAMDB_conn);

		if (rc == 0) {
			Trace(TR_ERR, "archive update: no rows affected "
			    "ino=%u.%u", t->ino, t->gen);
		}

		if (rc > 1) {
			Trace(TR_ERR, "archive update: multipe rows affected "
			    "ino=%u.%u", t->ino, t->gen);
		}
		tc += rc;
	}

	return (tc);
}

char *
sam_db_id_to_path(
	sam_id_t id, 		/* Inode/generation number */
	char *mount_point) 	/* Mount point */
{
	my_ulonglong n_path;
	sam_db_path_t *T_path;
	char *path; /* File path */
	char *fullpath; /* Full path/name */
	char *obj; /* Object name */
	struct sam_perm_inode inode;
	sam_db_path_t *t;
	my_ulonglong n;
	int i, l;

	if (id.ino == SAMDB_INO_MOUNT_POINT) {
		SamMalloc(path, 2);
		*path = '\0';
		return (path);
	}

	path = NULL;
	T_path = NULL;
	n_path = sam_db_query_path(&T_path, id.ino, id.gen);

	for (n = 0; n < n_path; n++) {
		t = T_path + n;
		if (!t->deleted) {
			break;
		}
	}

	if (n < n_path) { /* Active entry found	*/
		l = strlen(t->path) + strlen(t->obj) + 2;
		SamMalloc(path, l);
		strcpy(path, t->path);
		strcat(path, t->obj);
		strcat(path, "/");
		goto free1;
	}

	i = idstat(id.ino, id.gen, &inode);

	if (i < 0) {
		Trace(TR_ERR, "can't stat ino=%u.%u", id.ino, id.gen);
		goto free1;
	}

	if (!S_ISDIR(inode.di.mode)) {
		Trace(TR_ERR, "not directory ino=%u.%u", id.ino, id.gen);
		goto free1;
	}

	path = sam_db_id_to_path(inode.di.parent_id, mount_point);

	if (path == NULL) {
		goto free1;
	}

	SamMalloc(fullpath, strlen(mount_point) + strlen(path) + 2);
	strcpy(fullpath, mount_point);
	strcat(fullpath, "/");
	strcat(fullpath, path);

	obj = idfind(inode.di.parent_id, id);
	if (obj == NULL) {
		Trace(TR_ERR, "can't find inode in %s, ino=%u.%u",
		    fullpath, id.ino, id.gen);
		goto close1;
	}

	if (obj == (char *)-1) {
		Trace(TR_ERR, "can't idgetdents for %s, ino=%u.%u", fullpath,
		    inode.di.parent_id.ino, inode.di.parent_id.gen);
		goto close1;
	}

	l = strlen(path) + strlen(obj) + 2;
	SamRealloc(path, l);
	strcat(path, obj);
	strcat(path, "/");
	SamFree(obj);

close1:
	SamFree(fullpath);

free1:
	if (T_path != NULL) { /* Free path table	*/
		for (n = 0; n < n_path; n++) {
			t = T_path + n;
			SamFree(t->path);
			SamFree(t->obj);
			SamFree(t->initial_path);
			SamFree(t->initial_obj);
		}
		SamFree(T_path);
	}

	return (path);
}
