/*
 * event_handler.c - handlers that update database for filesystem events
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

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include <sys/vfs.h>

#include <sam/samevent.h>
#include <sam/sam_trace.h>
#include <sam/sam_db.h>
#include <sam/fs/sblk.h>

#include "event_handler.h"

#define	EV_RENAME_NEWPID  0
#define	EV_RENAME_OLDPID  1

static int file_path_consistency(sam_id_t pid, sam_dirent_t *dirent, void *);
static int dirent_consistency(sam_id_t pid, sam_dirent_t *dirent, void *);
static int ignore_handler(sam_db_context_t *, sam_event_t *);
static int create_handler(sam_db_context_t *, sam_event_t *);
static int attr_handler(sam_db_context_t *, sam_event_t *);
static int rename_handler(sam_db_context_t *, sam_event_t *);
static int remove_handler(sam_db_context_t *, sam_event_t *);
static int archive_handler(sam_db_context_t *, sam_event_t *);
static int modify_handler(sam_db_context_t *, sam_event_t *);

/* Passthrough argument for file_path_consistency */
static struct file_path_arg {
	sam_db_context_t *con;
	sam_db_inode_t *inode;
};

/* Map event numbers to event handlers, must match samevent.h */
static event_handler_t ev_handler[ev_max] = {
	ignore_handler,		/* ev_none */
	create_handler,		/* ev_create */
	attr_handler,		/* ev_change */
	attr_handler,		/* ev_close */
	rename_handler,		/* ev_rename */
	remove_handler,		/* ev_remove */
	attr_handler,		/* ev_offline */
	attr_handler,		/* ev_online */
	archive_handler,	/* ev_archive */
	modify_handler,		/* ev_modify */
	archive_handler,	/* ev_archange */
	ignore_handler,		/* ev_restore */
	ignore_handler		/* ev_umount */
};

static char *ev_names[ev_max] = {
	"ev_none",
	"ev_create",
	"ev_change",
	"ev_close",
	"ev_rename",
	"ev_remove",
	"ev_offline",
	"ev_online",
	"ev_archive",
	"ev_modify",
	"ev_archange",
	"ev_restore",
	"ev_umount"
};

/*
 * Get the event handler function for the specified event number.
 *
 * Returns the corresponding event_handler_t function, or null if not valid.
 */
event_handler_t
get_event_handler(int ev_num)
{
	if (ev_num < ev_none || ev_num >= ev_max) {
		return (NULL);
	}

	return (ev_handler[ev_num]);
}

/*
 * Get the event name corrensponding to the given event number.
 *
 * Returns the name, or the string "Unknown" if not valid.
 */
char *
get_event_name(int ev_num) {
	return (ev_num >= 0 && ev_num < ev_max ? ev_names[ev_num] : "Unknown");
}

/*
 * Make the database consistent with the filesystem for the
 * information contained in the provided event.  This method is
 * intended to be called after an error has been detected.
 *
 * repair_dir - whether to run namespace repair for directory.  This
 * 	operation can be expensive, and if not needed inefficient.
 *
 * Algorithm description:
 * - Check if the inode from the event still exists.  If not, delete
 *   all its data from database.
 * - If inode exists then we need to update the database with the most
 *   current information.  We update all of the database tables with
 *   the appropriate information for the type of inode.
 * - We also make sure the parent directory links are correct.  If the
 *   parent directory described in the event doesn't exist we use the
 *   parent id from the inode.
 * - If the inode is a directory then we update all of the entries in
 *   the file table for the directory.
 */
int
check_consistency(
	sam_db_context_t *con,
	sam_event_t *event,
	boolean_t repair_dir)
{
	sam_perm_inode_t perm;
	sam_perm_inode_t pid_perm;
	sam_db_inode_t inode;
	sam_db_path_t path;
	sam_db_archive_t *archive;
	struct file_path_arg fpc_arg;
	sam_id_t all_ids = {0, -1};
	int ret;
	int copy;

	ret = sam_db_id_stat(con, event->ev_id.ino, event->ev_id.gen, &perm);

	/*
	 * If ino is not supposed to be in database or
	 * idstat fails then inode no longer exists, remove all data
	 */
	if (ret < 0 || !IS_DB_INODE(event->ev_id.ino) ||
	    !IS_DB_INODE(perm.di.parent_id.ino)) {
		if (sam_db_inode_delete(con, event->ev_id) < 0) {
			return (-1);
		}
		if (sam_db_file_delete_byid(con, event->ev_id) < 0) {
			return (-1);
		}
		/* Delete by file pid in case inode was a directory */
		if (sam_db_file_delete_bypid(con, event->ev_id) < 0) {
			return (-1);
		}
		if (sam_db_path_delete(con, event->ev_id) < 0) {
			return (-1);
		}
		if (sam_db_archive_delete(con, event->ev_id, -1) < 0) {
			return (-1);
		}
	} else {
		/* We have valid inode, update database accordingly */
		if (sam_db_inode_new(con, event->ev_id, &inode) < 0) {
			return (-1);
		}
		ret = sam_db_inode_update(con, &inode);
		if (ret < 0) {
			return (-1);
		} else if (ret == 0) {
			if (sam_db_inode_insert(con, &inode) < 0) {
				return (-1);
			}
		}

		/* Replace all archive information */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if (sam_db_archive_delete(con,
			    event->ev_id, copy) < 0) {
				return (-1);
			}
			if (perm.di.arch_status & (1<<copy)) {
				int nvsn, i;
				if ((nvsn = sam_db_archive_new(con,
				    event->ev_id, copy, &archive)) < 0) {
					return (-1);
				}
				for (i = 0; i < nvsn; i++) {
					if (sam_db_archive_replace(con,
					    &archive[i]) < 0) {
						sam_db_archive_free(&archive);
						return (-1);
					}
				}
				sam_db_archive_free(&archive);
			}
		}

		/* Check path consistency */
		if (event->ev_id.ino != SAM_ROOT_INO) {
			/* Check if the event parent still exists */
			if (sam_db_id_stat(con, event->ev_pid.ino,
			    event->ev_pid.gen, &pid_perm) < 0) {
				/* Doesn't exist, delete parent file info */
				if (sam_db_file_delete_bypid(con,
				    event->ev_pid) < 0) {
					return (-1);
				}
				if (sam_db_path_delete(con,
				    event->ev_pid) < 0) {
					return (-1);
				}
				/* Perform recovery using inode's parent_id */
				if (sam_db_id_stat(con, perm.di.parent_id.ino,
				    perm.di.parent_id.gen, &pid_perm) < 0) {
					return (-1);
				}
			}

			/* Update parent path information */
			if (sam_db_path_new(con, pid_perm.di.id,
			    -1, &path) < 0) {
				return (-1);
			}
			ret = sam_db_path_update(con, &path);
			if (ret < 0) {
				return (-1);
			} else if (ret == 0) {
				if (sam_db_path_insert(con, &path) < 0) {
					return (-1);
				}
			}

			/*
			 * Delete all hardlinks to the inode in the directory
			 * to catch links that no longer exist.  Any existing
			 * links will be recreated in the next step.
			 */
			if (sam_db_file_delete_bypidid(con, pid_perm.di.id,
			    event->ev_id) < 0) {
				return (-1);
			}

			/*
			 * Update file/path information for all names matching
			 * id in parent.  Uses callback function
			 * file_path_consistency.
			 */
			fpc_arg.con = con;
			fpc_arg.inode = &inode;
			if (sam_db_id_allname(con, pid_perm.di.id,
			    event->ev_id, file_path_consistency,
			    &fpc_arg) < 0) {
				return (-1);
			}
		}

		/* If a directory update its path and children file entries */
		if (repair_dir && inode.type == FTYPE_DIR) {
			if (sam_db_path_new(con, event->ev_id,
			    -1, &path) < 0) {
				return (-1);
			}
			ret = sam_db_path_update(con, &path);
			if (ret < 0) {
				return (-1);
			} else if (ret == 0) {
				if (sam_db_path_insert(con, &path) < 0) {
					return (-1);
				}
			}
			if (sam_db_file_delete_bypid(con, event->ev_id) < 0) {
				return (-1);
			}
			if (sam_db_id_allname(con, event->ev_id, all_ids,
			    dirent_consistency, con) < 0) {
				return (-1);
			}
		}
	}

	return (0);
}

/*
 * Ensure that the file and path information is valid in the database.
 * The provided argument is a pointer to a file_path_arg struct.
 *
 * After completion the database will have the correct path and file
 * information for the given directory entry.
 *
 * Return 0 on success, -1 on failure.
 */
static int
file_path_consistency(sam_id_t pid, sam_dirent_t *dirent, void *arg) {
	sam_db_context_t *con;
	sam_db_inode_t *inode;
	sam_db_file_t file;
	sam_db_path_t path;
	sam_db_path_t pidpath;
	int ret;

	con = ((struct file_path_arg *)arg)->con;
	inode = ((struct file_path_arg *)arg)->inode;

	/* Non-zero ino means update path table. */
	path.ino = 0;

	/*
	 * Update file information.  We know that we can insert since the
	 * previous step in consistency check deleted links to the file.
	 */
	if (sam_db_file_new_byname(dirent->d_id, pid, dirent->d_namehash,
	    (char *)dirent->d_name, &file) < 0) {
		return (-1);
	}
	if (sam_db_file_insert(con, &file) < 0) {
		return (-1);
	}

	/*
	 * Update path if directory or symbolic link.  We know that the path for
	 * the parent is correct in the database because of the previous
	 * step in the consistency check.  We use this to build the path.
	 */
	if (inode->type == FTYPE_DIR && inode->ino != SAM_ROOT_INO) {
		if (sam_db_path_select(con, pid, &pidpath) != 0) {
			return (-1);
		}
		path.ino = dirent->d_id.ino;
		path.gen = dirent->d_id.gen;
		ret = strlcpy(path.path, pidpath.path, MAXPATHLEN+1);
		if (ret + dirent->d_namlen > MAXPATHLEN) {
			return (-1);
		}
		strncat(path.path, (char *)dirent->d_name, dirent->d_namlen);
	} else if (inode->type == FTYPE_LINK) {
		if (sam_db_path_new(con, dirent->d_id,
		    dirent->d_namehash, &path) < 0) {
			return (-1);
		}
	}

	/* If path entry, update path table */
	if (path.ino != 0) {
		ret = sam_db_path_update(con, &path);
		if (ret < 0) {
			return (-1);
		} else if (ret == 0) {
			if (sam_db_path_insert(con, &path) < 0) {
				return (-1);
			}
		}
	}

	return (0);
}

/*
 * Ensure that the file information is valid for the provided
 * directory entry.  This is a callback function for sam_db_id_allname
 * used by check_consistency to update directory entries.
 *
 * Return 0 on success, -1 on failure.
 */
static int
dirent_consistency(sam_id_t pid, sam_dirent_t *dirent, void *arg) {
	sam_db_context_t *con;
	sam_db_file_t file;

	con = (sam_db_context_t *)arg;

	/* Ignore . and .. */
	if (dirent->d_name[0] == '.' && (dirent->d_namlen == 1 ||
	    (dirent->d_namlen == 2 && dirent->d_name[1] == '.'))) {
		return (0);
	}

	/* Ignore non database inodes */
	if (!IS_DB_INODE(dirent->d_id.ino)) {
		return (0);
	}

	if (sam_db_file_new_byname(dirent->d_id, pid, dirent->d_namehash,
	    (char *)dirent->d_name, &file) < 0) {
		return (-1);
	}

	if (sam_db_file_insert(con, &file) < 0) {
		return (-1);
	}

	return (0);
}

/* Ignore the provided event, always returns 0 */
static int
/*LINTED argument unused in function */
ignore_handler(sam_db_context_t *con, sam_event_t *event) {
	return (0);
}

/*
 * Handle ev_create events.
 *
 * Event parameter 1: number of hard links
 * Event parameter 2: name hash code for directory entry
 *
 * Inode, file and path information will be populated for the
 * file specified in the event.
 */
static int
create_handler(sam_db_context_t *con, sam_event_t *event) {
	sam_db_inode_t db_inode;
	sam_db_path_t db_path;
	sam_db_file_t db_file;
	sam_event_t pid_event = *event;
	ushort_t nlink = event->ev_param;
	ushort_t namehash = event->ev_param2;

	if (sam_db_inode_new(con, event->ev_id, &db_inode) < 0) {
		return (-1);
	}

	/* Inserts for new inodes, directories do not have hard links */
	if (nlink == 1 || db_inode.type == FTYPE_DIR) {
		if (sam_db_inode_insert(con, &db_inode) < 0) {
			return (-1);
		}

		if (db_inode.type == FTYPE_DIR ||
		    db_inode.type == FTYPE_LINK) {
			if (sam_db_path_new(con, event->ev_id,
			    namehash, &db_path) < 0) {
				return (-1);
			}

			if (sam_db_path_insert(con, &db_path)) {
				return (-1);
			}
		}
	}

	/* Insert file information */
	if (sam_db_file_new(con, event->ev_id, event->ev_pid,
	    namehash, &db_file) < 0) {
		return (-1);
	}
	if (sam_db_file_insert(con, &db_file) < 0) {
		return (-1);
	}

	/* Parent attributes may have been updated */
	pid_event.ev_id = event->ev_pid;
	return (attr_handler(con, &pid_event));
}

/*
 * Update the attributes for the inode specified in event.
 */
static int
attr_handler(sam_db_context_t *con, sam_event_t *event) {
	sam_db_inode_t db_inode;

	if (sam_db_inode_new(con, event->ev_id, &db_inode) < 0) {
		return (-1);
	}
	if (sam_db_inode_update(con, &db_inode) < 0) {
		return (-1);
	}

	return (0);
}

/*
 * Handle ev_rename
 * Parameter 1: new (EV_RENAME_NEWPID) or old (EV_RENAME_OLDPID) parent
 * Parameter 2: name hashcode (new or old)
 */
static int
rename_handler(sam_db_context_t *con, sam_event_t *event) {
	/* Since we receive two events, we need to remember the first. */
	static sam_event_t newpid_event;
	sam_perm_inode_t perm;
	sam_db_file_t file;
	sam_db_path_t path;
	sam_db_ftype_t ftype;
	int dir_nlink;

	/* Handle multiple event ordering */
	if (event->ev_param == EV_RENAME_NEWPID) {
		if (newpid_event.ev_num == ev_rename) {
			Trace(TR_ERR, "Received two newpid "
			    "events consecutively.");
			check_consistency(con, &newpid_event, TRUE);
		}
		memcpy(&newpid_event, event, sizeof (sam_event_t));
		return (0);
	} else if (event->ev_param == EV_RENAME_OLDPID) {
		if (newpid_event.ev_num != ev_rename) {
			Trace(TR_ERR, "Received rename oldpid before newpid.");
			return (-1);
		}
		if (newpid_event.ev_id.ino != event->ev_id.ino) {
			Trace(TR_ERR, "Interleaving of rename events.");
			goto error;
		}
	} else {
		Trace(TR_ERR, "Received invalid rename event parameter.");
		return (-1);
	}

	if (sam_db_id_stat(con, newpid_event.ev_id.ino,
	    newpid_event.ev_id.gen, &perm) < 0) {
		Trace(TR_ERR, "Failed to stat inode %d.%d",
		    newpid_event.ev_id.ino, newpid_event.ev_id.gen);
		return (-1);
	}

	ftype = sam_db_inode_ftype(&perm);

	/*
	 * Determine the number of indistinguishable hard-links
	 * in the "old" directory.  Hard-links are indistinguishable
	 * if the links are in the same directory and has the same
	 * name hashcode.
	 */
	if (perm.di.nlink == 1 || ftype == FTYPE_DIR) {
		dir_nlink = 1;
	} else {
		dir_nlink = sam_db_file_count_byhash(con, event->ev_pid,
		    event->ev_id, event->ev_param2);
	}

	if (dir_nlink == 1) {
		int count;
		if (sam_db_file_new(con, newpid_event.ev_id,
		    newpid_event.ev_pid, newpid_event.ev_param2, &file) < 0) {
			goto error;
		}

		count = sam_db_file_update_byhash(con, event->ev_pid,
		    event->ev_param2, &file);
		if (count != 1) {
			Trace(TR_ERR, "Unexpected update count %d for rename"
			    " %d.%d using %d.%d %u", count, file.ino, file.gen,
			    event->ev_pid.ino, event->ev_pid.gen,
			    event->ev_param2);
			goto error;
		}

		if (ftype == FTYPE_DIR || ftype == FTYPE_LINK) {
			sam_db_path_t oldpath;
			if (sam_db_path_select(con,
			    event->ev_id, &oldpath) < 0) {
				goto error;
			}

			if (sam_db_path_new(con, newpid_event.ev_id,
			    newpid_event.ev_param2, &path) < 0) {
				goto error;
			}
			if (sam_db_path_update(con, &path) != 1) {
				goto error;
			}

			/* Update prefixes of subdir paths */
			if (sam_db_path_update_subdir(con,
			    oldpath.path, &path) < 0) {
				goto error;
			}
		}
	} else {
		/*
		 * We let the consistency check take care of the rare case
		 * where at least two hard links in the same directory to
		 * the same file have identical name hashes.
		 */
		goto error;
	}

	/* Parent attributes may have changed, update them */
	if (newpid_event.ev_pid.ino != event->ev_pid.ino) {
		newpid_event.ev_id = newpid_event.ev_pid;
		(void) attr_handler(con, &newpid_event);
	}
	newpid_event.ev_id = event->ev_pid;
	(void) attr_handler(con, &newpid_event);

	newpid_event.ev_num = 0;
	return (0);

error:
	/*
	 * We run one consistency check here for the first event and the other
	 * will be ran by the caller.
	 */
	(void) check_consistency(con, &newpid_event, TRUE);
	newpid_event.ev_num = 0;
	return (-1);
}

/*
 * Handle ev_remove events.
 * Parameter 1: Number of remaining hard links
 * Parameter 2: Name hashcode for directory entry
 */
static int
remove_handler(sam_db_context_t *con, sam_event_t *event) {
	ushort_t nlink = event->ev_param;
	ushort_t namehash = event->ev_param2;
	sam_event_t pid_event = *event;

	if (sam_db_file_delete_byhash(con, event->ev_pid,
	    event->ev_id, namehash) != 1) {
		return (-1);
	}

	if (nlink == 0) {
		if (sam_db_archive_delete(con, event->ev_id, -1) < 0) {
			return (-1);
		}

		if (sam_db_path_delete(con, event->ev_id) < 0) {
			return (-1);
		}

		if (sam_db_inode_delete(con, event->ev_id) != 1) {
			return (-1);
		}
	}

	/* The attributes of the parent may have changed, handle it */
	pid_event.ev_id = event->ev_pid;
	(void) attr_handler(con, &pid_event);

	return (0);
}

/*
 * Handle ev_archive events.  Enter archive copy information
 * into database and update inode attributes.
 *
 * Parameter 1: copy number
 */
static int
archive_handler(sam_db_context_t *con, sam_event_t *event) {
	sam_db_archive_t *archive = NULL;
	int nvsn;
	int i;
	int copy = event->ev_param;

	nvsn = sam_db_archive_new(con, event->ev_id, copy, &archive);

	if (nvsn > 0) {
		for (i = 0; i < nvsn; i++) {
			if (sam_db_archive_replace(con,
			    &archive[i]) < 0) {
				nvsn = -1;
				break;
			}
		}
	}

	sam_db_archive_free(&archive);
	return (nvsn > 0 ? attr_handler(con, event) : nvsn);
}

/*
 * Handle ev_modify events.  Mark archive copies stale and update
 * inode attributes.
 */
static int
modify_handler(sam_db_context_t *con, sam_event_t *event) {
	if (sam_db_archive_stale(con, event->ev_time, event->ev_id)) {
		return (-1);
	}

	/* Update attributes for parent */
	if (event->ev_pid.ino > 0) {
		sam_event_t pid_event = *event;
		pid_event.ev_id = event->ev_pid;
		(void) attr_handler(con, &pid_event);
	}

	return (attr_handler(con, event));
}
