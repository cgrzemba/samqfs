/*
 * dat.c - read/write a recycler's dat file.
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
#pragma ident "$Revision: 1.8 $"

static char *_SrcFile = __FILE__;

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/acl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/bitmap.h>

#include "recycler_c.h"
#include "recycler_threads.h"

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <dirent.h>

#include "sam/types.h"
#include "aml/shm.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "aml/device.h"
#include "aml/catlib.h"
#include "sam/names.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/fs/ino.h"
#include "sam/fs/ino_ext.h"
#include "aml/diskvols.h"
#include "aml/sam_rft.h"
#include "aml/id_to_path.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "../../../src/fs/cmd/dump-restore/csd.h"

#include "recycler.h"

extern boolean_t RegenDatfiles;
extern boolean_t InfiniteLoop;
extern MediaTable_t ArchMedia;

static char *errmsg1 = "Error reading dat file";
static char *errmsg2 = "Error writing dat file";

static int readOpen(char *path);
static int readHeader(int fd, char *path);

/*
 * Initialize recycler dat for each samfsdump file.  Generate the dat file
 * name and see if this file was created on a previous run of the recycler.
 */
void
DatInit(
	CsdDir_t *csdDir)
{
	int idx;
	struct stat sb;
	char *filename;
	CsdEntry_t *csdEntry;
	int fd;
	int rval;

	for (idx = 0; idx < csdDir->cd_count; idx++) {
		csdEntry = &csdDir->cd_entry[idx];

		if (csdEntry->ce_skip == B_TRUE) {
			continue;
		}
		filename = csdEntry->ce_path;	/* path to samfsdump file */

		/*
		 * Build path to recycler's dat file for the samfsdump file.
		 */
		SamMalloc(csdEntry->ce_datPath, sizeof (upath_t));
		(void) snprintf(csdEntry->ce_datPath, sizeof (upath_t), "%s%s",
		    filename, DAT_FILE_SUFFIX);

		if (stat(csdEntry->ce_datPath, &sb) == 0) {
			/*
			 * Dat file exists.
			 */

			if (RegenDatfiles == B_TRUE) {
				Trace(TR_MISC,
				    "Regenerating recycler dat file '%s'",
				    csdEntry->ce_datPath);

				(void) unlink(csdEntry->ce_datPath);
				continue;
			}

			/*
			 * Read and validate header.
			 */
			csdEntry->ce_exists = B_FALSE;

			fd = readOpen(csdEntry->ce_datPath);
			if (fd < 0) {
				Trace(TR_MISC,
				    "Failed to open dat file '%s' errno: %d",
				    csdEntry->ce_datPath, errno);
				continue;
			}

			rval = readHeader(fd, csdEntry->ce_datPath);
			if (rval == 0) {
				csdEntry->ce_exists = B_TRUE;
			} else {
				Trace(TR_MISC,
				    "Invalid recycler dat file '%s'",
				    csdEntry->ce_datPath);

				(void) unlink(csdEntry->ce_datPath);
			}
			(void) DatClose(fd);
		}

		/*
		 * Allocate and initialize media table for samfsdump file.
		 */
		SamMalloc(csdEntry->ce_table, sizeof (MediaTable_t));
		(void) memset(csdEntry->ce_table, 0, sizeof (MediaTable_t));

		if (MediaInit(csdEntry->ce_table, csdEntry->ce_path) != 0) {
			Trace(TR_MISC,
			    "Error: media table initialization failed");
			return;
		}
	}
}

/*
 * Accumulate media entries from recycler's dat file.
 */
int
DatAccumulate(
	CsdEntry_t *csd)
{
	size_t size;
	MediaTable_t *dat_table;
	MediaEntry_t *datfile_cache;
	MediaEntry_t *vsn;
	MediaEntry_t *datfile;	/* vsn entry from dat file */
	MediaEntry_t *dat;	/* vsn entry in samfs dump's dat table */
	size_t ngot;
	int i;
	int idx;
	int num_inodes;
	DatTable_t table;
	int rval;
	off_t pos;
	char *path;
	int fd;

	path = csd->ce_datPath;		/* path to dat file */
	fd = readOpen(path);		/* open file descriptor for dat file */
	if (fd < 0) {
		Trace(TR_MISC, "%s '%s', dat file open failed, errno= %d",
		    errmsg1, path, errno);
		return (-1);
	}
	dat_table = csd->ce_table;	/* media table generated for dat file */

	/*
	 * Need to search from the beginning.  Rewind dat file and
	 * the read header again.
	 */
	rval = readHeader(fd, path);
	if (rval != 0) {
		Trace(TR_MISC, "%s '%s', dat table header read failed",
		    errmsg1, path);
		DatClose(fd);
		return (-1);
	}

	num_inodes = 0;

	/*
	 * Read datfile table entries until we find the entry we are
	 * currently processing.
	 */
	while (InfiniteLoop) {
		datfile_cache = NULL;

		ngot = read(fd, &table, sizeof (DatTable_t));
		if (ngot != sizeof (DatTable_t)) {
			Trace(TR_MISC, "%s '%s', dat table read failed",
			    errmsg1, path);
			DatClose(fd);
			return (-1);
		}

		if (table.dt_mapmin != dat_table->mt_mapmin) {
			continue;
		}
		if (table.dt_mapchunk != dat_table->mt_mapchunk) {
			Trace(TR_MISC, "%s '%s', map chunk does not match",
			    errmsg1, path);
			DatClose(fd);
			return (-1);
		}

		pos = lseek(fd, 0, SEEK_CUR);

		Trace(TR_MISC, "[%s] Read dat entries 0x%lx "
		    "count: %d seqnum candidates: %lld-%lld",
		    dat_table->mt_name, pos, table.dt_count, table.dt_mapmin,
		    table.dt_mapmin + table.dt_mapchunk - 1);

		size = table.dt_count * sizeof (MediaEntry_t);
		SamMalloc(datfile_cache, size);
		(void) memset(datfile_cache, 0, size);

		for (i = 0; i < table.dt_count; i++) {
			datfile = &datfile_cache[i];

			ngot = read(fd, datfile, sizeof (MediaEntry_t));
			if (ngot != sizeof (MediaEntry_t)) {
				Trace(TR_MISC, "%s '%s', header read error "
				    "read: %d expected: %d, errno= %d",
				    errmsg1, path,
				    ngot, sizeof (MediaEntry_t), errno);
				num_inodes = -1;
				goto out;
			}

			if ((datfile->me_type == DT_DISK) &&
			    (datfile->me_mapsize != 0)) {
				SamMalloc(datfile->me_bitmap,
				    datfile->me_mapsize);
				(void) memset(datfile->me_bitmap, 0,
				    datfile->me_mapsize);

				ngot = read(fd, datfile->me_bitmap,
				    datfile->me_mapsize);
				if (ngot != datfile->me_mapsize) {
					Trace(TR_MISC,
					    "%s '%s', bitmap read error "
					    "read: %d expected: %d, errno= %d",
					    errmsg1, path, ngot,
					    datfile->me_mapsize, errno);
					num_inodes = -1;
					goto out;
				}
			}

			vsn = MediaFind(&ArchMedia,
			    datfile->me_type, datfile->me_name);
			if (vsn == NULL) {
				Trace(TR_MISC,
				    "%s '%s', failed to find vsn %s.%s",
				    errmsg1, path,
				    sam_mediatoa(datfile->me_type),
				    datfile->me_name);
				num_inodes = -1;
				goto out;
			}

			/*
			 * For completeness, update in-memory's dat_table for
			 * samfs dump file.  This is not really necessary but
			 * might be useful for debugging purposes.
			 */
			dat = NULL;
			if (dat_table != NULL) {
				dat = MediaFind(dat_table, datfile->me_type,
				    datfile->me_name);
				if (dat == NULL) {
					Trace(TR_MISC,
					    "Error failed to find vsn %s.%s",
					    sam_mediatoa(datfile->me_type),
					    datfile->me_name);
					num_inodes = -1;
					goto out;
				}
			}

			Trace(TR_SAMDEV,
			    "[%s.%s] Accumulate dat active files: %d",
			    sam_mediatoa(vsn->me_type), vsn->me_name,
			    datfile->me_files);

			PthreadMutexLock(&vsn->me_mutex);
			vsn->me_files += datfile->me_files;
			PthreadMutexUnlock(&vsn->me_mutex);

			if (dat != NULL) {
				PthreadMutexLock(&dat->me_mutex);
				dat->me_files += datfile->me_files;
				PthreadMutexUnlock(&dat->me_mutex);
			}

			num_inodes += datfile->me_files;

			if ((datfile->me_type == DT_DISK) &&
			    (datfile->me_mapsize != 0)) {

				for (idx = 0;
				    idx <= dat_table->mt_mapchunk; idx++) {

				/* FIXME */
				if (BT_TEST(datfile->me_bitmap, idx) == 1) {
				PthreadMutexLock(&vsn->me_mutex);
				BT_SET(vsn->me_bitmap, idx);
				PthreadMutexUnlock(&vsn->me_mutex);

				if (dat != NULL) {
					PthreadMutexLock(&dat->me_mutex);
					BT_SET(dat->me_bitmap, idx);
					PthreadMutexUnlock(&dat->me_mutex);
				}
				}
				}
			}

			/*
			 * Free memory allocated for bitmap read from disk.
			 */
			if (datfile->me_bitmap != NULL) {
				SamFree(datfile->me_bitmap);
				datfile->me_bitmap = NULL;
			}
		}
		break;
	}

out:
	if (datfile_cache != NULL) {
		SamFree(datfile_cache);
		for (i = 0; i < table.dt_count; i++) {
			datfile = &datfile_cache[i];
			if (datfile->me_bitmap != NULL) {
				SamFree(datfile->me_bitmap);
				datfile->me_bitmap = NULL;
			}
		}
	}
	DatClose(fd);
	return (num_inodes);
}

/*
 * Create dat file.
 */
int
DatCreate(
	char *path)
{
	int fd;

	(void) unlink(path);

	fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0664);
	if (fd == -1) {
		Trace(TR_MISC,
		    "Error: cannot create recycler dat file '%s', errno= %d",
		    path, errno);
	}
	Trace(TR_DEBUG, "Dat create file %d", fd);
	return (fd);
}

/*
 * Open dat file for write.
 */
int
DatWriteOpen(
	char *path)
{
	int fd;

	fd = open(path, O_WRONLY, 0664);
	if (fd == -1) {
		Trace(TR_MISC,
		    "Error: cannot open recycler dat file '%s', errno= %d",
		    path, errno);
	}
	Trace(TR_DEBUG, "Dat write open file %d", fd);
	return (fd);
}

int
DatWriteHeader(
	int fd,
	CsdEntry_t *csd,
	pid_t pid)
{
	size_t num_written;
	DatHeader_t dat;
	char *path;

	path = csd->ce_datPath;		/* path to dat file */

	(void) lseek(fd, 0, SEEK_SET);
	(void) memset(&dat, 0, sizeof (DatHeader_t));
	dat.dh_magic = DAT_FILE_MAGIC;
	dat.dh_version = DAT_FILE_VERSION;
	dat.dh_ctime = time(NULL);
	/*
	 * PID of process that is writing to this dat file.  If 0, then file
	 * is complete.
	 */
	dat.dh_pid = pid;

	num_written = write(fd, &dat, sizeof (DatHeader_t));
	if (num_written != sizeof (DatHeader_t)) {
		Trace(TR_MISC, "%s '%s', header write failed "
		    "wrote %d expected %d, errno= %d",
		    errmsg2, path, num_written, sizeof (DatHeader_t), errno);
		return (-1);
	}
	return (0);
}

int
DatWriteTable(
	int fd,
	CsdEntry_t *csd)
{
	size_t num_written;
	size_t len;
	DatTable_t dat_table;
	int i;
	MediaEntry_t *vsn;
	off_t pos;
	char *path;
	MediaTable_t *vsn_table;

	path = csd->ce_datPath;		/* path to dat file */
	vsn_table = csd->ce_table;	/* media table generated for dat file */

	dat_table.dt_count = vsn_table->mt_tableUsed;
	dat_table.dt_mapchunk = vsn_table->mt_mapchunk;
	dat_table.dt_mapmin = vsn_table->mt_mapmin;

	pos = lseek(fd, 0, SEEK_CUR);

	Trace(TR_MISC, "[%s] Write dat entries 0x%lx "
	    "count: %d seqnum candidates: %lld-%lld",
	    vsn_table->mt_name, pos, dat_table.dt_count, dat_table.dt_mapmin,
	    dat_table.dt_mapmin + dat_table.dt_mapchunk - 1);

	len = sizeof (DatTable_t);
	num_written = write(fd, &dat_table, len);

	if (num_written != len) {
		Trace(TR_MISC, "%s '%s' table write failed "
		    "wrote: %d expected: %d, errno= %d",
		    errmsg1, path, num_written, len, errno);
		return (-1);
	}

	for (i = 0; i < vsn_table->mt_tableUsed; i++) {
		vsn = &vsn_table->mt_data[i];

		len = sizeof (MediaEntry_t);
		num_written = write(fd, vsn, len);

		if (num_written != len) {
			Trace(TR_MISC, "%s '%s' table write failed "
			    "wrote: %d expected: %d, errno= %d",
			    errmsg1, path, num_written, len, errno);
			return (-1);
		}

		if ((vsn->me_type == DT_DISK) && (vsn->me_bitmap != NULL)) {
			len = vsn->me_mapsize;
			num_written = write(fd, vsn->me_bitmap, len);

			if (num_written != len) {
				Trace(TR_MISC, "%s '%s' table write failed "
				    "wrote: %d expected: %d, errno= %d",
				    errmsg1, path, num_written, len, errno);
				return (-1);
			}
		}
	}
	return (0);
}

/*
 * Close dat file.
 */
void
DatClose(
	int fd)
{
	Trace(TR_DEBUG, "Dat close %d", fd);
	close(fd);
}

static int
readOpen(
	char *path)
{
	int fd;

	fd = open(path, O_RDONLY, 0664);
	if (fd == -1) {
		Trace(TR_MISC, "Error: cannot open recycler dat file '%s'",
		    path);
	}
	Trace(TR_DEBUG, "Dat read open %d", fd);
	return (fd);
}

static int
readHeader(
	int fd,
	char *path)
{
	int ngot;
	DatHeader_t header;

	if (fd < 0) {
		return (-1);
	}

	(void) lseek(fd, 0, SEEK_SET);

	ngot = read(fd, &header, sizeof (DatHeader_t));
	if (ngot != sizeof (DatHeader_t)) {
		Trace(TR_MISC,
		    "%s '%s', header read error, got %d expected %d, errno= %d",
		    errmsg1, path, ngot, sizeof (DatHeader_t), errno);
		return (-1);
	}

	if (header.dh_magic != DAT_FILE_MAGIC) {
		Trace(TR_MISC, "%s '%s', file is not in dat format",
		    errmsg1, path);
		return (-1);
	}

	if (header.dh_version != DAT_FILE_VERSION) {
		Trace(TR_MISC,
		    "%s '%s', header version mismatch, got %d expected %d",
		    errmsg1, path, header.dh_version, DAT_FILE_VERSION);
		return (-1);
	}

	if (header.dh_pid != 0) {
		Trace(TR_MISC, "%s '%s', file is not complete, pid= %d",
		    errmsg1, path, (int)header.dh_pid);
		return (-1);
	}

	return (0);
}
