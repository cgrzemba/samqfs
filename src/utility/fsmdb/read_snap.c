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

/* Includes */
#include <sys/types.h>
#include <errno.h>
#include <sys/param.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <zlib.h>
#include <sys/vfs.h>
#include <ftw.h>
#include <pub/stat.h>
#include "mgmt/cmn_csd.h"
#include "sam/fs/sblk.h"
#include "sam/lib.h"
#include "mgmt/fsmdb.h"
#include "mgmt/file_metrics.h"
#include "pub/mgmt/restore.h"


/* globals */

/* thread-specific data for live-filesystem data gathering */
static pthread_key_t  fmkey;
static pthread_once_t fmkey_once = PTHREAD_ONCE_INIT;
static void fmkey_init(void);

/*
 * Struct to hold the report arguments and results
 */
typedef struct {
	fs_entry_t	*fsent;
	fsmsnap_t	*snapdata;
	void		*arg;
	void		*res;
} fm_tsd_t;

fm_tsd_t *fmkey_tsd(void);

int
walk_nftw(const char *filename, const struct stat64 *statptr,
	int fileflags, struct FTW *pfwt);

/*
 * Read a csd dump file so as to index files within.
 *
 * This should mutate into a function that returns a generic set
 * of stuff that could be used by samindexdump and the database stuff,
 * when and IF we're able to link against libfsmgmt.
 */
int
read_snapfile_entry(
	dumpspec_t	*dsp,		/* in */
	char		**filename,	/* out */
	filvar_t	*filvar,	/* out */
	filinfo_t	*filinfo,	/* out */
	int		*media		/* out */
)
{
	gzFile			gzf = dsp->fildmp;
	int			csd_version = dsp->csdversion;
	boolean_t		swap = dsp->byteswapped;
	csd_fhdr_t 		file_hdr;
	char			name[MAXPATHLEN + 1];
	char			link[MAXPATHLEN + 1];
	int			namelen;
	struct sam_perm_inode	perm_inode;
	struct sam_vsn_section	*vsnp;
	void			*data = NULL;
	int			n_acls;
	aclent_t		*aclp;
	int			st;
	char			*namep;
	off_t			inode_offset;
	char			*ptr;
	int			copy;
	audvsn_t		ivsn;
	uint32_t		vsnid;

	/*
	 *	Read the dump file
	 */

	st = common_csd_read_header(gzf, csd_version, swap, &file_hdr);

	if (st <= 0) {
		return (st);
	}

	namelen = file_hdr.namelen;
	inode_offset = gztell(gzf);

	st = common_csd_read(gzf, name, namelen, swap, &perm_inode);
	if (st != 0) {
		goto done_with_file;
	}

	/* we store the offset of the inode itself, so update it */
	inode_offset += namelen;

	/*
	 * Strip the first slash to make relative path and
	 * then move location ahead past first '/'
	 */
	if ('/' == *name) {
		/* if root inode, just skip it */
		if (namelen == 1) {
			goto done_with_file;
		}
		namep = &name[1];
		namelen--;
		/* kill off the mountpoint part of the path */
		ptr = strchr(namep, '/');
		if (ptr != NULL) {
			ptr++;	/* move past slash */
			namelen -= (ptr - namep);
			namep = ptr;
		}
	} else if ((name[0] == '.') && (name[1] == '/')) {
		namep = &name[2];
		namelen -= 2;
	} else {
		namep = &name[0];
	}

	data = NULL;
	vsnp = NULL;
	aclp = NULL;
	n_acls = 0;
	if (S_ISREQ(perm_inode.di.mode)) {
		data = malloc(perm_inode.di.psize.rmfile);
	}

	common_csd_read_next(gzf, swap, csd_version, &perm_inode,
	    &vsnp, link, data, &n_acls, &aclp);

	/* Check for special files.  */

	/* Skip priviledge inodes except root */
	if (SAM_PRIVILEGE_INO(perm_inode.di.version, perm_inode.di.id.ino)) {
		if (perm_inode.di.id.ino != SAM_ROOT_INO) {
			goto done_with_file;
		}
	}

	*filename = strdup(namep);
	if (*filename == NULL) {
		(void) fprintf(stderr, "Out of memory.\n");
		return (1);
	}

	/* fixed size structures, no need to malloc */
	memset(filvar, 0, FILVAR_DATA_OFF + FILVAR_DATA_SZ);
	memset(filinfo, 0, FILINFO_DATA_OFF + FILINFO_DATA_SZ);
	memset(media, 0, 4 * sizeof (int));

	if (file_hdr.flags & CSD_FH_DATA) {
		filinfo->flags |= FILE_HASDATA;
	}

	filvar->offset = inode_offset;
	filvar->atime = perm_inode.di.access_time.tv_sec;

	filvar->mtime = filinfo->fuid.mtime = perm_inode.di.modify_time.tv_sec;
	filinfo->ctime = perm_inode.di.creation_time;
	filinfo->size = perm_inode.di.rm.size;
	/*
	 * some files have a bogus size recorded - -1 on an unsigned int,
	 * so check for this and treat it as if it were 0 length.
	 */
	if (filinfo->size == INT64_MAX) {
		filinfo->size = 0;
	}

	filinfo->owner = perm_inode.di.uid;
	filinfo->group = perm_inode.di.gid;
	filinfo->perms = perm_inode.di.mode;

	/* if it's a partial file, keep track of the online size */
	if (perm_inode.di.status.b.pextents == 1) {
		filinfo->flags |= FILE_PARTIAL;
		filinfo->osize = perm_inode.di.psize.partial * KILO;
		if (filinfo->osize > filinfo->size) {
			filinfo->osize = filinfo->size;
		}
	}
	if (perm_inode.di.status.b.offline == 0) {
		filinfo->osize = filinfo->size;
	}

	/* What should we do with segments? */
	if (S_ISSEGI(&perm_inode.di)) {
		struct sam_perm_inode	seg_inode;
		struct sam_vsn_section	*seg_vsnp;
		int			i;
		offset_t		seg_size;
		int			no_seg;

		/*
		 * Read each segment inode. If archive copies
		 * overflowed, read vsn sections directly after each
		 * segment inode.
		 */

		seg_size = perm_inode.di.rm.info.dk.seg_size *
		    SAM_MIN_SEGMENT_SIZE;
		no_seg = (perm_inode.di.rm.size + seg_size - 1) / seg_size;

		for (i = 0; i < no_seg; i++) {
			gzread(gzf, &seg_inode, sizeof (seg_inode));
			seg_vsnp = NULL;
			common_csd_read_mve(gzf, csd_version,
			    &seg_inode, &seg_vsnp, swap);
			if (seg_vsnp) {
				free(seg_vsnp);
			}
		}
	}

	/* Notify user of previously or newly damaged files */
	if (perm_inode.di.status.b.damaged) {
		filinfo->flags |= ARCH_DAMAGED;
	} else if ((S_ISREG(filinfo->perms) && !S_ISSEGI(&perm_inode.di)) &&
	    (perm_inode.di.arch_status == 0) && (filinfo->size != 0) &&
	    (filinfo->flags & FILE_HASDATA)) {

		/*
		 * If inode has stale copies, file is marked
		 * inconsistent.
		 */
		for (copy = 0; copy < MAX_ARCHIVE; copy++) {
			if ((perm_inode.di.ar_flags[copy]
			    & AR_inconsistent) &&
			    (perm_inode.ar.image[copy].vsn[0] != 0)) {
				filinfo->flags |= ARCH_INCONSISTENT;
			}
		}

		/*
		 *  If the file has not been archived, has data,
		 *  and is not inconsistent, mark it as damaged.
		 */
		if (!(filinfo->flags & ARCH_INCONSISTENT)) {
			filinfo->flags |= ARCH_DAMAGED;
		}
	}

	for (copy = 0; copy < 4; copy ++) {
		if (perm_inode.ar.image[copy].n_vsns < 1) {
			continue;
		}
		filinfo->arch[copy].archtime =
		    perm_inode.ar.image[copy].creation_time;
		/* add vsn offset when available */
		/* are there flags to be saved?? */

		memset(&ivsn, 0, sizeof (audvsn_t));

		ivsn.mtype = perm_inode.di.media[copy];
		(void) strcpy(ivsn.vsn, perm_inode.ar.image[copy].vsn);

		st = db_add_vsn(&ivsn, &vsnid);
		if (st != 0) {
			goto done_with_file;
		}

		filinfo->arch[copy].vsn = vsnid;

		media[copy] = ivsn.mtype;
	}

	/* Notify user if file was online at the time of samfsdump */
	if (!perm_inode.di.status.b.offline) {
		filinfo->flags |= FILE_ONLINE;
	}

/* add leak handling for getting here on error ... */

done_with_file:
	if (data) {
		free(data);
	}
	if (vsnp) {
		free(vsnp);
	}
	if (aclp) {
		free(aclp);
	}
	if (filinfo->flags & FILE_HASDATA) {
		common_skip_embedded_file_data(gzf);
	}

	return (st);
}

/* initializes key for thread-specific data */
static void
fmkey_init(void)
{
	(void) pthread_key_create(&fmkey, free);
}

/*
 * Function to return the thread specific data
 */
fm_tsd_t *
fmkey_tsd(void)
{
	fm_tsd_t   *fmtsd = NULL;

	/* initialize the key, if it hasn't been already */
	(void) pthread_once(&fmkey_once, fmkey_init);

	fmtsd = pthread_getspecific(fmkey);
	if (fmtsd == NULL) {		/* not yet set */
		fmtsd = calloc(1, sizeof (fm_tsd_t));
		(void) pthread_setspecific(fmkey, fmtsd);
	}

	return (fmtsd);
}

/*
 * walk_live_fs()
 *
 * Uses nftw() to walk through the live filesystem to gather metrics
 * information.
 */
int
walk_live_fs(fs_entry_t *fsent, char *mountpt)
{
	int		flags = FTW_MOUNT|FTW_PHYS;
	fm_tsd_t	*fmtsd = fmkey_tsd();
	time_t		now = time(NULL);
	fsmsnap_t	*snapdata = NULL;
	int		st = 0;
	uint32_t	snapid;
	size_t		len = sizeof (fsmsnap_t) + 24; /* add for snapname */
	DBT		key;
	DB		*dbp;

	if (fsent == NULL) {
		return (EINVAL);
	}

	if (fmtsd == NULL) {
		return (ENOMEM);
	}

	dbp = fsent->fsdb->snapDB;

	/* set up the thread specific data */
	fmtsd->fsent = fsent;
	fmtsd->arg = NULL;
	fmtsd->res = NULL;

	snapdata = calloc(1, len);
	if (snapdata == NULL) {
		st = ENOMEM;
		goto done;
	}
	snprintf(snapdata->snapname, 24, "live_%ld", now);
	snapdata->snapdate = now;

	fmtsd->snapdata = snapdata;

	st = nftw64(mountpt, walk_nftw, 5, flags);
	if (st == 0) {
		/*
		 * record the snapfile entry - we need the snapid to
		 * store the metrics results
		 */
		snapdata->end = time(NULL);
		st = db_add_snapshot(fsent, snapdata, len, &snapid);

		/* store the metrics results */
		st = finish_snap_metrics(fsent->fsdb, snapdata, &(fmtsd->arg),
		    &(fmtsd->res));
		if (st != 0) {
			/* remove this snapshot entry */
			memset(&key, 0, sizeof (DBT));

			key.data = &snapdata->snapid;
			key.size = sizeof (uint32_t);

			dbp->del(dbp, NULL, &key, 0);

			goto done;
		}

		/* mark metrics available for display */
		snapdata->flags |= METRICS_AVAIL;

		(void) db_update_snapshot(fsent, snapdata, len);
	}

done:

	/* free results and reset the thread-specific data */
	fmtsd->fsent = NULL;
	fmtsd->snapdata = NULL;

	free_metrics_results(&fmtsd->arg, &fmtsd->res);

	fmtsd->arg = NULL;
	fmtsd->res = NULL;

	free(snapdata);

	return (st);

}

int
walk_nftw(
	const char		*filename,
	const struct stat64	*statptr,	/* ARGSUSED */
	int			fileflags,
	struct FTW		*pfwt)
{
	int			st = 0;
	struct sam_stat		sbuf;
	struct sam_stat		sbuf2;
	fm_tsd_t		*fmtsd = fmkey_tsd();
	filvar_t		filvar;
	filinfo_t		filinfo;
	int			media[4] = {0, 0, 0, 0};
	int			i;
	char			buf[MAXPATHLEN +1];


	if (fmtsd == NULL) {
		return (-1);
	}

	/* see if we need to stop */
	(void) pthread_mutex_lock(&(fmtsd->fsent->statlock));
	if (fmtsd->fsent->status == FSENT_DELETING) {
		st = EINTR;
	}
	(void) pthread_mutex_unlock(&(fmtsd->fsent->statlock));

	if (st != 0) {
		return (st);
	}

	/* ignore metadata-only nodes */
	if ((fileflags != FTW_F) && (fileflags != FTW_D) &&
	    (fileflags != FTW_DP)) {
			return (0);
	}

	st = sam_stat(filename, &sbuf, sizeof (struct sam_stat));
	if (st != 0) {
		/* skip any invalid files */
		return (0);
	}

	/*
	 * Skip SAM special files - not tracked in samfsdump, we shouldn't
	 * count them either.
	 */
	if (SS_ISSAMFS(sbuf.attr)) {
		if ((sbuf.st_ino == 1) || (sbuf.st_ino == 4) ||
		    (sbuf.st_ino == 5)) {
			return (0);
		}

		/* make sure this isn't a child of the .archive dir */
		if ((pfwt->level == 1) && (fileflags == FTW_F)) {
			snprintf(buf, sizeof (buf), "%s/..", filename);
			st = sam_stat(buf, &sbuf2, sizeof (struct sam_stat));

			if ((st != 0) || (sbuf2.st_ino == 4)) {
				return (0);
			}
		}
	}

	memset(&filvar, 0, sizeof (filvar_t));
	memset(&filinfo, 0, sizeof (filinfo_t));

	filvar.mtime = filinfo.fuid.mtime = sbuf.st_mtime;
	filvar.atime = sbuf.st_atime;
	filinfo.owner = sbuf.st_uid;
	filinfo.group = sbuf.st_gid;
	/*
	 * some files have a bogus size recorded - -1 on an unsigned int,
	 * so check for this and treat it as if it were 0 length.
	 */
	if (sbuf.st_size == INT64_MAX) {
		sbuf.st_size = 0;
	}

	filinfo.size = filinfo.osize = sbuf.st_size;

	if (SS_ISSAMFS(sbuf.attr)) {
		if (SS_ISOFFLINE(sbuf.attr)) {
			if (SS_ISPARTIAL(sbuf.attr)) {
				filinfo.osize = sbuf.partial_size;
			} else {
				filinfo.osize = 0;
			}
		}

		/* get media information */
		for (i = 0; i < 4; i++) {
			/* no copy? */
			if ((sbuf.copy[i].creation_time == 0) ||
			    (sbuf.copy[i].media[0] == '\0')) {
				continue;
			}

			media[i] = sam_atomedia(sbuf.copy[i].media);
		}
	}

	st = gather_snap_metrics(fmtsd->fsent->fsdb, fmtsd->snapdata, &filinfo,
	    &filvar, &(fmtsd->arg), media, &(fmtsd->res));

	return (st);
}
