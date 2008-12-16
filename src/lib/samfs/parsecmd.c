/*
 * parsecmd.c - Parse command file.
 *
 * This is an attempt to provide general use utility to parse various command
 * file. Current implementation covers simple format of command file: keyword
 * = value
 *
 * Almost straight it could be used to parse defaults.conf, releaser.cmd,
 * preview.cmd, and with minor modifications recycler.cmd. Parse cmd API
 * consist of: parse_init() - to set command file name and error logging
 * parameters, parse_cmd_file() - does all processing with the help of user's
 * post processing functions parse_fini() - releases all resourses used for
 * processing.
 *
 * Examples of using this parse cmd API are in ./src/tests/lib directory.
 * samparse_simple.c is simply assigning values to different types of
 * commands. samparse.c deals with command file where some command will
 * change the exact meaning of subsequent statements in the command file.
 * Example of such command file will be the releaser.cmd and the preview.cmd:
 *
 * fs = sam1 # parameters for filesystem sam1 lwm_priority = 0 hwm_priority =
 * 100 fs = sam2 # parameters for filesystem sam2 lwm_priority = 0
 * hwm_priority = 200
 *
 * include file "sam/parsecmd.h" contains description of function parameters and
 * structures used in parse API.
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

#pragma ident "$Revision: 1.18 $"

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <sys/param.h>
#include <syslog.h>

/* Solaris headers. */


/* SAM-FS headers.  */
#include "aml/parsecmd.h"
#include "aml/errorlog.h"
#include "sam/nl_samfs.h"
#include "sam/lib.h"

#define	BASE_10    10


typedef struct parse_handle {
	int	ph_magic;
	char	*ph_program;
	int	ph_errlog;
	FILE	*ph_logfile;
	char	*ph_cmdname;
	FILE	*ph_cmdfile;
	char	ph_ibuf[256];
	char	ph_tbuf[256];
	int	ph_lineno;
	char	*ph_next_char;
	int	ph_errors;
} parse_handle_t;



/* Private functions. */
static int parse_getline(parse_handle_t *);
static char *parse_gettoken(parse_handle_t *);

static int LookupKeyword(char *, char *, parse_handle_t *, cmd_ent_t *);
static void parse_emit(int, FILE *, int, char *, ...);

static int parse_float(cmd_ent_t *cmd, char *token);
static int parse_long(cmd_ent_t *cmd, char *token);
static int parse_bool(cmd_ent_t *cmd, char *token);
static int parse_string(cmd_ent_t *cmd, char *token);




/*
 * parse_init - set program name and logging info to be used for all
 * subsequent calls to parse_cmd_file()
 */
void *
parse_init(char *cmdname, char *program, int errlog, FILE *log)
{
	parse_handle_t *ph;

	if ((ph = malloc(sizeof (parse_handle_t))) == NULL)
		return (NULL);

	ph->ph_magic = 'PMGC';

	/*
	 * Open the command file.
	 */
	if ((ph->ph_cmdfile = fopen(cmdname, "r")) == NULL)
		return (NULL);

	ph->ph_cmdname = strdup(cmdname);
	if (program)
		ph->ph_program = strdup(program);

	ph->ph_errlog = errlog;
	ph->ph_logfile = log;
	ph->ph_lineno = 0;
	ph->ph_errors = 0;

	return ((void *) ph);
}



/*
 * parse_fini - release all resources allocated in parse_init
 */
void
parse_fini(void *vph)
{
	parse_handle_t *ph = (parse_handle_t *)vph;

	free(ph->ph_program);
	(void) fclose(ph->ph_cmdfile);
	free(ph);
}


/*
 * parse_cmd_file - reads and parses command file.
 *
 * Parameters:
 *
 * Commands  -  commands table, supplied by caller,
 *
 */
int
parse_cmd_file(void *vph, cmd_ent_t *Commands)	/* commands table */
{
	parse_handle_t	*ph = (parse_handle_t *)vph;
	char		*token;

	if (ph->ph_magic != 'PMGC') {
		errno = EBADF;
		return (-1);
	}
	/*
	 * Read the command file.
	 */
	while (parse_getline(ph) != EOF) {
		char name[MAXPATHLEN];

		token = parse_gettoken(ph);
		if (token == NULL || *token == '\0')
			continue;
		if (isalpha(*token)) {
			strncpy(name, token, sizeof (name) - 1);

			token = parse_gettoken(ph);
			if (token != NULL || strcmp(token, "=") == 0)
				if (LookupKeyword(name, token, ph, Commands)
				    != 0)
					break;
		} else {
			parse_emit(ph->ph_errlog, ph->ph_logfile, 12010,
			    "%s: bad token '%s' at line %d of %s",
			    ph->ph_program, token, ph->ph_lineno,
			    ph->ph_cmdname);
			ph->ph_errors++;
		}
	}

	if (ph->ph_errors) {
		parse_emit(ph->ph_errlog, ph->ph_logfile, 12011,
		    "%s: %d error(s) in command file %s",
		    ph->ph_program, ph->ph_errors, ph->ph_cmdname);
	}
	return (ph->ph_errors);
}


/*
 * LookupKeyword - Matches keyword in command table and call appropriate
 * function to assign the value. Return -1 if post processor function fails,
 * this is the way for caller to interrupt parsing command file. Otherwise
 * returns 0 to move on to next line.
 */
static int
LookupKeyword(char *keyword, char *token, parse_handle_t *ph, cmd_ent_t *Table)
{
	cmd_ent_t	*cmd = Table;
	int		rc = 0;

	/*
	 * Search table for keyword match.
	 */
	while (strcmp(cmd->key, keyword) != 0) {
		cmd++;
		if (cmd->key == NULL) {
			parse_emit(ph->ph_errlog, ph->ph_logfile, 12016,
			    "%s: unknown keyword at line %d of %s",
			    ph->ph_program, ph->ph_lineno, ph->ph_cmdname);
			return (0);
		}
	}

	/*
	 * Check argument.
	 */
	if (cmd->parse_type != PARSE_NONE) {
		if ('\0' == *token && cmd->parse_type != PARSE_BOOL) {
			parse_emit(ph->ph_errlog, ph->ph_logfile, 12018,
			    "%s: missing value at line %d of %s",
			    ph->ph_program, ph->ph_lineno, ph->ph_cmdname);
			return (0);
		}
		if ((token = parse_gettoken(ph)) == NULL &&
		    cmd->parse_type != PARSE_BOOL) {

			parse_emit(ph->ph_errlog, ph->ph_logfile, 12018,
			    "%s: missing value at line %d of %s",
			    ph->ph_program, ph->ph_lineno, ph->ph_cmdname);
			return (0);
		}
	}
	/*
	 * Call handler.
	 */
	switch (cmd->parse_type) {
	case PARSE_LONG:
		rc = parse_long(cmd, token);
		break;

	case PARSE_FLOAT:
		rc = parse_float(cmd, token);
		break;

	case PARSE_BOOL:
		rc = parse_bool(cmd, token);
		break;

	case PARSE_STRING:
		rc = parse_string(cmd, token);
		break;

	case PARSE_NONE:
		/* Caller opted to ignore this command at this time  */
		break;

	default:
		/* Just ignore */
		break;
	}

	if (rc == -1) {
		switch (errno) {
		case EINVAL:
			parse_emit(ph->ph_errlog, ph->ph_logfile, 12012,
			    "%s: error converting '%s' to %s at line %d"
			    " of %s", ph->ph_program, token, cmd->key,
			    ph->ph_lineno, ph->ph_cmdname);
			break;

		case ERANGE:
			if (cmd->parse_type == PARSE_STRING)
				parse_emit(ph->ph_errlog, ph->ph_logfile, 12014,
				    "%s: invalid string \"%s\" for command %s "
				    "at line %d of %s",
				    ph->ph_program, token, cmd->key,
				    ph->ph_lineno, ph->ph_cmdname);
			else
				parse_emit(ph->ph_errlog, ph->ph_logfile, 12013,
				    "%s: out of range value %s for command"
				    " %s at line %d of %s",
				    ph->ph_program, token, cmd->key,
				    ph->ph_lineno, ph->ph_cmdname);
			break;

		default:
			break;
		}
		ph->ph_errors++;
	} else {
		/* Call user's post processor function  */
		if (cmd->post_func != NULL) {
			if (cmd->post_func(cmd) != 0)
				return (-1);
		}
	}

	return (0);
}


static int
parse_float(
	cmd_ent_t *cmd,		/* command entry in the table */
	char *token)		/* token read from cmd file   */
{
	float	*range = (float *)cmd->valid;
	char	*p;
	float	value;

	errno = 0;
	value = strtod(token, &p);
	if ((value == 0.0 && errno == ERANGE) || *p != '\0') {
		if (errno != ERANGE)
			errno = EINVAL;
		return (-1);
	} else if (range != NULL && (value < range[0] || value > range[1])) {
		errno = ERANGE;
		return (-1);
	}
	*(float *)cmd->var = value;
	return (0);
}


static int
parse_long(
	cmd_ent_t *cmd,	/* command entry in the table */
	char *token)	/* token read from cmd file   */
{
	long	*range = (long *)cmd->valid;
	char	*p;
	long	value;

	errno = 0;
	value = strtol(token, &p, BASE_10);
	if ((value == 0 && errno == EINVAL) || *p != '\0') {
		errno = EINVAL;
		return (-1);
	} else if (range != NULL && (value < range[0] || value > range[1])) {
		errno = ERANGE;
		return (-1);
	}
	*(long *)cmd->var = value;
	return (0);
}

static int
parse_string(
	cmd_ent_t *cmd,		/* command entry in the table */
	char *token)		/* token read from cmd file   */
{
	char	**valid_str = (char **)cmd->valid;
	int	i;

	for (i = 0; valid_str[i] != NULL; i++) {
		if (strcmp(token, valid_str[i]) == 0)
			break;
	}

	if (valid_str[i]) {
		strcpy((char *)cmd->var, token);
		return (0);
	} else {
		errno = ERANGE;
		return (-1);
	}
}


static int
parse_bool(
	cmd_ent_t *cmd,	/* command entry in the table */
	char *token)	/* token read from cmd file   */
{
	if (token == NULL || *token == '\0')
		*(boolean_t *)cmd->var = B_TRUE;
	else if (strcasecmp(token, "TRUE") == 0)
		*(boolean_t *)cmd->var = B_TRUE;
	else if (strcasecmp(token, "FALSE") == 0)
		*(boolean_t *)cmd->var = B_FALSE;
	else {
		errno = ERANGE;
		return (-1);
	}

	return (0);
}

/*
 * Get line from source input file.
 */
static int	/* EOF if end of file, else 0 */
parse_getline(parse_handle_t *ph)
{
	if (fgets(ph->ph_ibuf, sizeof (ph->ph_ibuf) - 1, ph->ph_cmdfile)
	    == NULL)
		return (EOF);

	ph->ph_ibuf[strlen(ph->ph_ibuf) - 1] = '\0';
	ph->ph_next_char = ph->ph_ibuf;
	ph->ph_lineno++;
	return (0);
}

/*
 * Provides input tokens.
 */
static char *
parse_gettoken(parse_handle_t *ph)
{
	char	c;
	char	*p;
	char	*token;
	char	*next_char = ph->ph_next_char;

	p = token = ph->ph_tbuf;

next:
	*p = '\0';
	if (NULL == next_char) {
		if (parse_getline(ph) == EOF)
			return (NULL);
		next_char = ph->ph_next_char;
	}
	/* Skip white space. */
	while (isspace((c = *next_char)))
		next_char++;
	if ('\\' == c && '\0' == *(next_char + 1)) {
		/* continuation line */
		next_char = NULL;
		goto next;
	}
	/* End of line or comment. */
	if ('\0' == c || '#' == c)
		return (token);

	if (p == token && '"' == c) {	/* quoted string */
		int	n;

		n = sizeof (ph->ph_tbuf) - 1;
		next_char++;
		while ((c = *next_char++) != '"') {
			if ('\0' == c)
				break;

			/*
			 * Character escape sequence.
			 */
			if ('\\' == c) {
				static char	transtab[] = "b\bf\fn\nr\rt\t";
				char		*pt;

				c = *next_char++;
				/*
				 * Next character in escape sequence table.
				 */
				if (islower(c) && (pt = strchr(transtab, c))
				    != NULL) {
					c = *(pt + 1);
				}
				/*
				 * Octal value.
				 */
				else if ('0' <= c && c <= '7') {
					int	nc;

					c -= '0';
					for (nc = 1; nc < 3; nc++) {
						char	cc = *next_char;

						if (!('0' <= cc && cc <= '7'))
							break;
						c = 8 * c + cc - '0';
						next_char++;
					}
				}
				/*
				 * End of line.
				 */
				else if ('\0' == c) {
					if (parse_getline(ph) == EOF)
						break;
					next_char = ph->ph_next_char;
					continue;
				}
				/*
				 * Next character is itself.
				 */
			}
			if (n > 0) {
				*p++ = c;
				n--;
			}
		}
		if ('\0' == c || 0 == n) {
			parse_emit(ph->ph_errlog, ph->ph_logfile, 12015,
			    "%s: malformed token at line %d of %s",
			    ph->ph_program, ph->ph_lineno, ph->ph_cmdname);
			ph->ph_errors++;
			return (NULL);
		}
		*p = '\0';
	} else
		while (*next_char != '\0' && !isspace(*next_char)) {
			if ('\\' == *next_char && '\0' == *(next_char + 1)) {
				/* continuation line */
				next_char = NULL;
				goto next;
			}
			*p++ = *next_char++;
		}
	*p++ = '\0';
	*p = '\0';
	ph->ph_next_char = next_char;
	return (token);
}


void
parse_emit(int where, FILE *log, int msgno, char *msg, ...)
{
	va_list	ap;

#define	MAXLINE 1024+256
	char	buf[MAXLINE];
	char	*format;

	if (catfd)
		format = catgets(catfd, SET, msgno, msg);
	else
		format = msg;

	va_start(ap, msg);
	vsprintf(buf, format, ap);
	va_end(ap);

	if (where & TO_TTY) {
		fprintf(stderr, "%s\n", buf);
	}
	if (where & TO_SYS) {
		sam_syslog(LOG_ERR, "%s", buf);
	}
	if (where & TO_FILE && log != NULL) {
		fprintf(log, "%s\n", buf);
	}
}
