/*
 *	csd_defs.h -	Control structure dump/restore internal structures.
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

#ifndef _DUMP_RESTORE_CSD_DEFS_H
#define	_DUMP_RESTORE_CSD_DEFS_H

#pragma ident "$Revision: 1.14 $"

#include <sys/param.h>
#include <sys/acl.h>
#include <sam/fs/ino_ext.h>
#include "csd.h"

#if defined(lint)
#include <sam/lint.h>
#endif /* defined(lint) */

/* csd_statistics	*/


struct csd_stats	{
	int	errors;		/* No. of errors (not warnings) */
	int	errors_dir;	/* No. of unreadable directories */

	int	segments;	/* No. of segments */
	int	files;		/* No. of files */
	int	file_archives;	/* No. of file archive entries */
	int	file_damaged;	/* No. of damaged files */
	int	file_warnings;	/* No. of file damaged warnings */
	int	hlink;		/* No. of hardlink files */
	int	hlink_first;	/* No. of first hardlink files */
	int	dirs;		/* No. of directories */
	int	resources;	/* No. of resource files */
	int	links;		/* No. of symbolic links */
	int	specials;	/* No. of special files */
	int	data_files;	/* No. of files with data */
	int	xattr_files;	/* No. of files with xattr */
	longlong_t data_dumped;	/* No. of data bytes dumped */
};

extern struct csd_stats	csd_statistics;

#define	BUMP_STAT(field) { csd_statistics.field++; }

#define	CSDTMAGIC "ustar"

/* borrowed from sam/fs/inodes.h */
#define SAM_INO_IS_XATTR(dip2)  ((dip2)->p2flags & P2FLAGS_XATTR)
#define SAM_INODE_IS_XATTR(ip)  SAM_INO_IS_XATTR(&((ip)->di2))

struct csd_tar {
	longlong_t	csdt_bytes;		/* file bytes to follow */
	char		*csdt_name;		/* name */
	struct sp_list	*csdt_sparse;		/* sparse list */
};

typedef	struct csd_tar csd_tar_t;

/* hardlink table, N_HARDLINKS is the initial size of the hardlink table */

#define	N_HARDLINKS 10000000	/* 10 million inodes for each allocation */

typedef struct csd_hardtbl {
	sam_id_t	id[1];
} csd_hardtbl_t;

/* options must be integral powers of two so that we can do bitwise checking */
typedef enum {NONE = 0, DUMP = 1, RESTORE = 2, LIST = 4, LISTDUMP = 5}
		major_function;

typedef enum {false = 0, true = 1} boolean;

extern int csd_version;
extern int Directio;
extern boolean debugging;
extern boolean replace;
extern boolean replace_newer;
extern boolean verbose;
extern boolean qfs;
extern boolean quiet;
extern boolean replace;
extern boolean online_data;
extern boolean partial_data;
extern boolean scan_only;
extern boolean swapped;
extern boolean unarchived_data;
extern boolean use_file_list;
extern boolean verbose;
extern boolean list_by_inode;
extern int lower_inode;
extern int upper_inode;
extern int dont_process_this_entry;
extern int dumping_file_data;
extern int copy_array[];
extern long read_buffer_size;
extern long write_buffer_size;
extern long block_size;
extern char *program_name;
extern char *excluded[];
extern int nexcluded;
extern int prelinks;
extern int stale_flags;

extern int SAM_fd;
extern int CSD_fd;

extern FILE *log_st;
extern FILE *DB_FILE;
extern FILE *DL_FILE;

extern csd_hdrx_t csd_header;

/* prototypes */
void	abort(void);
void	bflush(int fildes);
char	*buf_fill(int fildes, size_t nbyte);
size_t	buffered_read(int fildes, const void	*buf, size_t nbyte);
size_t	buffered_write(int fildes, const void  *buf, size_t nbyte);
void	copy_file_data_to_dump(int fildes, u_longlong_t nbyte, char *name);
u_longlong_t copy_file_data_fr_dump(int fildes, u_longlong_t nbyte,
				    char *name);
void	cs_list(int fcount, char **flist);
void	cs_restore(boolean strip_slashes, int fcount, char **flist);
void	csd_dump_path(char *tag, char *path, mode_t fmode);
void	csd_read(char *name, int namelen,
			struct sam_perm_inode *perm_inode);
void	csd_read_mve(struct sam_perm_inode   *perm_inode,
			struct sam_vsn_section  **vsnpp);
void	csd_read_next(struct sam_perm_inode *perm_inode,
			struct sam_vsn_section **vsn_information,
			char *linkname, void *data,
			int *n_aclp, aclent_t **aclpp);
int		csd_read_header(csd_fhdr_t *hdr);
int		csd_write(char *name,
			struct sam_perm_inode *perm_inode,
			int n_vsns,
			struct sam_vsn_section *vsn_information,
			char *link, void *data,
			int n_acls, aclent_t *aclp,
			long flags);
int		csd_write_csdheader(int fildes, csd_hdrx_t *buf);
int		fake_ioctl(int SAM_fd, int function,
			struct sam_perm_inode *perm_inode,
			struct sam_vsn_section *vsn_information[]);
int		filecmp(char *a, char *b);
int		get_id(char *path, sam_id_t *id);
int		is_demo_license(void);
char	*mode_string(mode_t mode, char str[]);
int		open_samfs(char *filename);
int	samopendir(char *dir_name);
int	samattropen(char *dir_name);
void	process_saved_dir_list(char *dirname);
void	readcheck(void *buffer, size_t size, int msgNum);
void	read_old_resource_record(struct sam_resource_file *resource,
			int rmf_size);
void	print_reslog(FILE *log_st, char *pathname,
			struct sam_perm_inode *pid, char *status);
int		sam_getdent(struct sam_dirent ** dirent);
void	sam_db_list(char *path, struct sam_perm_inode *perm_inode,
			char *link, sam_vsn_section_t *vsnp);
void	sam_ls(char *path, struct sam_perm_inode *perm_inode, char *link);
void	set_fa(char *path, struct sam_disk_inode *inode);
void	set_lat(char *path, struct sam_perm_inode *inode);
void	skip_embedded_file_data();
void	skip_file_data(u_longlong_t nbytes);
void	skip_pad_data(long nbytes);
int		strip_path_items(char *cp, char **start_ptr);
void	writecheck(void *buffer, size_t size, int msgNum);
u_longlong_t write_embedded_file_data(int fildes, char *name);

#endif /* _DUMP_RESTORE_CSD_DEFS_H */
