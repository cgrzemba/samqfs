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

#pragma ident "$Revision: 1.8 $"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/types.h>
#include <libgen.h>

typedef struct mod_s {
	char *line;
	int found;
} mod_t;

#define	LINES_SAR 3
mod_t mod_sar [LINES_SAR] = {
{"0 * * * 0-6 /usr/lib/sa/sa1", 0},
{"20,40 8-17 * * 1-5 /usr/lib/sa/sa1", 0},
{"5 18 * * 1-5 /usr/lib/sa/sa2 -s 8:00 -e 18:01 -i 1200 -A", 0}
};

#define	LINES_PM 6
mod_t mod_pm [LINES_PM] = {
{"05 0 * * *    /etc/opt/SUNWsamfs/sampm/proc-csd", 0},
{"05 * * * *    /etc/opt/SUNWsamfs/sampm/proc-csd-hour", 0},
{"55 23 * * *    /etc/opt/SUNWsamfs/sampm/proc-logs", 0},
{"00,30 * * * * /etc/opt/SUNWsamfs/sampm/watch", 0},
{"00 * 01-31 * * /etc/opt/SUNWsamfs/sampm/watch-day", 0},
{"15 0 * * *    /etc/opt/SUNWsamfs/sampm/system_report", 0}
};

#define	LINES_PERF 13
mod_t mod_perf [LINES_PERF] = {
{"if [ -z \"$_INIT_RUN_LEVEL\" ]; then", 0},
{"set -- `/usr/bin/who -r`", 0},
{"_INIT_RUN_LEVEL=\"$7\"", 0},
{"_INIT_RUN_NPREV=\"$8\"", 0},
{"_INIT_PREV_LEVEL=\"$9\"", 0},
{"fi", 0},
{"\0", 0},
{"if [ $_INIT_RUN_LEVEL -ge 2 -a $_INIT_RUN_LEVEL -le 4 -a \\", 0},
{"$_INIT_RUN_NPREV -eq 0 -a \\( $_INIT_PREV_LEVEL = 1 -o \\", 0},
{"$_INIT_PREV_LEVEL = S \\) ]; then", 0},
{"\0", 0},
{"/usr/bin/su sys -c \"/usr/lib/sa/sadc /var/adm/sa/sa`date +%d`\"", 0},
{"fi", 0}
};

#define	LINES_DEFS 2
mod_t mod_defs [LINES_DEFS] = {
{"devlog = all all", 0},
{"debug = all all", 0}
};

enum {ACTIVATE, DEACTIVATE} action;
char *option_str [] = {
	"sar",
	"pm",
	"perf",
	"defs",
	NULL
};
enum {SAR, PM, PERF, DEFS} option;

#define	LINE 256


/* --- modify_file - comment or uncomment file lines. */
int
modify_file(char *page, mod_t *mod)
{
	char *p1, *p2;
	int k;
	char buf[LINE];

	if ((p1 = strstr(page, mod->line)) == NULL)
		return (B_FALSE);

	if (option == PERF && mod->line[0] == '\0')
		return (B_TRUE);

	if (action == ACTIVATE) {
		if ((p2 = strchr(page, '#')) == NULL || p2 > p1) {
			mod->found++;
			return (B_TRUE);
		}

		for (k = 0; k < strlen(page) && page < p1; k++) {
			if (isspace(page[k]))
				return (B_FALSE);

			if (page[k] == '#') {
				page[k] = ' ';
				mod->found++;
				break;
			}
		}
	} else if (action == DEACTIVATE) {
		if ((p2 = strchr(page, '#')) != NULL && p2 < p1)
			return (B_TRUE);

		for (k = 0; k < strlen(page) && page < p1; k++) {
			if (isspace(page[k])) {
				break;
			}

			if (page[k] == '#') {
				mod->found++;
				break;
			}
		}

		if (!mod->found) {
			if (isspace(page[0])) {
				page[0] = '#';
			} else {
				strcpy(buf, page);
				page[0] = '#';
				strcpy(&page[1], buf);
			}
		} else {
			/* comment out duplicates */
			mod->found = 0;
		}
	}

	return (B_TRUE);
}

/* --- cnv_path - convert path / to \/ for sed. */
void
cnv_path(char *path)
{
	int i, j, count = 0;
	char *buf;

	for (i = 0; i < strlen(path); i++)
		if (path[i] == '/')
			count++;

	buf = (char *)malloc(strlen(path)+count+1);

	for (i = 0, j = 0; i < strlen(path); i++) {
		if (path[i] == '/')
			buf[j++] = '\\';
		buf[j++] = path[i];
	}

	printf("%s", buf);
	free(buf);
}

/* --- main - setup files and perform utility tasks. */
int
main(int argc, char **argv)
{
	FILE *fp;
	char buf[LINE];
	unsigned char found = 0;
	int i, j, k, count;
	char **page, *p1, *p2;
	int rval;
	char *fn = NULL, perf_fn [] = "/etc/init.d/perf";
	char defaults_fn [] = "/etc/opt/SUNWsamfs/defaults.conf";
	int c;
	int LINES;
	mod_t *mod;

	while ((c = getopt(argc, argv, "hva:d:f:c:")) != EOF) {
		switch (c) {
		case 'a':
		case 'd':
		action = ((char)c == 'a' ? ACTIVATE : DEACTIVATE);

		found = 0;
		for (i = 0; !found && option_str[i] != NULL; i++) {
			if (strcmp(option_str[i], optarg) == 0) {
				found = 1;
				option = i;
			}
		}
		if (!found) {
			fprintf(stderr, "Invalid option -%c %s\n",
			    c, optarg);
			exit(-1);
		}
		break;

		case 'f':
		fn = optarg;
		break;

		case 'c':
		cnv_path(optarg);
		exit(0);

		case 'v':
		fprintf(stderr, "%s v%.1f\n", basename(argv[0]), VERSION);
		exit(0);

		case 'h':
		break;

		default:
		exit(1);
		} /* switch */
	} /* while */

	if (!found) {
		fprintf(stderr,
		    "usage: edit [-h|-v] [-a|-b sar|pm -f filename] "
		    "[-a|-b perf|defs] [-c path]\n");
		exit(2);
	}


	/* Check for crontab temp page. */
	if ((option == SAR || option == PM) && fn == NULL) {
		fprintf(stderr, "Missing tmp crontab file name.\n");
		return (-1);
	}

	if (option == SAR) {
		LINES = LINES_SAR;
		mod = mod_sar;
	} else if (option == PM) {
		LINES = LINES_PM;
		mod = mod_pm;
	} else if (option == PERF) {
		LINES = LINES_PERF;
		mod = mod_perf;
		fn = perf_fn;
	} else if (option == DEFS) {
		LINES = LINES_DEFS;
		mod = mod_defs;
		fn = defaults_fn;
	} else {
		fprintf(stderr, "Action and option not understood.\n");
		exit(-2);
	}

	/* Open crontab page. */
	if ((fp = fopen(fn, "r+")) == NULL) {
		fprintf(stderr, "Unable to open %s\n", fn);
		return (-2);
	}

	/* Count crontab page lines. */
	count = 0;
	while (fgets(buf, LINE, fp)) {
		count++;
	}
	rewind(fp);
	page = (char **)malloc(count * sizeof (char *));

	/* Read and modify crontab page lines. */
	j = 0;
	for (i = 0; i < count; i++) {
		page[i] = (char *)malloc(LINE);
		fgets(page[i], LINE, fp);

		if (option == SAR || option == PM || option == DEFS) {
			for (j = 0; j < LINES; j++) {
				modify_file(page[i], &mod[j]);
			}
		} else if (option == PERF) {
			if (j < LINES) {
				if (modify_file(page[i], &mod[j]) == B_TRUE) {
					j++;
				} else if (j > 0) {
				/* N.B. bad indentation to meet cstyle */
				/* requirements */
				fprintf(stderr,
				    "Quiting, lines not in expected sequence.");
				goto cleanup;
				}
			}
		}
	}

	/* Check for at most one entry. */
	if (action == ACTIVATE &&
	    (option == SAR || option == PM || option == DEFS)) {
		for (i = 0; i < LINES; i++) {
			if (mod[i].found > 1) {
				rval = 1;
				goto cleanup;
			}
		}
	}

	/* Rewrite page lines. */
	rewind(fp);
	for (i = 0; i < count; i++) {
		fprintf(fp, "%s", page[i]);
	}

	/* Add not found crontab page lines. */
	if (action == ACTIVATE &&
	    (option == SAR || option == PM || option == DEFS)) {
		for (i = 0; i < LINES; i++) {
			if (mod[i].found == 0) {
				fprintf(fp, "%s\n", mod[i].line);
			}
		}
	}

	rval = 0;

cleanup:
	/* Cleanup. */
	for (i = 0; i < count; i++) {
		free(page[i]);
	}
	free(page);
	fclose(fp);
	return (rval);
}
