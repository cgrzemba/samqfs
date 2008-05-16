/*
 *   archive_audit.c  - generate an archive audit.
 *
 *	Generate a file for each VSN and media type that contains the names
 *	of files on that media.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.26 $"


/* Feature test switches. */
	/* None. */

#define	MAIN

/* ANSI C headers. */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

/* POSIX headers. */
#include <sys/types.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <libgen.h>
#include <sys/vfs.h>
#include <sys/param.h>

/* SAM-FS headers. */
#include "pub/stat.h"
#include "pub/rminfo.h"
#include "sam/custmsg.h"
#include "sam/types.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "sam/fioctl.h"
#include "sam/uioctl.h"
#include "sam/fs/dirent.h"
#include "sam/fs/ino.h"
#include "sam/fs/sblk.h"
#include "sam/nl_samfs.h"
#include "sam/fs/macros.h"
#include "sam/fs/inode.h"
#include "sam/sam_trace.h"

/* Local headers. */
	/* None. */

/*
 * Exit status:
 * 0    = Audit completed successfully.
 * 6	= Non fatal: An issue with the filename or path to it.
 * 7	= Non fatal: Closing of directory failed.
 * 10	= Non fatal: sam_segement_vsn_stat failure.
 * 11	= Non fatal: sam_vsn_stat failure.
 * 12	= Non fatal: sam_readrminfo failure.
 * 13	= Non fatal: idstat of file failure.
 * 14	= Non fatal: getdent of directory failure.
 * 15	= Non fatal: Invalid segment size for file.
 * 30   = Fatal: Argument errors.
 * 31   = Fatal: Audit file issues.
 * 32	= Fatal: An issue with the root path or subdirectory was encountered.
 * 35   = Fatal: Malloc errors.
 */
#define	ES_ok			0
#define	ES_File			6
#define	ES_CloseFile	7
#define	ES_SVSN_Stat	10
#define	ES_VSN_Stat		11
#define	ES_ReadRM		12
#define	ES_IDstat		13
#define	ES_GetDent		14
#define	ES_Invalid_SEG	15
#define	ES_Args			30
#define	ES_OutputFile	31
#define	ES_Path			32
#define	ES_Malloc		35

int	exit_status	=	ES_ok;	/* Exit status */


/* Macros. */
#define	DIRBUF_SIZE	10240	/* Size of buffer for directory reads */
#define	SORT	"/usr/bin/sort"	/* Name of the sort program */

#define	FOPEN	fopen64

/* Types. */
	/* None. */

/* Structures. */
/* VSN information. */
struct VSN_info {
	media_t media;
	vsn_t	vsn;
	offset_t size;		/* Total size of all files on media */
	long	files;		/* Number of files */
	long	damaged;	/* Number of damaged */
				/* archive copies */
};

static char *_SrcFile = __FILE__;

/* Private data. */
static char *o_fname;			/* Audit file */
static int o_fname_specified = FALSE;
static FILE *o_st;			/* Audit file stream */

static char *base_name;			/* Start of basename in fullpath */
static char fullpath[MAXPATHLEN + 4];	/* Current full path name */
static int Verbose = FALSE;
static int CopyMask = 0;		/* interesting copies */
static int Damaged = 0;			/* damaged copies only */

/* Directory reading. */
static char *dir_buf = NULL;
static struct sam_dirent *dir;
static sam_ioctl_getdents_t getdent_s = { NULL, DIRBUF_SIZE, 0 };
static char *dir_names = NULL;	/* Unprocessed directory base names */
static int dir_buf_count;
static int dir_fd;
static int dn_size = 0;		/* Current size of dir_names */

/* Inode stat information. */
static struct sam_perm_inode inode;
static struct sam_ioctl_idstat idstat = { { 0, 0 }, sizeof (inode), &inode };
static int fs_fd;		/* File system fd for ioctl() */

/* Pointer to sam_stat structure for stat of data segments */
static struct sam_stat *seg_stat_buf = (struct sam_stat *)NULL;

/* Number of sam_stat structures pointed to by seg_stat_buf */
int seg_capacity = 0;

/* Current segment number of fullpath being archive audited */
int seg_num = 0;


/* VSN information. */
static struct VSN_info *vi_first = NULL;
static int vi_numof = 0;
static int vi_max_numof = 0;

/* Private functions. */
static void AddFile(media_t media, char *vsn, offset_t section_size,
			boolean_t CopyDamaged);
static void CheckArchiveStatus(void);
static void ListVsns(void);
static char *NormalizePath(char *path);
static void adddir(char *name);
static void dodir(char *name);
static int getdent(void);
static int getnumsegs(struct sam_disk_inode *di);
static int seg_stat_path(int *num_segs);


/* Public data. */
	/* None. */

/* External data. */
extern char *program_name;	/* Program name: used by error */

/* Function macros. */
	/* None. */

/* Signal catching functions. */
	/* None. */


int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	extern int optind;
	char *cwd;
	int c, i;
	int errors = 0;

	/*
	 * Process arguments.
	 */
	CustmsgInit(0, NULL);
	program_name = basename(argv[0]);
	while ((c = getopt(argc, argv, "c:df:Vv")) != EOF) {
		switch (c) {
		case 'c':
			i = atoi(optarg);
			if (i < 1 || i > 4) {
				errors++;
				fprintf(stderr,
				    "%s: -c %s: invalid copy number\n",
				    program_name, optarg);
			}
			CopyMask |= 1<<(i-1);
			break;

		case 'd':
			Damaged = TRUE;
			break;

		case 'f':
			o_fname = optarg;
			o_fname_specified = TRUE;
			break;

		case 'V':
		case 'v':
			Verbose = TRUE;
			break;

		case '?':
		default:
			errors++;
		}
	}

	if (optind == argc)  errors++;	/* No root_path */
	if (errors != 0) {
		fprintf(stderr, catgets(catfd, SET, 13001, "Usage: %s %s\n"),
		    program_name,
		    "[-c copy_number]... [-f audit_file] [-V] root_path");
		return (ES_Args);
	}


	if (CopyMask == 0) CopyMask = 0xf;
			/* If no -c,interested in all copies */

	if ((cwd = getcwd(NULL, sizeof (fullpath)-1)) == NULL) {
		error(1, errno, catgets(catfd, SET, 587, "Cannot get cwd"));
	}
	/*
	 * Open the audit output file.  Rule is:
	 *    If -f not specified, or "-f -" specified, then use stdout
	 *    else use filename from -f argument.
	 */
	if (o_fname_specified && strcmp(o_fname, "-") != 0) {
		uid_t uid = getuid();
		gid_t gid = getgid();

		if ((*o_fname != '/') && (cwd != NULL)) {
			strncpy(fullpath, cwd, sizeof (fullpath)-1);
			strncat(fullpath, "/", sizeof (fullpath)-1);
		} else  *fullpath = '\0';
		strncat(fullpath, o_fname, sizeof (fullpath)-1);
		if (NormalizePath(fullpath) == NULL) {
			error(ES_OutputFile, 0,
			    catgets(catfd, SET, 1423,
			    "Invalid output file path"));
		}
		if ((o_st = FOPEN(fullpath, "a")) == NULL) {
			error(ES_OutputFile, errno,
			    catgets(catfd, SET, 574,
			    "Cannot create %s"), fullpath);
		}
		if (chown(fullpath, uid, gid) < 0) {
			error(0, errno,
			    catgets(catfd, SET, 573,
			    "Cannot chown %s"), fullpath);
			exit_status = ES_OutputFile;
		}
	} else {
		o_st = stdout;
	}


	/*
	 * Check the path to audit.
	 */
	if ((*argv[optind] != '/') && (cwd != NULL)) {
		strncpy(fullpath, cwd, sizeof (fullpath)-1);
		strncat(fullpath, "/", sizeof (fullpath)-1);
	} else  *fullpath = '\0';
	strncat(fullpath, argv[optind], sizeof (fullpath)-1);
	if (NormalizePath(fullpath) == NULL)
		error(ES_Path, 0, catgets(catfd, SET, 1425,
		    "Invalid path to audit"));
	if ((dir_buf = malloc(DIRBUF_SIZE)) == NULL) {
		error(ES_Malloc, errno,
		    catgets(catfd, SET, 1773,
		    "No memory for directory buffer %d"),
		    DIRBUF_SIZE);
	}
	if ((fs_fd = open(fullpath, O_RDONLY)) < 0) {
		error(ES_Path, errno,
		    catgets(catfd, SET, 613, "Cannot open %s"), fullpath);
	}
	/*LINTED pointer cast may result in improper alignment */
	getdent_s.dir.ptr = (struct sam_dirent *)dir_buf;

	/*
	 * Examine the audit path.
	 */
	base_name = fullpath;
	dodir(fullpath);
	if (o_st != stdout) fclose(o_st);

	if (Verbose)  ListVsns();

	if (seg_stat_buf) {
		free(seg_stat_buf);

		seg_stat_buf = (struct sam_stat *)NULL;
		seg_capacity = 0;
	}

	return (exit_status);
}


/*
 *	Add a file to the vsn table.
 *	inode = inode information.
 */
static void
AddFile(
	media_t media,
	char *vsn,		/* VSN */
	offset_t section_size,	/* Section size */
	boolean_t CopyDamaged)	/* Damaged copy status */
{
	struct VSN_info *vi;
	int n;

	for (n = 0, vi = vi_first; n < vi_numof; n++, vi++) {
		if (vi->media != media)  continue;
		if (strcmp(vi->vsn, vsn) != 0)  continue;
		vi->files++;
		vi->size += section_size;
		if (CopyDamaged)  vi->damaged ++;
		return;
	}
	if (vi_numof >= vi_max_numof) {
		size_t size;
		/*
		 * No room for entry.  realloc() room for more entries.
		 */
		vi_max_numof += 50;
		size = vi_max_numof * sizeof (struct VSN_info);
		if ((vi_first = realloc(vi_first, size)) == NULL) {
			error(ES_Malloc, errno,
			    catgets(catfd, SET, 1778, "No memory for vsns"));
		}
	}
	vi = &vi_first[vi_numof++];
	vi->media = media;
	strncpy(vi->vsn, vsn, sizeof (vi->vsn)-1);
	vi->files = 1;
	vi->size = section_size;
	vi->damaged = CopyDamaged ? 1 : 0;
}


/*
 *	Check archive status.
 *	Determine data from all archive copies
 */
static void
CheckArchiveStatus(void)
{
	char *mn, *vsn;
	int copy, mask;
	boolean_t CopyDamaged;

	for (copy = 0, mask = 1; copy < MAX_ARCHIVE; copy++, mask += mask) {
		char sflags[5] = "----";

		if (!(mask & CopyMask))  continue;
		if (!(inode.di.arch_status & mask))  continue;
		vsn = inode.ar.image[copy].vsn;
		mn = (char *)sam_mediatoa(inode.di.media[copy]);
		if (*vsn == '\0')  vsn = "Blank";
		if (*mn == '\0')  mn = "??";
		if (inode.di.ar_flags[copy]) {
			if (inode.di.ar_flags[copy] & CF_STALE)
				sflags[0] = 'S';
			if (inode.di.ar_flags[copy] & CF_REARCH)
				sflags[1] = 'r';
			if (inode.di.ar_flags[copy] & CF_DAMAGED)
				sflags[3] = 'D';
		}
		if (inode.ar.image[copy].n_vsns > 1) {
			struct sam_section *vsns;
			struct sam_section *ivsns;
			offset_t size;
			int n;

			if ((vsns = (struct sam_section *)malloc
			    (SAM_SECTION_SIZE(
			    inode.ar.image[copy].n_vsns))) == NULL) {
				error(ES_Malloc, errno,
				    catgets(catfd, SET, 1606,
				    "malloc: %s\n"), "sam_vsn_stat");
			}

			if ((fullpath == NULL) || (*fullpath == '\0')) {
				if (errno == 0) {
					errno = EINVAL;
				}
				error(0, errno,
				    catgets(catfd, SET, 1088,
				    "Cannot sam_vsn_stat; file path is NULL."));
				if (exit_status < ES_File)
					exit_status = ES_File;
				free(vsns);
				continue;
			} else if (S_ISSEGS(&inode.di)) {
				if (sam_segment_vsn_stat(fullpath, copy,
				    (int)inode.di.rm.info.dk.seg.ord, vsns,
				    (int)SAM_SECTION_SIZE(
				    inode.ar.image[copy].n_vsns)) < 0) {
					error(0, errno,
					    catgets(catfd, SET, 1089,
					    "Cannot sam_segment_vsn_stat %s "
					    "(copy %d, segment %d"),
					    fullpath, (int)copy + 1,
					    (int)
					    inode.di.rm.info.dk.seg.ord + 1);
					if (exit_status < ES_SVSN_Stat)
						exit_status = ES_SVSN_Stat;
					free(vsns);
					continue;
				}
			} else {
				if (sam_vsn_stat(fullpath, copy, vsns,
				    (int)SAM_SECTION_SIZE(
				    inode.ar.image[copy].n_vsns)) < 0) {
					error(0, errno,
					    catgets(catfd, SET, 634,
					    "Cannot sam_vsn_stat %s"),
					    fullpath);
					if (exit_status < ES_VSN_Stat)
						exit_status = ES_VSN_Stat;
					free(vsns);
					continue;
				}
			}
			ivsns = vsns;
			for (n = 0; n < inode.ar.image[copy].n_vsns;
			    n++, ivsns++) {
				vsn = ivsns->vsn;
				size = ivsns->length;
				CopyDamaged = inode.di.ar_flags[copy] &
				    AR_damaged;
				AddFile(inode.di.media[copy], vsn, size,
				    CopyDamaged);
				if (!Damaged || (Damaged && CopyDamaged)) {
					fprintf(o_st,
					"%s %s %s %d %d %llx.%llx %lld %s %d\n",
					    mn, vsn, sflags, copy + 1, n,
					    ivsns->position, ivsns->offset,
					    size, fullpath, seg_num);
				}
			}
			free(vsns);
		} else {
			offset_t size;
			offset_t file_offset = inode.ar.image[copy].file_offset;
			if ((inode.ar.image[copy].arch_flags & SAR_size_block)
			    == 0) {
				file_offset /= 512;
			}
			CopyDamaged = inode.di.ar_flags[copy] & AR_damaged;

			if (S_ISSEGI(&inode.di)) {
				/* Inode is index inode. */
				size = (offset_t)inode.di.rm.info.dk.seg.fsize;
			} else {
				size = (offset_t)inode.di.rm.size;
			}

			AddFile(inode.di.media[copy], vsn, size, CopyDamaged);
			if (!Damaged || (Damaged && CopyDamaged)) {
				fprintf(o_st,
				"%s %s %s %d %d %llx.%llx %lld %s %d\n",
				    mn, vsn, sflags, copy + 1, 0,
				    (offset_t)inode.ar.image[copy].position,
				    file_offset,
				    size, fullpath, seg_num);
			}
		}
	}
}


/*
 *	Check removable media status.
 */
static void
CheckRmStatus(void)
{
	char *mn, *vsn;
	struct sam_rminfo rb, *rbp;
	int n;
	offset_t size;
	int rmsize = 0;
	media_t media;

	if (Damaged) {
		return;
	}
	rbp = &rb;
	if ((sam_readrminfo(fullpath, &rb, sizeof (rb))) < 0) {
		if (errno == EOVERFLOW) {
			rmsize = SAM_RMINFO_SIZE(rb.n_vsns);
			if ((rbp = malloc(rmsize)) == NULL) {
				error(ES_Malloc, ENOMEM,
				    catgets(catfd, SET, 1606, "malloc %s"),
				    "sam_rminfo");
			} else {
				if ((sam_readrminfo(
				    fullpath, rbp, rmsize)) < 0) {
					error(0, errno,
					    catgets(catfd, SET, 632,
					    "Cannot sam_readrminfo %s"),
					    fullpath);
					if (exit_status < ES_ReadRM)
						exit_status = ES_ReadRM;
					return;
				}
			}
		} else {
			error(0, errno,
			    catgets(catfd, SET, 632,
			    "Cannot sam_readrminfo %s"),
			    fullpath);
			if (exit_status < ES_ReadRM) exit_status = ES_ReadRM;
			return;
		}
	}
	mn = rbp->media;
	if (*mn == '\0')  mn = "??";
	for (n = 0; n < rbp->n_vsns; n++) {
		vsn = rbp->section[n].vsn;
		if (*vsn == '\0')  vsn = "Blank";
		media = sam_atomedia(mn);
		size = rbp->section[n].length;
		if (size != MAXOFFSET_T) {
			AddFile(media, vsn, size, 0);
			fprintf(o_st, "%s %s %s %d %d %llx.%x %lld %s %d\n",
			    mn, vsn, "----", 0, n,
			    rbp->section[n].position,
			    0, size, fullpath, seg_num);
		}
	}
	if (rmsize)  free(rbp);
}


/*
 *	List VSN usage.
 */
static void
ListVsns(void)
{
	struct VSN_info *vi;
	int n;

	for (n = 0, vi = vi_first; n < vi_numof; n++, vi++) {
		char *mn;

		mn = (char *)sam_mediatoa(vi->media);
		if (*mn == '\0')  mn = "??";
		printf(catgets(catfd, SET, 13032,
		    "%s %s %ld files, %lld bytes, %ld damaged copies\n"),
		    mn, vi->vsn, vi->files, vi->size, vi->damaged);
	}
}


/*
 *	Normalize path.
 *	Remove ./ ../ // sequences in a path.
 *	Note: path array must be able to hold one more character.
 * 	Returns Start of path.  NULL if invalid, (too many ../'s)
 */
static char *
NormalizePath(
	char *path)		/* Path to be normalized. */
{
	char *p, *ps, *q;

	ps = path;
	/* Preserve an absolute path. */
	if (*ps == '/')  ps++;

	strcat(ps, "/");
	p = q = ps;
	while (*p != '\0') {
		char *q1;

		if (*p == '.') {
			if (p[1] == '/') {
				/*
				 * Skip "./".
				 */
				p++;
			} else if (p[1] == '.' && p[2] == '/') {
				/*
				 * "../"  Back up over previous component.
				 */
				p += 2;
				if (q <= ps) {
					return (NULL);
				}
				q--;
				while (q > ps && q[-1] != '/')  q--;
			}
		}
		/*
		 * Copy a component.
		 */
		q1 = q;
		while (*p != '/')  *q++ = *p++;
		if (q1 != q) *q++ = *p;
		/*
		 * Skip successive '/'s.
		 */
		while (*p == '/')  p++;
	}
	if (q > ps && q[-1] == '/')  q--;
	*q = '\0';
	return (path);
}


/*
 *	Descend through the directories, starting at "name".
 *	Call dofile() for each directory entry.
 *	Save each directory name, and process at the end.
 */
static void
dodir(char *name)
{
	size_t dn_mark, dn_next;
	char *prev_base;
	int seg_stat_err;
	int num_segs;

	strcpy(base_name, name);

	/*
	 * Change to the new directory.
	 * Extend the full path.
	 */
	if ((dir_fd = open(name, O_RDONLY)) == -1) {
		error(0, errno,
		    catgets(catfd, SET, 3088, "Cannot open directory %s"),
		    fullpath);
		if (exit_status < ES_Path) exit_status = ES_Path;
		return;
	}
	getdent_s.offset = 0;
	dir_buf_count = 0;
	if (chdir(name) == -1 && chdir(fullpath) == -1) {
		error(0, errno,
		    catgets(catfd, SET, 3038, "cannot chdir to %s"),
		    fullpath);
		(void) close(dir_fd);
		if (exit_status < ES_Path) exit_status = ES_Path;
		return;
	}
	prev_base = base_name;
	base_name += strlen(name);
	if (base_name[-1] != '/') {
		*base_name++ = '/';
	}
	*base_name = '\0';

	/*
	 * Mark the directory name stack.
	 */
	dn_mark = dn_next = dn_size;

	while (getdent() > 0) {
		/* ignore dot and dot-dot */
		if (strcmp((const char *) dir->d_name, ".") == 0 ||
		    strcmp((const char *) dir->d_name, "..") == 0)
			continue;

		/*
		 * check to assure that construction of the full path name does
		 * not exceed the limit before doing the idstat call
		 */

		if ((int)(strlen(fullpath) + dir->d_namlen + 1) > MAXPATHLEN) {
			error(0, errno,
			    catgets(catfd, SET, 268,
			    "%s: Pathname too long"), fullpath);
			if (exit_status < ES_File) exit_status = ES_File;
			continue;
		}

		strcpy(base_name, (const char *) dir->d_name);
		idstat.id = dir->d_id;
		seg_num = 0;

		if (ioctl(fs_fd, F_IDSTAT, &idstat) < 0) {
			error(0, errno,
			    catgets(catfd, SET, 1090,
			    "Ioctl call to stat %s (%d, %d) failed."),
			    fullpath, (int)idstat.id.ino, idstat.id.gen);
			if (exit_status < ES_IDstat) exit_status = ES_IDstat;
			continue;
		}
		if (S_ISREQ(inode.di.mode)) {
			/*
			 * Try to eliminate removable media files which don't
			 * actually represent space on tape.  These are those
			 * for the archiver and stager, and those created by
			 * users for disaster recovery.
			 * For newly created filesystems, those for the
			 * archiver and stager will have parent inode
			 * SAM_ARCH_INO (5) and SAM_STAGE_INO (7) respectively.
			 * For legacy file systems we don't have a clue.
			 * Those created by users for reading
			 * (disaster recovery) which haven't been referenced
			 * will have size of MAXOFFSET_T, so throw these out too
			 * (in CheckRmStatus() ).
			 */
			if ((inode.di.parent_id.ino != SAM_ARCH_INO) &&
			    (inode.di.parent_id.ino != SAM_STAGE_INO)) {
				CheckRmStatus();
			}
		} else {
			CheckArchiveStatus();
		}

		if (S_ISSEGI(&inode.di)) {
			/*
			 * Inode is an index inode, perform archive_audit of
			 * any archive copies of the file's data segments.
			 */
			seg_stat_err = seg_stat_path(&num_segs);

			if (seg_stat_err) {
				continue;
			}

			for (seg_num = 1; seg_num <= num_segs; seg_num++) {
				idstat.id.ino =
				    (sam_ino_t)seg_stat_buf[seg_num - 1].st_ino;
				idstat.id.gen =
				    (int32_t)seg_stat_buf[seg_num - 1].gen;

				if (ioctl(fs_fd, F_IDSTAT, &idstat) < 0) {
					error(0, errno,
					    catgets(catfd, SET, 1091,
					    "Ioctl call to stat %s, "
					    "segment %d (%d, %d) failed."),
					    fullpath, seg_num,
					    (int)idstat.id.ino, idstat.id.gen);

					if (exit_status < ES_IDstat)
						exit_status = ES_IDstat;
					continue;
				}

				CheckArchiveStatus();
			}
		}

		if (S_ISDIR(inode.di.mode))  adddir((char *)dir->d_name);
	}

	if (close(dir_fd) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 3040,
		    "cannot close directory %s"), fullpath);
		if (exit_status < ES_CloseFile) exit_status = ES_CloseFile;
	}

	/*
	 * Process all the directories found.
	 */
	while (dn_next < dn_size) {
		char *name;

		name = &dir_names[dn_next];
		dn_next += strlen(name) + 1;
		dodir(name);
	}
	dn_size = dn_mark;

	base_name = prev_base;
	*base_name = '\0';
	if (chdir("..") < 0 && chdir(fullpath) < 0) {
		error(0, errno,
		    catgets(catfd, SET, 572,
		    "cannot chdir to \"..\" %s"), fullpath);
		if (exit_status < ES_Path) exit_status = ES_Path;
	}
}


/*
 *	Add a directory name to table.
 */
static void
adddir(char *name)
{
	static int dn_limit = 0;
	size_t l;

	l = strlen(name) + 1;
	if (dn_size + l >= dn_limit) {
		/*
		 * No room in table for entry.
		 * realloc() space for enlarged table.
		 */
		dn_limit += 5000;
		if ((dir_names = realloc(dir_names, dn_limit)) == NULL) {
			error(ES_Malloc, errno,
			    catgets(catfd, SET, 628,
			    "Cannot realloc for directory names %d"),
			    dn_limit);
		}
	}
	memcpy(&dir_names[dn_size], name, l);
	dn_size += l;
}


/*
 *	Get directory entry.
 *	Returns > 0 if entry to process.
 */
static int
getdent(void)
{
	static char *buf;
	int size;

	do {
		if (dir_buf_count <= 0) {
			if ((dir_buf_count =
			    ioctl(dir_fd, F_GETDENTS, &getdent_s)) < 0) {
				error(0, errno,
				    catgets(catfd, SET, 3039,
				    "Cannot read directory %s"),
				    fullpath);
				if (exit_status < ES_GetDent)
					exit_status = ES_GetDent;
				return (-1);
			} else if (dir_buf_count == 0) {
				return (0);	/* EOF */
			}
			buf = dir_buf;
		}
	/* LINTED pointer cast may result in improper alignment */
		dir = (struct sam_dirent *)buf;
		size = SAM_DIRSIZ(dir);
		buf += size;
		dir_buf_count -= size;
	} while (dir->d_fmt == 0);
	return (size);
}

/*
 *	Get total number of segments used by this file.
 */
static int
getnumsegs(struct sam_disk_inode *di)
{
	ASSERT(di != (struct sam_disk_inode *)NULL);

	if (SAM_SEGSIZE(di->rm.info.dk.seg_size) <= 0) {
		if (!errno) {
			errno = EINVAL;
		}
		error(0, errno,
		    catgets(catfd, SET, 1092, "File %s has segment size %d."),
		    fullpath, (int)di->rm.info.dk.seg_size);
		if (exit_status < ES_Invalid_SEG) exit_status = ES_Invalid_SEG;
		return (-1);
	} else {
		int num_segs;

		num_segs = (int)
		    (di->rm.size /
		    (offset_t)SAM_SEGSIZE(di->rm.info.dk.seg_size));

		if (di->rm.size >
		    ((offset_t)num_segs * SAM_SEGSIZE(
		    di->rm.info.dk.seg_size))) {
			num_segs++;
		}

		return (num_segs);
	}
}

/*
 *	Perform the sam_segment_stat operation for the segmented file.
 */
static int
seg_stat_path(int *num_segs)
{
	int err;

	ASSERT(num_segs != (int *)NULL);

	*num_segs = getnumsegs(&inode.di);

	if (*num_segs < 0) {
		*num_segs = 0;

		return (-1);
	}

	if (*num_segs > seg_capacity) {
		if (seg_stat_buf) {
			free(seg_stat_buf);
		}

		seg_stat_buf = (struct sam_stat *)
		    malloc(sizeof (struct sam_stat)* *num_segs);

		if (seg_stat_buf == (struct sam_stat *)NULL) {
			error(0, errno, "malloc(sizeof(struct sam_stat)*%d)",
			    *num_segs);
			seg_capacity = 0;
			if (exit_status < ES_Invalid_SEG)
				exit_status = ES_Invalid_SEG;
			return (-1);
		} else {
			seg_capacity = *num_segs;
		}
	}

	(void) memset((void *)seg_stat_buf, 0,
	    sizeof (struct sam_stat)* *num_segs);

	err = sam_segment_stat(fullpath, seg_stat_buf,
	    sizeof (struct sam_stat)* *num_segs);

	if (err) {
		error(0, errno,
		    "sam_segment_stat(%s, seg_stat_buf, "
		    "sizeof(struct sam_stat)*%d)",
		    fullpath, *num_segs);
		if (exit_status < ES_Invalid_SEG) exit_status = ES_Invalid_SEG;
		return (-1);
	} else {
		return (0);
	}
}
