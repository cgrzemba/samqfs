/*
 * ----	samdb_load - Load Database from Samfsdump.
 *
 *	Description:
 *	    Populate database using file list output from
 *	    samfsdump/restore(1).
 *
 *	Command Syntax:
 *
 *	    samdb_load family_set [-g] [-r] [-v|-V] input_file(s)
 *
 *	    where:
 *
 *		input_file(s)	List of samfsdump load files.
 *				See format below.
 *
 *		family_set	Specifies the family set name.
 *
 *		-g		Show progress. Generates hash marks (#), one
 * 				for each 1000 files processed.
 *
 *		-r		Restore database.  Performed after a
 *				samfsddump/restore operation.  This option
 *				restores the database with the new inode/
 *				generation numbers that have been assigned.
 *
 *		-help		Print command syntax summary and exit.
 *
 *		-version	Print program version and exit.
 *
 *		-v		Verbose mode.  Enables verbose messages.
 *
 *		-V		Debug mode.  Enables debug messages, most
 *				notably is the query string sent to the
 *				databsase engine.
 *
 *	Environment Variables:
 *
 *	    SAM_FS_NAME		Default family set name.
 *
 *	Load File Format:
 *
 *	    sam_db load file format (all fields seperated by tabs):
 *
 *		ino   gen   type    path    object_name
 *		size  uid   gid     creation_time    modify_time
 *		symbolic_link_value
 *		copy seq  media_type  VSN  create_tiime  modify_time size
 *		/EOS
 *
 *		File types:
 *		0       regular file (S_ISREG)
 *		1       directory (S_ISDIR)
 *		2       segment file segment (S_ISSEGS)
 *		3       segment file (S_ISSEGI)
 *		4       symbolic link (S_ISLNK)
 *		5       something else
 *
 *	    All entries end with /EOS line.
 *	    Archive link repeats for each archive entry, seq column identifies
 *	    multi-VSN set, with size being the number of bytes this VSN.
 *
 * Example:
 * 163543  1       0       /sam/sam1/3ksnn64/testdata file_10
 * 34359738368     0       0       1212694692      1212695114
 * 1   1   lt      500000  1212704193      1212695114      26685472256
 * 1   2   lt      500001  1212704193      1212695114      7674266112
 * 2   1   lt      500008  1212704295      1212695114      27537702400
 * 2   2   lt      500005  1212704295      1212695114      6822035968
 * /EOS
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
#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Include files: */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <mysql.h>
#include <errmsg.h>

#include <sam/sam_malloc.h>
#define	DEC_INIT
#include <sam/sam_db.h>

#include "arg.h"
#include "mtrlog.h"
#include "util.h"

#define	MAX_ARCH 100	/* Maximum no. of archive */

typedef struct { 	/* Archive Level table */
	int level; 		/* Level number */
	int seq; 		/* Sequence number */
	int vsn_id; 		/* VSN id */
	char media[10]; 	/* Media type */
	char vsn[60]; 		/* VSN */
	char size[20];		/* Size of data chunck */
	char create[20];	/* Archive creation time */
	char modify[20];	/* Modify time */
} LEVEL_T;

extern int parsetabs(char *, char **, int *, int);

/* Global tables & pointers: */
char *program_name;	/* Program name: used by error	*/
int exit_status = 0;	/* Exit status */

Buffer in;		/* Input buffer */

int HashCounts;		/* Record counts periodicly */
int Restore; 		/* Restore flag */
char *fs_name;		/* Family set name */
time_t Start_Time;	/* Start time */

int n_files;	/* Number of files processed */
int n_path;	/* Number of path/inode records */
int n_archive;	/* Number of archive records */
int n_vsn;	/* Number of vsn records */

char *Log_prefix = NULL;	/* Log file name prefix */
char *Log_file = NULL;		/* Log file name */
char *Errlog_file = NULL;	/* Error log file name */
char *Filelog_file = NULL;	/* File operation log file name */

FILE *MTR_log;	/* Log file descriptor */
FILE *MTR_err;	/* Errlog file descriptor */
FILE *MTR_file; /* Filelog file descriptor */

static void Process_Input_Load(FILE *, char *, int);
static void Process_Input_Restore(FILE *, char *, int);
static char *get_normalized_path(char *, char *, int);
static int SAM_db_insert_path(char *, char *, char *, char *, char *);
static int SAM_db_insert_inode(char *, char *, char *, char **);
static int SAM_db_insert_link(char *, char *, char *);
static int SAM_db_insert_archive(char *, char *, LEVEL_T *, char *);
static int SAM_db_insert_vsn(char *, char *);
static void query_error(char *);
static void program_help(void);

arg_t Arg_Table[] = {	/* Argument table */
	{ "-g", ARG_TRUE, (char *)&HashCounts },
	{ "-r", ARG_TRUE, (char *)&Restore },
	{ "-help", ARG_PART, (char *)1 },
	{ "-version", ARG_VERS, NULL },
	{ "-v", ARG_TRUE, (char *)&SAMDB_Verbose },
	{ "-V", ARG_TRUE, (char *)&SAMDB_Debug },
	{ NULL, ARG_PART, NULL } };

int
main(
	int argc,	/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	char *input;			/* Syslog file/path */
	FILE *F;
	sam_db_access_t *dba;		/* DB access information */
	sam_db_connect_t SAM_db;	/* mySQL connect parameters */
	char *mp;			/* Mount point */
	int mpl;			/* Mount point string length */
	int i;

	/* ---	Process arguments: --- */
	HashCounts = 0;
	program_name = "samdb_load";
	in.bufl = L_IN_BUFFER;
	SamMalloc(in.buf, L_IN_BUFFER);

	if (argc < 2) {
		program_help();
	}

	fs_name = *++argv;
	argc--;

	init_trace(NULL, 0);
	sam_db_init_vsn_cache();

	while ((i = Process_Command_Arguments(Arg_Table, &argc, &argv)) > 0) {
		if (i == 1) {
			program_help();
		}
	}

	if (i < 0) {
		error(1, 0, "Argument errors\n", 0);
	}

	if (SAMDB_Debug) {
		SAMDB_Verbose = 1;
	}

	if ((dba = sam_db_access(SAMDB_ACCESS_FILE, fs_name)) == NULL) {
		error(1, 0, "family set name [%s] not found", fs_name);
	}

	if (argc < 1) {
		error(1, 0, "input file not specified.", 0);
	}

	SAM_db.SAM_host = dba->db_host;
	SAM_db.SAM_user = dba->db_user;
	SAM_db.SAM_pass = dba->db_pass;
	SAM_db.SAM_name = dba->db_name;
	SAM_db.SAM_port = *dba->db_port != '\0' ?
	    atoi(dba->db_port) : SAMDB_DEFAULT_PORT;
	SAM_db.SAM_client_flag = *dba->db_client != '\0' ?
	    atoi(dba->db_client) : SAMDB_CLIENT_FLAG;

	if (sam_db_connect(&SAM_db)) {
		error(1, 0, "cannot connect: %s", mysql_error(SAMDB_conn));
	}

	if ((n_vsn = sam_db_load_vsns()) < 0) {
		error(0, 0, "Errors loading VSNs from db", 0);
	}

	if (SAMDB_Verbose) {
		error(0, 0, "VSNs loaded from db: %d", n_vsn);
	}

	mpl = strlen(dba->db_mount);
	SamMalloc(mp, mpl+2);
	strcpy(mp, dba->db_mount);

	/* Add terminating '/' */
	if (*(mp+mpl-1) != '/') {
		strcat(mp, "/");
		mpl++;
	}

	/* Process file:  */
	while (argc-- != 0) {
		input = *argv++;
		Start_Time = time(0);
		n_files = 0;
		n_path = 0;
		n_archive = 0;
		n_vsn = 0;

		if ((F = fopen64(input, "r")) == NULL) {
			error(1, errno, input, 0);
		}

		Log_prefix = input;
		Log_file = logfile(Log_prefix, ".log");
		Errlog_file = logfile(Log_prefix, ".err");
		Filelog_file = logfile(Log_prefix, ".full");

		if ((MTR_log = fopen64(Log_file, "a")) == NULL) {
			error(1, errno, Log_file, 0);
		}
		if ((MTR_err = fopen64(Errlog_file, "a")) == NULL) {
			error(1, errno, Errlog_file, 0);
		}
		if ((MTR_file = fopen64(Filelog_file, "a")) == NULL) {
			error(1, errno, Filelog_file, 0);
		}

		init_mtrlog(MTR_log, MTR_err);
		mtrlog(0, 0, "Processing file: %s", input);

		if (Restore) {
			Process_Input_Restore(F, mp, mpl);
		} else {
			Process_Input_Load(F, mp, mpl);
		}
		fclose(F);

		mtrlog(0, 0, "files processed %d, elapse time %d",
		    n_files, time(0)-Start_Time);
		mtrlog(0, 0, "records processed %d, %d/%d/%d",
		    2*n_path+n_archive+n_vsn, n_path, n_archive, n_vsn);

		fclose(MTR_log);
		fclose(MTR_err);
		fclose(MTR_file);
	}

	sam_db_disconnect();
	exit(exit_status);
	return (0);
}

/*
 * Process_Input_Load - Process input file.
 */
static void Process_Input_Load(
	FILE *F,	/* Input file descriptor */
	char *mp,	/* Mount point */
	int mpl) 	/* Mount point string length */
{
	int logc; 	/* Argument count */
	char *logv[20]; /* Argument vector */
	int h, i, k, l, s;
	LEVEL_T ar[MAX_ARCH];
	char ino[20], gen[20], type[20];
	char size[40];
	char modify_time[40];
	char *path;
	unsigned int n_ino;	/* Inode number */
	int ftype;		/* File type	*/

	for (h = 1; ; h++) {
		if (readin(F, &in) == EOF) {
			break;
		}
		if (SAMDB_Debug) {
			printf("buf:%s\n", in.buf);
		}

		if (HashCounts && (h % 1000) == 0) {
			fprintf(stderr, "%d\n", h);
		}

		(void) parsetabs(in.buf, logv, &logc, 5);

		if (logc < 5) {
			mtrlog(1, 0, "insufficent items in record, line %d",
			    readin_ln);
			error(1, 0, "insufficent items in record, line %d",
			    readin_ln);
		}

		strcpy(ino, logv[0]);
		strcpy(gen, logv[1]);
		strcpy(type, logv[2]);

		ftype = atoi(type);
		n_ino = atoi(ino);

		if (n_ino == 2) {
			error(0, 0, "skipping path %s, ino %d, line %d",
			    logv[3], n_ino, readin_ln);
			for (;;) {
				if (readin(F, &in) == EOF)
					break;
				if (SAMDB_Debug)
					printf("buf:%s\n", in.buf);
				if (strcmp(in.buf, "/EOS") == 0)
					break;
			}
			continue;
		}

		path = get_normalized_path(logv[3], mp, mpl);
		n_path++;
		(void) SAM_db_insert_path(ino, gen, type, path, logv[4]);

		if (readin(F, &in) == EOF) {
			break;
		}
		if (SAMDB_Debug) {
			printf("buf:%s\n", in.buf);
		}

		(void) parsetabs(in.buf, logv, &logc, 5);

		if (logc < 5) {
			mtrlog(1, 0, "insufficent items in record, line %d",
			    readin_ln);
			error(1, 0, "insufficent items in record, line %d",
			    readin_ln);
		}

		strcpy(size, logv[0]);
		strcpy(modify_time, logv[3]);
		(void) SAM_db_insert_inode(ino, gen, type, &logv[0]);

		if (ftype == FTYPE_LINK) {
			if (readin(F, &in) == EOF) {
				break;
			}
			if (SAMDB_Debug) {
				printf("buf:%s\n", in.buf);
			}

			(void) parsetabs(in.buf, logv, &logc, 1);

			(void) SAM_db_insert_link(ino, gen, logv[0]);
		}

		for (k = 0; ; ) {
			if (readin(F, &in) == EOF) {
				break;
			}
			if (SAMDB_Debug) {
				printf("buf:%s\n", in.buf);
			}

			if (strcmp(in.buf, "/EOS") == 0) {
				break;
			}

			(void) parsetabs(in.buf, (char **)&logv, &logc, 7);

			if (logc < 6) {
				mtrlog(1, 0, "insufficent items in record, "
				    "line %d", readin_ln);
				error(1, 0, "insufficent items in record,"
				    " line %d", readin_ln);
			}

			l = atoi(logv[0]);

			if (l >= 4) {
				mtrlog(1, 0, "invalid archive level, line %d",
				    readin_ln);
				continue;
			}

			s = atoi(logv[1]);

			ar[k].level = l;
			ar[k].seq = s;
			strncpy(ar[k].media, logv[2], 10);
			strncpy(ar[k].vsn, logv[3], 60);
			strncpy(ar[k].create, logv[4], 20);
			strncpy(ar[k].modify, logv[5], 20);
			strncpy(ar[k].size, logv[6], 20);
			ar[k].vsn_id =
			    sam_db_find_vsn(l, ar[k].media, ar[k].vsn);

			if (ar[k].vsn_id == 0) {
				n_vsn++;
				ar[k].vsn_id = SAM_db_insert_vsn(ar[k].media,
				    ar[k].vsn);
				if (ar[k].vsn_id <= 0) {
					continue;
				}
				sam_db_cache_vsn(ar[k].vsn_id, ar[k].media,
				    ar[k].vsn);
			}
			k++;
		}

		for (i = 0; i < k; i++) {
			n_archive++;
			(void) SAM_db_insert_archive(ino, gen,
			    &ar[i], modify_time);
		}

		n_files++;
	}
}

/*
 * Process_Input_Restore - Process input file
 */
static void Process_Input_Restore(
	FILE *F,	/* Input file descriptor */
	char *mp,	/* Mount point */
	int mpl)	/* Mount point string length */
{
	int logc; 		/* Argument count */
	char *logv[20];		/* Argument vector */
	int h;
	char ino[20], gen[20], type[20];
	char *path;
	unsigned int n_ino;	/* Inode number */
	unsigned int n_gen;	/* Generation number */
	int ftype;		/* File type */

	for (h = 1; ; h++) {
		if (readin(F, &in) == EOF) {
			break;
		}
		if (SAMDB_Debug) {
			printf("buf:%s\n", in.buf);
		}

		if (HashCounts && (h % 1000) == 0) {
			fprintf(stderr, "%d\n", h);
		}

		(void) parsetabs(in.buf, logv, &logc, 5);

		if (logc < 5) {
			mtrlog(1, 0, "insufficent items in record, line %d",
			    readin_ln);
			error(1, 0, "insufficent items in record, line %d",
			    readin_ln);
		}

		strcpy(ino, logv[0]);
		strcpy(gen, logv[1]);
		strcpy(type, logv[2]);

		ftype = atoi(type);
		n_ino = atoi(ino);
		n_gen = atoi(gen);

		if (n_ino == 2) {
			mtrlog(1, 0, "skipping path %s, ino %d, "
			    "line %d", logv[3], n_ino, readin_ln);
			error(0, 0, "skipping path %s, ino %d, "
			    "line %d", logv[3],	n_ino, readin_ln);
			for (;;) {
				if (readin(F, &in) == EOF)
					break;
				if (SAMDB_Debug)
					printf("buf:%s\n", in.buf);
				if (strcmp(in.buf, "/EOS") == 0)
					break;
			}
			continue;
		}

		path = get_normalized_path(logv[3], mp, mpl);
		n_path++;
		sam_db_restore(n_ino, n_gen, ftype, path, logv[4]);

		if (readin(F, &in) == EOF) {
			break;
		}
		if (SAMDB_Debug) {
			printf("buf:%s\n", in.buf);
		}

		if (ftype == FTYPE_LINK) {
			if (readin(F, &in) == EOF) {
				break;
			}
			if (SAMDB_Debug) {
				printf("buf:%s\n", in.buf);
			}
		}

		for (; ; ) {
			if (readin(F, &in) == EOF) {
				break;
			}
			if (SAMDB_Debug) {
				printf("buf:%s\n", in.buf);
			}
			if (strcmp(in.buf, "/EOS") == 0) {
				break;
			}
		}

		n_files++;
	}
}

/*
 * Munges the provided path into a normalized format
 * required by the database. Specifically absolute paths
 * have their mount point removed and relative paths
 * have any leading dot directories removed.
 */
static char *
get_normalized_path(char *path, char *mp, int mpl) {
	char *norm_path;

	/*
	 * Check if absolute path (begins with /).
	 * If so must begin with mount point.  Otherwise
	 * we assume it is a relative path.
	 */
	if (*path == '/') {
		if (strncmp(path, mp, mpl) == 0) {
			norm_path = path + mpl;
		} else {
			error(0, 0, "absolute path missing mount point, "
			    "path %s", path);
			norm_path = NULL;
		}
	} else {
		/* Skip past leading relative path ./ */
		if (strncmp(path, "./", 2) == 0) {
			norm_path = path + 2;
			if (*norm_path == '\0') {
				norm_path = "/";
			}
		} else {
			norm_path = path;
		}
	}

	return (norm_path);
}

/*
 * 	SAM_db_insert_path - Insert file into DB.
 *
 *	Description:
 *	    Insert file into sam_path table.
 *
 *	Entry:
 *	    ino		Inode number.
 *	    gen		Generation number.
 *	    type	File type (file, dir, seg, seg file, link, other)
 *	    path	Path
 *	    obj		Object name
 */
static int
SAM_db_insert_path(
	char *ino,	/* Inode number */
	char *gen,	/* Generation number */
	char *type,	/* File type */
	char *path,	/* Path	*/
	char *obj)	/* Object name */
{
	my_ulonglong rc; /* Record count from query	*/
	char *q;

	/* Build insert for sam_path table */
	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_PATH);
	q = strmov(q, " (ino, gen, type, path, obj, "
	    "initial_path, initial_obj) ");
	sprintf(q, " VALUES ('%s', '%s', '%s', '", ino, gen, type);
	q = strend(q);
	q += mysql_real_escape_string(SAMDB_conn, q, path, strlen(path));
	q = strmov(q, "', '");
	q += mysql_real_escape_string(SAMDB_conn, q, obj, strlen(obj));
	q = strmov(q, "', '");
	q += mysql_real_escape_string(SAMDB_conn, q, path, strlen(path));
	q = strmov(q, "', '");
	q += mysql_real_escape_string(SAMDB_conn, q, obj, strlen(obj));
	q = strmov(q, "');");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(MTR_file, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		mtrlog(1, 0, "insert fail [%s]: ino=%s, %s",
		    T_SAM_PATH, ino, mysql_error(SAMDB_conn));
		query_error(T_SAM_PATH);
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		mtrlog(1, 0, "insert rows [%s]: ino=%s, rows=%ld",
		    T_SAM_PATH, ino, rc);
	}

	return (0);
}

/*
 * 	SAM_db_insert_inode - Insert file into DB.
 *
 *	Description:
 *	    Insert file into sam_inode table.
 *
 *	Entry:
 *	    ino		Inode number.
 *	    gen		Generation number.
 *	    type	File type (file, dir, seg, seg file, link, other)
 *	    p		Parameter vector: size, uid, gid, creation_time,
 *			modify_time
 */
static int
SAM_db_insert_inode(
	char *ino,	/* Inode number */
	char *gen,	/* Generation number */
	char *type,	/* File type */
	char *p[])	/* Parameters file */
{
	my_ulonglong rc; /* Record count from query	*/
	char *q;

	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_INODE);
	q = strmov(q, " (ino, gen, type, size, uid, gid,"
	    " create_time, modify_time) ");
	sprintf(q, " VALUES ('%s', '%s', '%s', '%s', "
	    "'%s', '%s', '%s', '%s')", ino, gen, type,
	    p[0], p[1], p[2], p[3], p[4]);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(MTR_file, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int) (q-SAMDB_qbuf))) {
		mtrlog(1, 0, "insert fail [%s]: ino=%s, %s",
		    T_SAM_INODE, p[0], mysql_error(SAMDB_conn));
		query_error(T_SAM_INODE);
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		mtrlog(1, 0, "insert rows [%s]: ino=%s, rows=%ld",
		    T_SAM_INODE, p[0], rc);
	}

	return (0);
}

/*
 * 	SAM_db_insert_link - Insert symbolic link into DB.
 *
 *	Description:
 *	    Insert symbol link into sam_link table.
 *
 *	Entry:
 *	    ino	 Inode number.
 *	    gen	 Generation number.
 *	    link Parameter vector: path, obj_name.
 */
static int
SAM_db_insert_link(
	char *ino,	/* Inode number */
	char *gen,	/* Generation number */
	char *link)	/* Synmbolic link */
{
	my_ulonglong rc; /* Record count from query	*/
	char *q;

	/* Build insert for sam_path table */
	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_LINK);
	q = strmov(q, " (ino, gen, link) ");
	sprintf(q, " VALUES ('%s', '%s', '", ino, gen);
	q = strend(q);
	q += mysql_real_escape_string(SAMDB_conn, q, link, strlen(link));
	q = strmov(q, "');");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(MTR_file, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		mtrlog(1, 0, "insert fail [%s]: ino=%s, %s",
		    T_SAM_LINK, ino, mysql_error(SAMDB_conn));
		query_error(T_SAM_LINK);
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		mtrlog(1, 0, "insert rows [%s]: ino=%s, rows=%ld",
		    T_SAM_PATH, ino, rc);
	}

	return (0);
}

/*
 *  SAM_db_insert_vsn - Insert VSN into DB.
 *
 *	Description:
 *	    Insert VSN link into sam_vsns table.
 *
 *	Entry:
 *	    media	Media type (e.g. li).
 *		vsn		VSN.
 */
static int
SAM_db_insert_vsn(
	char *media,	/* Media type */
	char *vsn)	/* VSN */
{
	my_ulonglong rc; /* Record count from query */

	MYSQL_RES *res;	/* mySQL fetch results */
	MYSQL_ROW row;	/* mySQL row result */
	int vsn_id;	/* VSN id ordinals */
	char *q;

	vsn_id = 0;

	/* Lookup VSN from sam_vsns table */
	q = strmov(SAMDB_qbuf, "SELECT id FROM ");
	q = strmov(q, T_SAM_VSNS);
	q = strmov(q, " WHERE media_type = '");
	sprintf(q, "%s", media);
	q = strend(q);
	q = strmov(q, "' AND vsn = '");
	sprintf(q, "%s", vsn);
	q = strend(q);
	q = strmov(q, "';");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(MTR_file, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		mtrlog(1, 0, "select fail [%s]: %s, %s, %s",
		    T_SAM_VSNS, media, vsn, mysql_error(SAMDB_conn));
		query_error(T_SAM_VSNS);
		return (-1);
	}

	if ((res = mysql_store_result(SAMDB_conn)) == NULL) {
		mtrlog(1, 0, "results fail [%s]: %s, %s, %s",
		    T_SAM_VSNS, media, vsn, mysql_error(SAMDB_conn));
		return (-1);
	}

	rc = mysql_num_rows(res);

	if (rc == 0) {
		goto free1;
	}

	if (rc != 1) {
		mtrlog(1, 0, "multiple VSNs [%s]: %s, %s",
		    T_SAM_VSNS, media, vsn);
		vsn_id = -1;
		goto free1;
	}

	row = mysql_fetch_row(res);

	if (row == NULL) {
		mtrlog(1, 0, "Mising row [%s]: %s, %s", T_SAM_VSNS, media, vsn);
		vsn_id = -1;
		goto free1;
	}

	if (row[0] == NULL) {
		mtrlog(1, 0, "Mising id [%s]: %s, %s", T_SAM_VSNS, media, vsn);
		vsn_id = -1;
	} else {
		vsn_id = atoi(row[0]);
	}

free1:
	mysql_free_result(res);

	if (vsn_id == -1) {
		return (-1);
	}

	/* Insert missing VSN into sam_vsns table */
	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_VSNS);
	q = strmov(q, " (media_type, vsn)");
	sprintf(q, " VALUES ( '%s', '%s')", media, vsn);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(MTR_file, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		mtrlog(1, 0, "insert fail [%s]: %s, %s",
		    T_SAM_VSNS, media, vsn);
		query_error(T_SAM_VSNS);
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		mtrlog(1, 0, "insert rows [%s]: %s, %s, rows=%ld",
		    T_SAM_VSNS, media, vsn, rc);
		return (-1);
	}

	vsn_id = mysql_insert_id(SAMDB_conn);
	return (vsn_id);
}

/*
 * SAM_db_insert_archive - Insert archive entry into DB.
 *
 *	Description:
 *	    Insert archive entry into sam_archive.
 *
 *	Entry:
 *	    p	Parameter vector:, ino, gen, dir, size, modify_time
 *		creation_time, path, obj_name.
 */
static int
SAM_db_insert_archive(
	char *ino,
	char *gen,
	LEVEL_T *ar,
	char *modify_time)
{
	my_ulonglong rc;	/* Record count from query */
	char *q;
	int stale;		/* Stale flag */

	stale = strcmp(ar->modify, modify_time) != 0;

	/* 1. Build insert for sam_path table */
	q = strmov(SAMDB_qbuf, "INSERT INTO ");
	q = strmov(q, T_SAM_ARCHIVE);
	q = strmov(q, " (ino, gen, copy, seq, vsn_id, size,"
	    " create_time, modify_time, stale) ");
	sprintf(q, " VALUES ('%s', '%s', '%d', '%d', '%d', "
	    "'%s', '%s', '%s', '%d')", ino, gen, ar->level,
	    ar->seq, ar->vsn_id, ar->size, ar->create,
	    ar->modify, stale);
	q = strend(q);
	q = strmov(q, ";");
	*q = '\0';

	if (SAMDB_Debug) {
		fprintf(MTR_file, "QUERY: %s\n", SAMDB_qbuf);
	}

	if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
	    (unsigned int)(q-SAMDB_qbuf))) {
		mtrlog(1, 0, "insert fail [%s]: id=%d, %s",
		    T_SAM_ARCHIVE, ar->vsn_id, mysql_error(SAMDB_conn));
		query_error(T_SAM_ARCHIVE);
		return (-1);
	}

	rc = mysql_affected_rows(SAMDB_conn);

	if (rc != 1) {
		mtrlog(1, 0, "insert rows [%s]: id=%d, rows=%ld",
		    T_SAM_ARCHIVE, ar->vsn_id, rc);
		return (-1);
	}

	return (0);
}

void query_error(
	char *tbl)	/* Table name	*/
{
	int ec;	/* Error code */

	mtrlog(1, 0, "Query: %s", SAMDB_qbuf);

	ec = mysql_errno(SAMDB_conn);

	if (ec < CR_MIN_ERROR || ec > CR_MAX_ERROR) {
		return;
	}

	mtrlog(0, 0, "files processed %d, elapse time %d", n_files, time(0)
	    -Start_Time);
	mtrlog(0, 0, "records processed %d, %d/%d/%d", 2*n_path+n_archive+n_vsn,
	    n_path, n_archive, n_vsn);

	error(1, 0, "Client error [%s]: %s", tbl, mysql_error(SAMDB_conn));
}

void program_help(void) {
	printf("Usage:\t%s %s\n", program_name,
			"fs_name [-g] [-r] [-v|V] load_file(s)");
	exit(-1);
}
