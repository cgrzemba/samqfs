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
#pragma ident   "$Revision: 1.25 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * parser_utils.c
 * contains some functions commonly used by the parsing code.
 */

#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#include "sam/types.h"
#include "sam/devnm.h"
#include "sam/setfield.h"
#include "sam/sam_trace.h"

#include "parser_utils.h"
#include "mgmt/config/common.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "sam/lib.h"
#include "mgmt/util.h"
#include "pub/mgmt/file_util.h"

static char *backup_path = CFG_BACKUP_DIR;
#define	BACKUP_COUNT 25		/* for backup_cfg */


#define	VFSTAB_EQUALS "="
#define	SAMFS_CMD_EQUALS " = "
#define	MNT_CLEAR_FLAG		0x10000000

/*
 * returns the index of mt in the dev_nm2mt array or -1 if not found.
 * this method replaces asmMedia's checking.
 */
int
check_media_type(const char *mt)
{

	int i;

	Trace(TR_DEBUG, "checking media type %s", Str(mt));
	for (i = 0; dev_nm2mt[i].nm != NULL; i++) {
		if (strcmp(dev_nm2mt[i].nm, mt) == 0) {
			Trace(TR_DEBUG, "checked media type %s", Str(mt));
			return (i);
		}
	}
	Trace(TR_DEBUG, "media type %s is invalid", Str(mt));
	return (-1);
}


int
dup_parsing_error_list(
sqm_lst_t *in,
sqm_lst_t **out)
{

	node_t *node;
	sqm_lst_t *new_list = NULL;

	new_list = lst_create();
	if (new_list == NULL) {
		*out = NULL;

		Trace(TR_DEBUG, "duplicating parsing error list %s%s",
		    " failed:  ", samerrmsg);

		return (-1);
	}

	if (in != NULL && in->length != 0) {
		for (node = in->head; node != NULL;
		    node = node->next) {
			parsing_error_t *e;
			e = (parsing_error_t *)mallocer(
			    sizeof (parsing_error_t));

			if (e == NULL) {
				lst_free_deep(new_list);
				*out = NULL;

				Trace(TR_DEBUG, "%s failed: %s",
				    " duplicating parsing error list",
				    samerrmsg);

				return (-1);
			}
			memcpy(e, node->data, sizeof (parsing_error_t));

			if (lst_append(new_list, e) != 0) {
				lst_free_deep(new_list);
				*out = NULL;
				free(e);
				Trace(TR_DEBUG, "%s failed: %s",
				    " duplicating parsing error list",
				    samerrmsg);

				return (-1);
			}
		}
	}

	*out = new_list;
	return (0);
}



/*
 * duplicate a list of strings.
 */
int
dup_string_list(
sqm_lst_t *in,
sqm_lst_t **out)
{

	node_t *node;
	sqm_lst_t *new_list = NULL;

	if (in != NULL) {
		new_list = lst_create();
		if (new_list == NULL) {
			*out = NULL;
			Trace(TR_DEBUG, "duplicating string list%s%s",
			    " failed: ", samerrmsg);

			return (-1);
		}


		for (node = in->head; node != NULL;
		    node = node->next) {
			char *s;
			s = (char *)mallocer(strlen(node->data) + 1);

			if (s == NULL) {
				lst_free_deep(new_list);
				*out = NULL;

				Trace(TR_DEBUG, "%s failed: %s",
				    " duplicating string list",
				    samerrmsg);

				return (-1);
			}
			strcpy(s, node->data);
			if (lst_append(new_list, s) != 0) {
				lst_free_deep(new_list);
				*out = NULL;
				free(s);
				Trace(TR_DEBUG, "%s failed: %s",
				    " duplicating string list",
				    samerrmsg);
				return (-1);
			}
		}
	}

	*out = new_list;
	return (0);
}


/*
 * checks the str for spaces. Sets samerrno and samerrmsg and returns if
 * the string contains any whitespace characters.
 */
int
has_spaces(
char *str,		/* string to check */
char *field_name)	/* for error messages */
{

	if (strpbrk(str, WHITESPACE) != NULL) {
		samerrno = SE_FIELD_CONTAINED_SPACES;

		/* Field %s contained whitespace characters */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_FIELD_CONTAINED_SPACES), field_name);
		Trace(TR_DEBUG, "field %s contained spaces", Str(field_name));
		return (-1);
	}
	return (0);
}

/*
 * conditionally backup src_file to the backup_path directory.
 * The backup copy will be made if src_file does not have a known
 * backup copy.
 *
 * src_file is the full path to the src file.  so src_file = path/file
 * it will be backed up to the backup_path/file.src_file's_mod_time
 */
int
backup_cfg(
char *src_file)
{

	char		file_path[MAXPATHLEN+1] = {'\0'};
	char		backup_time_path[MAXPATHLEN+1] = {'\0'};
	FILE		*time_file_ptr = NULL;
	int		fd;
	struct stat	cfg_stat;
	time_t		mybk_time = 0;
	int		filenum;
	char		*file_name_and_slash;
	char		err_buf[80];

	Trace(TR_DEBUG, "backup %s", src_file);

	/* verify existance of or try to create the backup dir */
	if (create_dir(NULL, backup_path) != 0) {
		samerrno = SE_CREATE_BACKUP_DIR_FAILED;

		/* Unable to create the backup directory %s. */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_CREATE_BACKUP_DIR_FAILED),
		    backup_path);
		Trace(TR_ERR, samerrmsg);
		return (-1);
	}

	/*
	 * figure out the name of the file that holds the backup time
	 *
	 * /mcf_bktime is replaced by the last element of the path +_bktime
	 */
	file_name_and_slash = (char *)strrchr(src_file, '/');
	if (file_name_and_slash == NULL) {
		samerrno = SE_INVALID_FILE_NAME;
		/* Invalid backup file name %s */
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_INVALID_FILE_NAME), src_file);
		Trace(TR_ERR, samerrmsg);
		return (-1);
	}


	/* Get the time the api last backed up the config file. */
	snprintf(backup_time_path, sizeof (backup_time_path), "%s%s%s\0",
	    backup_path, file_name_and_slash, "_bktime");

	if ((time_file_ptr = fopen(backup_time_path, "r")) == NULL) {
		Trace(TR_ERR, "open failed: %s", strerror(errno));
		filenum = 0;
		/* Do nothing */
	} else {
		int cnt = 0;

		cnt = fscanf(time_file_ptr, "%d %ld", &filenum, &mybk_time);
		if (cnt != 2) {
			filenum = 0;
		}

		filenum %= BACKUP_COUNT;
		fclose(time_file_ptr);
	}

	/*
	 * backup src_file if it is different than known backup.
	 * cant simply use > since user could copy a random backup into
	 * file location
	 */
	if (stat(src_file, &cfg_stat) != 0) {
		if (errno == ENOENT) {
			/*
			 * the absence of a file is not an error
			 * simply return success.
			 */
			Trace(TR_DEBUG, "backup successfull - no file");
			return (0);
		}

		samerrno = SE_UNABLE_TO_BACKUP_CONFIG;
		StrFromErrno(errno, err_buf, sizeof (err_buf));
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_UNABLE_TO_BACKUP_CONFIG),
		    file_name_and_slash + 1, err_buf);

		Trace(TR_ERR, "backup failed: %s", samerrmsg);
		return (-1);
	} else if (cfg_stat.st_mtime == mybk_time) {
		Trace(TR_DEBUG, "backup successfull - backup current");
		return (0);
	}

	/*
	 * make the file name for the backup file.
	 * cant use src_file because it might not be long enough so make a
	 * new char *
	 */
	snprintf(file_path, sizeof (file_path), "%s%s.%d", backup_path,
	    file_name_and_slash, filenum);

	if (cp_file(src_file, file_path) != 0) {
		samerrno = SE_UNABLE_TO_BACKUP_CONFIG;
		snprintf(samerrmsg, MAX_MSG_LEN,
		    GetCustMsg(SE_UNABLE_TO_BACKUP_CONFIG),
		    file_name_and_slash + 1, "cp error");

		Trace(TR_ERR, "backup failed: %s", samerrmsg);
		return (-1);
	}

	time_file_ptr = NULL;
	if ((fd = open(backup_time_path, O_WRONLY|O_CREAT|O_TRUNC, 0644))
	    != -1) {
		time_file_ptr = fdopen(fd, "w");
	}

	if (time_file_ptr != NULL) {
		Trace(TR_DEBUG, "open to write succeeded");
		fprintf(time_file_ptr, "%d %ld", ++filenum, cfg_stat.st_mtime);
		if (fclose(time_file_ptr) != 0) {
			Trace(TR_ERR, "backup failed: close failed");
		}
	} else {
		/* cant open file to write backup time but continue success */
		Trace(TR_ERR, "failed to open file for writing backup time");
	}

	return (0);
}




/*
 * This function is not complete. It handles only types that were needed sofar.
 *
 * General pattern for change is->
 * 0. Get the current value in the struct.
 */
int
check_field_value(
const void *st,
struct fieldVals *table,	/* table describing the type of st */
char *field,			/* name of the field in st to check */
char *buf,			/* buffer to contain an error */
int bufsize)
{

	char our_buf[256];
	void	*v;
	int		type;

	if (buf == NULL) {
		buf = our_buf;
		bufsize = sizeof (our_buf);
	}
	if (bufsize < sizeof (our_buf)) {
		*buf = '\0';
		Trace(TR_DEBUG,
		    "checking field values failed: buffer too small");
		return (-1);
	}
	*buf = '\0';


	while (strcmp(field, table->FvName) != 0) {

		table++;
		if (table->FvName == NULL) {
			snprintf(buf, bufsize,
			    GetCustMsg(14100), field);
			errno = ENOENT;
			Trace(TR_DEBUG, "checking field values failed %s",
			    buf);
			return (-1);
		}
	}

	type = table->FvType & ~FV_FLAGS;

	errno = EINVAL;	/* Preset it for error returns. */
	v = ((char *)st + table->FvLoc);

	/*
	 * The SET/CLEARFLAG have no associated value.
	 * Value must be NULL or empty.
	 */
	if (type == SETFLAG || type == CLEARFLAG) {
		/* nothing to check its done */
		goto success;
	}


	switch (type) {
	case INTERVAL:
		snprintf(buf, bufsize, "%s checking not implemented",
		    "INTERVAL types");
		return (-1);
		break;	/* NOTREACHED */


	case INT16:
		snprintf(buf, bufsize, "%s checking not implemented",
		    "short types");
		return (-1);
		break;	/* NOTREACHED */

	case FSIZE:
	case INT64:
	case MULL8:
		/*
		 * Would need to verify that the value is a multiple of 8.
		 */
		snprintf(buf, bufsize, "%s checking not implemented",
		    "long types");
		return (-1);
		break;	/* NOTREACHED */

	case PWR2:
	case MUL8:
	case INT: {
		struct fieldInt *vals = (struct fieldInt *)table->FvVals;
		int64_t	val = *(int *)v;

		/*
		 * If get put in they will need to be unprocessed here.
		 */

		/*
		 * Process minimum/maximum values.
		 */
		errno = EINVAL;
		if (val < vals->min && val > vals->max) {
			snprintf(buf, bufsize,
			    GetCustMsg(14102), field, vals->min,
			    vals->max);
			return (-1);
		}
		if (val < vals->min) {
			snprintf(buf, bufsize,
			    GetCustMsg(14103), field, vals->min);
			return (-1);
		}
		if (val > vals->max) {
			snprintf(buf, bufsize,
			    GetCustMsg(14104), field, vals->max);
			return (-1);
		}

		if (type == PWR2) {
			/*
			 * Verify that the value is a power of 2.
			 */
			if ((val & (val - 1)) != 0) {
				snprintf(buf, bufsize,
				    GetCustMsg(14110), field);
				return (-1);
			}
		}
		if (type == MUL8 || type == MULL8) {
			/*
			 * Verify that the value is a multiple of 8.
			 */
			if ((val & 7) != 0) {
				snprintf(buf, bufsize,
				    GetCustMsg(14111), field);
				return (-1);
			}
		}

		break;
	}

	case FLOAT:
	case DOUBLE: {
		struct fieldDouble *vals = (struct fieldDouble *)table->FvVals;
		double val = *(double *)v;

		/*
		 * Don't Unprocess multiplier since we don't process them
		 */

		/*
		 * Process minimum/maximum values.
		 */
		errno = EINVAL;
		if (val < vals->min && val > vals->max) {
			snprintf(buf, bufsize,
			    GetCustMsg(14105), field,
			    vals->min, vals->max);
			return (-1);
		}
		if (val < vals->min) {
			snprintf(buf, bufsize,
			    GetCustMsg(14106), field, vals->min);
			return (-1);
		}
		if (val > vals->max) {
			snprintf(buf, bufsize,
			    GetCustMsg(14107), field, vals->max);
			return (-1);
		}
		break;
	}

	case ENUM:
		snprintf(buf, bufsize, "%s checking not implemented", "ENUM");
		return (-1);

		break;	/* NOTREACHED */


	case FLAG:
		/* should never be here since flags can't be checked. */
		break;

	case FUNC:
		snprintf(buf, bufsize, "%s checking not implemented", "FUNC");
		return (-1);

		break;	/* NOTREACHED */

	case MEDIA: {
		char *str = (char *)v;
		int m;

		m = sam_atomedia(str);
		if (m == 0) {
			snprintf(buf, bufsize,
			    GetCustMsg(14108), table->FvName, v);
			return (-1);
		}
		break;
	}
	case STRING: {
		struct fieldString *vals = (struct fieldString *)table->FvVals;
		char *str = v;
		if (strlen(str) >= vals->length) {
			snprintf(buf, bufsize,
			    GetCustMsg(14109), field, vals->length - 1);
			return (-1);
		}
		break;
	}
	}

success:
	errno = 0;
	return (0);
}


/*
 * A utility function to print headers only if needed.
 * writes the string and sets printed to true if printed is false.
 */
int
write_once(
FILE *f,
char *print_first,	/* string to optionally print */
boolean_t *printed)	/* print if false, then set to true */
{

	if (!(*printed) && print_first != NULL) {
		fprintf(f, print_first);
		*printed = B_TRUE;
	}
	return (0);
}




/*
 * checks to see if defbits has been set to the CLR flag value for the field
 * described by entry.
 */
int
is_reset_value(
int32_t defbits,		/* the change_flag for the struct */
struct fieldVals *entry,	/* describes the field to check */
boolean_t *ret_val)		/* true if defbits contains the reset flag */
{

	if (defbits & (entry->FvDefBit << 16)) {

		*ret_val = B_TRUE;
	} else {
		*ret_val = B_FALSE;
	}

	return (0);
}


/*
 * Based on SetFieldValueToStr this method however gets rid of multipliers
 * and such. and returns the buffer set to contain a valid samfs.cmd line
 * or segment of a vfstab options entry depending on the flag vfstab.
 *
 * e.g. if fvstab = B_FALSE
 *	"high = 80" or "qwrite"
 * for vfstab = B_TRUE
 *	high = 80
 * no commas are included
 *
 * This method is not finished.	 Simply takes care of numeric multipliers
 * for now. and flags.
 *
 */
char *
get_cfg_str(
const void *st,			/* struct containing field to get str for */
struct fieldVals *entry,	/* struct describing the field in st */
boolean_t vals_only,		/* if true write out only the value */
char *buf,			/* buffer to contain result */
int bufsize,			/* size of result buffer */
    boolean_t vfstab,		/* if true output will have no spaces */
    char skip_entry[][20])	/* an array of the names of fields to skip */
{

	static char our_buf[32];
	void	*v;
	int	type;
	double doub;
	int i;
	short s;
	float fl;
	char *equals = SAMFS_CMD_EQUALS;

	if (buf == NULL) {
		buf = our_buf;
		bufsize = sizeof (our_buf);
	}
	if (bufsize < sizeof (our_buf)) {
		*buf = '\0';
		return (buf);
	}
	*buf = '\0';

	if (vfstab) {
		equals = VFSTAB_EQUALS;
	}

	if (skip_entry != NULL) {
		for (i = 0; *skip_entry[i] != '\0'; i++) {
			if (strcmp(entry->FvName, skip_entry[i]) == 0) {
				return (NULL);
			}
		}
	}

	v = (char *)st + entry->FvLoc;
	type = entry->FvType & ~FV_FLAGS;
	switch (type) {

	case DOUBLE: {
		double multiplier = ((struct fieldDouble *)entry->FvVals)->mul;
		if (multiplier != 0) {
			doub = *(double *)v / multiplier;
		} else {
			doub = *(double *)v;
		}
		if (vals_only) {
			snprintf(buf, bufsize, "%g", doub);
		} else {
			snprintf(buf, bufsize, "%s%s%g",
			    entry->FvName, equals, doub);
		}
		break;
	}
	case ENUM: {
		struct fieldEnum *vals = (struct fieldEnum *)entry->FvVals;
		int		e;
		int		i;

		e = *(int *)v;
		for (i = 0; vals->enumTable[i].EeName != NULL; i++) {
			if (vals->enumTable[i].EeName == NULL) {
				snprintf(buf, bufsize, "%d", e);
				break;
			}
			if (vals->enumTable[i].EeValue == e) {
				buf = vals->enumTable[i].EeName;
				break;
			}
		}
		break;
	}
	case FLOAT: {
		float multiplier =
		    (float)((struct fieldDouble *)entry->FvVals)->mul;
		if (multiplier != 0) {
			fl = *(float *)v / multiplier;
		} else {
			fl = *(float *)v;
		}

		if (vals_only) {
			snprintf(buf, bufsize, "%g", fl);
		} else {
			snprintf(buf, bufsize, "%s%s%g", entry->FvName,
			    equals, fl);
		}
		break;
	}
	case CLEARFLAG: {
		/* If yes then print the name. */
		struct fieldFlag *vals = (struct fieldFlag *)entry->FvVals;
		if (!(*(uint32_t *)v & vals->mask)) {
			if (vals_only) {
				strcpy(buf, "false");
			} else {
				snprintf(buf, bufsize, "%s", entry->FvName);
			}
		}
		break;
	}
	case SETFLAG:
	case FLAG: {
		/* If yes then print the name. */
		struct fieldFlag *vals = (struct fieldFlag *)entry->FvVals;
		if ((*(uint32_t *)v & vals->mask)) {
			if (vals_only) {
				strcpy(buf, "true");
			} else {
				snprintf(buf, bufsize, "%s", entry->FvName);
			}
		}
		break;
	}
	case FSIZE: {
		struct fieldFsize *vals = (struct fieldFsize *)entry->FvVals;

		(void) StrFromFsize(*(int64_t *)v, vals->prec, buf, bufsize);
		while (*buf == ' ') {
			buf++;
		}
		break;
	}
	case FUNC: {
		struct fieldFunc *vals = (struct fieldFunc *)entry->FvVals;

		if (vals->tostr != NULL) {
			char *c;
			vals->tostr(v, buf, bufsize);
			if (vals_only) {
				if ((c = strpbrk(buf, "=")) != NULL) {
					return (c);
				}
			}
		}
		break;
	}
	case MUL8:
	case PWR2:
	case INT: {
		int multiplier = ((struct fieldInt *)entry->FvVals)->mul;
		if (multiplier != 0) {
			i = *(int *)v / multiplier;
		} else {
			i = *(int *)v;
		}
		if (vals_only) {
			snprintf(buf, bufsize, "%d", i);
		} else {
			snprintf(buf, bufsize, "%s%s%d",
			    entry->FvName, equals, i);
		}
		break;
	}
	case INT16: {
		int16_t multiplier =
		    (int16_t)((struct fieldInt *)entry->FvVals)->mul;
		if (multiplier != 0) {
			s = (*(int16_t *)v) / multiplier;
		} else {
			s = (*(int16_t *)v);
		}
		if (vals_only) {
			snprintf(buf, bufsize, "%d", (int16_t)s);
		} else {
			snprintf(buf, bufsize, "%s%s%d", entry->FvName,
			    equals, (int16_t)s);
		}
		break;
	}
	case MULL8:
	case INT64: {
		int64_t multiplier =
		    ((struct fieldInt *)entry->FvVals)->mul;
		if (multiplier != 0) {
			i = *(int64_t *)v / multiplier;
		} else {
			i = *(int64_t *)v;
		}
		if (vals_only) {
			snprintf(buf, bufsize, "%lld",	(int64_t)i);
		} else {
			snprintf(buf, bufsize, "%s%s%lld", entry->FvName,
			    equals, (int64_t)i);
		}
		break;
	}
	case INTERVAL:
		(void) StrFromInterval(*(int *)v, buf, bufsize);
		break;


	case MEDIA:
		buf = (char *)sam_mediatoa(*(media_t *)v);
		break;

	case STRING:
		buf = (char *)v;
		break;

	case DEFBITS:
	default:
		buf = "??";
	}
	return (buf);

}


/*
 * returns true if the set_bits have the change flag that corresponds to
 * the fieldVals entry set.
 */
boolean_t
is_explicit_set(
uint32_t set_bits,
struct fieldVals *entry)
{


	if (set_bits & entry->FvDefBit) {
		return (B_TRUE);
	}

	return (B_FALSE);
}
