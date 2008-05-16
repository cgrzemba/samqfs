/*
 * readcfg.c - Read configuration file.
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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

/*
 * The readcfg.c module provides a common mechanism for reading configuration
 * files.  The configuration files consist of ASCII lines. Each of these
 * lines, called a directive, consists of one or more fields (tokens)
 * separated by white space.
 *
 * ReadCfg() is called with the name of the configuration file to be
 * read and processed.  The file is opened, and read to EOF processing
 * the directives according to arguments passed by the caller.
 *
 * Blank lines are skipped, and the remainder of a line after a '#'
 * character is ignored.  Lines may be continued by using '\' as the last
 * character on the line.
 *
 * Tokens are:
 *   A sequence of characters terminated by white-space or the '=' character
 *   The '=' character.
 *   A sequence of characters enclosed by '"'.
 *
 * The first token of each line is considered the directive name.  Three
 * forms of processing for directives are provided directly in ReadCfg():
 * 1.  When the name is followed by the token '=', the name specifies
 *     the name of a configuration parameter.  The token after the '=' is
 *     the specification for the parameter.
 * 2.  When a dirname is not followed by a '=', The parameter is turned
 *     on (set 'TRUE').
 * 3.  For directives that do not follow the above convention, ReadCfg()
 *     calls a function specific for that directive.
 *
 * The ReadCfg() caller provides a table that defines the processing for
 * directives.  The caller defined processing functions have no arguments,
 * and do not return a value to ReadCfg().  The caller also provides
 * storage for the directive name (dirname), and each token read (token).
 * When the caller's function is called by ReadCfg(), dirname is set to
 * the directive name, and token is set as follows for the cases above:
 * 1.  token = the token following the '='
 * 2.  token = ""
 * 3.  token = next token on the line
 *
 * For processing of lines that do not follow any of the above conventions,
 * the table terminator may provide a function to be called if a directive
 * name is not found in the table.
 *
 *
 * The caller of this module may provide a message handling function in
 * the ReadCfg() call.  If not provided, an internal message function
 * is used that writes the messages to stdout.
 *
 * The message function will be passed three arguments:
 * 1.  msg - The character string of the message.
 * 2.  lineno - The line number for the line in the configuration file
 *     being processed.
 * 3.  line - The text of the line.
 *
 * This function will be called by the module:
 *
 * For informational messages - msg == NULL:
 * 1.  At the start reading the file.
 *	   line = "Reading ...", lineno = 0, msg = NULL
 * 2.  Each line is read.
 *     line = line read, lineno != 0, msg = NULL
 *
 * For error messages - msg == error message:
 * 3.  When an error message is composed for a line.
 *     line = line read, lineno != 0
 * 4.  If the configuration file cannot be opened.
 *     line = NULL, lineno = -1
 * 5.  If the configuration file file is empty.
 *     line = NULL, lineno = 0
 * 6.  At the end of reading the configuration file if errors were
 *     encountered.   line = NULL, lineno != 0
 *
 * Four additional functions are provided in this module.
 * 1.  ReadCfgError() processes an error message for a caller's function.
 * 2.  ReadCfgToken() provides the next token from the directive line.
 * 3.  ReadCfgLookupDirname() looks up a directive and processes it.
 * 4.  ReadCfgSetTable() can be called to change the directive processing
 *     table.  This is needed for the archiver.cmd file.
 *
 * See the test section of this module for examples of usage.
 *
 */

#pragma ident "$Revision: 1.20 $"

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "sam/lint.h"

/* Private functions. */
static int getLine(void);
static void defaultMsgFunc(char *msg, int lineno, char *line);

/* Private data. */
static void (*msgFunc)(char *msg, int lineno, char *line);
static DirProc_t *dirProcTable;
static FILE *cfg_st;			/* Configuration file stream */
static char *cfg_name;			/* Configuration file name */
static char *dirname;			/* Where to store command name */
static char *next_char;			/* Next character in input line */
static char *token;			 /* Token from input line */
static char line_buf[LINE_LENGTH];	/* Input line buffer */
static char *line = line_buf;
static char msg_buf[LINE_LENGTH];	/* Message composition buffer */
static int lineno;			/* Current line number */
static jmp_buf errorReturn;		/* Set during line processing */
static int errors;			/* Error counter */


/*
 * Read configuration file.
 * Returns error count.  If -1, configuration file
 * could not be opened.
 */
int
ReadCfg(
	char *FileName,
	DirProc_t *Table,
	char *Dirname,		/* Where to store directive name */
	char *Token,		/* Where to store token */
	void (*MsgFunc)		/* Message function processor */
	(char *msg, int lineno, char *line))
{
	/*
	 * Initialize module.
	 */
	line = NULL;
	lineno = -1;
	errors = 0;
	cfg_name = FileName;
	dirProcTable = Table;
	token = Token;
	dirname = Dirname;
	if (MsgFunc != NULL) {
		msgFunc = MsgFunc;
	} else {
		msgFunc = defaultMsgFunc;
	}

	/*
	 * Open the configuration file.
	 */
	cfg_st = fopen(cfg_name, "r");
	if (NULL == cfg_st) {
		char err_msg[STR_FROM_ERRNO_BUF_SIZE];

		/* Cannot open configuration file "%s": %s */
		ReadCfgError(14010, cfg_name,
		    StrFromErrno(errno, err_msg, sizeof (err_msg)));
		return (-1);
	}

	/*
	 * Read the file.
	 */
	line = line_buf;
	lineno = 0;
	(void) setjmp(errorReturn);
	while (getLine() != EOF) {
		if (ReadCfgGetToken() == 0) {
			continue;
		}
		strncpy(dirname, token, TOKEN_SIZE-1);
		ReadCfgLookupDirname(dirname, dirProcTable);
	}
	(void) fclose(cfg_st);
	cfg_st = NULL;
	line = NULL;

	/*
	 * Report errors.
	 */
	errno = 0;
	if (lineno == 0) {
		/* Configuration file "%s" is empty. */
		ReadCfgError(14011, cfg_name);
		return (1);
	}
	if (errors != 0) {
		if (errors != 1) {
			/* %d errors in configuration file %s */
			ReadCfgError(14000, errors, cfg_name);
		} else {
			/* 1 error in configuration file %s */
			ReadCfgError(14001, cfg_name);
		}
	}
	return (errors);
}


/*
 * Process error message.
 *
 * Note:  Does not return to caller while reading the configuration file.
 */
void
ReadCfgError(
	int MsgNum,
	...)
{
	va_list args;
	char	*msg;

	msg = msg_buf;
	if (cfg_st != NULL) {
		errors++;
		/* Error in line %d */
		sprintf(msg, GetCustMsg(14004), lineno);
		strcat(msg, ": ");
		msg += strlen(msg);
	}

	va_start(args, MsgNum);
	if (MsgNum != 0) {
		vsprintf(msg, GetCustMsg(MsgNum), args);
	} else {
		char *fmt;

		fmt = va_arg(args, char *);
		vsprintf(msg, fmt, args);
	}
	msg += strlen(msg);
	*msg = '\0';

	msgFunc(msg_buf, lineno, line);
	if (cfg_st != NULL) {
		for (;;) {
			/*
			 * Skip to end of continuation lines.
			 */
			int		n;

			n = strlen(line);
			if (n < 1 || line[n-1] != '\\') {
				break;
			}
			(void) getLine();
		}
		longjmp(errorReturn, 1);
	}
}


/*
 * Provides input tokens.
 * Tokens are:
 *   A sequence of characters terminated by white-space or a separator
 *   character - one of =+-*&|^~.
 *   A sequence of characters enclosed by '"'.
 */
int
ReadCfgGetToken(void)
{
	char c;
	char *p;

	p = token;

next:
	*p = '\0';
	if (NULL == next_char && getLine() == EOF) {
		return (0);
	}

	/* Skip white space. */
	while (isspace((c = *next_char))) {
		next_char++;
	}
	if ('\\' == c && '\0' == *(next_char + 1)) {
		/* continuation line */
		next_char = NULL;
		goto next;
	}

	/* End of line or comment. */
	if ('\0' == c || '#' == c) {
		return (0);
	}

	if (p == token && '"' == c) {	/* quoted string */
		int	n;

		n = TOKEN_SIZE - 1;
		next_char++;
		while ((c = *next_char++) != '"') {
			if (c == '\0') {
				/* Unterminated string */
				ReadCfgError(14002);
				/* NOTREACHED */
			}

			/*
			 * Character escape sequence.
			 */
			if ('\\' == c) {
				static char transtab[] = "b\bf\fn\nr\rt\t";
				char	*pt;

				c = *next_char++;
				/*
				 * Next character in escape sequence table.
				 */
				if (islower(c) &&
				    (pt = strchr(transtab, c)) != NULL) {
					c = *(pt+1);
				} else if ('0' <= c && c <= '7') {
					/*
					 * Octal value.
					 */
					int nc;

					c -= '0';
					for (nc = 1; nc < 3; nc++) {
						char cc = *next_char;

						if (!('0' <= cc && cc <= '7')) {
							break;
						}
						c = 8 * c + cc - '0';
						next_char++;
					}
				} else if ('\0' == c) {
					/*
					 * End of line.
					 */
					if (getLine() == EOF) {
						break;
					}
					continue;
				}
				/*
				 * Next character is itself.
				 */
			}
			if (n > 0) {
				*p++ = c;
				n--;
			} else {
				ReadCfgError(14003);  /* String is too long */
				/* NOTREACHED */
			}
		}
		*p = '\0';
	} else while (*next_char != '\0' && *next_char != '#') {
		if ('\\' == *next_char && '\0' == *(next_char + 1)) {
			/* continuation line */
			next_char = NULL;
			goto next;
		}

		/*
		 * Check for the '=' separator.  If it's the first character,
		 * return it as the token.
		 * Otherwise, it terminates the token.
		 */
		if (*next_char == '=') {
			if (p == token) {
				*p++ = *next_char++;
			}
			break;
		}

		/* Collect until whitespace is reached. */
		if (isspace(*next_char)) {
			break;
		}
		*p++ = *next_char++;
	}
	*p = '\0';
	return (1);
}


/*
 * Lookup the directive name.
 */
void
ReadCfgLookupDirname(
	char *dirname,
	DirProc_t *dirProcTable)
{
	DirProc_t *table;

	/*
	 * Search table for dirname match.
	 */
	for (table = dirProcTable; /* Terminated inside */; table++) {
		if (table->DpName == NULL) {
			/*
			 * End of table.  Process 'not found' entry if present.
			 */
			if (table->DpFunc != NULL) {
				break;
			}
			if (table->DpType != 0) {
				ReadCfgError(table->DpType, dirname);
			}
			/* \"%s\" is not a recognized directive name */
			ReadCfgError(14005, dirname);
			/* NOTREACHED */
		}
		if (strcmp(table->DpName, dirname) == 0) {
			break;
		}
	}

	/*
	 * Check argument.
	 */
	switch (table->DpType) {
	case DP_set:
		/*
		 * There must be no value field.
		 */
		if (ReadCfgGetToken() != 0) {
			/* No value allowed for %s */
			ReadCfgError(14006, dirname);
			/* NOTREACHED */
		}
		break;

	case DP_value:
		/*
		 * There must be a value field in the form '= value'.
		 */
		if (ReadCfgGetToken() == 0 || strcmp(token, "=") != 0) {
			/* = missing */
			ReadCfgError(14007);
			/* NOTREACHED */
		}
		if (ReadCfgGetToken() == 0) {
			/* value missing */
			if (table->DpMsgNum == 0) {
				ReadCfgError(14008, dirname);
				/* NOTREACHED */
			} else {
				ReadCfgError(table->DpMsgNum, dirname);
				/* NOTREACHED */
			}
		}
		break;

	case DP_setfield:
		/*
		 * We don't know if there is a required value field.
		 * But, if there is, it must be in the form '= value'.
		 */
		if (ReadCfgGetToken() != 0) {
			if (strcmp(token, "=") != 0) {
				/* = missing */
				ReadCfgError(14007);
				/* NOTREACHED */
			}
			if (ReadCfgGetToken() == 0) {
				if (table->DpMsgNum == 0) {
					ReadCfgError(14008, dirname);
					/* NOTREACHED */
				} else {
					ReadCfgError(table->DpMsgNum, dirname);
					/* NOTREACHED */
				}
			}
		}
		break;

	case DP_param:
		/*
		 * There must be a value field in the form 'value'.
		 */
		if (ReadCfgGetToken() == 0) {
			if (table->DpMsgNum == 0) {
				ReadCfgError(14008, dirname);
				/* NOTREACHED */
			} else {
				ReadCfgError(table->DpMsgNum, dirname);
				/* NOTREACHED */
			}
		}
		break;

	case DP_other:
		break;

	default:
		/*
		 * We should never get here do to user action.
		 * This is a programming error.
		 */
		ReadCfgError(0, "ASSERT: directive type %d problem for %s",
		    table->DpType, dirname);
		/* NOTREACHED */
		break;
	}

	/*
	 * Process directive.
	 */
	table->DpFunc();
}


/*
 * Set the directive processing table.
 */
void
ReadCfgSetTable(
	DirProc_t *Table)
{
	dirProcTable = Table;
}


/*
 * Process configuration file processing message.
 */
static void
defaultMsgFunc(
	char *msg,
	int lineno,
	char *line)
{
	/*
	 * Only error messages to stderr.
	 */
	if (msg != NULL) {
		if (line != NULL) {
			fprintf(stderr, "%d: %s\n", lineno, line);
		}
		fprintf(stderr, "%s\n", msg);
	}
}


/*
 * Get line from source input file.
 * Returns EOF if end of file, else 0.
 */
static int
getLine(void)
{
	if (fgets(line, sizeof (line_buf)-1, cfg_st) == NULL) {
		return (EOF);
	}
	if (lineno == 0) {
		/* Reading '%s'. */
		sprintf(msg_buf, GetCustMsg(14012), cfg_name);
		msgFunc(NULL, 0, msg_buf);
	}
	line[strlen(line)-1] = '\0';
	next_char = line;
	lineno++;
	msgFunc(NULL, lineno, line);
	return (0);
}

#if defined(TEST)

/* ANSI C headers. */
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#if defined(lint)
#undef alarm
#undef execl
#undef execv
#undef sleep
#undef unlink
#endif /* defined(lint) */
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "sam/lint.h"

/* Private functions. */
static void other(void);
static void setParam(void);
static void valParam(void);
static void strParam(void);
static void ourmsgFunc(char *msg, int lineno, char *line);

/* Private data. */
static DirProc_t ourdirProcTable[] = {
	{ "setparam",		setParam,		DP_set },
	{ "valparam",		valParam,		DP_value },
	{ "strparam",		strParam,		DP_value },
	{ NULL, other, DP_other }
};

static char ourdirname[TOKEN_SIZE];
static char ourtoken[TOKEN_SIZE];
static boolean_t setparam = FALSE;
static char *strparam = "";
static int valparam = -1;

/*ARGSUSED0*/
int
main(
	int argc,
	char *argv[])
{
	FILE	*st;
	char	*FileName = "file.cfg";

	printf("** 1.  Fail with no config file.\n");
	(void) ReadCfg("none", ourdirProcTable, ourdirname, ourtoken, NULL);

	printf("\n** 2.  Fail with an empty config file.\n");
	if ((st = fopen(FileName, "w")) == NULL) {
		fprintf(stderr, "Cannot create cfg_file\n");
		exit(EXIT_FAILURE);
	}
	(void) ReadCfg(FileName, ourdirProcTable, ourdirname, ourtoken, NULL);

	fputs("# This is a comment\n", st);
	fputs("\n", st);
	fputs("badname       # 'badname' is not a recognized directive name\n",
	    st);
	fputs("setparam = 1  # No value allowed for 'setparam'\n", st);
	fputs("setparam 1    # No value allowed for 'setparam'\n", st);
	fputs("setparam\n", st);
	fputs("valparam      # '=' missing\n", st);
	fputs("valparam =    # Missing 'valparam' value\n", st);
	fputs("valparam=123\n", st);
	fputs("valparam = 1.0G\n", st);
	fputs("valparam = 23G # Value for 'valparam' too large. "
	    "Max = 2147483647\n", st);
	fputs("valpa\\\n", st);
	fputs("ram = 1.0M\n", st);
	fputs("strparam = \"String parameter\"\n", st);
	fputs("other\n", st);
	fputs("\n", st);
	fclose(st);

	printf("\n** 3.  Process with default message function.\n");
	(void) ReadCfg(FileName, ourdirProcTable, ourdirname, ourtoken, NULL);

	printf("\n** 4.  Process with example message function.\n");
	(void) ReadCfg(FileName, ourdirProcTable, ourdirname,
	    ourtoken, ourmsgFunc);

	printf("\n  Parameters set:\n");
	printf("setparam = %s\n", (setparam) ? "on" : "off");
	printf("strparam = \"%s\"\n", strparam);
	printf("valparam = %d\n", valparam);
	unlink(FileName);
	return (EXIT_SUCCESS);
}


/*
 * Set a parameter.
 */
static void
setParam(void)
{
	setparam = TRUE;
}


/*
 * Set string parameter value.
 */
static void
strParam(void)
{
	strparam = strdup(ourtoken);
}


/*
 * Set parameter value.
 */
static void
valParam(void)
{
	boolean_t neg;
	uint64_t val;

	if (strcmp(ourtoken, "-") == 0) {
		if (ReadCfgGetToken() == 0) {
			ReadCfgError(0, "No value specified for %s",
			    ourdirname);
			/* NOTREACHED */
		}
		neg = TRUE;
	} else {
		neg = FALSE;
	}
	if (StrToFsize(ourtoken, &val) != 0) {
		ReadCfgError(0, "Conversion of %s failed");
		/* NOTREACHED */
	}
	if (val > INT_MAX) {
		ReadCfgError(0, "Value for \"%s\" too large.  Max = %d",
		    ourdirname, INT_MAX);
		/* NOTREACHED */
	}
	if (!neg) {
		valparam = (int)val;
	} else {
		valparam = -(int)val;
	}
}


/*
 * Process other directive.
 */
static void
other(void)
{
	if (strcmp(ourdirname, "other") != 0) {
		ReadCfgError(14005, ourdirname);
		/* NOTREACHED */
	}

}


/*
 *  Message handler for ReadCfg module.
 */
static void
ourmsgFunc(
	char *msg,
	int lineno,
	char *line)
{
	if (line != NULL) {
		/*
		 * While reading the file.
		 */
		static boolean_t line_printed = FALSE;

		if (msg != NULL) {
			if (!line_printed) {
				printf("%d: %s\n", lineno, line);
			}
			fprintf(stderr, " *** %s\n", msg);
		} else {
			/*
			 * Informational messages.
			 */
			if (lineno == 0) {
				printf("%s\n", line);
			} else {
				printf("%d: %s\n", lineno, line);
				line_printed = TRUE;
				return;
			}
		}
		line_printed = FALSE;
	} else {
		/*
		 * Other error messages.
		 */
		printf("%s\n", msg);
	}
}

#endif /* !defined(TEST) */
