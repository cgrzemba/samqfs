/*
 * check.c - consistency check the database against the current filesystem
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
 * The purpose of check is to make sure the database is
 * consistent with the filesystem.  The checker scans the .inodes file
 * and examines the entries in the database against each file.
 */

#pragma ident "$Revision: 1.1 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <time.h>
#include <signal.h>
#include <strings.h>

#include "sam/param.h"
#include "sam/types.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "sam/custmsg.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "sam/fs/sblk.h"
#include "sam/sam_trace.h"
#include "sam/sam_malloc.h"

#define	DEC_INIT
#include "sam/sam_db.h"
#undef DEC_INIT

/* Local Includes */
#include "arg.h"
#include "util.h"

#ifndef INO_BLK_FACTOR
#define	INO_BLK_FACTOR 32
#endif

/* Buffer for inodes read from disk */
static union sam_di_ino
	inodes[(INO_BLK_FACTOR * INO_BLK_SIZE) / sizeof (union sam_di_ino) + 2];

/* Statistics buffer */
static struct {
	int	total_inodes;
	int	wrong_inode_number;
	int	zero_inode_number;
	int	total_deleted_rows;
} stats;

/* From lib/update.c */
extern void Update(sam_id_t, sam_id_t, int, time_t, char *);

/* Local prototypes */
static void init(void);
static void sigint_handler(int);
static void cleanup_resources(void);
static void check_inode(struct sam_perm_inode *);
static int delete_prev_gen(sam_id_t, time_t);
static void show_stats(void);
static void program_help(void);

/* my name */
char *program_name = "samdb_check";

/* command-line arguments */
static char *fs_name;
static char *mount_point;
static int inode_fd;
static sam_db_connect_t SAM_db;		/* mySQL connect parameters */

arg_t Arg_Table[] = {	/* Argument table */
	{ "-help", ARG_PART, (char *)1 },
	{ "-version", ARG_VERS, NULL },
	{ "-v", ARG_TRUE, (char *)&SAMDB_Verbose },
	{ "-V", ARG_TRUE, (char *)&SAMDB_Debug },
	{ NULL, ARG_PART, NULL } };

/* database consitency check */
int
main(
	int argc,
	char **argv)
{
	int expected_ino;
	int inode_i;
	int ninodes;
	int ngot;
	int i;

	if (argc < 2) {
		program_help();
	}

	fs_name = *++argv;
	argc--;

	init_trace(NULL, 0);

	while ((i = Process_Command_Arguments(Arg_Table, &argc, &argv)) > 0) {
		if (i == 1) {
			program_help();
		}
	}

	if (i < 0) {
		Trace(TR_ERR, "Argument errors");
		exit(EXIT_FAILURE);
	}

	if (SAMDB_Debug) {
		SAMDB_Verbose = TRUE;
	}

	init();

	expected_ino = 0;

	/*
	 * read the inodes file, check database for each inode read
	 */
	while ((ngot = read(inode_fd, &inodes,
	    INO_BLK_FACTOR * INO_BLK_SIZE)) > 0) {
		ninodes = ngot / sizeof (union sam_di_ino);

		for (inode_i = 0; inode_i < ninodes; inode_i++) {
			struct sam_perm_inode *ip = &inodes[inode_i].inode;
			expected_ino++;
			stats.total_inodes++;

			if ((ip->di.id.ino == 0) ||
			    (ip->di.id.gen == 0)) {
				stats.zero_inode_number++;
				continue;
			}

			/* Skip privileged inodes and removable media inodes */
			if (SAM_PRIVILEGE_INO(ip->di.version, ip->di.id.ino) ||
			    ip->di.parent_id.ino == SAM_STAGE_INO ||
			    ip->di.parent_id.ino == SAM_ARCH_INO ||
			    S_ISREQ(ip->di.mode)) {
				continue;
			}

			if (ip->di.id.ino != expected_ino) {
				stats.wrong_inode_number++;
				continue;
			}

			check_inode(ip);
		}
	}

	if (SAMDB_Verbose) {
		show_stats();
	}

	return (EXIT_SUCCESS);
}

static void
show_stats()
{
#define	display(x) fprintf(stdout, #x ": %d\n", stats.x);
	display(total_inodes);
	display(wrong_inode_number);
	display(zero_inode_number);
	display(total_deleted_rows);
}


/*
 * init()
 * 	setup signal handling
 * 	initialize file system variables
 * 	setup database connection
 */
static void
init(void)
{
	struct sam_fs_info fi;
	sam_db_access_t *dba; /* DB access information */

	/* Init stats */
	memset(&stats, 0, sizeof (stats));

	/* Redirect trace messages to stdout/stderr.  */
	TraceInit(NULL, TI_none);
	*TraceFlags = (1 << TR_module) | ((1 << TR_date) - 1);
	*TraceFlags &= ~(1 << TR_alloc);
	*TraceFlags &= ~(1 << TR_dbfile);
	*TraceFlags &= ~(1 << TR_files);
	*TraceFlags &= ~(1 << TR_oprmsg);
	*TraceFlags &= ~(1 << TR_sig);

	/*
	 * set up signal handling.  SIGINT makes us exit;
	 * sam-fsd will send us a SIGHUP on a configure; ignore it.
	 */
	if (signal(SIGINT, &sigint_handler) == (void (*)(int))SIG_ERR) {
		Trace(TR_ERR, "Cannot set handler for SIGINT");
		exit(EXIT_FAILURE);
	}
	if (signal(SIGHUP, SIG_IGN) == (void (*)(int))SIG_ERR) {
		Trace(TR_ERR, "Cannot set handler for SIGHUP");
		exit(EXIT_FAILURE);
	}

	if (GetFsInfo(fs_name, &fi) == -1) {
		/* Filesystem \"%s\" not found. */
		SendCustMsg(HERE, 620, fs_name);
		exit(EXIT_FAILURE);
	}
	if (!(fi.fi_status & FS_MOUNTED)) {
		/* Filesystem \"%s\" not mounted. */
		SendCustMsg(HERE, 8001, fs_name);
		exit(EXIT_FAILURE);
	}

	mount_point = fi.fi_mnt_point;

	if ((SAMDB_fd = open(mount_point, O_RDONLY)) < 0) {
		Trace(TR_ERR, "Can't open mount point: %s", mount_point);
	}

	if ((inode_fd = OpenInodesFile(mount_point)) < 0) {
		Trace(TR_ERR, "Could not open %s/.inodes", fi.fi_mnt_point);
		exit(EXIT_FAILURE);
	}

	/* begin processing .inodes at its beginning */
	if (lseek(inode_fd, 0, SEEK_SET) != 0) {
		Trace(TR_ERR, "Could not lseek() .inodes to BOF");
		exit(EXIT_FAILURE);
	}

	/* Get database access data */
	if ((dba = sam_db_access(SAMDB_ACCESS_FILE, fs_name)) == NULL) {
		Trace(TR_ERR, "family set name '%s' not found in %s",
		    fs_name, SAMDB_ACCESS_FILE);
		exit(EXIT_FAILURE);
	}

	SAM_db.SAM_host = dba->db_host;
	SAM_db.SAM_user = dba->db_user;
	SAM_db.SAM_pass = dba->db_pass;
	SAM_db.SAM_name = dba->db_name;
	SAM_db.SAM_port = *dba->db_port != '\0' ?
	    atoi(dba->db_port) : SAMDB_DEFAULT_PORT;
	SAM_db.SAM_client_flag = *dba->db_client != '\0' ?
	    atoi(dba->db_client) : SAMDB_CLIENT_FLAG;

	/* Connect to the database */
	if (sam_db_connect(&SAM_db)) {
		Trace(TR_ERR, "Connect fail: %s", mysql_error(SAMDB_conn));
		exit(EXIT_FAILURE);
	}

	SamFree(dba);

	/* Disconnect from database at exit */
	atexit(cleanup_resources);
}

static void
cleanup_resources(void)
{
	sam_db_disconnect();
}

/*
 * Checks the provided inode against the database to make sure
 * the database is up to date.
 */
static void
check_inode(struct sam_perm_inode *ip)
{
	time_t cur_time;
	time(&cur_time);

	/* Marks previous generations deleted */
	stats.total_deleted_rows += delete_prev_gen(ip->di.id, cur_time);

	/*
	 * Update regular files, directories and links.
	 *
	 * FIXME: There are more efficient ways
	 * to perform this check, but for now this
	 * utilizes existing code and meets our
	 * current implementation time constraint.
	 */
	if (S_ISREG(ip->di.mode) || S_ISDIR(ip->di.mode) ||
	    S_ISLNK(ip->di.mode)) {
		Update(ip->di.id, ip->di.parent_id, 0,
		    cur_time, mount_point);
	}
}

/*
 * Mark deleted all entries in the database with generation number
 * less than the current generation number.
 */
static int
delete_prev_gen(sam_id_t id, time_t cur_time)
{
#define	NUM_DEL_TABLES 2
	static char *del_tables[] = {T_SAM_INODE, T_SAM_PATH};
	char *del_q;
	int num_del = 0;
	int i;

	for (i = 0; i < NUM_DEL_TABLES; i++) {
		/*
		 * Example: Marks rows deleted with smaller gen numbers
		 *  UPDATE sam_inode SET deleted=1, delete_time=123
		 *  WHERE ino=567 AND gen<56 AND deleted=0
		 */
		del_q = strmov(SAMDB_qbuf, "UPDATE ");
		del_q = strmov(del_q, del_tables[i]);
		del_q = strmov(del_q, " SET deleted=1,");
		sprintf(del_q, "delete_time=%lu ", cur_time);
		del_q = strend(del_q);
		sprintf(del_q, "WHERE ino=%u AND gen<%u AND deleted=0",
		    id.ino, id.gen);
		del_q = strend(del_q);

		if (SAMDB_Debug) {
			Trace(TR_DEBUG, SAMDB_qbuf);
		}

		if (mysql_real_query(SAMDB_conn, SAMDB_qbuf,
		    (unsigned int)(del_q-SAMDB_qbuf))) {
			Trace(TR_ERR, "update fail: %s",
			    mysql_error(SAMDB_conn));
			exit(EXIT_FAILURE);
		}

		num_del += mysql_affected_rows(SAMDB_conn);
	}

	return (num_del);
}

/*
 * sigint_handler - handle a SIGINT.  SIGINT causes an orderly shutdown.
 * Any clean-up tasks have been queued by calling atexit(), so all we
 * have to do is exit().
 */
static void
sigint_handler(
/*LINTED argument unused in function */
	int ignored)
{
	exit(EXIT_FAILURE);
}

static void
program_help(void)
{
	printf("Usage:\t%s %s\n", program_name, "fs_name [-v|V]");
	exit(EXIT_FAILURE);
}
