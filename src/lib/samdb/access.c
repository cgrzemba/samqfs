/*
 *  --	sam_db_access - Get SAM DB access information.
 *
 *	Description:
 *	    sam_db_access returns DB access information.
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

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sam/sam_malloc.h>

#define	SAM_DB_NO_MYSQL
#include <sam/sam_db.h>

#define	BUFLEN	4096			/* Buffer length for entry	*/

/*
 * --	fgetent - Get access entry.
 *
 *	Description:
 *	    Read one entry from the access file.
 *
 *	On Entry:
 *	    F		Acces file to read from.
 */
static
sam_db_access_t	*fgetent(
	FILE *F		/* Input file descriptor */
)
{
	char *entry;
	sam_db_access_t *dba;
	char *p, *q;

	SamMalloc(entry, BUFLEN);
	SamMalloc(dba, sizeof (sam_db_access_t));
	memset(dba, 0, sizeof (sam_db_access_t));

	if (!fgets(entry, BUFLEN, F)) {
		SamFree(entry);
		SamFree(dba);
		return (NULL);
	}

	SamStrdup(p, entry);
	SamFree(entry);

	dba->db_fsname = p;		/* Family set name */

	if ((q = strchr(p, ':')) == NULL) {
		goto no_entry;
	}
	*q = '\0';
	p  = q+1;

	dba->db_host = p;		/* Hostname */

	if ((q = strchr(p, ':')) == NULL) {
		goto no_entry;
	}
	*q = '\0';
	p  = q+1;

	dba->db_user = p;		/* DB user name */

	if ((q = strchr(p, ':')) == NULL) {
		goto no_entry;
	}
	*q = '\0';
	p  = q+1;

	dba->db_pass = p;		/* DB password */

	if ((q = strchr(p, ':')) == NULL) {
		goto no_entry;
	}
	*q = '\0';
	p  = q+1;

	dba->db_name = p;		/* DB name */

	if ((q = strchr(p, ':')) == NULL) {
		goto no_entry;
	}
	*q = '\0';
	p  = q+1;

	dba->db_port = p;		/* Port number */

	if ((q = strchr(p, ':')) == NULL) {
		goto no_entry;
	}
	*q = '\0';
	p  = q+1;

	dba->db_client = p;		/* Client flag */

	if ((q = strchr(p, ':')) == NULL) {
		goto no_entry;
	}
	*q = '\0';
	p  = q+1;

	dba->db_mount = p;		/* Mount point */

	q = strchr(p, '\n');	/* Delele trailing LF */
	if (q != NULL) {
		*q = '\0';
	}
	q = strchr(p, '\r');	/* Delele trailing CR */
	if (q != NULL) {
		*q = '\0';
	}
	q = p + strlen(p) - 1;
	if (*q == '/') {
		*q = '\0';			/* Delete trailing / */
	}
	return (dba);

no_entry:		/* If incomplete entry, ignore.	*/
	if (dba != NULL) {
		if (dba->db_fsname != NULL) {
			*dba->db_fsname = '\0';
		}
	}
	return (dba);
}


/*
 * --	sam_db_access - Get machine access information.
 *
 *	Description:
 *	    Locate SAM family set name in access file and return information.
 *
 *	On Entry:
 *	    fsname	Family set name to match.
 *	    file	Path/name of access file.
 *
 *	Returns:
 *	    NULL if user not found, else return access information.
 */
sam_db_access_t	*sam_db_access(
	char	*file,		/* Access file path/name	*/
	char	*fsname	/* Family set name to locate	*/
)
{
	FILE		*F;
	sam_db_access_t	*p;

	F = fopen(file, "r");

	if (F == NULL) {
		return (NULL);
	}

	while ((p = fgetent(F)) != NULL) {
		if (strcasecmp(p->db_fsname, fsname) == 0) {
			break;
		}
		if (p->db_fsname != NULL) {
			SamFree(p->db_fsname);
		}
		SamFree(p);
	}

	fclose(F);
	return (p);
}


/*
 * --	sam_db_access_mp - Get db access information by mount point.
 *
 *	Description:
 *	    Locate mount point in access file and return information.
 *
 *	On Entry:
 *	    mount	Mount point to locate.
 *	    file	Path/name of access file.
 *
 *	Returns:
 *	    NULL if user not found, else return access information.
 */
sam_db_access_t	*sam_db_access_mp(
	char	*file,		/* Access file path/name	*/
	char	*mount		/* Machine name to locate	*/
)
{
	FILE *F;
	sam_db_access_t	*p;

	F = fopen(file, "r");

	if (F == NULL) {
		return (NULL);
	}

	while ((p = fgetent(F)) != NULL) {
		if (strcasecmp(p->db_mount, mount) == 0) {
			break;
		}
		if (p->db_fsname != NULL) {
			SamFree(p->db_fsname);
		}
		SamFree(p);
	}

	fclose(F);
	return (p);
}
