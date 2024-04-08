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

#pragma ident   "$Revision: 1.17 $"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#define DEC_INIT
#include "sam/types.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include "tapealert_text.h"

#define	SENDTRAP	"/etc/opt/SUNWsamfs/scripts/sendtrap"
#define	COMPONENT	"TapeAlert"
#define	CRITICAL	"Critical"
#define	WARNING		"Warning"
#define	INFORMATION	"Information"
#define	UNKNOWNHOST	"Unknown"
#define	NUMARGS		12

/*
 * tapealert_trap.c
 *
 * The sysevent information received from syseventd for tapealerts, contains
 * vendor, product, revision, faulttime, equipment ordinal, device name,
 * type, vsn, flags_len and flags. To send a trap we can use these and
 * reconstruct a more user-friendly description of the alert, complete with
 * system_id, alert_id, appmsg, probable cause of the alert, errot type etc.
 *
 * tapealert_trap maps the flags sent by sysevent of class 'Device' and
 * subclass 'TapeAlert' to an appropriate user-friendly application message
 * and invokes the sendtrap script to send SNMP trap. It uses tapealert_text()
 * to get the textual description of the tapealert flag
 *
 * Input arguments to the program are:
 *
 * 1) vendor	- vendor (SCSI INQUIRY)
 * 2) product	- product id
 * 3) rev	- revision
 * 4) faulttime - time at which the event occured (in time_t format)
 * 5) eq	- equipment ordinal
 * 6) name	- tape/device path
 * 7) type	- the scsi inquiry peripheral device type
 * 8) vsn	- vsn
 * 9) flags_len - tapealert flags number
 * 10) flags	- tapealert flags 64-1
 * 11) version  - standard inquiry version
 */

typedef struct sysevent_tapealert_data {
	uchar_t	vendor_id[8+1];
	uchar_t	product_id[16 +1];
	uchar_t	revision[4+1];
	time_t	tm;
	equ_t	eq;
	upath_t	device_path;
	int	scsi_type;
	uname_t	vsn;
	int	flags_len;
	uint64_t	flags;
	uchar_t version;

} tapealert_data_t;

static int get_error_type(char *str_errortype);
static char *get_system_id();
static tapealert_data_t *format_input(int argc, char *argv[]);
static void convert_to_oid(char *str);
static void strip_quotes(char *str);

int
main(int argc, char *argv[])
{

	tapealert_data_t *tapealert_data;
	tapealert_text_t text;
	char *system_id, *command;	/* system id, sendtrap cmd and arg */
	char alert_id[32];		/* concat of manual name and code */
	size_t command_len;
	int i;

	if ((tapealert_data = format_input(argc, argv)) != NULL) {

		/* get system identifier */
		system_id = get_system_id();

		/*
		 * get the mapping between flags and application message
		 * in a tapealert_text_t structure
		 */
		tapealert_text(tapealert_data->version,
		    tapealert_data->scsi_type,
		    tapealert_data->flags_len,
		    tapealert_data->flags, &text);

		if (text.count == 0) {	/* no mapping of flags to text */

			sam_syslog(LOG_ERR, GetCustMsg(31000),
			    tapealert_data->flags, &text,
			    tapealert_data->scsi_type,
			    tapealert_data->flags_len);
		}

		/* how many alerts ? */
		for (i = 0; i < text.count; i++) {

			/* convert the tapealert flag to oid naming format */
			convert_to_oid(text.msg[i].flag);

			/* manual name and parameter code for alertId */
			snprintf(alert_id, sizeof (alert_id),
			    "%s %d", text.msg[i].manual,
			    text.msg[i].pcode);

			/*
			 * Allocate space for command string
			 *
			 * The length of appmsg, cause and flags do not have
			 * any imposed limit
			 * The number 512 was arrived at by adding up the space
			 * required by all other strings and padding it for
			 * use by null terminating character and quotes
			 */
			command_len = strlen(text.msg[i].appmsg) +
			    strlen(text.msg[i].cause) +
			    strlen(text.msg[i].flag) + 512;

			if ((command = (char *)malloc(command_len)) != NULL) {

			/*
			 * send a trap
			 * This will include CATEGORY oid-DESCRIPTOR
			 * ERRORTYPE ALERTID SYSTEMID MESSAGE FAULTTIME
			 * TAPEVENDORID, TAPEPRODUCTID, TAPEREV, TAPENAME,
			 * TAPEVSN and PROBABLE-CAUSE
			 *
			 */
			snprintf(command, command_len,
			    "%s \"%s\" \"%s\" %d \"%s\" \"%s\" \"%s\" \
				\"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \"%s\" \
				\"%s\"",
			    SENDTRAP,
			    COMPONENT,
			    text.msg[i].flag,
			    get_error_type(text.msg[i].severity),
			    alert_id,
			    system_id,
			    text.msg[i].appmsg,
			    ctime(&(tapealert_data->tm)),
			    tapealert_data->vendor_id,
			    tapealert_data->product_id,
			    tapealert_data->revision,
			    tapealert_data->device_path,
			    tapealert_data->vsn,
			    text.msg[i].cause);
			/* log unsuccessful attempts to sent trap */
			if (system(command) == -1) {

				sam_syslog(LOG_ERR, GetCustMsg(31001),
				    COMPONENT, text.msg[i].flag,
				    strerror(errno));
			}

			free(command);
			}
		}

		if (system_id != NULL)
			free(system_id);

		if (tapealert_data != NULL)
			free(tapealert_data);
	}

	return (0);
}


/*
 * format_input
 *
 * vendor, product, revision, faulttime, equipment ordinal, device name,
 * type, vsn, flags_len and flags are given as input args
 * All are required, nothing is optional. Positional command line
 * arguments are expected
 *
 */
tapealert_data_t *
format_input(
	int	argc,		/* number of arguments */
	char	*argv[])	/* command line arguments */
{

	tapealert_data_t *data = NULL;
	int j;

	if (argc != NUMARGS) {
		sam_syslog(LOG_ERR, GetCustMsg(31002), NUMARGS, argc);
		return (NULL);
	}
	if ((data = (tapealert_data_t *)malloc(
	    sizeof (tapealert_data_t))) == NULL) {

		return (NULL);
	}

	memset(data, 0, sizeof (tapealert_data_t));

	for (j = 0; j < argc; j++) {
		/* input from sysevent conf file contains quotes */
		strip_quotes(argv[j]);
	}

	strncpy((char *)data->vendor_id, argv[1],
	    (sizeof (data->vendor_id) - 1));
	strncpy((char *)data->product_id, argv[2],
	    (sizeof (data->product_id) - 1));
	strncpy((char *)data->revision, argv[3],
	    (sizeof (data->revision) - 1));

	data->tm = strtol(argv[4], NULL, 0);
	data->eq = strtol(argv[5], NULL, 0);

	strncpy(data->device_path, argv[6], (sizeof (data->device_path) - 1));

	data->scsi_type = strtol(argv[7], NULL, 0);

	strncpy(data->vsn, argv[8], (sizeof (data->vsn) - 1));

	data->flags_len = strtol(argv[9], NULL, 0);
	data->flags = (uint64_t)strtoll(argv[10], NULL, 16);

	data->version = strtol(argv[11], NULL, 0);

	return (data);
}


/*
 * get_system_id
 *
 * This always returns a string, never NULL. If the hostname cannot be
 * obtained, then it returns UNKNOWNHOST
 */
char *
get_system_id()
{
	char hostname[MAXHOSTNAMELEN];	/* system id */

	if (gethostname(hostname, sizeof (hostname)) < 0) {
		sam_syslog(LOG_ERR, GetCustMsg(12526), strerror(errno));
		strncpy(hostname, UNKNOWNHOST, (sizeof (hostname) - 1));
	}

	return (strdup(hostname));
}


/*
 * get_error_type
 *
 * The tapealert severities are either critical, information or warning
 * get_error_type converts a string description of the severity level to
 * an integer
 *
 * follows the same priority as in syslog.h
 * critical		= LOG_CRIT = 2
 * warning		= LOG_WARNING = 4
 * information		= LOG_INFO = 6
 */
int
get_error_type(
	char *str_errortype)	/* error type in string format */
{
	int errortype = LOG_INFO;	/* default is info */

	if (str_errortype != NULL) {
		if (strcmp(str_errortype, CRITICAL) == 0)
			errortype = LOG_CRIT;
		else if (strcmp(str_errortype, WARNING) == 0)
			errortype = LOG_WARNING;
		else if (strcmp(str_errortype, INFORMATION) == 0)
			errortype = LOG_INFO;
	}
	return (errortype);
}


/*
 * convert_to_oid
 *
 * convert the input argument (tapealert flag) to an acceptable oid
 * (naming format). The input string is modified to hold the formatted string
 *
 * oid naming rules :
 * The identifier consists of one or more letters or digits,
 * Hyphens and spaces are not allowed
 *
 * e.g. Clean periodic is converted to CleanPeriodic
 * e.g. Dual-port interface error is converted to DualportInterfaceError
 *
 */
void
convert_to_oid(
	char *str)	/* str to be converted, also modified to return oid */
{

	char prevc = '\0';
	char *oid;

	if (str == NULL) {
		return;
	}
	oid = str;

	/* capitalize first letter of the string */
	if ((*oid) && islower((int)oid[0])) {
		oid[0] = toupper((int)oid[0]);
	}

	while (*oid) {

		/*
		 * Capitalize every letter after a space
		 */
		if (isspace((int)prevc) && islower((int)*oid)) {
			*str = toupper((int)*oid);
			str++;
		} else {
			/*
			 * oid naming only used alphanumeric characters
			 */
			if (isalnum((int)(*oid))) {
				*str = *oid;
				str++;
			}
		}
		prevc = *oid;
		oid++;
	}
	*str = '\0';

}


/*
 * strip_quotes
 *
 * Inputs from script sometimes have quotation marks in them
 * strip_quotes removes the quotes, the input argument is modified
 *
 *
 * If str is NULL, then nothing is done.
 */
void
strip_quotes(
	char *str)	/* input str, also modified to hold unquoted str */
{
	char *quote;

	if (str != NULL) {
		quote = str;

		while (*quote) {
			if (*quote != '"') {
				*str = *quote;
				str++;
			}
			quote++;
		}
		*str = '\0';
	}
}
