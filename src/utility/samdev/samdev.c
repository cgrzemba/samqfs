/*
 * samdev.c  -	create device links for the devices controlled by
 * 		samst (alias bst).
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

#pragma ident "$Revision: 1.16 $"


#define	MAIN

#include <stdio.h>
#include <unistd.h>
#include <grp.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#define DEC_INIT
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/custmsg.h"

#define	DEVPATH	"/dev/samst"
#define	DEVICES	"/devices"
#define	DEVNAME	"samst"

static int debug = 0;
static int high_control = -1;

typedef struct control {
	int  	cont_num;
	uint_t 	target;
	int  	lunit;
	char	*this_path;
	char	*name;
	struct 	control *next_path;
	struct 	control *next;
} control_t;

static void init_cont(control_t *, int *);
static control_t *add_comp(char *, int *, control_t *);
static void scan_devices(char *, control_t *, int *);
static void build_names(control_t *);
static void build_link(control_t *);


int
main(int argc, char **argv)
{
	int 		count = -1;
	char		c;
	char		errmes[100];
	struct stat	stat_buf;
	control_t	*first;

	const char *progname = strdup(basename(*argv));
	CustmsgInit(0, NULL);
	while ((c = getopt(argc, argv, "d")) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;
		default:
			fprintf(stderr, "usage: %s [-d]\n", progname);
			break;
		}
	}
	first = (control_t *)malloc(sizeof (control_t));
	memset(first, 0, sizeof (control_t));
	first->cont_num = -1;
	init_cont(first, &count);
	scan_devices(DEVICES, first, &count);
	if (count == -1) {
		fprintf(stderr,
		    catgets(catfd, SET, 2214,
		    "samdev: warning: No samst devices found.\n"));
		fprintf(stderr, "\n");
		exit(0);
	}
	if (stat(DEVPATH, &stat_buf) < 0) {
		if (errno != ENOENT) {
			sprintf(errmes,
			    catgets(catfd, SET, 2212,
			    "samdev: cannot stat %s:"),
			    DEVPATH);
			perror(errmes);
			exit(1);
		}
		if (mkdir(DEVPATH, 0775)) {
			sprintf(errmes,
			    catgets(catfd, SET, 2210,
			    "samdev: cannot create %s:"),
			    DEVPATH);
			perror(errmes);
			exit(1);
		}
	}
	if (debug)
		printf("\n\n");
	build_names(first);
	return (0);
}

static void
build_names(control_t *cont)
{
	control_t *current;

	for (current = cont; current != NULL; current = current->next_path) {
		if (current->cont_num == -1)
			current->cont_num = ++high_control;

		build_link(current);
		if (current->next != NULL) {
			control_t *nxt_curr = current->next;
			for (; nxt_curr != NULL; nxt_curr = nxt_curr->next) {
				nxt_curr->cont_num = current->cont_num;
				build_link(nxt_curr);
			}
		}
	}
}

static void
build_link(control_t *entry)
{
	struct	stat	stat_buf;
	char	linkentry[MAXPATHLEN], path[MAXPATHLEN];
	char	errmes[MAXPATHLEN];

	sprintf(linkentry, "%s/c%dt%du%d", DEVPATH, entry->cont_num,
	    entry->target, entry->lunit);
	sprintf(path, "%s/%s", entry->this_path, entry->name);
	if (debug)
		printf("Building link %s -> %s\n", linkentry, path);
	if (lstat(linkentry, &stat_buf) >= 0) {
		unlink(linkentry);
	} else if (errno != ENOENT) {
		sprintf(errmes,
		    catgets(catfd, SET, 2213, "samdev: Unable to lstat: %s"),
		    linkentry);
		perror(errmes);
	}
	if (symlink(path, linkentry)) {
		sprintf(errmes,
		    catgets(catfd, SET, 2211,
		    "samdev: cannot create symbolic link: %s -> %s:"),
		    linkentry, path);
		perror(errmes);
	}
}


static void
init_cont(control_t *cont, int *count)
{
	int 		lnklen;
	char		link[MAXPATHLEN], full_name[MAXPATHLEN];
	control_t	*where;
	struct dirent	*entry;
	DIR		*dir;

	if ((dir = opendir(DEVPATH)) == (DIR *) NULL)
		return;

	while ((entry = readdir(dir)) != (struct dirent *)NULL) {
		if (memcmp(&entry->d_name, "..", 2) == 0)
			continue;

		if (memcmp(&entry->d_name, ".", 1) == 0)
			continue;


		sprintf(full_name, "/dev/samst/%s", entry->d_name);
		if ((lnklen = readlink(full_name, &link[0], MAXPATHLEN)) < 0)
			continue;
		if (lnklen == 0 || strlen(link) == 0) {	/* links to nowhere */
			unlink(full_name);
			continue;
		}
		link[lnklen] = '\0';
		where = add_comp(link, count, cont);
		if (where == (control_t *)NULL)
			continue;
		sscanf(entry->d_name, "c%d", &where->cont_num);
		if (where->cont_num > high_control)
			high_control = where->cont_num;
	}
	closedir(dir);
}

static void
scan_devices(char *path, control_t *cont, int *count)
{
	char	full_path[MAXPATHLEN];
	char	errmes[MAXPATHLEN];
	struct 	stat	stat_buf;
	struct 	dirent	*entry;
	DIR	*dir;

	if ((dir = opendir(path)) == (DIR *) NULL)
		return;

	while ((entry = readdir(dir)) != (struct dirent *)NULL) {
		if (memcmp(&entry->d_name, "..", 2) == 0)
			continue;

		if (memcmp(&entry->d_name, ".", 1) == 0)
			continue;

		sprintf(full_path, "%s/%s", path, entry->d_name);
		if (stat(full_path, &stat_buf)) {
			sprintf(errmes, "stat: %s:", full_path);
			perror(errmes);
			exit(1);
		}
		if (S_ISDIR(stat_buf.st_mode))
			scan_devices(full_path, cont, count);

		if (memcmp(entry->d_name, DEVNAME, 5) == 0) {
			if (S_ISCHR(stat_buf.st_mode)) {
				chown(full_path, -1, 0);
				chmod(full_path, 0664);
				(void) add_comp(full_path, count, cont);
			} else if (S_ISBLK(stat_buf.st_mode)) {
				chown(full_path, -1, 0);
				chmod(full_path, 0664);
			}
		}
	}
	closedir(dir);
}

static control_t *
add_comp(char *path, int *count, control_t *cont)
{
	char		*tmp, *tmp2, *dev_name;
	control_t	*current, *last;

	current = cont;
	last = current;
	tmp = strdup(path);
	tmp2 = strstr(tmp, "/samst@");
	if (tmp2 == NULL)
		return ((control_t *)NULL);
	*tmp2 = '\0';		/* remove the / */
	dev_name = tmp2 + 1;	/* point to the name */

	if (*count < 0) {
		*count = 0;
		current->this_path = tmp;
		current->name = dev_name;
		sscanf(dev_name, "samst@%x,%d", &current->target,
		    &current->lunit);
		if (debug) {
			printf("Add      - path = %s - device = %s "
			    "- targ = %x, lunit = %d\n",
			    current->this_path, current->name,
			    current->target, current->lunit);
		}
		return (current);
	}
	for (current = cont; current != NULL; current = current->next_path) {
		if (strcmp(current->this_path, tmp) == 0) {
			int i = 0;

			last = current;
			if (strcmp(current->name, dev_name) == 0)
				return (current);

			for (current = current->next;
			    current != NULL;
			    current = current->next) {
				if (strcmp(current->name, dev_name) == 0)
					return (current);
				i++;
				last = current;
			}
			current = (control_t *)malloc(sizeof (control_t));
			memset(current, 0, sizeof (control_t));
			last->next = current;
			current->cont_num = last->cont_num;
			current->this_path = tmp;
			current->name = dev_name;
			sscanf(dev_name, "samst@%x,%d", &current->target,
			    &current->lunit);
			if (debug) {
				printf("Add(%3.3d) - path = %s - device = %s "
				    " - targ = %x, lunit = %d\n",
				    i,
				    current->this_path, current->name,
				    current->target, current->lunit);
			}
			return (current);
		}
		last = current;
	}

	current = (control_t *)malloc(sizeof (control_t));
	memset(current, 0, sizeof (control_t));
	last->next_path = current;
	current->cont_num = -1;
	current->this_path = tmp;
	current->name = dev_name;
	sscanf(dev_name, "samst@%x,%d", &current->target, &current->lunit);
	if (debug) {
		printf("Add      - path = %s - device = %s "
		    "- targ = %x, lunit = %d\n",
		    current->this_path, current->name,
		    current->target, current->lunit);
	}
	return (current);
}
