/*
 *
 *	umount.c
 *
 *	Unmount filesystem.  Called by umount program when a request
 * is made to unmount a SAM-QFS filesystem.
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
/*
 * Copyright 2005 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*	Copyright (c) 1984, 1986, 1987, 1988, 1989 AT&T	*/
/*	  All Rights Reserved	*/

/*
 * University Copyright- Copyright (c) 1982, 1986, 1988
 * The Regents of the University of California
 * All Rights Reserved
 *
 * University Acknowledgment- Portions of this document are derived from
 * software developed by the University of California, Berkeley, and its
 * contributors.
 */

#pragma ident	"$Revision: 1.9 $"

/*
 * SAM-QFS umount
 *
 * Mostly needed because default umount ignores signals for the umount2 call.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/mnttab.h>
#include <sys/mount.h>
#include <errno.h>
#include <sam/osversion.h>
#include <sam/shareops.h>
#include <priv.h>

#include <sam/custmsg.h>

#define	RET_OK	0
#define	RET_ERR	32

#ifdef __STDC__
static void pr_err(const char *fmt, ...);
#else
static void pr_err(char *fmt, va_dcl);
#endif
static	void	usage();
static	int	samfs_unmount(char *, int);
static	struct extmnttab *mnttab_find();
static struct extmnttab *fsdupmnttab(struct extmnttab *mnt);
static void fsfreemnttab(struct extmnttab *mnt);
static void setdefaults(void);

static char typename[64];

static int await_clients = 0;	/* wait for mounted clients (seconds) */

char* program_name = NULL;

int
main(int argc, char *argv[])
{
	extern int optind;
	int c;
	int umnt_flag = 0;
	int errflag = 0;
	char *myname;

	myname = strrchr(argv[0], '/');
	myname = myname ? myname+1 : argv[0];
	(void) sprintf(typename, "samfs %s", myname);
	argv[0] = typename;

	/*
	 * Set options
	 */
	setdefaults();

	while ((c = getopt(argc, argv, "fo:")) != EOF) {
		switch (c) {
		case 'f':
			umnt_flag |= MS_FORCE; /* forced unmount is desired */
			break;
		case 'o': {
			char *await_opt = "await_clients=";

			if (strncmp(optarg, await_opt,
			    strlen(await_opt)) == 0) {
				await_clients =
				    atoi(&optarg[strlen(await_opt)]);
			} else {
				errflag++;
			}
			break;
			}
		default:
			errflag++;
		}
	}
	if (argc - optind != 1) {
		errflag++;
	}

	if (errflag) {
		usage();
		exit(RET_ERR);
	}

	if (!priv_ineffect(PRIV_SYS_MOUNT)) {
		/* insufficient privilege */
		pr_err(GetCustMsg(17401));
		exit(RET_ERR);
	}

	/*
	 * exit, really
	 */
	return (samfs_unmount(argv[optind], umnt_flag));
}


static void
pr_err(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	(void) fprintf(stderr, "%s: ", typename);
	(void) vfprintf(stderr, fmt, ap);
	(void) fflush(stderr);
	va_end(ap);
}


/*
 * ---- setdefaults
 *
 * Initialize any default values set in the environment.
 */
static void
setdefaults(void)
{
	char *p = getenv("SAM_AWAIT_CLIENTS");

	if (p != NULL) {
		if (*p == '\0') {
			await_clients = atoi(p);
		} else {
			await_clients = 30;
		}
	}
}


static void
usage()
{
	pr_err(GetCustMsg(17400));
	exit(RET_ERR);
}


static int
samfs_unmount(char *pathname, int umnt_flag)
{
	struct extmnttab *mntp;
	char *special = "";

	mntp = mnttab_find(pathname);
	if (mntp) {
		pathname = mntp->mnt_mountp;
		special = mntp->mnt_special;
	}

	if (await_clients) {
		time_t fin = time(NULL) + await_clients;

		while (await_clients < 0 || time(NULL) < fin) {
			if (sam_shareops(special, SHARE_OP_CL_MOUNTED,
			    0) <= 0) {
				break;
			}
			sleep(1);
		}
	}

	if (umount2(pathname, umnt_flag) < 0) {
		switch (errno) {
		case EBUSY:
			if (sam_shareops(special, SHARE_OP_CL_MOUNTED,
			    0) > 0) {
				/* shared file system has client(s) mounted */
				pr_err(GetCustMsg(17405), pathname);
			} else {
				/* %s: is busy */
				pr_err(GetCustMsg(17403), pathname);
			}
			break;

		case ENOENT:
			/* %s: not mounted */
			pr_err(GetCustMsg(17404), pathname);
			break;

		default:
			/* %s: umount failed: %s */
			pr_err(GetCustMsg(17402), pathname, strerror(errno));
		}
		return (RET_ERR);
	}

	return (RET_OK);
}


/*
 * Free a single mnttab structure
 */
static void
fsfreemnttab(struct extmnttab *mnt)
{

	if (mnt) {
		if (mnt->mnt_special)
			free(mnt->mnt_special);
		if (mnt->mnt_mountp)
			free(mnt->mnt_mountp);
		if (mnt->mnt_fstype)
			free(mnt->mnt_fstype);
		if (mnt->mnt_mntopts)
			free(mnt->mnt_mntopts);
		if (mnt->mnt_time)
			free(mnt->mnt_time);
		free(mnt);
	}
}


static struct extmnttab zmnttab = { 0 };


static struct extmnttab *
fsdupmnttab(struct extmnttab *mnt)
{
	struct extmnttab *new;

	new = (struct extmnttab *)malloc(sizeof (*new));
	if (new == NULL)
		goto alloc_failed;

	*new = zmnttab;
	/*
	 * Allocate an extra byte for the mountpoint
	 * name in case a space needs to be added.
	 */
	new->mnt_mountp = (char *)malloc(strlen(mnt->mnt_mountp) + 2);
	if (new->mnt_mountp == NULL)
		goto alloc_failed;
	(void) strcpy(new->mnt_mountp, mnt->mnt_mountp);

	if ((new->mnt_special = strdup(mnt->mnt_special)) == NULL)
		goto alloc_failed;

	if ((new->mnt_fstype = strdup(mnt->mnt_fstype)) == NULL)
		goto alloc_failed;

	if (mnt->mnt_mntopts != NULL)
		if ((new->mnt_mntopts = strdup(mnt->mnt_mntopts)) == NULL)
			goto alloc_failed;

	if (mnt->mnt_time != NULL)
		if ((new->mnt_time = strdup(mnt->mnt_time)) == NULL)
			goto alloc_failed;

	new->mnt_major = mnt->mnt_major;
	new->mnt_minor = mnt->mnt_minor;
	return (new);

alloc_failed:
	/* out of memory */
	pr_err(GetCustMsg(17420));
	fsfreemnttab(new);
	return (NULL);
}


/*
 *  Find the mnttab entry that corresponds to "name".
 *  We're not sure what the name represents: either
 *  a mountpoint name, or a special name (server:/path).
 *  Return the last entry in the file that matches.
 */
static struct extmnttab *
mnttab_find(dirname)
	char *dirname;
{
	FILE *fp;
	struct extmnttab mnt;
	struct extmnttab *res = NULL;

	fp = fopen(MNTTAB, "r");
	if (fp == NULL) {
		pr_err("%s: %s", MNTTAB, strerror(errno));
		return (NULL);
	}
	while (getextmntent(fp, &mnt, sizeof (struct extmnttab)) == 0) {
		if (strcmp(mnt.mnt_mountp, dirname) == 0 ||
				strcmp(mnt.mnt_special, dirname) == 0) {
			if (res)
				fsfreemnttab(res);
			res = fsdupmnttab(&mnt);
		}
	}

	(void) fclose(fp);
	return (res);
}
