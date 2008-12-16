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
#pragma ident	"$Revision: 1.29 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * readcmd.c - read and parse stager's command file
 */

/*
 * ANSI C headers.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

/*
 * POSIX headers.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>


/*
 * SAM-FS headers.
 */
#include "sam/sam_trace.h"
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/readcfg.h"
#include "aml/stager.h"


/* API headers */
#include "pub/mgmt/error.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/stage.h"
#include "mgmt/config/stager.h"
#include "parser_utils.h"
#include "pub/devstat.h"
#include "pub/mgmt/device.h"
#include "mgmt/util.h"
#include "mgmt/config/media.h"
#include "mgmt/config/archiver.h"
#include "mgmt/private_file_util.h"

/* private functions */
static void setSimpleParam(void);
static void setBufsizeParams(void);
static void setDrivesParams(void);
static void setLogfileParam(void);
static void setfield_err_msg(int code, char *msg);
static void readcfg_err_msg(char *msg, int lineno, char *line);
static void setDirectioParam(void);
static void dirNoBegin(void);
static void dirEndStreams(void);
static void dirStreams(void);
static void procStreams(void);
static int init_static_variables(void);

static void write_stager_stream(FILE *f, stream_cfg_t *ss_cfg);


static char *noEnd = NULL;	/* which end statement is missing */
static char *endStreamsStr = "endstreams";

#include "mgmt/config/stager_setfields.h"

/*
 * Stager's command table.
 */
static DirProc_t commands[] = {
	{ "maxactive",		setSimpleParam,			DP_value },
	{ "maxretries",		setSimpleParam,			DP_value },
	{ "logfile",		setLogfileParam,		DP_value },
	{ "bufsize",		setBufsizeParams,		DP_value },
	{ "drives",		setDrivesParams,		DP_value },
	{ "directio",		setDirectioParam,		DP_value },
	{ "streams",		dirStreams,			DP_set },
	{ "endstreams",		dirNoBegin,			DP_set },
	{ NULL, NULL }
};


/* private parser data */
static stager_cfg_t *stage_cfg;
static sqm_lst_t *lib_list;
static char dirName[TOKEN_SIZE];
static char token[TOKEN_SIZE];
static sqm_lst_t *error_list = NULL;
static char open_error[80];
static boolean_t no_cmd_file = B_FALSE;

/*
 * Read stager.cmd file
 */
int
parse_stager_cmd(
char *cmd_file,		/* cmd file to check */
sqm_lst_t *libraries,	/* list of libraries to check against */
stager_cfg_t **cfg)	/* malloced return value */
{

	int errors;

	Trace(TR_OPRMSG, "parsing stager.cmd %s",
	    cmd_file ? cmd_file : "NULL");

	if (ISNULL(cmd_file, libraries, cfg)) {
		Trace(TR_OPRMSG, "parsing stager.cmd failed: %s", samerrmsg);
		return (-1);
	}

	if (init_static_variables() != 0) {
		Trace(TR_OPRMSG, "parsing stager.cmd failed: %s", samerrmsg);
		return (-1);
	}

	/* get the stager's defaults. */
	if (get_default_stager_cfg(NULL, &stage_cfg) != 0) {
		Trace(TR_OPRMSG, "parsing stager.cmd failed: %s", samerrmsg);
		return (-1);
	}

	*cfg = stage_cfg;
	lib_list = libraries;

	errors = ReadCfg(cmd_file, commands, dirName, token, readcfg_err_msg);
	if (errors != 0) {

		/* The absence of a command file is not an error */
		if (errors == -1) {

			if (no_cmd_file) {
				/*
				 * The absence of a command file
				 * is not an error
				 */
				Trace(TR_OPRMSG, "parsing stager.cmd %s",
				    "no file present");
				return (0);
			}

			free_stager_cfg(*cfg);

			/* other access errors are an error */
			samerrno = SE_CFG_OPEN_FAILED;

			/* open failed for %s: %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_CFG_OPEN_FAILED), cmd_file,
			    open_error);

			Trace(TR_OPRMSG, "parsing stager.cmd failed: %s",
			    samerrmsg);

			return (-1);

		}
		free_stager_cfg(*cfg);

		samerrno = SE_CONFIG_CONTAINED_ERRORS;
		/* %s configuration contains %d error(s) */

		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CONFIG_CONTAINED_ERRORS), cmd_file, errors);

		Trace(TR_OPRMSG, "parsing stager.cmd failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "parsed stager.cmd");
	return (0);
}


/*
 * returns the parsing errors encountered in the most recent parsing.
 */
int
get_stager_parsing_errors(
sqm_lst_t **l)	/* malloced list of parsing_error_t */
{

	int err = 0;

	Trace(TR_DEBUG, "getting stager parsing errors");

	err = dup_parsing_error_list(error_list, l);

	Trace(TR_DEBUG, "get stager parsing errors returning: %d", err);
	return (err);
}


/*
 * initialize private parser data.
 */
static int
init_static_variables(void)
{

	Trace(TR_DEBUG, "initializing static variables for stager parsing");

	if (error_list != NULL) {
		lst_free_deep(error_list);
	}

	error_list = lst_create();
	if (error_list == NULL) {
		Trace(TR_DEBUG, "initializing static variables failed: %s",
		    samerrmsg);
		return (-1);
	}
	no_cmd_file = B_FALSE;

	Trace(TR_DEBUG, "static variables initialized");
	return (0);
}



/*
 * Set simple, one value, parameter from stager's command file.
 */
static void
setSimpleParam(void)
{

	Trace(TR_DEBUG, "setting stager param");
	(void) SetFieldValue(stage_cfg, stage_cfg_table,
	    dirName, token, setfield_err_msg);
	Trace(TR_DEBUG, "set stager param");
}




/*
 *  Set bufsize parameters from stager's command file.
 */
static void
setBufsizeParams(void)
{

	char *p;
	buffer_directive_t *buf;
	uint64_t val;
	node_t *n;
	boolean_t found = B_FALSE;

	Trace(TR_DEBUG, "setting bufsize");
	/*
	 *  Copy media to stager's parameters.
	 */
	if (token != NULL && *token != '\0') {

		if (check_media_type(token) == -1) {
			ReadCfgError(CustMsg(4431));
		}

		/*
		 * if a default value existed for media type use its
		 * structure here.
		 */
		for (n = stage_cfg->stage_buf_list->head; n != NULL;
		    n = n->next) {

			buf = (buffer_directive_t *)n->data;
			if (strcmp(buf->media_type, token) == 0) {
				found = B_TRUE;
				break;
			}
		}

		if (!found) {
			buf = (buffer_directive_t *)mallocer(
			    sizeof (buffer_directive_t));

			if (buf == NULL) {
				ReadCfgError(samerrno);
			}

			memset(buf, 0, sizeof (buffer_directive_t));
			strlcpy(buf->media_type, token,
			    sizeof (buf->media_type));
		}

		/*
		 *  Media value processed.  Next token must be buffer size.
		 */
		if (ReadCfgGetToken() == 0) {
			/* Missing '%s' value */
			free(buf);
			ReadCfgError(CustMsg(14008), dirName);
		}

		/*
		 * Set bufsize.
		 */
		errno = 0;
		val = strtoull(token, &p, 0);
		if (errno != 0 || *p != '\0') {
			/* Invalid '%s' value '%s' */
			free(buf);
			ReadCfgError(CustMsg(14101), dirName, token);
		}


		if (val < 2) {
			free(buf);
			ReadCfgError(CustMsg(14103), dirName, (long long)2);
		} else if (val > 1024) {
			free(buf);
			ReadCfgError(CustMsg(14104), dirName, (long long)1024);
		}

		buf->size = (uint64_t)val;

		if (ReadCfgGetToken() != 0) {
			if (strcmp(token, "lock") == 0) {
				buf->lock = TRUE;
				buf->change_flag |= BD_lock;
			} else {
				/* bufsize option must be 'lock' */
				free(buf);
				ReadCfgError(CustMsg(4521));
			}
		}
		buf->change_flag |= BD_size;

		if (!found) {
			if (lst_append(stage_cfg->stage_buf_list, buf) != 0) {
				free(buf);
				ReadCfgError(CustMsg(samerrno));
			}
		}
	}
	Trace(TR_DEBUG, "set bufsize");
}


/*
 *  Set drives parameters from stager's command file.
 */
static void
setDrivesParams(void)
{

	char *p;
	drive_directive_t *drive;
	int count;
	library_t *lib;
	node_t *n;
	boolean_t found = B_FALSE;

	Trace(TR_DEBUG, "setting drives");

	/*
	 * first find if a default has been set. If so use that struct.
	 */
	for (n = stage_cfg->stage_drive_list->head; n != NULL; n = n->next) {
		drive = (drive_directive_t *)n->data;
		if (strcmp(drive->auto_lib, token) == 0) {
			found = B_TRUE;
			break;
		}
	}

	/*
	 * find Library in lib_list to check number of drives
	 */
	if (find_library_by_family_set(lib_list, token, &lib) != 0) {
		ReadCfgError(CustMsg(4444), token);
	}

	if (!found) {
		drive = (drive_directive_t *)mallocer(
		    sizeof (drive_directive_t));

		if (drive == NULL) {
			ReadCfgError(samerrno);
		}
		memset(drive, 0, sizeof (drive_directive_t));
		(void) strlcpy(drive->auto_lib, token, sizeof (uname_t));
	}

	/*
	 *  Robot name saved in drive; next token must be the drive count.
	 */
	if (ReadCfgGetToken() == 0) {
		if (!found) {
			free(drive);
		}
		ReadCfgError(CustMsg(14008), dirName);
	}

	/*
	 * check the drive count.
	 * Must be less than count in the library. The number of drives
	 * in the library is the number of entries in the family set
	 * list for the library minus one(the robot entry)
	 */
	p = token;
	errno = 0;
	count = strtoll(token, &p, 0);
	if (errno != 0 || count < 0 || p == token) {
		/* Invalid drive count */
		if (!found) {
			free(drive);
		}
		ReadCfgError(CustMsg(14008), dirName);
	}

	if (count > lib->no_of_drives) {
		if (!found) {
			free(drive);
		}
		ReadCfgError(CustMsg(4446), lib->no_of_drives);
	}

	drive->count = count;
	drive->change_flag |= DD_count;
	if (!found) {
		if (lst_append(stage_cfg->stage_drive_list, drive) != 0) {
			if (!found) {
				free(drive);
			}

			ReadCfgError(samerrno);
		}
	}
	Trace(TR_DEBUG, "set drives");
}


/*
 *  Set logfile parameter from stager's command file.
 */
static void
setLogfileParam(void)
{

	Trace(TR_DEBUG, "setting logfile");

	if (token != NULL && *token != '\0') {
		if (verify_file(token, B_TRUE) == B_FALSE) {
			ReadCfgError(CustMsg(19018), "log", token);
		}

		if (strlen(token) > sizeof (stage_cfg->stage_log)-1) {
			ReadCfgError(CustMsg(19019),
			    sizeof (stage_cfg->stage_log)-1);
		}

		(void) strlcpy(stage_cfg->stage_log, token,
		    sizeof (stage_cfg->stage_log));

		stage_cfg->change_flag |= ST_stage_log;
	}
	if (ReadCfgGetToken() == 0) {
		stage_cfg->options |= ST_LOG_DEFAULTS;
	} else {
		stage_cfg->options &= ~(ST_LOG_ALL | ST_LOG_START |
		    ST_LOG_ERROR | ST_LOG_CANCEL | ST_LOG_FINISH);

		while (token != NULL && *token != '\0') {
			if (strcmp(token, ST_START_STR) == 0) {
				stage_cfg->options |= ST_LOG_START;

			} else if (strcmp(token, ST_FINISH_STR) == 0) {
				stage_cfg->options |= ST_LOG_FINISH;

			} else if (strcmp(token, ST_CANCEL_STR) == 0) {
				stage_cfg->options |= ST_LOG_CANCEL;

			} else if (strcmp(token, ST_ERROR_STR) == 0) {
				stage_cfg->options |= ST_LOG_ERROR;

			} else if (strcmp(token, ST_ALL_STR) == 0) {
				stage_cfg->options = ST_LOG_START |
				    ST_LOG_FINISH |
				    ST_LOG_CANCEL |
				    ST_LOG_ERROR;
			} else {
				ReadCfgError(CustMsg(19028), token);
			}
			ReadCfgGetToken();
		}
		stage_cfg->change_flag |= ST_log_events;
	}
	Trace(TR_DEBUG, "set logfile");
}

/*
 *  Set directio parameter from stager's command file.
 */
static void
setDirectioParam(void)
{
	if (token != NULL && *token != '\0') {
		if (strcmp(token, "on") == 0) {
			stage_cfg->options |= ST_DIRECTIO_ON;
			stage_cfg->change_flag |= ST_directio;
		} else if (strcmp(token, "off") == 0) {
			/*
			 * Set the change_flag to indicate directio
			 * was explicitly set to off.
			 */
			stage_cfg->change_flag |= ST_directio;
		} else {
			ReadCfgError(CustMsg(19038));
		}
	} else {
		ReadCfgError(CustMsg(14008), token);
	}
}

/*
 * Handle the streams directive.
 */
static void
dirStreams(void)
{
	static DirProc_t table[] = {
		{ "endstreams", dirEndStreams,	DP_set },
		{ NULL,		procStreams,	DP_other }
	};

	char *msg;

	/*  Set the table to handle the streams directives */
	ReadCfgSetTable(table);
	msg = noEnd;
	noEnd = endStreamsStr;
	if (msg != NULL) {
		/* '%s' missing */
		ReadCfgError(CustMsg(4462), msg);
	}
}

/*
 * No beginning statement for * 'streams' directive.
 */
static void
dirNoBegin(void)
{
	/* No preceding '%s' statement */
	ReadCfgError(CustMsg(4447), "streams");
}


/*
 * Handle the endstreams directive.
 */
static void
dirEndStreams(void)
{
	ReadCfgSetTable(commands);
	noEnd = NULL;
}

static void
procStreams(void) {
	boolean_t params_defined;
	char *end_ptr;

	/*
	 * Streams are only supported for disk in 5.0.
	 * Check that the media type (contained in dirName) is for disk.
	 */
	if (strcmp(dirName, "dk") != 0) {
		/* Invalid media type */
		ReadCfgError(CustMsg(19039), dirName);
		return;
	}

	/* Check to see if a dk_stream has already been parsed */
	if (stage_cfg->dk_stream->change_flag != 0) {
		ReadCfgError(CustMsg(19040), dirName);
	}

	params_defined = B_FALSE;

	while (ReadCfgGetToken() != 0) {
		stream_cfg_t *ss_cfg = stage_cfg->dk_stream;
		params_defined = B_TRUE;

		if (strcmp(token, "-drives") == 0) {
			if (ReadCfgGetToken() == 0) {
				/* Missing value */
				ReadCfgError(CustMsg(14113), "-drives");
			}

			errno = 0;
			ss_cfg->drives =
			    strtoll(token, &end_ptr, 10);

			if (errno != 0 || *end_ptr != '\0' ||
			    ss_cfg->drives < 0 || ss_cfg->drives > INT_MAX) {

				/* Invalid '%s' value '%s' */
				ReadCfgError(CustMsg(14101), "-drives",
				    token);
			}
			ss_cfg->change_flag |= ST_SS_DRIVES;

		} else if (strcmp(token, "-maxsize") == 0) {
			uint64_t value;

			if (ReadCfgGetToken() == 0) {

				/* Missing %s value */
				ReadCfgError(CustMsg(14113), "-maxsize");
			}
			if (StrToFsize(token, &value) == -1) {

				/* Invalid '%s' value ('%s') */
				ReadCfgError(CustMsg(14101), "-maxsize",
				    token);
			}
			ss_cfg->max_size = value;
			ss_cfg->change_flag |= ST_SS_MAX_SIZE;

		} else if (strcmp(token, "-maxcount") == 0) {
			if (ReadCfgGetToken() == 0) {

				/* Missing value */
				ReadCfgError(CustMsg(14113), "-maxcount");
			}
			errno = 0;
			ss_cfg->max_count = strtoll(token, &end_ptr, 0);
			if (errno != 0 || *end_ptr != '\0' ||
			    ss_cfg->max_count < 0 ||
			    ss_cfg->max_count > INT_MAX) {

				/* Invalid '%s' value '%s' */
				ReadCfgError(CustMsg(14101),
				    "-maxcount", token);
			}
			ss_cfg->change_flag |= ST_SS_MAX_COUNT;
		} else {
			/* %s is not a recognized directive name */
			ReadCfgError(CustMsg(14101), token);
		}

	}
	if (!params_defined) {
		/* No stream parameters were defined. */
		ReadCfgError(CustMsg(19041));
	}
}



/*
 *	Error handler for SetField function.
 */
static void
setfield_err_msg(
int code,	/* error code */
char *msg)	/* error message */
{

	ReadCfgError(CustMsg(code), msg);
}


/*
 *	Error handler for ReadCfg function.
 */
static void
readcfg_err_msg(
char *msg,	/* error message */
int lineno,	/* line number of input line */
char *line)	/* input line(if any) containing error */
{

	parsing_error_t *err;

	/*
	 *  ReadCfg function sends every line here.
	 *  If msg string is NULL its not an error.
	 *  ReadCfg function also issues messages before and after
	 *  processing the configuration file.  If line is NULL.
	 */
	if (line != NULL) {
		if (msg != NULL) {
			/*
			 * Error message.
			 */
			Trace(TR_DEBUG, "stager.cmd line:%d: %s",
			    lineno, line);

			Trace(TR_DEBUG, "stager.cmd error: %s", msg);

			err = (parsing_error_t *)mallocer(
			    sizeof (parsing_error_t));
			strlcpy(err->input, line, sizeof (err->input));
			strlcpy(err->msg, msg, sizeof (err->msg));
			err->line_num = lineno;
			err->error_type = ERROR;
			err->severity = 1;
			lst_append(error_list, err);
		}
	} else if (lineno <= 0) {
		/*
		 * Missing command file is not an error.
		 */
		if (errno == ENOENT) {
			Trace(TR_DEBUG,
			    "no stager.cmd file found using defaults");

			no_cmd_file = B_TRUE;
		} else {
			no_cmd_file = B_FALSE;
			Trace(TR_DEBUG, "file open error: %s", msg);
			snprintf(open_error, sizeof (open_error), msg);
		}
	}
}


/*
 * write the stager_cfg struct to location.
 */
int
write_stager_cmd(
const char *location,	/* location at which to write */
const stager_cfg_t *s)	/* cfg to write */
{

	FILE *f = NULL;
	int fd;
	time_t the_time;
	node_t *node;

	Trace(TR_DEBUG, "writing stager.cmd %s", Str(location));

	if ((fd = open(location, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}
	if (f == NULL) {
		char err_msg[STR_FROM_ERRNO_BUF_SIZE];
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_msg, sizeof (err_msg));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), location, err_msg);
		Trace(TR_DEBUG, "writing stager.cmd failed: %s", samerrmsg);
		return (-1);
	}

	fprintf(f, "#\n#\tstager.cmd\n");
	fprintf(f, "#\n");
	the_time = time(0);
	fprintf(f, "#\tGenerated by config api %s#", ctime(&the_time));
	if (*s->stage_log != '\0' && s->change_flag & ST_stage_log) {
		fprintf(f, "\nlogfile = %s", s->stage_log);
		if (s->change_flag & ST_log_events) {

			if (s->options & ST_LOG_ALL) {
				fprintf(f, " %s", ST_ALL_STR);
			}
			if (s->options & ST_LOG_START) {
				fprintf(f, " %s", ST_START_STR);
			}
			if (s->options & ST_LOG_ERROR) {
				fprintf(f, " %s", ST_ERROR_STR);
			}
			if (s->options & ST_LOG_CANCEL) {
				fprintf(f, " %s", ST_CANCEL_STR);
			}
			if (s->options & ST_LOG_FINISH) {
				fprintf(f, " %s", ST_FINISH_STR);
			}
		}
	}

	if (s->max_active != int_reset && s->change_flag & ST_max_active) {
		fprintf(f, "\nmaxactive = %d", s->max_active);
	}

	if (s->max_retries != int_reset && s->change_flag & ST_max_retries) {
		fprintf(f, "\nmaxretries = %d", s->max_retries);
	}

	if (s->change_flag && ST_directio) {
		if (s->options & ST_DIRECTIO_ON) {
			fprintf(f, "\ndirectio = on");
		} else {
			fprintf(f, "\ndirectio = off");
		}
	}

	if (s->stage_drive_list != NULL && s->stage_drive_list->length != 0) {
		drive_directive_t *d;
		for (node = s->stage_drive_list->head;
		    node != NULL; node = node->next) {

			d = (drive_directive_t *)node->data;
			if (d->count != int_reset &&
			    d->change_flag & DD_count) {

				fprintf(f, "\ndrives = %s %d", d->auto_lib,
				    d->count);
			}
		}
	}

	if (s->stage_buf_list != NULL && s->stage_buf_list->length != 0) {
		buffer_directive_t *b;
		for (node = s->stage_buf_list->head;
		    node != NULL; node = node->next) {
			b = (buffer_directive_t *)node->data;
			// if lock is set
			if (b->change_flag & BD_lock && b->lock) {
				// if buf size is set as well
				if (b->size != fsize_reset &&
				    b->change_flag & BD_size) {
					fprintf(f, "\nbufsize = %s %d lock",
					    b->media_type, (int)b->size);
				} else {
					fprintf(f, "\nbufsize = %s %d lock",
					    b->media_type,
					    DEFAULT_AR_BUFSIZE);
				}
			// if lock is not set but size is set
			} else if (b->size != fsize_reset &&
			    b->change_flag & BD_size) {
				fprintf(f, "\nbufsize = %s %d",
				    b->media_type, (int)b->size);
			}
			// if they both are not set, do nothing
		}
	}

	write_stager_stream(f, s->dk_stream);

	fprintf(f, "\n");
	fflush(f);

	fclose(f);

	Trace(TR_DEBUG, "wrote stager.cmd");
	return (0);
}

static void
write_stager_stream(FILE *f, stream_cfg_t *ss_cfg) {

	if (f == NULL || ss_cfg == NULL || *ss_cfg->media == '\0' ||
	    ss_cfg->change_flag == 0) {
		return;
	}

	fprintf(f, "\nstreams\n%s", ss_cfg->media);

	if (ss_cfg->change_flag & ST_SS_DRIVES &&
	    ss_cfg->drives != 0) {
		fprintf(f, " -drives %d", ss_cfg->drives);
	}

	if (ss_cfg->change_flag & ST_SS_MAX_COUNT &&
	    ss_cfg->max_count != 0) {
		fprintf(f, "-maxcount %d", ss_cfg->max_count);
	}

	if (ss_cfg->change_flag & ST_SS_MAX_SIZE &&
	    ss_cfg->max_size != 0) {
		char fsizestr[FSIZE_STR_LEN];
		fprintf(f, "-maxsize %s",  fsize_to_str(ss_cfg->max_size,
		    fsizestr, FSIZE_STR_LEN));
	}

	fprintf(f, "\nendstreams\n");
}


/*
 * the following methods are for a nowrite verify.  That is they support
 * verification of the stager.cmd file without the need to write to a tmp file.
 */
int
check_bufsize(
const buffer_directive_t *b)
{

	Trace(TR_DEBUG, "checking bufsize");

	if (ISNULL(b)) {
		Trace(TR_DEBUG, "checking bufsize failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (check_media_type(b->media_type) == -1) {
		samerrno = SE_INVALID_MEDIA_TYPE;
		/* %s is not a valid media type */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_MEDIA_TYPE), b->media_type);
		Trace(TR_DEBUG, "checking bufsize failed: %s",
		    samerrmsg);
		return (-1);
	}
	if (b->size == int_reset) {
		return (0);
	}
	if (b->size < 2) {
		samerrno = SE_FSIZE_TOO_SMALL;
		/* %s value must be greater than %lld */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FSIZE_TOO_SMALL), "bufsize",
		    (long long)2);

		Trace(TR_DEBUG, "checking bufsize failed: %s",
		    samerrmsg);

		return (-1);

	} else if (b->size > 1024) {
		samerrno = SE_FSIZE_TOO_LARGE;

		/* %s value must be less than %lld */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FSIZE_TOO_LARGE),
		    "bufsize", (long long)32);

		Trace(TR_DEBUG, "checking bufsize failed: %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "checked bufsize");
	return (0);
}


int
check_stage_drives(
const drive_directive_t *d,	/* drive directive to check */
sqm_lst_t *libs)			/* mcf to check against */
{

	library_t *lib;

	Trace(TR_DEBUG, "checking stage drives");
	if (ISNULL(d, libs)) {
		Trace(TR_DEBUG, "checking stage drives failed: %s",
		    samerrmsg);
		return (-1);
	}

	/*
	 * if the count is being reset to default this entry will no longer
	 * even exist in the cfg so don't check the libs list in that case.
	 */
	if (d->count == int_reset || !(d->change_flag & DD_count)) {
		Trace(TR_DEBUG, "no stage drives require checking");
		return (0);
	}

	/*
	 * find Library and check number of drives
	 */
	if (find_library_by_family_set(libs, d->auto_lib, &lib) != 0) {
		Trace(TR_DEBUG, "checking stage drives failed: %s", samerrmsg);
		return (-1);
	}

	/*
	 * check the drive count.
	 * Must be less than count in the library.
	 */
	if (d->count < 0) {
		/* %d is not a valid drive count for %s */
		samerrno = SE_INVALID_DRIVE_COUNT;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DRIVE_COUNT), d->count,
		    d->auto_lib);

		Trace(TR_DEBUG, "checking stage drives failed: %s",
		    samerrmsg);

		return (-1);
	}

	if (d->count > lib->no_of_drives) {
		samerrno = SE_INVALID_DRIVE_COUNT;
		/* %d is not a valid drive count for %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_DRIVE_COUNT), d->count,
		    d->auto_lib);

		Trace(TR_DEBUG, "checking stage drives failed: %s",
		    samerrmsg);

		return (-1);
	}

	Trace(TR_DEBUG, "checked stage drives");
	return (0);
}


/*
 * verify that max retries is valid.
 */
int
check_maxretries(
const stager_cfg_t *s)
{

	Trace(TR_DEBUG, "checking maxretries %d", s->max_retries);

	if (ISNULL(s)) {
		Trace(TR_DEBUG, "checking maxretries failed: %s", samerrmsg);
		return (-1);
	}

	/* don't check it, if it will not get written into the file */
	if (s->max_retries == int_reset ||
	    !(s->change_flag & ST_max_retries)) {

		Trace(TR_DEBUG, "checking maxretries exit");
		return (0);
	}

	if (check_field_value(s, stage_cfg_table, "maxretries",
	    samerrmsg, 256) != 0) {

		samerrno = SE_INVALID_VALUE;
		/* %s %d is invalid */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_VALUE), "max_retries",
		    s->max_retries);

		Trace(TR_DEBUG, "checking maxretries failed: %s", samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "checked maxretries");
	return (0);
}


/*
 * verify that max active is valid.
 */
int
check_maxactive(
const stager_cfg_t *s)
{

	Trace(TR_DEBUG, "checking maxactive %d", s->max_active);
	if (ISNULL(s)) {
		Trace(TR_DEBUG, "checking maxactive() failed: %s",
		    samerrmsg);
		return (-1);
	}

	/* don't check it if it will not get written into the file */
	if (s->max_active == int_reset || !(s->change_flag & ST_max_active)) {
		Trace(TR_DEBUG, "checked maxactive");
		return (0);
	}

	if (check_field_value(s, stage_cfg_table, "maxactive",
	    samerrmsg, 256) != 0) {

		samerrno = SE_INVALID_VALUE;
		/* %s %d is invalid */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_VALUE), "max_active",
		    s->max_active);
		Trace(TR_DEBUG, "checking maxactive failed: %s",
		    samerrmsg);
		return (-1);
	}
	Trace(TR_DEBUG, "checked maxactive");
	return (0);
}


/*
 * check that logfile exists or can be created.
 */
int
check_logfile(
const char *logfile)
{

	Trace(TR_DEBUG, "checking logfile %s", Str(logfile));
	if (ISNULL(logfile)) {
		Trace(TR_DEBUG, "checking logfile failed: %s",
		    samerrmsg);
		return (-1);
	}

	if (*logfile == '\0') {
		Trace(TR_DEBUG, "logfile not set");
		return (0);
	}

	if (verify_file(logfile, B_TRUE) != B_TRUE) {
		samerrno = SE_LOGFILE_CANNOT_BE_CREATED;

		/* Logfile %s can not be created */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_LOGFILE_CANNOT_BE_CREATED), logfile);

		Trace(TR_DEBUG, "checking logfile failed: %s",
		    samerrmsg);

		return (-1);
	}
	Trace(TR_DEBUG, "checked logfile %s", Str(logfile));
	return (0);
}


int
check_stager_stream(stream_cfg_t *ss_cfg) {


	/* Not having a stream config is OK */
	if (ss_cfg == NULL || ss_cfg->change_flag == 0) {
		return (0);
	}

	if (strcmp(ss_cfg->media, "dk") != 0) {
		setsamerr(SE_STREAMS_ONLY_DISK);
		return (-1);
	}
	if (ss_cfg->change_flag & ST_SS_DRIVES && ss_cfg->drives < 0) {
		samerrno = SE_INVALID_VALUE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_VALUE), "-drives",
		    ss_cfg->drives);
		return (-1);
	}
	if (ss_cfg->change_flag & ST_SS_MAX_SIZE && ss_cfg->max_size < 0) {
		samerrno = SE_INVALID_FSIZE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_FSIZE), ss_cfg->max_size);
		return (-1);
	}
	if (ss_cfg->change_flag & ST_SS_MAX_COUNT && ss_cfg->max_count < 0) {
		samerrno = SE_INVALID_VALUE;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_VALUE), "-maxcount",
		    ss_cfg->max_count);
		return (-1);
	}

	return (0);
}
