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

/*
 *	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T
 *	  All Rights Reserved
 */

/*
 * University Copyright- Copyright (c) 1982, 1986, 1988
 * The Regents of the University of California
 * All Rights Reserved
 *
 * University Acknowledgment- Portions of this document are derived from
 * software developed by the University of California, Berkeley, and its
 * contributors.
 */

#pragma ident	"$Revision: 1.1 $"

/*
 * schproj [-fhR] projectname|projectID file
 * schproj -R [-f] [-H|-L|-P] projectname|projectID file
 */

/*
 * This code is derived from OpenSolaris chgrp(1) version June 2008.
 * Any changes to that should likely be reflected back here.
 *              chown() -> sam_chprojid()
 *              lchown() -> sam_lchprojid()
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/avl.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>
#include <strings.h>
#include <pwd.h>
#include <project.h>

/* SAM-FS headers. */
#define DEC_INIT
#include <sam/lib.h>
#include <sam/custmsg.h>
#include <sam/quota.h>
#include <sam/nl_samfs.h>
#include <sam/syscall.h>

static struct stat	stbuf;
static struct stat	stbuf2;
static projid_t		projid;
static int		hflag = 0,
			fflag = 0,
			rflag = 0,
			Hflag = 0,
			Lflag = 0,
			Pflag = 0;
static int		status = 0;	/* total number of errors received */

static avl_tree_t	*tree;		/* search tree to store inode data */

static void usage(void);
static int isnumber(char *);
static int Perror(char *);
static void chprojidr(char *, projid_t, struct stat *);
static int check_access(const char *, int);


#ifdef XPG4
/*
 * Check to see if we are to follow symlinks specified on the command line.
 * This assumes we've already checked to make sure neither -h or -P was
 * specified, so we are just looking to see if -R -L, or -R -H was specified,
 * or, since -R has the same behavior as -R -L, if -R was specified by itself.
 * Therefore, all we really need to check for is if -R was specified.
 */
#define	FOLLOW_CL_LINKS	(rflag)
#else
/*
 * Check to see if we are to follow symlinks specified on the command line.
 * This assumes we've already checked to make sure neither -h or -P was
 * specified, so we are just looking to see if -R -L, or -R -H was specified.
 * Note: -R by itself will change the project ID of a directory referenced by a
 * symlink however it will not follow the symlink to any other part of the
 * file hierarchy.
 */
#define	FOLLOW_CL_LINKS	(rflag && (Hflag || Lflag))
#endif

#ifdef XPG4
/*
 * Follow symlinks when traversing directories.  Since -R behaves the
 * same as -R -L, we always want to follow symlinks to other parts
 * of the file hierarchy unless -H was specified.
 */
#define	FOLLOW_D_LINKS	(!Hflag)
#else
/*
 * Follow symlinks when traversing directories.  Only follow symlinks
 * to other parts of the file hierarchy if -L was specified.
 */
#define	FOLLOW_D_LINKS	(Lflag)
#endif

int sam_chprojid(const char *path, int projid, struct stat *stp);
int sam_lchprojid(const char *path, int projid, struct stat *stp);

#define	CHPROJID(f, p, s)	if (sam_chprojid(f, p, s) < 0) { \
				status += Perror(f); \
				}

#define	LCHPROJID(f, p, s)	if (sam_lchprojid(f, p, s) < 0) { \
				status += Perror(f); \
				}

extern int		optind;

int add_tnode(avl_tree_t **, dev_t, ino_t);

int
main(int argc, char *argv[])
{
	uid_t euid, ruid;
	struct passwd *pwent = NULL;
	char *un = NULL;
	char *pname = NULL;
	struct project proj;
	char *buf = NULL;
	int bufsize;
	int isinproj = 1;
	projid_t projid;
	int c;

	while ((c = getopt(argc, argv, "RhfHLP")) != EOF)
		switch (c) {
			case 'R':
				rflag++;
				break;
			case 'h':
				hflag++;
				break;
			case 'f':
				fflag++;
				break;
			case 'H':
				/*
				 * If more than one of -H, -L, and -P
				 * are specified, only the last option
				 * specified determines the behavior of
				 * schproj.  In addition, make [-H|-L]
				 * mutually exclusive of -h.
				 */
				Lflag = Pflag = 0;
				Hflag++;
				break;
			case 'L':
				Hflag = Pflag = 0;
				Lflag++;
				break;
				case 'P':
				Hflag = Lflag = 0;
				Pflag++;
				break;
			default:
				usage();
		}

	/*
	 * Check for sufficient arguments
	 * or a usage error.
	 */
	argc -= optind;
	argv = &argv[optind];

	if ((argc < 2) ||
	    ((Hflag || Lflag || Pflag) && !rflag) ||
	    ((Hflag || Lflag || Pflag) && hflag)) {
		usage();
	}

	/*
	 * Allocate buffer space needed by the
	 * project ID library calls.
	 */
	buf = calloc(4096, 1);
	bufsize = 4096;

	if (isnumber(argv[0])) {
		errno = 0;
		projid = (projid_t)atoi(argv[0]);
	} else {
		pname = strdup(argv[0]);
		/*
		 * Find the project ID from the supplied name.
		 */
		projid = getprojidbyname(pname);
		if (projid < 0) {
			fprintf(stderr,
			    "Can't get project id for project %s\n", pname);
			free(buf);
			exit(2);
		}
	}

	/*
	 * If needed find the project name from the project ID.
	 */
	if (pname == NULL) {
		if (getprojbyid((projid_t)projid, &proj, buf,
		    bufsize) == NULL) {
			fprintf(stderr,
			    "Can't get project name for project id %d\n",
			    projid);
			free(buf);
			exit(2);
		}
		pname = strdup(proj.pj_name);
	}

	/*
	 * Project membership is by username not userid so
	 * get the current thread username from the real userid.
	 */
	ruid = getuid();

	/*
	 *  If not root check that the user is in the project.
	 */
	if (ruid != 0) {
		pwent = getpwuid(ruid);
		if (pwent) {
			un = strdup(pwent->pw_name);
		} else {
			fprintf(stderr,  "Can't get username for uid %uid\n",
			    ruid);
			free(pname);
			free(buf);
			exit(2);
		}
		isinproj = inproj(un, pname, buf, bufsize);

		if (!isinproj) {
			fprintf(stderr, "%s not a member of %s\n", un, pname);
			free(pname);
			free(un);
			free(buf);
			exit(2);
		}
		free(pname);
		free(un);
		free(buf);
	}

	for (c = 1; c < argc; c++) {
		tree = NULL;
		if (lstat(argv[c], &stbuf) < 0) {
			status += Perror(argv[c]);
			continue;
		}
		if (rflag && ((stbuf.st_mode & S_IFMT) == S_IFLNK)) {
			if (hflag) {
				/*
				 * Change the project id of the symbolic link
				 * specified on the command line.
				 * Don't follow the symbolic link to
				 * any other part of the file hierarchy.
				 */
				LCHPROJID(argv[c], projid, &stbuf);

			} else {
				if (stat(argv[c], &stbuf2) < 0) {
					status += Perror(argv[c]);
					continue;
				}
				/*
				 * We know that we are to change the
				 * project ID of the file referenced by the
				 * symlink specified on the command line.
				 * Now check to see if we are to follow
				 * the symlink to any other part of the
				 * file hierarchy.
				 */
				if (FOLLOW_CL_LINKS) {
					if ((stbuf2.st_mode & S_IFMT)
					    == S_IFDIR) {
						/*
						 * We are following symlinks so
						 * traverse into the directory.
						 * Add this node to the search
						 * tree so we don't get into an
						 * endless loop.
						 */
						if (add_tnode(&tree,
						    stbuf2.st_dev,
						    stbuf2.st_ino) == 1) {
							chprojidr(argv[c],
							    projid, &stbuf2);
						} else {
							/*
							 * Error occurred.
							 * rc can't be 0
							 * as this is the first
							 * node to be added to
							 * the search tree.
							 */
							status += Perror(
							    argv[c]);
						}
					} else {
						/*
						 * Change the project ID of the
						 * file referenced by the
						 * symbolic link.
						 */
						CHPROJID(argv[c], projid,
						    &stbuf2);
					}
				} else {
					/*
					 * Change the project ID of the file
					 * referenced by the symbolic link.
					 */
					CHPROJID(argv[c], projid, &stbuf2);

				}
			}
		} else if (rflag && ((stbuf.st_mode & S_IFMT) == S_IFDIR)) {
			/*
			 * Add this node to the search tree so we don't
			 * get into a endless loop.
			 */
			if (add_tnode(&tree, stbuf.st_dev,
			    stbuf.st_ino) == 1) {
				chprojidr(argv[c], projid, &stbuf);

			} else {
				/*
				 * An error occurred while trying
				 * to add the node to the tree.
				 * Continue on with next file
				 * specified.  Note: rc shouldn't
				 * be 0 as this was the first node
				 * being added to the search tree.
				 */
				status += Perror(argv[c]);
			}
		} else {
			if (hflag || Pflag) {
				LCHPROJID(argv[c], projid, &stbuf);
			} else {
				CHPROJID(argv[c], projid, &stbuf);
			}
		}
	}
	return (status);
}

/*
 * chprojidr() - recursive change file project ID
 *
 * Recursively chproj's the input directory then its contents.  rflag must
 * have been set if chprojidr() is called.  The input directory should not
 * be a sym link (this is handled in the calling routine).  In
 * addition, the calling routine should have already added the input
 * directory to the search tree so we do not get into endless loops.
 * Note: chprojidr() doesn't need a return value as errors are reported
 * through the global "status" variable.
 */
static void
chprojidr(char *dir, projid_t projid, struct stat *stp)
{
	struct dirent *dp;
	DIR *dirp;
	struct stat st, st2;
	char savedir[1024];

	if (getcwd(savedir, 1024) == 0) {
		(void) fprintf(stderr, "schproj: ");
		(void) fprintf(stderr, gettext("%s\n"), savedir);
		exit(255);
	}

	/*
	 * Attempt to chproj the directory, however don't return if we
	 * can't as we still may be able to chproj the contents of the
	 * directory.
	 */
	CHPROJID(dir, projid, stp);

	if (chdir(dir) < 0) {
		status += Perror(dir);
		return;
	}
	if ((dirp = opendir(".")) == NULL) {
		status += Perror(dir);
		return;
	}
	for (dp = readdir(dirp); dp != NULL; dp = readdir(dirp)) {
		if ((strcmp(dp->d_name, ".") == 0) ||
		    (strcmp(dp->d_name, "..") == 0)) {
			continue;	/* skip "." and ".." */
		}
		if (lstat(dp->d_name, &st) < 0) {
			status += Perror(dp->d_name);
			continue;
		}
		if ((st.st_mode & S_IFMT) == S_IFLNK) {
			if (hflag || Pflag) {
				/*
				 * Change the project ID of the symbolic link
				 * encountered while traversing the
				 * directory.  Don't follow the symbolic
				 * link to any other part of the file
				 * hierarchy.
				 */
				LCHPROJID(dp->d_name, projid, &st);
			} else {
				if (stat(dp->d_name, &st2) < 0) {
					status += Perror(dp->d_name);
					continue;
				}
				/*
				 * We know that we are to change the
				 * project ID of the file referenced by the
				 * symlink encountered while traversing
				 * the directory.  Now check to see if we
				 * are to follow the symlink to any other
				 * part of the file hierarchy.
				 */
				if (FOLLOW_D_LINKS) {
					if ((st2.st_mode & S_IFMT) == S_IFDIR) {
						/*
						 * We are following symlinks so
						 * traverse into the directory.
						 * Add this node to the search
						 * tree so we don't get into an
						 * endless loop.
						 */
						int rc;
						if ((rc = add_tnode(&tree,
						    st2.st_dev,
						    st2.st_ino)) == 1) {
							chprojidr(dp->d_name,
							    projid, &st2);

						} else if (rc == 0) {
							/* already visited */
							continue;
						} else {
							/*
							 * An error occurred
							 * while trying to add
							 * the node to the tree.
							 */
							status += Perror(
							    dp->d_name);
							continue;
						}
					} else {
						/*
						 * Change the project ID of the
						 * file referenced by the
						 * symbolic link.
						 */
						CHPROJID(dp->d_name, projid,
						    &st2);

					}
				} else {
					/*
					 * Change the project ID of the file
					 * referenced by the symbolic link.
					 */
					CHPROJID(dp->d_name, projid, &st2);

				}
			}
		} else if ((st.st_mode & S_IFMT) == S_IFDIR) {
			/*
			 * Add this node to the search tree so we don't
			 * get into a endless loop.
			 */
			int rc;
			if ((rc = add_tnode(&tree, st.st_dev,
			    st.st_ino)) == 1) {
				chprojidr(dp->d_name, projid, &st);

			} else if (rc == 0) {
				/* already visited */
				continue;
			} else {
				/*
				 * An error occurred while trying
				 * to add the node to the search tree.
				 */
				status += Perror(dp->d_name);
				continue;
			}
		} else {
			CHPROJID(dp->d_name, projid, &st);
		}
	}
	(void) closedir(dirp);
	if (chdir(savedir) < 0) {
		(void) fprintf(stderr, "chproj: ");
		(void) fprintf(stderr, gettext("can't change back to %s\n"),
		    savedir);
		exit(255);
	}
}

static int
isnumber(char *s)
{
	int c;

	while ((c = *s++) != '\0')
		if (!isdigit(c))
			return (0);
	return (1);
}


static int
Perror(char *s)
{
	if (!fflag) {
		(void) fprintf(stderr, "chproj: ");
		perror(s);
	}
	return (!fflag);
}


static void
usage(void)
{
	(void) fprintf(stderr, "usage:\n"
	    "\tschproj [-fhR] projectname|projectID file\n"
	    "\tschproj -R [-f] [-H|-L|-P] projectname|projectID file\n");
	exit(2);
}


/*
 * Change file's project ID
 */
int
sam_chprojid(const char *path, int projid, struct stat *stp)
{
	struct sam_chaid_arg arg;
	uid_t uid = getuid();

	if ((uid != stp->st_uid) && (uid != 0)) {
		errno = EPERM;
		return (-1);
	}
	if ((errno = check_access(path, 1)) != 0) {
		return (-1);
	}

	arg.path.ptr = path;
	arg.admin_id = projid;
	arg.follow = 1;
	return (sam_syscall(SC_projid, &arg, sizeof (arg)));
}

/*
 * Same thing, but if the target is a symbolic link,
 * don't follow it.
 */
int
sam_lchprojid(const char *path, int projid, struct stat *stp)
{
	struct sam_chaid_arg arg;
	uid_t uid = getuid();

	if ((uid != stp->st_uid) && (uid != 0)) {
		errno = EPERM;
		return (-1);
	}
	if ((errno = check_access(path, 0)) != 0) {
		return (-1);
	}

	arg.path.ptr = path;
	arg.admin_id = projid;
	arg.follow = 0;
	return (sam_syscall(SC_projid, &arg, sizeof (arg)));
}


/*
 * Stub this until we can get connected
 * to the real one. For now we don't detect
 * directory recursion.
 */
int
add_tnode(avl_tree_t **t, dev_t d, ino_t i)
{
	return (1);
}


/*
 * Check access permission to the target path.  We are suid root
 * and need to make sure the path is reachable by the real user.
 */
static int			/* Return 0 if access is allowed, else errno */
check_access(
	const char *path,	/* Path to check */
	int lflag)		/* Follow symlink */
{
	struct stat stb;
	int r = 0;
	uid_t saved_euid;

	saved_euid = geteuid();
	if (seteuid(getuid()) < 0) {	/* become real user */
		r = errno;
		(void) fprintf(stderr, "schproj: seteuid failed\n");
		return (r);
	}
	if (((lflag != 0) && (stat(path, &stb) < 0)) ||
	    ((lflag == 0) && (lstat(path, &stb) < 0))) {
		r = EACCES;
	}
	if (seteuid(saved_euid) < 0) {	/* revert back to suid */
		r = errno;
		(void) fprintf(stderr, "schproj: seteuid back failed\n");
		return (r);
	}
	return (r);
}
