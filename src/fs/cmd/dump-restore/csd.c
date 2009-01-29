/*
 *	csd.c -  csd create/restore a csd dump file.
 *
 *	Main for csd create/restore a csd dump file.
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


#pragma ident "$Revision: 1.16 $"

/*
 * Modified during 1997/01 to handle files with archive copies that
 * overflow volumes, as well as to clean up the restore interface.
 * Currently, the request command and the filesystem does not
 * support more than 8 VSNS. Dump format 4 is not tied to this
 * limitation.
 *
 * dump format:
 * version 4,5 & 6		version 3		   version 2
 *
 * For directories, regular file, symlinks, & removable media file.
 * int namelen			int namelen		int namelen
 * char name[namelen]		char name[namelen]	char name[namelen]
 * struct sam_perm_inode	struct sam_perm_inode	struct sam_perm_inode
 *		 optional fields:
 *
 * For a regular file with embedded data (version 5 and greater),
 * One or more tar headers, followed by file data.
 *
 * For a regular file that has archive copies where n_vsns > 1.
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 *
 * For a segment file, repeat until all segment inodes:
 * struct sam_perm_inode
 * For a segment inode that has archive copies where n_vsns > 1.
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 *
 * For a segment file, repeat until all segment inodes:
 * struct sam_perm_inode
 * For a segment inode that has archive copies where n_vsns > 1.
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 * arcopy copy: struct sam_section[0]
 *	...
 * struct sam_section[n_vsns[copy]]
 *
 * For a symlink
 * int linklen			int linklen		int linklen
 * char link[namelen]		char link[namelen]	char link[namelen]
 *
 * For a removable media file
 * sam_resource_file_t		old_sam_rminfo_t	old_sam_resource_file_t
 */


/*
 *	A note on dump version (contained in the dump header at the beginning
 *	of a non-headerless csd dump file):
 *
 *	Versions 0 and 1 were IDS format.
 *	Version 2 is Solaris format with sam_old_resource_file information
 *	Version 3 is Solaris format with sam_old_rminfo information
 *	Version 4 is Solaris format with sam_resource_file information and
 *	volume overflow handling.
 *	Version 5 is Solaris format with embedded data (csd info followed by
 *	a tar header, then file data).
 *	Version 6 is Solaris format with embedded data (csd info followed by
 *	a tar header, then file data). It always dump with Extended CSD
 *	header. It has a fix to zero out the di2 area of the inode in all
 *	dumps taken at Version 5 and lower if it is SAM_INODE_VERS_2 and not
 *	a WORM file when restoring.  The dump file itself is not modified.
 */

#define	MAIN

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <pub/stat.h>
#include <sys/dirent.h>
#include <pub/rminfo.h>
#include <sam/uioctl.h>
#include <unistd.h>
#include <sam/fioctl.h>
#include <sam/fs/dirent.h>
#include <sam/fs/ino.h>
#include <sam/fs/bswap.h>
#include <limits.h>
#include <libgen.h>
#include <stdlib.h>

#include "sam/types.h"
#include "sam/defaults.h"
#include "sam/nl_samfs.h"
#include "sam/devnm.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "aml/shm.h"
#include "aml/proto.h"
#include "sam_ls.h"
#include "csd_defs.h"


/*	Local definitions */

#define	RESTORE_OPT	"df:g:ilrRsStTv2B:b:Z:"
#define	QFSRESTORE_OPT	"df:ilrRstTv2B:b:D"
#define	DUMP_OPT	"df:HI:nPqSTuUvWB:b:X:YZ:"
#define	QFSDUMP_OPT	"df:HI:qTvB:b:DX:"

#define	STDIN	0	/* Ordinal of stdin */
#define	STDOUT	1	/* Ordinal of stdout */

/*	External global declarations */

extern char *optarg;
extern int optind;

/* Global declarations */

struct csd_stats csd_statistics;
int ls_options = 0;

char *program_name;			/* Program name */
int exit_status;

int CSD_fd;			/* File descriptor for CSD file */
int SAM_fd = -1;		/* File descriptor for SAM-FS file */
FILE *log_st = NULL;		/* File handle for restore output */
FILE *DB_FILE = NULL;		/* File handle for data base load file */
FILE *DL_FILE = NULL;		/* File handle for dump file list */
boolean verbose;		/* Verbose output flag */
boolean quiet;			/* suppress Warnings */
boolean debugging;		/* Debugging option */
int prelinks = 0;		/* Allocated hard links table */
boolean replace_newer;		/* Replace file if dump is newer option */
boolean replace;		/* Replace file option */
boolean qfs;			/* Dump for QFS (file data included) */
boolean online_data;		/* Dump online data */
boolean partial_data;		/* Dump partial-online data */
boolean scan_only;		/* Scan file system/dump file for db */
boolean swapped;		/* Dump is different endian than this machine */
boolean unarchived_data;	/* Dump unarchived data */
boolean use_file_list;		/* Dump using file list */
boolean list_by_inode;		/* list inode range only */
int csd_version = 0;		/* solaris csd format */
int Directio = 0;		/* use direct io when reading */
				/* or writing samfs data */
int lower_inode;
int upper_inode;
int dont_process_this_entry;
long read_buffer_size;
long write_buffer_size;
long block_size;

csd_hdrx_t	csd_header;

/*
 * fast lookup array to determine the number of copies existing without
 * having to individually shift and mask bits
 */
int copy_array[] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};

int nexcluded;
int nincluded;
char *excluded[CSD_MAX_EXCLUDED];
char *included[CSD_MAX_INCLUDED];


/* Module-private declarations */

static boolean noheaders;		/* No header option */
static boolean statistics;		/* Statistics flag */
static boolean strip_slashes;		/* Strip leading /'s on restore */
static char *Initial_path;		/* Initial working directory */
static major_function operation;


/* External function calls */

extern void *sam_mastershm_attach(int, int);		/* from libsamut */

void csd_dump_files(FILE *DL, char *filename);

/* Local function calls */

static void usage();
static void print_stats();
static char *get_path(int argc, char **argv);

/* Main entry point */

int
main(int argc, char *argv[])
{
	char *dump_file = (char *)NULL;	/* Dump path/file name */
	char *load_file = (char *)NULL;	/* DB load path/file name */
	char *optstring;		/* Command option string */
	char *logfile = NULL;	/* Log file name for samfsrestore -g option */
	struct sam_stat sb;
	int option;		/* Current getopt option being processed */
	int	io_sz, read_size;
	int len;
	operator_t	operator;	/* Operator data */
	shm_alloc_t	master_shm;	/* Master shared memory structure */

	program_name = basename(argv[0]);

	operation = NONE;
	replace_newer = false;
	replace = false;
	swapped = false;
	verbose = false;
	quiet = false;
	noheaders = false;
	strip_slashes = false;
	list_by_inode = false;
	online_data = false;
	partial_data = false;
	scan_only = false;
	unarchived_data = false;
	use_file_list = false;
	read_buffer_size = write_buffer_size = CSD_DEFAULT_BUFSZ;
	block_size = 0;
	memset(&csd_header, '\0', sizeof (csd_header));

	CustmsgInit(0, NULL);
	if ((Initial_path = getcwd(NULL, (MAXPATHLEN + 1))) == NULL) {
		error(1, errno, catgets(catfd, SET, 590, "cannot getcwd()"));
	}

	/*
	 *	If basename is samfsrestore, fake out "r" option; if
	 *	samfsdump, fake out "c" option.  Allow redundant specification
	 *	specifications like "samfsrestore r", "samfsdump c".	Allow
	 *	"samfsrestore t" to override "r" option so that we look
	 *	like "ufsrestore t".	If basename is none of the above, then
	 *	user must specify one of c,r,t,O.
	 */

	if (strcmp(program_name, "samfsrestore") == 0) {
		operation = RESTORE;
		optstring = RESTORE_OPT;
		qfs = false;
	} else if (strcmp(program_name, "samfsdump") == 0) {
		operation = DUMP;
		optstring = DUMP_OPT;
		qfs = false;
	} else if (strcmp(program_name, "qfsdump") == 0) {
		operation = DUMP;
		optstring = QFSDUMP_OPT;
		qfs = true;
	} else if (strcmp(program_name, "qfsrestore") == 0) {
		operation = RESTORE;
		optstring = RESTORE_OPT;
		qfs = true;
	}
	if (operation == NONE) {
		error(1, 0, catgets(catfd, SET, 13508,
		    "Improper invocation of SAM-FS csd "
		    "utility %s"),
		    program_name);
	}

	for (nexcluded = CSD_MAX_EXCLUDED - 1; nexcluded >= 0; nexcluded--) {
		excluded[nexcluded] = NULL;
	}
	nexcluded = 0;
	for (nincluded = CSD_MAX_INCLUDED - 1; nincluded >= 0; nincluded--) {
		included[nincluded] = NULL;
	}
	nincluded = 0;

	while ((option = getopt(argc, argv, optstring)) != EOF) {
		switch (option) {
		case 'B':		/* select buffer size */
			io_sz = atoi(optarg) * 512;
			if (io_sz < CSD_MIN_BUFSZ || io_sz > CSD_MAX_BUFSZ) {
				error(1, 0,
				    catgets(catfd, SET, 13509,
				    "Invalid buffer size, must "
				    "be >= %d and <= %d, "
				    "specified in 512b units"),
				    CSD_MIN_BUFSZ/512, CSD_MAX_BUFSZ/512);
			}
			read_buffer_size = write_buffer_size = io_sz;
			break;

		case 'b':		/* select blocking factor */
			io_sz = atoi(optarg) * 512;
			if (io_sz < 0 || io_sz > CSD_MAX_BUFSZ) {
				error(1, 0, catgets(catfd, SET, 13510,
				    "Invalid blocking factor, "
				    "must be >= 0 and <= %d, "
				    "specified in 512b units"),
				    CSD_MAX_BUFSZ/512);
			}
			block_size = io_sz;
			break;

		case 'd':		/* enable debug messages */
			debugging = 1;
			break;

		case 'D':		/* enable directio */
			Directio++;
			break;

		case 'I':		/* file of paths to process */
			if (nincluded > CSD_MAX_INCLUDED) {
				error(1, 0, catgets(catfd, SET, 13537,
		"Maximum -I (include file) entries exceeded, max. is %d."),
				    CSD_MAX_INCLUDED);
			}
			included[nincluded] = optarg;
			len = strlen(optarg);
			if (len > 0) {
				*(included[nincluded] + len) = '\0';
				nincluded++;
			} else {
				error(1, 0, catgets(catfd, SET, 13538,
				    "-I (include file) entry "
				    "<%s> invalid"),
				    optarg);
			}
			break;

		case 'X':		/* excluded directory list */
			if (nexcluded > CSD_MAX_EXCLUDED) {
				error(1, 0, catgets(catfd, SET, 13520,
				    "Maximum -X (excluded "
				    "directory) entries "
				    "exceeded, max. is %d."),
				    CSD_MAX_EXCLUDED);
			}
			excluded[nexcluded] = optarg;
			len = strlen(optarg);
			if (len > 0) {
				*(excluded[nexcluded] + len) = '\0';
				nexcluded++;
			} else {
				error(1, 0, catgets(catfd, SET, 13521,
				    "-X (excluded directory) entry <%s> "
				    "invalid"),
				    optarg);
			}
			break;

		case 'f':		/* specify dump file */
			dump_file = optarg;
			break;

		case 'g':		/* specify log file */
			logfile = optarg;
			break;

		case 'H':		/* no headers */
			noheaders = true;
			break;

		case 'i':		/* print inode numbers */
			ls_options |= LS_INODES;
			break;

		case 'l':		/* print 1 line ls */
			verbose = true;
			ls_options |= LS_LINE1;
			break;

		case 'n':		/* Deprecated. Do not break script. */
			break;

		case 'P':		/* dump partial online data */
			partial_data = true;
			break;

		case 'q':		/* Do not report damage warnings */
			quiet = true;
			break;

		case 'r':		/* replace file if dump is newer */
			replace_newer = true;
			break;

		case 'R':		/* replace file */
			replace = true;
			break;

		case 's':		/* strip leading /'s */
			strip_slashes = true;
			break;

		case 'S':		/* scan only */
			scan_only = true;
			break;

		case 't':		/* list dump file */
			if (operation & DUMP) {
				error(1, 0,
				    catgets(catfd, SET, 941,
				    "Do not specify 't' option."));
			}
			/* samfsrestore t is LIST, not RESTORE */
			operation = LIST;
			break;

		case 'T':		/* statistics mode */
			statistics = true;
			break;

		case 'u':		/* dump unarchived data */
			unarchived_data = true;
			break;

		case 'U':		/* dump online data */
			online_data = true;
			break;

		case 'v':		/* verbose mode */
			verbose = true;
			break;

		case 'W':
			/* warning mode, allowed for historical reasons */
			break;

		case '2':		/* print 2 line ls */
			verbose = true;
			ls_options |= LS_LINE2;
			break;

		case 'Y':		/* file list for dump */
			use_file_list = true;
			break;

		case 'Z':		/* generate database load file	*/
			load_file = optarg;
			break;

		case '?':
			usage();	/* doesn't return */
			break;
		}
	}

	if (scan_only) {		/* clear dump related flags */
		online_data = partial_data = unarchived_data = false;
	}

	if (block_size && (block_size > read_buffer_size)) {
		block_size = read_buffer_size;
	}

	if (operation == DUMP && use_file_list) {
		operation = LISTDUMP;
	}

	if (debugging) {
		int i;

		fprintf(stderr, "\nCOMMAND ARGUMENT VALUES\n");
		fprintf(stderr, "operation = 0x%x\n", operation);
		fprintf(stderr, "debugging = %d\n", debugging);
		fprintf(stderr, "dump_file = %s\n",
		    ((dump_file != (char *)NULL) ? dump_file : "UNDEFINED"));
		fprintf(stderr, "logfile = %s\n",
		    ((logfile != (char *)NULL) ? logfile : "UNDEFINED"));
		fprintf(stderr, "noheaders = %d\n", noheaders);
		fprintf(stderr, "verbose = %d\n", verbose);
		fprintf(stderr, "quiet = %d\n", quiet);
		fprintf(stderr, "qfs = %d\n", qfs);
		fprintf(stderr, "replace = %d\n", replace);
		fprintf(stderr, "replace_newer = %d\n", replace_newer);
		fprintf(stderr, "strip_slashes = %d\n", strip_slashes);
		fprintf(stderr, "ls_options = 0x%x\n", ls_options);
		fprintf(stderr, "load_file = %s\n",
		    ((load_file != (char *)NULL) ? load_file : "UNDEFINED"));
		fprintf(stderr, "online_data = %d\n", online_data);
		fprintf(stderr, "partial_data = %d\n", partial_data);
		fprintf(stderr, "scan_only = %d\n", scan_only);
		fprintf(stderr, "unarchived_data = %d\n", unarchived_data);
		fprintf(stderr, "use_file_list = %d\n", use_file_list);
		fprintf(stderr, "read buffer size = %ld\n", read_buffer_size);
		fprintf(stderr, "write buffer size = %ld\n",
		    write_buffer_size);
		fprintf(stderr, "blocking factor = %ld\n", block_size);

		for (i = optind; i < argc; i++) {
			fprintf(stderr, "File argument %d = %s\n",
			    (i - optind + 1), argv[i]);
		}

		fprintf(stderr, "END COMMAND ARGUMENT VALUES\n\n");
	}

	/*
	 *	If dump file has not been specified, exit with error and usage
	 */

	if ((!scan_only) && dump_file == (char *)NULL) {
		error(0, 0,
		    catgets(catfd, SET, 5019,
		    "%s: Dump file name not specified (-f)"),
		    program_name);
		usage();
	}

	/*
	 *	Access master shared memory segment
	 */

	if ((master_shm.shared_memory = (shm_ptr_tbl_t *)
	    sam_mastershm_attach(0, SHM_RDONLY)) == (void *)-1) {
		/*
		 * If shared memory not available, effective ID must be
		 * super-user.
		 */

		if (geteuid() != 0) {
			error(1, 0,
			    catgets(catfd, SET, 100,
			    "%s may be run only by super-user.\n"),
			    program_name);
		}
	} else {
		/*
		 * If operator is not super-user, issue error message and exit
		 */

		SET_SAM_OPER_LEVEL(operator);

		shmdt((char *)master_shm.shared_memory);

		if (!SAM_ROOT_LEVEL(operator)) {
			error(1, 0,
			    catgets(catfd, SET, 100,
			    "%s may be run only by super-user.\n"),
			    program_name);
		}
	}

	/*
	 *	Open dump file and process header
	 */
	switch (operation) {
	case NONE:
		error(1, 0,
		    catgets(catfd, SET, 1845,
		    "One of c, r, O or t options must be specified."));
		usage();  /* doesn't return */
		break;

	case DUMP:
	case LISTDUMP:
		if (!scan_only) {
			if (strcmp(dump_file, "-") == 0) {
				CSD_fd = dup(STDOUT);
			} else {
				CSD_fd = open(dump_file,
				    O_WRONLY|O_TRUNC|O_CREAT|SAM_O_LARGEFILE,
				    0666 & ~umask(0));
			}

			close(STDOUT);

			if (CSD_fd < 0) {
				error(1, errno,
				    catgets(catfd, SET, 216,
				    "%s: Cannot create dump file"),
				    dump_file);
			}
		}

		csd_header.csd_header.time = time(0l);

		if (verbose) {
			fprintf(stderr, catgets(catfd, SET, 972,
			    "Dump created:%s"),
			    ctime((const time_t *)&csd_header.csd_header.time));
		}

		if (noheaders || scan_only) {
			break;
		}

		csd_header.csd_header.magic = csd_header.csd_header_magic =
		    CSD_MAGIC;

		csd_header.csd_header.version = CSD_VERS_6;
		csd_version = csd_header.csd_header.version;
		if (csd_write_csdheader(CSD_fd, &csd_header) < 0) {
			error(1, errno,
			    catgets(catfd, SET, 245,
			    "%s: Header record write error"),
			    dump_file);
		}
		break;

	case RESTORE:
	case LIST:
		if (0 == strcmp(dump_file, "-")) {
			CSD_fd = dup(STDIN);
		} else {
			CSD_fd = open(dump_file, O_RDONLY | SAM_O_LARGEFILE);
		}

		close(STDIN);

		if (CSD_fd < 0) {
			error(1, errno,
			    catgets(catfd, SET, 225,
			    "%s: Cannot open dump file"),
			    dump_file);
		}
		if (noheaders) {
			break;
		}
		if (buffered_read(CSD_fd, &csd_header, sizeof (csd_hdr_t)) !=
		    sizeof (csd_hdr_t)) {
			error(1, errno,
			    catgets(catfd, SET, 244,
			    "%s: Header record read error"),
			    dump_file);
		}
		if (csd_header.csd_header.magic != CSD_MAGIC) {
			if (csd_header.csd_header.magic != CSD_MAGIC_RE) {
				error(1, 0,
				    catgets(catfd, SET, 13529,
				    "%s: Volume is not in dump format."),
				    dump_file);
			} else {
				error(0, 0,
				    "Dump file was generated on an "
				    "opposite endian machine");
				swapped = true;
				sam_byte_swap(csd_header_swap_descriptor,
				    &csd_header, sizeof (csd_hdr_t));
			}
		}
		csd_version = csd_header.csd_header.version;
		switch (csd_version) {

		case CSD_VERS_6:
		case CSD_VERS_5:
			/* read remainder of extended header */
			io_sz = sizeof (csd_hdrx_t) - sizeof (csd_hdr_t);
			read_size = buffered_read(CSD_fd,
			    (char *)((char *)(&csd_header) +
			    sizeof (csd_hdr_t)), io_sz);
			if (swapped) {
				sam_bswap4(&csd_header.csd_header_flags, 1);
				sam_bswap4(&csd_header.csd_header_magic, 1);
			}
			if ((read_size != io_sz) ||
			    csd_header.csd_header_magic != CSD_MAGIC) {
				error(1, errno,
				    catgets(catfd, SET, 244,
				    "%s: Header record read error"),
				    dump_file);
			}
			break;

		case CSD_VERS_4:
		case CSD_VERS_3:
		case CSD_VERS_2:
			break;

		default:
			error(1, 0,
			    catgets(catfd, SET, 5013,
			    "%s: Header record error: Bad version number (%d)"),
			    dump_file, csd_version);
		}

		if (verbose) {
			printf(catgets(catfd, SET, 972, "Dump created:%s"),
			    ctime((const time_t *)&csd_header.csd_header.time));
		}
		if (debugging) {
			fprintf(stderr, "Dump created:%s\n",
			    ctime((const time_t *)&csd_header.csd_header.time));
		}
	}

	if ((logfile != NULL) && (log_st = fopen(logfile, "w")) == NULL) {
		error(0, errno, "%s", logfile);
		error(0, 0, catgets(catfd, SET, 1856,
		    "Open failed on (%s)"), logfile);
	}

	if ((load_file != NULL)) {
		if (0 == strcmp(load_file, "-")) {
			DB_FILE = fdopen(dup(STDOUT), "w");
			close(STDOUT);
		} else if ((DB_FILE = fopen64(load_file, "w")) == NULL) {
			error(0, errno, "%s", load_file);
			error(1, 0, catgets(catfd, SET, 1856,
			    "Open failed on (%s)"), load_file);
		}
	}

	switch (operation) {
	case NONE:
		break;

	case DUMP: {
		char *filename;

		if (debugging) {
			fprintf(stderr, "dumping.  argc %d, initial "
			    "path '%s' \n",
			    optind, Initial_path);
		}
		while ((filename = get_path(argc, argv)) != NULL) {
			if (debugging) {
				fprintf(stderr, "dumping path '%s' \n",
				    filename);
			}
			/*
			 * Try to make sure we don't dump from a non-SAM-FS
			 * filesystem by calling sam_stat() and checking the
			 * attributes.
			 */
			if (sam_stat(filename, &sb, sizeof (sb)) < 0 ||
			    ! SS_ISSAMFS(sb.attr)) {
				BUMP_STAT(errors);
				BUMP_STAT(errors_dir);
				error(1, errno,
				    catgets(catfd, SET, 259,
				    "%s: Not a SAM-FS file."),
				    filename);
			} else {
				if (SAM_fd != -1) {
					close(SAM_fd);
				}
				SAM_fd = open_samfs(filename);
				csd_dump_path("initial", filename,
				    (mode_t)sb.st_mode);
				if (S_ISDIR(sb.st_mode)) {
					process_saved_dir_list(filename);
				}
			}

			if (chdir(Initial_path) < 0) {
				error(1, errno,
				    catgets(catfd, SET, 212,
				    "%s: cannot chdir()"),
				    Initial_path);
			}

		}
		}
		bflush(CSD_fd);
		break;

	case LISTDUMP: {
		char *filename;

		if (debugging) {
			fprintf(stderr, "dumping.  argc %d, initial "
			    "path '%s' \n",
			    optind, Initial_path);
		}

		while ((filename = get_path(argc, argv)) != NULL) {
			if (debugging) {
				fprintf(stderr, "dumping path '%s' \n",
				    filename);
			}
			if (strcmp(filename, "-") == 0) {
				DL_FILE = fdopen(dup(STDIN), "r");
				close(STDIN);
			} else if ((DL_FILE = fopen(filename, "r")) == NULL) {
				error(0, errno, "%s", load_file);
				error(1, 0, catgets(catfd, SET, 1856,
				    "Open failed on (%s)"), load_file);
			}

			if (SAM_fd != -1) {
				close(SAM_fd);
			}
			csd_dump_files(DL_FILE, filename);
			fclose(DL_FILE);
			DL_FILE = NULL;
		}
		}
		bflush(CSD_fd);
		break;

	case RESTORE:
		umask(0);
		cs_restore(strip_slashes, (argc - optind), &argv[optind]);
		break;

	case LIST:
		cs_list((argc - optind), &argv[optind]);
	}

	close(CSD_fd);
	if (statistics) {
		print_stats();
	}
	return (exit_status);
}


/*
 * ----- get_path(int argc, char *argv[])
 *
 * Get the next file or path to dump.
 */
static char *
get_path(
int argc,
char *argv[])
{
	char *path;
	typedef enum {GPINIT = 0, GPFILE, GPCMD, GPDONE} gpse;
	static gpse gp_state = GPINIT;
	static char line[MAXPATHLEN + 1];
	static FILE  *I_str = NULL;
	static int ni = 0;

	path = NULL;
	switch (gp_state) {
	case GPINIT:
		if ((optind >= argc) && nincluded == 0) {
			/* If no paths or file given, default to "." */
			path = ".";
			gp_state = GPDONE;
			break;
		}
		gp_state = GPFILE;
		/*FALLTHROUGH*/
	case GPFILE:
		while (nincluded > 0) {
			if (I_str == NULL) {
				if ((I_str = fopen(included[ni], "r")) ==
				    NULL) {
					error(1, errno,
					    catgets(catfd, SET, 613,
					    "Cannot open %s"),
					    included[ni]);
				}
			}
			path = fgets(line, sizeof (line), I_str);
			if (path != NULL) {
				int trim;

				path += strspn(path, "\t\n ");
				trim = strcspn(path, "\t\n");
				path[trim--] = '\0';
				while (trim > 0 && path[trim] == ' ') {
					path[trim--] = '\0';
				}
				return (path);
			} else {
				fclose(I_str);
				I_str = NULL;
				nincluded--;
				ni++;
			}
		}
		gp_state = GPCMD;
		/*FALLTHROUGH*/
	case GPCMD:
		if (optind >= argc) {
			path = NULL;
			gp_state = GPDONE;
		} else {
			path = argv[optind++];
		}
		break;
	case GPDONE:
		path = NULL;
		break;
	}
	return (path);
}


/*
 * ----- open-samfs(char *filename)
 *
 * Open the (possibly large) file and log it if we're doing logging.
 */
int
open_samfs(
char *filename)		/* file to open */
{
	int fd;

	if (debugging) {
		fprintf(stderr, "Opening %s for SAM_fs file\n", filename);
	}

	if ((fd = open(filename, O_RDONLY | SAM_O_LARGEFILE)) < 0) {
		error(1, errno,
		    catgets(catfd, SET, 613, "Cannot open %s"),
		    filename);
	}
	return (fd);
}


/*
 * ----- print_stats - print statistics
 *
 * Print statistics for T option.
 */
void
print_stats()
{
	fprintf(stderr, "\n%s statistics:\n", program_name);
	fprintf(stderr, "    Files:\t\t%d\n", csd_statistics.files);
	fprintf(stderr, "    Directories:\t%d\n", csd_statistics.dirs);
	fprintf(stderr, "    Symbolic links:\t%d\n", csd_statistics.links);
	fprintf(stderr, "    Resource files:\t%d\n", csd_statistics.resources);
	fprintf(stderr, "    Files as members of hard links :\t%d\n",
	    csd_statistics.hlink);
	fprintf(stderr, "    Files as first hard link :\t%d\n",
	    csd_statistics.hlink_first);
	fprintf(stderr, "    File segments:\t%d\n", csd_statistics.segments);
	fprintf(stderr, "    File archives:\t%d\n",
	    csd_statistics.file_archives);
	fprintf(stderr, "    Damaged files:\t%d\n",
	    csd_statistics.file_damaged);
	fprintf(stderr, "    Files with data:\t%d\n",
	    csd_statistics.data_files);
	fprintf(stderr, "    File  warnings:\t%d\n",
	    csd_statistics.file_warnings);
	fprintf(stderr, "    Errors:\t\t%d\n", csd_statistics.errors);
	fprintf(stderr, "    Unprocessed dirs:\t%d\n",
	    csd_statistics.errors_dir);
	fprintf(stderr, "    File data bytes:\t%lld\n",
	    csd_statistics.data_dumped);
}

/*
 * Print usage message and exit
 */
static void
usage()
{
	fprintf(stderr, "Usage: %s ", program_name);
	switch (operation) {
	case DUMP:
	case LISTDUMP:
		if (!qfs) {
			fprintf(stderr, "[-dHnPqSTuUvW] ");
		} else {
			fprintf(stderr, "[-dHqTvD] ");
		}
		fprintf(stderr,
		    "[-b size] [-B size] [-I include_dir] [-X excluded_dir] "
		    "[-Y list_file] [-Z samdb_load_file] "
		    "-f dump_file [file...]\n");
		break;
	case RESTORE:
		if (!qfs) {
			fprintf(stderr, "[-dilrRsStTv2] ");
		} else {
			fprintf(stderr, "[-dilrRstTv2D] ");
		}
		fprintf(stderr, "[-b size] [-B size] [-Z samdb_load_file] "
		    "-f dump_file [file...]\n");
		break;
	default:
		fprintf(stderr,
		    "Usage: %s -c|-r|-t|-O [-HilRsTvW2] [-f dump_file ] "
		    "[dir...]\n",
		    program_name);
		fprintf(stderr,
		    "%s: Obsolete or experimental invocation of csd utility\n",
		    program_name);
	}
	exit(1);
}
