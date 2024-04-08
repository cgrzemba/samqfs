/*
 *  csd.h -  Control structure dump record defintions.
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

#ifndef _FND_FX_CSD_H
#define	_FND_FX_CSD_H

#pragma ident "$Revision: 1.16 $"

#include <sam/fs/ino_ext.h>
#include <sys/param.h>

#define	CSD_MAGIC	0x63647378	/* Dump identifier word "csdx"	*/
#define	CSD_VERS_2	2		/* version number 2	*/
#define	CSD_VERS_3	3		/* version number 3 */
#define	CSD_VERS	4		/* current version number */


/* ----	csd header record - Dump header record. */

extern struct csd_header {
	int magic;
	int version;
	time_t time;
}   csd_header;

typedef struct csd_header CSD_HDR_RECORD;

/* -----	csd_statistics  */


typedef	struct	{
	int	errors;			/* No. of errors (not warnings)	*/
	int	errors_dir;		/* No. of unreadable directories */

	int	files;			/* No. of files			*/
	int	file_archives;		/* No. of file archive entries	*/
	int	file_damaged;		/* No. of damaged files		*/
	int	file_warnings;		/* No. of file damaged warnings	*/
	int	dirs;			/* No. of directories		*/
	int	resources;		/* No. of resource files	*/
	int	links;			/* No. of symbolic links	*/
} CSD_STATISTICS;
extern CSD_STATISTICS	csd_statistics;
#define	BUMP_STAT(field) {}

/*
 * Options must be integral powers of two so that we can do
 * bitwise checking.
 */
typedef enum {
	NONE = 0, DUMP = 1, RESTORE = 2, LIST = 4, CONVERT = 8, VFY_LIST = 16
} major_function;

typedef enum {false = 0, true = 1} boolean;

extern int csd_version;
extern boolean debugging;
extern boolean replace;
extern boolean verbose;
extern boolean quiet;
extern boolean replace;
extern boolean verbose;
extern boolean list_by_inode;
extern int lower_inode;
extern int upper_inode;
extern int dont_process_this_entry;
extern int copy_array[];

extern int SAM_fd;
extern int CSD_fd;

/* prototypes */
void    abort(void);
size_t	bflush(int fildes);
size_t  buffered_read(int fildes, const void  *buf, size_t nbyte);
size_t  buffered_write(int fildes, const void  *buf, size_t nbyte);
int		display_saved_dir_list(void);
void	cs_list(int fcount, char **flist);
void	cs_restore(boolean strip_slashes, int fcount, char **flist);
void	csd_dump_path(char *tag, char *path);
void	csd_read(char *name, int namelen,
			struct sam_perm_inode *perm_inode);
void	csd_read_next(struct sam_perm_inode *perm_inode,
			struct sam_vsn_section *vsn_information,
			char *linkname, struct sam_resource_file *resource);
void	csd_write(char *name,
			struct sam_perm_inode *perm_inode,
			struct sam_vsn_section *vsn_information,
			char *link, struct sam_resource_file *resource);
void	dump_directory_entry(struct sam_dirent *dirent,
			char *name_append_point, char *path);
void	dump_dirent(int i, struct sam_dirent dirent);
int		fake_ioctl(int SAM_fd, int function,
			struct sam_perm_inode *perm_inode,
			struct sam_vsn_section *vsn_information[]);
int		filecmp(char *a, char *b);
int		get_id(char *path, sam_id_t *id);
int		is_demo_license(void);
int		main(int argc, char *argv[]);
char	*mode_string(mode_t mode, char str[]);
int		open_samfs(char *filename);
int		opendir(char *dir_name);
void	pop_directory_stack(void);
void	print_stats(void);
void	process_saved_dir_list(void);
void	readcheck(void *buffer, size_t size, char *explanation);
void	read_old_resource_record(struct sam_resource_file *resource,
		int rmf_size);
int		sam_getdent(struct sam_dirent ** dirent);
int	sam_getdents(int dir_fd, char *dirent, int *offset, int dirent_size);
void	sam_ls(char *path, struct sam_perm_inode *perm_inode,
			struct sam_vsn_section *vsn_information,
			char *link, struct sam_resource_file *rs);
void	sam_restore_a_file(char *path,
			struct sam_perm_inode *perm_inode,
			struct sam_vsn_section *vsn_information,
			char *link, struct sam_resource_file *resource);
void	save_dir_name(char *path, char *name);
void	set_fa(char *path, struct sam_disk_inode *inode);
void	set_lat(char *path, struct sam_perm_inode *inode);
void	stack_directory(char *path,
			struct sam_perm_inode *perm_inode);
void	usage(void);
void	writecheck(void *buffer, size_t size, char *explanation);

#endif /* _FND_FX_CSD_H */
