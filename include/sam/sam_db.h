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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */
#if !defined(SAM_DB_H)
#define	SAM_DB_H

#ifndef SAM_DB_NO_MYSQL
#include <mysql.h>
#endif

/* Public data declaration/initialization macros. */
#undef DCL
#undef IVAL
#if defined(DEC_INIT)
#define	DCL
#define	IVAL(v) = v
#else /* defined(DEC_INIT) */
#define	DCL extern
#define	IVAL(v) /* v */
#endif /* defined(DEC_INIT) */

#define	SAMDB_DEFAULT_PORT	3306
#define	SAMDB_CLIENT_FLAG	0

#define	SAMDB_ACCESS_FILE	"/etc/opt/SUNWsamfs/samdb.conf"
#define	SAMDB_SCHEMA_FILE	"/opt/SUNWsamfs/etc/samdb.schema"

#define	SAMDB_INO_MOUNT_POINT	2	/* Ino of mount point directory	*/

/*
 *	When a file system is initialized (samfsdump/restore), a marker
 *	record is created in the sam_inode table.  The generation number
 *	identifies the bases used for the mark.  That is for all inodes
 *	created during the life of this file system are incremented by
 *	the identified bases.
 */

#define	SAMDB_INO_MARK		1	/* Ino for file system mark */
#define	SAMDB_GEN_MARK_BASE	100000	/* Base gen no. for mark */

/* SAM table names */
#define	T_SAM_PATH	"sam_path"
#define	T_SAM_LINK	"sam_link"
#define	T_SAM_INODE	"sam_inode"
#define	T_SAM_ARCHIVE	"sam_archive"
#define	T_SAM_VSNS	"sam_vsns"
#define	T_SAM_MEDIA	"sam_media"

#define	SAMDB_QBUF_LEN	5000		/* Query buffer length		*/

typedef	struct	{			/* SAM DB connect control:	*/
	char		*SAM_host;	/* Hostname			*/
	char		*SAM_user;	/* DB user name			*/
	char		*SAM_pass;	/* DB password			*/
	char		*SAM_name;	/* DB name			*/
	unsigned int	SAM_port;	/* Port number */
	unsigned long	SAM_client_flag; /* Client flag (see mySQL)	*/
}	sam_db_connect_t;

typedef	struct {			/* SAM DB access control:	*/
	char	*db_fsname;		/* Family set name		*/
	char	*db_host;		/* hostname			*/
	char	*db_user;		/* DB user name			*/
	char	*db_pass;		/* DB password			*/
	char	*db_name;		/* DB name			*/
	char	*db_port;		/* Port number			*/
	char	*db_client;		/* Client flag (see mySQL)	*/
	char	*db_mount;		/* Mount point			*/
}	sam_db_access_t;

typedef	enum	{			/* SAM db file types		*/
	FTYPE_REG   = 0,		/* Regular file 		*/
	FTYPE_DIR   = 1,		/* Directory			*/
	FTYPE_SEGI  = 2,		/* Segment file			*/
	FTYPE_SEG   = 3,		/* Segment file			*/
	FTYPE_LINK  = 4,		/* Symbolic Link		*/
	FTYPE_OTHER = 5			/* Something else		*/
}	sam_db_ftype;


/*
 * 	mySQL database schemas.
 *	These structures reflect the database schema.
 */
typedef	struct	{			/* SAM path table:		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
	sam_db_ftype	type;		/* File type			*/
	unsigned char	deleted;	/* Deleted flag			*/
	time_t		delete_time;	/* Time entry was deleted	*/
	char		*path;		/* Path name			*/
	char		*obj;		/* Object name			*/
	char		*initial_path;	/* Initial path name		*/
	char		*initial_obj;	/* Initial object name		*/
	int		flag;		/* Flag register (not in DB)	*/
}	sam_db_path_t;

typedef	struct	{			/* SAM link table:		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
	char		*link;		/* Link string			*/
	int		flag;		/* Flag register (not in DB)	*/
}	sam_db_link_t;

typedef	struct	{			/* SAM inode table:		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
	sam_db_ftype	type;		/* File type			*/
	unsigned char	deleted;	/* Deleted flag			*/
	off64_t		size;		/* File size			*/
	time_t		create_time;	/* File creation time		*/
	time_t		modify_time;	/* File modification time	*/
	time_t		delete_time;	/* File deletion time		*/
	uid_t		uid;		/* User id			*/
	gid_t		gid;		/* Group id			*/
	int		flag;		/* Flag register (not in DB)	*/
}	sam_db_inode_t;

typedef	struct	{			/* SAM archive table:		*/
	unsigned int	ino;		/* Inode number			*/
	unsigned int	gen;		/* Generation number		*/
	unsigned short	copy;		/* Archive copy number		*/
	unsigned short	seq;		/* Multi-VSN sequence number	*/
	unsigned int	vsn_id;		/* VSN identifier (DB index)	*/
	unsigned char	recycled;	/* Entry recycled flag		*/
	unsigned char	stale;		/* Stale entry flag		*/
	off64_t		size;		/* Archive file size		*/
	time_t		create_time;	/* Archive creation time	*/
	time_t		modify_time;	/* File modification time	*/
	time_t		recycle_time;	/* Archive recycled time	*/
	int		flag;		/* Flag register (not in DB)	*/
} sam_db_archive_t;

typedef	struct	{			/* SAM vsn table:		*/
	unsigned int	id;		/* Identifier ordinal		*/
	char		media_type[4];	/* Media type			*/
	char		vsn[34];	/* VSN				*/
	unsigned char	recycled;	/* VSN recycled flag		*/
	unsigned int	files_active;	/* Number of active files	*/
	unsigned int	files_dorment;	/* Number of dorment files	*/
	unsigned int	files_expired;	/* Number of expired files	*/
	unsigned int	files_recycled;	/* Number of recycled files	*/
	off64_t		size_active;	/* Number of active files	*/
	off64_t		size_dorment;	/* Number of dorment files	*/
	off64_t		size_expired;	/* Number of expired files	*/
	off64_t		size_recycled;	/* Number of recycled files	*/
	time_t		expire_time;	/* Expiration time		*/
	time_t		destory_time;	/* Time VSN was destroyed	*/
	short		copy;		/* Restricted copy number	*/
	uid_t		uid;		/* Restricted user id		*/
	gid_t		gid;		/* Restricted group id		*/
	int		flag;		/* Flag register (not in DB)	*/
}	sam_db_vsn_t;

typedef	struct	{			/* SAM media table:		*/
	unsigned int	id;		/* Identifier ordinal		*/
	char		media_type[4];	/* Media type identifier string	*/
	off64_t		size;		/* Capacity size		*/
}	sam_db_media_t;

#ifndef	SAM_DB_NO_MYSQL
DCL MYSQL *SAMDB_conn IVAL(NULL);		/* mySQL context */
DCL char *SAMDB_qbuf IVAL(NULL);		/* Query buffer */
#endif

/* Public flags */
DCL int SAMDB_Debug IVAL(0);
DCL int SAMDB_Verbose IVAL(0);

/* Public samfs file descriptor */
DCL int SAMDB_fd IVAL(0);

/* From libmysqlclient (m_string.h) */
extern char *strend(const char *s);
extern char *strmov(char *, char *);

/* sam_db_util.c items: */
int sam_db_connect(sam_db_connect_t *);
int sam_db_disconnect(void);

/* sam_db_access.c items: */
sam_db_access_t *sam_db_access(char *, char *);
sam_db_access_t *sam_db_access_mp(char *, char *);

/* sam_db_query.c items: */
#ifndef	SAM_DB_NO_MYSQL
my_ulonglong sam_db_query_path(sam_db_path_t **,
    unsigned long, unsigned long);
my_ulonglong sam_db_query_inode(sam_db_inode_t **,
    unsigned long, unsigned long);
my_ulonglong sam_db_query_link(sam_db_link_t **,
    unsigned long, unsigned long);
my_ulonglong sam_db_query_archive(sam_db_archive_t **,
    unsigned long, unsigned long);
my_ulonglong sam_db_row_count(void);
#endif

/*	sam_db_insert.c items: */
int sam_db_insert_inode(sam_db_inode_t *);
int sam_db_insert_path(sam_db_path_t *);
int sam_db_insert_link(sam_db_link_t *);
int sam_db_insert_vsn(char *, char *);
int sam_db_insert_archive(sam_db_archive_t *, time_t);

/*	sam_db_new.c items: */
#ifdef	_SAM_FS_INO_H
sam_db_inode_t *sam_db_new_inode(struct sam_perm_inode *);
sam_db_path_t *sam_db_new_path(struct sam_perm_inode *,
    char *, char *);
sam_db_link_t *sam_db_new_link(struct sam_perm_inode *, char *);
sam_db_ftype sam_db_get_ftype(struct sam_perm_inode *);
#endif

/*	sam_db_rename.c items: */
#ifndef	SAM_DB_NO_MYSQL
my_ulonglong sam_db_rename(char *, char *,
    char *, char *);
#endif

/*	sam_db_restore.c items: */
#ifndef	SAM_DB_NO_MYSQL
my_ulonglong sam_db_restore(unsigned int, unsigned int,
    sam_db_ftype, char *, char *);
my_ulonglong sam_db_query_by_path(sam_db_path_t **, char *, char *);
my_ulonglong sam_db_restore_table(char *, unsigned int,
    unsigned int, unsigned int, unsigned int);
#endif

/*	sam_db_query_error.c items: */
void sam_db_query_error(void);

/* VSN Cache: */
typedef	struct	{
	int	vsn_id;			/* VSN ordinal */
	char	media[4];		/* Media type */
	char	vsn[40];		/* VSN */
}	vsn_cache_t;

#define	MAX_LEVELS	4		/* Maximum no. archive copies	*/
#define	L_VSN_CACHE	2000		/* Initial VSN cache size */
#define	L_VSN_CACHE_INC	1000		/* VSN cache increment size */

DCL vsn_cache_t *VSN_Cache IVAL(NULL);	/* VSN cache table */
DCL int n_vsns IVAL(0);			/* Number of VSNs in cache */

void sam_db_init_vsn_cache(void);
int sam_db_find_vsn(int, char *, char *);
int sam_db_cache_vsn(int, char *, char *);
int sam_db_load_vsns(void);

#endif /* SAM_DB_H */
