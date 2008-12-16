/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any	*/
/*	actual or intended publication of such source code.	*/

/*	Portions Copyright (c) 1988, Sun Microsystems, Inc.	*/
/*	Portions Copyright (c) 2001, Sun Microsystems, Inc.	*/
/*	All Rights Reserved.					*/

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

#ident	"$Revision: 1.13 $"

/*
 * chaid [-f] [-h] [-R] gid file ...
 */

/*
 * Code directly ripped off from Solaris chgrp(1).
 * Any changes to that should likely be reflected back here.
 *		chown() -> sam_chaid()
 *		lchown() -> sam_lchaid()
 *
 * Changes have been kept to a minimum; 'gid' remains gid
 * throughout instead of 'aid'.  The logic to disable setgid
 * bits on files has been #ifdef'd out.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <grp.h>
#include <dirent.h>
#include <unistd.h>
#include <libgen.h>

#include <sam/lib.h>
#include <sam/custmsg.h>
#include <sam/quota.h>
#include <sam/nl_samfs.h>

struct	group	*gr;
struct	stat64	stbuf;
struct	stat64	stbuf2;
gid_t	gid;
int	hflag = 0;
int	fflag = 0;
int	rflag = 0;
int	status;
int	acode = 0;
extern  int optind;

void usage(void);
int Perror(char *s);
int isnum(char *s);
int chaidr(char *dir, gid_t gid);


int
main(int argc, char *argv[])
{
	register int c;

	/* sam style localization */
	CustmsgInit(0, NULL);
	program_name = basename(argv[0]);

	while ((c = getopt(argc, argv, "hRf")) != EOF) {
		switch (c) {

			/* recurse directories */
			case 'R':
				rflag++;
				break;

			/* chaid links, not targets */
			case 'h':
				hflag++;
				break;

			/* force; don't complain on error */
			case 'f':
				fflag++;
				break;
			default:
				usage();
		}
	}

	/*
	 * Check for sufficient arguments
	 * or a usage error.
	 */
	argc -= optind;
	argv = &argv[optind];
	if (argc < 2) {
		usage();
	}

	if (isnum(argv[0])) {
		gid = (gid_t)atoi(argv[0]); /* gid is an int */
	} else {
		/*
		 * Don't handle this yet.
		 * We should map these things through /etc/opt/SUNWsamfs/aid
		 * ...
		 * cardiac:22
		 * pulmonary:23
		 * ...
		 *
		 * We should maybe rip-off the code
		 * for getgrnam/getgrent for this.
		 */
		fprintf(stderr,
		    catgets(catfd, SET, 13261,
		    "%s: unknown admin set ID: %s\n"),
		    program_name, argv[0]);
		exit(2);

#if _XX_AID_MAP_FILE
		if ((gr = getgrnam(argv[0])) == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 13261,
			    "%s: unknown admin set ID: %s\n"),
			    program_name, argv[0]);
			exit(2);
		}
		gid = gr->gr_gid;
#endif
	}

	for (c = 1; c < argc; c++) {
		if (lstat64(argv[c], &stbuf) < 0) {
			status += Perror(argv[c]);
			continue;
		}
		if (rflag & ((stbuf.st_mode & S_IFMT) == S_IFLNK)) {
			if (!hflag) {
				if (stat64(argv[c], &stbuf2) < 0) {
					status += Perror(argv[c]);
					continue;
				}
				if (sam_chaid(argv[c], gid) < 0) {
					status = Perror(argv[c]);
				}
#if _XX_SETAID_BITS
				/*
				 * If the object is a directory reset the
				 * SUID bits.
				 */
				if ((stbuf2.st_mode & S_IFMT) == S_IFDIR) {
					if (chmod(argv[c],
					    stbuf2.st_mode & ~S_IFMT) < 0) {
						status += Perror(argv[c]);
					}
				}
#endif
			} else {
				if (sam_lchaid(argv[c], gid) < 0) {
					status = Perror(argv[c]);
				}
			}
		} else if (rflag && ((stbuf.st_mode&S_IFMT) == S_IFDIR)) {
			status += chaidr(argv[c], gid);
		} else if (hflag) {
			if (sam_lchaid(argv[c], gid) < 0) {
				status = Perror(argv[c]);
			}
		} else {
			if (sam_chaid(argv[c], gid) < 0) {
				status = Perror(argv[c]);
			}
		}
#if _XX_SETAID_BITS
		/*
		 * If the object is a directory reset the SUID bits
		 */
		if ((stbuf.st_mode & S_IFMT) == S_IFDIR) {
			if (chmod(argv[c], stbuf.st_mode & ~S_IFMT) < 0) {
				status = Perror(argv[c]);
			}
		}
#endif
	}
	return (status += acode);
}


/*
 * Walk a directory tree
 */
int
chaidr(char *dir, gid_t gid)
{
	register struct dirent *dp;
	register DIR *dirp;
	struct stat64 st, st2;
	char savedir[1024];
	int ecode = 0;

	if (getcwd(savedir, 1024) == 0) {
		fprintf(stderr, catgets(catfd, SET, 13262,
		    "%s: Can't get current directory %s\n"),
		    program_name, savedir);
		exit(255);
	}

	/*
	 * Change what we are given before doing its contents.
	 */
	if (hflag) {
		if (sam_lchaid(dir, gid) < 0 && Perror(dir)) {
			return (1);
		}
	} else {
		if (stat64(dir, &st2) < 0) {
			status += Perror(dir);
		}
		if (sam_chaid(dir, gid) < 0 && Perror(dir)) {
			return (1);
		}
#if _XX_SETAID_BITS
		/*
		 * If the object is a directory reset the
		 * SUID bits.
		 */
		if ((st2.st_mode & S_IFMT) == S_IFDIR) {
			if (chmod(dir, st2.st_mode & ~S_IFMT) < 0) {
				status += Perror(dir);
			}
		}
#endif
	}

	if (chdir(dir) < 0) {
		return (Perror(dir));
	}
	if ((dirp = opendir(".")) == NULL) {
		return (Perror(dir));
	}
	dp = readdir(dirp);
	dp = readdir(dirp); /* read "." and ".." */
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if (lstat64(dp->d_name, &st) < 0) {
			ecode += Perror(dp->d_name);
			continue;
		}
		if (rflag & ((st.st_mode & S_IFMT) == S_IFLNK)) {
			if (!hflag) {
				if (stat64(dp->d_name, &st2) < 0) {
					status += Perror(dp->d_name);
					continue;
				}

				if (sam_chaid(dp->d_name, gid) < 0) {
					acode = Perror(dp->d_name);
				}
#if _XX_SETAID_BITS
				/*
				 * If the object is a directory reset the
				 * SUID bits.
				 */
				if ((st2.st_mode & S_IFMT) == S_IFDIR) {
					if (chmod(dp->d_name,
					    st2.st_mode & ~S_IFMT) < 0) {
						status += Perror(dp->d_name);
					}
				}
#endif
			} else {
				if (sam_lchaid(dp->d_name, gid) < 0) {
					acode = Perror(dp->d_name);
				}
			}
		} else if (rflag && ((st.st_mode&S_IFMT) == S_IFDIR)) {
			acode += chaidr(dp->d_name, gid);
		} else if (hflag) {
			if (sam_lchaid(dp->d_name, gid) < 0) {
				acode = Perror(dp->d_name);
			}
		} else {
			if (sam_chaid(dp->d_name, gid) < 0) {
				acode = Perror(dp->d_name);
			}
		}
#if _XX_SETAID_BITS
		/*
		 * If the object is a directory reset the SUID bits
		 */
		if ((st.st_mode & S_IFMT) == S_IFDIR) {
			if (chmod(dp->d_name, st.st_mode & ~S_IFMT) < 0) {
				status = Perror(dp->d_name);
			}
		}
#endif
	}
	closedir(dirp);
	if (chdir(savedir) < 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 13263,
		    "%s: Can't chdir back to '%s'\n"),
		    program_name, savedir);
		exit(255);
	}
	return (ecode);
}


int
isnum(char *s)
{
	register int c;

	while ((c = *s++) != 0) {
		if (!isdigit(c)) {
			return (0);
		}
	}
	return (1);
}


int
Perror(char *s)
{
	if (!fflag) {
		fprintf(stderr,  "%s: ", program_name);
		perror(s);
	}
	return (!fflag);
}


void
usage(void)
{
	fprintf(stderr,
	    catgets(catfd, SET, 13260, "Usage: %s [-fhR] admid file ...\n"),
	    program_name);
	exit(2);
}
