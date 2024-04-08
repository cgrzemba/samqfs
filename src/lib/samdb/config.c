/*
 *  --	config.c - Get SAM DB configuration information.
 *
 *	Description:
 *	    sam_db_config returns DB config information.
 *
 *	File Format:
 *	    name:host:user:pass:name:port:cf:mount_point
 *	    fs_name	db_fsname	Family set name
 *	    host	db_host		Hostname
 *	    user	db_user		DB user name
 *	    pass	db_pass		DB password
 *	    name	db_name		DB name
 *	    port	db_port		Port
 *	    cf		db_client	Client flag (see mySQL)
 *	    mount_point	db_mount	Mount point
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

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sam/sam_malloc.h>

#include <sam/sam_db.h>

#define	BUFLEN	4096			/* Buffer length for entry	*/

/* static functions */
static sam_db_conf_t *sam_db_conf_new(void);
static sam_db_conf_t *fgetconf(FILE *, sam_db_conf_t *);

/*
 * --	sam_db_conf_get - Get machine configuration information.
 *
 *	Description:
 *	    Locate SAM family set name in config file and return information.
 *
 *	On Entry:
 *	    file	Path/name of access file.
 *	    fsname	Family set name to match.
 *
 *	Returns:
 *	    NULL if user not found, else return configuration information.  This
 * 		must be freed using sam_db_conf_free().
 */
sam_db_conf_t	*sam_db_conf_get(
	char	*file,		/* Access file path/name	*/
	char	*fsname	/* Family set name to locate	*/
)
{
	FILE		*F;
	sam_db_conf_t	*conf;
	boolean_t	is_found = FALSE;

	F = fopen(file, "r");

	if (F == NULL) {
		return (NULL);
	}

	conf = sam_db_conf_new();

	while (fgetconf(F, conf) != NULL) {
		if (strcasecmp(conf->db_fsname, fsname) == 0) {
			is_found = TRUE;
			break;
		}
	}

	if (!is_found) {
		sam_db_conf_free(conf);
		conf = NULL;
	}

	fclose(F);
	return (conf);
}

/*
 * sam_db_conf_free - Frees samdb config structure
 */
void
sam_db_conf_free(sam_db_conf_t *conf)
{
	if (conf != NULL) {
		if (conf->db_fsname != NULL) {
			SamFree(conf->db_fsname);
		}
		SamFree(conf);
	}
}

/*
 * sam_db_conf_new - Allocates a new sam_db_conf_t struct, must be
 *   freed using sam-db_conf_free.
 */
static sam_db_conf_t
*sam_db_conf_new(void)
{
	sam_db_conf_t *conf;

	SamMalloc(conf, sizeof (sam_db_conf_t));
	memset(conf, 0, sizeof (sam_db_conf_t));
	SamMalloc(conf->db_fsname, BUFLEN);
	memset(conf->db_fsname, 0, BUFLEN);

	return (conf);
}

/*
 * --	fgetconf - Get next configuration entry from file.
 *
 *	Description:
 *	    Read one entry from the access file.
 *
 *	On Entry:
 *	    F		Config file to read from.
 *	    conf	configuration entry to populate
 *
 * 	Returns:
 *	    conf if entry successfully populated, otherwise null.
 */
static
sam_db_conf_t *fgetconf(
	FILE *F,		/* Input file descriptor */
	sam_db_conf_t *conf)	/* Configuration entry */
{
#define	NUM_FIELDS	7
	char **fields[NUM_FIELDS] = {
		&conf->db_host,
		&conf->db_user,
		&conf->db_pass,
		&conf->db_name,
		&conf->db_port,
		&conf->db_client,
		&conf->db_mount };
	char *cur;
	char *end;
	int i;

	cur = conf->db_fsname;

	do {
		if (!fgets(cur, BUFLEN, F)) {
			return (NULL);
		}
	} while (*cur == '#');

	/* Set all fields */
	for (i = 0; i < NUM_FIELDS; i++) {
		if ((end = strchr(cur, ':')) == NULL) {
			goto no_entry;
		}
		*end = '\0';
		cur = end+1;

		*fields[i] = cur;
	}

	/* Delele trailing LF */
	end = strchr(cur, '\n');
	if (end != NULL) {
		*end = '\0';
	}

	/* Delele trailing CR */
	end = strchr(cur, '\r');
	if (end != NULL) {
		*end = '\0';
	}

	/* Delete trailing / */
	end = cur + strlen(cur) - 1;
	if (*end == '/') {
		*end = '\0';
	}

	return (conf);

no_entry:		/* If incomplete entry, ignore.	*/
	if (conf != NULL) {
		if (conf->db_fsname != NULL) {
			*conf->db_fsname = '\0';
		}
	}
	return (NULL);
}
