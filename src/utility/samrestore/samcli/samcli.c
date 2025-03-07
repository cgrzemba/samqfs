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

/*
 * samcli.c - samrestore cli testbed
 */
#pragma ident	"$Revision: 1.14 $"
#include "samcli.h"
#include <pub/mgmt/error.h>
#include <unistd.h>
#include "aml/shm.h"

/* globals */
shm_alloc_t              master_shm, preview_shm;

/* Macro to refer to an integer on a list */
#define	LISTINT(X) (*((int *)(X)))

/* Helper routine. Safely determine one and only one entry in list */
static int
notone(sqm_lst_t *ptr)
{
	if (ptr == NULL)
		return (-1);
	if (ptr->length != 1)
		return (-1);
	return (0);
}

/* Main entry point */
int
main(int argc, char *argv[])
{
	sqm_lst_t	*lptr = NULL;
	sqm_lst_t	*ilptr = NULL;
	char	*strptr = NULL;
	char	*jobid;
	node_t	*ptr;
	int	rtnval = 0;
	int	rval;

	aparse(argc, argv, cli_args);	/* Do the heavy lifting parse */

	/* Dump out various lists we parsed */

	if (pathlist != NULL) {
		printf("Pathnames:\n");
		for (ptr = pathlist->head; ptr != NULL; ptr = ptr->next) {
			printf("  %s\n", ptr->data);
		}
	}
	if (csdlist != NULL) {
		printf("CSD options:\n");
		for (ptr = csdlist->head; ptr != NULL; ptr = ptr->next) {
			printf("  %s\n", ptr->data);
		}
	}
	if (destlist != NULL) {
		printf("Destinations:\n");
		for (ptr = destlist->head; ptr != NULL; ptr = ptr->next) {
			printf("  %s\n", ptr->data);
		}
	}
	if (dmplist != NULL) {
		printf("Dump files:\n");
		for (ptr = dmplist->head; ptr != NULL; ptr = ptr->next) {
			printf("  %s\n", ptr->data);
		}
	}
	if (restlist != NULL) {
		printf("Restrictions:\n");
		for (ptr = restlist->head; ptr != NULL; ptr = ptr->next) {
			printf("  %s\n", ptr->data);
		}
	}
	if (filstructlist != NULL) {
		printf("File Structures:\n");
		for (ptr = filstructlist->head; ptr != NULL; ptr = ptr->next) {
			printf("  %s\n", ptr->data);
		}
	}
	/* Handlle command */

	switch (command) {
	case v_null:
		printf(
		    "A command is required, one of setcsd, "
		    "getcsd, listdumps,\n");
		printf(
		    "readdumps, restore, stagefiles, listdir, "
		    "or getfilestatus\n");
		rtnval = 0;
		break;

	case v_setcsd:
		if (notone(filstructlist) || notone(csdlist)) {
			printf(
			    "setcsd requires exactly one each"
			    " of -f and -s\n");
			break;
		}
		rtnval = set_csd_params(
		    NULL,
		    filstructlist->head->data,
		    csdlist->head->data);
		break;

	case v_getcsd:
		if (notone(filstructlist)) {
			printf("getcsd requires exactly one -f\n");
			break;
		}
		rtnval = get_csd_params(
		    NULL,
		    filstructlist->head->data,
		    &strptr);
		break;

	case v_listdumps:
		if (notone(filstructlist)) {
			printf("listdumps requires exactly one -f\n");
			break;
		}
		rtnval = list_dumps_by_dir(
		    NULL,
		    filstructlist->head->data,
		    NULL,
		    &lptr);
		break;

	case v_restore:
		if ((filstructlist == NULL) || (dmplist == NULL) ||
		    (pathlist == NULL) || (destlist == NULL) ||
		    (cpylist == NULL)) {
			printf("restore requires -f, -u, -d, -c and -p\n");
			break;
		}
		rtnval = restore_inodes(
		    NULL,
		    filstructlist->head->data,
		    dmplist->head->data,
		    pathlist,
		    destlist,
		    cpylist,
		    0,
		    &jobid);
		if (rtnval)
			break;
		printf("Jobid returned: %s\n", jobid);

		while (get_search_results(NULL, filstructlist->head->data,
		    &lptr)) {
			if (samerrno != SE_RESTOREBUSY)
				break;
			printf("Waiting... %d\n", samerrno);
			sleep(1);
		}
		break;

	case v_takedump:
		if ((filstructlist == NULL) || (pathlist == NULL)) {
			printf("takedump requires -f and -p\n");
			break;
		}
		rtnval = take_dump(
		    NULL,
		    filstructlist->head->data,
		    pathlist->head->data,
		    &strptr);
		break;

	case v_stagefiles:
		if ((pathlist == NULL) || (cpylist == NULL) ||
		    (cpylist->length != pathlist->length)) {
			printf("stagefiles requires -p and -c "
			    "in equal numbers\n");
			break;
		}
		rtnval = stage_files_pre46(NULL, cpylist, pathlist, NULL);
		break;

	case v_listdir:
		if (notone(entrylist) || notone(pathlist) || notone(restlist)) {
			printf("listdir requires -p, -r and -e\n");
			break;
		}
		rtnval = list_dir(
		    NULL,
		    *(int*)(entrylist->head->data),
		    pathlist->head->data,
		    restlist->head->data, &lptr);
		break;

	case v_listver:
		if (notone(entrylist) || notone(pathlist) || notone(restlist) ||
		    notone(dmplist) || notone(filstructlist)) {
			printf("listver requires -f, -u, -p, -r and -e\n");
			break;
		}
		rtnval = list_versions(
		    NULL,
		    filstructlist->head->data,
		    dmplist->head->data,
		    *(int *)(entrylist->head->data),
		    pathlist->head->data,
		    restlist->head->data,
		    &lptr);
		break;

	case v_getverdetails:
		if (notone(pathlist) || notone(dmplist) ||
		    notone(filstructlist)) {
			printf("listver requires -f, -u, -p");
			break;
		}
		rtnval = get_version_details(
		    NULL,
		    filstructlist->head->data,
		    dmplist->head->data,
		    pathlist->head->data,
		    &lptr);
		break;

	case v_getfilestatus:
		if (pathlist == NULL) {
			printf("getfilestatus requires -p\n");
			break;
		}
		rtnval = get_file_status(NULL, pathlist, &ilptr);
		break;

	case v_getfiledetails:
		if (pathlist == NULL) {
			printf("getfiledetails requires -p\n");
			break;
		}
		if (notone(filstructlist)) {
			printf("getfiledetails requires -f\n");
			break;
		}
		rtnval = get_file_details(
		    NULL,
		    filstructlist->head->data,
		    pathlist,
		    &lptr);
		break;

	case v_getdumpstatus:
		if (pathlist == NULL) {
			printf("getdumpstatus requires -p\n");
			break;
		}
		if (notone(filstructlist)) {
			printf("getdumpstatus requires -f\n");
			break;
		}
		rtnval = get_dump_status(
		    NULL,
		    filstructlist->head->data,
		    pathlist,
		    &lptr);
		break;

	case v_decompressdump:
		if (notone(dmplist) || notone(filstructlist)) {
			printf("decompressdump requires one each -f and -u\n");
			break;
		}
		rtnval = decompress_dump(
		    NULL,
		    filstructlist->head->data,
		    dmplist->head->data,
		    &jobid);

		for (;;) {
			rtnval = get_dump_status(
			    NULL, filstructlist->head->data, dmplist, &lptr);
			if (strncmp(lptr->head->data,
			    "status=available", 16) == 0)
				break;

			rtnval = list_activities(NULL, 200, "", &lptr);
			printf("Found %d tasks: \n", lptr->length);
			ptr = lptr->head;
			while (ptr != NULL) {
				printf("  %s\n", ptr->data);
				ptr = ptr->next;
			}

			sleep(5);
		}

		break;

	case v_search:
		if (notone(filstructlist) || notone(dmplist) ||
		    notone(restlist) || notone(entrylist)) {
			printf("search requires one each -f, -u, -e, and -r\n");
		}
		rtnval = search_versions(
		    NULL,
		    filstructlist->head->data,
		    dmplist->head->data,
		    *(int *)(entrylist->head->data),
		    NULL,
		    restlist->head->data,
		    &jobid);
		if (rtnval)
			break;
		printf("Jobid returned: %s\n", jobid);

		for (;;) {
			rtnval = get_search_results(
			    NULL, filstructlist->head->data, &lptr);
			if ((rtnval == 0) && (lptr != NULL))
				break;

			rtnval = list_activities(NULL, 200, "", &lptr);
			printf("Found %d tasks: \n", lptr->length);
			ptr = lptr->head;
			while (ptr != NULL) {
				printf("  %s\n", ptr->data);
				ptr = ptr->next;
			}
			sleep(1);
		}
		break;

	default:
		printf("Unrecognized command number %d\n", command);
		break;
	}			/* End switch (command) */

	if (rtnval) {
		if (samerrno) {
			printf("%s\n", samerrmsg);
		} else {
			printf("Non-zero return %d but no samerrno\n", rtnval);
		}
		return (rtnval); /* If non-zero return */
	}
	if (strptr != NULL)
		printf("Returned string: %s\n", strptr);


	if (lptr != NULL) {
		printf("Returned list of %d strings: \n", lptr->length);
		ptr = lptr->head;
		while (ptr != NULL) {
			printf("  %s\n", ptr->data);
			ptr = ptr->next;
		}
	}
	if (ilptr != NULL) {
		printf("Returned list of %d integers: \n", ilptr->length);
		ptr = ilptr->head;
		while (ptr != NULL) {
			printf("  %d\n", LISTINT(ptr->data));
			ptr = ptr->next;
		}
	}
}
