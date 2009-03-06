/*
 *	sam_dump.c
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


#pragma ident "$Revision: 1.15 $"


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <sys/dirent.h>
#include <pub/stat.h>
#include <sam/lib.h>
#include <sam/sam_malloc.h>
#include <sam/uioctl.h>
#include <unistd.h>
#include <sam/fioctl.h>
#include <sam/fs/dirent.h>
#include <sam/fs/ino.h>
#include "csd_defs.h"
#include <sam/fs/sblk.h>
#include "sam/nl_samfs.h"
#include <sys/types.h>
#include <sys/acl.h>

extern int csd_hdr_inited;
extern uint32_t dump_fs_magic;
extern void init_csd_header(uint32_t fsmagic);

extern	void WriteHeader(char *name, struct sam_stat *st, char type);

extern	int	parsetabs(char *string, char **argv, int *argc, int argmax);
extern	int	readin(FILE *, char *, int);
extern	int	readin_ln;

static void dump_file_data(char *, int partial, int file_fd);
static void BuildHeader(char *, int, struct sam_stat *, int partial);
static int check_directory_excluded(char *);
static int cmp_id(const void *id1, const void *id2);

static char *work_dir = NULL;		/* working list of directory names */
static int work_dir_size = 0;		/* cur allocated size of work_dir */
static int work_dir_used = 0;		/* currently used size of work_dir */
#define	WORK_DIR_SIZE_INCREMENT 10240	/* expand work_dir by this many bytes */

static char name[MAXPATHLEN + MAXPATHLEN + 2];
static char linkname[MAXPATHLEN + MAXPATHLEN + 2];

int stale_flags = ((AR_stale<<24)|(AR_stale<<16)|(AR_stale<<8)|AR_stale);
static int inconsistent_flags = ((AR_inconsistent<<24)|(AR_inconsistent<<16)|
	(AR_inconsistent<<8)|AR_inconsistent);
int	dumping_file_data;

static dev_t	initial_dev = 0;
static void display_saved_dir_list(void);
static int search_saved_dir_list(char *);

/*
 * Structures and constants for directory sorting (by inode number).
 */
#define	NAMES_INCR (1024*1024)
#define	MIN_SORT 10

struct inode_list {
	sam_id_t	id;		/* inode/gen nos */
	char *name;			/* name (null terminated) */
};

struct names {
	struct names *next;		/* Pointer to next struct */
	int	chars_remaining;	/* characters remaining */
	char	*first;			/* character buffer */
	char	*unused;		/* next available char */
};

static char *_SrcFile = __FILE__;


/*
 * remember that path/name is a directory which needs to be processed
 */
static void
save_dir_name(
	char *path,
	char *name)
{
	int path_len = strlen(path);
	int name_len = strlen(name);

	if (debugging) {
		fprintf(stderr, "save_dir_name( %s / %s )\n", path, name);
	}

	if (strcmp(name, ".") == 0) {
		return;
	}

	if (strcmp(name, "..") == 0) {
		return;
	}

	if (name_len == 0) {
		error(1, 0, "%s/%s: Name_len == 0 in save_dir_name", path,
		    name);
	}

	/* allocate more memory if needed */
	while (work_dir_used + path_len + name_len + 2 > work_dir_size) {
		work_dir = (char *)realloc(work_dir,
		    work_dir_size + WORK_DIR_SIZE_INCREMENT);
		if (work_dir == NULL) {
			error(1, errno,
			    catgets(catfd, SET, 192,
			    "%s/%s: Out of memory for directory name cache,"
			    " current size of cache is %d bytes"),
			    path, name, work_dir_size);
		}
		work_dir_size += WORK_DIR_SIZE_INCREMENT;
	}

	if (*path != '\0') {
		/* add directory prefix */
		strcpy(&work_dir[work_dir_used], path);
		work_dir_used += path_len;
		work_dir[work_dir_used] = '/';		/* add slash */
		work_dir_used += 1;
	}
	strcpy(&work_dir[work_dir_used], name);	/* add filename */
	work_dir_used += name_len + 1;
}

static void
dump_directory_entry(
	sam_id_t id,
	char *component_name,
	char *name_append_point,
	char *path)
{
	struct sam_ioctl_idstat idstat;
	struct sam_perm_inode perm_inode;
	struct sam_perm_inode_v1 *perm_inode_v1;
	struct sam_vsn_section *vsnp = NULL;
	mode_t mode;
	int file_fd = -1;
	int n_vsns;
	int ngot;
	int copy;
	int	dumping_partial_data = 0;
	long flags = 0;
	void * data = NULL;
	int n_acls = -1;
	aclent_t *aclp = NULL;

	/* path is null for initial directory. */
	if (*path != '\0') {
		*name_append_point = '/';
		strcpy(name_append_point+1, (const char *)component_name);
	}

	/* Stat the file.  We know the "id" from the directory entry. */
	idstat.id	  = id;
	idstat.size   = sizeof (perm_inode);
	idstat.dp.ptr = (void *)&perm_inode;
	if (ioctl(SAM_fd, F_IDSTAT, &idstat) < 0) {
		if (errno != EXDEV) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 5016,
			    "%s: cannot F_IDSTAT, %d.%d"),
			    name, id.ino, id.gen);
			return;
		}
		errno = 0;
	}

	/* Verify that we stat()ed what we thought we were supposed to. */
	if ((perm_inode.di.id.ino != id.ino) ||
	    (perm_inode.di.id.gen != id.gen)) {
		BUMP_STAT(file_warnings);
		error(0, 0,
		    catgets(catfd, SET, 273,
		    "%s: stat() id mismatch: expected: %d.%d, got %d.%d, "
		    "entry not processed"),
		    name, id.ino, id.gen,
		    perm_inode.di.id.ino, perm_inode.di.id.gen);
		return;
	}

	/*
	 * If the dump header has not been initialized, initialize it.  If
	 * the dumper header has been initialized, ensure that we only allow
	 * 1 FS Magic type in the same dump.
	 */
	if (csd_hdr_inited) {
		if (idstat.magic != dump_fs_magic) {
			BUMP_STAT(file_warnings);
			error(1, 0,
			    catgets(catfd, SET, 274,
			    "%s: idstat() fs version mismatch: expected: %s "
			    "got %s, "
			    "dump terminated"), name,
			    dump_fs_magic == SAM_MAGIC_V1 ? "1" :
			    (dump_fs_magic == SAM_MAGIC_V2) ? "2" : "2A",
			    idstat.magic == SAM_MAGIC_V1 ? "1" :
			    (idstat.magic == SAM_MAGIC_V2) ? "2" : "2A");
		}
	} else {
		init_csd_header(idstat.magic);
	}

	/* Process the file using the returned stat information.	*/
	mode = perm_inode.di.mode;
	dumping_file_data = 0;
	if (S_ISSEGI(&perm_inode.di)) {
		int copy_bits;
		struct sam_ioctl_idseginfo sii;
		offset_t seg_size;

		sii.size = perm_inode.di.rm.info.dk.seg.fsize;
		if (sii.size <= 0) {	/* XXX - do we have an upper bound? */
			BUMP_STAT(errors);
			error(0, 0,
			    catgets(catfd, SET, 5034,
			    "%s: segment index is empty or invalid, size %d."),
			    name, sii.size);
			goto clean_up;
		}
		SamMalloc(data, sii.size);
		sii.id.ino = id.ino;
		sii.id.gen = id.gen;
		sii.offset = 0;
		sii.buf.ptr = data;
		if (ioctl(SAM_fd, F_IDSEGINFO, &sii) != 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 5022,
			    "%s: cannot read segment file."),
			    name);
			goto clean_up;
		}
		BUMP_STAT(files);
		seg_size = (offset_t)perm_inode.di.rm.info.dk.seg_size *
		    SAM_MIN_SEGMENT_SIZE;
		csd_statistics.segments += (perm_inode.di.rm.size +
		    seg_size - 1)
		    / seg_size;
		copy_bits = perm_inode.di.arch_status & 0xF;
		csd_statistics.file_archives += copy_array[copy_bits];
		if (qfs && (perm_inode.di.rm.size > 0)) {
			dumping_file_data++;
			flags |= CSD_FH_DATA;
		} else if (online_data &&
		    (perm_inode.di.rm.size > 0) &&
		    (perm_inode.di.status.b.offline == 0)) {
			dumping_file_data++;
			flags |= CSD_FH_DATA;
		} else if (partial_data &&
		    (perm_inode.di.rm.size > 0) &&
		    (perm_inode.di.status.b.pextents == 1)) {
			dumping_partial_data++;
			flags |= CSD_FH_DATA;
		} else if (unarchived_data &&
		    (perm_inode.di.rm.size > 0) &&
		    (copy_bits == 0)) {
			dumping_file_data++;
			flags |= CSD_FH_DATA;
		}
	} else if (S_ISREG(mode)) {
		int copy_bits;

		BUMP_STAT(files);
		copy_bits = perm_inode.di.arch_status & 0xF;
		csd_statistics.file_archives += copy_array[copy_bits];
		if (qfs && (perm_inode.di.rm.size > 0)) {
			dumping_file_data++;
			flags |= CSD_FH_DATA;
		} else if (online_data &&
		    (perm_inode.di.rm.size > 0) &&
		    (perm_inode.di.status.b.offline == 0)) {
			dumping_file_data++;
			flags |= CSD_FH_DATA;
		} else if (partial_data &&
		    (perm_inode.di.rm.size > 0) &&
		    (perm_inode.di.status.b.pextents == 1)) {
			dumping_partial_data++;
			flags |= CSD_FH_DATA;
		} else if (unarchived_data &&
		    (perm_inode.di.rm.size > 0) &&
		    (copy_bits == 0)) {
			dumping_file_data++;
			flags |= CSD_FH_DATA;
		}
	} else if (S_ISDIR(mode)) {
		/*
		 * path is null for initial directory.  We still have
		 * to dump it, but don't save the name.
		 */
		BUMP_STAT(dirs);
		save_dir_name(path, (char *)component_name);
	} else if (S_ISLNK(mode)) {
		BUMP_STAT(links);
		if ((ngot = readlink(name, linkname, sizeof (linkname)-1)) ==
		    -1) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 227,
			    "%s: Cannot read link info."),
			    name);
			goto clean_up;
		}
		linkname[ngot] = '\0';
	} else if (S_ISREQ(mode)) {
		struct sam_ioctl_idresource idresource;
		struct sam_resource_file *resource;

		BUMP_STAT(resources);
		/* Get resource information. */
		SamMalloc(resource, perm_inode.di.psize.rmfile);
		idresource.id = id;
		idresource.size = perm_inode.di.psize.rmfile;
		idresource.rp.ptr = (void *)resource;
		data = (void *)resource;
		if (ioctl(SAM_fd, F_IDRESOURCE, &idresource) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 228,
			    "%s: Cannot read resource file."),
			    name);
			goto clean_up;
		}
	} else if (S_ISFIFO(mode) || S_ISCHR(mode) || S_ISBLK(mode)) {
		BUMP_STAT(specials);
	} else if (S_ISSOCK(mode)) {
		/* Just ignore sockets, as ufs does. */
		goto clean_up;
	} else {
		BUMP_STAT(errors);
		error(0, 0, catgets(catfd, SET, 283,
		    "%s: Unrecognized mode (0x%lx)"),
		    name, perm_inode.di.mode);
		goto clean_up;
	}

	if (dumping_file_data || dumping_partial_data) {
		if ((file_fd = open(name, O_RDONLY|SAM_O_LARGEFILE)) < 0) {
			error(0, errno, catgets(catfd, SET, 13530,
			    "Unable to open file <%s> to dump data, "
			    "skipping"), name);
			goto clean_up;
		}
	}

	/* Count the number of multivolume archive vsns for the inode */
	n_vsns = 0;
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		/* Current version */
		if (perm_inode.di.version >= SAM_INODE_VERS_2) {
			if (perm_inode.ar.image[copy].n_vsns > 1) {
				n_vsns += perm_inode.ar.image[copy].n_vsns;
			}
		} else if (perm_inode.di.version == SAM_INODE_VERS_1) {
			/* Prev vers */
			perm_inode_v1 =
			    (struct sam_perm_inode_v1 *)&perm_inode;
			if (perm_inode_v1->aid[copy].ino != 0) {
				if (perm_inode.ar.image[copy].n_vsns > 1) {
					n_vsns +=
					    perm_inode.ar.image[copy].n_vsns;
				} else {
					perm_inode_v1->aid[copy].ino = 0;
					perm_inode_v1->aid[copy].gen = 0;
				}
			}
		}
	}

	/* If multivolume archive vsns for any archive copy, get them */
	if (n_vsns) {
		struct sam_ioctl_idmva idmva;

		SamMalloc(vsnp, n_vsns * sizeof (struct sam_vsn_section));
		idmva.id = id;
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (perm_inode.di.version >= SAM_INODE_VERS_2) {
				/* Current version */
				idmva.aid[copy].ino = idmva.aid[copy].gen = 0;
			} else if (perm_inode.di.version == SAM_INODE_VERS_1) {
				/* Previous vers */
				perm_inode_v1 =
				    (struct sam_perm_inode_v1 *)&perm_inode;
				idmva.aid[copy] = perm_inode_v1->aid[copy];
			}
		}
		idmva.size = sizeof (struct sam_vsn_section) * n_vsns;
		idmva.buf.ptr = (void *)vsnp;
		if (ioctl(SAM_fd, F_IDMVA, &idmva) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 5024,
			    "%s: cannot F_IDMVA, %d.%d"),
			    name, id.ino, id.gen);
			goto clean_up;
		}
	}

	sam_db_list(name, &perm_inode, linkname, vsnp);

	/* If scan only */
	if (scan_only) {
		goto clean_up;
	}

	/* Retrieve any access control list info for the inode */
	if (perm_inode.di.status.b.acl && !S_ISLNK(perm_inode.di.mode)) {
		if ((n_acls = acl(name, GETACLCNT, 0, (aclent_t *)NULL)) < 0) {
			BUMP_STAT(errors);
			error(0, errno,
			    catgets(catfd, SET, 5025,
			    "%s: cannot read acl information"),
			    name);
			goto clean_up;
		}
		if (n_acls > 0) {
			SamMalloc(aclp, n_acls * sizeof (aclent_t));
			if ((n_acls = acl(name, GETACL, n_acls, aclp)) < 0) {
				BUMP_STAT(errors);
				error(0, errno,
				    catgets(catfd, SET, 5025,
				    "%s: cannot read acl information"),
				    name);
				goto clean_up;
			}
		}
	}

	if (csd_write(name, &perm_inode, n_vsns, vsnp, linkname, data,
	    n_acls, aclp, flags)) {
		goto clean_up;
	}
	if (dumping_file_data) {
		dump_file_data(name, 0, file_fd);
	} else if (dumping_partial_data) {
		dump_file_data(name, 1, file_fd);
	}

clean_up:
	if (file_fd >= 0) {
		(void) close(file_fd);
	}
	if (data) {
		SamFree(data);
	}
	if (vsnp) {
		SamFree(vsnp);
	}
	if (aclp) {
		SamFree(aclp);
	}
	if (perm_inode.di.status.b.damaged) {
		BUMP_STAT(file_damaged);
		if (!quiet) {
			error(0, 0,
			    catgets(catfd, SET, 13514,
			    "%s: File data was previously not "
			    "recoverable (file is marked damaged)."),
			    name);
		}
	} else if ((S_ISREG(perm_inode.di.mode) &&
	    !S_ISSEGI(&perm_inode.di)) &&
	    (perm_inode.di.arch_status == 0) &&
	    (perm_inode.di.rm.size != 0) &&
	    !dumping_file_data && !dumping_partial_data) {
		BUMP_STAT(file_warnings);
		if (!quiet) {
			if (perm_inode.di.ar_flags[0] & inconsistent_flags) {
				error(0, 0,
				    catgets(catfd, SET, 5033,
				    "%s: Warning! File is inconsistent."),
				    name);
			} else if (perm_inode.di.ar_flags[0] & stale_flags) {
				error(0, 0,
				    catgets(catfd, SET, 5018,
				    "%s: Warning! File will be "
				    "damaged, stale archive copy exists."),
				    name);
			} else {
				error(0, 0,
				    catgets(catfd, SET, 13515,
				    "%s: File data will not be "
				    "recoverable (file will be marked "
				    "damaged)."),
				    name);
			}
		}
	}
}


/*
 * ----- csd_dump_path - process a directory.
 *	Open the directory.  Read the entries in it. For entries which
 *	are directories themselves, call save_dir_name to remember that
 *	we need to come back and process that name in the future.
 *	For all entries in the directory, call csd_write to write the
 *	state of the entry to the csd dump file.
 */
void
csd_dump_path(
	char *tag,	/* "initial" or "recursive" */
	char *path,	/* the directory's pathname */
	mode_t mode)	/* the mode of the file in path, if "initial" */
{
	int					dir_fd;
	int					initial = 0;
	char				*name_append_point;
	char				*dirname;
	char				*filename = NULL;
	char				*last_slash;
	char				*start_ptr;
	struct sam_stat		statb;
	struct sam_dirent	*dirent;
	int			next_inode, num_inodes, max_inodes = 10000;
	boolean			directory_complete = FALSE;
	struct inode_list	*inode_list = NULL;
	struct names		*cur_namesp, *names_space = NULL;

	if (debugging) {
		fprintf(stderr, "csd_dump_path(%s, %s, %lx)\n", tag, path,
		    mode);
	}

	dirname = path;
	if (strcmp(tag, "initial") == 0) {
		initial = 1;
	}
	if (initial && !S_ISDIR(mode)) {
		/*
		 * To dump a specified filename, get the directory
		 * it belongs to, strip trailing slash
		 */
		if (path[strlen(path) - 1] == '/') {
			path[strlen(path) - 1] = '\0';
		}
		last_slash = strrchr(path, '/');
		if (last_slash != NULL) {
			if (last_slash != path) {
				*last_slash = '\0';
				filename = last_slash + 1;
			}
		} else {
			if ((strcmp(path, ".") != 0) &&
			    (strcmp(path, "..") != 0)) {
				filename = path;
			}

			if (strcmp(path, "..") != 0) {
				path = dirname = ".";
			} else {
				path = dirname = "..";
			}
		}
	}

	/*
	 *	Check if this directory is excluded.
	 */
	if (nexcluded) {
		if (check_directory_excluded(dirname)) {
			return;
		}
	}

	/*
	 * open the directory
	 */
	dir_fd = samopendir(dirname);
	if (dir_fd < 0) {
		BUMP_STAT(errors);
		BUMP_STAT(errors_dir);
		error(0, errno, catgets(catfd, SET, 613,
		    "Cannot open %s"), path);
		return;
	}
	if (sam_stat(dirname, &statb, sizeof (statb)) < 0) {
		error(1, errno, catgets(catfd, SET, 13506,
		    "Unable to sam_stat() file %s"), dirname);
	}

	if (initial) {

		/*
		 * If an initial directory, save the dev for EXDEV check.
		 */
		initial_dev = statb.st_dev;
		/*
		 * Dump the initial directory if it's not the mount point
		 * or '.', and it's not already dumped.
		 */
		if (strip_path_items(dirname, &start_ptr) != 0 &&
		    search_saved_dir_list(dirname) == 0) {
			sam_id_t	id;

			strcpy(name, dirname);
			name_append_point = name + strlen(name);
			id.ino = statb.st_ino;
			id.gen = statb.gen;
			dump_directory_entry(id, dirname, name_append_point,
			    "");
		}
	} else if (statb.st_dev != initial_dev) {
		return;
	}

	/* preload name with the directory's name */
	strcpy(name, dirname);
	name_append_point = name + strlen(name);

	/* loop to process each entry in the directory except . and .. */

	while (directory_complete != TRUE) {
		num_inodes = 0;
		while (sam_getdent(&dirent) > 0) {
			int name_length;

			/*
			 * Skip dumping some special inode numbers. Note,
			 * .stage was added in version 2. We only need to check
			 * SAM_STAGE_INO when we drop version 1 support.
			 */
			if (dirent->d_id.ino == SAM_INO_INO ||
			    dirent->d_id.ino == SAM_HOST_INO ||
			    dirent->d_id.ino == SAM_ARCH_INO ||
			    (dirent->d_id.ino == SAM_STAGE_INO &&
			    dirent->d_id.gen == SAM_STAGE_INO &&
			    strcmp((char *)dirent->d_name, ".stage") == 0)) {
				continue;
			}

			if (strcmp((const char *)dirent->d_name, "..") == 0 ||
			    strcmp((const char *)dirent->d_name, ".") == 0) {
				continue;
			}
			if (filename != NULL &&
			    strcmp((const char *)dirent->d_name, filename) ==
			    0) {
				/*
				 * Found the single file name specified on
				 * the command line.
				 */
				dump_directory_entry(dirent->d_id,
				    (char *)dirent->d_name,
				    name_append_point, path);
				goto free_names;
			}
			/*
			 * Add directory entry to the inode list.
			 */
			name_length = strlen((char *)dirent->d_name) + 1;
			if (inode_list == NULL) {	/* Initial entry */
				long	malloc_size;

				malloc_size = sizeof (struct inode_list) *
				    max_inodes;
				SamMalloc(inode_list, malloc_size);
				SamMalloc(names_space, sizeof (struct names));
				cur_namesp = names_space;
				cur_namesp->next = NULL;
				SamMalloc(cur_namesp->first, NAMES_INCR);
				cur_namesp->unused = cur_namesp->first;
				cur_namesp->chars_remaining = NAMES_INCR;
			} else if (cur_namesp->chars_remaining < name_length) {
				if (cur_namesp->next == NULL) {
					SamMalloc(cur_namesp->next,
					    sizeof (struct names));
					cur_namesp->next->next = NULL;
				}
				cur_namesp = cur_namesp->next;
				SamMalloc(cur_namesp->first, NAMES_INCR);
				cur_namesp->unused = cur_namesp->first;
				cur_namesp->chars_remaining = NAMES_INCR;
			}
			strncpy(cur_namesp->unused, (char *)dirent->d_name,
			    name_length);
			inode_list[num_inodes].name = cur_namesp->unused;
			inode_list[num_inodes].id = dirent->d_id;
			cur_namesp->unused += name_length;
			cur_namesp->chars_remaining -= name_length;
			if (++num_inodes >= max_inodes) {
				break;
			}
		}
		/*
		 * Process a pile of inodes.
		 */
		if (num_inodes > MIN_SORT) {
			/*
			 * Sort the inode list by inode number and gen number.
			 */
			qsort((void *)inode_list, (size_t)num_inodes,
			    (size_t)(sizeof (struct inode_list)), cmp_id);
		}
		for (next_inode = 0; next_inode < num_inodes; next_inode++) {
			dump_directory_entry(inode_list[next_inode].id,
			    inode_list[next_inode].name,
			    name_append_point, path);
		}
		if (num_inodes >= max_inodes) {
			/*
			 * More directory entries to process. Reset the
			 * names lists.
			 */
			num_inodes = 0;
			cur_namesp = names_space;
			do {
				cur_namesp->unused = cur_namesp->first;
				cur_namesp->chars_remaining = NAMES_INCR;
				cur_namesp = cur_namesp->next;
			} while (cur_namesp != NULL);
			cur_namesp = names_space;
		} else {
			directory_complete = TRUE;
		}
	}
free_names:
	if (inode_list != NULL) {
		/*
		 * Release the name lists.
		 */
		do {
			cur_namesp = names_space->next;
			free(names_space->first);
			free(names_space);
			names_space = cur_namesp;
		} while (names_space != NULL);
		free(inode_list);
	}
}

void	csd_dump_files(
	FILE		*DL,		/* File list descriptor		*/
	char		*filename	/* File name			*/
)

{
	char		buf[MAXPATHLEN + MAXPATHLEN + 60];
	char		*argv[4];
	int		argc;
	sam_id_t	id;
	char		*path;
	char		*append_point;
	char		*p;

	readin_ln = 0;			/* reset line number		*/

	while (readin(DL, buf, 2*MAXPATHLEN+59) == 0) {
		if (parsetabs(buf, argv, &argc, 3)) {
			error(0, 0, catgets(catfd, SET, 13507, "Format error"
			    " on file:%s line:%d: insufficient fields"),
			    filename, readin_ln);
			continue;
		}
		id.ino = atol(argv[0]);
		id.gen = atol(argv[1]);
		path   = argv[2];
		p = strrchr(path, '/');

		if (p == NULL) {
			error(0, 0, catgets(catfd, SET, 13511, "Format error"
			    " on file: %s line:%d: no / in path"),
			    filename, readin_ln);
			continue;
		}

		*p = '\0';
		if (SAM_fd == - 1) {
			SAM_fd = open_samfs(path);
		}
		strcpy(name, path);
		append_point = name + strlen(path);
		dump_directory_entry(id, p+1, append_point, name);
	}
}


/* a debugging function */
static void
display_saved_dir_list()
{
	/* start at last valid character */
	char *this_character = work_dir + work_dir_used;
	char *terminal_character, *path;
	int l_work_dir_used = work_dir_used;

	/* while there are still characters we haven't printed yet */
	while (l_work_dir_used > 0) {

		/* last char of this name */
		terminal_character = this_character;

		/*
		 * scan backwards from the last character of the pathname until
		 * we reach the beginning of the work list or we find a null
		 * character.
		 */
		for (this_character = this_character - 2;
		    this_character >= work_dir && *this_character;
		    this_character--) {
			/*LINTED empty loop body */
		}
		this_character++;  /* now points to first char of a pathname */

		path = (char *)strdup(this_character);
		if (path == NULL) {
			error(1, errno,
			    catgets(catfd, SET, 1891,
			    "Out of memory for a directory name."));
		}

		l_work_dir_used -= Ptrdiff(terminal_character, this_character);

		if (debugging) {
			fprintf(stderr, "saved_dir_list(%d): %s\n",
			    Ptrdiff(terminal_character, work_dir), path);
		}

		free(path);
	}
}

/*
 * ----- process_saved_dir_list - process saved directory list
 *	Take the newest entry off the saved directory list.  Pass it to
 *	csd_dump_path to be dealt with.  Assume that process_path may place
 *	additional entries on the saved directory list.  Keep looping
 *	until the saved directory list is empty.
 */
void
process_saved_dir_list(char *dirname)
{
	char *this_character;
	char *terminal_character, *path;

	if (debugging) {
		display_saved_dir_list();
	}

	if (debugging) {
		fprintf(stderr, "Process_saved_dir_list()\n");
	}

	while (work_dir_used > 0) {
		/* find the beginning of the last name in the saved list */
		terminal_character = work_dir + work_dir_used;
		for (this_character = terminal_character - 2;
		    this_character >= work_dir && *this_character;
		    this_character--) {
			/*LINTED empty loop body */
		}
		this_character++;

		/* make copy of the name at the end of the worklist */
		path = (char *)strdup(this_character);
		if (path == NULL) {
			error(1, errno,
			    catgets(catfd, SET, 1892,
			    "Out of memory for a directory named %s"),
			    this_character);
		}

		/* remove that entry from the saved list */
		work_dir_used -= Ptrdiff(terminal_character, this_character);

		if (debugging) {
			fprintf(stderr, "(%d) work_dir_used %d ",
			    Ptrdiff(terminal_character, work_dir),
			    work_dir_used);
		}

		/* process that directory name unless it's the initial one */
		if (strcmp(path, dirname) != 0) {
			csd_dump_path("recursive", path, NULL);
		}
		free(path);
	}
	if (debugging) {
		fprintf(stderr, "Process_saved_dir_list()\n");
	}
}


/*
 * ----- search_saved_dir_list - search saved directory list
 * Compare dir to each entry in the saved directory list.
 * Returns: 1 - entry found
 *			0 - entry not found
 */
static int
search_saved_dir_list(char *dir)
{
	char *terminal_character;
	char *this_character;
	int s_work_dir_used = work_dir_used;

	if (debugging) {
		display_saved_dir_list();
	}

	if (debugging) {
		fprintf(stderr, "Search_saved_dir_list() wdu %d\n",
		    work_dir_used);
	}

	while (s_work_dir_used > 0) {
		/* find the beginning of the last name in the saved list */
		terminal_character = work_dir + s_work_dir_used;
		for (this_character = terminal_character - 2;
		    this_character >= work_dir && *this_character;
		    this_character--) {
			/*LINTED empty loop body */
		}
		this_character++;
		if (strcmp(dir, this_character) == 0) {
			return (1);
		}

		s_work_dir_used -= Ptrdiff(terminal_character, this_character);

		if (debugging) {
			fprintf(stderr, "(%d) work_dir_used %d ",
			    Ptrdiff(terminal_character, work_dir),
			    work_dir_used);
		}

	}
	if (debugging) {
		fprintf(stderr, "Search_saved_dir_list() failed\n");
	}
	return (0);
}


static void
dump_file_data(char *name, int partial, int file_fd)
{
	struct sam_stat statb;
	int				name_l;

	if (debugging) {
		fprintf(stderr, "dump_file_data(%s)\n", name);
	}
	if (sam_stat(name, &statb, sizeof (statb)) < 0) {
		error(1, errno, catgets(catfd, SET, 13506,
		    "Unable to sam_stat() file %s"), name);
	}
	if (partial != 0) {
		if (!SS_ISRELEASE_P(statb.attr) || !SS_ISPARTIAL(statb.attr)) {
			return;
		}
	}
	name_l = strlen(name);
	BUMP_STAT(data_files);
	BuildHeader(name, name_l, &statb, partial);
	if (partial == 0) {
		copy_file_data_to_dump(file_fd, statb.st_size, name);
		csd_statistics.data_dumped += statb.st_size;
	} else {
		copy_file_data_to_dump(file_fd,
		    statb.partial_size*SAM_DEV_BSIZE, name);
		csd_statistics.data_dumped += statb.partial_size*SAM_DEV_BSIZE;
	}
	(void) close(file_fd);
}


/*
 * Build tar header record.
 */
void
BuildHeader(
	char *name,		/* File name. */
	int name_l,		/* Length of file name. */
	struct sam_stat *st,	/* File status. */
	int partial)		/* Nonzero if dumping partial data only */
{
	struct sam_stat sta;	/* extra for long name builds */

	if (name_l >= NAMSIZ) {
		sta.st_size = name_l;
		WriteHeader("././@LongLink", &sta, LF_LONGNAME);
		writecheck(name, name_l, 6666);
	}
	if (S_ISREG(st->st_mode)) {
		if (partial != 0) {
			WriteHeader(name, st, LF_PARTIAL);
		} else {
			WriteHeader(name, st, LF_NORMAL);
		}
	} else {
		fprintf(stderr, "illegal mode specified\n");
	}
}


/*
 * check_directory_excluded() returns true if the dirname
 * (stripped of leading / or ./ and trailing / or /.) matches
 * an excluded directory.
 */
static int
check_directory_excluded(char *dir_nm)
{
	char *st_dir;
	int dir_len;
	int element;

	if ((dir_len = strip_path_items(dir_nm, &st_dir)) != 0) {
		for (element = 0; element < nexcluded; element++) {
			if (strlen(excluded[element]) != dir_len) {
				continue;
			}
			if (strncmp(excluded[element], st_dir, dir_len) == 0)
				return (1);
		}
	}
	return (0);
}


/*
 *	strip_path_items strips the potential leading "/", "./" and trailing
 *		"/" and "/." from a path string. The return value in the string
 *		length in characters. Also returned is a character pointer to
 *		the first element of the string.
 *
 *		Note: The original string will be not modified.
 *
 *		If the resultant string needs to be terminated, use
 *		ptr+length to do so.
 */
int
strip_path_items(
	char *cp,
	char **start_ptr)
{
	int length;
	char *sp, *ep;

	sp = cp;

	if ((cp == NULL) || (start_ptr == NULL)) {
		return (0);
	}

	length = strlen(sp);

	while (length > 0) {
		if (*sp == '/') {
			sp++;
			length--;
			continue;
		} else if (*sp == '.') {
			if (((length - 1) > 0) && (*(sp + 1) == '/')) {
				sp += 2;
				length -= 2;
				continue;
			} else if (*(sp + 1) == '\0') {
				sp++;
				length--;
				break;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	*start_ptr = sp;

	ep = sp + length - 1;

	while (length > 0) {
		if (*ep == '/') {
			ep--;
			length--;
			continue;
		} else if (*ep == '.') {
			if (((length - 1) > 0) && (*(ep - 1) == '/')) {
				ep -= 2;
				length -= 2;
			} else {
				break;
			}
		} else {
			break;
		}
	}

	return (length);
}

/*
 * cmp_id - compare two sam_id_t values for qsort.
 */

static int
cmp_id(
	const void *id1,
	const void *id2)
{
	uint64_t *long1 = (uint64_t *)id1;
	uint64_t *long2 = (uint64_t *)id2;

	if (*long1 > *long2) {
		return (1);
	}
	if (*long1 < *long2) {
		return (-1);
	}
	return (0);
}
