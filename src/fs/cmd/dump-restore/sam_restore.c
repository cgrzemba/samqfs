/*
 *	sam_restore.c
 *
 *	Work routines for csd restore.
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

#pragma ident "$Revision: 1.16 $"

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <malloc.h>
#include <strings.h>
#include <sys/stat.h>
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
#include <sam/sam_malloc.h>
#include <sam/fs/dirent.h>
#include <sam/fs/bswap.h>
#include "csd_defs.h"
#include <sam/fs/sblk.h>
#include "sam/nl_samfs.h"
#include "sam/lib.h"
#include "sam/checksum.h"

extern boolean_t unworm;
extern void conv_to_v2inode(sam_perm_inode_t *);
static void check_samfs_fs();
static void sam_restore_a_file(char *, struct sam_perm_inode *,
			struct sam_vsn_section *, char *, void *,
			int, aclent_t *, int dflag, int *dread);
static void allocate_dirent_space();
static boolean_t sam_check_nlink(sam_id_t id, char *path,
    struct sam_perm_inode *);
static void sam_res_by_name(char *, char *,
    struct sam_perm_inode *, struct sam_vsn_section *);
static void sam_set_dirent(char *);
static int sam_open_directory(char *);
static void pop_permissions_stack(void);
static void pop_times_stack(void);
static void stack_permissions(char *, struct sam_perm_inode *);
static void stack_times(char *, struct sam_perm_inode *);
static struct sam_dirent *namep;
static int open_dir_fd;
static csd_hardtbl_t *hardp = NULL;

static char *_SrcFile = __FILE__;

# define XA_CNT 3
const char *fn_xattr[XA_CNT]  =
	    {"user.DOSATTRIB", "user.SUNWattr_rw", "user.SUNWattr_ro"};
const int sl[XA_CNT] = {14, 16, 16};
boolean_t
is_xattr(const char* path) {

	if (csd_version < 7)
		return false;
	size_t len = strlen(path);
	if (len < 18)
		return false;
	for (int i = 0; i < XA_CNT; i++) {
		if (memcmp(fn_xattr[i], path + len - sl[i], sl[i]) == 0)
			return true;
	}
	return false;
}

/*
 * ----- cs_restore -  CSD restore.
 *	Read a csd dump file and restore selected files within.
 */

void
cs_restore(
	boolean strip_slashes,	/* should leading slash be stripped? */
	int fcount,		/* number of filename patterns in flist */
	char **flist)		/* filename patterns to select for restore  */
{
	int		file_data = 0;
	int		file_data_read;
	csd_fhdr_t file_hdr;
	char	name[MAXPATHLEN + 1];
	/* name to compare for selective rstr */
	char	*name_compare = &name[0];
	char	slink[MAXPATHLEN + 1];
	char	*save_path = (char *)NULL; /* Prev path opened for SAM_fd */
	char	*last_path = NULL;	/* Last compared path */
	char	*slash_loc;		/* Last loc of a '/' in filename */
	char	unused;			/* A safe value for slash_loc */
	int		namelen;
	int		skipping;
	struct sam_perm_inode perm_inode;
	struct sam_stat sb;
	struct sam_vsn_section *vsnp;
	void	*data;
	int		n_acls;
	aclent_t *aclp;

	/*
	 *	Read the dump file
	 */
	if (debugging == 1)
		quiet = false;
	while (csd_read_header(&file_hdr) > 0) {
		namelen = file_hdr.namelen;
		csd_read(name, namelen, &perm_inode);
		data = NULL;
		vsnp = NULL;
		aclp = NULL;
		n_acls = 0;
		if (file_hdr.flags & CSD_FH_DATA) {
			BUMP_STAT(data_files);
			file_data_read = 0;
			file_data++;
		} else {
			file_data_read = file_data = 0;
		}

		if (S_ISREQ(perm_inode.di.mode)) {
			SamMalloc(data, perm_inode.di.psize.rmfile);
		}

		csd_read_next(&perm_inode, &vsnp, slink, data, &n_acls, &aclp);

		if (dont_process_this_entry) {   /* if problem reading inode */
			BUMP_STAT(errors);
			goto skip_file;
		}

		/* Skip privileged files except root */

		if (SAM_PRIVILEGE_INO(perm_inode.di.version,
		    perm_inode.di.id.ino)) {
			if (perm_inode.di.id.ino != SAM_ROOT_INO) {
				goto skip_file;
			}
		}

		if (perm_inode.di.cs_algo >= CS_FUNCS) {
			error(0,0, catgets(catfd, SET, 13541,
			    "%s: unsupported checksum algo %d, resetting"),
			    name, perm_inode.di.cs_algo);
			perm_inode.di.cs_algo = 0;
			perm_inode.di.status.b.cs_gen = 0;
			perm_inode.di.status.b.cs_val = 0;
		}

		if (unworm && perm_inode.di.status.b.worm_rdonly) {
			perm_inode.di.status.b.worm_rdonly = 0;
			perm_inode.di.mode &= ~(04000); /* reset S bit in mode */
		}
		/* select subset of file names to restore. */

		name_compare = &name[0];
		if (fcount != 0) {	/* Search the file name list */
			int i;

			for (i = 0; i < fcount; i++) {
				/*
				 * If stripping first slash to make relative
				 * path and dump file name begins with '/' and
				 * select file name does not being with '/',
				 * then move name compare ahead past first '/'
				 */
				if (strip_slashes && ('/' == *name) &&
				    ('/' != *flist[i])) {
					name_compare = &name[1];
				}

				/*
				 * filecmp returns:
				 *	0 for no match
				 *	1 exact match,
				 *	2 name prefix of flist[i],
				 *	3 flist[i] prefix of name.
				 */
				if (filecmp(name_compare, flist[i]) != 0) {
					break;
				}
			}

			if (i >= fcount) {
				if (S_ISSEGI(&perm_inode.di)) {
					skipping = 1;
					goto skip_seg_file;
				}
				goto skip_file;
			}
		}
		if (is_xattr(name_compare))
			goto skip_file;

		/*
		 * If stripping first slash to make relative path and
		 *	dump file name begins with '/'
		 * then move location ahead past first '/'
		 */
		if (strip_slashes && ('/' == *name)) {
			name_compare = &name[1];
		}

		/*
		 * Make sure that we don't restore into a non-SAM-FS
		 * filesystem.
		 * If relative path to be restored, check current directory for
		 * being in a SAM-FS filesystem.  If absolute path to be
		 * restored, search backwards through path for a viable
		 * SAM-FS path by calling sam_stat() and checking the
		 * directory attributes.
		 */
		if ((strip_slashes && ('/' == *name)) || ('/' != *name)) {
			char *check_name = ".";
			char *next_name;

			if ((save_path != (char *)NULL) &&
			    (strcmp(check_name, save_path) != 0)) {
				check_samfs_fs();
				free(save_path);
				(void) close(SAM_fd);
				save_path = strdup(check_name);
				SAM_fd = open_samfs(save_path);
			} else if (save_path == (char *)NULL) {
				check_samfs_fs();
				save_path = strdup(check_name);
				SAM_fd = open_samfs(save_path);
			}

			/*
			 * Check if last path matches this path. Otherwise,
			 * If subpath does not yet exist, make directories
			 * as needed.
			 */
			if (last_path != NULL) {
				char *name_cmp;
				int path_len;

				path_len = strlen(last_path);
				/* Copy filename */
				name_cmp = strdup(name_compare);
				if ((slash_loc = strrchr(name_cmp, '/')) !=
				    NULL) {
					*slash_loc = '\0';
					if ((path_len == strlen(name_cmp)) &&
					    (last_path[path_len-1] ==
					    name_cmp[path_len-1]) &&
					    (bcmp(last_path, name_cmp,
					    path_len) == 0)) {

						free(name_cmp);
						goto restore_file;
					}
				}
				free(name_cmp);
			}
			if (last_path != NULL) {
				free(last_path);
				last_path = NULL;
			}
			/* Save path for check above */
			last_path = strdup(name_compare);
			if ((slash_loc = strrchr(last_path, '/')) != NULL) {
				*slash_loc = '\0';
			} else {
				free(last_path);
				last_path = NULL;
			}
			/* Copy filename */
			next_name = check_name = strdup(name_compare);
			while ((slash_loc = strchr(next_name, '/')) !=
			    (char *)NULL) {
				*slash_loc = '\0';

				if (mkdir(check_name,
				    S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
					if (EEXIST == errno) {
						*slash_loc = '/';
						next_name = slash_loc + 1;
						continue;
					}

					BUMP_STAT(errors);
					error(1, errno,
					    catgets(catfd, SET, 222,
					    "%s: Cannot mkdir()"),
					    check_name);
					break;
				}

				*slash_loc = '/';
				next_name = slash_loc + 1;
			}

			free(check_name);
		} else {
			/*
			 * Search through absolute path from end for
			 * SAM-FS file system
			 */
			/* Copy filename */
			char *check_name = strdup(name_compare);

			slash_loc = &unused;

			do {
				*slash_loc = '\0';
				/*
				 * Make sure we don't restore into a
				 * non-SAM-FS filesystem
				 * by calling sam_stat() and checking the
				 * directory attributes.
				 */
				if (sam_stat(check_name, &sb,
				    sizeof (sb)) == 0 &&
				    SS_ISSAMFS(sb.attr)) {
					if ((save_path != (char *)NULL) &&
					    (strcmp(check_name,
					    save_path) != 0)) {
						free(save_path);
						(void) close(SAM_fd);
						save_path = strdup(check_name);
						SAM_fd = open_samfs(save_path);
					} else if (save_path == (char *)NULL) {
						save_path = strdup(check_name);
						SAM_fd = open_samfs(save_path);
					}

					break;
				}
				slash_loc = strrchr(check_name, '/');
			} while (slash_loc != (char *)NULL);


			/*
			 * If no SAM-FS file system found in path, issue error
			 */
			if ((char *)NULL == slash_loc) {
				BUMP_STAT(errors);
				BUMP_STAT(errors_dir);
				error(1, 0,
				    catgets(catfd, SET, 259,
				    "%s: Not a SAM-FS file."),
				    name_compare);
			}

			free(check_name);
		}

restore_file:
		if (verbose) {
			sam_ls(name_compare, &perm_inode, NULL);
		}
		/* If not already offline */
		if (!perm_inode.di.status.b.offline) {
			/* If archived */
			if (perm_inode.di.arch_status) {
				print_reslog(log_st, name_compare,
				    &perm_inode, "online");
			}
		} else if (perm_inode.di.status.b.pextents) {
			print_reslog(log_st, name_compare, &perm_inode,
			    "partial");
		}

		/*
		 * if segment index, we have to skip the data segment inodes
		 * before we can get to the dumped data.
		 */
		if (!(S_ISSEGI(&perm_inode.di) && file_data)) {
			sam_restore_a_file(name_compare, &perm_inode,
			    vsnp, slink, data,
			    n_acls, aclp, file_data, &file_data_read);
		}
		skipping = 0;

skip_seg_file:
		if (S_ISSEGI(&perm_inode.di)) {
			struct sam_perm_inode seg_inode;
			int i;
			offset_t seg_size;
			int no_seg;
			sam_id_t seg_parent_id = perm_inode.di.id;

			/*
			 * If we are restoring the data, don't restore the
			 * data segment inodes.  This means we will lose the
			 * archive copies, if any.
			 */
			if (file_data) skipping++;

			/*
			 * Read each segment inode. If archive copies
			 * overflowed,
			 * read vsn sections directly after each segment inode.
			 */

			seg_size =
			    (offset_t)perm_inode.di.rm.info.dk.seg_size *
			    SAM_MIN_SEGMENT_SIZE;
			no_seg = (perm_inode.di.rm.size + seg_size - 1) /
			    seg_size;
			for (i = 0; i < no_seg; i++) {
				struct sam_vsn_section *seg_vsnp;

				readcheck(&seg_inode, sizeof (seg_inode),
				    5002);
				if (swapped) {
					if (sam_byte_swap(
					    sam_perm_inode_swap_descriptor,
					    &seg_inode, sizeof (seg_inode))) {
						error(0, 0,
						    catgets(catfd, SET, 13531,
						"%s: segment inode byte "
						"swap error - skipping."),
						    name);
						dont_process_this_entry = 1;
					}
				}
				if (!(SAM_CHECK_INODE_VERSION(
				    seg_inode.di.version))) {
					if (debugging) {
						fprintf(stderr,
						    "cs_restore: seg %d "
						    "inode version %d\n",
						    i, seg_inode.di.version);
					}
					error(0, 0, catgets(catfd, SET, 739,
					    "%s: inode version incorrect - "
					    "skipping"),
					    name);
					dont_process_this_entry = 1;
				}
				if (skipping || dont_process_this_entry) {
					continue;
				}
				seg_inode.di.parent_id = seg_parent_id;
				seg_vsnp = NULL;
				csd_read_mve(&seg_inode, &seg_vsnp);
				if (verbose) {
					sam_ls(name_compare, &seg_inode, NULL);
				}
				sam_restore_a_file(name_compare, &seg_inode,
				    seg_vsnp,
				    NULL, NULL, 0, NULL, file_data, NULL);
				if (seg_vsnp) {
					SamFree(seg_vsnp);
				}
			}
			/*
			 * Now that we have skipped the data segment inodes
			 * we can restore the dumped data if any.
			 */
			if (file_data) {
				sam_restore_a_file(name_compare, &perm_inode,
				    vsnp, slink, data,
				    n_acls, aclp, file_data, &file_data_read);
			}
		}

skip_file:
		if (data) {
			SamFree(data);
			data = NULL;
		}
		if (vsnp) {
			SamFree(vsnp);
			vsnp = NULL;
		}
		if (aclp) {
			SamFree(aclp);
			aclp = NULL;
		}
		if (file_data && file_data_read == 0) {
			skip_embedded_file_data();
		}
	}

	if (last_path != NULL) {
		free(last_path);
	}
	if (save_path != (char *)NULL) {
		free(save_path);
	}
	if (open_dir_fd > 0) {
		(void) close(open_dir_fd);
	}
	pop_permissions_stack();
	pop_times_stack();
}


/*
 * ----- check_samfs_fs - verify path is in samfs file system.
 */
static void
check_samfs_fs()
{
	char *check_name = ".";
	struct sam_stat sb;

	if (sam_stat(check_name, &sb,
	    sizeof (sb)) < 0 || !SS_ISSAMFS(sb.attr)) {
		BUMP_STAT(errors);
		BUMP_STAT(errors_dir);
		error(1, 0, catgets(catfd, SET, 259,
		    "%s: Not a SAM-FS file."), check_name);
	}
}


/*
 * ----- print_reslog - print a log for the benefit of restore.sh.
 */
void
print_reslog(
	FILE *log_st,
	char *pathname,
	struct sam_perm_inode *pid,
	char *status)
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

static void
sam_restore_a_file(
	char	*path,		/* the path name of the file */
	struct sam_perm_inode *perm_inode,
	struct sam_vsn_section *vsnp,
	char	*slink,		/* symlink resolution, if any */
	void	*data,		/* resource file info, if any */
	int	n_acls,		/* number of acl entries */
	aclent_t *aclp,		/* acl entries, if any */
	int	dflag,		/* file has associated data */
	int	*dread)		/* associated data has been read */
{
	struct sam_ioctl_idrestore idrestore = { NULL, NULL, NULL, NULL };
	char *basename = strrchr(path, '/');
	int entity_fd = -1;
	int copy;
	int force_writable = 0;
	u_longlong_t file_write_size;
	mode_t mode;

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

	if (replace) {
		if (S_ISDIR(mode)) {
			(void) rmdir(path);
		} else if (!S_ISSEGS(&perm_inode->di)) {
			unlink(path);
		}
	}

	/*
	 * Hook for inode conversion for a WORM'd inode.
	 * Perform the conversion only once so as not to
	 * destroy existing retention information in a
	 * version 2 WORM inode. Set a flag to guarantee
	 * we convert only once.
	 */
	if ((perm_inode->di.version == SAM_INODE_VERS_2) &&
	    perm_inode->di.status.b.worm_rdonly) {
		if (((perm_inode->di2.p2flags & P2FLAGS_WORM_V2) == 0) &&
		    (S_ISREG(mode) || S_ISDIR(mode))) {
			conv_to_v2inode(perm_inode);
		}
	}

	/* Notify user of previously or newly damaged files */
	if (perm_inode->di.status.b.damaged) {
		BUMP_STAT(file_damaged);
		error(0, 0,
		    catgets(catfd, SET, 13513,
		    "%s: File data was not recoverable prior to dump "
		    "(file is marked damaged)."),
		    path);
	} else if ((S_ISREG(mode) && !S_ISSEGI(&perm_inode->di)) &&
	    (perm_inode->di.arch_status == 0) &&
	    (perm_inode->di.rm.size != 0) &&
	    (dflag == 0)) {
		int copy;
		boolean_t inconsistent = FALSE;
		boolean_t stale = FALSE;
		uint_t *arp = (uint_t *)(void *)&perm_inode->di.ar_flags[0];

		/* These stale entries should not be rearchived */
		*arp &= ~((AR_rearch<<24)|(AR_rearch<<16)|(AR_rearch<<8)|
		    AR_rearch);

		/*
		 * If inode has any inconsistent copies, mark copy as
		 * archived.
		 */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			/* XXX missing code XXX */
		}
		/*
		 * If inode has any stale copies, file is still marked
		 * damaged.
		 */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if ((perm_inode->di.ar_flags[copy] &
			    AR_inconsistent) &&
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
			error(0, 0, catgets(catfd, SET, 5032,
			    "%s: File has inconsistent copy."), path);
		} else if (stale) {
			error(0, 0, catgets(catfd, SET, 5017,
			    "%s: File is now damaged, stale archive "
			    "copy exists."), path);
		} else {
			error(0, 0,
			    catgets(catfd, SET, 13512,
		"%s: File data cannot be recovered (file is marked damaged)."),
			    path);
		}
		if (!inconsistent) {
			BUMP_STAT(file_damaged);
			perm_inode->di.status.b.damaged = 1;
		}
	}

	if (replace_newer) {
		struct sam_stat st1;

		if (sam_lstat(path, &st1, sizeof (struct sam_stat)) == 0 &&
		    (st1.st_mtime < perm_inode->di.modify_time.tv_sec)) {
			if (S_ISDIR(mode)) {
				(void) rmdir(path);
			} else if (!S_ISSEGS(&perm_inode->di)) {
				unlink(path);
			}
		}
	}

	/*
	 * Create the subject entity.  It might be one of a number of different
	 * things, each of which must be created slightly differently.
	 */
	if (S_ISSEGS(&perm_inode->di)) {
		int copy_bits;
		int open_flg = (O_RDONLY | SAM_O_LARGEFILE);

		if ((entity_fd = open(path, open_flg, mode & 07777)) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 213, "%s: cannot open()"),
			    path);
			return;
		}
		BUMP_STAT(segments);
		copy_bits = perm_inode->di.arch_status & 0xF;
		csd_statistics.file_archives += copy_array[copy_bits];

	} else if (S_ISREG(mode) || S_ISSEGI(&perm_inode->di)) {
		int copy_bits, restored = 0;
		int	open_flg = (O_CREAT | O_EXCL| SAM_O_LARGEFILE);
		int nlink;
		sam_id_t id;
		boolean_t found = FALSE;

		id = perm_inode->di.id;
		nlink = perm_inode->di.nlink;
		if (!dflag &&			/* Attempt F_SAMRESTORE */
		    (perm_inode->di.status.b.acl == 0) &&
		    !S_ISSEGI(&perm_inode->di)) {
			/*
			 * If this is the first occurrence of a hard link,
			 * sam_check_nlink() sets di.nlink to 1 in the old
			 * inode image.  Then sam_res_by_name will create
			 * the disk inode.
			 * If it's a hard link we've seen before,
			 * sam_check_nlink() leaves di.nlink greater than 1,
			 * but updates the inode number
			 * from the hardp table.  Then we create the link.
			 */
			if (nlink != 1) {
				found = sam_check_nlink(id, path, perm_inode);
			}
			sam_res_by_name(basename, path, perm_inode, vsnp);

			if (nlink != 1) {
				BUMP_STAT(hlink);
				/*
				 * If first occurrence of a hard link,
				 * save the new ino in the table.
				 */
				if (hardp->id[id.ino].ino == 0) {
					BUMP_STAT(hlink_first);
					hardp->id[id.ino] = perm_inode->di.id;
				}
			}
			restored = 1;
		} else {
			if (dflag) {
				open_flg |= O_WRONLY;
			}
			/*
			 * If this is the first occurrence of a hard link,
			 * we create the file and put the new inode number
			 * in the hard link lookup table.
			 * If it's a hard link we've seen before,
			 * sam_check_nlink() leaves di.nlink greater than 1,
			 * but updates the inode number from the hardp table.
			 * Then sam_res_by_name() creates the link.
			 */
			if (nlink != 1) {
				found = sam_check_nlink(id, path, perm_inode);
				BUMP_STAT(hlink);
			}
			if (!found) {
				struct sam_stat sb;

				if ((entity_fd = open(path, open_flg,
				    mode & 07777)) < 0) {
					BUMP_STAT(errors);
					if(!quiet || (errno != ENOTDIR)) {
						error(0, errno,
						    "%s: Cannot creat(l)",
						    path);
					}
					return;
				}
				if (nlink != 1 && prelinks &&
				    hardp->id[id.ino].ino == 0) {
					/*
					 * If first occurrence of a hard link,
					 * save the new ino in the table.
					 */
					BUMP_STAT(hlink_first);
					sam_stat(path, &sb, sizeof (sb));
					hardp->id[id.ino].ino = sb.st_ino;
					hardp->id[id.ino].gen = sb.gen;
				}
			} else {
				sam_res_by_name(basename, path,
				    perm_inode, vsnp);
				restored = 1;
			}
		}
		BUMP_STAT(files);
		copy_bits = perm_inode->di.arch_status & 0xF;
		csd_statistics.file_archives += copy_array[copy_bits];
		if (restored) {
			return;
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
			if(!quiet || errno != EEXIST) {
				error(0, errno,
				    catgets(catfd, SET, 222,
				    "%s: Cannot mkdir()"), path);
			}
			return;
		}
		/*
		 * No need for SAM_O_LARGEFILE here, directories
		 * always < 2GB
		 */
		if ((entity_fd = open(path, open_flg)) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 5010,
			    "%s: Cannot open() following mkdir()"), path);
			return;
		}
		BUMP_STAT(dirs);
	} else if (S_ISREQ(mode)) {
		struct sam_resource_file *rfp =
		    (struct sam_resource_file *)data;
		int open_flg = (O_WRONLY | O_CREAT |
		    O_EXCL | O_TRUNC | SAM_O_LARGEFILE);

		if (rfp->resource.revision != SAM_RESOURCE_REVISION) {
			BUMP_STAT(errors);
			error(0, 0,
			    catgets(catfd, SET, 5014,
			    "%s: Unsupported revision for removable "
			    "media info"), path);
			return;
		}
		if ((entity_fd = open(path, open_flg, mode & 07777)) < 0) {
			BUMP_STAT(errors);
			if(!quiet || errno != EEXIST) {
				error(0, errno,
				    "%s: Cannot creat(q)",
				    path);
			}
			return;
		}
		idrestore.rp.ptr = (void *)data;
		BUMP_STAT(resources);
	} else if (S_ISLNK(mode)) {
		if ((perm_inode->di.nlink == 1) &&
		    (perm_inode->di.status.b.acl == 0)) {
			struct sam_ioctl_samrestore samrestore = {NULL,
				NULL, NULL, NULL};
			int dir_fd;

			/*
			 * If a symlink, just restore a file by name.
			 * Don't create
			 * a file, nor an open file descriptor.
			 */
			if (namep == NULL) {
					allocate_dirent_space();
			}
			sam_set_dirent(basename);
			dir_fd = sam_open_directory(path);
			samrestore.name.ptr = namep;
			samrestore.lp.ptr = (void *)slink;
			samrestore.dp.ptr = perm_inode;
			samrestore.vp.ptr = NULL;
			if (ioctl(dir_fd, F_SAMRESTORE, &samrestore) < 0) {
				if(!quiet || errno != EEXIST) {
					error(0, errno,
					    "%s: ioctl(F_SAMRESTORE)", path);
				}
				BUMP_STAT(errors);
			}
			BUMP_STAT(links);
			return;
		} else {
			int open_flg = (O_WRONLY | O_CREAT | O_EXCL | O_TRUNC);

			if ((entity_fd = open(path, open_flg,
			    mode & 07777)) < 0) {
				BUMP_STAT(errors);
				if(!quiet || errno != EEXIST) {
					error(0, errno,
					    "%s: Cannot creat()", path);
				}
				return;
			}
			idrestore.lp.ptr = (void *)slink;
			BUMP_STAT(links);
		}
	} else if (S_ISFIFO(mode) || S_ISCHR(mode) || S_ISBLK(mode)) {
		struct sam_ioctl_samrestore samrestore = {NULL,
			NULL, NULL, NULL};
		int dir_fd;

		/*
		 * If a special, just restore a file by name. Don't create
		 * a file, nor an open file descriptor.
		 */
		if (namep == NULL) {
			allocate_dirent_space();
		}
		sam_set_dirent(basename);
		dir_fd = sam_open_directory(path);
		samrestore.name.ptr = namep;
		samrestore.lp.ptr = NULL;
		samrestore.dp.ptr = perm_inode;
		samrestore.vp.ptr = NULL;
		if (ioctl(dir_fd, F_SAMRESTORE, &samrestore) < 0) {
			if(!quiet || errno != EEXIST) {
				error(0, errno,
				    "%s: ioctl(F_SAMRESTORE)", path);
			}
			BUMP_STAT(errors);
		}
		BUMP_STAT(specials);
		return;
	}
	if (dflag) {
		if (S_ISSEGI(&perm_inode->di)) {
			char ops[64];
			uint64_t seg_size = perm_inode->di.rm.info.dk.seg_size;
			uint_t stage_ahead = perm_inode->di.stage_ahead;

			/*
			 * File was segmented.  Make new file segmented so
			 * we can copy the data back.
			 */

			snprintf(ops, sizeof (ops), "l%llds%d",
			    seg_size * SAM_MIN_SEGMENT_SIZE, stage_ahead);
			if (sam_segment(path, ops) < 0) {
				error(0, errno, "sam_segment(%s, %s)", path,
				    ops);
				BUMP_STAT(errors);
				return;
			}
		}
		if (S_ISREG(mode) && !(S_ISSEGS(&perm_inode->di))) {

			file_write_size = write_embedded_file_data(entity_fd,
			    path);
			csd_statistics.data_dumped += file_write_size;
			*dread = 1;

	/* N.B. Bad indentation here to meet cstyle requirements */
	if (file_write_size != perm_inode->di.rm.size) {
		if (perm_inode->di.status.b.pextents == 0) {
			BUMP_STAT(errors);
			if (debugging) {
				fprintf(stderr,
				    "sam_restore_a_file size"
				    " mismatch %lld, %lld\n",
				    (u_longlong_t)file_write_size,
				    (u_longlong_t)perm_inode->di.rm.size);
			}
			error(0, errno,
			    catgets(catfd, SET, 13505,
			    "Error writing to data file %s"),
			    path);
			return;
		} else {
			if (debugging) {
				fprintf(stderr,
				"sam_restore_a_file partial "
				"size %lld, size %lld\n",
				    (u_longlong_t)file_write_size,
				    (u_longlong_t)perm_inode->di.rm.size);
			}
		}
	} /* if file_write_size ... */




		} /* if S_ISREG... */
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
		error(0, errno, "%s: ioctl(F_IDRESTORE)", path);
		BUMP_STAT(errors);
	}

	/* Restore any access control list associated with file. */
	if (n_acls > 0) {
		if (facl(entity_fd, SETACL, n_acls, aclp) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 5026,
			    "%s: cannot write acl information"),
			    path);
		}
	}
	if (close(entity_fd) < 0) {
		error(0, errno, "%s: close() ", path);
		BUMP_STAT(errors);
	}

	/*
	 *	Remember the correct inum and time values
	 *	for directories and segment indexes. The IDRESTORE
	 *	has already been called, which returns the correct
	 *	id for directories and segment indexes.
	 */
	if (S_ISDIR(perm_inode->di.mode) || S_ISSEGI(&perm_inode->di)) {
		stack_times(path, perm_inode);
	}

	/*
	 *	If a write was forced, remember the path and clear it
	 *	when all done.
	 */

	if (force_writable) {
		stack_permissions(path, perm_inode);
	}
}

static boolean_t
sam_check_nlink(
	sam_id_t id,
	char *path,
	struct sam_perm_inode *perm_inode)
{
	boolean_t found = FALSE;

	if (hardp == NULL) {
		prelinks = N_HARDLINKS;
		if (id.ino > prelinks) {
			prelinks = ((id.ino + N_HARDLINKS) >> 9) << 9;
		}
		SamMalloc(hardp, prelinks * sizeof (csd_hardtbl_t));
		bzero(hardp, (prelinks * sizeof (csd_hardtbl_t)));
	}
	if (id.ino >= prelinks) {
		int oldlinks = prelinks;
		int i;

		prelinks += N_HARDLINKS;
		if (id.ino > prelinks) {
			prelinks = ((id.ino + N_HARDLINKS) >> 9) << 9;
		}
		hardp = (csd_hardtbl_t *)realloc(hardp,
		    prelinks * sizeof (csd_hardtbl_t));
		if (hardp == NULL) {
			error(0, errno, catgets(catfd, SET, 13539, "%s: Cannot "
			    "malloc space for hard link table."), path);
			return (found);
		}
		bzero(hardp + oldlinks, ((prelinks - oldlinks) *
		    sizeof (csd_hardtbl_t)));
	}
	if (hardp->id[id.ino].ino != 0) {
		perm_inode->di.id = hardp->id[perm_inode->di.id.ino];
		found = TRUE;
	} else {
		perm_inode->di.nlink = 1;
	}
	return (found);
}

static void
sam_res_by_name(
	char *basename,
	char *path,
	struct sam_perm_inode *perm_inode,	/* image of the old inode */
	struct sam_vsn_section *vsnp)
{
	struct sam_ioctl_samrestore samrestore = {NULL, NULL, NULL, NULL};
	int copy;
	int dir_fd;

	/*
	 * If a nodata, non-access-control-listed regular file,
	 * just restore a file by name. Don't create
	 * a file, nor an open file descriptor.
	 */
	if (namep == NULL) {
		allocate_dirent_space();
	}
	sam_set_dirent(basename);
	dir_fd = sam_open_directory(path);
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
				samrestore.vp.ptr = (void *)vsnp;
				break;
			}
		}
	}
	if (ioctl(dir_fd, F_SAMRESTORE, &samrestore) < 0) {
		if(!quiet || errno != EEXIST) {
			error(0, errno,
			    "%s: ioctl(F_SAMRESTORE)", path);
		}
		if (errno == ENOTTY) {
			error(1, 0, catgets(catfd, SET, 259,
			    "%s: Not a SAM-FS file."), basename);
		}
	}
}

/*
 * ----- stack_times - remember inode times.
 *	Save the times associated with a directory or a segment file index.
 *	After all files are restored, we'll set the times into the directory or
 *	segment file index, because as we create entries, we are changing the
 *	times.
 */

static int time_stack_is_full = 0;	/* cannot malloc more; skip dirs */
static struct sam_ioctl_idtime *time_stack = NULL; /* ptr to base of stack */
static int time_stack_size = 0;		/* size of stack (capacity)	*/
static int time_stack_next_free = 0;	/* next free slot in stack	*/

#define	TIME_STACK_SIZE_INCREMENT 1024	/* expand time_stack by this	*/
					/* many entries when it fills	*/


void
stack_times(
	char *path,
	struct sam_perm_inode *perm_inode)
{
	struct sam_ioctl_idtime *idtime;

	if (time_stack_is_full) {
		return;
	}

	if (time_stack_next_free >= time_stack_size) {
		int new_time_size = sizeof (struct sam_ioctl_idtime) *
		    (time_stack_size + TIME_STACK_SIZE_INCREMENT);
		time_stack = (struct sam_ioctl_idtime *)
		    realloc(time_stack, new_time_size);
		/* may want to spill to disk, "next rev" */
		if (time_stack == NULL) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 13518,
			    "%s: Cannot malloc space for directory/segment "
			    "index stack."),
			    path);
			error(0, 0,
			    catgets(catfd, SET, 13519,
			    "Not all directory or segment index times "
			    "will be set correctly."));
			time_stack_is_full = 1;
			return;
		}
		time_stack_size += TIME_STACK_SIZE_INCREMENT;
	}

	idtime = &time_stack[time_stack_next_free];
	idtime->id = perm_inode->di.id;
	idtime->atime = perm_inode->di.access_time.tv_sec;
	idtime->mtime = perm_inode->di.modify_time.tv_sec;
	idtime->xtime = perm_inode->di.creation_time;
	idtime->ytime = perm_inode->di.attribute_time;

	time_stack_next_free++;
}


/*
 * ----- pop_times_stack - unroll the times stack, doing the IDTIME
 *	as we go.
 */

void
pop_times_stack()
{
	while (time_stack_next_free > 0) {
		time_stack_next_free--;
		if (ioctl(SAM_fd, F_IDTIME,
		    &time_stack[time_stack_next_free]) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 1376,
			    "inode %d.%d: Cannot ioctl(F_IDTIME)"),
			    time_stack[time_stack_next_free].id.ino,
			    time_stack[time_stack_next_free].id.gen);
		}
	}
	if (time_stack != NULL)  {
		free(time_stack);
		time_stack = NULL;
	}
}

/*
 * ----- stack_permissions - remember directory permissions.
 *	Save the permissions and pathname to any directory whose original
 *	permissions do not include a "owner writable" permission.
 *	After all files are restored, we'll change the permissions to
 *	those set at the time of the dump.
 */

static int perm_stack_is_full = 0;	/* cannot malloc more; skip dirs */
static struct perm_stack {
	mode_t	perm;			/* permissions */
	char *path;			/* pointer to path */
} *perm_stack = NULL;			/* pointer to start of stack */
static int perm_stack_size = 0;		/* size of stack (capacity) */
static int perm_stack_next_free = 0;	/* next free slot in stack */

#define	PERM_STACK_SIZE_INCREMENT 1024	/* expand perm_stack by this	*/
					/* many entries when it fills	*/
void
stack_permissions(
	char	*path,
	struct sam_perm_inode *perm_inode)
{
	struct perm_stack *new_ent;

	if (perm_stack_is_full) {
		return;
	}

	if (perm_stack_next_free >= perm_stack_size) {
		int new_perm_size = sizeof (struct perm_stack) *
		    (perm_stack_size + PERM_STACK_SIZE_INCREMENT);
		perm_stack = (struct perm_stack *)
		    realloc(perm_stack, new_perm_size);
		/* may want to spill to disk, "next rev" */
		if (perm_stack == NULL) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 13526,
			    "%s: Cannot malloc space for directory "
			    "permission stack."),
			    path);
			error(0, 0,
			    catgets(catfd, SET, 13528,
			    "Not all directory permissions will be "
			    "set correctly."));
			perm_stack_is_full = 1;
			return;
		}
		perm_stack_size += PERM_STACK_SIZE_INCREMENT;
	}

	new_ent = &perm_stack[perm_stack_next_free];
	new_ent->perm = perm_inode->di.mode & ~S_IWUSR;
	new_ent->perm &= 07777;		/* retain only permission bits */
	new_ent->path = strdup(path);
	if (new_ent->path == NULL) {
		BUMP_STAT(errors);
		error(0, errno,
		    catgets(catfd, SET, 13527,
		    "%s: Cannot malloc space for directory permission path."),
		    path);
		error(0, 0,
		    catgets(catfd, SET, 13528,
		    "Not all directory permissions will be set correctly."));
		perm_stack_is_full = 1;
		return;
	}

	perm_stack_next_free++;
}


/*
 * ----- pop_permissions_stack - unroll the permissions stack, doing a chmod
 *	as we go.
 */

void
pop_permissions_stack()
{
	struct perm_stack *ent;

	while (perm_stack_next_free > 0) {
		perm_stack_next_free--;
		ent = &perm_stack[perm_stack_next_free];
		if (chmod(ent->path, ent->perm) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 13524,
			    "Cannot chmod directory %s"),
			    ent->path);
		}
		free(ent->path);
		ent->path = NULL;
	}
	if (perm_stack != NULL)  {
		free(perm_stack);
		perm_stack = NULL;
	}
}

void
allocate_dirent_space()
{
	if (namep == NULL) {
		SamMalloc(namep, sizeof (struct sam_dirent) + MAXNAMLEN);
	}
}

void
sam_set_dirent(char *np)
{
	char *pdn = (char*)namep->d_name;

	memset(namep, 0, sizeof (struct sam_dirent) + MAXNAMLEN);
	namep->d_reclen = namep->d_namlen = strlen(np);
	strncpy(pdn, np, MAXNAMLEN);
}

int
sam_open_directory(char *np)
{
	static char last_dir[MAXPATHLEN * 2];
	char dirname[MAXPATHLEN * 2];
	char *last_slash;

	strncpy(dirname, np, MAXPATHLEN * 2);
	last_slash = strrchr(dirname, '/');
	if (last_slash == NULL || last_slash == dirname) {
		strcpy(dirname, ".");
	} else {
		*last_slash = '\0';
	}
	if (open_dir_fd == 0 || strcmp(last_dir, dirname) != 0) {
		int new_fd;

		if ((new_fd = open(dirname, O_RDONLY)) <= 0) {
			return (new_fd);
		}
		if (open_dir_fd > 0) {
			(void) close(open_dir_fd);
		}
		strcpy(last_dir, dirname);
		open_dir_fd = new_fd;
	}
	return (open_dir_fd);
}
