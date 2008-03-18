/*
 * tapealert.c - TapeAlert devlog processing
 *
 *
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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
#pragma ident "$Revision: 1.12 $"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <signal.h>
#include "tapealert_text.h"

typedef struct event {		/* tapealert event */
	char *date;		/* devlog date */
	char *time;		/* devlog time */
	int version;		/* std inquiry version byte */
	unsigned short eq;	/* device equipment number */
	int type;		/* scsi inquiry peripheral device type */
	int64_t seq_no;		/* sysevent sequence number */
	int len;		/* num of valid tapealert flags */
	uint64_t flags;		/* tapealert flags */
	char *vsn;		/* samfs volume serial number */
} event_t;

typedef struct words {		/* devlog line of = and ' ' delimited words */
	int max;		/* max number of words */
	int num;		/* number of valid words */
	char **word;		/* array of devlog lines */
} words_t;

#define	MAX_WORDS 100		/* max number of words */
#define	DATE_WORD 0		/* date offset in array of devlog lines */
#define	TIME_WORD 1		/* time offset in array of devlog lines */
#define	MSG_WORD 2		/* msg num offset in array of devlog lines */
#define	START_WORD 3		/* value offset in array of devlog lines */
#define	LEN 256			/* line length */
#define	EMPTY_STR "\0"		/* empty string */

static void free_words(words_t **);


/*
 * --- decode_tapealert - decode tapealert into text
 */
static void
decode_tapealert(event_t *event)	/* tapealert event */
{
	int 			i;
	tapealert_text_t 	text;

	tapealert_text(event->version, event->type, event->len,
	    event->flags, &text);
	if (text.count < 1) {
		return;
	}

	printf("%s %s Eq no. %u Seq no. %lld",
	    event->date, event->time, event->eq, event->seq_no);
	if (strcmp(event->vsn, EMPTY_STR) != 0) {
		printf(" VSN %s", event->vsn);
	}
	printf("\n");

	for (i = 0; i < text.count; i++) {
		printf("Code: 0x%x\n", text.msg [i].pcode);
		printf("Flag: %s\n", text.msg [i].flag);
		printf("Severity: %s\n", text.msg [i].severity);

		printf("Application message:\n");
		printf("%s\n", text.msg [i].appmsg);
		printf("Probable cause:\n");
		printf("%s\n", text.msg [i].cause);

		printf("\n");
	}

	(void) tapealert_text_done(&text);

	printf("\n");
}

/*
 * --- get_words - devlog line of words
 * Create a list of date, time, devlog message number, and = ' ' delimited
 * words.
 * Returns structured devlog line of words
 */
static words_t *
get_words(char *line)	/* raw devlog line */
{
	int		i;
	words_t		*w;
	char		*p;
	char		*pline;

	if ((w = (words_t *)calloc(1, sizeof (words_t))) == NULL) {
		printf("%s\n", strerror(errno));
		return (NULL);
	}
	w->max = MAX_WORDS;
	if ((w->word = (char **)calloc(1, sizeof (char *)*w->max)) == NULL) {
		printf("%s\n", strerror(errno));
		free(w);
		return (NULL);
	}
	for (i = 0; i < w->max; i++) {
		if ((w->word [i] = (char *)calloc(1, strlen(line)+1)) == NULL) {
			printf("%s\n", strerror(errno));
			free(w->word);
			free(w);
			return (NULL);
		}
	}

	strncpy(w->word [DATE_WORD], line, 10);
	w->word [DATE_WORD] [10] = '\0';
	if (w->word [DATE_WORD][4] != '/' || w->word [DATE_WORD][7] != '/') {
		free_words(&w);
		return (NULL);
	}

	strncpy(w->word [TIME_WORD], &line[11], 8);
	w->word [TIME_WORD] [8] = '\0';
	if (w->word [TIME_WORD][2] != ':' || w->word [TIME_WORD][5] != ':') {
		free_words(&w);
		return (NULL);
	}

	strcpy(w->word [MSG_WORD], &line[21]);
	*(strchr(w->word [MSG_WORD], ' ')) = '\0';

	pline = line;
	w->num = START_WORD;
	for (i = START_WORD; i < w->max; i++) {
		p = strchr(pline, '=');
		if (!p) {
			break;
		}
		strcpy(w->word [i], p+1);
		pline = p + 1;
		p = strchr(w->word [i], ' ');
		if (p) {
			*p = '\0';
		}
		w->num++;
		if (p == NULL) {
			break;
		}
	}

	return (w);
}

/*
 * --- free_words - release structured devlog line of words
 */
static void
free_words(words_t **words)	/* structured devlog line of words */
{
	int i;
	words_t *w = *words;

	if (*words == NULL) {
		return;
	}
	if (w->word) {
		for (i = 0; i < w->max; i++) {
			if (w->word [i]) {
				free(w->word [i]);
			}
		}
		free(w->word);
	}
	free(*words);
	*words = NULL;
}

/*
 * --- usage - program usage
 */
static void
usage(char *filename)	/* program cmd line argv 0 */
{
	printf("usage: %s [-f /var/opt/SUNWsamfs/devlog/nn] [-i stdin]\n",
	    basename(filename));
}

void
handler(int num)
{
	printf("handler received %d\n", num);
	exit(0);
}

/*
 * --- main - devlog tapealert processing
 * Read in a devlog to extract, decode, and display tapealert
 * events. Tapealert events are recorded in media changer and
 * tape drive devlogs. A typical user would not expect to see
 * any tapealert events other than cleaning events. See also
 * www.t10.org SSC-n and SMC-n for more tapealert information.
 */
int
main(int argc, char **argv)
{
	FILE		*fp = NULL;
	event_t		event;
	char		line [LEN];
	int		num;
	char		*p;
	words_t		*w;
	char		*strnum;
	int		ch;
	int 		done = 0;

	signal(SIGPIPE, handler);

	while (!done && (ch = getopt(argc, argv, "f:i")) != EOF) {
		switch (ch) {
		case 'f':
			if ((fp = fopen64(optarg, "r")) == NULL) {
				printf("%s\n", strerror(errno));
				return (1);
			}
			done = 1;
			break;
		case 'i':
			fp = stdin;
			done = 1;
			break;
		default:
			usage(argv [0]);
			return (1);
		}
	}

	if (fp == NULL) {
		usage(argv [0]);
		return (1);
	}

	memset(&event, 0, sizeof (event_t));
	while (fgets(line, LEN, fp)) {
		if (p = strrchr(line, '\n')) {
			*p = '\0';
		}
		if (strlen(line) < 26) { /* devlog line ch past 12xxx to ' ' */
			continue;
		}
		strnum = strdup(&line [20]);
		if (p = strchr(strnum, ' ')) {
			*p = '\0';
		}
		num = atoi(strnum);
		free(strnum);

		switch (num) {
		case 12006:
		case 12007:
			if ((w = get_words(line)) == NULL) {
				break;
			}
			event.date = w->word[DATE_WORD];
			event.time = w->word[TIME_WORD];
			event.version = atoi(w->word[START_WORD]);
			event.eq = (unsigned short)atoi(w->word[START_WORD+1]);
			event.type = atoi(w->word[START_WORD+2]);
			if (event.type > 0x1f) {
				printf("Invalid type: %s\n", line);
				goto done;
			}
			event.seq_no = (int64_t)strtoll(w->word[START_WORD+3],
			    NULL, 10);
			event.len = atoi(w->word[START_WORD+4]);
			if (event.len > 64) {
				printf("Invalid len: %s\n", line);
				goto done;
			}
			event.flags = (uint64_t)strtoll(w->word[START_WORD+5],
			    NULL, 16);
			if (event.vsn) {
				free(event.vsn);
			}
			if (num == 12006) {
				event.vsn = strdup(EMPTY_STR);
			} else {
				event.vsn = strdup(w->word[START_WORD+6]);
			}
			decode_tapealert(&event);
done:
			free_words(&w);
			break;
		}
	}

	return (0);
}
