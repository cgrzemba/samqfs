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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define	X			512
#define	NVSNS		1024
#define	MAXLINE		1024

unsigned long long position, offset;
int line;    /* current line number in archiver.cmd */

/*  Fields read from the archiver.cmd file */
char flag[X], date[X], time[X], mt[X], vsn[X];
char aset[X], op[X], fs[X], ino[X], size[X], name[X];
int arcopy;

/*
 *  state information about each VSN:
 *		poss - the last-known position
 *		offsets - the last-known offset
 *		sizes - the size of the file at (pos, offset)
 *		is_bads - boolean flag indicating we have known problem
 */
unsigned long long poss[NVSNS], offsets[NVSNS], sizes[NVSNS];
char *vsns[NVSNS];
int is_bads[NVSNS];


void dumpline(void);
void repair(int index);

int
main(int argc, char **argv)
{
	unsigned long long isize;
	char key[X*2];
	char buf[MAXLINE];

	char *c;
	int x;

	int i, errs = 0, fmterrs = 0, nvsns = 0;

	int first_only = 0;
	int relabel_check = 0;

	while ((x = getopt(argc, argv, "fr")) != EOF) {
		switch (x) {
			case 'f':	first_only = 1;
						break;
			case 'r':	relabel_check = 1;
						break;
			case '?':	printf("%s: usage: %s [-f] [-r]\n",
					    argv[0], argv[0]);
						exit(1);
		}
	}

	/* read a line from the archiver log */
	while (fgets(buf, sizeof (buf), stdin)) {
		line ++;

		/* break it into the various fields */
		if (sscanf(buf,
		    "%s %s %s %s %s %s %s %s %s %s %s\n",
		    flag, date, time, mt, vsn,
		    aset, op, fs, ino, size, name) != 11) {
			/*
			 * Note that this error occurs for the root
			 * directory of the filesystem each time it's dumped.
			 * That is, the archiver doesn't get the id_to_path
			 * right for ino == 2.
			 */
			dumpline();
			printf("Line doesn't have 11 fields.  Skipping.\n");
			fmterrs++;
			continue;
		}

		/* get the archive copy number */
		if ((c = strchr(aset, '.')) == (char *)NULL) {
			dumpline();
			printf("Malformed offset.position, missing '.':%s\n",
			    op);
			fmterrs++;
			continue;
		}
		arcopy = atoi(c+1);

		/* break the position.offset into pieces */
		if ((c = strchr(op, '.')) == (char *)NULL) {
			dumpline();
			printf("Malformed offset.position, missing '.':%s\n",
			    op);
			fmterrs++;
			continue;
		}
		position = strtoull(op, (char **)NULL, 16);
		offset = strtoull(c+1, (char **)NULL, 16);

		/* get the size of the file at this (position, offset)  */
		isize = strtoull(size, (char **)NULL, 10);

		/* Lookup VSN */
		sprintf(key, "%s:%s", mt, vsn);
		for (i = 0; i < nvsns; i++) {
			if (strcmp(vsns[i], key) == 0) {
				break;
			}
		}
		if (i == nvsns) {
			/* New vsn.  add to tables */
			vsns[i] = strdup(key);
			poss[i] = position;
			offsets[i] = offset;
			sizes[i] = isize;
			is_bads[i] = 0;
			nvsns++;
			if (nvsns >= NVSNS) {
				printf("Too many vsns: %d\n", nvsns);
				exit(1);
			}
			continue;   /* New vsn, nothing to check against */
		}

		/*
		 * Is the new position less than the last-known one?
		 * If so, they probably relabeled the medium.
		 */
		if (relabel_check &&
		    (position < (unsigned long long)poss[i])) {
			dumpline();
			printf("Position decremented!  %s was "
			    "%llx.%llx(size %lld), now %llx.%llx\n",
			    vsns[i], poss[i], offsets[i], sizes[i],
			    position, offset);
			errs++;
			printf("This error cannot be corrected.\n");
			poss[i] = position; /* But, we use the new location */
			offsets[i] = offset;
			continue;
		}

		/*
		 * If it's already bad, then we continue to complain until we
		 * see a new position.  Otherwise, check for duplicate(pos,
		 * offset), and the case that started this all:  the position
		 * stays the same, but the offset gets smaller.
		 */
		if (is_bads[i]) {

			if (poss[i] == position && !first_only) {
				dumpline();
				printf("This file also bad.\n");
				repair(i);
				errs++;
			} else {
				/* we're good when we see the next position */
				is_bads[i] = 0;
			}

		} else if (poss[i] == position && offsets[i] == offset) {

			dumpline();
			printf("Offset duplicated!  %s was "
			    "%llx.%llx(size %lld), now %llx.%llx\n",
			    vsns[i], poss[i], offsets[i],
			    sizes[i], position, offset);
			is_bads[i] = 1;
			repair(i);
			errs++;

		} else if (poss[i] == position && offset < offsets[i]) {

			dumpline();
			printf("Offset decremented!  %s was "
			    "%llx.%llx(size %lld), now %llx.%llx\n",
			    vsns[i], poss[i], offsets[i],
			    sizes[i], position, offset);
			is_bads[i] = 1;
			repair(i);
			errs++;

		}

		/* save what we know */
		poss[i] = position;
		offsets[i] = offset;
		sizes[i] = isize;
	}

	printf("\n\nThere were %d position.offset errors.\n", errs);
	printf("\n\nThere were %d archiver.log formatting  errors.\n",
	    fmterrs);
	printf("\n\nThere were %d unique VSNs:\n", nvsns);
	for (i = 0; i < nvsns; i++) {
		printf("%-10s %ll20x.%-20llx %lld\n",
		    vsns[i], poss[i], offsets[i], sizes[i]);
	}
	return ((errs ? 4 : 0) | (fmterrs ? 2 : 0));
}

void
dumpline(void)
{
	printf("\nAt line %d:\n%s %s %s %s %s %s %s %s %s %s %s\n",
	    line, flag, date, time, mt, vsn, aset, op, fs, ino, size,
	    name);
}

void
repair(int index)
{
	char NOP[X];   /* New Offset and Position */
	unsigned long long noff;

	/* Here's the magic!  The "2" is for the tar header and the EOF mark */
	noff = offsets[index] + sizes[index]/512 + 2;
	sprintf(NOP, "%llx.%llx", position, noff);

	/* emit the corrected archiver log entry */
	printf("%s %s %s %s %s %s %s %s %s %s %s\n",
	    flag, date, time, mt, vsn, aset, NOP, fs, ino, size, name);

	/* emit the call to the correction program */
	printf("echo \"%s %d 0 %#llx %#llx\" | fix\n",
	    name, arcopy, position, noff);
	fprintf(stderr, "/var/tmp/lsci/fixit '%s' %d 0 %#llx %#llx\n",
	    name, arcopy, position, noff);
	offset = noff;
}
