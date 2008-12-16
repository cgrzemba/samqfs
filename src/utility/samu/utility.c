/*
 * utility.c - Utility functions.
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

#pragma ident "$Revision: 1.27 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* ANSI C headers. */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

/* POSIX headers. */
#include <sys/types.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>

/* Solaris headers. */
#include <curses.h>
#include <libgen.h>
#include <sys/param.h>
#include <sys/varargs.h>

/* SAM-FS headers. */
#include "aml/device.h"
#include "sam/devnm.h"
#include "sam/param.h"
#include "aml/robots.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/nl_samfs.h"
#include "aml/catalog.h"
#include "aml/catlib.h"

/* Local headers. */
#include "samu.h"

/* Structures. */
struct userid {
	uid_t uid;
	char *name;
	struct userid *next;
};

/*
 * Check if eq is a mounted disk family set device.
 */
void
CheckFamilySetByEq(char *eqarg, struct sam_fs_info *fi)
{
	int eq;
	char *p;

	errno = 0;
	eq = strtol(eqarg, &p, 0);
	if (*p != '\0' || errno != 0 || eq == 0 ||
	    (GetFsInfoByEq(eq, fi) < 0)) {
		Error(catgets(catfd, SET, 1805,
		    "Not a disk family set device (%s)."),
		    eqarg);
	}
	if ((fi->fi_status & FS_MOUNTED) == 0)  {
		Error(catgets(catfd, SET, 1180,
		    " Filesystem %s is not mounted."),
		    eqarg);
	}
}


/*
 * Check if eq is a mounted disk partition device.
 */
char *
CheckFSPartByEq(char *eqarg, struct sam_fs_info *fi)
{
	int eq;
	char *p;
	char *retval = NULL;

	errno = 0;
	eq = strtol(eqarg, &p, 0);
	if (*p != '\0' || errno != 0 || eq == 0 ||
	    (GetFsInfoByPartEq(eq, fi) < 0)) {
		retval = catgets(catfd, SET, 1804,
		    "Not a disk eq number in a family set (%s).");
	} else {
		if ((fi->fi_status & FS_MOUNTED) == 0)  {
			retval = catgets(catfd, SET, 1180,
			    "Filesystem %s is not mounted.");
		}
	}
	return (retval);
}

/*
 * Check if name is a mount point or family set name, and mounted.
 * Clear Mount_Point if not.
 */
static void
CheckFsMpByName(char *name, struct sam_fs_info *fi)
{
	if ((GetFsInfo(name, fi) < 0)) {
		Mount_Point = NULL;
		Error(catgets(catfd, SET, 7006,
		    "Not a SAM-FS file system or not mounted (%s)."), name);
	}
	if ((fi->fi_status & FS_MOUNTED) == 0)  {
		Mount_Point = NULL;
		Error(catgets(catfd, SET, 1180,
		    " Filesystem %s is not mounted."),
		    name);
	}
}

/*
 *	Return pointer to 8.3 size conversion in bytes with suffix.
 *	To the start of the string.
 */
char *
FsizeToB(
	fsize_t v)	/* Value to convert. */
{
#define	EXA  (1024LL * 1024 * 1024 * 1024 * 1024 * 1024)
#define	PETA (1024LL * 1024 * 1024 * 1024 * 1024)
#define	TERA (1024LL * 1024 * 1024 * 1024)
#define	GIGA (1024LL * 1024 * 1024)
#define	MEGA (1024LL * 1024)
#define	KILO (1024LL)

/* lower bounds  - will round up to the given value */
#define	EXAL  (((1024LL * 1024) - 512) * 1024 * 1024 * 1024 * 1024)
#define	PETAL (((1024LL * 1024) - 512) * 1024 * 1024 * 1024)
#define	TERAL (((1024LL * 1024) - 512) * 1024 * 1024)
#define	GIGAL (((1024LL * 1024) - 512) * 1024)
#define	MEGAL ((1024LL * 1024) - 512)
#define	KILOL (1023.512)

	static char buf[32];
	char		c;
	float		f;

	c = '\0';
	if (v >= EXAL) {
		c = 'E';
		f = ((float)v) / (PETA);
	} else if (v >= PETAL) {
		c = 'P';
		f = ((float)v) / (TERA);
	} else if (v >= TERAL) {
		c = 'T';
		f = ((float)v) / (GIGA);
	} else if (v >= GIGAL) {
		c = 'G';
		f = ((float)v) / (MEGA);
	} else if (v >= MEGAL) {
		c = 'M';
		f = ((float)v) / (KILO);
	} else {
		f = ((float)v);
		if (f >= KILOL) {
			c = 'k';
		}
	}

	/* %f format will round up */

	if (f >= KILOL) {
		f = f / 1024.0;
		sprintf(buf, "%8.3f%c", f, c);
	} else {
		sprintf(buf, "%4.0f     ", f);
	}
	return (buf);
}

/*
 * Find device.
 * Locate a specified device by ordinal.
 * Device must be a robotic library, removable media device, or
 * remote sam pseudo-device.
 */
int
findDev(
	char *eq)	/* Ordinal of device */
{
	int n;
	char *endptr;
	dev_ent_t *dev;

	if (isdigit(*eq)) {
		n = strtol(eq, &endptr, 0);
		if (*endptr != '\0') {
			Error(catgets(catfd, SET, 2350,
			    "%s is not a valid equipment ordinal.\n"), eq);
			/* NOTREACHED */
		}
		if (n < 0 || n > Max_Devices) {
			Error(catgets(catfd, SET, 882,
			    "Device number out of range (0 < %d < %d)"),
			    n, Max_Devices);
		}
		if (Dev_Tbl->d_ent[n] == NULL)
			Error(catgets(catfd, SET, 854,
			    "Device %d not defined."), n);
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[n]);
		if (IS_OPTICAL(dev) || IS_TAPE(dev) || IS_ROBOT(dev) ||
		    IS_RSS(dev) || IS_RSC(dev) || IS_RSD(dev)) {
			return (n);
		}
		Error(catgets(catfd, SET, 7008,
		    "Invalid equipment %s for this command."), eq);
	}
	Error(catgets(catfd, SET, 856, "Device %s not defined."), eq);
	/* NOTREACHED */
}

/*
 * Find device.
 * Locate a specified device either by name or by ordinal.
 */
int
finddev(
	char *name)	/* Name or ordinal of device */
{
	dev_ent_t *dev;
	int n;
	char *endptr;

	if (Dev_Tbl == NULL) {
		Error(catgets(catfd, SET, 856, "Device %s not defined."), name);
		/* NOTREACHED */
	}

	if (isdigit(*name)) {
		n = strtol(name, &endptr, 0);
		if (*endptr != '\0') {
			Error(catgets(catfd, SET, 2350,
			    "%s is not a valid equipment ordinal.\n"), name);
			/* NOTREACHED */
		}
		if (n < 0 || n > Max_Devices) {
			Error(catgets(catfd, SET, 882,
			    "Device number out of range (0 < %d < %d)"),
			    n, Max_Devices);
		}
		if (Dev_Tbl->d_ent[n] == NULL)
			Error(catgets(catfd, SET, 854,
			    "Device %d not defined."), n);
		return (n);
	}
	for (n = 0; n <= Max_Devices; n++) {
		if (Dev_Tbl->d_ent[n] == NULL)  continue;
		dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[n]);
		if (strcmp(name, dev->name) == 0)
			return (n);
		if (strcmp(name, basename(dev->name)) == 0)
			return (n);
	}
	Error(catgets(catfd, SET, 856, "Device %s not defined."), name);
	/* NOTREACHED */
}

/*
 * Find file system partition.
 * Locate a specified file system partition either by name or by ordinal.
 */
struct sam_fs_part *
findfsp(
	char *name,			/* Name or ordinal of device */
	struct sam_fs_part **fp)
{
	int	i;
	int	j;
	int	n;
	int	numfs;
	int	nparts;
	char *endptr;
	struct sam_fs_status *fsarray;
	struct sam_fs_info   *fi = NULL;
	struct sam_fs_part *fpd;

	n = 0;
	if (isdigit(*name)) {
		/*
		 *	 Looks like an eq ordinal.  Check it.
		 */
		n = strtol(name, &endptr, 0);
		if (*endptr != '\0') {
			Error(catgets(catfd, SET, 2350,
			    "%s is not a valid equipment ordinal.\n"), name);
			/* NOTREACHED */
		}
	}
	if ((numfs = GetFsStatus(&fsarray)) == -1) {
		Error("GetFsStatus failed");
		/* NOTREACHED */
	}
	fi = (struct sam_fs_info *)malloc(sizeof (struct sam_fs_info));
	*fp = (struct sam_fs_part *)malloc(sizeof (struct sam_fs_part));

	for (i = 0; i < numfs; i++) {
		struct sam_fs_status *fs;

		fs = fsarray + i;
		if (GetFsInfo(fs->fs_name, fi) == -1) {
			free(fsarray);
			free(fi);
			free(*fp);
			Error("GetFsInfo(%s) failed", fs->fs_name);
			/*NOTREACHED*/
		}
		nparts = fi->fs_count + fi->mm_count;
		if (nparts == 0) {
			free(fsarray);
			free(fi);
			free(*fp);
			return (NULL);
		}
		fpd = (struct sam_fs_part *)malloc(nparts *
		    sizeof (struct sam_fs_part));

		if (GetFsParts(fs->fs_name, nparts, fpd) == -1) {
			free(fsarray);
			free(fi);
			free(*fp);
			free(fpd);
			Error("GetFsInfo(%s) failed", fs->fs_name);
			/* NOTREACHED */
		}
		for (j = 0; j < nparts; j++) {
			struct sam_fs_part *fp1;

			fp1 = fpd + j;
			if (n != 0) {
				if (n == fp1->pt_eq) {
					memcpy(*fp, fp1,
					    sizeof (struct sam_fs_part));
					free(fi);
					free(fsarray);
					free(fpd);
					return (*fp);
				}
			} else if (strcmp(fp1->pt_name, name) == 0) {
				memcpy(*fp, fp1, sizeof (struct sam_fs_part));
				free(fi);
				free(fsarray);
				free(fpd);
				return (*fp);
			}
		}
		free(fpd);
	}
	free(fi);
	free(*fp);
	free(fsarray);
	return (NULL);
}

/*
 * Get device.
 * Prompt the user to enter either a device name or an equipment ordinal.
 * Validate the entry.
 */
int
getdev(
	char *name)	/* Device name or ordinal */
{
	Mvprintw(LINES - 1, 0, catgets(catfd, SET, 873, "Device name:"));
	(void) Clrtoeol();
	cbreak();
	echo();
	getstr(name);
	noecho();

	if (*name != '\0')  DisEq = finddev(name);
	return (DisEq);
}


/*
 * Get robot.
 * Prompt the user to enter either a device name or an equipment ordinal.
 * Validate the entry as a robot.
 */
int
getrobot(char *name)
{
	dev_ent_t *dev;			/* Device entry */
	int n;

	Mvprintw(LINES - 1, 0, catgets(catfd, SET, 2139, "Robot name:"));
	(void) Clrtoeol();
	cbreak();
	echo();
	getstr(name);
	noecho();

	if (*name == '\0')
		return (DisRb);
	n = finddev(name);
	dev = (dev_ent_t *)SHM_ADDR(master_shm, Dev_Tbl->d_ent[n]);
	if ((dev->type & DT_CLASS_MASK) != DT_ROBOT) {
		Error(catgets(catfd, SET, 874, "Device not a robot (%s)."),
		    name);
	}
	DisRb = (equ_t)n;
	return (n);
}


/*
 * Convert a file mode to characters.
 */
char *
mode_string(mode_t mode)
{
	static char str[11];

	str[0] = '?';
	str[1] = (mode & S_IRUSR) ? 'r' : '-';	/* user */
	str[2] = (mode & S_IWUSR) ? 'w' : '-';
	str[3] = (mode & S_IXUSR) ? 'x' : '-';
	str[4] = (mode & S_IRGRP) ? 'r' : '-';	/* group */
	str[5] = (mode & S_IWGRP) ? 'w' : '-';
	str[6] = (mode & S_IXGRP) ? 'x' : '-';
	str[7] = (mode & S_IROTH) ? 'r' : '-';	/* world */
	str[8] = (mode & S_IWOTH) ? 'w' : '-';
	str[9] = (mode & S_IXOTH) ? 'x' : '-';
	str[10] = '\0';

	if (mode & S_ISUID)  str[3] = (mode & S_IXUSR) ? 's' : 'S';
	if (mode & S_ISGID)  str[6] = (mode & S_IXGRP) ? 's' : 'S';
	if (mode & S_ISVTX)  str[9] = (mode & S_IXOTH) ? 't' : 'T';

	if ((mode & S_IFMT) == S_IFBLK)  str[0] = 'b';	/* sp block file */
	if ((mode & S_IFMT) == S_IFCHR)  str[0] = 'c';	/* sp char file */
	if ((mode & S_IFMT) == S_IFDIR)  str[0] = 'd';	/* directory */
	if ((mode & S_IFMT) == S_IFREG)  str[0] = '-';	/* regular file */
	if ((mode & S_IFMT) == S_IFLNK)  str[0] = 'l';	/* symbolic link */
	if ((mode & S_IFMT) == S_IFIFO)  str[0] = 'p';	/* fifo */
	if ((mode & S_IFMT) == S_IFSOCK) str[0] = 's';	/* socket */
	if ((mode & S_IFMT) == S_IFREQ)  str[0] = 'R';	/* removable media */
							/* file */
	return (str);
}


/*
 * Get file system family set name.
 */
void
get_fs(void)
{
	static char name[MAXPATHLEN];

	if (FsInitialized != 0)
		return;

	if (File_System == NULL) {
		if (ScreenMode == FALSE) {
			if (Argc > 2) {
				strcpy(name, Argv[2]);
			} else Error(NULL);
		} else {
			Mvprintw(LINES - 1, 0,
			    catgets(catfd, SET, 7159, "File System:"));
			(void) Clrtoeol();
			cbreak();
			echo();
			getstr(name);
			noecho();
		}
		if (strlen(name) == 0)  Error(NULL);
	} else {
		strncpy(name, File_System, sizeof (name)-1);
	}

	File_System = strdup(name);
	FsInitialized = 1;
}


/*
 * Convert an extension inode mode to characters.
 */
char *
ext_mode_string(mode_t mode)
{
	static char str[11];

	str[0] = '?';
	str[1] = '-';
	str[2] = '-';
	str[3] = '-';
	str[4] = '-';
	str[5] = '-';
	str[6] = '-';
	str[7] = '-';
	str[8] = '-';
	str[9] = '-';
	str[10] = '\0';

	if (S_ISMVA(mode)) str[0] = 'M'; /* multivolume extension */
	if (S_ISSLN(mode)) str[0] = 'S'; /* symlink extension */
	if (S_ISRFA(mode)) str[0] = 'R'; /* resource file attr extension */
	if (S_ISHLP(mode)) str[0] = 'H'; /* hard link parent extension */
	if (S_ISACL(mode)) str[0] = 'A'; /* access control list extension */
	if (S_ISOBJ(mode)) str[0] = 'O'; /* object layout inode extension */
	return (str);
}


/*
 * Open mount point for the file system.
 */
void
open_mountpt(char *mp)
{
	static char name[MAXPATHLEN];
	struct sam_fs_info fi;

	if (Ioctl_fd != 0)
		return;

	if (mp == NULL) {
		if (ScreenMode == FALSE) {
			Error(NULL);
		} else {
			Mvprintw(LINES - 1, 0,
			    catgets(catfd, SET, 1688, "Mount point:"));
			(void) Clrtoeol();
			cbreak();
			echo();
			getstr(name);
			noecho();
		}
		if (strlen(name) == 0)  Error(NULL);
	} else  strncpy(name, mp, sizeof (name)-1);

	CheckFsMpByName(name, &fi);
	if (strcmp(name, fi.fi_mnt_point) != 0) {
		strncpy(name, fi.fi_mnt_point, sizeof (name)-1);
	}
	if ((Ioctl_fd = open(name, O_RDONLY)) < 0) {
		Ioctl_fd = 0;
		Error("open(%s)", name);
	}
}


/*
 * Convert string to printable string.
 */
char *
string(char *s)
{
	static char str[80];
	int i;

	for (i = 0; i < sizeof (str)-1; i++) {
		if (s[i] == '\0')  break;
		str[i] = isprint(s[i]) ? s[i] : '.';
	}
	str[i] = '\0';
	return (str);
}

/* curses cover functions. */

static int	Lastx = 0;
static int	Lasty = 0;

void
Attron(int attrs)
{
	if (ScreenMode) {
		(void) attron(attrs);
	}
}

void
Attroff(int attrs)
{
	if (ScreenMode) {
		(void) attroff(attrs);
	}
}

void
Clear(void)
{
	if (ScreenMode) {
		(void) clear();
	}
}

void
Clrtobot(void)
{
	if (ScreenMode) {
		(void) clrtobot();
	}
}

void
Clrtoeol(void)
{
	if (ScreenMode) {
		(void) clrtoeol();
	}
}

void
Mvprintw(int y, int x, char *fmt, /* args */ ...)
{
	va_list	args;
	char	line[MAX_INPUT];

	if (ScreenMode) {
		va_start(args, fmt);
		vsprintf(line, fmt, args);
		va_end(args);
		(void) mvprintw(y, x, "%s", line);
		return;
	} else {
		if ((y == Lasty) && (x > Lastx)) {	/* same line */
			Lastx = x;
			printf(" ");
		} else {
			Lastx = 0;
			Lasty = y;
			printf("\n");
		}
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
}

void
Printw(char *fmt, /* args */ ...)
{
	va_list	args;
	char	line[MAX_INPUT];

	if (ScreenMode) {
		va_start(args, fmt);
		vsprintf(line, fmt, args);
		va_end(args);
		printw("%s", line);
		return;
	} else {
		printf(" ");
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	}
}


/*
 * Initialize the catalog library access.
 * Error exit if not possible.
 */
void
CatlibInit(void)
{
	if (CatalogInit("samu") == -1) {
		Error("%s", catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		/* Error does not return. */
	}
}
