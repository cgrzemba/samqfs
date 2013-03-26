/*
 * sam_db.h - SAMFS Database Definitions.
 *
 *	Description:
 *	    sam_db.h contains definitions for the SAMdb database system.
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#if !defined(SAM_DB_H)
#define	SAM_DB_H

#include <sam/fs/ino.h>
#include <sam/fs/dirent.h>
#include <sam/resource.h>
#include <mysql.h>

#define	SAMDB_DEFAULT_PORT	3306
#define	SAMDB_CLIENT_FLAG	CLIENT_FOUND_ROWS
#define	SAMDB_SQL_MAXLEN	32768
#define	SAMDB_CACHE_LEN 	20

#define	SAMDB_ACCESS_FILE	"/etc/opt/SUNWsamfs/samdb.conf"
#define	SAMDB_SCHEMA_FILE	"/opt/SUNWsamfs/etc/samdb.schema"
#define	SAMDB_SQL_CATALOG	"/opt/SUNWsamfs/etc/samdb_sql.cat"

/* SAM table names */
#define	SAMDB_T_INODE		"sam_inode"
#define	SAMDB_T_PATH		"sam_path"
#define	SAMDB_T_LINK		"sam_file"
#define	SAMDB_T_ARCHIVE		"sam_archive"
#define	SAMDB_T_VERSION		"sam_version"

/* Bind helper usage: SAMDB_BIND(bind[0], p->ino, MYSQL_TYPE_LONG, TRUE) */
#define	SAMDB_BIND(var, val, type, is_unsgn) \
	var.is_unsigned = is_unsgn;\
	var.buffer_type = type;\
	var.buffer_length = sizeof (val);\
	var.buffer = &val

#define	SAMDB_BIND_STR(var, val) \
	var.buffer_type = MYSQL_TYPE_STRING;\
	var.buffer_length = strlen(val);\
	var.buffer = val

typedef struct sam_db_conf {		/* SAM DB access control:	*/
	char	*db_fsname;		/* Family set name		*/
	char	*db_host;		/* hostname			*/
	char	*db_user;		/* DB user name			*/
	char	*db_pass;		/* DB password			*/
	char	*db_name;		/* DB name			*/
	char	*db_port;		/* Port number			*/
	char	*db_client;		/* Client flag (see mySQL)	*/
	char	*db_mount;		/* Mount point			*/
} sam_db_conf_t;

typedef struct cache_stmt {		/* SAM DB statment cache entry	*/
	int		sql_id;		/* id of prepared statement	*/
	int		last_access;	/* counter time of last access	*/
	MYSQL_STMT	*stmt;		/* Cached MYSQL statement	*/
} cache_stmt_t;

typedef	struct sam_db_context {		/* SAM DB connect control:	*/
	char		*host;		/* Hostname			*/
	char		*user;		/* DB user name			*/
	char		*pass;		/* DB password			*/
	char		*dbname;	/* DB name			*/
	unsigned int	port;		/* Port number 			*/
	unsigned long	client_flag; 	/* Client flag (see mySQL)	*/
	MYSQL 		*mysql;		/* mySQL connection 		*/
	char 		*qbuf;		/* Query buffer 		*/
	char		*mount_point;	/* SAM mount point		*/
	int		sam_fd;		/* SAM mount point descriptor	*/
	int		cache_size;	/* Current size of stmt cache   */
	cache_stmt_t 	stmt_cache[SAMDB_CACHE_LEN]; /* LRU stmt cache	*/
} sam_db_context_t;

typedef	enum sam_db_ftype {		/* SAM db file types		*/
	FTYPE_REG   = 0,		/* Regular file 		*/
	FTYPE_DIR   = 1,		/* Directory			*/
	FTYPE_SEGI  = 2,		/* Segment file			*/
	FTYPE_SEG   = 3,		/* Segment file			*/
	FTYPE_LINK  = 4,		/* Symbolic Link		*/
	FTYPE_OTHER = 5			/* Something else		*/
} sam_db_ftype_t;

/*
 * 	mySQL database schemas.
 *	These structures reflect the database schema.
 */
typedef	struct sam_db_inode {		/* SAM inode table:		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
	unsigned char	type;		/* File type			*/
	long long	size;		/* File size			*/
	char		csum[34];	/* File checksum		*/
	unsigned int	create_time;	/* File creation time		*/
	unsigned int	modify_time;	/* File modification time	*/
	unsigned int	uid;		/* User id			*/
	unsigned int	gid;		/* Group id			*/
	unsigned char	online;		/* File online/offline		*/
} sam_db_inode_t;

typedef	struct sam_db_path {		/* SAM path table:		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
	char		path[MAXPATHLEN+2]; /* Path name		*/
} sam_db_path_t;

typedef	struct sam_db_file {		/* SAM file table:		*/
	unsigned int	p_ino;		/* Parent Inode number		*/
	unsigned int	p_gen;		/* Parent Generation number	*/
	unsigned short	name_hash;	/* File name hash code		*/
	char		name[MAXNAMELEN+2]; /* File name		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
} sam_db_file_t;

typedef	struct sam_db_archive {		/* SAM archive table:		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
	unsigned char	copy;		/* Archive copy number		*/
	unsigned short	seq;		/* Multi-VSN sequence number	*/
	char		media_type[4];	/* Media type			*/
	char		vsn[34];	/* VSN				*/
	unsigned long long position;	/* Archive file position	*/
	unsigned int	offset;		/* Archive file offset		*/
	unsigned long long size;	/* Archive file size		*/
	unsigned int	create_time;	/* Archive creation time	*/
	unsigned char	stale;		/* Stale entry flag		*/
} sam_db_archive_t;

/* Callback function type for sam_db_id_allname(). Returns -1 on error */
typedef int (*sam_db_dirent_cb)(sam_id_t pid, struct sam_dirent *, void *);

/* From libmysqlclient (m_string.h) */
extern char *strend(const char *);
/* extern char *strmov(char *, char *); */

/* config functions */
sam_db_conf_t *sam_db_conf_get(char *cfgfile, char *fsname);
void sam_db_conf_free(sam_db_conf_t *);

/* connection management */
sam_db_context_t *sam_db_context_conf_new(sam_db_conf_t *);
sam_db_context_t *sam_db_context_new(char *host, char *user, char *pass,
    char *dbname, unsigned int port,
    unsigned long client_flag, char *sam_mount);
int sam_db_connect(sam_db_context_t *);
int sam_db_disconnect(sam_db_context_t *);
void sam_db_context_free(sam_db_context_t *);

/* utility functions */
char *sam_db_get_sql(int sql_id);
MYSQL_STMT *sam_db_get_stmt(sam_db_context_t *, int sql_id);
MYSQL_STMT *sam_db_execute_sql(sam_db_context_t *, MYSQL_BIND *bind,
    MYSQL_BIND *result, int sql_id);
int sam_db_id_stat(sam_db_context_t *, unsigned int ino, unsigned int gen,
    struct sam_perm_inode *inode);
int sam_db_id_name(sam_db_context_t *, sam_id_t dir, sam_id_t id,
    int namehash, char *name);
int sam_db_id_allname(sam_db_context_t *, sam_id_t dir, sam_id_t id,
    sam_db_dirent_cb func, void *args);
int sam_db_id_mva(sam_db_context_t *, struct sam_perm_inode *,
    int copy, sam_vsn_section_t **vsns);

/* inode functions */
int sam_db_inode_new(sam_db_context_t *, sam_id_t id, sam_db_inode_t *);
int sam_db_inode_new_perm(sam_perm_inode_t *ip, sam_db_inode_t *);
sam_db_ftype_t sam_db_inode_ftype(struct sam_perm_inode *);
int sam_db_inode_insert(sam_db_context_t *, sam_db_inode_t *);
int sam_db_inode_select(sam_db_context_t *, sam_id_t id,
    sam_db_inode_t *result);
int sam_db_inode_update(sam_db_context_t *, sam_db_inode_t *);
int sam_db_inode_delete(sam_db_context_t *, sam_id_t id);

/* path functions */
int sam_db_path_new(sam_db_context_t *, sam_id_t id, int namehash,
    sam_db_path_t *);
int sam_db_path_insert(sam_db_context_t *, sam_db_path_t *);
int sam_db_path_select(sam_db_context_t *, sam_id_t id, sam_db_path_t *);
int sam_db_path_update(sam_db_context_t *, sam_db_path_t *);
int sam_db_path_update_subdir(sam_db_context_t *con,
    char *oldpath, sam_db_path_t *path);
int sam_db_path_delete(sam_db_context_t *, sam_id_t id);

/* file functions */
int sam_db_file_new(sam_db_context_t *, sam_id_t id, sam_id_t parent_id,
    int namehash, sam_db_file_t *);
int sam_db_file_new_byname(sam_id_t id, sam_id_t parent_id, int namehash,
    char *name, sam_db_file_t *);
int sam_db_file_insert(sam_db_context_t *, sam_db_file_t *);
int sam_db_file_select(sam_db_context_t *, sam_id_t pid, char *name,
    sam_db_file_t *);
int sam_db_file_count_byhash(sam_db_context_t *, sam_id_t pid,
    sam_id_t id, int namehash);
int sam_db_file_update(sam_db_context_t *, sam_id_t old_pid,
    char *oldname, sam_db_file_t *);
int sam_db_file_update_byhash(sam_db_context_t *, sam_id_t old_pid,
    int oldhash, sam_db_file_t *);
int sam_db_file_delete(sam_db_context_t *, sam_id_t pid, char *name);
int sam_db_file_delete_byhash(sam_db_context_t *, sam_id_t pid,
    sam_id_t id, int namehash);
int sam_db_file_delete_bypid(sam_db_context_t *, sam_id_t pid);
int sam_db_file_delete_byid(sam_db_context_t *, sam_id_t id);
int sam_db_file_delete_bypidid(sam_db_context_t *, sam_id_t pid, sam_id_t id);

/* archive functions */
int sam_db_archive_new(sam_db_context_t *, sam_id_t id,
    int copy, sam_db_archive_t **);
void sam_db_archive_free(sam_db_archive_t **);
int sam_db_archive_replace(sam_db_context_t *, sam_db_archive_t *);
int sam_db_archive_stale(sam_db_context_t *, unsigned int ev_time, sam_id_t id);
int sam_db_archive_select(sam_db_context_t *, sam_id_t id,
    int copy, sam_db_archive_t **);
int sam_db_archive_delete(sam_db_context_t *, sam_id_t id, int copy);

#endif /* SAM_DB_H */
