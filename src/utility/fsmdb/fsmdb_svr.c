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
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <errno.h>
#include <door.h>
#include <pthread.h>
#include <signal.h>
#include <dirent.h>
#include <string.h>
#include <rpc/xdr.h>
#include <stdarg.h>
#include "mgmt/fsmdb_int.h"
#include "mgmt/fsmdb.h"
#include "mgmt/file_metrics.h"
#include "mgmt/file_details.h"

/*
 *  Main file system database function.
 *
 *  Uses the "door" interface for inter-process communication.
 *
 *  Horribly, the doors interface doesn't provide a way to do
 *  post-results processing (i.e., closing file descriptors,
 *  freeing memory needed to hold results, etc.).  As such, this
 *  daemon will require that callers pass in a file descriptor
 *  for those functions that return significant results.
 *
 *  From the server's perspective, we'll just use the provided
 *  fd to write data to be consumed by the client.  Before calling
 *  door_return to signal the client that we're done processing, we
 *  will close our copy of the file descriptor.
 *
 *  The client must ensure that the originally-passed file descriptor
 *  and any temporary files get properly cleaned up.  This could be
 *  done by opening a temporary file, then unlink() it before calling
 *  the server.  That way if either (or both) the client or server
 *  misbehave, we don't leave temporary files lying around.
 *
 *  If pipes are used, it is up to the caller to ensure that SIGPIPE
 *  is correctly handled.
 *
 */

/*  Function declarations */
static void *handle_signal(void *arg);
static void fsmsvr(void *cookie, char *argp, size_t arg_size, door_desc_t *dp,
	uint_t n_desc);
static int write_buf(int fd, void *buffer, int len);
static void fsm_free_file_details(filedetails_t *details);

/* thread to clean up any aborted deletes */
static void *finish_partial_deletes(void *arg);
static void *do_checkpoint(void *arg);
extern int samcftime(char *s, const char *format, const time_t *clock);

/*  Globals */
boolean_t		do_daemon = TRUE;
pthread_attr_t		pattr;
pthread_mutex_t		glock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		quitcond = PTHREAD_COND_INITIALIZER;
boolean_t		stopserver = FALSE;

/* mutex and condition for checkpoint thread */
pthread_mutex_t		ckptmutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t		ckptcond = PTHREAD_COND_INITIALIZER;
int			active = 0;

/* list of known file systems */
fs_entry_t		*fs_entry_list = NULL;
pthread_rwlock_t	fslistlock = PTHREAD_RWLOCK_INITIALIZER;

/* error log file */
static char		*fsmdb_errLog = "/var/opt/SUNWsamfs/fsmdb.log";
FILE			*fsmdb_errFilep = stderr;
pid_t			fsmdb_pid = -1;
static char		*timefmt = "%e %b %Y %T %Z";

/*
 *  This database server process typically runs as an independent
 *  daemon.  For debugging purposes, use the "-d" option to run
 *  the server in the foreground.  "-d" is only available if the
 *  server has been compiled with -DDEBUG.
 *
 */
int
main(int argc, char *argv[])
{
	int		st;
	char		c;
	pid_t		pid;
	int		nullfd;
	sigset_t	mask;
	int		doorfd = -1;
	int		lockfd = -1;
	flock64_t	flk;
	pthread_t	tid;
	int		logfd = -1;
	char		*errpfx = "main";

	while ((c = getopt(argc, argv, "d")) != -1) {
		switch (c) {
#ifdef DEBUG
			case 'd':
				do_daemon = FALSE;
				break;
#endif /* DEBUG */
			default:
				/* ignore invalid args */
				break;
		}
	}

	/* make sure we didn't inherit a weird creation mask */
	(void) umask(0);

	/* close any inherited file descriptors */
	closefrom(STDERR_FILENO + 1);

	nullfd = open("/dev/null", O_RDWR);

	/* and disassociate from our parent */
	if (do_daemon) {
		pid = fork();
		if (pid < 0) {
			(void) printf("Cannot fork process, exiting\n");
			exit(1);
		} else if (pid > 0) {
			/* parent exits now */
			exit(0);
		}
		/* become session leader */
		(void) setsid();

		/* set out working directory to something rational */
		/* Should this be /var/opt/SUNWsamfs/cores?? */
		(void) chdir("/");
	}

	/* block most signals.  We only care about the die now ones */
	(void) sigfillset(&mask);

	/*
	 * if we're in debug mode, most likely in the debugger so
	 * allow SIGINT
	 */
	if (!do_daemon) {
		(void) sigdelset(&mask, SIGINT);
	}

	(void) pthread_sigmask(SIG_BLOCK, &mask, NULL);

	if (do_daemon) {
		/*
		 * One last fork to make sure we're really really
		 * not going to inherit a controlling terminal...
		 */
		pid = fork();
		if (pid != 0) {
			exit(0);
		};

		/* we're not using stdin/out/err */
		dup2(nullfd, STDIN_FILENO);
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
	} else {
		/* assign stderr to stdout */
		dup2(STDOUT_FILENO, STDERR_FILENO);
	}

	/* initialize log - defaults to stderr if log can't be opened */
	fsmdb_pid = getpid();
	logfd = open64(fsmdb_errLog, O_RDWR|O_CREAT, 0744);
	if (logfd != -1) {
		fsmdb_errFilep = fdopen(logfd, "a+");
	}

	LOGERR("fsmdb starting");

	/* all threads we create should be detached */
	(void) pthread_attr_init(&pattr);
	(void) pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_DETACHED);

	/* Set up a signal handling thread */
	(void) pthread_create(&tid, &pattr, handle_signal, NULL);

	/*
	 * Setup the operating environment
	 */
	st = mkdirp(fsmdbdir, 0655);
	if ((st != 0) && (errno != EEXIST)) {
		LOGERR("Could not open database dir %s", fsmdbdir);
		return (st);
	}

	/*
	 * Make sure the door directory exists
	 */
	st = mkdirp(fsmdbdoordir, 0655);
	if ((st != 0) && (errno != EEXIST)) {
		LOGERR("Could not open door dir %s", fsmdbdoordir);
		return (st);
	}

	/* lock so multiple db processes don't start */
	lockfd = open(fsmdbdoorlock, O_WRONLY|O_CREAT, 0655);
	if (lockfd == -1) {
		LOGERR("Could not create lock file %s", fsmdbdoorlock);
		return (errno);
	}

	memset(&flk, 0, sizeof (flock64_t));
	flk.l_type = F_WRLCK;
	flk.l_whence = SEEK_SET;

	st = fcntl(lockfd, F_SETLK64, &flk);
	if (st == -1) {
		if (errno == EAGAIN) {
			/* already locked */
			LOGERR("fsmdb process already running");
			st = 0;
		}
		goto done;
	}

	/*
	 * lock the fslistlock to prevent callers from proceeding until
	 * the environment is open
	 */
	(void) pthread_rwlock_wrlock(&fslistlock);

	/* open the doors! */
	doorfd = door_create(fsmsvr, NULL, 0);
	if (doorfd == -1) {
		(void) pthread_rwlock_unlock(&fslistlock);
		st = -1;
		goto done;
	}

	/*
	 * recreate the door itself.  If a previous process exited
	 * abnormally (core dump, whatever), the door won't be revoked
	 * and we won't be able to start a new process.  Yet another
	 * weird door-ism.  The locking above should prevent a door
	 * from being removed out from under a running process.
	 */
	unlink(fsmdbdoor);
	st = mknod(fsmdbdoor, 0655, 0);
	if (st == -1) {
		st = errno;
		(void) pthread_rwlock_unlock(&fslistlock);
		LOGERR("Could not create door.");
		goto done;
	}

	st = fattach(doorfd, fsmdbdoor);
	if (st == -1) {
		st = errno;
		if (st == EBUSY) {
			/* shouldn't happen - another process got here first */
			st = 0;
		} else {
			LOGERR("Could not attach to door %d", st);
		}
		(void) pthread_rwlock_unlock(&fslistlock);
		goto done;
	}

	/* get the database stuff running */
	st = open_db_env(fsmdbdir);
	if (st != 0) {
		LOGERR("Could not open the DB environment %d", st);
		(void) pthread_rwlock_unlock(&fslistlock);
		goto done;
	}

	st = open_vsn_db();
	if (st != 0) {
		LOGERR("Could not open the VSN DB %d", st);
		(void) pthread_rwlock_unlock(&fslistlock);
		goto done;
	}

	/* start the checkpoint thread */
	pthread_create(&tid, &pattr, do_checkpoint, NULL);

	/* ready to work now */
	(void) pthread_rwlock_unlock(&fslistlock);

	/* clean up any deletes that weren't finished when we exited */
	(void) pthread_create(&tid, &pattr, finish_partial_deletes, NULL);

	/* the fsmsvr function now does all the work.  Sit and wait to exit */
	(void) pthread_mutex_lock(&glock);
	while (!stopserver) {
		(void) pthread_cond_wait(&quitcond, &glock);
	}
	(void) pthread_mutex_unlock(&glock);

done:
	/* TODO:  wait for all tasks to end */
	LOGERR("fsmdb exiting with status %d", st);

	/* don't let any more calls in */
	door_revoke(doorfd);

	/* clean up the database stuff */
	destroy_fs_list(NULL);
	close_vsn_db();
	close_env();

	if (lockfd != -1) {
		close(lockfd);
	}

	/* all done */
	return (st);
}

/* exit cleanly if we're told to stop */
static void *
handle_signal(void *arg)	/* ARGSUSED */
{
	int		count;
	int		st = 0;
#ifndef	__lint
	int		signum;
#endif	/* __lint */
	sigset_t	mask;
	char		*errpfx = "handle_signal";

	(void) sigemptyset(&mask);
	(void) sigaddset(&mask, SIGINT);
	(void) sigaddset(&mask, SIGQUIT);
	(void) sigaddset(&mask, SIGTERM);

	(void) pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

	/*
	 * wait forever, or until sigwait fails 10 times.  sigwait()
	 * shouldn't fail, but we don't want to be looping frantically
	 * if it does.
	 */
	for (count = 0; count < 10; count++) {
		/*
		 * for reasons I don't understand, lint is unhappy with
		 * the sigwait() function declaration in signal.h.
		 */
#ifndef __lint
		st = sigwait(&mask, &signum);
#endif
		if (st == 0) {
			break;
		}
	}

	if (st == 0) {
		/* we've been asked to exit */
		(void) pthread_mutex_lock(&glock);
		stopserver = TRUE;
		(void) pthread_mutex_unlock(&glock);
		(void) pthread_cond_broadcast(&quitcond);
	}

	return (NULL);
}

/* main dispatch function */
static void
fsmsvr(
	void		*cookie,	/* ARGSUSED */
	char		*argp,
	size_t		arg_size,	/* ARGSUSED */
	door_desc_t	*dp,
	uint_t		n_desc)
{

/* LINTED ["pointer cast may result in improper alignment"] */
	fsmdb_door_arg_t	*inargs = (fsmdb_door_arg_t *)argp;
	int			st;
	int			i;
	int			fd = -1;
	fsmdb_ret_t		ret;
	FILE			*outfile = NULL;
	fs_entry_t		*fsent = NULL;
	char			*buf;
	size_t			buflen;
	uint32_t		num;
	sqm_lst_t			*lstp;
	flist_arg_t 		l;
	XDR			xdrs;
	delete_snap_t		*delargs = NULL;
	pthread_t		tid;
	char			logbuf[MAXPATHLEN * 2];
	char			*errpfx = "fsmsvr";

	if (inargs == NULL) {
		st = EINVAL;
		LOGERR("No arguments received");
		goto done;
	}

	if ((n_desc > 0) && (dp != NULL) &&
	    (dp->d_attributes == DOOR_DESCRIPTOR)) {
		fd = dp->d_data.d_desc.d_descriptor;
	}

	if (fd != -1) {
		outfile = fdopen(fd, "w");
	}

	switch (inargs->task) {
		case IMPORT_SAMFS:
			if ((inargs->u.i.fsname[0] == '\0') ||
			    (inargs->u.i.snapshot[0] == '\0')) {
				LOGERR(
				    "missing filesystem name or recovery "
				    "point name");
				st = EINVAL;
				break;
			}
			LOGERR(
			    "Importing %s for filesystem %s",
			    inargs->u.i.snapshot,
			    inargs->u.i.fsname);

			st = get_fs_entry(inargs->u.i.fsname, TRUE, &fsent);

			LOGERR(
			    "Finished indexing %s, status = %d",
			    inargs->u.i.snapshot,
			    st);

			if (st != 0) {
				break;
			}
			st = db_process_samfsdump(fsent, inargs->u.i.snapshot);
			done_with_db_task(fsent);

			break;
		case IMPORT_LIVEFS:
			if ((inargs->u.i.fsname[0] == '\0') ||
			    (inargs->u.i.snapshot[0] == '\0')) {
				st = EINVAL;
				LOGERR(
				    "missing filesystem name or metrics name");
				break;
			}

			LOGERR("Gathering metrics for %s", inargs->u.i.fsname);

			st = get_fs_entry(inargs->u.i.fsname, TRUE, &fsent);

			if (st != 0) {
				LOGERR("Internal error %d", st);
				break;
			}

			/*
			 * make sure we checkpoint here too.  Gathering metrics
			 * uses in-memory databases, which end up generating
			 * transaction logs.
			 */
			incr_active();

			st = walk_live_fs(fsent, inargs->u.i.snapshot);

			decr_active();

			done_with_db_task(fsent);
			LOGERR(
			    "Finished gathering metrics for %s, status = %d",
			    inargs->u.i.fsname, st);

			break;

		case CHECK_VSN:
			if (inargs->u.c[0] == '\0') {
				LOGERR("No VSN specified");
				st = EINVAL;
				break;
			}

			st = db_check_vsn_inuse(inargs->u.c);

			LOGERR(
			    "Check VSN %s complete, status = %d",
			    inargs->u.c, st);

			break;
		case GET_ALL_VSN:
			if (outfile == NULL) {
				st = EBADF;
				break;
			}

			LOGERR("Listing VSNs");

			st = db_list_all_vsns(&i, &buf);
			if (st != 0) {
				LOGERR("List VSNs failed %d", st);
				break;
			}

			if (buf != NULL) {
				(void) fprintf(outfile, "%s", buf);
				(void) fflush(outfile);
				ret.count = i;
				free(buf);
			}
			break;
		case GET_SNAPSHOT_VSN:
			if ((inargs->u.i.fsname[0] == '\0') ||
			    (inargs->u.i.snapshot[0] == '\0')) {
				st = EINVAL;
				LOGERR(
				    "missing filesystem name or recovery "
				    "point name");
				break;
			}
			if (outfile == NULL) {
				st = EBADF;
				break;
			}

			LOGERR(
			    "Getting VSNs for filesystem %s recovery point %s",
			    inargs->u.i.fsname, inargs->u.i.snapshot);

			st = get_fs_entry(inargs->u.i.fsname, FALSE, &fsent);
			if (st == 0) {
				st = db_get_snapshot_vsns(fsent,
				    inargs->u.i.snapshot, &num, &buf);
				done_with_db_task(fsent);
			}

			LOGERR("Getting VSNs in recovery point %s complete %d",
			    inargs->u.i.snapshot, st);

			if (st != 0) {
				break;
			}
			if (buf != NULL) {
				(void) fprintf(outfile, "%s", buf);
				(void) fflush(outfile);
				ret.count = num;
				free(buf);
			}

			break;
		case GET_FS_METRICS:
			if (inargs->u.m.fsname[0] == '\0') {
				LOGERR("No filesystem specified");
				st = EINVAL;
				break;
			}
			if (outfile == NULL) {
				/* invalid fd provided */
				st = EBADF;
				break;
			}
			LOGERR(
			    "Get Metrics for filesystem %s",
			    inargs->u.m.fsname);

			st = get_fs_entry(inargs->u.m.fsname, FALSE, &fsent);
			if (st != 0) {
				break;
			}
			st = generate_xml_fmrpt(fsent, outfile,
			    inargs->u.m.rptType, inargs->u.m.start,
			    inargs->u.m.end);

			LOGERR("Finished getting metrics for %s, status = %d",
			    inargs->u.m.fsname, st);

			done_with_db_task(fsent);

			break;
		case GET_FS_SNAPSHOTS:
			if (outfile == NULL) {
				st = EBADF;
				break;
			}

			st = get_fs_entry(inargs->u.c, FALSE, &fsent);
			if (st != 0) {
				break;
			}

			LOGERR("Listing recovery points for %s", inargs->u.c);

			st = db_get_snapshots(fsent, &num, &buf);
			done_with_db_task(fsent);

			if (st != 0) {
				LOGERR(
				    "Failed to get recovery points, "
				    "status = %d",
				    st);
				break;
			}

			LOGERR("Finished getting recovery points for %s",
			    inargs->u.c);

			if (buf != NULL) {
				(void) fprintf(outfile, "%s", buf);
				(void) fflush(outfile);
				ret.count = num;
				free(buf);
			}

			break;
		case DELETE_SNAPSHOT:
			if ((inargs->u.i.fsname[0] == '\0') ||
			    (inargs->u.i.snapshot[0] == '\0')) {
				LOGERR("Missing filesystem name or "
				    "recovery point");
				st = EINVAL;
				break;
			}

			LOGERR(
			    "Removing index for recovery point %s "
			    "for filesystem %s",
			    inargs->u.i.snapshot, inargs->u.i.fsname);

			st = get_fs_entry(inargs->u.i.fsname, FALSE, &fsent);
			if (st != 0) {
				break;
			}
			delargs = malloc(sizeof (delete_snap_t));
			if (delargs == NULL) {
				LOGERR(
				    "Cannot delete recovery point, no memory");
				st = ENOMEM;
				done_with_db_task(fsent);
				break;
			}
			delargs->fsent = fsent;
			strlcpy(delargs->snapname, inargs->u.i.snapshot,
			    sizeof (delargs->snapname));
			/* start delete thread */
			st = pthread_create(&tid, &pattr, db_delete_snapshot,
			    delargs);

			if (st != 0) {
				done_with_db_task(fsent);
			}

			/* db_delete_snapshot calls done_with_db_task */

			break;
		case GET_SNAPSHOT_STATUS:
			if (fd == -1) {
				st = EBADF;
				break;
			}

			st = get_fs_entry(inargs->u.c, FALSE, &fsent);
			if (st != 0) {
				break;
			}

			buflen = 0;

			LOGERR("Getting recovery point status for %s",
			    inargs->u.c);

			st = db_get_snapshot_status(fsent, &num, &buf, &buflen);
			done_with_db_task(fsent);

			LOGERR(
			    "Finished get recovery point status for %s, "
			    "status = %d",
			    inargs->u.c, st);

			if (st != 0) {
				break;
			}

			if (buflen > 0) {
				(void) write_buf(fd, buf, buflen);
				ret.count = num;
				free(buf);
			}

			break;
		case LIST_SNAPSHOT_FILES:

			if (fd == -1) {
				st = EBADF;
				break;
			}

			l = inargs->u.l;

			st = get_fs_entry(l.fsname, FALSE, &fsent);
			if (st != 0) {
				break;
			}

			lstp = NULL;

			LOGERR("Listing recovery point files for %s",
			    inargs->u.l.snapshot);

			st = db_list_files(fsent, l.snapshot, l.startDir,
			    l.startFile, l.howmany, l.which_details,
			    l.restrictions, l.includeStart,
			    &ret.count, &lstp);

			LOGERR(
			    "Finished list recovery point files for %s st = %d",
			    inargs->u.l.snapshot, st);

			done_with_db_task(fsent);

			if (st != 0) {
				break;
			}

			/* write out the list to a file */
			xdrstdio_create(&xdrs, outfile, XDR_ENCODE);
			(void) xdr_filedetails_list(&xdrs, lstp);
			xdr_destroy(&xdrs);

			lst_free_deep_typed(lstp,
			    FREEFUNCCAST(fsm_free_file_details));

			break;

		case DELETE_FSDB_FILESYS:
			if (inargs->u.c[0] == '\0') {
				/* not an error */
				st = 0;
				break;
			}
			st = get_fs_entry(inargs->u.c, TRUE, &fsent);
			if (st != 0) {
				break;
			}

			LOGERR("Remove DB for filesystem %s", inargs->u.c);

			db_remove_filesys(fsent);

			LOGERR("Removed DB for filesystem %s", inargs->u.c);

			done_with_db_task(fsent);
			break;

		default:
			LOGERR("Inavalid operation %d", inargs->task);
			st = ENOTSUP;
			break;
	}

done:

	/*
	 *  If a file descriptor was passed in, close it
	 *  or the associated FILE*
	 */
	if (outfile != NULL) {
		fclose(outfile);
	} else {
		if ((n_desc > 0) && (dp != NULL) &&
		    (dp->d_attributes == DOOR_DESCRIPTOR)) {
			if (fd > STDERR_FILENO) {
				close(fd);
			}
		}
	}

	ret.status = st;

	if (ret.status != 0) {
		LOGERR("Completed task type = %d, status = %d",
		    (inargs != NULL) ? inargs->task : -1, ret.status);
	}

	door_return((char *)&ret, sizeof (fsmdb_ret_t), NULL, 0);
}

int
add_fsent(char *dirnam, char *fsname, boolean_t create)
{
	int		st;
	fs_db_t		*fsdb;
	fs_entry_t	*fsent;

	fsent = calloc(1, sizeof (fs_entry_t));
	if (fsent == NULL) {
		return (-1);
	}

	fsdb = calloc(1, sizeof (fs_db_t));
	if (fsdb == NULL) {
		free(fsent);
		return (-1);
	}

	st = open_fs_db(dirnam, fsname, dbEnv, create, fsdb);
	if (st != 0) {
		free(fsdb);
		free(fsent);
		return (st);
	}

	fsent->fsname = strdup(fsname);
	fsent->fsdb = fsdb;

	(void) pthread_mutex_init(&(fsent->statlock), NULL);
	(void) pthread_mutex_init(&(fsent->lock), NULL);
	(void) pthread_cond_init(&fsent->statcond, NULL);


	(void) pthread_rwlock_wrlock(&fslistlock);
	fsent->next = fs_entry_list;
	fs_entry_list = fsent;
	(void) pthread_rwlock_unlock(&fslistlock);

	return (0);
}

/*
 *  Remove a filesystem from the list.
 *  If delete_databases is TRUE, removes the databases from the
 *  system.  Otherwise, the databases are just closed.
 */
void
destroy_fsent(fs_entry_t *fsent, boolean_t delete_databases)
{
	int		wait_active = 0;

	if (fsent == NULL) {
		return;
	}

	/* update the entry status and let active threads drain off */
	(void) pthread_mutex_lock(&fsent->statlock);
	fsent->status = FSENT_DELETING;

	/*
	 * if delete_databases is true, we've got an activity in progress
	 * to do the deletion.
	 */
	if (delete_databases) {
		wait_active = 1;
	}

	while (fsent->active > wait_active) {
		(void) pthread_cond_wait(&fsent->statcond, &fsent->statlock);
	}
	(void) pthread_mutex_unlock(&fsent->statlock);

	if (fsent->fsdb) {
		close_fsdb(fsent->fsname, fsent->fsdb, delete_databases);
		free(fsent->fsdb);
	}

	if (!delete_databases) {
		fsent->fsdb = NULL;
		if (fsent->fsname != NULL) {
			free(fsent->fsname);
		}

		(void) pthread_mutex_destroy(&(fsent->statlock));
		(void) pthread_cond_destroy(&(fsent->statcond));
		(void) pthread_mutex_destroy(&(fsent->lock));

		free(fsent);
	}
}

/*
 *  Removes filesystem entries from the available list.  If
 *  fsname is NULL, destroys the whole list.  Otherwise, just removes
 *  the specified entry.
 */
void
destroy_fs_list(char *fsname)
{
	fs_entry_t	*listent = fs_entry_list;
	fs_entry_t	*prev = NULL;
	fs_entry_t	*next = NULL;

	(void) pthread_rwlock_wrlock(&fslistlock);
	listent = fs_entry_list;

	while (listent != NULL) {
		next = listent->next;

		/* remove from list before destroying */
		if (fsname != NULL) {
			if (strcmp(fsname, listent->fsname) == 0) {
				if (prev != NULL) {
					prev->next = next;
				} else {
					/* removing first on the list */
					fs_entry_list = next;
				}
			} else {
				prev = listent;
				listent = NULL;
			}
		}

		if (listent != NULL) {
			destroy_fsent(listent, FALSE);
		}

		listent = next;
	}

	if (fsname != NULL) {
		fs_entry_list = NULL;
	}
	(void) pthread_rwlock_unlock(&fslistlock);
}

/* thread to clean up any aborted deletes */
static void *
finish_partial_deletes(void *arg)	/* ARGSUSED */
{
	int		st;
	fs_entry_t	*listent;
	DBT		key;
	DBT		data;
	fsmsnap_t	*snapinfo = NULL;
	DBC		*curs = NULL;
	DB		*dbp = NULL;
	uint32_t	snapid;
	DB_TXN		*txn = NULL;

	(void) pthread_rwlock_rdlock(&fslistlock);
	listent = fs_entry_list;

	for (; listent != NULL; listent = listent->next) {
		(void) pthread_mutex_lock(&listent->statlock);
		if (listent->status == FSENT_DELETING) {
			(void) pthread_mutex_unlock(&listent->statlock);
			continue;
		}
		(void) pthread_mutex_unlock(&listent->statlock);

		dbp = listent->fsdb->snapDB;

		memset(&key, 0, sizeof (DBT));
		memset(&data, 0, sizeof (DBT));

		snapid = 0;

		key.data = &snapid;
		key.size = key.ulen = sizeof (uint32_t);
		key.flags = DB_DBT_USERMEM;

		data.flags = DB_DBT_REALLOC;

		st = dbEnv->txn_begin(dbEnv, NULL, &txn, 0);
		if (st != 0) {
			break;
		}

		st = dbp->cursor(dbp, txn, &curs, 0);
		if (st != 0) {
			txn->abort(txn);
			continue;
		}

		st = curs->c_get(curs, &key, &data, DB_FIRST|DB_RMW);
		snapinfo = (fsmsnap_t *)data.data;

		while ((st == 0) && (snapinfo != NULL)) {
			if (snapinfo->flags & SNAPFLAG_DELETING) {
				/* ignore failures for now */
				(void) start_delete_snap_task(listent,
				    snapinfo);
			}
			st = curs->c_get(curs, &key, &data, DB_NEXT|DB_RMW);
		}

		curs->c_close(curs);

		txn->commit(txn, 0);

		if (snapinfo != NULL) {
			free(snapinfo);
			snapinfo = NULL;
		}
	}
	(void) pthread_rwlock_unlock(&fslistlock);

	return (NULL);
}

int
start_delete_snap_task(fs_entry_t *fsent, fsmsnap_t *snapinfo)
{
	delete_snap_t	*delarg;
	pthread_t	tid;

	if ((fsent == NULL) || (snapinfo == NULL)) {
		return (EINVAL);
	}

	/* start thread */
	delarg = malloc(sizeof (delete_snap_t));
	if (delarg == NULL) {
		/*
		 * clean it up next time when we're
		 * not short of memory...
		 */
		return (ENOMEM);
	}
	delarg->fsent = fsent;
	strlcpy(delarg->snapname, snapinfo->snapname,
	    sizeof (delarg->snapname));
	(void) pthread_create(&tid, &pattr, db_delete_snapshot, delarg);

	return (0);
}

int
get_fs_entry(char *fsname, boolean_t create, fs_entry_t **fsent)
{
	fs_entry_t	*listent = NULL;
	int		st = 0;

	if (fsent == NULL) {
		return (EINVAL);
	}

	(void) pthread_rwlock_rdlock(&fslistlock);
	listent = fs_entry_list;

	while (listent != NULL) {
		if (strcmp(listent->fsname, fsname) == 0) {
			/* increment active counter */
			(void) pthread_mutex_lock(&listent->statlock);
			if (listent->status == FSENT_DELETING) {
				st = EINTR;
			} else {
				(listent->active)++;
			}
			(void) pthread_mutex_unlock(&listent->statlock);
			break;
		}
		listent = listent->next;
	}
	(void) pthread_rwlock_unlock(&fslistlock);

	if (st != 0) {
		return (st);
	}

	if (listent == NULL) {
		st = add_fsent(fsmdbdir, fsname, create);
		if (st == 0) {
			/* don't try to create more than once */
			st = get_fs_entry(fsname, FALSE, &listent);
		}
	}

	/* if failure, st will be non-0 & *fsent will be NULL */
	*fsent = listent;
	return (st);
}

/* helper function to use write() correctly */
static int
write_buf(int fd, void *buffer, int len)
{
	int	written = 0;
	int	ret;
	char	*bufp;

	if ((buffer == NULL) || (fd == -1)) {
		return (-1);
	}

	bufp = buffer;

	while (written < len) {
		ret = write(fd, bufp, (len - written));

		if (ret == -1) {
			if (errno == EAGAIN) {
				continue;
			}
			written = -1;
			break;
		}

		written += ret;
		bufp += written;
	}

	return (written);
}

/* Helper function to ensure activity counter decremented */
void
done_with_db_task(fs_entry_t *fsent)
{
	if (fsent == NULL) {
		return;
	}

	(void) pthread_mutex_lock(&fsent->statlock);
	(fsent->active)--;
	(void) pthread_cond_signal(&fsent->statcond);
	(void) pthread_mutex_unlock(&fsent->statlock);
}

/* Copied from file_details.h - remove if/when can link with libfsmgmt.so */
static void
fsm_free_file_details(filedetails_t *details)
{
	int		i;
	int		j;
	fildet_seg_t	*segp;

	if (details == NULL) {
		return;
	}

	free(details->file_name);

	/* free segments */
	if ((details->segCount > 0) && (details->segments)) {
		for (i = 0; i < details->segCount; i++) {
			segp = &(details->segments[i]);

			if (segp == NULL) {
				break;
			}

			for (j = 0; j < MAX_ARCHIVE; j++) {
				if (segp->copy[j].vsns != NULL) {
					free(segp->copy[j].vsns);
				}
			}
		}
		free(details->segments);
	}

	/* free summary */
	segp = &(details->summary);
	for (j = 0; j < MAX_ARCHIVE; j++) {
		if (segp->copy[j].vsns != NULL) {
			free(segp->copy[j].vsns);
		}
	}
}

void
fsmdb_log_err(
	const DB_ENV	*dbenv,		/* ARGSUSED */
	const char	*errpfx,	/* ARGSUSED */
	const char	*msg)
{
	char		timbuf[MAXPATHLEN];
	time_t		logtime;
	char		*pfxp = (char *)errpfx;

	if (msg == NULL) {
		return;
	}

	if (pfxp == NULL) {
		pfxp = "";
	}

	logtime = time(NULL);
	(void) samcftime(timbuf, timefmt, &logtime);
	(void) fprintf(fsmdb_errFilep, "%s [%ld] %s: %s\n", timbuf, fsmdb_pid,
	    pfxp, msg);
	(void) fflush(fsmdb_errFilep);
}

void
do_log_err(char *errpfx, char *fmt, ...)
{
	va_list		ap;
	char		buf[2048];

	va_start(ap, fmt);
	vsnprintf(buf, sizeof (buf), fmt, ap);
	va_end(ap);

	fsmdb_log_err(dbEnv, errpfx, buf);
}

void
incr_active(void)
{
	pthread_mutex_lock(&ckptmutex);
	active++;
	pthread_cond_signal(&ckptcond);
	pthread_mutex_unlock(&ckptmutex);
}

void
decr_active(void)
{
	pthread_mutex_lock(&ckptmutex);
	active--;
	pthread_cond_signal(&ckptcond);
	pthread_mutex_unlock(&ckptmutex);
}

/*
 *  function to periodically run dbEnv->checkpoint to keep the
 *  number of transaction log files manageable.
 */
static void *
do_checkpoint(void *arg)
{
	boolean_t	stopme = B_FALSE;
	struct timespec	ts;
	int		txnsize = (MEGA * 40) / KILO; /* sz in kb */
	char		*errpfx = "ckpt";

	/* 10 second sleeps while we're busy - may need to adjust this */
	ts.tv_sec = 10;
	ts.tv_nsec = 0;

	pthread_mutex_lock(&glock);
	stopme = stopserver;
	pthread_mutex_unlock(&glock);

	while (!stopme) {
		/* Checkpoint if at least 40 MB of logs */
		dbEnv->txn_checkpoint(dbEnv, txnsize, 0, 0);

		/* sleep for a little while */
		nanosleep(&ts, NULL);

		/* is there anything to do? */
		pthread_mutex_lock(&ckptmutex);
		while (active <= 0) {
			/*
			 * we'll be woken up if an index
			 * or delete request comes in.
			 */
			pthread_cond_wait(&ckptcond, &ckptmutex);

			/* make sure the server isn't supposed to exit */
			pthread_mutex_lock(&glock);
			stopme = stopserver;
			pthread_mutex_unlock(&glock);
		}
		pthread_mutex_unlock(&ckptmutex);
	}

	return (NULL);
}
