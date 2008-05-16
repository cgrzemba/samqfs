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

#pragma ident "$Revision: 1.15 $"


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <strings.h>
#include <libgen.h>
#include <sys/vfs.h>
#include <sys/acl.h>
#include <sam/fs/ino.h>
#include <errno.h>
#include <string.h>
#include <pub/rminfo.h>
#include <pub/lib.h>
#include <sam/resource.h>
#include <sam/fioctl.h>
#include <sam/uioctl.h>
#include <sam/types.h>
#include <sam/fs/dirent.h>
#include "src/fs/cmd/dump-restore/csd_defs.h"
#include <sam/fs/sblk.h>
#include "sam/nl_samfs.h"
#include "sam/lib.h"
#include "mgmt/cmn_csd.h"

static void common_allocate_dirent_space(struct sam_dirent **);
static void common_sam_set_dirent(char *, struct sam_dirent *);
static int common_sam_open_directory(char *, int);
static void common_stack_permissions(char *, struct sam_perm_inode *,
		permstackvars_t *);
static void common_stack_times(char *, struct sam_perm_inode *,
		timestackvars_t *);

#if !defined(KEEP_STATISTICS)
#ifdef	BUMP_STAT
#undef	BUMP_STAT
#endif
#define	BUMP_STAT(field) {}
#define	ADD_STAT(field, val) {}
#else
#define	ADD_STAT(field, val) {csd_statistics.field += value; }
#endif

/* globals */


/*
 * fast lookup array to determine the number of copies existing without
 * having to individually shift and mask bits
 */
int copy_array[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

/*
 * ----- print_reslog - print a log for the benefit of restore.sh.
 */
void
common_print_reslog(
	FILE 			*log_st,
	char			*pathname,
	struct sam_perm_inode	*pid,
	char			*status)
{
	int copy;
	char type;

	if (log_st == NULL) {
		return;
	}
	if (S_ISDIR(pid->di.mode))		type = 'd';
	else if (S_ISSEGI(&pid->di))	type = 'I';
	else if (S_ISSEGS(&pid->di))	type = 'S';
	else if (S_ISREQ(pid->di.mode)) type = 'R';
	else if (S_ISLNK(pid->di.mode)) type = 'l';
	else if (S_ISREG(pid->di.mode)) type = 'f';
	else if (S_ISBLK(pid->di.mode)) type = 'b';
	else type = '?';

	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		uint_t offset;

		if (pid->ar.image[copy].vsn[0] != '\0') {
			offset = pid->ar.image[copy].file_offset;
			if ((pid->ar.image[copy].arch_flags &
			    SAR_size_block) == 0) {
				offset >>= 9;
			}
			fprintf(log_st, "%c %s.%s %x.%x %s %s\n",
			    type,
			    sam_mediatoa(pid->di.media[copy]),
			    pid->ar.image[copy].vsn,
			    pid->ar.image[copy].position,
			    offset,
			    status,
			    pathname);
			break;
		}
	}
}

/*
 * ----- sam_restore_a_file -  CSD restore.
 *	Restore a single file to the filesystem from a csd image.
 */

void
common_sam_restore_a_file(
	gzFile			gzf,		/* open stream to dump file */
	char			*path,		/* the path name of the file */
	struct sam_perm_inode	*perm_inode,
	struct sam_vsn_section	*vsnp,
	char			*link,		/* symlink resolution, if any */
	void			*data,		/* resource file info, if any */
	int			n_acls,		/* number of acl entries */
	aclent_t		*aclp,		/* acl entries, if any */
	int			dflag,		/* file has associated data */
	int			*dread,		/* assoc. data has been read */
	int			*in_dir_fd,	/* directory file desc */
	replace_t		rtype, 		/* how to handle conflicts */
	timestackvars_t		*tvars,		/* time stack handle */
	permstackvars_t		*pvars)		/* permissions stack handle */
{
	struct sam_ioctl_idrestore 	idrestore = { NULL, NULL, NULL, NULL };
	char				*basename = strrchr(path, '/');
	int 				entity_fd = -1;
	int 				copy;
	int 				force_writable = 0;
	u_longlong_t 			file_write_size;
	mode_t 				mode;
	struct sam_dirent		*namep = NULL;

	/*	don't do much for "..".  */
	if (basename != NULL) {
		if ((strcmp(basename, "/..") == 0)) {
			return;
		} else if (strcmp(basename, "/.") == 0) {
			return;
		}
		basename++;				/* skip / */
	} else {
		basename = path;
	}

	mode = perm_inode->di.mode;

	if (rtype == REPLACE_ALWAYS) {
		if (S_ISDIR(mode)) {
			(void) rmdir(path);
		} else if (!S_ISSEGS(&perm_inode->di)) {
			unlink(path);
		}
	}

	/* Notify user of previously or newly damaged files */
	if (perm_inode->di.status.b.damaged) {
		BUMP_STAT(file_damaged);
		csd_error(0, 0, catgets(catfd, SET, 13513,
		    "%s: File data was not recoverable prior to dump "
		    "(file is marked damaged)."), path);
	} else if ((S_ISREG(mode) && !S_ISSEGI(&perm_inode->di)) &&
	    (perm_inode->di.arch_status == 0) &&
	    (perm_inode->di.rm.size != 0) &&
	    (dflag == 0)) {
		int copy;
		boolean_t inconsistent = FALSE;
		boolean_t stale = FALSE;
		uint_t *arp = (uint_t *)(void *)&perm_inode->di.ar_flags[0];

		/* These stale entries should not be rearchived */
		*arp &= ~((AR_rearch<<24)|(AR_rearch<<16)|(AR_rearch<<8)
		    |AR_rearch);

		/* If inode has inconsistent copies, mark copy as archived */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			/* XXX missing code XXX */
		}
		/* If inode has stale copies, file is still marked damaged */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if ((perm_inode->di.ar_flags[copy] & AR_inconsistent) &&
			    (perm_inode->ar.image[copy].vsn[0] != 0)) {
				inconsistent = TRUE;
				perm_inode->di.arch_status |= (1 << copy);
			}
			if ((perm_inode->di.ar_flags[copy] & AR_stale) &&
			    (perm_inode->ar.image[copy].vsn[0] != 0)) {
				stale = TRUE;
			}
		}
		if (inconsistent) {
			*arp &= ~((AR_inconsistent<<24)|(AR_inconsistent<<16)|
			    (AR_inconsistent<<8)|AR_inconsistent);
			csd_error(0, 0, catgets(catfd, SET, 5032,
			    "%s: File has inconsistent copy."), path);
		} else if (stale) {
			csd_error(0, 0, catgets(catfd, SET, 5017,
			"%s: File is now damaged, stale archive copy exists."),
			    path);
		} else {
			csd_error(0, 0, catgets(catfd, SET, 13512,
		"%s: File data cannot be recovered (file is marked damaged)."),
			    path);
		}
		if (!inconsistent) {
			BUMP_STAT(file_damaged);
			perm_inode->di.status.b.damaged = 1;
		}
	}

	if (rtype == REPLACE_WITH_NEWER) {
		struct stat64 st1;

		if ((lstat64(path, &st1) == 0) &&
		    (st1.st_mtim.tv_sec < perm_inode->di.modify_time.tv_sec)) {
			if (S_ISDIR(mode)) {
				(void) rmdir(path);
			} else if (!S_ISSEGS(&perm_inode->di)) {
				unlink(path);
			}
		}
	}

	/*
	 * Create the subject entity.	It might be one of a number of different
	 * things, each of which must be created slightly differently.
	 */
	if (S_ISSEGS(&perm_inode->di)) {
#ifdef	KEEP_STATISTICS
		int copy_bits;
#endif	/* KEEP_STATISTICS */
		int open_flg = (O_RDONLY | SAM_O_LARGEFILE);

		if ((entity_fd = open64(path, open_flg, mode & 07777)) < 0) {
			BUMP_STAT(errors);
			csd_error(0, errno, catgets(catfd, SET, 213,
			    "%s: cannot open()"), path);
			return;
		}
#ifdef	KEEP_STATISTICS
		BUMP_STAT(segments);
		copy_bits = perm_inode->di.arch_status & 0xF;
		ADD_STAT(file_archives, copy_array[copy_bits]);
#endif	/* KEEP_STATISTICS */

	} else if (S_ISREG(mode) || S_ISSEGI(&perm_inode->di)) {
#ifdef	KEEP_STATISTICS
		int	copy_bits;
#endif	/* KEEP_STATISTICS */
		int	restored = 0;
		int	open_flg = (O_CREAT | O_EXCL| SAM_O_LARGEFILE);

		if (!dflag &&			/* Attempt F_SAMRESTORE */
		    (perm_inode->di.nlink == 1) &&
		    (perm_inode->di.status.b.acl == 0) &&
		    !S_ISSEGI(&perm_inode->di)) {
			sam_ioctl_samrestore_t samrestore =
			    {NULL, NULL, NULL, NULL};

			/*
			 * If a nodata, non-hardlinked, non-access-control-
			 * listed regular file, just restore a file by name.
			 * Don't create a file, nor an open file descriptor.
			 */
			if (namep == NULL) {
				common_allocate_dirent_space(&namep);
				if (namep == NULL) {
					BUMP_STAT(errors);
					goto done;
				}
			}
			common_sam_set_dirent(basename, namep);
			*in_dir_fd = common_sam_open_directory(path,
			    *in_dir_fd);
			samrestore.name.ptr = namep;
			samrestore.lp.ptr = NULL;
			samrestore.dp.ptr = perm_inode;
			samrestore.vp.ptr = NULL;
			if (perm_inode->di.version >= SAM_INODE_VERS_2) {
				if (perm_inode->di.ext_attrs & ext_mva) {
					samrestore.vp.ptr = (void *)vsnp;
				}
			} else if (perm_inode->di.version == SAM_INODE_VERS_1) {
				sam_perm_inode_v1_t *perm_inode_v1 =
				    (sam_perm_inode_v1_t *)perm_inode;

				for (copy = 0; copy < MAX_ARCHIVE; copy++) {
					if (perm_inode_v1->aid[copy].ino) {
						samrestore.vp.ptr =
						    (void *)vsnp;
						break;
					}
				}
			}
			if (ioctl(*in_dir_fd, F_SAMRESTORE, &samrestore) < 0) {
				csd_error(0, errno, "%s: ioctl(F_SAMRESTORE)",
				    path);
			} else {
				restored = 1;
			}
		}
		if (restored == 0) {
			if (dflag) {
				open_flg |= O_WRONLY;
			}
			if ((entity_fd = open64(path, open_flg,
			    perm_inode->di.mode & 07777)) < 0) {
				BUMP_STAT(errors);
				csd_error(0, errno, catgets(catfd, SET, 215,
				    "%s: Cannot creat()"), path);
				goto done;
			}
		}

#ifdef	KEEP_STATISTICS
		BUMP_STAT(files);
		copy_bits = perm_inode->di.arch_status & 0xF;
		ADD_STAT(file_archives, copy_array[copy_bits]);
#endif	/* KEEP_STATISTICS */
		if (restored) {
			goto done;
		}
	} else if (S_ISDIR(mode)) {
		int open_flg = (O_RDONLY);

		if ((mode & S_IWUSR) == 0) {
			force_writable = 1;
			mode |= S_IWUSR;
			perm_inode->di.mode |= S_IWUSR;
		}
		if (mkdir(path, mode & 07777) < 0) {
			BUMP_STAT(errors);
			csd_error(0, errno,
			    catgets(catfd, SET, 222, "%s: Cannot mkdir()"),
			    path);
			goto done;
		}
		/* No need for SAM_O_LARGEFILE here, directories always < 2GB */
		if ((entity_fd = open64(path, open_flg)) < 0) {
			BUMP_STAT(errors);
			csd_error(0, errno, catgets(catfd, SET, 5010,
			    "%s: Cannot open() following mkdir()"), path);
			goto done;
		}
		BUMP_STAT(dirs);
	} else if (S_ISREQ(mode)) {
		struct sam_resource_file *rfp =
		    (struct sam_resource_file *)data;
		int open_flg = (O_WRONLY | O_CREAT |
		    O_EXCL | O_TRUNC | SAM_O_LARGEFILE);

		if (rfp->resource.revision != SAM_RESOURCE_REVISION) {
			BUMP_STAT(errors);
			csd_error(0, 0, catgets(catfd, SET, 5014,
			"%s: Unsupported revision for removable media info"),
			    path);
			goto done;
		}
		if ((entity_fd = open64(path, open_flg, mode & 07777)) < 0) {
			BUMP_STAT(errors);
			csd_error(0, errno, catgets(catfd, SET, 215,
			    "%s: Cannot creat()"), path);
			goto done;
		}
		idrestore.rp.ptr = (void *)data;
		BUMP_STAT(resources);
	} else if (S_ISLNK(mode)) {
		if ((perm_inode->di.nlink == 1) &&
		    (perm_inode->di.status.b.acl == 0)) {
			struct sam_ioctl_samrestore samrestore =
			    {NULL, NULL, NULL, NULL};

			/*
			 * If a symlink, just restore a file by name. Don't
			 * create a file, nor an open file descriptor.
			 */
			if (namep == NULL) {
				common_allocate_dirent_space(&namep);
				if (namep == NULL) {
					BUMP_STAT(errors);
					goto done;
				}
			}
			common_sam_set_dirent(basename, namep);
			*in_dir_fd = common_sam_open_directory(path,
			    *in_dir_fd);
			samrestore.name.ptr = namep;
			samrestore.lp.ptr = (void *)link;
			samrestore.dp.ptr = perm_inode;
			samrestore.vp.ptr = NULL;
			if (ioctl(*in_dir_fd, F_SAMRESTORE, &samrestore) < 0) {
				csd_error(0, errno, "%s: ioctl(F_SAMRESTORE)",
				    path);
				BUMP_STAT(errors);
			}
			BUMP_STAT(links);
			goto done;
		} else {
			int open_flg = (O_WRONLY | O_CREAT | O_EXCL | O_TRUNC);

			entity_fd = open64(path, open_flg, mode & 07777);
			if (entity_fd < 0) {
				BUMP_STAT(errors);
				csd_error(0, errno, catgets(catfd, SET, 215,
				    "%s: Cannot creat()"), path);
				goto done;
			}
			idrestore.lp.ptr = (void *)link;
			BUMP_STAT(links);
		}
	} else if (S_ISFIFO(mode) || S_ISCHR(mode) || S_ISBLK(mode)) {
		struct sam_ioctl_samrestore samrestore =
			{NULL, NULL, NULL, NULL};

		/*
		 * If a special, just restore a file by name. Don't create
		 * a file, nor an open file descriptor.
		 */
		if (namep == NULL) {
			common_allocate_dirent_space(&namep);
			if (namep == NULL) {
				BUMP_STAT(errors);
				goto done;
			}
		}
		common_sam_set_dirent(basename, namep);
		*in_dir_fd = common_sam_open_directory(path, *in_dir_fd);
		samrestore.name.ptr = namep;
		samrestore.lp.ptr = NULL;
		samrestore.dp.ptr = perm_inode;
		samrestore.vp.ptr = NULL;
		if (ioctl(*in_dir_fd, F_SAMRESTORE, &samrestore) < 0) {
			csd_error(0, errno, "%s: ioctl(F_SAMRESTORE)", path);
			BUMP_STAT(errors);
		}
		BUMP_STAT(specials);
		goto done;
	}
	if (dflag) {
		if (S_ISSEGI(&perm_inode->di)) {
			char ops[64];
			uint64_t seg_size = perm_inode->di.rm.info.dk.seg_size;
			uint_t stage_ahead = perm_inode->di.stage_ahead;

			/*
			 * File was segmented.  Make new file segmented so we
			 * can copy the data back.
			 */

			snprintf(ops, sizeof (ops), "l%llds%d",
			    seg_size * SAM_MIN_SEGMENT_SIZE, stage_ahead);
			if (sam_segment(path, ops) < 0) {
				csd_error(0, errno, "sam_segment(%s, %s)",
				    path, ops);
				BUMP_STAT(errors);
				goto done;
			}
		}
		if (S_ISREG(mode) && !(S_ISSEGS(&perm_inode->di))) {

			file_write_size = common_write_embedded_file_data(
			    gzf, path);
			ADD_STAT(data_dumped, file_write_size);
			*dread = 1;
			if (file_write_size != perm_inode->di.rm.size) {
				if (perm_inode->di.status.b.pextents == 0) {
					BUMP_STAT(errors);
					csd_error(0, errno,
					    catgets(catfd, SET, 13505,
					    "Error writing to data file %s"),
					    path);
					goto done;
				}
			}
		}
	}

	/*
	 * Pass the old inode to the filesystem; let it copy into the new
	 * inode that part of it which makes sense.
	 */
	idrestore.dp.ptr = (void *)perm_inode;
	if (perm_inode->di.version >= SAM_INODE_VERS_2) {
		if (perm_inode->di.ext_attrs & ext_mva) {
			idrestore.vp.ptr = (void *)vsnp;
		}
	} else if (perm_inode->di.version == SAM_INODE_VERS_1) {
		sam_perm_inode_v1_t *perm_inode_v1 =
		    (sam_perm_inode_v1_t *)perm_inode;

		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (perm_inode_v1->aid[copy].ino) {
				idrestore.vp.ptr = (void *)vsnp;
				break;
			}
		}
	}
	if (ioctl(entity_fd, F_IDRESTORE, &idrestore) < 0) {
		csd_error(0, errno, "%s: ioctl(F_IDRESTORE)", path);
		BUMP_STAT(errors);
	}

	/* Restore any access control list associated with file. */
	if (n_acls > 0) {
		if (facl(entity_fd, SETACL, n_acls, aclp) < 0) {
			BUMP_STAT(errors);
			csd_error(0, errno,
			    catgets(catfd, SET, 5026,
			    "%s: cannot write acl information"),
			    path);
		}
	}
	if (close(entity_fd) < 0) {
		csd_error(0, errno, "%s: close() ", path);
		BUMP_STAT(errors);
	}

	/*
	 *	Remember the correct inum and time values
	 *	for directories and segment indexes. The IDRESTORE
	 *	has already been called, which returns the correct
	 *	id for directories and segment indexes.
	 */
	if (S_ISDIR(perm_inode->di.mode) || S_ISSEGI(&perm_inode->di)) {
		common_stack_times(path, perm_inode, tvars);
	}

	/*
	 *	If a write was forced, remember the path and clear it
	 *	when all done.
	 */

	if (force_writable) {
		common_stack_permissions(path, perm_inode, pvars);
	}

done:
	if (namep != NULL) {
		free(namep);
	}
}


/*
 * ----- stack_times - remember inode times.
 *	Save the times associated with a directory or a segment file index.
 *	After all files are restored, we'll set the times into the directory or
 *	segment file index, because as we create entries, we are changing the
 *	times.
 */

void
common_stack_times(
	char				*path,
	struct sam_perm_inode 		*perm_inode,
	timestackvars_t			*tvars)
{
	struct sam_ioctl_idtime *idtime;

	if (tvars->time_stack_is_full) {
		return;
	}

	if (tvars->time_stack_next_free >= tvars->time_stack_size) {
		int new_time_size = sizeof (struct sam_ioctl_idtime) *
		    (tvars->time_stack_size + TIME_STACK_SIZE_INCREMENT);
		tvars->time_stack = (struct sam_ioctl_idtime *)
		    realloc(tvars->time_stack, new_time_size);

		/* may want to spill to disk, "next rev" */
		if (tvars->time_stack == NULL) {
			BUMP_STAT(errors);
			csd_error(0, errno, catgets(catfd, SET, 13518,
		"%s: Cannot malloc space for directory/segment index stack."),
			    path);
			csd_error(0, 0, catgets(catfd, SET, 13519,
	"Not all directory or segment index times will be set correctly."));
			tvars->time_stack_is_full = 1;
			return;
		}
		tvars->time_stack_size += TIME_STACK_SIZE_INCREMENT;
	}

	idtime = &tvars->time_stack[tvars->time_stack_next_free];
	idtime->id = perm_inode->di.id;
	idtime->atime = perm_inode->di.access_time.tv_sec;
	idtime->mtime = perm_inode->di.modify_time.tv_sec;
	idtime->xtime = perm_inode->di.creation_time;
	idtime->ytime = perm_inode->di.attribute_time;

	tvars->time_stack_next_free++;
}


/*
 * ----- pop_times_stack - unroll the times stack, doing the IDTIME
 *	as we go.
 */

void
common_pop_times_stack(
	int			fd,
	timestackvars_t		*tvars)
{
	int	i;

	while (tvars->time_stack_next_free > 0) {
		tvars->time_stack_next_free--;
		i = tvars->time_stack_next_free;

		if (ioctl(fd, F_IDTIME, &tvars->time_stack[i]) < 0) {
			BUMP_STAT(errors);
			csd_error(0, errno, catgets(catfd, SET, 1376,
			"inode %d.%d: Cannot ioctl(F_IDTIME)"),
			    tvars->time_stack[i].id.ino,
			    tvars->time_stack[i].id.gen);
		}
	}
	if (tvars->time_stack != NULL)  {
		free(tvars->time_stack);
		tvars->time_stack = NULL;
	}
}

/*
 * ----- stack_permissions - remember directory permissions.
 *	Save the permissions and pathname to any directory whose original
 *	permissions do not include a "owner writable" permission.
 *	After all files are restored, we'll change the permissions to
 *	those set at the time of the dump.
 */

static void
common_stack_permissions(
	char				*path,
	struct sam_perm_inode		*perm_inode,
	permstackvars_t			*pvars)
{
	perm_stack_t	*new_ent;

	if (pvars->perm_stack_is_full) {
		return;
	}

	if (pvars->perm_stack_next_free >= pvars->perm_stack_size) {
		int new_perm_size = sizeof (perm_stack_t) *
		    (pvars->perm_stack_size + PERM_STACK_SIZE_INCREMENT);
		pvars->perm_stack = (perm_stack_t *)
		    realloc(pvars->perm_stack, new_perm_size);

		/* may want to spill to disk, "next rev" */
		if (pvars->perm_stack == NULL) {
			BUMP_STAT(errors);
			csd_error(0, errno, catgets(catfd, SET, 13526,
		    "%s: Cannot malloc space for directory permission stack."),
			    path);
			csd_error(0, 0, catgets(catfd, SET, 13528,
		    "Not all directory permissions will be set correctly."));
			pvars->perm_stack_is_full = 1;
			return;
		}
		pvars->perm_stack_size += PERM_STACK_SIZE_INCREMENT;
	}

	new_ent = &pvars->perm_stack[pvars->perm_stack_next_free];
	new_ent->perm = perm_inode->di.mode & ~S_IWUSR;
	new_ent->perm &= 07777;		/* retain only permission bits */
	new_ent->path = strdup(path);
	if (new_ent->path == NULL) {
		BUMP_STAT(errors);
		csd_error(0, errno, catgets(catfd, SET, 13527,
		"%s: Cannot malloc space for directory permission path."),
		    path);
		csd_error(0, 0, catgets(catfd, SET, 13528,
		    "Not all directory permissions will be set correctly."));
		pvars->perm_stack_is_full = 1;
		return;
	}

	pvars->perm_stack_next_free++;
}


/*
 * ----- pop_permissions_stack - unroll the permissions stack, doing a chmod
 *	as we go.
 */

void
common_pop_permissions_stack(
	permstackvars_t		*pvars)
{
	perm_stack_t *ent;

	while (pvars->perm_stack_next_free > 0) {
		pvars->perm_stack_next_free--;
		ent = &(pvars->perm_stack[pvars->perm_stack_next_free]);
		if (chmod(ent->path, ent->perm) < 0) {
			BUMP_STAT(errors);
			csd_error(0, errno, catgets(catfd, SET, 13524,
			    "Cannot chmod directory %s"), ent->path);
		}
		free(ent->path);
		ent->path = NULL;
	}
	if (pvars->perm_stack != NULL)  {
		free(pvars->perm_stack);
		pvars->perm_stack = NULL;
	}
}

static void
common_allocate_dirent_space(struct sam_dirent **namep)
{
	if (*namep == NULL) {
		*namep = malloc(sizeof (struct sam_dirent) + MAXNAMELEN);
	}
}

static void
common_sam_set_dirent(char *np, struct sam_dirent *namep)
{
	memset(namep, 0, sizeof (struct sam_dirent));
	namep->d_reclen = namep->d_namlen = strlen(np);
	strlcpy((char *)namep->d_name, np, MAXNAMELEN);
}

static int
common_sam_open_directory(char *np, int open_dir_fd)
{
	static char	last_dir[MAXPATHLEN * 2];
	char		dir_name[MAXPATHLEN * 2];
	char		*dirp;

	strlcpy(dir_name, np, MAXPATHLEN * 2);

	dirp = dirname(dir_name);

	if (open_dir_fd <= 0 || strcmp(last_dir, dirp) != 0) {
		int new_fd;

		if ((new_fd = open64(dirp, O_RDONLY)) <= 0) {
			return (new_fd);
		}
		if (open_dir_fd > 0) {
			(void) close(open_dir_fd);
		}
		strcpy(last_dir, dirp);
		open_dir_fd = new_fd;
	}
	return (open_dir_fd);
}
