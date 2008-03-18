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
#pragma ident   "$Revision: 1.11 $"

static char *_SrcFile = __FILE__;

#include <strings.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>

#include "sam/sam_trace.h"

#include "mgmt/util.h"
#include "parser_utils.h"
#include "mgmt/config/common.h"
#include "mgmt/config/data_class.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/config/archiver.h"
#include "pub/mgmt/archive.h"


/* externs used here defined elsewhere */
extern char *StrFromInterval(int interval, char *buf, int buf_size);

static int write_class_file(sqm_lst_t *class_list);
static int update_class_struct(ar_set_criteria_t *class,
    const ar_set_criteria_t *in);

int
get_data_classes(
ctx_t *c	/* ARGSUSED */,
sqm_lst_t **l)
{

	Trace(TR_OPRMSG, "getting data classes");
	if (ISNULL(l)) {
		Trace(TR_ERR, "getting data classes failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if (parse_class_file(CLASS_FILE, l) != 0) {
		Trace(TR_ERR, "getting data classes failed %d %s",
		    samerrno, samerrmsg);
		*l = NULL;
		return (-1);
	}

	Trace(TR_OPRMSG, "got data classes");

	return (0);
}


int
remove_data_class(
ctx_t *c	/* ARGSUSED */,
char *name)
{


	sqm_lst_t *l;
	node_t *n;
	boolean_t found = B_FALSE;

	if (ISNULL(name)) {
		Trace(TR_ERR, "remove data class failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	Trace(TR_OPRMSG, "removing data class %s", name);

	if (get_data_classes(NULL, &l) != 0) {
		Trace(TR_ERR, "remove data class failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	for (n = l->head; n != NULL; n = n->next) {
		ar_set_criteria_t *c = (ar_set_criteria_t *)n->data;
		if (strcmp(c->class_name, name) == 0) {
			if (lst_remove(l, n) != 0) {
				free_ar_set_criteria_list(l);
				Trace(TR_ERR,
				    "remove data class failed %d %s",
				    samerrno, samerrmsg);

				return (-1);
			}
			found = B_TRUE;
			break;
		}
	}

	if (!found) {
		free_ar_set_criteria_list(l);
		samerrno = SE_NOT_FOUND;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_NOT_FOUND), name);
		Trace(TR_ERR, "remove data class failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	if (write_class_file(l) != 0) {
		free_ar_set_criteria_list(l);
		Trace(TR_ERR, "remove data class failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}
	Trace(TR_OPRMSG, "removed data class %s", name);
	free_ar_set_criteria_list(l);
	return (0);
}


/*
 * Returns a pointer into the list of classes. Do not free the returned
 * criteria separately.
 */
int
find_class_by_name(
char *class_name,
sqm_lst_t *classes,
ar_set_criteria_t **data_class)
{

	node_t *n;

	/* look for the class by name */
	for (n = classes->head; n != NULL; n = n->next) {
		ar_set_criteria_t *cr = (ar_set_criteria_t *)n->data;
		if (strncmp(cr->class_name, class_name,
		    sizeof (cr->class_name)) == 0) {
			*data_class = cr;
			return (0);
		}
	}

	samerrno = SE_NOT_FOUND;
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND), class_name);
	return (-1);
}


/*
 * Returns a pointer into the list of classes. Do not free the returned
 * criteria separately.
 */
int
find_class_by_criteria(
ar_set_criteria_t *cr,
sqm_lst_t *classes,
ar_set_criteria_t **data_class)
{

	node_t *n;

	for (n = classes->head; n != NULL; n = n->next) {
		ar_set_criteria_t *class = (ar_set_criteria_t *)n->data;
		if (class_cmp(cr, class) == 0) {
			*data_class = class;
			return (0);
		}
	}

	samerrno = SE_NOT_FOUND;
	snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(SE_NOT_FOUND), "class");
	return (-1);
}


/*
 * compare two classes based only on their criteria. This does not include
 * checking if they apply to the same file system or the same policies.
 */
int
class_cmp(
ar_set_criteria_t *cr1,
ar_set_criteria_t *cr2)
{

	char buf1[1024], buf2[1024];

	if (get_class_str(cr1, B_TRUE, buf1, 1024) != 0) {
		return (-1);
	}
	if (get_class_str(cr2, B_TRUE, buf2, 1024) != 0) {
		return (-1);
	}

	if (strcmp(buf1, buf2) == 0) {
		return (0);
	}
	return (1);
}


/*
 * This function takes a string and a regular expression type. It
 * applies the rules for the type to the input string to come up with
 * a regexp acceptable in the archiver.cmd. If the input string can't
 * be made into a regexp of the type indicated (e.g. the type is not
 * valid), the results buffer will simply contain the input string.
 */
int
compose_regex(
char		*str,
regexp_type_t	type,
char		*res,
int		res_sz)
{


	*res = '\0';

	if (type == ENDS_WITH) {
		if (*str == '.') {
			strlcat(res, "\\", res_sz);
		}
		strlcat(res, str, res_sz);
		strlcat(res, ENDS_WITH_SUFFIX, res_sz);

	} else if (type == FILE_NAME_CONTAINS) {
		strlcat(res, str, res_sz);
		strlcat(res, FILE_CONTAINS_SUFFIX, res_sz);

	} else if (type == REGEXP || type == PATH_CONTAINS) {
		strlcat(res, str, res_sz);

	} else {
		/*
		 * This could return -1 but it is safer to simply treat as
		 * a generic regular expression.
		 */
		Trace(TR_OPRMSG, "Unknown regular expression type: %d", type);
		strlcat(res, str, res_sz);
	}

	return (0);
}


static int
update_class_struct(
ar_set_criteria_t	*class,
const ar_set_criteria_t	*in)
{

	/*
	 * handling for classes that apply to multiple file systems
	 * is not yet implemented here
	 */

	memcpy(class, in, sizeof (ar_set_criteria_t));

	/*
	 * set the pointers to NULL so freeing the class won't
	 * cause the copies pointed to by the criteria to be freed.
	 */
	class->arch_copy[0] = NULL;
	class->arch_copy[1] = NULL;
	class->arch_copy[2] = NULL;
	class->arch_copy[3] = NULL;
	if (in->description != NULL) {
		class->description = strdup(in->description);
		if (class->description == NULL) {
			setsamerr(SE_NO_MEM);
			return (-1);
		}
	}

	return (0);
}


/*
 * this function will add the class if it is not already present
 */
int
update_data_class(
ctx_t *c	/* ARGSUSED */,
ar_set_criteria_t *in)
{

	sqm_lst_t *l;
	int retval;
	ar_set_criteria_t *class;

	Trace(TR_OPRMSG, "updating data class %s", in->class_name);

	if (get_data_classes(NULL, &l) != 0) {
		Trace(TR_ERR, "update data class failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}


	retval = find_class_by_name(in->class_name, l, &class);
	if (retval == 0) {
		/* Update the data class you just found */
		Trace(TR_OPRMSG, "update data class updating class");
		update_class_struct(class, in);

	} else if (retval != 0 && samerrno == SE_NOT_FOUND) {
		Trace(TR_OPRMSG, "update data class new class");
		/*  dup the input and add it to the class list  */
		if (dup_ar_set_criteria(in, &class) != 0) {
			free_ar_set_criteria_list(l);
			Trace(TR_ERR, "update data class failed %d %s",
			    samerrno, samerrmsg);
			return (-1);

		}

		if (lst_append(l, class) != 0) {
			free_ar_set_criteria_list(l);
			Trace(TR_ERR, "update data class failed %d %s",
			    samerrno, samerrmsg);
			return (-1);
		}

	} else {

		Trace(TR_OPRMSG, "update data class other error");
		/* some other error occurred */
		free_ar_set_criteria_list(l);
		Trace(TR_ERR, "update data class failed %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}

	if (write_class_file(l) != 0) {
		free_ar_set_criteria_list(l);
		Trace(TR_ERR, "update data class failed %d %s",
		    samerrno, samerrmsg);

		return (-1);
	}
	free_ar_set_criteria_list(l);

	Trace(TR_OPRMSG, "updated data class %s", in->class_name);
	return (0);
}


static int
write_class_file(
sqm_lst_t *classes)
{

	FILE		*f = NULL;
	char		err_buf[80];
	int		fd;
	node_t		*n;
	char		classbuf[1024];
	time_t		the_time;

	if (ISNULL(classes)) {
		Trace(TR_OPRMSG, "writing class file failed: %s",
		    samerrmsg);
		return (-1);
	}

	backup_cfg(CLASS_FILE);

	if ((fd = open(CLASS_FILE, O_WRONLY|O_CREAT|O_TRUNC, 0644)) != -1) {
		f = fdopen(fd, "w");
	}
	if (f == NULL) {
		samerrno = errno;
		/* Open failed for %s: %s */
		StrFromErrno(samerrno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CFG_OPEN_FAILED), CLASS_FILE, err_buf);

		Trace(TR_OPRMSG, "writing class failed: %s",
		    samerrmsg);

		return (-1);
	}
	fprintf(f, "#\n#\tdata_class.cmd\n#");
	the_time = time(0);
	fprintf(f, "\n#\tGenerated by config api %s#\n", ctime(&the_time));

	for (n = classes->head; n != NULL; n = n->next) {
		ar_set_criteria_t *cr = (ar_set_criteria_t *)n->data;
		get_class_str(cr, B_FALSE, classbuf, sizeof (classbuf));
		fprintf(f, classbuf);
		fprintf(f, "\n");
	}

	fclose(f);
	backup_cfg(CLASS_FILE);
	Trace(TR_OPRMSG, "wrote class file");

	return (0);
}



/*
 * if arch_fmt is true this will return a string in the format familiar to
 * users of the archiver.cmd file.
 * If it is false this will return a string in the
 * format expected by the data_class.cmd file.
 */
int
get_class_str(
ar_set_criteria_t *crit,
boolean_t arch_fmt,
char *buf,
int buflen)
{
	char *sep = " ";
	char tmpbuf[32];

	*buf = '\0';



	if (!arch_fmt) {
		/*
		 * if the request was not for archiver format
		 * set separator to newline
		 */
		sep = "\n";


		if (*(crit->class_name) != '\0') {
			strlcat(buf, "class=", buflen);
			strlcat(buf, crit->class_name, buflen);
			strlcat(buf, "\n", buflen);
		}
		if (crit->description != NULL &&
		    *(crit->description) != '\0') {
			strlcat(buf, "desc=", buflen);
			strlcat(buf, crit->description, buflen);
			strlcat(buf, "\n", buflen);
		}
		if (*(crit->fs_name) != '\0') {
			strlcat(buf, "fs=", buflen);
			strlcat(buf, crit->fs_name, buflen);
			strlcat(buf, "\n", buflen);
		}
		if (*(crit->set_name) != '\0') {
			strlcat(buf, "-policy ", buflen);
			strlcat(buf, crit->set_name, buflen);
			strlcat(buf, "\n", buflen);
		}

		if (crit->change_flag & AR_ST_priority) {
			strlcat(buf, "-priority ", buflen);
			snprintf(tmpbuf, sizeof (tmpbuf), "%d\n",
			    crit->priority);
			strlcat(buf, tmpbuf, buflen);
		}

	}

	if (crit->change_flag & AR_ST_path &&
	    *(crit->path) != char_array_reset) {

		if (!arch_fmt) {
			strlcat(buf, "-dir ", buflen);
		}
		strlcat(buf, crit->path, buflen);
		strlcat(buf, sep, buflen);
	}

	if (crit->change_flag & AR_ST_name &&
	    *(crit->name) != char_array_reset) {

		strlcat(buf, "-name ", buflen);
		if (arch_fmt) {
			size_t curlen = strlen(buf);
			char *buf_end = buf + curlen;
			compose_regex(crit->name, crit->regexp_type,
			    buf_end, buflen - curlen);
			strlcat(buf, sep, buflen);

		} else {
			strlcat(buf, crit->name, buflen);
			strlcat(buf, "\n", buflen);
			strlcat(buf, "-regexptype ", buflen);
			strlcat(buf, RegExpTypes[crit->regexp_type].EeName,
			    buflen);
			strlcat(buf, "\n", buflen);
		}
	}

	if (crit->change_flag & AR_ST_user &&
	    *(crit->user) != char_array_reset) {

		strlcat(buf, "-user ", buflen);
		strlcat(buf, crit->user, buflen);
		strlcat(buf, sep, buflen);
	}
	if (crit->change_flag & AR_ST_group &&
	    *(crit->group) != char_array_reset) {

		strlcat(buf, "-group ", buflen);
		strlcat(buf, crit->group, buflen);
		strlcat(buf, sep, buflen);
	}


	if (crit->change_flag & AR_ST_minsize && crit->minsize != fsize_reset) {

		strlcat(buf, "-minsize ", buflen);
		strlcat(buf, fsize_to_str(crit->minsize, tmpbuf,
		    sizeof (tmpbuf)), buflen);
		strlcat(buf, sep, buflen);
	}
	if (crit->change_flag & AR_ST_maxsize && crit->maxsize != fsize_reset) {

		strlcat(buf, "-maxsize ", buflen);
		strlcat(buf, fsize_to_str(crit->maxsize, tmpbuf,
		    sizeof (tmpbuf)), buflen);
		strlcat(buf, sep, buflen);
	}

	if (crit->change_flag & AR_ST_access && crit->access != int_reset) {
		strlcat(buf, "-access ", buflen);
		strlcat(buf, StrFromInterval(crit->access, tmpbuf,
		    sizeof (tmpbuf)), buflen);
		strlcat(buf, sep, buflen);
	}

	if (crit->change_flag & AR_ST_nftv && crit->nftv != B_FALSE) {
		strlcat(buf, " -nftv", buflen);
		strlcat(buf, sep, buflen);
	}

	if (crit->change_flag & AR_ST_after &&
	    *(crit->after) != char_array_reset) {
		strlcat(buf, " -after ", buflen);
		strlcat(buf, crit->after, buflen);
		strlcat(buf, sep, buflen);
	}
	return (0);
}


int
setup_class_data(
ar_set_criteria_t *cr,
sqm_lst_t *classes)
{

	ar_set_criteria_t *data_class = NULL;
	node_t *n;
	boolean_t found = B_FALSE;

	if (ISNULL(cr, classes)) {
		Trace(TR_ERR, "Setting up data classes failed %d %s",
		    samerrno, samerrmsg);
		return (-1);
	}

	/*
	 * Can't use a helper function to find the class because we
	 * may need the nth instance of a class with these criteria in
	 * a general scenario (to allow reversals of priority between
	 * two classes in different file systems).
	 */
	for (n = classes->head; n != NULL; n = n->next) {
		data_class = (ar_set_criteria_t *)n->data;

		if (data_class == NULL) {
			Trace(TR_OPRMSG,
			    "data class NULL in setup data classes");
			continue;
		} else if (class_cmp(cr, data_class) == 0) {

			if (class_applies_to_fs(data_class,
			    cr->fs_name)) {
				found = B_TRUE;
				break;
			}
			/*
			 * else crit matches but fs does not could make
			 * use of this for world writable archiver.cmd.
			 */
		}
	}

	if (found) {
		strlcpy(cr->class_name, data_class->class_name,
		    sizeof (cr->class_name));
		cr->regexp_type = data_class->regexp_type;
		strlcpy(cr->name, data_class->name, sizeof (cr->class_name));

		if (data_class->description != NULL) {
			cr->description = strdup(data_class->description);
		}

	} else {

		/* if no class name is found include the criteria string */
		get_class_str(cr, B_TRUE, cr->class_name,
		    sizeof (cr->class_name));
	}

	return (0);
}


boolean_t
class_applies_to_fs(
ar_set_criteria_t *data_class,
char *fs_name) /* ARGSUSED */
{
	/* This check is not yet implemented */
	return (B_TRUE);
}
