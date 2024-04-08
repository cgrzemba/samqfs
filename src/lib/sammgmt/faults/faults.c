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

#pragma ident	"$Revision: 1.35 $"

/*
 * faults.c
 *
 * SAM-FS/QFS anamolies and tapealerts are persisted as faults in FAULTLOG
 * These anamolies are propagated as sysevents and 'write to FAULTLOG'
 * program is a subscriber of these sysevents.
 *
 * faults.c is an interface to retrieve the faults from FAULTLOG and
 * provide api to ACK/DELETE these faults.
 */

/* ANSI C headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

/* POSIX headers. */
#include <sys/stat.h>

/* Solaris headers. */
#include <sys/mman.h>

/* SAM API headers. */
#include "pub/mgmt/faults.h"
#include "pub/mgmt/error.h"
#include "mgmt/util.h"

/* SAM-FS headers. */
#include "sam/defaults.h"
#include "sam/sam_trace.h"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */


static int read_faults(
	int oflags, int prot, int flags, void **mp, size_t *size);
static int remove_deleted_fault(void *mp, size_t size);
static int helper_get_faults(char *libname, equ_t eq, sqm_lst_t **faults_list);

/*
 * get all faults
 */
int
get_all_faults(
ctx_t	*ctx,		/* ARGSUSED */
sqm_lst_t **faults_list)	/* return - list of faults */
{

	int ret = -1;

	Trace(TR_MISC, "getting faults...");

	ret = helper_get_faults(
	    "", /* don't filter by library name */
	    -1, /* dont filter by library eq */
	    faults_list);

	if (ret != 0) {
		Trace(TR_ERR, "get all faults failed:[%d]%s",
		    samerrno, samerrmsg);
	} else {
		Trace(TR_MISC, "get faults success");
	}
	return (ret);
}

/*
 * get_faults_by_lib().
 * Get all faults by library name
 * returns all the faults that are associated with the library name
 * The order of the list is just as is it is found in the file
 * Since there is no way the user can specify the number of faults
 * desired, there is no need to get the latest faults first as all
 * the faults (by library name) are returned
 *
 */
int
get_faults_by_lib(
ctx_t	*ctx, /* ARGSUSED */
uname_t library, /* lib. family setname */
sqm_lst_t **faults_list) /* the list of faults */
{
	int ret = -1;

	Trace(TR_MISC, "getting faults by lib[%s]...", library);

	ret = helper_get_faults(
	    library, /* filter by library name */
	    (EQU_MAX + 1), /* don't filter by library eq */
	    faults_list);

	if (ret != 0) {
		Trace(TR_ERR, "get faults by lib name[%s] failed:[%d]%s",
		    library, samerrno, samerrmsg);
	} else {
		Trace(TR_MISC, "get faults success");
	}

	return (ret);
}

/*
 * get_faults_by_eq().
 * Get all faults by library eq
 * returns all the faults that are associated with the library eq
 * The order of the list is just as is it is found in the file
 * Since there is no way the user can specify the number of faults
 * desired, there is no need to get the latest faults first as all
 */
int
get_faults_by_eq(
ctx_t	*ctx, /* ARGSUSED */
equ_t	eq,	/* the eq # */
sqm_lst_t **faults_list) /* the list of faults */
{
	int ret = -1;

	Trace(TR_MISC, "getting faults by eq[%d]...", eq);

	ret = helper_get_faults(
	    "", /* don't filter by library name */
	    eq, /* filter by library eq */
	    faults_list);

	if (ret != 0) {
		Trace(TR_ERR, "get faults by eq[%d] failed:[%d]%s",
		    eq, samerrno, samerrmsg);
	} else {
		Trace(TR_MISC, "get faults success");
	}

	return (ret);
}

/*
 * is_faults_gen_status_on
 * get the status of the fault generation.
 */
int
is_faults_gen_status_on(
ctx_t	*ctx, /* ARGSUSED */
boolean_t	*enabled)	/* the status */
{

	sam_defaults_t *defaults = NULL;  /* defaults.conf table */

	Trace(TR_MISC, "getting faults generation status");

	if (ISNULL(enabled)) {
		Trace(TR_ERR, "Check if faults enabled failed:%s", samerrmsg);
		return (-1);
	}

	defaults = (sam_defaults_t *)GetDefaults();

	/* check if the flag to see if SNMP support is on is set */
	*enabled = ((defaults != NULL) && (defaults->flags & DF_ALERTS)) ?
	    B_TRUE : B_FALSE;

	Trace(TR_MISC, "Is faults enabled return SUCCESS");

	return (0);
}

/*
 * marks the state of the faults to ACK
 *
 * Returns: -1 on error or partial failure, 0 on complete success
 * Partial failures - If unable to chage state of some faults
 * 			return -1 with samerrmsg populated with the
 *			Ids that could not be ACK'ed.
 *			The remaining faults are ACK'ed.
 * A -2 is not returned because the client GUI does not handle -2 for this case.
 */
int
ack_faults(
ctx_t	*ctx,	/* ARGSUSED */
int	num,	/* num of faults to be acknowledged */
long errorID[]	/* error id of faults to be acknowledged */
)
{
	void		*mp = NULL;		/* pointer to mem-map */
	fault_attr_t	*fp = NULL;		/* ptr to the mem-mapped file */
	size_t		size = 0;		/* size of mem-map file */
	int		i, n;
	int		total_faults = 0;	/* total faults in system */
	char		erridsmsg[MAX_MSG_LEN] = {0}; /* ACK failed for Id */
	int		ack_fail = 0;		/* ACK failed fault count */

	Trace(TR_MISC, "Acknowledging %d faults", num);

	/* Get a memory map of all the faults in the system */
	if (read_faults(O_RDWR, PROT_READ | PROT_WRITE, MAP_SHARED, &mp, &size)
	    == -1) {

		/* Could not map the faults */
		Trace(TR_ERR, "Acknowledging faults failed: %s", samerrmsg);
		return (-1);
	}

	total_faults = size / sizeof (fault_attr_t);

	/* traverse the fault records */
	for (fp =  (fault_attr_t *)mp, n = 0; n < total_faults; fp++, n++) {

		/* Traverse the errorID[] and mark the fault state to ACK */
		for (i = 0; i < num; i++) {
			if (errorID[i] != -1 && fp->errorID == errorID[i]) {
				fp->state = ACK;
				errorID[i] = -1; /* acknowledged */
				break;
			}
		}
	}

	for (i = 0; i < num; i++) {
		if (errorID[i] != -1) {
			/* Could not find the fault to be ACK */
			ack_fail++;

			snprintf(erridsmsg, sizeof (erridsmsg), "%s %ld",
			    erridsmsg, errorID[i]);
		}
	}
	erridsmsg[sizeof (erridsmsg) - 1] = '\0';
	if (mp != NULL) {
		(void) munmap(mp, size);
	}

	if (ack_fail > 0) {
		/* some or all faults could not be found */
		Trace(TR_MISC, "ACK failed for %s", erridsmsg);

		samerrno = SE_ACK_FAULT_PARTIAL_FAIL;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ACK_FAULT_PARTIAL_FAIL), erridsmsg);

		return (-1);	/* GUI does not handle -2 error codes */
	}
	Trace(TR_MISC, "Acknowledged the faults successfully");
	return (0);
}

/*
 * delete_faults deletes the faults identified by their ID from FAULTLOG
 *
 * Returns: -1 on error or partial failure, 0 on complete success
 * Partial failures - If unable to delete some faults
 * 			return -1 with samerrmsg populated with the
 *			Ids that could not be DELETE'd.
 * A -2 is not returned because the client GUI does not handle -2 for this case.
 */
int
delete_faults(
ctx_t	*ctx,		/* ARGSUSED */
int	num,		/* num of faults to be deleted */
long errorID[]		/* error id of faults to be deleted */
)
{
	void		*mp = NULL;		/* pointer to mem-map */
	fault_attr_t	*fp = NULL;		/* ptr to the mem-mapped file */
	size_t		size = 0;		/* size of mem-map file */
	int		i, n;
	int		total_faults = 0;
	char		erridsmsg[MAX_MSG_LEN] = {0};	/* del failed for */
	int		del_pass = 0;		/* state marked to delete */

	Trace(TR_MISC, "Deleting faults");

	/*
	 * get a copy of the pointer to the mapped area, MAP_PRIVATE
	 * don't want to change the state in the FAULTLOG file
	 * A new file will be written which will exclude the
	 * deleted faults
	 */
	if (read_faults(O_RDWR, PROT_READ | PROT_WRITE, MAP_PRIVATE,
	    &mp, &size) == -1) {

		/* Could not map the faults */
		Trace(TR_ERR, "Deleting faults failed: %s", samerrmsg);
		return (-1);

	} /* able to get memory map */

	total_faults = size / sizeof (fault_attr_t);

	/*
	 * Traverse the array of errorID and mark the fault state to
	 * DELETED. The actual removing of faults from the FAULTLOG
	 * is done once (in bulk) instead of once per fault as it
	 * involves expensive file io operation
	 */
	for (fp =  (fault_attr_t *)mp, n = 0; n < total_faults; fp++, n++) {

		for (i = 0; i < num; i++) {
			Trace(TR_DEBUG, "compare [%ld] with [%ld]",
			    fp->errorID, errorID[i]);
			if ((errorID[i] != -1) && (fp->errorID == errorID[i])) {
				fp->state = DELETED;
				errorID[i] = -1; /* deleted */
				del_pass++;
				break;
			}
		}
	}

	for (i = 0; i < num; i++) {
		if (errorID[i] != -1) {

			/* Could not find the fault to be deleted */
			snprintf(erridsmsg, sizeof (erridsmsg), "%s %ld",
			    erridsmsg, errorID[i]);
		}
	}

	Trace(TR_MISC, "Marked the state of the faults to DELETED");

	/*
	 * The faults have been marked with state equal to DELETED state.
	 * if all the faults are to be deleted, simply remove the
	 * FAULTLOG
	 */
	if (del_pass == total_faults) {
		remove(FAULTLOG);
	} else {
		/* some faults or no faults could not be marked DELETED */
		if (del_pass > 0) {
			/*
			 * Copy the NON-DELETED faults to another file
			 * and rename the file to FAULTLOG
			 */
			if (remove_deleted_fault(mp, size) == -1) {

				Trace(TR_ERR, "Could not delete fault");
				(void) munmap(mp, size);
				return (-1);
			}
		}
	}
	if (del_pass < num) {
		Trace(TR_MISC, "Delete failed for %s", erridsmsg);

		samerrno = SE_DEL_FAULT_PARTIAL_FAIL;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_DEL_FAULT_PARTIAL_FAIL), erridsmsg);

		(void) munmap(mp, size);
		return (-1);
	}

	munmap(mp, size);
	Trace(TR_MISC, "Deleted the faults successfully");
	return (0);
}

/*
 * Remove the DELETED fault from the FAULTLOG file.
 *
 * removes the faults with state = DELETED from FAULTLOG. This is done
 * by writing the NON-DELETED faults to another file and renaming it to
 * FAULTLOG.
 *
 * Returns -1 on error with samerrmsg and samerrno filled in.
 * Returns 0 on success
 *
 */
static int
remove_deleted_fault(void *mp, size_t size)
{
	char tmpfilnam[MAXPATHLEN] = {0};
	FILE *tmpfile = NULL;
	fault_attr_t *fp = NULL;
	int fault_len = sizeof (fault_attr_t);
	int total_faults = 0;
	int fd = 0;

	Trace(TR_DEBUG, "Remove the faults with state = DELETED from FAULTLOG");

	total_faults = size / sizeof (fault_attr_t);

	/* get a temporary file to writer faults to */
	if (mk_wc_path(FAULTLOG, tmpfilnam, sizeof (tmpfilnam)) != 0) {
		Trace(TR_ERR, "Unable to delete faults: %s", samerrmsg);
		return (-1);
	}

	fp = (fault_attr_t *)mp;
	if ((fd = open64(tmpfilnam, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		tmpfile = fdopen(fd, "w+");
	}
	if (tmpfile == NULL) {
		samerrno = SE_NOTAFILE;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    tmpfilnam);
		Trace(TR_ERR, "Unable to delete faults: %s", samerrmsg);
		unlink(tmpfilnam);
		return (-1);
	}
	while (total_faults-- > 0) {
		if (fp->state != DELETED) {
			if ((write(fd, fp, fault_len)) != fault_len) {

				samerrno = SE_FILE_WRITTEN_FAILED;
				snprintf(samerrmsg, MAX_MSG_LEN,
				    GetCustMsg(samerrno), tmpfilnam);
				Trace(TR_ERR, "Delete faults failed: [%d] %s",
				    samerrno, samerrmsg);
				unlink(tmpfilnam);
				fclose(tmpfile);
				return (-1);
			}
		}
		fp++;
	}

	/* Now rename the temporary file to FAULTLOG */
	unlink(FAULTLOG);
	fclose(tmpfile);

	if (rename(tmpfilnam, FAULTLOG) < 0) {
		samerrno = SE_RENAME_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    tmpfilnam, FAULTLOG);

		Trace(TR_ERR, "Delete faults failed: [%d] %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_DEBUG, "Removed the faults from log");
	return (0);

}

/*
 * Read the faults from the FAULTLOG binary and map the file
 */
static int
read_faults(
int oflags,	/* FAULTLOG access modes */
int prot,	/* mapping access modes */
int flags,	/* handling of mapped data */
void **mp,	/* return - pointer to mmaped area with faults */
size_t *size	/* return - size of mmaped file */
)
{
	int		fd;	/* the file descr. */
	struct	stat64	st;	/* struct for file info */
	struct	stat64	buf;	/* struct for file info */

	*mp = NULL;
	*size = 0;

	if (lstat64(FAULTLOG, &buf) != 0) {
		/* If FAULTLOG does not exist, it is a clean system */
		Trace(TR_ERR, "No faults on system");
		return (0);
	}

	if ((fd = open64(FAULTLOG, oflags)) < 0) {

		samerrno = SE_CANT_OPEN_FLOG;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CANT_OPEN_FLOG), FAULTLOG);

		Trace(TR_ERR, "get faults failed: %s", samerrmsg);
		return (-1);
	}

	/* Stat the file for info to map */
	if (fstat64(fd, &st) != 0) {

		samerrno = SE_FSTAT_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FSTAT_FAILED), FAULTLOG);

		Trace(TR_ERR, "get faults failed: %s", samerrmsg);

		close(fd);
		return (-1);
	}

	*size = st.st_size;	/* size of memory mapped area */

	/*
	 * Map in the entire file.
	 * Solaris's MM takes care of the page-ins and outs.
	 */
	*mp = mmap(NULL, st.st_size, prot, flags, fd, 0);
	if (*mp == MAP_FAILED) {
		samerrno = SE_MMAP_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_MMAP_FAILED), FAULTLOG);

		Trace(TR_ERR, "get faults failed: %s", samerrmsg);

		close(fd);
		return (-1);
	}

	close(fd);
	return (0);
}


/*
 * gets fault summary
 *
 * returns the number of unacknowledged critical, major and minor faults
 */
int
get_fault_summary(
ctx_t *ctx,			/* context argument */
fault_summary_t	*fault_summary	/* return : number of faults */
)
{
	sqm_lst_t		*lst = NULL;
	node_t 		*node = NULL;
	fault_attr_t	*fault_attr = NULL;

	if (ISNULL(fault_summary)) {
		Trace(TR_ERR, "get fault summary failed: %s", samerrmsg);
		return (-1);
	}
	memset(fault_summary, 0, sizeof (fault_summary_t));

	if (get_all_faults(ctx, &lst) != 0) {
		/* samerrmsg and samerrno is already populated */
		Trace(TR_ERR, "get fault summary failed: %s", samerrmsg);
		return (-1);
	}

	node = lst->head;
	while (node != NULL) {
		fault_attr = (fault_attr_t *)node->data;
		/* count of unacknowledged faults only */
		if (fault_attr->state == UNRESOLVED) {
			if (fault_attr->errorType == 0) {
				/* critical fault */
				fault_summary->num_critical_faults++;
			} else if (fault_attr->errorType == 1) {
				/* major fault */
				fault_summary->num_major_faults++;
			} else {
				/* minor fault */
				fault_summary->num_minor_faults++;
			}
		}
		node = node->next;
	}
	lst_free_deep(lst);
	return (0);
}

static int
helper_get_faults(
char *libname,	/* If empty, don't filter by library name, return all */
equ_t eq,		/* If EQU_MAX + 1, don't filter by eq, return all */
sqm_lst_t ** faults_list)	/* return - list of faults */
{
	fault_attr_t *fp = NULL;	/* a ptr to fault in mem-mapped file */
	fault_attr_t *tmpp = NULL;	/* dup fault data */
	void*		mp = NULL;	/* a ptr to the mapped address */
	size_t		size = 0;	/* size of file memory mapped */
	int		total_faults = 0;
	int		ret = 0;	/* assume success */

	Trace(TR_MISC, "getting faults...");

	if (ISNULL(faults_list)) {
		Trace(TR_ERR, "get faults exit:[%d] %s", samerrno, samerrmsg);
		return (-1);
	}

	/* Get a memory map of all the faults in the system */
	if (read_faults(O_RDONLY, PROT_READ, MAP_SHARED, &mp, &size) == -1) {
		Trace(TR_ERR, "get faults exit:[%d] %s", samerrno, samerrmsg);
		return (-1);
	}

	*faults_list = NULL;
	*faults_list = lst_create();
	if (*faults_list == NULL) {
		Trace(TR_ERR, "get faults exit:[%d] %s", samerrno, samerrmsg);
		munmap(mp, size);
		return (-1);
	}

	/* calculate the total number of faults. */
	total_faults = size / sizeof (fault_attr_t);

	/* copy the pointer to the mapped area and cast it to a fault type */

	/*
	 * copy the fault records to a linked-list, start from the
	 * tail of the mmap pointer to get the latest faults first
	 */
	for (fp = (fault_attr_t *)mp;
	    (total_faults > 0 && fp != NULL);
	    fp++, total_faults--) {

		if ((libname != NULL) && (libname[0] != '\0') &&
		    (strncmp(fp->library, libname, strlen(libname)) != 0)) {

			continue;
		}

		if ((eq != (EQU_MAX + 1)) && (fp->eq != eq)) {
			continue;
		}

		tmpp = (fault_attr_t *)mallocer(sizeof (fault_attr_t));
		if (tmpp == NULL) {
			ret = -1;
			break;
		}
		memcpy(tmpp, fp, sizeof (fault_attr_t));

		if (lst_append(*faults_list, (void *)tmpp) != 0) {
			ret = -1;
			break;
		}
		tmpp = NULL;
	}

	munmap(mp, size);
	if (ret == -1) {
		Trace(TR_ERR, "get faults failed: [%d] %s",
		    samerrno, samerrmsg);
		if (tmpp != NULL) {
			free(tmpp);
			tmpp = NULL;
		}
		if (faults_list != NULL) {
			((*faults_list)->length > 0) ?
			    lst_free_deep(*faults_list) :
			    lst_free(*faults_list);

			*faults_list = NULL;
		}
		return (-1);
	} else {
		Trace(TR_MISC, "get faults success");
		return (0);
	}
}
