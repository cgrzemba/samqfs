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
#pragma ident   "$Revision: 1.33 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * vfstab.c
 * provides an interface to add, remove and modify vfstab entries from the
 * vfstab file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/vfstab.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#include "sam/lib.h"
#include "sam/sam_trace.h"

#include "pub/mgmt/filesystem.h"
#include "mgmt/config/vfstab.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "parser_utils.h"
#include "mgmt/util.h"
#include "sam/setfield.h"
#include "mgmt/config/mount_options.h"

/*
 * General Algorithm for any changes to the vfstab file.
 * 1. Make a Backup copy of the vfstab.
 * 2. read and parse it into mem
 * 3. if going to do some writing
 *	make a working copy of the file.
 * 4. open working copy to write
 * 5. write working copy.
 * 6. copy working copy to vfstab location.
 * 7. backup the resultant vfstab.
 *
 * The goal here is that the vfstab file be valid
 * at all times (with exception of during step 6).  That way if
 * a failure occurs it is as if these operations were never called. Also
 * we don't want to change the permissions of the files.  This achieves both.
 *
 */


#define	VFS_OPT_SEPARATOR ","

static char *mandatory_fields[] = { "shared", '\0' };

/* private data */
#define	INCREMENT 1024

static upath_t err_msg;


/* private functions */
static void msg_func(int code, char *msg);
static int _read_beginning(FILE *f, struct vfstab *ent, char ** file_beginp);
static int _read_remaining(FILE *f, fpos_t *pos, char ** file_endp);
static int _add_vfstab_entry(vfstab_entry_t *, boolean_t);
static int _delete_vfstab_entry(char *, boolean_t);
static void free_vfstab_fields(struct vfstab *ent);
static int mk_vfstab(vfstab_entry_t *in, struct vfstab *out);
static int mk_vfstab_entry(struct vfstab *ent, vfstab_entry_t **out);
static boolean_t is_mandatory_vfs_entry(struct fieldVals *entry);

int _modify_vfstab_entry(vfstab_entry_t *input);


/*
 * get the vfstab entry for the named file system.
 * returns -1 if the filesytem is not found.
 */
int
get_vfstab_entry(
char *fs_name,			/* name of fs to get */
vfstab_entry_t **ret_val)	/* malloced return value */
{

	FILE *f;
	struct vfstab tmp_ent;
	vfstab_entry_t *ret = NULL;
	int err = 0;
	char err_buf[80];

	Trace(TR_DEBUG, "getting vfstab entry for %s", Str(fs_name));

	if (ISNULL(fs_name, ret_val)) {
		return (-1);
	}

	f = fopen(VFSTAB, "r");
	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), VFSTAB, err_buf);

		goto ERR;
	}

	err = getvfsspec(f, &tmp_ent, fs_name);

	/* done with vfstab */
	fclose(f);

	if (err != 0) {
		Trace(TR_MISC, "vfstab error %d", err);
		set_samerrno_for_vfstab_error(fs_name, err);
		goto ERR;
	}

	if (mk_vfstab_entry(&tmp_ent, &ret) != 0) {
		goto ERR;
	}

	*ret_val = ret;
	Trace(TR_DEBUG, "got vfstab entry");
	return (0);

ERR:

	Trace(TR_OPRMSG, "getting vfstab entry failed %s", samerrmsg);
	return (-1);
}


/*
 * translate and set error codes from sys/vfstab.h into samerrno
 */
void
set_samerrno_for_vfstab_error(
char *fs_name,	/* file system name */
int err)	/* sys/vfstab.h error code */
{

	if (err == -1) {
		/*  fs_name not found in vfstab */
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    fs_name);

	} else if (err == VFS_TOOLONG) {
		samerrno = SE_MAX_LENGTH_EXCEEDED;

		/* Maximum length of %d exceeded for %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_MAX_LENGTH_EXCEEDED),
		    VFS_LINE_MAX, "vfstab entry");

	} else if (err == VFS_TOOMANY) {
		samerrno = SE_VFSTAB_TOO_MANY_FIELDS;

		/* The vfstab entry for %s contains too many fields */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_VFSTAB_TOO_MANY_FIELDS),
		    fs_name);

	} else if (err == VFS_TOOFEW) {
		samerrno = SE_VFSTAB_TOO_FEW_FIELDS;

		/* The vfstab entry for %s contains too few fields */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_VFSTAB_TOO_FEW_FIELDS),
		    fs_name);

	} else {
		samerrno = SE_UNRECOGNIZED_VFSTAB_ERROR;

		/* unrecognized vfstab error for filesystem %s: %d */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_VFSTAB_TOO_FEW_FIELDS),
		    fs_name);
	}
}


/*
 * delete the vfstab entry for the named file system.
 */
int
delete_vfstab_entry(char *fs_name)
{

	int error;

	Trace(TR_MISC, "deleting vfstab entry for %s",
	    fs_name ? fs_name : "NULL");

	error = _delete_vfstab_entry(fs_name, B_TRUE);
	if (error != 0) {
		Trace(TR_ERR, "deleting vfstab entry failed %s", samerrmsg);
		return (error);
	}

	Trace(TR_MISC, "deleted vfstab entry");

	return (0);
}


/*
 * private function to delete a vfstab entry.
 *
 * if copy_back_to_location == TRUE the vfstab file will be copied back
 * to its original location. Otherwise it will not be and no second
 * backup copy of the file will be made- in this case only working_copy
 * will be modified. This behavior is to support update without excessive
 * file operations.
 */
static int
_delete_vfstab_entry(
char *fs_name,				/* name of file system to delete */
boolean_t copy_back_to_location)	/* if true copy back to vfstab file */
{

	FILE *f;
	fpos_t aft_pos;
	int error = 0;
	char working_copy[MAXPATHLEN+1] = {0};
	struct vfstab entry;
	char err_buf[80];
	int fd;
	char *file_end = NULL;
	char *file_begin = NULL;

	Trace(TR_OPRMSG, "deleting vfstab entry %s %s", Str(fs_name),
	    copy_back_to_location ? "TRUE" : "FALSE");


	/* check params */
	if (ISNULL(fs_name)) {
		return (-1);
	}

	/* check fopen errors */
	f = fopen(VFSTAB, "r");
	if (NULL == f) {
		error = errno;
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		StrFromErrno(error, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), VFSTAB, err_buf);
		goto err;
	}

	error = getvfsspec(f, &entry, fs_name);
	if (error != 0) {
		fclose(f);
		set_samerrno_for_vfstab_error(fs_name, error);
		goto err;
	}


	/*
	 * get all of the data except the line with the
	 * filesystem on it. current position is after entry.
	 */
	if (fgetpos(f, &aft_pos) != 0) {
		error = errno;
		fclose(f);
		samerrno = SE_IO_OPERATION_FAILED;

		/* IO operation %s failed: %s */
		StrFromErrno(error, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "fgetpos", err_buf);
		goto err;
	}

	if (_read_remaining(f, &aft_pos, &file_end) != 0) {
		/* leave samerrno as set */
		fclose(f);
		goto err;
	}

	/* get the beginning of the file. */
	errno = 0;
	rewind(f);
	if (errno != 0) {
		fclose(f);
		samerrno = SE_IO_OPERATION_FAILED;

		/* IO operation %s failed: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "rewind", err_buf);
		goto err;
	}

	if (_read_beginning(f, &entry, &file_begin) != 0) {
		fclose(f);
		goto err;
	}

	/* close the file */
	fclose(f);

	/* Make Backup copy backup_path/vfstab.src_file's_mod_time */
	if (backup_cfg(VFSTAB) != 0) {
		goto err;
	}

	/*
	 * copy the vfstab to the working file
	 * open the copy and write out file.
	 */
	if (make_working_copy(VFSTAB, working_copy,
	    sizeof (working_copy)) != 0) {
		goto err;
	}

	f = NULL;
	if ((fd = open(working_copy, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}

	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), working_copy, err_buf);
		goto err;
	}

	if (file_begin != NULL) {
		fprintf(f, "%s", file_begin);
		fflush(f);
		free(file_begin);
		file_begin = NULL;
	}
	if (file_end != NULL) {
		fprintf(f, "%s", file_end);
		fflush(f);
		free(file_end);
		file_end = NULL;
	}

	fclose(f);

	if (copy_back_to_location) {

		/* copy the working copy back to vfstab */
		if (cp_file(working_copy, VFSTAB) != 0) {
			goto err;
		}

		/* log the config change */
		Trace(TR_FILES, "deleted %s from %s", fs_name, VFSTAB);

		if (backup_cfg(VFSTAB) != 0) {
			goto err;
		}
	}

	unlink(working_copy);
	Trace(TR_OPRMSG, "deleted vfstab entry");
	return (0);

err:
	if (working_copy[0] != '\0') {
		unlink(working_copy);
	}
	free(file_begin);
	free(file_end);

	Trace(TR_OPRMSG, "deleting vfstab entry failed: samerrno %d %s",
	    samerrno, samerrmsg);

	return (-1);
}


/*
 * read from f's current position to end storing in file_begin
 * until the vfstab entry ent is encountered.
 *
 * after successful execution FILE
 * if any error is encounterd file_begin is freed, samerrno is set and
 * -1 is returned.
 */
static int
_read_beginning(
FILE *f,
struct vfstab *ent,	/* entry to read up to */
char **file_beginp)	/* buffer to hold beginning of file */
{

	int index = 0;
	char test[1024];
	char *tmp;
	char err_buf[80];
	char *file_begin;
	char *bufp;
	boolean_t found = B_FALSE;
	int  saverrno = 0;
	int begin_sz;
	char special[128];
	int i;

	Trace(TR_OPRMSG, "reading beginning of vfstab file");

	if (ISNULL(f, ent, file_beginp)) {
		return (-1);
	}

	*file_beginp = NULL;

	file_begin = mallocer(INCREMENT);
	if (file_begin == NULL) {
		return (-1);
	}
	begin_sz = INCREMENT;

	file_begin[0] = '\0';

	/* search for the entry copying everything up to it into file_begin. */
	while ((fgets(test, sizeof (test), f)) != NULL) {

		/* eat whitespace before comparing */
		bufp = test;
		while (isspace(*bufp)) {
			bufp++;
		}

		/* grab just the device special name */
		for (i = 0; i < (sizeof (special) - 1); i++) {
			if (!isspace(bufp[i])) {
				special[i] = bufp[i];
			} else {
				break;
			}
		}
		special[i] = '\0';

		/*
		 * avoid false failures due to whitespace, and false
		 * matches on substrings/superstrings
		 */
		if (strcmp(special, ent->vfs_special) == 0) {
			/* found the beginning of the entry  */
			file_begin[index] = '\0';
			found = B_TRUE;

			break;
		}

		/* Make space in file_begin if needed */
		if ((index + strlen(test) + 1) >= (begin_sz + 1)) {
			tmp = (char *)realloc(file_begin,
			    begin_sz + INCREMENT);

			if (tmp == NULL) {
				setsamerr(SE_NO_MEM);
				goto err;
			}

			file_begin = tmp;

			begin_sz += INCREMENT;
		}

		/* write test into file_begin */
		index += snprintf(file_begin + index, begin_sz - index,
		    "%s", test);
	}
	saverrno = errno;

	if (!found) {
		if (saverrno != 0) {
			samerrno = SE_IO_OPERATION_FAILED;

			/* IO operation %s failed: %s */
			StrFromErrno(errno, err_buf, sizeof (err_buf));
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_IO_OPERATION_FAILED), "fgets",
			    err_buf);
		} else {
			/* no errors, but entry was not found */
			samerrno = SE_UNEXPECTED_ERROR;
			snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
			    "entry not found");
		}
		goto err;
	}

	*file_beginp = file_begin;

	Trace(TR_OPRMSG, "read beginning of vfstab file");
	return (0);

err:
	free(file_begin);
	Trace(TR_OPRMSG, "reading beginning of vfstab file failed %s",
	    samerrmsg);
	return (-1);
}


/*
 * store the rest of the file in a char array for subsequent writeback.
 */
static int
_read_remaining(
FILE *f,
fpos_t *pos,			/* position to read from */
char   **file_endp)		/* buffer to hold file data */
{

	char c;
	int index = 0;
	char *tmp;
	char err_buf[80];
	char *file_end;
	int end_sz;

	if (ISNULL(f, pos, file_endp)) {
		return (-1);
	}

	Trace(TR_OPRMSG, "reading remainder of vfstab file");

	*file_endp = NULL;

	file_end = mallocer(INCREMENT);
	if (file_end == NULL) {
		return (-1);
	}

	end_sz = INCREMENT;
	file_end[0] = '\0';

	if (fsetpos(f, pos) != 0) {
		samerrno = SE_IO_OPERATION_FAILED;

		/* IO operation %s failed: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "fsetpos", err_buf);

		goto err;
	}

	/* copy the rest of the file into file_end */
	while (EOF != (c = fgetc(f))) {
		if (index == end_sz) {
			tmp = (char *)realloc(file_end, end_sz + INCREMENT);

			if (tmp == NULL) {
				setsamerr(SE_NO_MEM);
				goto err;
			}
			file_end = tmp;

			end_sz += INCREMENT;
		}
		file_end[index++] = c;
	}
	file_end[index] = '\0';


	if (fsetpos(f, pos) != 0) {
		samerrno = SE_IO_OPERATION_FAILED;

		/* IO operation %s failed: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "fsetpos", err_buf);
		goto err;
	}

	*file_endp = file_end;

	Trace(TR_OPRMSG, "read remainder of vfstab file");
	return (0);

err:
	free(file_end);
	Trace(TR_OPRMSG, "reading remainder of vfstab file failed %s",
	    samerrmsg);
	return (-1);
}


/* Change the vfstqab entry without changing the order in the file */
int
update_vfstab_entry(
vfstab_entry_t *ent)	/* entry to update */
{

	Trace(TR_MISC, "updating vfstab entry");

	if (_modify_vfstab_entry(ent) != 0) {
		Trace(TR_ERR, "updating vfstab entry failed %s",
		    samerrmsg);
		return (-1);
	}

	Trace(TR_MISC, "updated vfstab entry");
	return (0);
}


int
_modify_vfstab_entry(
vfstab_entry_t *input)	/* entry to update */
{

	FILE *f = NULL;
	int fd;
	fpos_t aft_pos;
	int error = 0;
	struct vfstab cur_ent, new_ent;
	char working_copy[MAXPATHLEN+1] = {0};
	char err_buf[80];
	char *file_end = NULL;
	char *file_begin = NULL;

	/* set things up so that freeing will work on errors */
	memset(&new_ent, 0, sizeof (struct vfstab));
	memset(&cur_ent, 0, sizeof (struct vfstab));

	/*
	 * create a struct vfstab from the vfstab_entry_t. Do this first
	 * because if there are any problems, there is no point in continuing.
	 */
	if (mk_vfstab(input, &new_ent) != 0) {
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		return (-1);
	}


	f = fopen(VFSTAB, "r");
	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;
		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), VFSTAB, err_buf);
		goto err;
	}

	/* find the entry */
	error = getvfsspec(f, &cur_ent, input->fs_name);
	if (error != 0) {
		set_samerrno_for_vfstab_error(input->fs_name, error);
		goto err;
	}

	/*
	 * The current position after finding the entry is after the
	 * entry of interest. So get all of remaining file contents from
	 * that point on.
	 */
	if (fgetpos(f, &aft_pos) != 0) {
		samerrno = SE_IO_OPERATION_FAILED;

		/* IO operation %s failed: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "fgetpos", err_buf);
		fclose(f);
		goto err;
	}

	if (_read_remaining(f, &aft_pos, &file_end) != 0) {

		/* leave samerrno as set */
		fclose(f);
		goto err;
	}

	/* get the beginning of the file up to the entry */
	errno = 0;
	rewind(f);
	if (errno != 0) {
		StrFromErrno(errno, err_buf, sizeof (err_buf));

		fclose(f);

		/* IO operation %s failed: %s */
		samerrno = SE_IO_OPERATION_FAILED;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "rewind", err_buf);
		goto err;
	}

	if (_read_beginning(f, &cur_ent, &file_begin) != 0) {
		fclose(f);
		goto err;
	}

	/* close the file */
	fclose(f);

	/* Make Backup copy backup_path/vfstab.src_file's_mod_time */
	if (backup_cfg(VFSTAB) != 0) {
		goto err;
	}


	/*
	 * create a new copy of the file, write in the beginning, add the
	 * modified entry and then write out the end.
	 */
	if (mk_wc_path(VFSTAB, working_copy, sizeof (working_copy)) != 0) {
		goto err;
	}


	f = NULL;
	if ((fd = open(working_copy, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}
	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), working_copy, err_buf);
		goto err;
	}

	/* write the beginning of the file */
	if (file_begin != NULL) {
		fprintf(f, "%s", file_begin);
		fflush(f);
		free(file_begin);
		file_begin = NULL;
	}

	/* write the entry to the file */
	errno = 0;
	if (putvfsent(f, &new_ent) < 0) {
		samerrno = SE_IO_OPERATION_FAILED;

		/* IO operation %s failed: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "fprintf", err_buf);

		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		fclose(f);
		goto err;
	}

	/* write the end of the file */
	if (file_end != NULL) {
		fprintf(f, "%s", file_end);
		fflush(f);
		free(file_end);
		file_end = NULL;
	}

	fclose(f);

	/* copy the working copy back to vfstab */
	if (cp_file(working_copy, VFSTAB) != 0) {
		goto err;
	}

	/* log the config change */
	Trace(TR_FILES, "modified %s entry in %s", input->fs_name,
	    VFSTAB);

	if (backup_cfg(VFSTAB) != 0) {
		goto err;
	}

	/*
	 * free the malloced data- be sure not to free the fields
	 * of the cur_ent structure since it was fetched using
	 * getvfsspec which does not allocate space but instead
	 * uses static memory
	 */
	unlink(working_copy);
	free_vfstab_fields(&new_ent);

	Trace(TR_OPRMSG, "modified vfstab entry for %s", input->fs_name);
	return (0);

err:

	/*
	 * free the malloced data- be sure not to free the fields
	 * of the cur_ent structure since it was fetched using
	 * getvfsspec which does not allocate space but instead
	 * uses static memory
	 */
	free(file_end);
	free(file_begin);

	free_vfstab_fields(&new_ent);

	if (working_copy[0] != '\0') {
		unlink(working_copy);
	}

	Trace(TR_ERR, "modifying vfstab entry for %s failed %d %s",
	    input->fs_name, samerrno, samerrmsg);
	return (-1);
}

/*
 * add the vfstab entry to the vfstab file. Returns error if entry is already
 * present.
 */
int
add_vfstab_entry(
vfstab_entry_t *input)	/* entry to add */
{

	Trace(TR_MISC, "adding vfstab entry");

	if (_add_vfstab_entry(input, B_FALSE) == 0) {
		Trace(TR_FILES, "wrote %s to vfstab file %s", input->fs_name,
		    VFSTAB);
		Trace(TR_MISC, "added vfstab entry");
		return (0);
	} else {
		Trace(TR_ERR, "adding vfstab entry failed %s", samerrmsg);
		return (-1);
	}
}


/*
 * If from_working_copy is true:
 *	1. don't make a pre-backup.
 *	2. write to the existing working copy don't cpy a new one over.
 */
static int
_add_vfstab_entry(
vfstab_entry_t *input,		/* entry to add */
boolean_t from_working_copy)	/* if true use existing working copy */
{

	struct vfstab tab_ent, discard_ent;
	FILE *f = NULL;
	int fd;
	int err = 0;
	char working_copy[MAXPATHLEN+1] = {0};
	char err_buf[80];

	Trace(TR_OPRMSG, "adding vfstab entry");

	if (from_working_copy) {
		Trace(TR_OPRMSG,
		"_add_vfstab_entry with from_working_copy set to TRUE, fail.");
		return (-1);
	}

	if (ISNULL(input)) {
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		return (-1);
	}

	if (mk_vfstab(input, &tab_ent) != 0) {
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		return (-1);
	}

	/*
	 * copy the vfstab to the working file
	 * open the copy and write out file.
	 */
	if (make_working_copy(VFSTAB, working_copy,
	    sizeof (working_copy)) != 0) {

		free_vfstab_fields(&tab_ent);
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		return (-1);
	}

	if ((fd = open(working_copy, O_RDWR|O_CREAT, 0644)) != -1) {
		f = fdopen(fd, "r+");
	}

	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), VFSTAB, err_buf);
		free_vfstab_fields(&tab_ent);
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);

		unlink(working_copy);
		return (-1);
	}

	/*
	 * make sure fs does not already exist
	 *
	 * err == -1 is the desired case
	 */
	if ((err = getvfsspec(f, &discard_ent, input->fs_name)) == 0) {
		samerrno = SE_ALREADY_EXISTS;

		/* %s already exists */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_ALREADY_EXISTS),
		    input->fs_name);

		Trace(TR_OPRMSG,
		    "adding vfstab entry failed %s",
		    samerrmsg);
		free_vfstab_fields(&tab_ent);
		fclose(f);
		unlink(working_copy);
		return (-1);
	}

	/* make backup copy */
	if (backup_cfg(VFSTAB) != 0) {
		free_vfstab_fields(&tab_ent);
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		free_vfstab_fields(&tab_ent);
		fclose(f);
		unlink(working_copy);
		return (-1);
	}

	/*
	 * go to end of file and
	 *	a) check that it's terminated with a newline
	 *	b) add the new entry
	 */
	if (fseek(f, -1, SEEK_END) != 0) {
		samerrno = SE_IO_OPERATION_FAILED;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "fseek SEEK_END",
		    err_buf);
		free_vfstab_fields(&tab_ent);
		fclose(f);
		unlink(working_copy);
		return (-1);
	}

	if (fgetc(f) != '\n') {
		err = 1;
	}

	errno = 0;

	if (fseek(f, 0, SEEK_END) == 0) {
		if (err == 1) {
			fputc('\n', f);
		}
	}
	/* trap errors from fseek & fputc */
	err = errno;

	if (err == 0) {
		/* add the actual entry - macro from vfstab.h */
		errno = 0;
		if (putvfsent(f, &tab_ent) < 0) {
			err = errno;
		}
	}

	if (err != 0) {
		samerrno = SE_IO_OPERATION_FAILED;

		/* IO operation %s failed: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_IO_OPERATION_FAILED), "fprintf", err_buf);

		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		free_vfstab_fields(&tab_ent);
		fclose(f);
		unlink(working_copy);
		return (-1);
	}
	fflush(f);
	fclose(f);

	free_vfstab_fields(&tab_ent);


	if (cp_file(working_copy, VFSTAB) != 0) {
		/* unable to complete the modification */
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		unlink(working_copy);
		return (-1);
	}
	unlink(working_copy);

	/* Make Backup copy backup_path/vfstab.src_file's_mod_time */
	if (backup_cfg(VFSTAB) != 0) {
		Trace(TR_OPRMSG, "adding vfstab entry failed %s", samerrmsg);
		return (-1);
	}

	return (0);
}

/*
 * given a struct vfstab create a vfstab_entry_t
 */
static int
mk_vfstab_entry(
struct vfstab *ent,	/* struct to translate */
vfstab_entry_t **out)	/* malloced return */
{

	char *p = NULL;
	long val;

	Trace(TR_DEBUG, "mk vfstab entry");
	*out = (vfstab_entry_t *)mallocer(sizeof (vfstab_entry_t));
	if (*out == NULL) {
		Trace(TR_DEBUG, "mk vfstab entry failed %s", samerrmsg);
		return (-1);
	}

	memset(*out, 0, sizeof (vfstab_entry_t));
	(*out)->fsck_pass = -1;

	if (ent->vfs_special != NULL) {
		(*out)->fs_name = copystr(ent->vfs_special);
		if ((*out)->fs_name == NULL) {
			goto err;
		}
	}



	if (ent->vfs_fsckdev != NULL) {
		(*out)->device_to_fsck = copystr(ent->vfs_fsckdev);
		if ((*out)->device_to_fsck == NULL) {
			goto err;
		}
	}

	if (ent->vfs_mountp != NULL) {
		(*out)->mount_point = copystr(ent->vfs_mountp);
		if ((*out)->mount_point == NULL) {
			goto err;
		}
	}

	if (ent->vfs_fstype != NULL) {
		(*out)->fs_type = copystr(ent->vfs_fstype);
		if ((*out)->fs_type == NULL) {
			goto err;
		}
	}


	if (ent->vfs_fsckpass != NULL) {
		errno = 0;
		val = strtol(ent->vfs_fsckpass, &p, 0);

		if (errno != 0) {
			samerrno = SE_INVALID_FSCKPASS;

			/* Invalid fsckpass value %s */
			snprintf(samerrmsg, MAX_MSG_LEN,
			    GetCustMsg(SE_INVALID_FSCKPASS),
			    ent->vfs_fsckpass);

			goto err;
		}

		(*out)->fsck_pass = (int)val;

	} else {
		(*out)->fsck_pass = -1;
	}


	if (ent->vfs_automnt != NULL) {
		if (strcmp(ent->vfs_automnt, "yes") == 0) {
			(*out)->mount_at_boot = B_TRUE;
		} else {
			(*out)->mount_at_boot = B_FALSE;
		}
	} else {
		(*out)->mount_at_boot = B_FALSE;
	}

	if (ent->vfs_mntopts != NULL) {
		(*out)->mount_options = copystr(ent->vfs_mntopts);
		if ((*out)->mount_options == NULL) {
			goto err;
		}
	}

	Trace(TR_DEBUG, "made vfstab entry");
	return (0);

err:
	free_vfstab_entry(*out);
	Trace(TR_DEBUG, "mk vfstab entry failed %s", samerrmsg);
	return (-1);
}


/*
 * make a vfstab struct for the vfstab_entry_t.
 * note that total length includes the spaces and dashes too.
 */
static int
mk_vfstab(
vfstab_entry_t *in,	/* struct to translate */
struct vfstab *out)	/* struct to populate with translated data */
{

	char number[25];
	int total_length = 0;
	int size = 0;

	Trace(TR_DEBUG, "make vfstab struct");
	if (ISNULL(in, out)) {
		Trace(TR_DEBUG, "make vfstab struct failed %s", samerrmsg);
		return (-1);
	}

	/* set out to zeros so freeing can work in case of errors. */
	memset(out, 0, sizeof (struct vfstab));


	/* check the required fields first */
	if (in->fs_name == NULL) {
		samerrno = SE_REQUIRED_FIELD_MISSING;

		/* Required field %s is missing */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_REQUIRED_FIELD_MISSING), "fs_name");

		goto err;
	}

	if (in->fs_type == NULL) {
		samerrno = SE_REQUIRED_FIELD_MISSING;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_REQUIRED_FIELD_MISSING), "fs_type");

		goto err;
	}

	/* required fields existed so check for spaces and make them */
	if (has_spaces(in->fs_name, "fs_name") != 0) {
		goto err;
	}

	size = strlen(in->fs_name) + 1;
	out->vfs_special = (char *)mallocer(size);
	if (out->vfs_special == NULL) {
		goto err;
	}

	strcpy(out->vfs_special, in->fs_name);
	total_length += size;


	if (has_spaces(in->fs_type, "fs_type") != 0) {
		goto err;
	}

	size = strlen(in->fs_type) + 1;
	out->vfs_fstype = (char *)mallocer(size);

	if (out->vfs_fstype == NULL) {
		goto err;
	}

	strcpy(out->vfs_fstype, in->fs_type);
	total_length += size;


	if (in->device_to_fsck == NULL) {
		out->vfs_fsckdev = NULL;
		total_length += 2;
	} else {

		if (has_spaces(in->device_to_fsck, "device_to_fsck") != 0) {
			goto err;
		}

		size = strlen(in->device_to_fsck) + 1;
		out->vfs_fsckdev = (char *)mallocer(size);

		if (out->vfs_fsckdev == NULL) {
			goto err;
		}

		strcpy(out->vfs_fsckdev, in->device_to_fsck);
		total_length += size;
	}

	if (in->mount_point == NULL) {
		out->vfs_mountp = NULL;
		total_length += 2;
	} else {
		if (has_spaces(in->mount_point, "mount_point") != 0) {
			goto err;
		}

		size = strlen(in->mount_point) + 1;
		out->vfs_mountp = copystr(in->mount_point);
		if (out->vfs_mountp == NULL) {
			goto err;
		}

		total_length += size;
	}

	if (in->fsck_pass < 0) {
		out->vfs_fsckpass = NULL;
		total_length += 2;
	} else {
		sprintf(number, "%d", in->fsck_pass);

		size = strlen(number) + 1;
		out->vfs_fsckpass = (char *)mallocer(size);

		if (out->vfs_fsckpass == NULL) {
			goto err;
		}

		strcpy(out->vfs_fsckpass, number);
		total_length += size;
	}

	if (!in->mount_at_boot) {
		out->vfs_automnt = (char *)mallocer(strlen("no") + 1);
		if (out->vfs_automnt == NULL) {
			goto err;
		}

		strcpy(out->vfs_automnt, "no");
		total_length += 3;

	} else {
		out->vfs_automnt = (char *)mallocer(strlen("yes") + 1);
		if (out->vfs_automnt == NULL) {
			goto err;
		}

		strcpy(out->vfs_automnt, "yes");
		total_length += 4;
	}

	if (in->mount_options == NULL) {
		out->vfs_mntopts = NULL;
		total_length += 2;
	} else {

		if (has_spaces(in->mount_options, "mount_options") != 0) {
			goto err;
		}
		out->vfs_mntopts = (char *)mallocer(
		    strlen(in->mount_options) + 1);

		if (out->vfs_mntopts == NULL) {
			goto err;
		}
		strcpy(out->vfs_mntopts, in->mount_options);
		total_length += size;
	}

	/* ++total length for the \n */
	++total_length;

	/* ++total length for eof */
	if (++total_length >= (VFS_LINE_MAX)) {

		samerrno = SE_MAX_LENGTH_EXCEEDED;

		/* Maximum length of %d exceeded for %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_MAX_LENGTH_EXCEEDED),
		    VFS_LINE_MAX, "vfstab entry");

		goto err;
	}


	Trace(TR_DEBUG, "made vfstab struct");
	return (0);

err:
	free_vfstab_fields(out);
	Trace(TR_DEBUG, "make vfstab struct failed %s", samerrmsg);
	return (-1);
}


/*
 * free a vfstab_entry
 */
void
free_vfstab_entry(vfstab_entry_t *ent)
{

	if (ent == NULL)
		return;

	if (ent->fs_name != NULL) {
		free(ent->fs_name);
		ent->fs_name = NULL;
	}
	if (ent->device_to_fsck != NULL) {
		free(ent->device_to_fsck);
		ent->device_to_fsck = NULL;
	}
	if (ent->mount_point != NULL) {
		free(ent->mount_point);
		ent->mount_point = NULL;
	}
	if (ent->fs_type != NULL) {
		free(ent->fs_type);
		ent->fs_type = NULL;
	}
	if (ent->mount_options != NULL) {
		free(ent->mount_options);
		ent->mount_options = NULL;
	}
	free(ent);
}


/*
 * extract the mount options from the vfstab_entry_t
 */
int
add_mnt_opts_from_vfsent(
vfstab_entry_t *vfs_ent,	/* the entry to get options from */
mount_options_t *opts)
{

	char	*name;
	char	*mnt_str;
	char	*rest;
	uint32_t io_flag;
	uint32_t sam_flag;
	uint32_t share_flag;
	uint32_t multireader_flag;
	uint32_t qfs_flag;
	uint32_t general_flag;
	uint32_t post42_flag;
	uint32_t rel46_flag;

	Trace(TR_OPRMSG, "adding mount options from vfsent");

	/* if the vfstab_entry has no options return success */
	if (vfs_ent->mount_options == NULL) {
		return (0);
	}


	/*
	 * save all flag values and set flags to zero so duplicate checks will
	 * pass in setfield
	 */
	io_flag = opts->io_opts.change_flag;
	opts->io_opts.change_flag = 0;

	sam_flag = opts->sam_opts.change_flag;
	opts->sam_opts.change_flag = 0;

	share_flag = opts->sharedfs_opts.change_flag;
	opts->sharedfs_opts.change_flag = 0;

	multireader_flag = opts->multireader_opts.change_flag;
	opts->multireader_opts.change_flag = 0;

	qfs_flag = opts->qfs_opts.change_flag;
	opts->qfs_opts.change_flag = 0;

	general_flag = opts->change_flag;
	opts->change_flag = 0;

	post42_flag = opts->post_4_2_opts.change_flag;
	opts->post_4_2_opts.change_flag = 0;

	rel46_flag = opts->rel_4_6_opts.change_flag;
	opts->rel_4_6_opts.change_flag = 0;

	/*
	 * dup the mnt_str so strtok can be used without ruining
	 * the input.
	 */
	mnt_str = strdup(vfs_ent->mount_options);
	if (mnt_str == NULL) {
		Trace(TR_OPRMSG, "adding mount options from vfsent failed %s",
		    "no error defined");
		return (-1);
	}

	/* tokenize on , to parse all options */
	name = strtok_r(mnt_str, ",", &rest);
	while (name != NULL) {
		char	*value;

		if ((value = strchr(name, '=')) != NULL) {
			*value++ = '\0';
		}

		if (SetFieldValue(opts, cfg_mount_params, name, value,
		    msg_func) != 0) {
			free(mnt_str);
			samerrno = -1;
			snprintf(samerrmsg, MAX_MSG_LEN,
			    "Setfield failed with invalid mount option:%s",
			    err_msg);

			Trace(TR_OPRMSG,
			    "adding mount options from vfsent%s %s = %s",
			    " setfield failed for ", Str(name), Str(value));
			return (-1);
		}
		name = strtok_r(NULL, ",", &rest);
	}

	/*
	 * Or together the saved flags and new values from vfstab
	 */
	opts->io_opts.change_flag |= io_flag;
	opts->sam_opts.change_flag |= sam_flag;
	opts->sharedfs_opts.change_flag |= share_flag;
	opts->multireader_opts.change_flag |= multireader_flag;
	opts->qfs_opts.change_flag |= qfs_flag;
	opts->change_flag |= general_flag;
	opts->post_4_2_opts.change_flag |= post42_flag;
	opts->rel_4_6_opts.change_flag |= rel46_flag;

	free(mnt_str);
	Trace(TR_OPRMSG, "added mount options from vfsent");
	return (0);
}


static void
msg_func(
int code	/* ARGSUSED */,
char *msg)	/* the error message */
{

	strlcpy(err_msg, msg, sizeof (err_msg));
}


/*
 * set_vfstab_options.
 *
 * This function does NOT set all of the options. Instead it favors using
 * the samfs.cmd file.
 *
 * If something has a value in the vfstab it will have its value reset
 * in the vfstab. If a value is being reset to defaults there will be no value
 * in the vfstab after the set_vfstab_options is called. Any value that does
 * not have an entry in the vfstab prior to this call will not after it with
 * the exception of any field that MUST have its value set in the vfstab. This
 * includes shared.
 */
int
set_vfstab_options(
char *fs_name,		/* fs for which to set options */
mount_options_t *input)	/* options to set */
{

	struct fieldVals *table = cfg_mount_params;
	int32_t *fp = flag_pairs;
	vfstab_entry_t *vfs_ent;
	char buf[100];
	char vfs_opt_str[VFS_LINE_MAX];
	boolean_t ret;

	uint32_t input_bits; /* the flag for the input */
	uint32_t vfs_bits; /* the flag from vfstab */
	mount_options_t vfs_opts;


	Trace(TR_OPRMSG, "setting vfstab mount options for %s", Str(fs_name));
	if (ISNULL(fs_name, input)) {
		Trace(TR_OPRMSG, "setting vfstab mount options failed %s",
		    samerrmsg);
		return (-1);
	}

	/* They must exist or it is an error. */
	if (get_vfstab_entry(fs_name, &vfs_ent) != 0) {
		Trace(TR_OPRMSG, "setting vfstab mount options failed %s",
		    samerrmsg);
		return (-1);
	}

	/* get the current vfstab options. */
	memset(&vfs_opts, 0, sizeof (vfs_opts));
	if (add_mnt_opts_from_vfsent(vfs_ent, &vfs_opts) != 0) {
		free_vfstab_entry(vfs_ent);
		Trace(TR_OPRMSG, "setting vfstab mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	*vfs_opt_str = '\0';

	/* merge following the above rules. */
	while (table->FvName != NULL) {

		if (table->FvType == DEFBITS) {
			/* get defbits for input and vfstab */

			input_bits = *(uint32_t *)(void *)(((char *)
			    input) + table->FvLoc);

			vfs_bits = *(uint32_t *)(void *)(((char *)
			    &vfs_opts) + table->FvLoc);

			table++;
			fp++;
			continue;
		}

		/* skip entry if the input's flag is not set. */
		if (is_explicit_set(input_bits, table) != B_TRUE) {
			table++;
			fp++;
			continue;
		}


		/*
		 * We now know input had its flag set so...
		 * skip entry if vfstab is not set and its paired flag is not
		 * set set unless this is a mandatory vfstab entry option
		 */
		if ((is_explicit_set(vfs_bits, table) == B_FALSE &&
		    !(vfs_bits & *fp)) &&
		    is_mandatory_vfs_entry(table) == B_FALSE) {

			table++;
			fp++;
			continue;
		}

		/*
		 * finally skip entry if input has the reset value for the
		 * entry
		 */
		if (is_reset_value(input_bits, table, &ret) != 0) {
			return (-1);
		}

		if (ret == B_TRUE) {
			table++;
			fp++;
			continue;
		}

		/* if you are here the option needs to go into the vfstab */
		if (NULL == get_cfg_str(input, table, B_FALSE, buf, 100,
		    B_TRUE, non_printing_mount_options)) {
			table++;
			fp++;
			continue;
		}
		if (*buf == '\0') {
			table++;
			fp++;
			continue;
		}

		if (strlen(vfs_opt_str) == 0) {
			strlcat(vfs_opt_str, buf, sizeof (vfs_opt_str));
		} else {
			strlcat(vfs_opt_str, VFS_OPT_SEPARATOR,
			    sizeof (vfs_opt_str));
			strlcat(vfs_opt_str, buf, sizeof (vfs_opt_str));
		}

		table++;
		fp++;
	}

	/*
	 * If there are any mount options for the vfstab
	 * set the vfstab entry's mount_options otherwise set NULL to clear
	 * any existing options.
	 */
	if (strlen(vfs_opt_str) == 0) {
		free(vfs_ent->mount_options);
		vfs_ent->mount_options = NULL;
	} else {
		free(vfs_ent->mount_options);
		vfs_ent->mount_options = vfs_opt_str;
	}

	if (update_vfstab_entry(vfs_ent) != 0) {
		vfs_ent->mount_options = NULL;
		free_vfstab_entry(vfs_ent);
		Trace(TR_OPRMSG, "setting vfstab mount options failed: %s",
		    samerrmsg);
		return (-1);
	}

	free_vfstab_entry(vfs_ent);
	Trace(TR_OPRMSG, "set vfstab mount options");
	return (0);
}


/*
 * returns true if the mount option described by fieldVals MUST appear in the
 * vfstab if it is set.
 */
static boolean_t
is_mandatory_vfs_entry(
struct fieldVals *entry)	/* struct describing a particular entry */
{

	int i;

	for (i = 0; *mandatory_fields[i] != '\0'; i++) {
		if (strcmp(entry->FvName, mandatory_fields[i]) == 0) {
			return (B_TRUE);
		}
	}

	return (B_FALSE);
}


/*
 * free the dynamically allocated fields of a struct vfstab
 *
 * NOTE: This method should NOT be used on the vfstab structs
 * returned by the sys/vfstab.h methods! Their fields are NOT
 * dynamically allocated.
 */
static void
free_vfstab_fields(
struct vfstab *ent)
{

	Trace(TR_ALLOC, "freeing vfstab fields");

	if (ent == NULL)
		return;

	if (ent->vfs_special != NULL) {
		free(ent->vfs_special);
		ent->vfs_special = NULL;
	}

	if (ent->vfs_fsckdev != NULL) {
		free(ent->vfs_fsckdev);
		ent->vfs_fsckdev = NULL;
	}
	if (ent->vfs_mountp != NULL) {
		free(ent->vfs_mountp);
		ent->vfs_mountp = NULL;
	}

	if (ent->vfs_fstype != NULL) {
		free(ent->vfs_fstype);
		ent->vfs_fstype = NULL;
	}

	if (ent->vfs_fsckpass != NULL) {
		free(ent->vfs_fsckpass);
		ent->vfs_fsckpass = NULL;
	}

	if (ent->vfs_automnt != NULL) {
		free(ent->vfs_automnt);
		ent->vfs_automnt = NULL;
	}

	if (ent->vfs_mntopts != NULL) {
		free(ent->vfs_mntopts);
		ent->vfs_mntopts = NULL;
	}

	Trace(TR_ALLOC, "freed vfstab fields");
}


/*
 * for use when multiple vfstab entries will need to be examined by the
 * calling code.
 */
int
get_all_vfstab_entries(sqm_lst_t **l) {
	FILE *f;
	struct vfstab tmp_ent;
	char err_buf[80];
	int err = 0;

	Trace(TR_DEBUG, "getting all vfstab entries");
	if (ISNULL(l)) {
		return (-1);
	}

	*l = lst_create();
	if (*l == NULL) {
		return (-1);
	}

	f = fopen(VFSTAB, "r");
	if (NULL == f) {
		samerrno = SE_CFG_OPEN_FAILED;

		/* Open failed for %s: %s */
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
			GetCustMsg(SE_CFG_OPEN_FAILED), VFSTAB, err_buf);

		goto err;
	}

	while ((err = getvfsent(f, &tmp_ent)) != -1) {
		vfstab_entry_t *tmp;
		/*
		 * If there is an error for the vfstab entry
		 * trace it but skip to next.
		 */
		if (err > 0) {
			Trace(TR_MISC,
			    "vfstab error for line begining = %s %d",
			    Str(tmp_ent.vfs_special), err);
			continue;
		}
		if (mk_vfstab_entry(&tmp_ent, &tmp) != 0) {
			fclose(f);
			goto err;
		}
		lst_append(*l, tmp);
	}

	fclose(f);
	Trace(TR_DEBUG, "got vfstab entries");
	return (0);

err:
	lst_free_deep_typed(*l, FREEFUNCCAST(free_vfstab_entry));
	*l = NULL;
	Trace(TR_OPRMSG, "getting vfstab entries failed %s", samerrmsg);
	return (-1);
}
