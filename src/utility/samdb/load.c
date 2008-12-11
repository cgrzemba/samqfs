/*
 * load.c - load a sideband database for a filesystem
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

/*
 *      Load File Format:
 *
 *          sam_db load file format (all fields seperated by tabs):
 *
 *          ino gen type size checksum create_time modify_time uid gid online
 *	    path object_name
 *          symbolic_link_value
 *          copy seq media_type vsn position offset size create_time stale
 *          /EOS
 *
 *              File types:
 *              0       regular file (S_ISREG)
 *              1       directory (S_ISDIR)
 *              2       segment file segment (S_ISSEGS)
 *              3       segment file (S_ISSEGI)
 *              4       symbolic link (S_ISLNK)
 *              5       something else
 *
 *          All entries end with /EOS line.
 *          Archive link repeats for each archive entry, seq column identifies
 *          multi-VSN set, with size being the number of bytes this VSN.
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
#pragma ident "$Revision: 1.2 $"

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include <sys/vfs.h>

#include <pub/stat.h>
#include <pub/devstat.h>
#include <sam/mount.h>
#include <sam/samevent.h>
#include <sam/sam_db.h>

#define	MAIN
#include <sam/fs/block.h>
#include <sam/fs/sblk.h>
#include <sam/fs/utility.h>
#undef MAIN

#include "samdb.h"
#include "event_handler.h"
#include "util.h"

#define	LOAD_BUF_LEN 32767
#define	LOAD_MAX_FIELDS 10
#define	INODE_NUM_FIELDS 10
#define	FILE_NUM_FIELDS 2
#define	LINK_NUM_FIELDS 1
#define	ARCH_NUM_FIELDS 9

extern ushort_t sam_dir_gennamehash(int nl, char *np);

static int load_proc_opt(char opt, char *arg);
static int load_from_file(sam_db_context_t *con, FILE *file);
static int load_inode(sam_perm_inode_t *ip, void *arg);
static int load_dirent(sam_id_t pid, struct sam_dirent *dirent, void *arg);
static int load_entry(sam_db_context_t *con, char *entry);
static char *parse_fields(char *entry, char *fields[], int num_fields);
static int normalize_path(char *norm_path, char *path, char *mp, int len);

static boolean_t inode_load = FALSE;
static char *loadfile;
static char load_buf[LOAD_BUF_LEN+1];

int
samdb_load(samdb_args_t *args) {
	FILE *file;

	if (sam_db_connect(args->con) < 0) {
		fprintf(stderr, "Can't connect to %s database.\n",
		    args->fsname);
		return (-1);
	}

	if (inode_load) {
		/* Read inodes, loading each inode using callback function */
		if (read_inodes(args->con->mount_point,
		    load_inode, args->con) < 0) {
			fprintf(stderr, "Error reading inodes\n");
			return (-1);
		}
	} else {
		if (loadfile == NULL) {
			file = stdin;
		} else if ((file = fopen(loadfile, "r")) == NULL) {
			fprintf(stderr, "Can't open %s for read.\n", loadfile);
			return (-1);
		}

		if (load_from_file(args->con, file) < 0) {
			fprintf(stderr, "Error loading database.\n");
		}
		fclose(file);
	}

	return (0);
}

samdb_opts_t *
samdb_load_getopts(void) {
	static opt_desc_t opt_desc[] = {
		{"i", "Inode scan for load"},
		{"f file", "Load input file, default standard input."},
		NULL,
	};
	static samdb_opts_t opts = {
		"if:",
		load_proc_opt,
		opt_desc
	};

	return (&opts);
}

static int
load_proc_opt(char opt, char *arg) {
	switch (opt) {
	case 'i': inode_load = TRUE; break;
	case 'f': loadfile = arg; break;
	}
	return (0);
}

static int
load_inode(sam_perm_inode_t *ip, void *arg) {
	sam_db_context_t *con = (sam_db_context_t *)arg;
	sam_db_inode_t inode;
	sam_db_path_t path;
	sam_db_archive_t *archive;
	sam_event_t check_event;
	int copy;

	/* Skip inodes that don't belong in database */
	if (!IS_DB_INODE(ip->di.id.ino) ||
	    !IS_DB_INODE(ip->di.parent_id.ino)) {
		return (0);
	} else if (ip->di.id.ino == SAM_ROOT_INO) {
		/* We know that the root inode exists, check it */
		goto check_error;
	}

	memset(&inode, 0, sizeof (sam_db_inode_t));
	memset(&path, 0, sizeof (sam_db_path_t));
	memset(&check_event, 0, sizeof (sam_event_t));

	(void) sam_db_inode_new_perm(ip, &inode);
	if (sam_db_inode_insert(con, &inode) < 0) {
		goto check_error;
	}

	if (inode.type == FTYPE_DIR || inode.type == FTYPE_LINK) {
		sam_id_t allids = {0, -1};
		if (sam_db_path_new(con, ip->di.id, -1, &path) < 0) {
			goto check_error;
		}
		if (sam_db_path_insert(con, &path) < 0) {
			goto check_error;
		}
		/* Load directory entries into file table */
		if (inode.type == FTYPE_DIR && sam_db_id_allname(con,
		    ip->di.id, allids, load_dirent, con) < 0) {
			goto check_error;
		}
	}

	/* Insert archive information */
	for (copy = 0; copy < MAX_ARCHIVE; copy++) {
		if (ip->di.arch_status & (1<<copy)) {
			int nvsn, i;
			if ((nvsn = sam_db_archive_new(con, ip->di.id,
			    copy, &archive)) < 0) {
				goto check_error;
			}
			for (i = 0; i < nvsn; i++) {
				if (sam_db_archive_replace(con,
				    &archive[i]) < 0) {
					sam_db_archive_free(&archive);
					goto check_error;
				}
			}
			sam_db_archive_free(&archive);
		}
	}

	return (0);

check_error:
	/*
	 * If there was an error run check_consistency to see
	 * if we can recover from it.
	 */
	check_event.ev_num = ev_create;
	check_event.ev_id.ino = inode.ino;
	check_event.ev_id.gen = inode.gen;
	check_event.ev_pid.ino = ip->di.parent_id.ino;
	check_event.ev_pid.gen = ip->di.parent_id.gen;
	return (check_consistency(con, &check_event, TRUE));
}

/*
 * Load a directory entry into the file table.
 */
static int
load_dirent(sam_id_t pid, struct sam_dirent *dirent, void *arg) {
	sam_db_context_t *con = (sam_db_context_t *)arg;
	sam_db_file_t file;

	/* Ignore . and .. */
	if (dirent->d_name[0] == '.' && (dirent->d_namlen == 1 ||
	    (dirent->d_namlen == 2 && dirent->d_name[1] == '.'))) {
		return (0);
	}

	if (sam_db_file_new_byname(dirent->d_id, pid, dirent->d_namehash,
	    (char *)dirent->d_name, &file)) {
		return (-1);
	}
	if (sam_db_file_insert(con, &file) < 0) {
		return (-1);
	}
	return (0);
}

/*
 * Load the database with data from the provided file.
 *
 * Reads the file and loads each entry.
 * Entries are delimited by /EOS lines.
 *
 * Returns 0 on succes, -1 on failure.
 */
static int
load_from_file(sam_db_context_t *con, FILE *file) {
	int offset = 0;
	while (fread(load_buf + offset, 1, LOAD_BUF_LEN - offset, file) > 0) {
		char *start = load_buf;
		char *end = strstr(load_buf, "/EOS\n");
		if (end == NULL) {
			fprintf(stderr, "Entry >%d bytes, exiting.",
			    LOAD_BUF_LEN);
			return (-1);
		}
		*end = '\0';
		while (end != NULL) {
			offset = end - start;
			if (load_entry(con, start) < 0) {
				return (-1);
			}
			start = end + 5; // Move past /EOS\n
			end = strstr(start, "/EOS\n");
			if (end != NULL) {
				*end = '\0';
			}
		}
		offset = LOAD_BUF_LEN - (start - load_buf);
		memmove(load_buf, start, offset);
	}

	return (0);
}

/*
 * Parse the next entry from the file.
 * Creates inode, file, path and archive info and
 * inserts them into the database.
 * Returns 0 on success, -1 on error.
 */
static int
load_entry(sam_db_context_t *con, char *entry) {
	static int line_num = 0;
	char *field[LOAD_MAX_FIELDS];
	sam_db_inode_t inode;
	sam_db_file_t file;
	sam_db_path_t path;
	sam_db_archive_t archive;
	sam_event_t check_event;
	char pid_path[MAXPATHLEN];
	int len;
	struct sam_stat pid_stat;
	memset(&inode, 0, sizeof (sam_db_inode_t));
	memset(&file, 0, sizeof (sam_db_file_t));
	memset(&path, 0, sizeof (sam_db_path_t));
	memset(&archive, 0, sizeof (sam_db_archive_t));
	memset(&pid_stat, 0, sizeof (struct sam_stat));
	memset(&check_event, 0, sizeof (sam_event_t));

	/* Load first line */
	line_num++;
	if ((entry = parse_fields(entry, field, INODE_NUM_FIELDS)) == NULL) {
		fprintf(stderr, "Error parsing inode line %d\n", line_num);
		return (-1);
	}

	inode.ino = atoi(field[0]);
	inode.gen = atoi(field[1]);
	inode.type = atoi(field[2]);
	file.ino = inode.ino;
	file.gen = inode.gen;
	path.ino = inode.ino;
	path.gen = inode.gen;
	archive.ino = inode.ino;
	archive.gen = inode.gen;

	inode.size = atoll(field[3]);
	strncpy(inode.csum, field[4], sizeof (inode.csum));
	inode.create_time = atoi(field[5]);
	inode.modify_time = atoi(field[6]);
	inode.uid = atoi(field[7]);
	inode.gid = atoi(field[8]);
	inode.online = atoi(field[9]);

	/* Load path line */
	line_num++;
	if ((entry = parse_fields(entry, field, FILE_NUM_FIELDS)) == NULL) {
		fprintf(stderr, "Error parsing path line %d\n", line_num);
		return (-1);
	}

	if (normalize_path(path.path, field[0],
	    con->mount_point, MAXPATHLEN) < -1) {
		fprintf(stderr, "Invalid path: %s line %d\n",
		    field[0], line_num);
		return (-1);
	}
	strncpy(file.name, field[1], MAXNAMELEN);
	file.name_hash = sam_dir_gennamehash(strlen(file.name), file.name);

	/* Get parent information */
	strcpy(pid_path, con->mount_point);
	len = strlcat(pid_path, path.path, MAXPATHLEN);
	pid_path[len-1] = '\0';	/* Remove trailing / for stat */
	if (sam_stat(pid_path, &pid_stat, sizeof (pid_stat))) {
		fprintf(stderr, "Failed getting parent stat %s line %d\n",
		    pid_path, line_num);
		return (-1);
	}
	file.p_ino = pid_stat.st_ino;
	file.p_gen = pid_stat.gen;


	/* Load link line */
	if (inode.type == FTYPE_LINK) {
		line_num++;
		if ((entry = parse_fields(entry, field,
		    LINK_NUM_FIELDS)) == NULL) {
			fprintf(stderr, "Error parsing entry link line %d.\n",
			    line_num);
			return (-1);
		}
		strncpy(path.path, field[0], MAXPATHLEN);
	} else if (inode.type == FTYPE_DIR) {
		strcat(path.path, file.name);
		strcat(path.path, "/");
	}

	/* Insert into the database */
	if (sam_db_inode_insert(con, &inode) < 0) {
		goto check_error;
	}
	if (sam_db_file_insert(con, &file) < 0) {
		goto check_error;
	}
	if (inode.type == FTYPE_LINK || inode.type == FTYPE_DIR) {
		if (sam_db_path_insert(con, &path) < 0) {
			goto check_error;
		}
	}

	/* Process archive lines */
	while (*entry != '\0') {
		line_num++;
		if ((entry = parse_fields(entry, field,
		    ARCH_NUM_FIELDS)) == NULL) {
			fprintf(stderr, "Error parsing archive line %d.\n",
			    line_num);
			return (-1);
		}
		archive.copy = atoi(field[0]);
		archive.seq = atoi(field[1]);
		strncpy(archive.media_type, field[2],
		    sizeof (archive.media_type));
		strncpy(archive.vsn, field[3], sizeof (archive.vsn));
		archive.position = atoll(field[4]);
		archive.offset = atoi(field[5]);
		archive.size = atoll(field[6]);
		archive.create_time = atoi(field[7]);
		archive.stale = atoi(field[8]);

		/* Replace any existing archive entries */
		if (sam_db_archive_replace(con, &archive) < 0) {
			goto check_error;
		}
	}

	return (0);

check_error:
	/*
	 * If there was an error run check_consistency to see
	 * if we can recover from it.
	 */
	check_event.ev_num = ev_create;
	check_event.ev_id.ino = inode.ino;
	check_event.ev_id.gen = inode.gen;
	check_event.ev_pid.ino = file.p_ino;
	check_event.ev_pid.gen = file.p_gen;
	return (check_consistency(con, &check_event, TRUE));
}

/*
 * Parses a tab delimited line into fields.
 *	entry - line to parse
 *	field - field array to enter fields
 *	num_fields - number of fields to parse
 *
 * Returns pointer past parsed fields in entry
 * On Return: field will have pointers into entry
 * 	entry is modified to have tabs replaced by \0
 */
static char *
parse_fields(char *entry, char *field[], int num_fields) {
	static char *delim = "\t\n";
	char *lasts;
	int field_i = 0;
	char *tok = strtok_r(entry, delim, &lasts);

	while (tok != NULL && field_i < num_fields) {
		field[field_i++] = tok;
		if (field_i < num_fields) {
			tok = strtok_r(NULL, delim, &lasts);
		}
	}

	if (field_i != num_fields) {
		fprintf(stderr, "Insufficient fields in load file.\n");
		return (NULL);
	}

	if ((lasts = strchr(field[field_i-1], '\0')) == NULL) {
		return (NULL);
	}

	return (lasts + 1);
}

/*
 * Munges the provided path into a normalized format
 * required by the database. Specifically absolute paths
 * have their mount point removed and relative paths
 * have any leading dot directories removed.
 *
 * norm_path - buffer to copy normalized path into
 * path - path to normalize
 * mp - mount point
 * len - length of norm_path buffer
 *
 * Returns strlen(norm_path) on success, -1 on failure
 */
static int
normalize_path(char *norm_path, char *path, char *mp, int len) {
	int nlen;
	/*
	 * Check if absolute path (begins with /).
	 * If so must begin with mount point.  Otherwise
	 * we assume it is a relative path.
	 */
	if (*path == '/') {
		if (strncmp(path, mp, strlen(mp)) == 0) {
			nlen = strlcpy(norm_path, path + strlen(mp), len);
		} else {
			fprintf(stderr, "absolute path incorrect mount point, "
			    "path %s", path);
			return (-1);
		}
	} else {
		/* Skip past leading relative path ./ */
		if (strcmp(path, ".") == 0) {
			norm_path[0] = '/';
			nlen = 1;
		} else if (strncmp(path, "./", 2) == 0) {
			nlen = strlcpy(norm_path, path + 1, len);
		} else {
			nlen = strlcpy(norm_path, path, len);
		}
	}

	/* Last character should be '/' */
	if (nlen == 0 || norm_path[nlen-1] != '/') {
		norm_path[nlen++] = '/';
		norm_path[nlen] = '\0';
	}

	return (nlen);
}
