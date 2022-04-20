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
#pragma ident	"$Revision: 1.48 $"

#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <fnmatch.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <pthread.h>
#include <wait.h>
#include <libgen.h>
#include <syslog.h>
#include <errno.h>
#include <stddef.h>
#include "pub/mgmt/types.h"
#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/restore.h"
#include "pub/mgmt/process_job.h"
#include "sam/sam_trace.h"
#include "mgmt/util.h"
#include "pub/mgmt/fsmdb_api.h"
#include "pub/mgmt/task_schedule.h"

#define	BUFFSIZ		MAXPATHLEN
#define	UNCOMPRESS	"/usr/bin/uncompress"
#define	GUNZIP		"/usr/bin/gunzip"
#define	DIFF_LIMIT	10	/* allowable difference (%) between filesize */

/* Macro to propagate and return error value */
#define	RVAL(X) {rval = X; if (rval) return (rval); }

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/* static function prototypes */
static int parsekv_snapid(char *ptr, void *bufp);

/* ---------------------------------------------------------------- */
/* Utility routines */

int
getdumpdir(char *fsname, char *dumpdirname)
{
	int		st;
	char		mountpt[BUFFSIZ];
	char		tmpbuf[BUFFSIZ];
	snapsched_t	sched;
	struct stat64	statbuf;
	sqm_lst_t		*lstp = NULL;
	char		*schedstr;

	if (ISNULL(fsname, dumpdirname)) {
		return (-1);
	}

	dumpdirname[0] = '\0';

	st = get_specific_tasks(NULL, "SN", fsname, &lstp);
	if (st != 0) {
		return (-1);
	} else if (lstp->length == 0) {
		lst_free(lstp);
		return (-1);
	}

	schedstr = (char *)lstp->head->data;

	/* parse function */
	st = parse_snapsched(schedstr, &sched, sizeof (snapsched_t));

	lst_free_deep(lstp);

	if (st != 0) {
		return (-1);
	}

	/*
	 * location can be either fully-qualified or
	 * relative to the mountpoint.
	 */
	if (sched.location[0] == '/') {  /* fully-qualified */
		strlcpy(tmpbuf, sched.location, BUFFSIZ);
	} else {
		/* Obtain current location of file system */
		st = getfsmountpt(fsname, mountpt, sizeof (mountpt));

		if (st != 0) {
			st = samrerr(SE_FS_NOT_MOUNTED, fsname);
		}

		snprintf(tmpbuf, BUFFSIZ, "%s/%s", mountpt,
		    sched.location);
	}

	if (realpath(tmpbuf, dumpdirname)) {
		st = stat64(dumpdirname, &statbuf);
		if ((st != 0) || !(S_ISDIR(statbuf.st_mode))) {
			st = samrerr(SE_NOT_A_DIR, dumpdirname);
		}
	} else {
		st = samrerr(SE_NOT_A_DIR, tmpbuf);
	}

	return (st);
}

/*
 * Tree-walk of dump directory
 *
 * Recursiveness is currently disabled.
 */
int
walk_dumps(char *location, char *pathname, sqm_lst_t **dumps)
{
	int		rval = 0;
	DIR		*curdir;
	dirent64_t	*entry;
	dirent64_t	*entryp;
	struct stat64	sout;
	int		len = MAXPATHLEN + 1;
	char		filename[len];
	char		*data;
	char		useloc[len];
	int		loclen;
	char		*filenamep;

	if (ISNULL(location, pathname, dumps)) {
		return (-1);
	}

	if (strlen(pathname)) {
		snprintf(useloc, MAXPATHLEN, "%s/%s", location, pathname);
	} else {
		strlcpy(useloc, location, MAXPATHLEN);
	}
	loclen = strlen(useloc);

	curdir = opendir(useloc); /* Set up to ask for directory entries */
	if (curdir == NULL) {
		return (samrerr(SE_NOT_A_DIR, useloc));
	}

	/* allocate the dirent space */
	entryp = mallocer(sizeof (dirent64_t) + MAXPATHLEN + 1);
	if (entryp == NULL) {
		return (-1);
	}

	strlcpy(filename, useloc, sizeof (filename));
	filename[loclen] = '/';
	loclen++;

	/* Walk through directory entries */
	while ((rval = readdir64_r(curdir, entryp, &entry)) == 0) {
		if (entry == NULL) {
			break;
		}

		filenamep = NULL;

		if ((strcmp(entry->d_name, ".") == 0) ||
		    (strcmp(entry->d_name, "..") == 0)) {
			continue;
		}

		/* create pathname to file */
		strlcpy(filename + loclen, entry->d_name, len - loclen);

		/* Take a look at this file */
		rval = stat64(filename, &sout);

		if (rval) {
			continue; /* Ignore bad files */
		}

#if 0
		if (S_ISDIR(sout.st_mode)) {
			rval = walk_dumps(location, entry->d_name, dumps);
		}
#endif

		if (!S_ISREG(sout.st_mode)) {
			continue;
		};

		/*
		 * Look only at files ending in .dmp, .dmp.Z, and .dmp.gz
		 */
		if ((fnmatch("*.dmp", entry->d_name, 0) == 0) ||
		    (fnmatch("*.dmp.gz", entry->d_name, 0) == 0) ||
		    (fnmatch("*.dmp.Z", entry->d_name, 0) == 0)) {
			filenamep = strstr(entry->d_name, ".lk.dmp");
			if (filenamep == NULL) {
				filenamep = strstr(entry->d_name, ".dmp");
			}
		}

		if (filenamep == NULL) {
			/* not what we're looking for */
			continue;
		}

		/* strip off the .dmp file extension */
		*filenamep = '\0';

		strlcpy(filename + loclen, entry->d_name, len - loclen);

		/* This is a file we care about */
		data = copystr(filename);
		if (data == NULL) {
			rval = -1;
			break; /* If allocation fails, stop */
		}

		/* Append this string to list */
		rval = lst_append(*dumps, data);
		if (rval) {
			free(data);
			break;
		}
	}

	if (rval) {
		lst_free_deep(*dumps);
		*dumps = NULL;
	}

	closedir(curdir);
	free(entryp);

	return (rval);
}

/* Routine to make a dump available - decompress (if necessary) and index */
/* Called as independent thread, cannot return */
void *
samr_decomp(void* jobid)
{
	argbuf_t *arg;
	char 		zname[BUFFSIZ];
	struct stat64 	sout;
	char 		snapdir[BUFFSIZ];
	char 		cmd[BUFFSIZ];
	int 		rval;
	int 		dmpstat = 0;
	pid_t 		pid;
	char		*dirp = NULL;
	char		*namep = NULL;
	int		locked;
	int		compressed;
	int		status = 0;
	FILE		*logfil = NULL;
	int		fd;
	uint64_t	taskid;		/* for future fsmdb use */

	arg = samr_get_args((char *)jobid);
	if (arg == NULL) {
		end_this_activity((char *)jobid);
		return (NULL);
	}

	/*
	 * Free the jobid now and use the one in the arg from here on.
	 * This allows the cancelation handler to correctly free all
	 * data.
	 */
	free((char *)jobid);

	/*
	 * push a cleanup handler so if the thread gets canceled
	 * all resources get released
	 */
	pthread_cleanup_push(&decomcleanup, arg);

	namep = arg->d.dumppath;

	if (*(arg->d.dumppath) != '/') {
		/* Get CSD directory */
		rval = getdumpdir(arg->d.fsname, snapdir);
		if (rval) {
			Trace(TR_DEBUG, "Cannot find snapshot dir for %s\n",
			    arg->d.fsname);
			goto done;
		}
		dirp = snapdir;
	}

	/* Look for dump file, possibly compressed */
	rval = get_snap_name(dirp, namep, zname, &locked, &compressed, &sout);
	if (rval != 0) {
		goto done;
	}

	/* see if someone is cancelling this job */
	pthread_testcancel();

	/* Decompress .Z files.  Gzipped files can be used as-is. */

	if (compressed == 2) {
		char 		dname[BUFFSIZ];
		struct timeval	ztimes[2];
		char		*ptr;

		ztimes[0].tv_sec = sout.st_atim.tv_sec;
		ztimes[0].tv_usec = sout.st_atim.tv_nsec / 1000;
		ztimes[1].tv_sec = sout.st_mtim.tv_sec;
		ztimes[1].tv_usec = sout.st_mtim.tv_nsec / 1000;

		/* strip off the .Z */
		strcpy(dname, zname);

		ptr = strrchr(dname, '.');
		if (ptr != NULL) {
			*ptr = '\0';
		}
		snprintf(cmd, BUFFSIZ, "%s <%s >%s 2>>%s",
		    GUNZIP, zname, dname, RESTORELOG);

		pid = exec_get_output(cmd, NULL, NULL);

		if (pid == -1) {
			Trace(TR_DEBUG, "Decompression fork failed\n");
			goto done;
		}

		set_pid_or_tid(arg->d.xjobid, pid, 0);

		/* Wait for decompression fork to finish */
		while (waitpid(pid, &status, 0) >= 0) {
			sleep(1);
		}

		/* ensure that uncompress succeeded */
		dmpstat = stat64(dname, &sout);

		if ((!WIFEXITED(status)) || (WEXITSTATUS(status) != 0) ||
		    (dmpstat != 0)) {
			snprintf(cmd, sizeof (cmd),
			    GetCustMsg(SE_CANT_DECOMPRESS), zname);

			if ((fd = open64(RESTORELOG, O_WRONLY|O_APPEND|O_CREAT,
			    0644)) != -1) {
				logfil = fdopen(fd, "a");
			}

			if (logfil != NULL) {
				rlog(logfil, cmd, zname, NULL);
				fclose(logfil);
			}

			PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
			    SE_CANT_DECOMPRESS, LOG_WARNING, cmd,
			    NOTIFY_AS_FAULT | NOTIFY_AS_EMAIL);

			Trace(TR_DEBUG, "No dump for %s\n", arg->d.dumppath);
			goto done;
		}

		pthread_testcancel();

		/*
		 * reset the time on the decompressed file so it
		 * accurately reflects the time the snapshot was taken.
		 */
		rval = utimes(dname, ztimes);

		/* set the new name for use by indexing */
		strlcpy(zname, dname, sizeof (zname));
	}

	/* Create index file if necessary */
	rval = import_from_samfsdump(arg->d.fsname, zname, &taskid);
	if ((rval != 0) && (rval != EALREADY)) {
		snprintf(cmd, sizeof (cmd),
		    GetCustMsg(SE_DUMP_INDEX_FAILED), zname);

		logfil = NULL;
		if ((fd = open64(RESTORELOG, O_WRONLY|O_APPEND|O_CREAT, 0644))
		    != -1) {
			logfil = fdopen(fd, "a");
		}

		if (logfil != NULL) {
			rlog(logfil, cmd, zname, NULL);
			fclose(logfil);
		}

		PostEvent(DUMP_CLASS, DUMP_WARN_SUBCLASS,
		    SE_DUMP_INDEX_FAILED, LOG_WARNING, cmd,
		    NOTIFY_AS_FAULT | NOTIFY_AS_EMAIL);

		Trace(TR_DEBUG, "No index for %s\n", arg->d.dumppath);
	}

done:
	pthread_cleanup_pop(1);

	return (NULL);
}

/*
 * Cleanup snapshot:	Get rid of index and
 *			If delete_all is TRUE, delete everything
 *				associated with this dumppath
 *			If delete_all is FALSE, leave only the compressed
 *				dump (if it exists), or the
 * 				uncompressed dump itself
 *
 *			If snapshot is marked as locked, cleanup is
 *				performed, but a snapshot is always left.
 */
int
samrcleanup(char *fsname, char *dumppath, boolean_t delete_all)
{
	char		snapdir[MAXPATHLEN+1];
	char 		namebuff[MAXPATHLEN+1];
	char 		dmpname[MAXPATHLEN+1];
	int 		rval;
	int 		dbval;
	struct stat64 	sout;
	char		*exts[] = {"Z", "gz", NULL};
	char		*dirp = NULL;
	char		*namep = NULL;
	int		i = 0;
	int		locked;
	int		compressed;
	uint64_t	taskid;		/* for future fsmdb use */

	/*
	 * If dumppath is not fully qualified, find the
	 * Default dump directory for this file system
	 */
	if (dumppath[0] != '/') {
		rval = getdumpdir(fsname, snapdir);
		if (rval) {
			return (rval);
		}
		dirp = snapdir;
		namep = dumppath;
	} else {
		dirp = NULL;
		namep = dumppath;
	}

	dmpname[0] = '\0';

	/*
	 *  Get rid of index no matter what.  Depend on the existence
	 *  of compressed dumps to decide when to delete the uncompressed
	 *  dump file unless delete_all is TRUE, in which case, delete
	 *  all of them.  Note that get_snap_name sets compressed if
	 *  only a compressed snap file exists.
	 */

	rval = get_snap_name(dirp, namep, dmpname, &locked, &compressed, &sout);

	/* even if no snapshot file exists, make sure the index gets deleted */
	if (rval != 0) {
		if (dirp != NULL) {
			snprintf(dmpname, sizeof (dmpname), "%s/", dirp);
		}
		strlcat(dmpname, namep, sizeof (dmpname));
	}

	/* if this returns ENOENT, no index exists for this snapshot */
	dbval = delete_fsmdb_snapshot(fsname, dmpname, &taskid);
	if ((dbval != 0) && (dbval != ENOENT)) {
		samerrno = SE_INDEX_NOT_DELETED;
		snprintf(samerrmsg, MAX_MSG_LEN, GetCustMsg(samerrno),
		    dumppath, dbval);
		return (-1);
	}

	/* if no snapshot exists, we're done */
	if (rval != 0) {
		return (0);
	}

	/*
	 * if get_snap_name set compressed, no uncompressed dump exists
	 */
	if (!compressed) {
		while (exts[i] != NULL) {
			snprintf(namebuff, MAXPATHLEN, "%s.%s", dmpname,
			    exts[i]);
			if (stat64(namebuff, &sout) == 0) {
				unlink(dmpname);
				strcpy(dmpname, namebuff);

				break;
			}
			i++;
		}
	}

	/* all the interim files and the index have been tidied up now */
	if (delete_all) {
		if (locked) {
			return (samrerr(SE_SNAP_CANT_DELETE, dumppath));
		} else {
			unlink(dmpname);
		}
	}

	return (0);
}

/*
 * utility routine to determine dump file name and status
 *
 * snappath must be a buffer of at least MAXPATHLEN.
 *
 * compressed is set to 1 for gzipped snapshots, and 2 for compressed snaps.
 */
int
get_snap_name(
	char		*usepath,
	char		*dmpname,
	char		*snappath,
	int		*locked,
	int		*compressed,
	struct stat64	*snapstats)
{
	struct stat64 	sout;
	int 		dmpstat = -1;
	int		dmpzstat = -1;
	char		namebuf[MAXPATHLEN];
	char		*endp;
	char		*endp2;
	char		*exts[] = {".dmp", ".lk.dmp", NULL};
	int		i = 0;
	int		cmp = 0;

	if (ISNULL(dmpname, snappath)) {
		return (-1);
	}

	if (locked != NULL) {
		*locked = 0;
	}

	/* dmpname could be relative or fully qualified */
	if (dmpname[0] != '/') {
		if (ISNULL(usepath)) {
			return (-1);
		}

		snprintf(namebuf, MAXPATHLEN, "%s/%s", usepath, dmpname);
	} else {
		strlcpy(namebuf, dmpname, MAXPATHLEN);
	}

	endp = &namebuf[strlen(namebuf)];

	while (exts[i] != NULL) {
		strcpy(endp, exts[i]);

		dmpstat = stat64(namebuf, &sout);
		dmpzstat = -1;

		/* Only look for compressed dump if no uncompressed dump */
		if (dmpstat == -1) {
			endp2 = &namebuf[strlen(namebuf)];
			strcpy(endp2, ".gz");
			dmpzstat = stat64(namebuf, &sout);
			if (dmpzstat == 0) {
				cmp = 1;
			} else {
				strcpy(endp2, ".Z");
				dmpzstat = stat64(namebuf, &sout);
				if (dmpzstat == 0) {
					cmp = 2;
				}
			}
		}
		if ((dmpstat == 0) || (dmpzstat == 0)) {
			/* found it */
			if (locked != NULL) {
				*locked = i;
			}
			break;
		}
		i++;
	}

	/* Have details, start generating information */

	if ((dmpstat == -1) && (dmpzstat == -1)) {
		return (-1);
	}

	if (compressed != NULL) {
		*compressed = cmp;
	}

	if (snapstats != NULL) {
		memcpy(snapstats, &sout, sizeof (struct stat64));
	}

	strcpy(snappath, namebuf);

	return (0);
}

/* function to turn a snapsched_t into a string */
int
snapsched_to_string(snapsched_t *sched, char *buf, size_t buflen)
{
	int		a;
	int		i;

	if (ISNULL(sched, buf)) {
		return (-1);
	}

	a = snprintf(buf, buflen,
	    "task=SN,starttime=%s,periodicity=%s,id=%s",
	    sched->starttime, sched->periodicity, sched->id.fsname);

	if (sched->id.startDir[0] != '\0') {
		a += snprintf(buf + a, buflen - a, "/%s", sched->id.startDir);
	}

	if (sched->duration[0] != '\0') {
		a += snprintf(buf + a, buflen - a, ",duration=%s",
		    sched->duration);
	}

	a += snprintf(buf + a, buflen - a,
	    ",location=%s,names=%s,autoindex=%d,disabled=%d",
	    sched->location, sched->namefmt, sched->autoindex,
	    sched->disabled);

	if (sched->prescript[0] != '\0') {
		a += snprintf(buf + a, buflen - a,
		    ",prescript=%s,prescrfatal=%d",
		    sched->prescript, sched->prescriptFatal);
	}

	if (sched->postscript[0] != '\0') {
		a += snprintf(buf + a, buflen - a,
		    ",postscript=%s", sched->postscript);
	}

	if (sched->compress != 0) {
		a += snprintf(buf + a, buflen - a,
		    ",compress=%d", sched->compress);
	}

	if (sched->logfile[0] != '\0') {
		a += snprintf(buf + a, buflen - a,
		    ",logfile=%s", sched->logfile);
	}

	if (sched->excludeDirs[0][0] != '\0') {
		a += snprintf(buf + a, buflen - a, ",excludedirs=%s",
		    sched->excludeDirs[0]);
		for (i = 1; i < 10; i++) {
			if (sched->excludeDirs[i][0] == '\0') {
				break;
			}
			a += snprintf(buf + a, buflen - a,
			    ":%s", sched->excludeDirs[i]);
		}
	}

	return (0);
}

#define	sn_off(name) (offsetof(snapsched_t, name))

static parsekv_t snaptokens[] = {
	{"id",		sn_off(id),		parsekv_snapid},
	{"location",	sn_off(location),	parsekv_string_1024},
	{"names",	sn_off(namefmt),	parsekv_string_1024},
	{"prescript",	sn_off(prescript),	parsekv_string_1024},
	{"postscript",	sn_off(postscript),	parsekv_string_1024},
	{"logfile",	sn_off(logfile),	parsekv_string_1024},
	{"starttime",	sn_off(starttime),	parsekv_string_16},
	{"periodicity",	sn_off(periodicity),	parsekv_string_16},
	{"duration",	sn_off(duration),	parsekv_string_16},
	{"excludedirs",	sn_off(excludeDirs),	parsekv_dirs},
	{"compress",	sn_off(compress),	parsekv_int},
	{"autoindex",	sn_off(autoindex),	parsekv_bool},
	{"disabled",	sn_off(disabled),	parsekv_bool},
	{"prescrfatal",	sn_off(prescriptFatal),	parsekv_bool},
	{"difflimit",	sn_off(diffLimit),	parsekv_llu},
	{"",		0,			NULL}
};

static int
parsekv_snapid(char *ptr, void *bufp)
{
	char		*slash;
	snapsched_id_t	*id = (snapsched_id_t *)bufp;

	if (ISNULL(ptr, bufp)) {
		return (-1);
	}

	slash = strchr(ptr, '/');
	if (slash == NULL) {
		strlcpy(id->fsname, ptr, sizeof (id->fsname));
	} else {
		size_t	len = slash - ptr;
		if (len > sizeof (id->fsname)) {
			return (-1);
		}
		/* got a starting directory */
		strlcpy(id->startDir, (slash + 1), sizeof (id->startDir));
		strncpy(id->fsname, ptr, len);
		id->fsname[len - 1] = '\0';
	}

	return (0);
}

int
parse_snapsched(char *str, snapsched_t *sched, int len)	/* ARGSUSED */
{
	int		st;

	if (ISNULL(str, sched)) {
		return (-1);
	}

	memset(sched, 0, sizeof (snapsched_t));
	sched->prescriptFatal = TRUE;
	sched->diffLimit = DIFF_LIMIT;

	st = parse_kv(str, snaptokens, (void*)sched);
	if (st != 0) {
		return (st);
	}

	/* validate we got the params we absolutely need */
	if ((sched->id.fsname[0] == '\0') ||
	    (sched->location[0] == '\0') ||
	    (sched->namefmt[0] == '\0') ||
	    (sched->starttime[0] == '\0') ||
	    (sched->periodicity[0] == '\0')) {
		return (samrerr(SE_MISSINGKEY, str));
	}

	return (0);
}

/*
 * Turns a string of colon-separated entries (max of 10)
 * into an array of strings.
 *
 *  Make this a static function as soon as the old csdbuf parser is removed.
 */
int
parsekv_dirs(char *ptr, void *bufp)
{
	char	dirs[10][MAXPATHLEN];
	char	*tokp;
	char	*rest;
	int 	i = 0;

	if (ptr == NULL) {
		return (-1);
	}

	memset(dirs, 0, sizeof (dirs));

	tokp = strtok_r(ptr, ":", &rest);

	while ((tokp != NULL) && (i < 10)) {
		strlcpy(dirs[i], tokp, MAXPATHLEN);
		i++;
		tokp = strtok_r(NULL, ":", &rest);
	}

	memcpy(bufp, &dirs[0], sizeof (dirs));

	return (0);
}
