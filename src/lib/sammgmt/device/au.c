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
#pragma ident	"$Revision: 1.51 $"

static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

/*
 * disk slice/volume discovery of non-zero size /dev/[md|vx]/rdsk/...
 */

/* Solaris header files */
#include <stdio.h>
#include <stdlib.h>
#include <alloca.h>
#include <unistd.h>
#include <string.h>
#include <limits.h> // PATH_MAX
#include <fcntl.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mnttab.h>
#include <sys/types.h>
#include <sys/vtoc.h>
#include <sys/vfstab.h>
#include <errno.h>
#include <sys/efi_partition.h>
#include "efilabel.h"

/*  other header files */
#include "pub/mgmt/device.h"
#include "pub/mgmt/error.h"
#include "pub/mgmt/sqm_list.h"
#include "mgmt/config/common.h"
#include "mgmt/config/media.h"
#include "mgmt/util.h"
#include "sam/sam_trace.h"

/* device paths */
#define	SLICE_RPATH	"/dev/rdsk"
#define	SVM_RPATH	"/dev/md/rdsk"
#define	VXVM_RPATH	"/dev/vx/rdsk"
#define	ZVOL_RPATH	"/dev/zvol/rdsk"
#define	ROOTDG_DIR	"/dev/vx/rdsk/rootdg"
#define	VXVM_PATH_LEN	12

/* command paths */
#define	VXDISK_CMD	"/usr/sbin/vxdisk -e list"
#define	VXPRINT_CMD	"/usr/sbin/vxprint -hq"
#define	METASTAT_CMD	"/usr/sbin/metastat -p"
#define	METASTAT_S_CMD	"/usr/sbin/metastat -p -s"
#define	METADB_CMD	"/usr/sbin/metadb"
#define	SCDIDADM_CMD	"/usr/cluster/bin/scdidadm -L -o name -o host"
#define	ZVOL_CMD	"/usr/sbin/zfs list -H -t volume"

/* filter for raw slice (prefix may be missing) */
#define	MSTAT_FILTER1	" | /usr/bin/awk '{ for (i = 2; i <= NF; i++) \
				if ($i ~ /^.*c.*/) print $i }'"
/* filter for did slice (prefix may be missing) */
#define	MSTAT_FILTER2	" | /usr/bin/awk '{ for (i = 2; i <= NF; i++) \
				if ($i ~ /^.*d[0-9]+s[0-9]+/) print $i }'"

#define	PREF		"/dev/dsk"
#define	PREF_MD		"/dev/md"
#define	PREFDID		"/dev/did/dsk"
#define	PREFRDID	"/dev/did/rdsk"
#define	PREFZVOL	"/dev/zvol/dsk"
#define	PREFMAXLEN	strlen(PREFRDID)

#define	INUSE	"*"

// ------------------ private declarations -------------------------------

static int walkdir(const char *dir, au_type_t autype, sqm_lst_t *lst);
static int checkslice(const char *slice, au_type_t autype, dsize_t *capacity);
int comp_fsdata(void *fsdata1, void *fsdata2);
static void free_fsdata_lst(sqm_lst_t *lst);
static int checkmcf(sqm_lst_t *lst);
static int checkvfstab(sqm_lst_t *vfstablst);
static int checkmnttab(const char *device);
static sqm_lst_t *get_svm_slices(sqm_lst_t *svmslices);
static sqm_lst_t *get_metadb_slices(sqm_lst_t *metadbslices);
static sqm_lst_t *get_vxvm_slices(sqm_lst_t *vxvmslices);
static sqm_lst_t *get_zvol_slices(sqm_lst_t *zvoldevs);
static offset_t getzvolsize(const char *zvol);
static int addfsinfo(sqm_lst_t *aulst, sqm_lst_t *lsts[]);
static au_t *au_dup(const au_t *au, au_t *newau);
static int mark_inuse_aus(sqm_lst_t *allaus);
static void get_avail_aus_from_list(sqm_lst_t *avail);
static boolean_t metadbok();
static void add_svm_raid_info(sqm_lst_t *aus);
static void add_vxvm_raid_info(sqm_lst_t *aus);
static int slice_overlaps(char *slice);
static scsi_info_t *get_scsi_info_for_au(char *au_rpath);
// next function used by unit-test code
int discover_aus_by_type(const au_type_t type, sqm_lst_t **paus);

static sqm_lst_t *get_did_dsks(sqm_lst_t *hosts, sqm_lst_t **dids);
static int check_did_slices(sqm_lst_t *dids, sqm_lst_t **paus);
static sqm_lst_t *get_diskset_slices(sqm_lst_t *aus);
static sqm_lst_t *get_slices_from_stream(sqm_lst_t *slices,
	FILE *res_stream, char *fsinfo);

typedef struct fsdata {
	char *path;	/* /dev[md|vx|did|zvol]/dsk...  */
	char *fsinfo;	/* either fs type or family set name (for SAM/QFS) */
} fsdata_t;

static boolean_t vxvm_avail;
static boolean_t zvol_avail;
static sqm_lst_t *dsets = NULL;

// ------------------------ PUBLIC (API) FUNCTIONS --------------------------

int
discover_avail_aus(
ctx_t *ctx /* ARGSUSED */,
sqm_lst_t **pavail)
{

	int res;

	Trace(TR_MISC, "discover available AU-s");
	if (ISNULL(pavail)) {
		Trace(TR_ERR, "AU discovery failed - NULL argument");
		return (-1);
	}
	if (-1 == (res = discover_aus(ctx, pavail)))
		return (res);
	get_avail_aus_from_list(*pavail);
	Trace(TR_MISC, "%d AU-s available", (*pavail)->length);
	return (0);
}


int
discover_avail_aus_by_type(
ctx_t *ctx /* ARGSUSED */,
const au_type_t type,
sqm_lst_t **pavail)
{

	int res;

	if (ISNULL(pavail)) {
		Trace(TR_ERR, "type %d AU discovery failed - NULL argument",
		    type);
		return (-1);
	}
	if (-1 == (res = discover_aus_by_type(type, pavail)))
		return (res);
	get_avail_aus_from_list(*pavail);
	return (0);
}


int
discover_aus(
ctx_t *ctx /* ARGSUSED */,
sqm_lst_t **plst)	/* discovered AU-s are put in this list */
{

	sqm_lst_t *lst = lst_create();
	sqm_lst_t *lst1, *lst2, *lst3, *lst4;
	int ok, res = 0;

	if (NULL == lst || ISNULL(plst))
		return (-1);
	Trace(TR_MISC, "discover AU-s");

	*plst = lst;
	discover_aus_by_type(AU_SLICE, &lst1);
	discover_aus_by_type(AU_SVM,   &lst2);
	discover_aus_by_type(AU_VXVM,  &lst3);
	discover_aus_by_type(AU_ZVOL,  &lst4);
	// concatenate all four lists and put the result in lst
	if (-1 != lst_concat(lst3, lst4))
		if (-1 != lst_concat(lst2, lst3))
			if (-1 != lst_concat(lst1, lst2))
				if (-1 != lst_concat(lst, lst1))
					ok = 1;
	if (!ok) {
		Trace(TR_ERR, "AU discovery failed - concat err");
		return (-1);
	}
	// mark the aus that are in use
	res = mark_inuse_aus(lst);

	Trace(TR_MISC, "%d AU-s discovered", lst->length);
	return (res);
}


/*
 * result will include a subset of the input - slices that overlap
 * with 1/more other slices.
 * return code:
 *  0 = no overlaps (result will be an empty list).
 * -1 = an error occured
 * >0 = number of user-specified slices that overlap some other slices
 */
int
check_slices_for_overlaps(ctx_t *ctx /*ARGSUSED*/,
sqm_lst_t *slices,		/* each slice is a /dev[/did]/dsk/... string */
sqm_lst_t **result		/* overlapping slices */
)
{
	sqm_lst_t *ovrlapslices;
	node_t *node;
	char *slice;

	if (ISNULL(slices) || ISNULL(result))
		return (-1);
	ovrlapslices = *result = lst_create();
	node = slices->head;
	while (NULL != node) {
		slice = (char *)node->data;
		if (1 == slice_overlaps(slice))
			lst_append(ovrlapslices, strdup(slice));
		node = node->next;
	}

	return (ovrlapslices->length);
}


int
discover_ha_aus(ctx_t *ctx /* ARGSUSED */,
sqm_lst_t *hosts,
boolean_t avail,
sqm_lst_t **paus)
{
	sqm_lst_t *dids;
	int res = -1;

	Trace(TR_MISC, "discover available HA AU-s");
	if (ISNULL(paus)) {
		Trace(TR_ERR, "HA AU discovery failed - NULL argument");
		return (-1);
	}
	if (NULL == get_did_dsks(hosts, &dids))
		return (res);
	res = check_did_slices(dids, paus);
	if (res != -1)
		get_diskset_slices(*paus);
	if (avail) {
		if (-1 != mark_inuse_aus(*paus))
			get_avail_aus_from_list(*paus);
	}

	lst_free_deep(dids);

	Trace(TR_MISC, "%d HA AU-s available", (*paus)->length);
	return (res);
}

// ------------------------- NON-API FUNCTIONS ------------------------------

int
discover_aus_by_type(
const au_type_t type,	/* type of the AU */
sqm_lst_t **paus)
{

	int res;
	sqm_lst_t *aus = lst_create();
	struct stat buf;

	if (NULL == aus || ISNULL(paus))
		return (-1);
	*paus = aus;
	Trace(TR_MISC, "discover type %d AU-s", type);

	switch (type) {
	case AU_SLICE:
		if (-1 == (res = walkdir(SLICE_RPATH, type, aus))) {
			samerrno = SE_AU_DISCOVERY_FAILED;
			strlcpy(samerrmsg, GetCustMsg(SE_AU_DISCOVERY_FAILED),
			    MAX_MSG_LEN);
			return (res);
		}
		break;
	case AU_SVM:
		if (metadbok()) {
			if (-1 == (res = walkdir(SVM_RPATH, type, aus)))
				return (res);
			add_svm_raid_info(aus);
		} else {
			Trace(TR_MISC, "No metadbs found");
			return (0);
		}
		break;
	case AU_VXVM:
		vxvm_avail = B_TRUE;
		if (-1 == (res = walkdir(VXVM_RPATH, type, aus))) {
			vxvm_avail = B_FALSE;
			Trace(TR_MISC, "VxVM device dir not found - %s",
			    VXVM_RPATH);
			return (-1);
		}
		add_vxvm_raid_info(aus);
		break;
	case AU_ZVOL:
		zvol_avail = B_TRUE;

		/*
		 * if ZVOL_RPATH doesn't exist, don't look for zvols.
		 */

		if (stat(ZVOL_RPATH, &buf) < 0) {
			Trace(TR_MISC, "zvol device dir not found - %s",
			    ZVOL_RPATH);
			zvol_avail = B_FALSE;
			return (-1);
		} else if (! S_ISDIR(buf.st_mode)) {
			Trace(TR_MISC, "zvol device dir not found - %s",
			    ZVOL_RPATH);
			zvol_avail = B_FALSE;
			return (-1);
		}

		if (-1 == (res = walkdir(ZVOL_RPATH, type, aus))) {
			samerrno = SE_AU_DISCOVERY_FAILED;
			strlcpy(samerrmsg, GetCustMsg(SE_AU_DISCOVERY_FAILED),
			    MAX_MSG_LEN);
			return (res);
		}
		break;
	default:
		samerrno = SE_INVALID_AU_TYPE;
		strlcpy(samerrmsg, GetCustMsg(SE_INVALID_AU_TYPE),
		    MAX_MSG_LEN);
	} // type
	Trace(TR_MISC, "%d type %d AU-s found", aus->length, type);
	return (0);
}


char *
au2str(
au_t au,	/* the AU to be converted to a string */
char *au_str,	/* put the result here */
boolean_t info)	/* if B_TRUE then include fsinfo in the result */
{

	scsi_info_t *scsi = au.scsiinfo;
	sprintf(au_str,
	    info ? "%-50s%c%10.2fMB %-6s %-4s %-8s%-16s%-4s %-32s"
	    :"%-50s%c%10.2fMB %s %-4s %-8s%-16s%-4s %-32s",
	    au.path,
	    strstr(&au.path[strlen(au.path) - 2], "s2") ? '!' : ' ',
	    (float)au.capacity / MEGA,
	    (au.fsinfo == NULL) ? " " : au.fsinfo,
	    (au.type == AU_SLICE) ? "n/a" : ((au.raid == NULL) ? "-" : au.raid),
	    (scsi == NULL) ? "n/a" : scsi->vendor,
	    (scsi == NULL) ? "n/a" : scsi->prod_id,
	    (scsi == NULL) ? "n/a" : scsi->rev_level,
	    (scsi == NULL) ? "n/a" : Str(scsi->dev_id));
	return (au_str);
}


int
rdsk2dsk(
const char *rawpath,	/* convert this path */
char *dskpath)		/* the result is put here */
{

	size_t len = strlen(rawpath), idx;
	char *str2;

	if ((str2 = (char *)strstr(rawpath, "rdsk")) == NULL)
		return (-1);
	idx = len - strlen(str2);
	memcpy(dskpath, rawpath, idx);
	dskpath[idx] = '\0';
	strcat(dskpath, &rawpath[idx+1]);
	return (0);
}


/*
 * called internally by the get_fs() API, in order to populate scsiinfo field
 */
int
add_scsi_info(
au_t *au)
{
	char *rdsk;
	char *path;
	upath_t pathbuf;
	char *beg;

	if (au->type != AU_SLICE) {
		return (0); // skip volumes
	}

	/*
	 * Special case for global devices which don't seem to
	 * like the scsi inquiry- convert to a did device.
	 */
	if ((beg = strstr(au->path, "/dev/global/")) != NULL) {
		strlcpy(pathbuf, "/dev/did/", sizeof (upath_t));
		beg += 12;
		strlcat(pathbuf, beg, sizeof (upath_t));
		path = pathbuf;
	} else {
		path = au->path;
	}

	if (-1 == dsk2rdsk(path, &rdsk)) {
		return (-1);
	}



	au->scsiinfo = get_scsi_info_for_au(rdsk);
	free(rdsk);

	return (0);
}


/* return NULL if cannot allocate au */
static au_t *
create_au_elem(
const char *rawpath,	/* raw disk/volume path */
const au_type_t type,	/* AU type */
const dsize_t capacity, /* AU capacity */
const int inuse)  /* if non-zero, mark this AU as INUSE */
{

	au_t *aup = (au_t *)mallocer(sizeof (au_t));

	if (NULL == aup)
		return (NULL);
	if (rdsk2dsk(rawpath, aup->path)) { /* if error */
		free(aup);
		return (NULL);
	}

	aup->capacity = capacity;
	aup->type = type;
	// printf("inuse:%d cap:%llu\n", inuse, capacity); // debug

	if (0 != inuse)
		strcpy(aup->fsinfo, INUSE);
	else
		strcpy(aup->fsinfo, "");
	aup->raid = NULL;
	aup->scsiinfo = NULL;
	return (aup);
}


/* return a pointer to newly created au ('newau') */
static au_t *
au_dup(
const au_t *au,	/* the AU to be duplicated */
au_t *newau)	/* the clone is put here */
{
	if (NULL == au)
		return (NULL);
	if (au->path)
		strcpy(newau->path, au->path);
	newau->type = au->type;
	newau->capacity = au->capacity;
	if (au->fsinfo)
		strcpy(newau->fsinfo, au->fsinfo);
	newau->raid = (au->raid == NULL) ? NULL : strdup(au->raid);
	if (NULL != au->scsiinfo) {
		if (NULL ==
		    (newau->scsiinfo =
		    (scsi_info_t *)mallocer(sizeof (scsi_info_t))))
			return (NULL);
		strcpy(newau->scsiinfo->vendor, au->scsiinfo->vendor);
		strcpy(newau->scsiinfo->prod_id, au->scsiinfo->prod_id);
		strcpy(newau->scsiinfo->rev_level, au->scsiinfo->rev_level);
		newau->scsiinfo->dev_id = strdup(au->scsiinfo->dev_id);
	} else
		newau->scsiinfo = NULL;
	return (newau);
}


/* walk the specified dev directory and call checkslice() for each entry */
static int
walkdir(
const char *dir,	/* walk this directory */
au_type_t autype,	/* type of this AU */
sqm_lst_t *lst)		/* append available AU-s to this list */
{

	dsize_t capacity = 0;
	char rslice[MAXPATHLEN+1];
	DIR *dp;
	dirent64_t *dirp;
	dirent64_t *dirpp;
	int status;
	struct stat buf;
	au_t *au;

	Trace(TR_MISC, "analyzing %s", Str(dir));
	if ((dp = opendir(dir)) == NULL) {
		Trace(TR_ERR, "failed to open dir %s", Str(dir));
		return (-1);
	}

	dirp = mallocer(sizeof (dirent64_t) + MAXPATHLEN + 1);
	if (dirp == NULL) {
		return (-1);
	}

	while ((readdir64_r(dp, dirp, &dirpp)) == 0) {
		if (dirpp == NULL) {
			break;
		}

		if ((strcmp(dirp->d_name, ".") == 0) ||
		    (strcmp(dirp->d_name, "..") == 0)) {
			continue;
		}

		snprintf(rslice, sizeof (rslice), "%s/%s", dir, dirp->d_name);

		/* if in /dev/vx/rdsk then check for subdirs (diskgroups) */
		if (0 == strcmp(dir, VXVM_RPATH)) {
			if (stat(rslice, &buf) < 0) {
				continue;
			} else {
				/*
				 * According to VxVM documentation, in the case
				 * of the rootdg group, slices from the parent
				 * directory should be used rather than those
				 * under rootdg/.
				 * Therefore we do not look under rootdg.
				 */
				if ((S_ISDIR(buf.st_mode)) &&
				    (strcmp(rslice, ROOTDG_DIR) != 0)) {
					walkdir(rslice, autype, lst);
					continue;
				}
			}
		}

		/* if in /dev/zvol/rdsk, check subdirs for zvols */
		if (0 == strcmp(dir, ZVOL_RPATH)) {
			if (stat(rslice, &buf) < 0) {
				continue;
			} else {
				if (S_ISDIR(buf.st_mode)) {
					walkdir(rslice, autype, lst);
					continue;
				}
			}
		}

		if ((status = checkslice(rslice, autype, &capacity)) >= 0) {
			// printf(">%-50s %10.2fMB\n",
			// slice, (float)capacity / MEGA);
			au = create_au_elem(rslice, autype, capacity,
			    status);
			/* check au for NULL? */
			if (AU_SLICE == autype) {
				au->scsiinfo = get_scsi_info_for_au(rslice);
			}
			if (-1 == lst_append(lst, au)) {
				closedir(dp);
				free(dirp);
				return (-1);
			}
		}
	}
	closedir(dp);
	free(dirp);
	return (0);
}


static int
checkEFIslice(const char *slice, int fd, dsize_t *capacity)
{

	dk_gpt_t *efi;
	struct dk_part part;
	int slice_idx, part_idx;
	const int EFI_RESERVED_SLICE = 7;

	Trace(TR_DEBUG, "checking efi slice %s\n", slice);
	slice_idx = call_efi_alloc_and_read(fd, &efi);
	close(fd);

	// check if error reading label
	if (slice_idx < 0) {
		call_efi_free(efi);
		Trace(TR_ERR, "cannot find VTOC or EFI label");
		return (-1);
	}

	// if reserved slice then skip
	if (EFI_RESERVED_SLICE == slice_idx) {
		call_efi_free(efi);
		return (2);
	}

	if ((fd = open(slice, O_RDWR | O_EXCL | O_NDELAY)) < 0) {
		call_efi_free(efi);
		return (1);
	}
	for (part_idx = 0; part_idx < efi->efi_nparts; part_idx++)
		if (slice_idx == part_idx) {
			part = efi->efi_parts[part_idx];
			*capacity = (diskaddr_t)part.p_size * efi->efi_lbasize;
			if (*capacity == 0) // if size zero then skip
				return (-1);
			// printf("capacity:%lld\n", *capacity);
		}
	call_efi_free(efi);
	return (0);
}

/* this holds the name of the last slice for which open() returned EIO */
static char lastioerrslice[MAXPATHLEN + 1] = "";

/*
 * return
 *  -1 if open/read_vtoc fails for the specified device file, or empty dev.
 *   1 if device cannot be open in exclusive mode (probably mounted)
 *   2 if slice starts at sector 0 or stores an EFI label
 *  -2 if this is a 'p' or s8-s15 device (x86 only)
 *   0 otherwise
 * argument must include the full path
 */
static int
checkslice(
const char *slice,	/* check this slice */
au_type_t autype,	/* type of this AU */
dsize_t *capacity)	/* put its size in here */
{

	int slice_idx, fd;
	struct vtoc toc;
	struct partition part;
	int status = 0;
	size_t len;

	Trace(TR_DEBUG, "checkslice called for %s", slice);

	if (ISNULL(slice, capacity)) {
		return (-1);
	}

	/*
	 * zvols aren't slices, so we use other means to
	 * determine their size, and whether they are mounted.
	 */

	if (autype == AU_ZVOL) {
		if (checkmnttab(slice) == 0) {
			status = 1; /* the slice is mounted */
		} else {
			status = 0;
		}
		*capacity = getzvolsize(slice);
		return (status);
	}

	len = strlen(slice);
	if (len < 3) {
		return (-1);
	}

	if (autype == AU_SLICE) {
		if (slice[len - 2] == 'p' || // px
		    slice[len - 2] == 'd' || // dx
		    slice[len - 1] == '8' || // x8
		    slice[len - 1] == '9' || // x9
		    slice[len - 3] == 's') { // sxx
			Trace(TR_ERR, "%s skipped", slice);
			return (-2);
		}
	}

	/*
	 * if previously received IOE on a slice of the same disk
	 * then skip this slice (unless this is a SVM md device)
	 */
	if (slice[len - 2] == 's') {
		if (0 == strncmp(slice, lastioerrslice, (len - 1))) {
			Trace(TR_ERR, "%s skipped", slice);
			return (-1);
		}
	}
	if ((fd = open(slice, O_RDONLY | O_NDELAY)) < 0) {
		Trace(TR_ERR, "cannot open %s for rd, errno=%d:%s", slice,
		    errno, strerror(errno));
		return (-1);
	}

	if ((slice_idx = read_vtoc(fd, &toc)) < 0) {
		/* look for an EFI label */
		if (VT_ENOTSUP == slice_idx) {
			if (is_efi_present()) {
				status = checkEFIslice(slice, fd, capacity);
				close(fd);
				return (status);
			} else {
				Trace(TR_ERR, "efi library not found");
				close(fd);
				return (-1);
			}
		}
		strlcpy(lastioerrslice, slice, sizeof (lastioerrslice));
		close(fd);
		return (-1);
	}
	close(fd);

	if (slice_idx > (toc.v_nparts)) {
		Trace(TR_ERR, "Invalid slice index %d", slice_idx);
		return (-1);
	}

	part = toc.v_part[slice_idx];
	*capacity = (dsize_t)part.p_size * toc.v_sectorsz;

	/*
	 * check that nobody else is currently using this slice.
	 * Volume managers and Oracle keep the slice open.
	 */
	if ((fd = open(slice, O_RDWR | O_EXCL)) < 0) {
		status = 1;
		Trace(TR_ERR, "cannot open %s for rw, errno=%d:%s", slice,
		    errno, strerror(errno));
	} else {
		close(fd);
	}

	if (*capacity == 0) {
		status = -1;
	}

	Trace(TR_DEBUG, "capacity = %llukb, status=%d", *capacity, status);
	return (status);
}


/*
 * returns: false if 'metadb' returns non-zero => can skip SVM discovery
 *	    true otherwise
 */
static boolean_t
metadbok(void)
{

	int status; // for metadb command
	pid_t pid;
	int res;

	Trace(TR_DEBUG, "checking metadb");
	if ((pid = fork()) < 0) {
		Trace(TR_OPRMSG, "cannot check metadb - fork() failed");
		return (B_TRUE);
	}
	if (pid == 0) {
		// child
		Trace(TR_DEBUG, "exec-ing metadb");
		close(1); close(2); // supress stdout and stderr
		res = execl("/usr/sbin/metadb", "metadb", (char *)0);
		Trace(TR_ERR, "exec metadb returned %d", res);
		exit(1);
	}
	Trace(TR_DEBUG, "child's pid is %ld", pid);

	// parent
	if ((pid = waitpid(pid, &status, 0)) < 0)
		return (B_TRUE);
	Trace(TR_DEBUG, "pid=%ld status=%d", pid, status);
	if (status) // if metadb returns non-zero value
		return (B_FALSE);
	return (B_TRUE);
}

#ifdef TEST
static void
printaulst(const sqm_lst_t *lst) {

	node_t *nodep;
	nodep = lst->head;

	printf("> len:%d  head:%d tail:%d\n", lst->length, nodep, lst->tail);
	while (nodep != NULL) {
		printf("> %s\tnxt:%d\n",
		    (char *)(((au_t *)nodep->data)->path),
		    nodep->next);
		nodep = nodep->next;
	}
}

static void
printfslst(const sqm_lst_t *lst) {

	node_t *nodep;
	nodep = lst->head;

	printf("> len:%d  head:%d tail:%d\n", lst->length, nodep, lst->tail);
	while (nodep != NULL) {
		printf("> %s|\t%s\tnxt:%d\n",
		    (char *)(((fsdata_t *)nodep->data)->path),
		    (char *)(((fsdata_t *)nodep->data)->fsinfo),
		    nodep->next);
		nodep = nodep->next;
	}
}
#endif

/* used for sorting lists of au_t */
int
comp_au(void *au1, void *au2) {
	return (strcmp(((au_t *)au1)->path,
	    ((au_t *)au2)->path));
}


/* used for sorting lists of fsdata_t */
int
comp_fsdata(void *fsdata1, void *fsdata2) {
	return (strcmp(((fsdata_t *)fsdata1)->path,
	    ((fsdata_t *)fsdata2)->path));
}

void
free_fsdata_lst(sqm_lst_t *lst) {

	node_t *n;
	fsdata_t *fsd;

	if (NULL == lst)
		return;
	n = lst->head;
	while (n) {
		fsd = (fsdata_t *)n->data;
		free(fsd->path);
		free(fsd->fsinfo);
		free(fsd);
		n = n->next;
	}
	lst_free(lst);
}


static int
mark_inuse_aus(sqm_lst_t *allaus) {

	sqm_lst_t *info[10];
	sqm_lst_t *lst2, *lst3;

	/* check vfstab */
	checkvfstab(lst2 = lst_create());
	if (NULL == (info[0] = lst2))
		return (-1);

	/* check mcf */
	checkmcf(lst3 = lst_create());

	/* check SVM, VxVM and ZVOLS */
	if (NULL == (info[1] = lst3) ||
	    NULL == (info[2] = get_metadb_slices(lst_create())) ||
	    NULL == (info[3] = get_svm_slices(lst_create())) ||
	    NULL == (info[4] = get_vxvm_slices(lst_create())) ||
	    NULL == (info[5] = get_zvol_slices(lst_create()))) {
		Trace(TR_ERR, "AU discovery failed - null pointer");
		return (-1);
	}
	info[6] = NULL;

	return (addfsinfo(allaus, info));
}


/*
 * add the fs info from vfstab & mcf & also the slice info gathered from SVM &
 * VxVM, and ZFS for each au record in au_lst.
 * return 0 if success or -1 if error
 */
static int
addfsinfo(
sqm_lst_t *au_lst, /* sorted list with all >0 AU-s in the system */
sqm_lst_t *lsts[])  /* NULL-term. array of lists providing AU info */
{

	node_t *crtau,	/* current element in au_lst */
	    *crtfsd;	/* current element in info list */
	int i = 1,	/* index in the array of lists */
	    same_au_matches = 0, /* no. of matches (info elems) for crt au */
	    comp_res;
	size_t len;
	sqm_lst_t *info;
	au_t *aup, *newaup;
	fsdata_t *fsd;
	char fsinfostr[128];	/* same size as au_t->fsinfo */

	Trace(TR_DEBUG, "adding fsinfo");
	if (ISNULL(au_lst))
		return (-1);
	if (!au_lst->length)
		return (-1);

	lst_sort(au_lst, comp_au); /* already sorted but not the same way */
	crtau = au_lst->head;

	/* concat all lists */
	i = 1;
	while (lsts[i])
		if (-1 == lst_concat(lsts[0], lsts[i++]))
			return (-1);
	info = lsts[0];
	/* sort the resulting list */
	lst_sort(info, comp_fsdata);

	// printaulst(au_lst); printfslst(info); // debug

	/* now traverse au_lst and info in parallel */
	same_au_matches = 0;
	crtfsd = info->head;
	while (crtau && crtfsd) {
		aup = (au_t *)crtau->data;
		fsd = (fsdata_t *)crtfsd->data;
		comp_res = strcmp(aup->path, fsd->path);
		if (!comp_res) {

			/* if this is not the first match */
			if (same_au_matches++) {
				newaup = (au_t *)mallocer(sizeof (au_t));
				if (NULL == newaup)
					return (-1);
				au_dup(aup, newaup);
				if (-1 == lst_ins_after(au_lst, crtau, newaup))
					return (-1);
				aup = newaup;
			}
			len = strlen(aup->fsinfo);
			strlcpy(fsinfostr, fsd->fsinfo, sizeof (fsinfostr));
			if (INUSE[0] == aup->fsinfo[len - 1]) {
				strlcat(fsinfostr, INUSE, sizeof (fsinfostr));
			}
			strcpy(aup->fsinfo, fsinfostr);
			crtfsd = crtfsd->next;
			continue;
		} else
			if (comp_res < 0) {
				crtau = crtau->next;
				same_au_matches = 0;
			} else
				crtfsd = crtfsd->next;
	} // while
	free_fsdata_lst(info);
	Trace(TR_DEBUG, "done adding fsinfo");
	return (0);
}


/* return 0 if success, -1 if error */
static int
checkvfstab(
sqm_lst_t *vfstablst)	/* append all devices in vfstab to this list */
{

	struct vfstab *vp;
	FILE *vfsfile = fopen(VFSTAB, "r");
	fsdata_t *fsdata;
	int res = 0;

	if (vfsfile == NULL)
		return (-1);

	vp = (struct vfstab *)mallocer(sizeof (struct vfstab));
	if (NULL == vp) {
		fclose(vfsfile);
		return (-1);
	}

#ifdef TEST
	printf("\nIn use (based on info from %s):\n"
	    "----------------------------------------\n", VFSTAB);
#endif
	while ((res = getvfsent(vfsfile, vp)) == 0) {
		if (0 == strncmp(vp->vfs_special, "/dev/", 5)) {
#ifdef TEST
			printf("%s\t %s\n", vp->vfs_special, vp->vfs_fstype);
#endif
			fsdata = (fsdata_t *)mallocer(sizeof (fsdata_t));
			if (NULL == fsdata) {
				res = -1;
				break;
			}
			fsdata->path = (char *)strdup(vp->vfs_special);
			fsdata->fsinfo = (char *)strdup(vp->vfs_fstype);
			if (-1 == lst_append(vfstablst, fsdata)) {
				res = -1;
				break;
			}
		}
	}
	if (res > 0)
		Trace(TR_ERR, "error %d reading %s", res, VFSTAB);
	fclose(vfsfile);
	free(vp);
	return (res);
}

/*
 * Check if a device is in the mnttab. This function is
 * used for checking for mounted zvols that aren't in the
 * /etc/vfstab.
 */


static int
checkmnttab(const char *device) {

	char	*bdev;
	int	ret;
	struct	mnttab	mref, mget;
	FILE	*mntfile;

	Trace(TR_DEBUG, "checking mnttab");

	bdev = (char *)mallocer(strlen(device));
	if (NULL == bdev) {
		return (-1);
	}

	rdsk2dsk(device, bdev);

	mntnull(&mref);
	mref.mnt_special = bdev;

	if ((mntfile = fopen(MNTTAB, "r")) == NULL) {
		free(bdev);
		return (-1);
	}

	ret = getmntany(mntfile, &mget, &mref);
	(void) fclose(mntfile);
	free(bdev);

	Trace(TR_OPRMSG, "done checking /etc/mnttab");
	/* return 0, if the device is found the the mnmttab */
	return (ret);
}

static int
checkmcf(
sqm_lst_t *lst)	/* append all mcf devices to this list */
{

	FILE *res_stream;
	upath_t path;
	uname_t fsinfo;
	fsdata_t *fsd = NULL;
	char cmd[] = "/usr/bin/awk '/^\\/dev/ && /dsk/ {print $1, $4}' "MCF_CFG;
	int status, res = 0;
	pid_t pid;
	char line[256], *tmp;
	int num;
	char *pathp;
	upath_t	gpath;

	Trace(TR_DEBUG, "get mcf slices");
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		Trace(TR_ERR, "no mcf slices processed");
		return (-1);
	}
#ifdef TEST
	printf("mcf information:\n");
#endif
	while (NULL != fgets(line, 256, res_stream)) {
		num = sscanf(line, "%s %s", path, fsinfo);
		if (num != 2) {
			if (num == 1) {
				/* ok to not specify the fsname? */
				strcpy(fsinfo, "n/a");
			} else {
				/* bad input line */
				continue;
			}
		}
#ifdef TEST
		printf("path=%s fsinfo=%s\n", path, fsinfo);
#endif

		tmp = strstr(path, "/global/");
		if (tmp == NULL) {
			pathp = path;
		} else {
			// replace "global" with "did" (for dev. matching)
			gpath[0] = '\0';

			if (tmp > path) {
				strlcpy(gpath, path, PTRDIFF(tmp, path) + 1);
			}
			strlcat(gpath, "/did/", sizeof (gpath));
			tmp += strlen("/global/");
			strlcat(gpath, tmp, sizeof (gpath));
			pathp = gpath;
		}

		fsd = (fsdata_t *)mallocer(sizeof (fsdata_t));
		if (NULL == fsd) {
			res = -1;
			break;
		}
		fsd->path = (char *)mallocer(strlen(pathp) + 1);
		fsd->fsinfo = (char *)mallocer(strlen(fsinfo) + 1);
		if (NULL == fsd->path || NULL == fsd->fsinfo) {
			res = -1;
			break;
		}
		strcpy(fsd->path, pathp);
		strcpy(fsd->fsinfo, fsinfo);
		if (-1 == lst_append(lst, fsd)) {
			res = -1;
			break;
		}
		fsd = NULL;
	}
	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_OPRMSG, "done checking mcf slices");

	if (fsd != NULL) {
		if (fsd->fsinfo != NULL) {
			free(fsd->fsinfo);
		}
		if (fsd->path != NULL) {
			free(fsd->path);
		}
		free(fsd);
	}

	return (res);
}


/* build a list of slices based on the information read from a FILE */
static sqm_lst_t *
get_slices_from_stream(
sqm_lst_t *slices,
FILE *res_stream, /*  one slice name (e.g. [/dev/dsk/][cxty]dzsw) per line */
char *fsinfo)
{
	upath_t path;
	fsdata_t *fsd;
	sqm_lst_t *res = slices;
	size_t	len;

	while (NULL != fgets(path, sizeof (path), res_stream)) {
		fsd = (fsdata_t *)mallocer(sizeof (fsdata_t));
		if (NULL == fsd) {
			res = NULL;
			break;
		}
		len = PREFMAXLEN + strlen(path) + 2;
		fsd->path = (char *)mallocer(len);
		if (NULL == fsd->path) {
			free(fsd);
			res = NULL;
			break;
		}
		fsd->fsinfo = (char *)strdup(fsinfo);
		if (NULL == fsd->fsinfo) {
			free(fsd->path);
			free(fsd);
			res = NULL;
			break;
		}

		/* init this so strlcat later succeeds */
		fsd->path[0] = '\0';

		// determine the prefix that needs to be added (if any)
		if (path[0] == '/') {
			fsd->path[0] = '\0';
			// if slice used by svm, rdsk -> dsk
			if (0 == strncmp(fsinfo, "SVM", 3)) {
				rdsk2dsk(path, path);
			}
		} else {
			if (0 == strcmp(fsinfo, "SVMDS")) {
				strlcpy(fsd->path, PREFDID, len);
			} else {
				strlcpy(fsd->path, PREF, len);
			}
			strlcat(fsd->path, "/", len);
		}
		// add slice name to prefix
		strlcat(fsd->path, path, len);

		if (-1 == lst_append(slices, fsd)) {
			res = NULL;
			free(fsd->path);
			free(fsd);
			break;
		}
	}
	fclose(res_stream);
	return (res);
}



/* return the list of slices used by SVM volumes */
static sqm_lst_t *
get_svm_slices(
sqm_lst_t *svmslices)	/* append SVM slices to this list */
{

	FILE *res_stream;
	char fsinfo[6] = "SVM";
	sqm_lst_t *res = svmslices;
	char cmd[256] = METASTAT_CMD MSTAT_FILTER1;
	pid_t pid;
	int status;
	node_t *dsetnode;
	char *tmpstr = NULL;

	// get slices used by vols that do not belong to disksets
	Trace(TR_DEBUG, "get svm slices");
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		Trace(TR_OPRMSG, "no SVM slices processed");
		return (res);
	}
	res = get_slices_from_stream(svmslices, res_stream, fsinfo);
	waitpid(pid, &status, 0);

	// get slices used by vols that belong to disksets
	tmpstr = lst2str(dsets, " ");
	Trace(TR_DEBUG, "get svm slices used by disksets: %s",
	    Str(tmpstr));
	free(tmpstr);

	if (dsets != NULL) {
		strcpy(fsinfo, "SVMDS");
		dsetnode = dsets->head;
		while (dsetnode != NULL) {
			snprintf(cmd, sizeof (cmd), "%s %s %s", METASTAT_S_CMD,
			    Str(dsetnode->data), MSTAT_FILTER2);
			Trace(TR_DEBUG, "get svm slices used by diskset %s",
			    Str(dsetnode->data));
			if (-1 == (pid =
			    exec_get_output(cmd, &res_stream, NULL))) {
				Trace(TR_ERR, "cannot process diskset");
				continue;
			}
			res = get_slices_from_stream(svmslices,
			    res_stream, fsinfo);
			waitpid(pid, &status, 0);
			dsetnode = dsetnode->next;
		}
	}
	Trace(TR_OPRMSG, "SVM slices processed");
	return (res);
}


/* return the metadb slices */
static sqm_lst_t *
get_metadb_slices(
sqm_lst_t *mdbslices)	/* append metadb slices to this list */
{

	FILE *res_stream;
	char fsinfo[] = "mdb";
	char cmd[] = METADB_CMD
	    " | /usr/bin/awk -F\\t '$6 ~ /dev/ {print $6}'";
	sqm_lst_t *res = mdbslices;
	int status;
	pid_t pid;

	Trace(TR_DEBUG, "get metadb slices");
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		Trace(TR_OPRMSG, "no metadb slices processed");
		return (res);
	}
	res =
	    get_slices_from_stream(mdbslices, res_stream, fsinfo);
	waitpid(pid, &status, 0);
	Trace(TR_OPRMSG, "metadb slices processed");
	return (res);
}


/* return the list of slices used by VxVM volumes */
static sqm_lst_t *
get_vxvm_slices(
sqm_lst_t *vxvmslices)	/* append VxVM slices to this list */
{

	FILE *res_stream;
	char fsinfo[] = "VxVM";
	char cmd[] = VXDISK_CMD
	    " | /usr/bin/awk '$5 ~ /online/ {print $6}'";
	sqm_lst_t *res = vxvmslices;
	int status;
	pid_t pid;

	Trace(TR_DEBUG, "get VxVM slices");
	if (!vxvm_avail) {
		Trace(TR_OPRMSG, "VxVM not available, operation skipped");
		return (res);
	}
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL)))
		return (res);
	res =
	    get_slices_from_stream(vxvmslices, res_stream, fsinfo);
	waitpid(pid, &status, 0);
	Trace(TR_OPRMSG, "VxVM slices processed");
	return (res);
}


/*
 * Return a list of zvols
 */

static sqm_lst_t *
get_zvol_slices(
sqm_lst_t *zvoldevs)	/* append zvol devices to this list */
{

	FILE *res_stream;
	char fsinfo[] = "ZVOL";
	char cmd[] = ZVOL_CMD
	    " | /usr/bin/awk '{ print $1 }'";
	sqm_lst_t *res = zvoldevs;
	int status;
	pid_t pid;

	Trace(TR_DEBUG, "get zvol devices");
	if (!zvol_avail) {
		Trace(TR_OPRMSG, "ZVOLs not available, operation skipped");
		return (res);
	}

	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL)))
		return (res);
	res =
	    get_slices_from_stream(zvoldevs, res_stream, fsinfo);
	waitpid(pid, &status, 0);
	Trace(TR_OPRMSG, "done getting zvols");
	return (res);
}

/* Return the size of a zvol in bytes. */

static offset_t
getzvolsize(const char *zvol) {

	offset_t	offset;
	int 		fd;

	Trace(TR_DEBUG, "get ZVOL size");
	if ((fd = open(zvol, O_RDONLY)) < 0) {
		Trace(TR_ERR, "open() failed on %s", zvol);
		return (0);
	}

	if ((offset = llseek(fd, 0, SEEK_END)) < 0) {
		Trace(TR_ERR, "llseek() failed on %s", zvol);
		close(fd);
		return (0);
	}
	Trace(TR_OPRMSG, "done getting zvol size");
	close(fd);

	return (offset);
}

static void
get_avail_aus_from_list(
sqm_lst_t *avail)	/* remove the in-use AU-s from this list */
{

	node_t *crt, *nxt;
	au_t *aup;

	Trace(TR_DEBUG, "start filtering in-use AU-s");
	if (NULL == avail)
		return;
	if (0 == avail->length)
		return;
	crt = avail->head;
	while (crt) {
		aup = (au_t *)crt->data;
		nxt = crt->next;
		if (strlen(aup->fsinfo)) {
			free(aup);
			if (-1 == lst_remove(avail, crt))
				Trace(TR_ERR, "internal error: lst_remove()"
				    " failed in get_avail_aus_from_list():");
		}
		crt = nxt;
	}
	Trace(TR_DEBUG, "AU filtering done");
}


void
add_svm_raid_info(
sqm_lst_t *aus)
{
	FILE *res_stream;
	char cmd[] = METASTAT_CMD
	    " | /usr/bin/awk '{ if ($2 ~ /^-.*/) print $1 \" \" $2}'";

	int status;
	pid_t pid;
	char md[50];
	char raidinfo;
	node_t *node;
	au_t *au;

	Trace(TR_DEBUG, "adding SVM RAID info");
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		Trace(TR_ERR, "Could not collect SVM RAID info using %s",
		    METASTAT_CMD);
		return;
	}

	while (EOF != fscanf(res_stream, "%s -%c\n", md, &raidinfo)) {
		Trace(TR_OPRMSG, "md=%s raidinfo=%c", md, raidinfo);
		node = aus->head;
		while (NULL != node) {
			au = (au_t *)node->data;
			if (au->type != AU_SVM) {
				node = node->next;
				continue;
			}
			/* compare md w/ endof au->path */
			if (0 == strcmp(md, strrchr(au->path, '/') + 1)) {
				switch (raidinfo) {
				case 'm':
					/* mirror */
					au->raid = strdup("1");
					break;
				case 'r':
					au->raid = strdup("5");
					break;
				}
			}
			node = node->next;
		}
	}
	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_OPRMSG, "SVM RAID info obtained");
}


void
add_vxvm_raid_info(
sqm_lst_t *aus)
{
	FILE *res_stream;
	char cmd[] = VXPRINT_CMD
	    /* extract information about diskgroups, volumes and plexes */
	    " | /usr/bin/awk '/^dg|^v|^pl/ {print $1 \" \" $2 \" \" $3}'";
	int status;
	pid_t pid;
	int plexes;
	char type[3], vx[50], diskgrp[50],
	    dgvx[100]; // diskgroup + volname
	char info[20]; // raid5 or not
	node_t *node;
	au_t *au, *lastvol = NULL;

	Trace(TR_DEBUG, "adding VxVMM RAID info");
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		Trace(TR_ERR, "Could not collect VxVM RAID info using %s",
		    VXPRINT_CMD);
		return;
	}

	while (EOF != fscanf(res_stream, "%s %s %s\n", type, vx, info)) {

		switch (type[0]) {
		case 'd':
			if (0 == strcmp(vx, "rootdg"))
				diskgrp[0] = '\0';
			else {
				strlcpy(diskgrp, vx, sizeof (diskgrp));
				strlcat(diskgrp, "/", sizeof (diskgrp));
			}
			strlcpy(dgvx, diskgrp, sizeof (dgvx));
			continue;
		case 'v':
			plexes = 0;
			break;
		case 'p':
			plexes++;
			if (plexes == 2)
				if (lastvol != NULL)
					if (lastvol->raid == NULL) // notRAID5
						// mirror
						lastvol->raid = strdup("1");
			continue;
		}

		/* 'info' contains a volume name. check raid for this vol. */
		node = aus->head;
		while (NULL != node) {
			au = (au_t *)node->data;
			if (au->type != AU_VXVM) {
				node = node->next;
				continue;
			}
			/*
			 * the same volume name can be used in different
			 * diskgroups, so we need to include the diskgroup name
			 * in the comparison. The exception is rootdg.
			 */
			strcat(dgvx, vx);
			// printf("comparing %s w/ %s\n",
			//    dgvx, &au->path[VXVM_PATH_LEN]); // debug
			if (0 == strcmp(dgvx, &au->path[VXVM_PATH_LEN])) {
				if (0 == strcmp(info, "raid5"))
					au->raid = strdup("5");
				else
					lastvol = au;
			}
			dgvx[strlen(diskgrp)] = '\0';
			node = node->next;
		}
	}
	fclose(res_stream);
	waitpid(pid, &status, 0);
	Trace(TR_OPRMSG, "VxVM RAID info obtained");
}


/*
 * -1 = cannot check slice
 *  0 = no overlap
 *  1 = overlap
 */
static int
slice_overlaps(
char *slice)
{
	int fd, slice_idx, /* the part. index of the slice being checked */
	    part_idx;	   /* current part. index */
	struct vtoc toc;
	daddr_t start, end;	/* start/end sectors */
	char *rslice;		/* holds the /dev/rdsk/... path name */

	if (-1 == dsk2rdsk(slice, &rslice))
		return (-1);
	if ((fd = open(rslice, O_RDONLY)) < 0) {
		Trace(TR_ERR, "cannot open %s, errno=%d:%s", rslice,
		    errno, strerror(errno));
		free(rslice);
		return (-1);
	}
	slice_idx = read_vtoc(fd, &toc);
	close(fd);
	Trace(TR_DEBUG, "vtoc read. slice idx=%d\n", slice_idx);

	if ((slice_idx < 0) || 				/* possibly EFI */
	    (slice_idx > (toc.v_nparts - 1))) {		/* out of range */
		free(rslice);
		return (-1);
	}

	/* determine the boundaries of current partition */
	start = toc.v_part[slice_idx].p_start;
	end = start + toc.v_part[slice_idx].p_size - 1;

	Trace(TR_DEBUG, "start:%ld, end:%ld\n", start, end);
	/* check against all the other partitions on this disk */
	for (part_idx = 0; part_idx < toc.v_nparts; part_idx++) {
		if (slice_idx == part_idx)
			continue;
		Trace(TR_DEBUG, "part[%d]: start:%ld sz:%ld\n", part_idx,
		    toc.v_part[part_idx].p_start, toc.v_part[part_idx].p_size);
		if (toc.v_part[part_idx].p_start +
		    toc.v_part[part_idx].p_size - 1 < start	||
		    end < toc.v_part[part_idx].p_start)
			continue;
		if (0 == toc.v_part[part_idx].p_start)
			// slices such as s2; ignored by discovery
			continue;
		Trace(TR_OPRMSG, "Overlap detected for slice %s", rslice);
		free(rslice);
		return (1);
	}
	free(rslice);
	return (0);
}


/*
 *  information gathered via SCSI  inquiries
 */
static scsi_info_t *
extractinfo(
char *buf,		/* extract info from this buffer */
scsi_info_t **pscsi)	/* and populate this structure */
{

	scsi_info_t *scsi = (scsi_info_t *)mallocer(sizeof (scsi_info_t));

	if (NULL == scsi)
		return (NULL);

	*pscsi = scsi;
	strlcpy(scsi->vendor, buf, 8);
	strlcpy(scsi->prod_id, buf + 8, 16);
	strlcpy(scsi->rev_level, buf + 24, 4);
	scsi->vendor[8] = scsi->prod_id[16] = scsi->rev_level[4] = '\0';
	scsi->dev_id = NULL;

/* 	printf("Vend:\t%s.\nProd:\t%s.\nRevs:\t%s.\n", */
/*	    scsi->vendor, scsi->prod_id, scsi->rev_level); */
/* 	printf("SPC ver (%02xh):", (*(buf - 6)) & 0xff) */;
	return (scsi);
}

static char *std_inquiry(
devdata_t *devdata,
char *buf, /* result goes here */
int buflen)
{
	char cdb[] = { 0x12, 0, 0, 0, 0, 0 };

	cdb[4] = buflen;
	if (0 != issue_cmd(devdata, cdb, 6, buf, &buflen,
	    DATAIN, OPTIONAL)) { // error
		return (NULL);
	}
	return (buf);
}


static int
parse_id_descriptor(
char *desc,		/* identification descriptor */
char *result)
{
	int length = 0, i;
	int id_type;

	/* length validations are done by the calling function */
	id_type = desc [1] & 0xf;

	length = desc [3];
	if (id_type == 3) {
		for (i = 0; i < length; i++)
			sprintf(result, "%s%02x",
			    result, (unsigned char)desc[i+4]);
	}

	return (0);
}


static char *
devid_inquiry(
devdata_t *devdata)
{

#define	SCMD_MAX_INQUIRY_PAGE83_SIZE	0xff
#define	SCMD_MIN_INQUIRY_PAGE83_SIZE	0x08
#define	DEVIDSTR_LEN 0x1ff

	char cdb [] = {0x12, 1, (char)0x83, 0,
			(char)SCMD_MAX_INQUIRY_PAGE83_SIZE, 0};
	char data[SCMD_MAX_INQUIRY_PAGE83_SIZE];
	int datalen = SCMD_MAX_INQUIRY_PAGE83_SIZE;
	int usedlen = 0;
	int dlen = 0;
	char *dblk;

	char buf[DEVIDSTR_LEN];

	if (0 != issue_cmd(devdata, cdb, 6, data, &datalen, DATAIN, OPTIONAL)) {
		Trace(TR_ERR, "SCSI inquiry (pg.83h) failed for dev %s",
		    devdata->logdevpath);
		return (NULL);
	}

	/*
	 * Validate the page 0x83 data
	 * Standards Spec. (spc3r23.pdf) T10/1416-D SCSI Primary Commands
	 * 7.6.3.1 Device Identification VPD Page (Page 323)
	 * Also see on10 source - usr/src/common/devid/devid_scsi.c
	 *
	 * Peripheral device type (bits 0 - 4) should not be 0x1Fh (Unknown)
	 * page length field should contain a non zero length value
	 * and not be greater than 255 bytes
	 */
	if ((data[0] & 0x1F) == 0x1F) {
		Trace(TR_ERR, "au_devid_inquiry failed: Unknown Device");
		return (NULL);
	}
	if ((data[2] == 0) && (data[3] == 0)) {
		/* length field is 0 */
		Trace(TR_ERR, "au_devid_inquiry failed: length field is 0");
		return (NULL);
	}
	if (data[3] > (SCMD_MAX_INQUIRY_PAGE83_SIZE - 3)) {
		/* length field exceeds expected size of 255 bytes */
		Trace(TR_ERR, "au_devid_inquiry failed:"
		    "length field exceeds expected size of 255 bytes");
		return (NULL);
	}
	if (datalen >= SCMD_MIN_INQUIRY_PAGE83_SIZE) {
		Trace(TR_OPRMSG, "au_devid_inquiry returned ok len = %d",
		    datalen);

	} else {
		Trace(TR_ERR, "au_devid_inquiry returned short buf %d",
		    datalen);
		return (NULL);
	}

	Trace(TR_OPRMSG, "au_devid_inq: Page length = %x,"
	    "Peripheral qualifier = %x,"
	    "Device class = %x",
	    data[0] & 0xe0, data[0] & 0x1f, data[3]);
	/*
	 * convert device identification data to displayable format
	 *
	 * it is possible to have multiple descriptors blocks. Length of
	 * each descriptor block is contained in the descriptor header.
	 */
	memset(buf, 0, DEVIDSTR_LEN);
	dblk = &data[4];
	while (usedlen < data[3]) {
		dlen = dblk[3];
		if (dlen <= 0) {
			Trace(TR_ERR, "au_devid_inquiry failed:"
			    "length of identifier descriptor is invalid");
			return (NULL);
		}
		if ((usedlen + dlen) > data[3]) {
			Trace(TR_ERR, "au_devid_inquiry failed:"
			    "length is greater than expected");
			return (NULL);
		}
		if (parse_id_descriptor(dblk, buf) != 0) {
			Trace(TR_ERR, "au_devid_inquiry failed:"
			    "Parsing of descriptor failed");
			break;
		}
		/*
		 * advance to the next descriptor block,
		 * the descriptor block size is <desc header> + <desc data>
		 * <desc header> is equal to 4 bytes
		 * <desc data> is available in desc[3]
		 */
		dblk = &dblk[4 + dlen];
		usedlen += (dlen + 4);

		Trace(TR_OPRMSG, "au_devid_inq: usedlen = %d", usedlen);
	}

	return (strdup(buf));
}


static scsi_info_t *
get_scsi_info_for_au(
char *au_rpath)
{

#define	SCSI_INQUIRY_VID_EMC		"EMC     "
#define	SCSI_INQUIRY_VID_EMC_LEN	8

	int	fd;		/* File descriptor		*/
	char	buf[40];	/* Data buffer for inquiry	*/

	scsi_info_t *scsiinfo;
	devdata_t devdata;
	char *deviddata;

	if (ISNULL(au_rpath)) {
		return (NULL);
	}

	/* Get file descriptor to device */
	fd = open(au_rpath, O_RDONLY | O_NDELAY);
	if (fd == -1) {
		Trace(TR_ERR, "can't get scsi info - can't open %s: %s",
		    au_rpath, strerror(errno));
		return (NULL);
	}


	strlcpy(devdata.logdevpath, au_rpath, sizeof (devdata.logdevpath));
	devdata.fd = fd;
	devdata.verbose = B_FALSE;
	devdata.devtype = DISK_DEVTYPE;

	/* get product, vendor ID & rev level */
	if (NULL == std_inquiry(&devdata, buf, 40)) {
		close(fd);
		return (NULL);
	}

	extractinfo(&buf[8], &scsiinfo);

	/* EMC might not confrom to standards for page 0x83 */
	if (strncmp(scsiinfo->vendor,
	    SCSI_INQUIRY_VID_EMC, SCSI_INQUIRY_VID_EMC_LEN) != 0) {

		if (NULL == scsiinfo->dev_id) { // try pg code 0x83
			// get device identification information
			// using page 0x83
			if (NULL == (deviddata = devid_inquiry(&devdata))) {
				Trace(TR_ERR, "devid_inquiry failed: %s",
				    strerror(errno));
			}
			scsiinfo->dev_id = deviddata;
		}
	}
	if (NULL == scsiinfo->dev_id) {
		// printf("Issuing Inquiry (pg. 0x80) to %s...\n",
		//   devdata.logdevpath);
		/* get device identification information using page 0x80 */
		char s[256];
		if (0 != scsi_inq_pg80(&devdata, s)) {
			Trace(TR_ERR, "get scsi info failed: %s",
			    samerrmsg);
		} else {
			scsiinfo->dev_id = copystr(s);
			if (scsiinfo->dev_id == NULL) {
				Trace(TR_ERR, "get scsi info failed: %s",
				    samerrmsg);
			}
		}
	}
	close(fd);
	return (scsiinfo);
}


// DID device and diskset functions. added in 4.5

static sqm_lst_t *
get_did_dsks(sqm_lst_t *hosts, sqm_lst_t **dids) {

	FILE *res_stream;
	sqm_lst_t *res;
	char cmd[] = SCDIDADM_CMD;
	int status;
	pid_t pid;
	char did[16], prevdid[16], host[MAXHOSTNAMELEN];
	int hostcnt;
	char *tmpstr = NULL;

	if (ISNULL(hosts, dids))
		return (NULL);
	*dids = lst_create();
	res = *dids;

	tmpstr = lst2str(hosts, ",");
	Trace(TR_MISC, "get did-s visible from %s", Str(tmpstr));
	free(tmpstr);
	if (-1 == (pid = exec_get_output(cmd, &res_stream, NULL))) {
		Trace(TR_OPRMSG, "no did-s processed. SC not present?");
		return (res);
	}

	prevdid[0] = '\0';

	while ((status = fscanf(res_stream, "%s %s\n", did, host)) != EOF) {
		if (status != 2) {
			/* bad input line */
			Trace(TR_ERR, "bad fscanf in get_did_dsks");
			continue;
		}

		if (0 != strcmp(did, prevdid)) {
			strlcpy(prevdid, did, sizeof (prevdid));
			hostcnt = 0;
		}
		// printf("\t%s %s %d", did, host, hostcnt);
		if (NULL != lst_search(hosts, host, (lstsrch_t)strcmp)) {
			hostcnt++;
		}
		// printf(" %d. ", hostcnt);
		// if did accessible from all specified hosts, add it to res
		if (hostcnt == hosts->length) {
			lst_append(res, strdup(did));
			hostcnt = 0; // so we won't append same did again
			// printf("did %s added!", did);
		}
		// printf("\n");
	}
	fclose(res_stream);
	waitpid(pid, &status, 0);

	Trace(TR_OPRMSG,
	    "did-s processed. %d accessible from the %d specified hosts",
	    res->length, hosts->length);
	return (res);
}

static int
check_did_slices(sqm_lst_t *dids, sqm_lst_t **paus) {

	const int maxslices = 8;
	int res = 0, i, status;
	char rslice[MAXPATHLEN];
	dsize_t capacity = 0;
	node_t *did;
	au_t *au;

	*paus = lst_create();
	if (dids == NULL)
		return (0); // SC not present
	if (ISNULL(paus))
		return (-1);

	for (did = dids->head; did != NULL; did = did->next) {
		if (did->data == NULL) {
			continue;
		}

		for (i = 0; i < maxslices; i++) {
			snprintf(rslice, sizeof (rslice), "%s/%ss%d",
			    PREFRDID, (char *)did->data, i);
			if ((status = checkslice(rslice, AU_SLICE, &capacity))
			    >= 0) {
				au = create_au_elem(rslice, AU_SLICE,
				    capacity, status);
				if (au == NULL) {
					setsamerr(SE_NO_MEM);
					res = -1;
					break;
				}
				au->scsiinfo = get_scsi_info_for_au(rslice);
				lst_append(*paus, au);
			}
		}
	}
	return (res);
}

static sqm_lst_t *
get_diskset_slices(sqm_lst_t *aus) { // append them to this list

	char dsetpath[MAXPATHLEN + 1];
	DIR *dp;
	dirent64_t *dirp;
	dirent64_t *dirpp;
	struct stat buf;

	Trace(TR_MISC, "getting list of disksets");
	if ((dp = opendir(PREF_MD)) == NULL) {
		Trace(TR_ERR, "failed to open dir %s", PREF_MD);
		return (aus);
	}
	dirp = mallocer(sizeof (dirent64_t) + MAXPATHLEN + 1);
	if (dirp == NULL) {
		return (NULL);
	}

	if (dsets != NULL) // used in get_svm_slices()
		lst_free_deep(dsets);
	dsets = lst_create();

	while ((readdir64_r(dp, dirp, &dirpp)) == 0) {
		if (dirpp == NULL) {
			break;
		}

		// skip 'special' subdirs
		if (strcmp(dirp->d_name, ".") == 0 ||
		    strcmp(dirp->d_name, "..") == 0 ||
		    strcmp(dirp->d_name, "dsk") == 0 ||
		    strcmp(dirp->d_name, "rdsk") == 0 ||
		    strcmp(dirp->d_name, "admin") == 0 ||
		    strcmp(dirp->d_name, "shared") == 0)
			continue;

		// walk diskset subdirs
		snprintf(dsetpath, sizeof (dsetpath), "%s/%s", PREF_MD,
		    dirp->d_name);
		if (stat(dsetpath, &buf) < 0) {
			continue;
		} else {
			lst_append(dsets, strdup(dirp->d_name));
			if (S_ISDIR(buf.st_mode)) {
				strlcat(dsetpath, "/rdsk", sizeof (dsetpath));
				walkdir(dsetpath, AU_SVM, aus);
			}
		}
	}
	closedir(dp);
	free(dirp);
	Trace(TR_MISC, "%d disksets obtained", dsets->length);
	return (aus);
}
