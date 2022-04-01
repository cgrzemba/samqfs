/*
 *	---- request - Create a removable-media file.
 *
 *	Creates the removable-media file enabling access to the specified media.
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

#pragma ident "$Revision: 1.23 $"

/* Feature test switches. */
/* PRrminfo	If defined, print sam_rminfo to stdout */

#define	MAIN

/* ANSI C headers. */
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>

/* OS headers. */
#include <libgen.h>
#include <zone.h>

/* SAM-FS headers. */
#define DEC_INIT
#include <sam/types.h>
#include <sam/lib.h>
#include <sam/custmsg.h>
#include <pub/rminfo.h>
#include <pub/stat.h>
#include <sam/nl_samfs.h>
#include <pub/devstat.h>

/* Private data. */
struct sam_rminfo rb;		/* Removable media information */
struct sam_rminfo *rp;		/* Removable media information */
char *vsnp[MAX_VOLUMES];
u_longlong_t position[MAX_VOLUMES];

int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	extern int optind;
	struct sam_stat sb;
	char c;
	char *fname;
	int errors = 0;
	int media;
	int optical_opts = 0;
	int media_set = 0;
	int tape_opts = 0;
	char *argvsn;
	int n_pos = 0;
	int pos_set = 0;
	char *argpos;
	char *posp;

	program_name = basename(argv[0]);
	/*
	 * Process arguments.
	 */
	CustmsgInit(0, NULL);
	while ((c = getopt(argc, argv, "m:v:l:bdp:s:f:n:o:g:i:N")) != EOF) {
		switch (c) {

		case 'm': {
			strcpy(rb.media, optarg);
			if ((media = sam_atomedia(rb.media)) == 0) {
				error(2, 0, catgets(catfd, SET, 1416,
				    "Invalid media"));
			}
			media_set = 1;
			}
			break;

		case 'v': {
			argvsn = optarg;
			if (rb.n_vsns) {
				error(2, 0, catgets(catfd, SET, 13210,
				    "%s and %s are mutually exclusive."),
				    "v", "l");
			}
			while ((vsnp[rb.n_vsns] = strtok(argvsn, "/")) !=
			    NULL) {
				if (strlen(vsnp[rb.n_vsns]) >
				    sizeof (vsn_t)-1) {
					error(2, 0, catgets(catfd, SET, 2879,
					    "vsn longer than %d characters"),
					    sizeof (vsn_t)-1);
				}
				if (++rb.n_vsns > MAX_VOLUMES) {
					error(2, 0, catgets(catfd, SET, 1832,
					    "number of vsns greater than %d"),
					    MAX_VOLUMES);
				}
				argvsn = NULL;
			}
			}
			break;

		case 'l': {
			fname =  optarg;
			if (rb.n_vsns) {
				error(2, 0, catgets(catfd, SET, 13210,
				    "%s and %s are mutually exclusive."),
				    "l", "v");
			} else if (pos_set) {
				error(2, 0, catgets(catfd, SET, 13210,
				    "%s and %s are mutually exclusive."),
				    "l", "p");
			} else {
				FILE *vsnfd;

				if ((vsnfd = fopen(fname, "r")) == NULL) {
					error(1, errno,
					    catgets(catfd, SET, 20010,
					    "unable to open %s"), fname);
				}
				for (;;) {
					char c[80];
					char *v;

					/*
					 * if EOF
					 */
					if (fgets(c, 80, vsnfd) == NULL) {
						break;
					}
					if ((v = strtok(c, " \n\t")) != NULL) {
						vsnp[rb.n_vsns] = strdup(v);
						position[rb.n_vsns] = 0;
						if ((v =
						    strtok(NULL, " \n\t")) !=
						    NULL) {
							position[rb.n_vsns] =
							    strtoll(v, NULL, 0);
							pos_set = 1;
						}
						if (++rb.n_vsns > MAX_VOLUMES) {
	/* N.B. Bad indentation here to meet cstyle requirements */
	error(2, 0, catgets(catfd, SET, 1832,
	"number of vsns greater than %d"), MAX_VOLUMES);
						}
					}
				}
				if (pos_set) {
					n_pos = rb.n_vsns;
					rb.position = position[0];
				}
			}
			}
			break;

		case 'b':
			if (rb.flags == RI_blockio) {
				error(0, 0, catgets(catfd, SET, 2975,
				    "b and d options cannot be used together"));
				errors++;
			}
			rb.flags = RI_bufio;
			break;

		case 'd':
			if (rb.flags == RI_bufio) {
				error(0, 0, catgets(catfd, SET, 2975,
				    "b and d options cannot be used together"));
				errors++;
			}
			rb.flags = RI_blockio;
			break;

		case 'p': {
			char *p;

			if (n_pos) {
				error(2, 0, catgets(catfd, SET, 13210,
				    "%s and %s are mutually exclusive."),
				    "p", "l");
			}
			if (getzoneid() != GLOBAL_ZONEID) {
				error(0, 0, catgets(catfd, SET, 5039,
				    "cannot specify -p in"
				    " the non-global zone"));
				errors++;
			}
			if (getuid() != 0) {
				error(0, 0, catgets(catfd, SET, 2975,
				    "You must be root to set the position"));
				errors++;
			} else {
				argpos = optarg;
				while ((posp = strtok(argpos, "/")) != NULL) {
					position[n_pos] = strtoll(posp, &p, 0);
					if (*p != '\0') {
						error(0, 0, catgets(catfd,
						    SET, 1043,
						    "error in position"));
						errors++;
					}
					if (++n_pos > MAX_VOLUMES) {
						error(2, 0, catgets(catfd,
						    SET, 3042,
						    "number of positions"
						    " greater than %d"),
						    MAX_VOLUMES);
					}
					argpos = NULL;
				}
				rb.position = position[0];
				pos_set = TRUE;
			}
			}
			break;

		case 's': {
			char *p;

			rb.required_size = strtoll(optarg, &p, 0);
			if (*p != '\0') {
				error(0, 0, catgets(catfd, SET, 1044,
				    "error in required size"));
				errors++;
			}
			}
			break;

		case 'f':
			if (strlen(optarg) > sizeof (rb.file_id)-1) {
				error(0, 0, catgets(catfd, SET, 1144,
				    "file identifier longer than %d"
				    " characters"),
				    sizeof (rb.file_id)-1);
				errors++;
			} else {
				strcpy(rb.file_id, optarg);
			}
			optical_opts++;
			break;

		case 'n': {
			char *p;

			rb.version = strtol(optarg, &p, 0);
			if (*p != '\0') {
				error(0, 0, catgets(catfd, SET, 1047,
				    "error in version"));
				errors++;
			}
			}
			break;

		case 'o':
			if (strlen(optarg) > sizeof (rb.owner_id)-1) {
				error(0, 0, catgets(catfd, SET, 1898,
				    "owner identifier longer than %d"
				    " characters"),
				    sizeof (rb.owner_id)-1);
				errors++;
			} else {
				strcpy(rb.owner_id, optarg);
			}
			optical_opts++;
			break;

		case 'g':
			if (strlen(optarg) > sizeof (rb.group_id)-1) {
				error(0, 0, catgets(catfd, SET, 1270,
				    "group identifier longer than %d"
				    " characters"),
				    sizeof (rb.group_id)-1);
				errors++;
			} else {
				strcpy(rb.group_id, optarg);
			}
			optical_opts++;
			break;

		case 'i':
			if (strlen(optarg) > sizeof (rb.info)-1) {
				error(0, 0, catgets(catfd, SET, 2834,
				    "user info longer than %d characters"),
				    sizeof (rb.info)-1);
				errors++;
			} else {
				strcpy(rb.info, optarg);
			}
			optical_opts++;
			break;

		case 'N':
			rb.flags |= RI_foreign;
			tape_opts++;
			break;

		case '?':
		default:
			errors++;
		}
	}

	if (rb.flags == 0) {
		rb.flags = RI_blockio;   /* Block IO is the default */
	}

	/*
	 * Set media.
	 */
	if (!media_set) {
		if (optind >= argc) {
			exit(2);
		}
		if (strlen(argv[optind]) != 2) {
			error(2, 0, catgets(catfd, SET, 1416, "Invalid media"));
		}
		strcpy(rb.media, argv[optind++]);
		if ((media = sam_atomedia(rb.media)) == 0) {
			error(2, 0, catgets(catfd, SET, 1416, "Invalid media"));
		}
	}

	/*
	 * Set vsns.
	 */
	if (!rb.n_vsns) {
		if (optind >= argc) {
			exit(2);
		}
		argvsn = argv[optind];
		while ((vsnp[rb.n_vsns] = strtok(argvsn, "/")) != NULL) {
			if (strlen(vsnp[rb.n_vsns]) > sizeof (vsn_t)-1) {
				error(2, 0, catgets(catfd, SET, 2879,
				    "vsn longer than %d characters"),
				    sizeof (vsn_t)-1);
			}
			if (++rb.n_vsns > MAX_VOLUMES) {
				error(2, 0, catgets(catfd, SET, 1832,
				    "number of vsns greater than %d"),
				    MAX_VOLUMES);
			}
			argvsn = NULL;
		}
		optind++;
	}
	if (pos_set && (rb.n_vsns != n_pos)) {
		error(2, 0, catgets(catfd, SET, 3043,
		    "number of vsns %d does not match number of position %d"),
		    rb.n_vsns, n_pos);
	}

	/*
	 * Get sam_rminfo with length enough for n_vsns.
	 */
	rp = &rb;
	if (rb.n_vsns > 0) {
		int v;
		rp = (struct sam_rminfo *)malloc(SAM_RMINFO_SIZE(rb.n_vsns));
		memset(rp, 0, SAM_RMINFO_SIZE(rb.n_vsns));
		memcpy(rp, &rb, sizeof (struct sam_rminfo));
		for (v = 0; v < rb.n_vsns; v++) {
			strcpy(rp->section[v].vsn, vsnp[v]);
			if (pos_set) {
				rp->section[v].position = position[v];
			}
		}
	}

	fname = argv[optind];
	if (errors || (argc != (optind + 1))) {
		fprintf(stderr,
"usage:\ntape\n"
"  %s -m media [-v vsn1[/vsn2/...] [-p pos1[/pos2/...] | -l vsnfile]\n"
"        [-s n] [-N] file\n"
"optical\n"
"  %s -m media [-v vsn1[/vsn2/...] [-p pos1[/pos2/...] | -l vsnfile]\n"
"        [-s n]\n"
"        [-f file_id] [-n version] [-o owner] [-g group] [-i info] file\n",
		    program_name, program_name);
		exit(2);
	}

	/*
	 * Set optical defaults.
	 */
	if ((media & DT_CLASS_MASK) == DT_OPTICAL) {

		if (tape_opts != 0) {
			error(2, 0, catgets(catfd, SET, 5023,
			    "tape options not valid for this media"));
		}

		if (*rb.file_id == '\0') {
			char *p;

			p = basename(fname);
			if (strlen(p) > sizeof (rb.file_id)-1) {
				error(2, 0, catgets(catfd, SET, 1150,
				    "file name > %d chars, use shorter"
				    " file name or -f option"),
				    sizeof (rb.file_id)-1);
			}
			strcpy(rb.file_id, p);
		}
		if (*rb.owner_id == '\0') {
			struct passwd *pw;

			pw = getpwuid(getuid());
			strncpy(rb.owner_id, pw->pw_name,
			    sizeof (rb.owner_id)-1);
		}
		if (*rb.group_id == '\0') {
			struct group *gr;

			gr = getgrgid(getgid());
			strncpy(rb.group_id, gr->gr_name,
			    sizeof (rb.owner_id)-1);
		}
	} else {
		char *p;
		int v;

		if (optical_opts != 0) {
			error(2, 0, catgets(catfd, SET, 1876,
			    "optical options not valid for this media"));
		}

		/*
		 * Validate tape vsn.
		 */
		for (v = 0; v < rb.n_vsns; v++) {
			if ((int)strlen(rp->section[v].vsn) > 6) {
				error(2, 0, catgets(catfd, SET, 2469,
				    "tape vsn longer than 6 characters"));
			}
			for (p = rp->section[v].vsn; *p != '\0'; p++) {
				if (isupper(*p) || isdigit(*p)) {
					continue;
				}
				if (strchr(" !\"%&'()*+,-./:;<=>?_", *p) !=
				    NULL) {
					continue;
				}
				error(2, 0,
				    catgets(catfd, SET, 1319,
				    "illegal character in tape vsn"));
			}
		}
	}

#if defined(PRrminfo)
/* BLOCK for cstyle */	{
		struct tm *tm;
		char timestr[32];
		int  n;

		printf("file: %s  media: %s  required_size: 0x%llx\n",
		    fname, rp->media, rp->required_size);
		printf("file_id: %s  version: %d\n",  rp->file_id, rp->version);
		printf("owner: %s  group: %s\n", rp->owner_id, rp->group_id);
		printf("info: %s\n", rp->info);
		printf("n_vsns: %d  c_vsn: %d\n", rp->n_vsns, rp->c_vsn);
		for (n = 0; n < rp->n_vsns; n++) {
			printf("    vsn[%d]:%s  position:0x%llx\n",
			    n, rp->section[n].vsn, rp->section[n].position);
		}
		tm = localtime((time_t *)&rp->creation_time);
		strftime(timestr, sizeof (timestr)-1, "%Y/%m/%d %H:%M:%S", tm);
		printf("time: %s\n", timestr);
	}
#endif

	if (sam_stat(fname, &sb, sizeof (sb)) < 0) {
		int fd;

		if ((fd = open(fname, O_CREAT, 0666)) < 0) {
			error(1, errno, catgets(catfd, SET, 574,
			    "cannot create %s"),
			    fname);
		}
		close(fd);
		if (sam_stat(fname, &sb, sizeof (sb)) < 0) {
			error(1, errno, catgets(catfd, SET, 633,
			    "cannot sam_stat %s"),
			    fname);
		}
	}
	if (!SS_ISSAMFS(sb.attr)) {
		error(1, 0,
		    catgets(catfd, SET, 1146,
		    "file is not on a SAM filesystem"));
	}
	if (sam_request(fname, rp, SAM_RMINFO_SIZE(rb.n_vsns)) < 0) {
		switch (errno) {
		case EINVAL:
			error(2, 0, catgets(catfd, SET, 1416, "Invalid media"));
			break; /* statement not reached */
		/*
		 * The request operation is not supported on clients of
		 * the mounted FS. We could get here as a client via a
		 * syscall or ioctl. In the event that we do then unlink
		 * the file and tell the user they can't do this.
		 */
		case ENOTSUP:
			(void) unlink(fname);
			error(2, 0, catgets(catfd, SET, 630,
			    "cannot request: operation not supported"
			    " on shared client"));
			break; /* statement not reached */
		}
		error(1, errno, catgets(catfd, SET, 629,
		    "cannot request %s"), fname);
	}

#if defined(PRrminfo)
	/* BLOCK for cstyle */	{
		struct tm *tm;
		char timestr[32];
		int  n;
		int  nvsns = rb.n_vsns;

		memset(rp, 0, SAM_RMINFO_SIZE(nvsns));
		if (sam_readrminfo(fname,  rp, SAM_RMINFO_SIZE(nvsns)) < 0) {
			error(1, 1, catgets(catfd, SET, 627,
			    "cannot read rminfo %s"),
			    fname);
		}
		printf(catgets(catfd, SET, 2125, "Returned --\n"));
		printf("file: %s  media: %s  required_size: 0x%llx\n",
		    fname, rp->media, rp->required_size);
		printf("position: 0x%llx  file_id: %s  version: %d\n",
		    rp->position, rp->file_id, rp->version);
		printf("owner: %s  group: %s\n", rp->owner_id, rp->group_id);
		printf("info: %s\n", rp->info);
		printf("n_vsns: %d  c_vsn: %d\n", rp->n_vsns, rp->c_vsn);
		for (n = 0; n < nvsns; n++) {
			printf("    vsn[%d]:%s  position:0x%llx\n",
			    n, rp->section[n].vsn, rp->section[n].position);
		}
		tm = localtime((time_t *)&rp->creation_time);
		strftime(timestr, sizeof (timestr)-1, "%Y/%m/%d %H:%M:%S", tm);
		printf("time: %s\n", timestr);
	}
#endif
	return (0);
}
