/*
 * csd.c - read and accumulate archive copy data for csd (samfs) dump file.
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
#pragma ident "$Revision: 1.11 $"

static char *_SrcFile = __FILE__;

#include <sys/types.h>
#include <sys/sysmacros.h>
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
#include <ctype.h>
#include <errno.h>

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
#include "aml/tar_hdr.h"
#include "aml/id_to_path.h"
#include "sam/custmsg.h"
#include "sam/exit.h"
#include "../../../src/fs/cmd/dump-restore/csd.h"

#include "recycler.h"

extern boolean_t CannotRecycle;
extern MediaTable_t ArchMedia;

static char *errmsg1 = "Error reading samfs dump header";
static char *errmsg2 = "Corrupt samfs dump file";

static CsdDir_t *csdDir;

static int readDumpHeader(char *filename, CsdFildes_t **dump_fildes,
	csd_hdrx_t *dump_header);
static int readFileHeader(CsdFildes_t *fildes, int version,
	csd_fhdr_t *header);
static int readInode(CsdFildes_t *fildes, int namelen, char *name,
	struct sam_perm_inode *perm_inode, boolean_t *skip);
static int readExtInode(CsdFildes_t *fildes, struct sam_perm_inode *perm_inode,
	struct sam_vsn_section **vsnpp, char *link,
	void *data, int *n_aclp, aclent_t **aclpp);
static int readCheck(CsdFildes_t *fildes, void *buffer, size_t size);

static void walkDirectory(DIR *dirp, char *dirname);
static char *readDirectory(DIR *dirp, char *path, boolean_t *isdir);

static void initCsdTable();

static CsdFildes_t *openFildes(char *path);
static int readFildes(CsdFildes_t *fildes, void *buffer, size_t size);
static void closeFildes(CsdFildes_t *fildes);

static void traceInode(char *name, struct sam_perm_inode *perm_inode);

#define	CSDTMAGIC "ustar"

typedef struct csd_tar {
	longlong_t	csdt_bytes;	/* file bytes to follow */
	char		*csdt_name;	/* name */
	struct sp_list	*csdt_sparse;	/* sparse list */
} csd_tar_t;

static void readTarHeader(CsdFildes_t *fildes, csd_tar_t *hdr_info);
static void skipEmbeddedData(CsdFildes_t *fildes);
static int skipData(CsdFildes_t *fildes, u_longlong_t nbytes);
static longlong_t oct2ll(char *cp, int numch);
static longlong_t str2ll(char *cp, int numch);


/*
 * Initialize samfsdump file by walking the specified directory and
 * validating the contents.  Each file in the directory must be a
 * valid samfsdump file or a recycler dat file.
 */
CsdDir_t *
CsdInit(
	char *dirname)
{
	DIR *dirp;
	int idx;
	struct stat sb;
	upath_t datPath;

	csdDir = NULL;

	dirp = opendir(dirname);
	if (dirp == NULL) {
		Trace(TR_ERR,
		    "Error: cannot open SAM-QFS dump directory '%s' errno = %d",
		    dirname, errno);
		return (NULL);
	}

	initCsdTable();

	walkDirectory(dirp, dirname);

	for (idx = 0; idx < csdDir->cd_count; idx++) {
		char *filename;
		CsdEntry_t *fsdump;

		fsdump = &csdDir->cd_entry[idx];
		filename = fsdump->ce_path;

		/*
		 * Skip dat files.
		 */
		if (strstr(filename, DAT_FILE_SUFFIX) != NULL) {
			continue;
		}

		(void) snprintf(datPath, sizeof (datPath), "%s%s",
		    filename, DAT_FILE_SUFFIX);

		if (stat(datPath, &sb) == 0) {
			Trace(TR_MISC,
			    "Recycler dat file found for csd file '%s'",
			    filename);
		} else {

			if (readDumpHeader(filename, NULL, NULL) == -1) {
				char *p;

				/*
				 * Failed to read and validate dump file header.
				 * If hidden file, files that begin with a dot,
				 * skip it.  If not, generate error message and
				 * stop recycling.
				 */

				/* Find last occurrence of '/' */
				p = strrchr(filename, '/');
				if (p != NULL) {
					/* If char following '/' is dot */
					if (p[1] == '.') {
						fsdump->ce_skip = B_TRUE;
						Trace(TR_MISC,
						    "Skip hidden file '%s'",
						    filename);
						continue;
					}
				}

				Trace(TR_MISC,
				    "Error: invalid samfs dump file '%s'",
				    filename);
				Log(20406, filename);
				CANNOT_RECYCLE();
				break;
			}
		}
	}

	if (CannotRecycle == B_TRUE) {
		SamFree(csdDir);
		csdDir = NULL;
	}

	return (csdDir);
}

void
CsdSetSeqnum(
	CsdTable_t *fsdump_table,
	DiskVolumeSeqnum_t min,
	boolean_t diskArchive)
{
	int i;
	int j;
	CsdDir_t *fsdump_dir;
	CsdEntry_t *fsdump;

	for (i = 0; i < fsdump_table->ct_count; i++) {
		fsdump_dir = fsdump_table->ct_data[i];
		if (fsdump_dir == NULL) {
			Trace(TR_MISC,
			    "Error: samfs dump scan initialization invalid");
			abort();
		}

		for (j = 0; j < fsdump_dir->cd_count; j++) {
			fsdump = &fsdump_dir->cd_entry[j];
			if (fsdump->ce_skip == B_TRUE) {
				continue;
			}
			MediaSetSeqnum(fsdump->ce_table, min, diskArchive);
		}
	}
}

void
CsdScan(
	Crew_t *crew,
	CsdTable_t *fsdump_table,
	int num_dumps,
	int pass,
	void *(*worker)(void *arg))
{
	int i;
	int j;
	CsdDir_t *fsdump_dir;
	int status;
	int work_idx;
	ScanArgs_t *scanarg_arr;
	WorkItem_t *request_arr;

	SamMalloc(request_arr, num_dumps * sizeof (WorkItem_t));
	(void) memset(request_arr, 0, num_dumps * sizeof (WorkItem_t));

	SamMalloc(scanarg_arr, num_dumps * sizeof (ScanArgs_t));
	(void) memset(scanarg_arr, 0, num_dumps * sizeof (ScanArgs_t));

	work_idx = 0;

	for (i = 0; i < fsdump_table->ct_count; i++) {

		fsdump_dir = fsdump_table->ct_data[i];
		if (fsdump_dir == NULL) {
			Trace(TR_MISC,
			    "Error: samfs dump scan initialization invalid");
			abort();
		}

		for (j = 0; j < fsdump_dir->cd_count; j++) {
			WorkItem_t *request;
			ScanArgs_t *arg;
			CsdEntry_t *fsdump;

			fsdump = &fsdump_dir->cd_entry[j];
			if (fsdump->ce_skip == B_TRUE) {
				continue;
			}
			arg = &scanarg_arr[work_idx];
			request = &request_arr[work_idx];

			arg->data = (void *)fsdump;
			arg->pass = pass;

			request->wi_func = worker;
			request->wi_arg = (void *)arg;

			PthreadMutexLock(&crew->cr_mutex);
			if (crew->cr_first == NULL) {
				crew->cr_first = request;
				crew->cr_last = request;
			} else {
				crew->cr_last->wi_next = request;
				crew->cr_last = request;
			}
			crew->cr_count++;

			status = pthread_cond_signal(&crew->cr_go);
			if (status != 0) {
				Trace(TR_MISC,
				    "Error: pthread_cond_signal failed %d",
				    errno);
				abort();
			}
			PthreadMutexUnlock(&crew->cr_mutex);

			work_idx++;
		}
	}

	/*
	 * Wait for scans to finish.
	 */
	while (crew->cr_count > 0) {
		status = pthread_cond_wait(&crew->cr_done, &crew->cr_mutex);
		if (status != 0) {
			Trace(TR_MISC, "Error: pthread_cond_wait failed %d",
			    errno);
			abort();
		}
	}
	PthreadMutexUnlock(&crew->cr_mutex);

	SamFree(request_arr);
	SamFree(scanarg_arr);
}

void *
CsdAccumulate(
	void *arg)
{
	csd_hdrx_t dump_header;
	csd_fhdr_t file_header;
	int rval;
	int version;
	int ngot;
	int size;
	int namelen;
	union sam_di_ino inode;
	boolean_t skip;
	struct sam_vsn_section *vsnp;
	void *data;
	int n_acls;
	aclent_t *aclp;
	char link[MAXPATHLEN + 1];
	char name[MAXPATHLEN + 1];
	int num_inodes;
	int tot_inodes;
	CsdEntry_t *fsdump;
	ScanArgs_t *scan_arg;
	int pass;
	time_t start_time;
	time_t scan_time;
	char *path;
	char *mt_name;
	int fd;
	CsdFildes_t *fildes;

	scan_arg = (ScanArgs_t *)arg;

	fsdump = (CsdEntry_t *)scan_arg->data;
	pass = scan_arg->pass;

	num_inodes = 0;
	tot_inodes = 0;

	start_time = time(NULL);

	mt_name = fsdump->ce_table->mt_name;

	if (fsdump->ce_exists == B_TRUE) {

		Trace(TR_MISC, "[%s] Accumulate samfs dat file", mt_name);
		num_inodes = DatAccumulate(fsdump);

		if (num_inodes == -1) {
			fsdump->ce_exists = B_FALSE;
		} else {
			scan_time = time(NULL) - start_time;
			Trace(TR_MISC,
			    "[%s] End samfs dat file, "
			    "%d inodes accumulated in %ld secs",
			    mt_name, num_inodes, scan_time);
			return (NULL);
		}
	}

	ASSERT(fsdump->ce_table != NULL);
	path = fsdump->ce_path;
	Trace(TR_MISC, "[%s] Accumulate samfs dump", mt_name);

	if (readDumpHeader(path, &fildes, &dump_header) == -1) {
		CANNOT_RECYCLE();
		return (NULL);
	}

	version = dump_header.csd_header.version;
	switch (version) {

	case CSD_VERS_5:
		/*
		 * Read remainder of extended header.
		 */
		size = sizeof (csd_hdrx_t) - sizeof (csd_hdr_t);
		ngot = readFildes(fildes,
		    (char *)((char *)(&dump_header) +
		    sizeof (csd_hdr_t)), size);

		if ((ngot != size) ||
		    (dump_header.csd_header_magic != CSD_MAGIC)) {
			/*
			 * Header record read error.
			 */
			Trace(TR_MISC, "%s '%s', header record read error",
			    errmsg1, path);
			CANNOT_RECYCLE();
			goto out;
		}
		break;

	case CSD_VERS_4:
		break;

	default:
		Trace(TR_MISC, "%s '%s',  bad version number %d",
		    errmsg1, path, version);
		CANNOT_RECYCLE();
		goto out;
	}

	/*
	 * Read the dump file.
	 */
	while (readFileHeader(fildes, version, &file_header) == 0) {

		namelen = file_header.namelen;
		rval = readInode(fildes, namelen, name, &inode.inode, &skip);
		if (rval == -1) {
			Trace(TR_MISC,
			    "Error: samfs dump read inode failed '%s'", path);
			CANNOT_RECYCLE();
			goto out;
		}

		if (*TraceFlags & (1 << TR_debug)) {
			traceInode(name, &inode.inode);
		}

		if (S_ISSEGI(&inode.inode.di) || S_ISSEGS(&inode.inode.di)) {
			Trace(TR_MISC,
			    "Error: segmented file '%s' read failed", name);
			CANNOT_RECYCLE();
			goto out;
		}

		data = NULL;
		vsnp = NULL;
		aclp = NULL;
		n_acls = 0;

		if (S_ISREQ(inode.inode.di.mode)) {
			SamMalloc(data, inode.inode.di.psize.rmfile);
		}

		rval = readExtInode(fildes, &inode.inode, &vsnp, link,
		    data, &n_acls, &aclp);
		if (rval == -1) {
			CANNOT_RECYCLE();
			goto out;
		}

		if (skip == B_TRUE) {
			goto skip_file;
		}

		num_inodes++;
		tot_inodes++;

		if (num_inodes >= INODES_IN_PROGRESS) {
			Trace(TR_MISC, "...accumulated %d inodes for '%s'",
			    tot_inodes, path);
			num_inodes = 0;
		}

		if (inode.inode.di.arch_status != 0 &&
		    ! (S_ISEXT(inode.inode.di.mode))) {

			if (S_ISREQ(inode.inode.di.mode)) {
				CANNOT_RECYCLE();

			} else {
				if (FsInodeHandle(NULL, fsdump, &inode,
				    pass) == -1) {
					CANNOT_RECYCLE();
				}
			}
		}
		if (file_header.flags & CSD_FH_DATA) {
			skipEmbeddedData(fildes);
		}
skip_file:
		if (data != NULL) {
			SamFree(data);
		}

		if (vsnp != NULL) {
			SamFree(vsnp);
		}

		if (aclp != NULL) {
			SamFree(aclp);
		}
	}

out:
	(void) closeFildes(fildes);
	scan_time = time(NULL) - start_time;
	Trace(TR_MISC, "[%s] End samfs dump, %d inodes accumulated in %ld secs",
	    path, tot_inodes, scan_time);

	/*
	 * Upon successful reading of the samfs dump file generated
	 * the dat file.
	 */
	if (CannotRecycle == B_TRUE) {
		Trace(TR_MISC,
		    "Cannot recycle. No recycler dat file '%s' written",
		    fsdump->ce_datPath);
	} else {
		Trace(TR_MISC, "Generating recycler dat file '%s'",
		    fsdump->ce_datPath);

		fd = DatCreate(fsdump->ce_datPath);
		if (fd >= 0) {
			rval = DatWriteHeader(fd, fsdump, getpid());
			if (rval == 0) {
				rval = DatWriteTable(fd, fsdump);
			}
			DatClose(fd);
			if (rval == -1) {
				Trace(TR_MISC,
				    "Error: write to dat file '%s' failed",
				    fsdump->ce_datPath);
				(void) unlink(fsdump->ce_datPath);
			}
		}
	}

	return (NULL);
}

void
CsdAssembleName(
	char *path,
	char d_name[1],
	char *buf,
	/* LINTED argument unused in function */
	size_t buflen)
{
	(void) strcpy(buf, path);
	(void) strcat(buf, "/");
	(void) strcat(buf, d_name);
}

void
CsdCleanup(
	CsdTable_t *fsdump_table)
{
	int i;
	int j;
	CsdDir_t *fsdump_dir;

	for (i = 0; i < fsdump_table->ct_count; i++) {

		fsdump_dir = fsdump_table->ct_data[i];
		if (fsdump_dir == NULL) {
			Trace(TR_MISC,
			    "Error: samfs dump cleanup initialization invalid");
			abort();
		}

		for (j = 0; j < fsdump_dir->cd_count; j++) {
			int rval;
			int fd;
			CsdEntry_t *fsdump;

			fsdump = &fsdump_dir->cd_entry[j];
			if (fsdump->ce_skip == B_TRUE) {
				continue;
			}

			if (fsdump->ce_exists == B_TRUE) {
				continue;
			}

			rval = -1;
			fd = DatWriteOpen(fsdump->ce_datPath);

			if (fd >= 0) {
				rval = DatWriteHeader(fd, fsdump, 0);
				if (rval == 0) {
					DatClose(fd);
				}
			}

			if (rval == -1) {
				Trace(TR_MISC,
				    "Error: write to dat file '%s' failed",
				    fsdump->ce_datPath);
				(void) unlink(fsdump->ce_datPath);
			}
		}
	}
}

static int
readFileHeader(
	CsdFildes_t *fildes,
	int version,
	csd_fhdr_t *header)
{
	int namelen;
	int corrupt_header = 0;

	if (version <= CSD_VERS_3) {
		Trace(TR_MISC, "Error: unknown samfs dump file version %d",
		    version);
		return (-1);
	}

	if (version < CSD_VERS_5) {
		if (readFildes(fildes, &namelen,
		    sizeof (int)) == sizeof (int)) {
			header->magic = CSD_FMAGIC;
			header->flags = 0;
			header->namelen = (long)namelen;
		} else {
			return (-1);
		}
	} else if (version == CSD_VERS_5) {
		if (readFildes(fildes, header,
		    sizeof (csd_fhdr_t)) != sizeof (csd_fhdr_t)) {
			return (-1);
		}
		/* some minor validation */
		if (header->magic != CSD_FMAGIC) {
			corrupt_header++;
		} else if (header->flags & CSD_FH_PAD) {
			if (header->namelen) {
				if (skipData(fildes, header->namelen)) {
					Trace(TR_MISC, "skip pad data failed ");
					return (-1);
				}
			}
			return (0);
		} else if (header->namelen <= 0) {
			corrupt_header++;
		}
		if (corrupt_header) {
			Trace(TR_MISC,
			    "Error:  samfsdump file header error <0x%x/%d>",
			    header->magic, header->namelen);
			return (-1);
		}
	} else {
		Trace(TR_MISC, "Error: unknown samfs dump file version %d",
		    version);
		return (-1);
	}
	return (0);
}

static int
readInode(
	CsdFildes_t *fildes,
	int namelen,
	char *name,
	struct sam_perm_inode *perm_inode,
	boolean_t *skip)
{
	size_t ngot;

	*skip = B_FALSE;

	/*
	 * Read file name
	 */
	if (namelen <= 0 || namelen > MAXPATHLEN) {
		Trace(TR_MISC, "%s, invalid name length %d",
		    errmsg2, namelen);
		return (-1);
	}

	ngot = readFildes(fildes, name, namelen);
	if (ngot != namelen && ngot != (size_t)-1) {
		Trace(TR_MISC, "%s, read name failed", errmsg2);
		return (-1);
	}
	name[namelen] = '\0';

	/*
	 * Read perm inode.
	 */
	ngot = readFildes(fildes, perm_inode, sizeof (struct sam_perm_inode));
	if (ngot != sizeof (struct sam_perm_inode) && ngot != (size_t)-1) {
		Trace(TR_MISC, "%s, read failed", errmsg2);
		return (-1);
	}

	if (!(SAM_CHECK_INODE_VERSION(perm_inode->di.version))) {
		*skip = B_TRUE;
	}
	return (0);
}

/*
 * If removable media file, read the resource record.
 * If symlink, read the link name.
 * If archive copies overflowed, read vsn sections.
 * If access control list, read entries.
 */
static int
readExtInode(
	CsdFildes_t *fildes,
	struct sam_perm_inode *perm_inode,
	/* LINTED argument unused in function */
	struct sam_vsn_section **vsnpp,
	char *link,
	void *data,
	int *n_aclp,
	aclent_t **aclpp)
{
	int rval;
	int linklen;

	if (S_ISREQ(perm_inode->di.mode)) {
		struct sam_resource_file *resource;

		resource = (struct sam_resource_file *)data;
		rval = readCheck(fildes, resource, perm_inode->di.psize.rmfile);
		if (rval == -1) {
			Trace(TR_MISC,
			    "Error: removable media attrib read failed");
			return (-1);
		}
	}

	if (S_ISLNK(perm_inode->di.mode)) {
		rval = readCheck(fildes, &linklen, sizeof (int));
		if (rval == -1) {
			Trace(TR_MISC, "Error: symlink length read failed");
			return (-1);
		}

		if (linklen < 0 || linklen > MAXPATHLEN) {
			Trace(TR_MISC, "Error: invalid symlink name length");
			return (-1);
		}
		rval = readCheck(fildes, link, linklen);
		if (rval == -1) {
			Trace(TR_MISC, "Error: symlink name read failed");
			return (-1);
		}
		link[linklen] = '\0';
	}

	if (!S_ISLNK(perm_inode->di.mode)) {
		*aclpp = NULL;
		if (perm_inode->di.status.b.acl) {
			rval = readCheck(fildes, n_aclp, sizeof (int));
			if (rval == -1) {
				Trace(TR_MISC, "Error: acl numof read failed");
				return (-1);
			}

			if (*n_aclp) {
				SamMalloc(*aclpp, *n_aclp * sizeof (sam_acl_t));
				rval = readCheck(fildes, *aclpp,
				    *n_aclp * sizeof (sam_acl_t));
				if (rval == -1) {
					Trace(TR_MISC,
					    "Error: acl attrib read failed");
					return (-1);
				}
			}
		}
	}
	return (0);
}

static int
readDumpHeader(
	char *filename,
	CsdFildes_t **dump_fildes,
	csd_hdrx_t *dump_header)
{
	int ngot;
	csd_hdrx_t header;
	CsdFildes_t *fildes;
	int rval;

	rval = 0;

	fildes = openFildes(filename);
	if (fildes == NULL) {
		rval = -1;
		goto out;
	}

	fildes->offset = 0LL;
	ngot = readFildes(fildes, &header, sizeof (csd_hdr_t));

	if (ngot != sizeof (csd_hdr_t)) {
		struct stat sb;

		Trace(TR_MISC, "%s '%s', dump header record error "
		    "(expected %d got %d)",
		    errmsg1, filename, sizeof (csd_hdr_t), ngot);

		if (stat(filename, &sb) == 0) {
			/* Zero length samfs dump file. */
			Log(20415, filename);
		} else {
			Trace(TR_MISC, "\tstat failed errno: %d", errno);
		}

		rval = -1;
		goto out;
	}

	if (header.csd_header.magic != CSD_MAGIC) {
		if (header.csd_header.magic != CSD_MAGIC_RE) {
			/* Volume is not in dump format. */
			Trace(TR_MISC, "%s '%s', file is not in dump format",
			    errmsg1, filename);
			rval = -1;
			goto out;
		} else {
			/* Volume is not in dump format. */
			Trace(TR_MISC,
			    "%s '%s', dump was generated on opposite "
			    "endian machine",
			    errmsg1, filename);
			rval = -1;
			goto out;
		}
	}

out:
	if (rval == -1 || dump_fildes == NULL) {
		if (fildes != NULL) {
			(void) closeFildes(fildes);
		}
		fildes = NULL;
	}

	if (dump_fildes != NULL) {
		*dump_fildes = fildes;
	}

	if (dump_header != NULL) {
		(void) memcpy(dump_header, &header, sizeof (csd_hdrx_t));
	}
	return (rval);
}

static int
readCheck(
	CsdFildes_t *fildes,
	void *buffer,
	size_t size)
{
	size_t ngot;

	ngot = readFildes(fildes, buffer, size);
	if ((ngot != size) && (ngot != (size_t)-1)) {
		Trace(TR_ERR,
		    "Corrupt samfs dump file, read(%d) returned %d bytes",
		    size, ngot);
	} else if (ngot == (size_t)-1) {
		Trace(TR_ERR,
		    "Corrupt samfs dump file, read failed errno %d", errno);
	}
	return (ngot);
}

static void
skipEmbeddedData(
CsdFildes_t *fildes)
{
	csd_tar_t	header;
	u_longlong_t to_block;
	u_longlong_t nb;

	readTarHeader(fildes, &header);
	nb = header.csdt_bytes;
	to_block = roundup(fildes->offset, TAR_RECORDSIZE) - fildes->offset;
	if (nb > to_block) {
		/*
		 * data doesn't fit in current block,
		 * starts on tar block boundary.
		 */
		(void) skipData(fildes, to_block);
	}
	(void) skipData(fildes, nb);	/* skip file data */
	nb = roundup(fildes->offset, TAR_RECORDSIZE) - fildes->offset;
	(void) skipData(fildes, nb);	/* skip to next 512 boundary */
}

static int
skipData(
CsdFildes_t *fildes,
u_longlong_t nbytes)
{
	u_longlong_t skipped, size;
	char skipbuf[CSD_DEFAULT_BUFSZ];

	while (nbytes > 0) {
		errno = 0;
		size =
		    (nbytes > CSD_DEFAULT_BUFSZ) ? CSD_DEFAULT_BUFSZ : nbytes;
		skipped = gzread(fildes->inf, skipbuf, size);
		if (skipped <= 0) {
			Trace(TR_MISC,
			    "Error: gzread of samfsdump file, %s",
			    strerror(errno));
			return (errno);
		}
		fildes->offset += skipped;
		nbytes -= skipped;
	}
	return (0);
}

static void
readTarHeader(
CsdFildes_t *fildes,
csd_tar_t *hdr_info)
{
	union header_blk tarhdrblk;
	char		tar_name[MAXPATHLEN+1];
	struct header	*tarhdr = (struct header *)&tarhdrblk;
	longlong_t	field_value;
	int		linktype;
	int		name_set = 0;
	int		namelen;

	hdr_info->csdt_bytes = 0;
	for (;;) {
		if (((gzread(fildes->inf, &tarhdrblk,
		    sizeof (tarhdrblk))) !=
		    TAR_RECORDSIZE) ||
		    (memcmp(tarhdr->magic, CSDTMAGIC,
		    sizeof (CSDTMAGIC)-1) != 0)) {
			goto header_err;
		}
		fildes->offset += sizeof (tarhdrblk);
		linktype = tarhdr->linkflag;

		switch (linktype) {
		case LF_LONGNAME:	/* looks like a file containing name */
			field_value = oct2ll(tarhdr->size, 12);
			if (field_value > MAXPATHLEN || name_set != 0) {
				goto header_err;
			}

			namelen = (int)field_value;
			if (gzread(fildes->inf, tar_name, namelen) !=
			    namelen) {
				goto header_err;
			}
			fildes->offset += namelen;
			tar_name[namelen] = '\0';
			hdr_info->csdt_name = tar_name;
			name_set++;
			break;
		case LF_NORMAL:
		case LF_PARTIAL:
		case LF_OLDNORMAL:
			hdr_info->csdt_bytes = str2ll(tarhdr->size, 12);
			/* currently not dealing with GNU extra header */
			hdr_info->csdt_sparse = NULL;
			if (name_set == 0) {
				strncpy(tar_name, tarhdr->arch_name, NAMSIZ);
				hdr_info->csdt_name = tar_name;
				name_set++;
			}
			return;
		default:
			goto header_err;
		}
	}

header_err:
	/*	Some error in the header has occurred. */
	Trace(TR_ERR,
	    "Corrupt samfs dump file, file tar header error.");
	abort();
	/* NOTREACHED */
}

static longlong_t
oct2ll(char *cp, int numch)
{
	longlong_t	result;
	int		tmp;

	result = 0;
	while (numch > 0) {
		tmp = *cp;
		cp++; numch--;
		if (!isspace(tmp)) {
			tmp -= (int)'0';
			if (tmp >= 0 && tmp < 8) {
				result = (result * 8) + tmp;
			} else {
				return (result);
			}
		} else {
			if (result != 0) {
				return (result);
			}
		}
	}
	return (result);
}

static longlong_t
str2ll(char *cp, int numch)
{
	longlong_t	result;
	int		digit;
	int		tmp;
	int		base = 8;

	if (*cp == 'x') {
		base = 16;
		cp++;
		numch--;
	}
	result = 0;
	while (numch > 0) {
		tmp = *cp;
		cp++; numch--;
		if (!isspace(tmp)) {
			digit = tmp - (int)'0';
			if (digit > base && base > 10) {
				if (tmp >= (int)'a' && tmp <= (int)'z') {
					tmp -= (int)'a' - (int)'A';
				}
				digit = tmp + 10 - (int)'A';
			}
			if (digit >= 0 && digit < base) {
				result = (result * (longlong_t)base) +
				    (longlong_t)digit;
			} else {
				return (result);
			}
		} else {
			if (result != 0) {
				return (result);
			}
		}
	}
	return (result);
}

static void
walkDirectory(
	DIR *c_dirp,
	char *c_dirname)
{
	char n_dirname[128];
	DIR *n_dirp;
	boolean_t isdir;
	char *name;
	int idx;

	while ((name = readDirectory(c_dirp, c_dirname, &isdir)) != NULL) {

		if (isdir == B_TRUE) {
			(void) strcpy(n_dirname, name);
			n_dirp = opendir(n_dirname);
			walkDirectory(n_dirp, n_dirname);

		} else {
			idx = csdDir->cd_count;
			/*
			 * Skip dat files.
			 */
			if (strstr(name, DAT_FILE_SUFFIX) == NULL) {

				Trace(TR_DEBUG, "%d: 0x%x '%s'",
				    idx, (int)&csdDir->cd_entry[idx], name);

				/* Add dump file to table. */
				csdDir->cd_entry[idx].ce_path = strdup(name);
				csdDir->cd_count++;

				/* Check if need to increase csd table size. */
				if (csdDir->cd_count >= csdDir->cd_alloc) {
					initCsdTable();
				}
			}
		}
		free(name);
	}
}

char *
readDirectory(
	DIR *dirp,
	char *path,
	boolean_t *isdir)
{
	char tmpname[128];
	struct dirent *dp;
	char *name = NULL;
	struct stat sb;

	while ((dp = readdir(dirp)) != NULL) {
		if ((dp->d_name[0] == '.' && dp->d_name[1] == '\0') ||
		    (dp->d_name[0] == '.' && dp->d_name[1] == '.' &&
		    dp->d_name[2] == '\0')) {
			continue;
		}

		if (dp != NULL) {
			CsdAssembleName(path, dp->d_name, tmpname,
			    sizeof (tmpname));
			(void) stat(tmpname, &sb);
			name = strdup(tmpname);
			*isdir = S_ISDIR(sb.st_mode);
			break;
		}
	}
	return (name);
}

/*
 *  Initialize or increase size of the csd table.
 */
static void
initCsdTable()
{
	int i;
	size_t size;

	if (csdDir == NULL) {
		size = sizeof (CsdDir_t) +
		    (CSD_TABLE_INCREMENT * sizeof (CsdEntry_t));
		SamMalloc(csdDir, size);
		Trace(TR_DEBUG, "Alloc csd table: 0x%x %d",
		    (int)csdDir, size);
		(void) memset(csdDir, 0, size);
		csdDir->cd_alloc = CSD_TABLE_INCREMENT;
	} else {
		/* Increase table size. */
		csdDir->cd_alloc += CSD_TABLE_INCREMENT;
		i = csdDir->cd_count;

		size = sizeof (CsdDir_t) +
		    (csdDir->cd_alloc * sizeof (CsdEntry_t));
		SamRealloc(csdDir, size);
		Trace(TR_DEBUG, "Realloc csd table: 0x%x %d",
		    (int)csdDir, size);
		(void) memset(&csdDir->cd_entry[i], 0,
		    CSD_TABLE_INCREMENT * sizeof (CsdEntry_t));
	}
}

/*
 * Open csd file.
 */
static CsdFildes_t *
openFildes(
	char *path)
{
	CsdFildes_t *fildes;
	int	fd;

	SamMalloc(fildes, sizeof (CsdFildes_t));
	(void) memset(fildes, 0, sizeof (CsdFildes_t));

	fildes->flags = CSD_inflate;

	fd = open64(path, O_RDONLY);
	if (fd == NULL) {
		Trace(TR_MISC, "Cannot open samfs dump '%s': %s",
		    path, strerror(errno));
		return (NULL);
	}
	fildes->inf = gzdopen(fd, "r");
	if (fildes->inf == NULL) {
		Trace(TR_MISC, "Cannot gzdopen samfs dump '%s': %s",
		    path, strerror(errno));
		return (NULL);
	}

	Trace(TR_DEBUG, "Csd open %x %d", (int)fildes->inf, fd);

	return (fildes);
}

/*
 * Read csd file.
 */
static int
readFildes(
	CsdFildes_t *fildes,
	void *buffer,
	size_t size)
{
	int ngot;

	ngot = gzread(fildes->inf, buffer, size);
	fildes->offset += ngot;

	return (ngot);
}

/*
 * Close csd file.
 */
static void
closeFildes(
	CsdFildes_t *fildes)
{
	if (fildes != NULL) {
		if (fildes->inf != NULL) {
			gzclose(fildes->inf);
		}
		SamFree(fildes);
	}
	Trace(TR_DEBUG, "Csd close %x", (int)fildes->inf);
}

/*
 * Trace an inode.
 */
static void
traceInode(
	char *name,
	struct sam_perm_inode *perm_inode)
{
	char type;

	if (S_ISDIR(perm_inode->di.mode))	type = 'd';
	else if (S_ISSEGI(&perm_inode->di))	type = 'I';
	else if (S_ISSEGS(&perm_inode->di))	type = 'S';
	else if (S_ISREQ(perm_inode->di.mode))	type = 'R';
	else if (S_ISLNK(perm_inode->di.mode))	type = 'l';
	else if (S_ISREG(perm_inode->di.mode))	type = 'f';
	else if (S_ISBLK(perm_inode->di.mode))	type = 'b';
	else type = '?';

	Trace(TR_DEBUG, "%c %s", type, name);
}
