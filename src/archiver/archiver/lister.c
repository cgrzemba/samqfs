/*
 * lister.c - List archiver information.
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.70 $"

static char *_SrcFile = __FILE__;   /* Using __FILE__ makes duplicate strings */

/* Feature test switches. */
/* PRvsn_exp If defined, print vsn regular expressions */

/* ANSI C headers. */
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <unistd.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "aml/diskvols.h"
#include "sam/mount.h"
#include "sam/lib.h"
#include "aml/sam_rft.h"

/* Local headers. */
#define	NEED_ARCHSET_NAMES
#define	NEED_EXAM_METHOD_NAMES
#define	NEED_FILEPROPS_NAMES
#include "archiver.h"
#include "archset.h"
#include "device.h"
#include "fileprops.h"
#include "volume.h"

/* Private data. */
static struct ArchSet *as;
static struct ArchSet *asd;

/* Private functions. */
static void printArchiveSets(void);
static void printCatalog(int aln);
static void printDiskVolsDict(int printHcVols);
static void printConf(void);
static void printVsns(struct ArchSet *as);
static void printDiskVols(struct ArchSet *as);
static int cmp_asn(const void *p1, const void *p2);
static int cmp_vpn(const void *p1, const void *p2);

#define	paramsPrioritySet NULL
static char *paramsPriorityTostr(void *v, char *buf, int bufsize);
#define	paramsReserveSet NULL
static char *paramsReserveTostr(void *v, char *buf, int bufsize);
#include "aml/archset.hc"

#define	propAfterSet NULL
static char *propAfterTostr(void *v, char *buf, int bufsize);
#define	propGroupSet NULL
static char *propGroupTostr(void *v, char *buf, int bufsize);
#define	propNameSet NULL
static char *propNameTostr(void *v, char *buf, int bufsize);
#define	propUserSet NULL
static char *propUserTostr(void *v, char *buf, int bufsize);
#include "fileprops.hc"


/*
 * Check archiver initialization.
 * Read the configuration file.
 */
void
PrintInfo(
	char *fsname)
{
	if (ListOptions & LO_conf) {
		printConf();
	}

	/*
	 * List file system information.
	 */
	if (ListOptions & (LO_fs | LO_arch)) {
		boolean_t notRoot;
		int	fsn;

		if (ListOptions & LO_fs) {
			/* Filesystems */
			printf("\n%s:\n", GetCustMsg(4602));
		}

		/* Only root users can examine the file system. */
		notRoot = geteuid() != 0;
		if (notRoot) {
			/* You must be root to examine the file system. */
			printf("** %s **\n", GetCustMsg(4603));
		}
		for (fsn = 0; fsn < FileSysTable->count; fsn++) {
			struct FileSysEntry *fs;
			static struct sam_fs_info fi;

			fs = &FileSysTable->entry[fsn];
			if (fsname != NULL && strcmp(fs->FsName, fsname) != 0) {
				continue;
			}
			if (GetFsInfo(fs->FsName, &fi) == -1) {
				LibFatal(GetFsInfo, fs->FsName);
			}
			if (ListOptions & LO_fs) {
				printf("%s", fs->FsName);
				if (*fi.fi_mnt_point != '\0') {
					/* mount: */
					printf(" %s %s", GetCustMsg(4638),
					    fi.fi_mnt_point);
				} else {
					/* Not mounted */
					printf("*%s*", GetCustMsg(4604));
				}
				/* Examine */
				printf("\n   %s: %s", GetCustMsg(4634),
				    ExamMethodNames[fs->FsExamine]);
				/* Interval */
				printf("  %s: %s\n", GetCustMsg(4633),
				    StrFromInterval(fs->FsInterval, NULL, 0));
				/* Logfile */
				printf("   %s:",
				    GetCustMsg(4616));
				/* *None* */
				printf("%s\n",
				    (*fs->FsLogFile != '\0') ?
				    fs->FsLogFile : GetCustMsg(4605));
				printf("\n");
			}
			if (!notRoot && (*fi.fi_mnt_point != '\0')) {
				/*
				 * Open the .inodes file.
				 */
				MntPoint = fi.fi_mnt_point;
				if ((FsFd = OpenInodesFile(MntPoint)) == -1) {
					LibFatal(open, ".inodes");
				}
				if (ListOptions & LO_fs) {
					PrintFsStats();
				}
				if (ListOptions & LO_arch) {
					FileProps = FilesysProps[fsn];
					snprintf(ScrPath, sizeof (ScrPath),
					    ARCHIVER_DIR"/%s/"ARFIND_STATE,
					    fs->FsName);
					State = ArMapFileAttach(ScrPath,
					    AF_MAGIC, O_RDWR);
					if (State == NULL) {
						LibFatal(ArMapFileAttach,
						    ScrPath);
					}
					PrintArchiveStatus();
					(void) ArMapFileDetach(State);
				}
				(void) close(FsFd);
			}
		}
	}
}


/* Private functions. */


/*
 * Print Archive Sets.
 */
static void
printArchiveSets(void)
{
	struct ArchSet **as_sort;
	size_t  size;
	int	asn;

	/*
	 * List the archive sets in alphabetical order.
	 * With allsets first.
	 */
	size = ArchSetNumof * sizeof (struct ArchSet *);
	SamMalloc(as_sort, size);
	for (asn = 0; asn < ArchSetNumof; asn++) {
		as_sort[asn] = &ArchSetTable[asn];
	}
	qsort(as_sort + ALLSETS_MAX, ArchSetNumof - ALLSETS_MAX,
	    sizeof (struct ArchSet *), cmp_asn);

	/* Archive sets */
	printf("\n%s:", GetCustMsg(4618));
	for (asn = 0; asn < ArchSetNumof; asn++) {
		struct fieldVals *table;
		uint32_t *fldDef;
		uint32_t flags;
		uint32_t flagsd;

		/*
		 * Select default parameters that will apply to this archive
		 * set.  Skip the archive set if no VSNs are needed.
		 */
		as = as_sort[asn];
		if (asn < ALLSETS_MAX) {
			asd = &ArchSetTable[0];
		} else {
			if (!(as->AsCflags & AC_needVSNs)) {
				continue;
			}
			asd = &ArchSetTable[1 + AS_COPY(as->AsName)];
		}

		printf("\n%s\n", as->AsName);
		/*
		 * List parameters.
		 */
		flags = as->AsFlags;
		if (asn == 0) {
			/*
			 * Show all parameters for 'allsets'.
			 * They are the default values.
			 */
			flagsd = 0xffffffff;
		} else {
			flagsd = asd->AsFlags | ArchSetTable[0].AsFlags;
		}
		if (flags & AS_diskArchSet) {
			flags &= ~AS_rmonly;
			flagsd &= ~AS_rmonly;
		}
		fldDef = NULL;
		for (table = ArchSet; table->FvName != NULL; table++) {

			if (table->FvType == DEFBITS) {
				fldDef = (uint32_t *)(void *)((char *)as +
				    table->FvLoc);

				/*
				 * List priorities and recycling parameters
				 * under headings.
				 */
				if (table->FvLoc ==
				    offsetof(struct ArchSet, AsRyFlags)) {
					flags = as->AsRyFlags;
					flagsd = asd->AsRyFlags |
					    ArchSetTable[0].AsRyFlags;
					if ((flags | flagsd) != 0) {
						/* Recycling */
						printf("  %s:\n",
						    GetCustMsg(4632));
					}
				}
				if (table->FvLoc ==
				    offsetof(struct ArchSet, AsDbgFlags)) {
					flags = as->AsDbgFlags;
					flagsd = asd->AsDbgFlags |
					    ArchSetTable[0].AsDbgFlags;
					if ((flags | flagsd) != 0) {
						printf("  Debug:\n");
					}
				}
				continue;
			}
			if (fldDef !=
			    NULL && ((flags | flagsd) & table->FvDefBit)) {
				char	*deflt;

				if (strcmp(table->FvName, "priority") == 0) {
					/* Priorities */
					printf("  %s:\n", GetCustMsg(4630));
					(void) SetFieldValueToStr(as, table,
					    table->FvName, NULL, 0);
					continue;
				}
				/*
				 * Indicate allsets values with '.'.
				 */
				if (flags & table->FvDefBit) {
					deflt = " ";
				} else {
					deflt = ".";
				}
				printf("   %s%s: %s\n", deflt, table->FvName,
				    SetFieldValueToStr(as, table, table->FvName,
				    NULL, 0));
			}
		}

		/*
		 * List the volumes available.
		 */
		if (asn < ALLSETS_MAX) {
			continue;  /* ALL_SETS has no volumes */
		}
		/*  media: */
		printf("  %s %s", GetCustMsg(4640), as->AsMtype);
		if (as->AsCflags & AC_defVSNs) {
			/* (by default) */
			printf(" %s", GetCustMsg(4641));
		}
		printf("\n");

		if (as->AsVsnDesc == VD_none) {
			/* No volumes defined */
			printf("  ** %s **\n", GetCustMsg(4619));
			continue;
		}
		if (as->AsCflags & AC_defVSNs) {
			/* Volumes:  all (by default) */
			printf(" %s\n", GetCustMsg(4642));
			continue;
		}
		if (as->AsFlags & AS_diskArchSet) {
			printDiskVols(as);
		} else {
			printVsns(as);
		}
#if defined(PRvsn_exp)
/* BLOCK for cstyle */	{
		struct VsnExp *ve;
		int vsn_exp;

			/* List the vsn expressions. */
			for (vsn_exp = as->vsn_exp; vsn_exp != -1;
			    vsn_exp = ve->next) {
				char *p;

				ve = (struct VsnExp *)((char *)VsnExpTable +
				    vsn_exp);
				printf("    ");
				p = ve->expbuf;
				while (*p != 0) {
					printf("%x", *p++);
					if (*p != '\0') {
						printf(",");
					}
				}
				printf("\n");
			}
		}
#endif /* defined(PRvsn_exp) */
	}
	SamFree(as_sort);
}


/*
 * Print catalogs.
 */
static void
printCatalog(
	int aln)	/* Archive library number */
{
	boolean_t first;
	int vin;

	first = TRUE;
	for (vin = 0; /* Terminated inside */; vin++) {
		struct VolInfo vi;

		if (GetVolInfo(vin, &vi) == -1) {
			break;
		}
		if (vi.VfAln != aln) {
			continue;
		}
		if (first) {
			first = FALSE;
			/* Catalog */
			printf("  %s:\n", GetCustMsg(4620));
		}
		printf("  ");
		PrintVolInfo(stdout, &vi);
	}
}


/*
 * Print disk volume dictionary.
 */
static void
printDiskVolsDict(
	int printHcVols)
{
	struct DiskVolsDictionary *diskVols;
	boolean_t first;
	char *volName;
	struct DiskVolumeInfo *dv;
	char *mtype;

	diskVols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT, 0);
	if (diskVols == NULL) {
		return;
	}

	first = TRUE;
	diskVols->BeginIterator(diskVols);
	while (diskVols->GetIterator(diskVols, &volName, &dv) == 0) {
		if (dv == NULL) {
			continue;
		}
		if (first) {
			first = FALSE;
			/* Dictionary */
			printf("  %s:\n", GetCustMsg(4649));
		}

		mtype = sam_mediatoa(dv->DvMedia);
		if (printHcVols == 0) {
			if (DISKVOLS_IS_DISK(dv)) {
				printf("  %s.%-20s ", mtype, volName);
				printf("%s %6s ", GetCustMsg(4635),
				    StrFromFsize((fsize_t)dv->DvCapacity,
				    1, NULL, 0));
				printf("%s %6s ", GetCustMsg(4636),
				    StrFromFsize((fsize_t)dv->DvSpace,
				    1, NULL, 0));
				printf("\n");
			}
		} else {
			if (DISKVOLS_IS_HONEYCOMB(dv)) {
				printf("  %s.%-20s ", mtype, volName);
				printf("%s ", HONEYCOMB_DEVNAME);
				if (dv->DvPort == -1) {
					printf("%s", dv->DvAddr);
				} else {
					printf("%s:%d", dv->DvAddr, dv->DvPort);
				}
				printf("\n");
			}
		}
	}
	diskVols->EndIterator(diskVols);
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
}


/*
 * Print configuration information.
 */
static void
printConf(void)
{
	size_t	size;
	int	i;

	/*
	 * List the file names.
	 */
	/* Notify file */
	printf("%s: %s\n", GetCustMsg(4608), AdState->AdNotifyFile);

	/*
	 * List timeouts.
	 */
	/* Read timeout */
	printf("%s: %s\n", GetCustMsg(4651),
	    StrFromInterval(MediaParams->MpReadTimeout, NULL, 0));
	/* Request timeout */
	printf("%s: %s\n", GetCustMsg(4652),
	    StrFromInterval(MediaParams->MpRequestTimeout, NULL, 0));
	/* Stage timeout */
	printf("%s: %s\n", GetCustMsg(4653),
	    StrFromInterval(MediaParams->MpStageTimeout, NULL, 0));
#if defined(lint)
	printf("%s\n", Timeouts[0].EeName);
#endif /* defined(lint) */

	/*
	 * List archive media characteristics.
	 */
	/* Archive media */
	printf("\n%s:\n", GetCustMsg(4609));
	for (i = 0; i < MediaParams->MpCount; i++) {
		struct MediaParamsEntry *mp;

		mp = &MediaParams->MpEntry[i];
		if (!((mp->MpFlags & MP_avail) || (mp->MpFlags & MP_refer))) {
			continue;
		}
		/* media: */
		printf("%s%s", GetCustMsg(4640), mp->MpMtype);
		printf(" bufsize: %d", mp->MpBufsize);
		if (mp->MpFlags & MP_lockbuf) {
			printf(" (locked)");
		}
		printf(" archmax: %7s", StrFromFsize(mp->MpArchmax,
		    1, NULL, 0));
		if (mp->MpFlags & MP_volovfl) {
			printf(" ovflmin: %7s", StrFromFsize(mp->MpOvflmin,
			    1, NULL, 0));
		} else if (is_disk(mp->MpType) == B_FALSE &&
		    is_stk5800(mp->MpType) == B_FALSE) {
			/* Volume overflow not selected */
			printf(" %s", GetCustMsg(4621));
		}
		/* Write timeout */
		printf(" %s: %s", GetCustMsg(4654),
		    StrFromInterval(mp->MpTimeout, NULL, 0));
		printf("\n");
	}

	/*
	 * List archive library information.
	 */
	/* Archive libraries */
	printf("\n%s:\n", GetCustMsg(4611));
	for (i = 0; i < ArchLibTable->count; i++) {
		struct ArchLibEntry *al;

		al = &ArchLibTable->entry[i];
		if (al->AlFlags & AL_historian) {
#if !defined(USE_HISTORIAN)
			continue;
#endif /* !defined(USE_HISTORIAN) */
		}
		/* Device: */
		printf("%s %s", GetCustMsg(4644), al->AlName);
		if (!(al->AlFlags & (AL_disk | AL_honeycomb))) {
			/* drives_available */
			printf(" %s: %d", GetCustMsg(4612), al->AlDrivesAvail);
		}
		/* archive_drives */
		printf(" %s: %d\n", GetCustMsg(4613), al->AlDrivesAllow);
		if (ListOptions & LO_vsn) {
			if (al->AlFlags & (AL_disk | AL_honeycomb)) {
				printDiskVolsDict(al->AlFlags & AL_honeycomb);
			} else {
				printCatalog(i);
			}
		}
		printf("\n");
	}

	/*
	 * List filesystem information.
	 */
	/* Archive file selections */
	printf("\n%s:\n", GetCustMsg(4614));
	for (i = 0; i < FileSysTable->count; i++) {
		struct FileProps *fsfp;
		struct FileSysEntry *fs;
		int fpn;

		fs = &FileSysTable->entry[i];
		/* Filesystem */
		printf("%s %s", GetCustMsg(4615), fs->FsName);
		if (fs->FsFlags & FS_noarchive) {
			if (fs->FsFlags & FS_share_client) {
				printf("  share_client");
			} else if (fs->FsFlags & FS_share_reader) {
				printf("  share_reader");
			} else if (fs->FsFlags & FS_noarfind) {
				printf("  noarscan");
			} else {
				printf("  noarchive");
			}
			printf("\n\n");
			continue;
		}

		if (fs->FsFlags & FS_wait) {
			printf("  wait");
		}
		/* Examine */
		printf("  %s: %s", GetCustMsg(4634),
		    ExamMethodNames[fs->FsExamine]);
		/* Interval */
		printf("  %s: %s\n", GetCustMsg(4633),
		    StrFromInterval(fs->FsInterval, NULL, 0));
		printf("  archivemeta: %s",
		    (fs->FsFlags & FS_archivemeta) ? "on" : "off");
		printf("  scanlistsquash: %s",
		    (fs->FsFlags & FS_scanlist) ? "on" : "off");
		printf("  setarchdone: %s\n",
		    (fs->FsFlags & FS_setarchdone) ? "on" : "off");
		printf("  background_interval: %s",
		    StrFromInterval(fs->FsBackGndInterval,
		    NULL, 0));
		printf("  background_time: %02d:%02d\n",
		    fs->FsBackGndTime >> 8, fs->FsBackGndTime & 0xff);
		/* Logfile */
		printf("  %s: %s\n", GetCustMsg(4616), fs->FsLogFile);
		fsfp = FilesysProps[i];
		for (fpn = 0; fpn < fsfp->FpCount; fpn++) {
			struct FilePropsEntry *fp;
			int	copy;

			fp = &fsfp->FpEntry[fpn];
			printf("%s ", ArchSetTable[fp->FpBaseAsn].AsName);
			if (fp->FpFlags & FP_noarch) {
				printf("  Noarchive");
			}
			if (!(fp->FpFlags & FP_metadata)) {
				/* path */
				printf("  %s ", GetCustMsg(4645));
				if (*fp->FpPath != '\0') {
					printf("%s", fp->FpPath);
				} else {
					printf(".");
				}
			} else {
				/* Metadata */
				printf("  %s", GetCustMsg(4617));
			}
			printf("\n");

			if (fp->FpFlags & (FP_props | FP_nftv)) {
				struct fieldVals *table;

				for (table = FilePropsEntry;
				    table->FvName != NULL; table++) {
					if (table->FvType == DEFBITS) {
						continue;
					}
					if (fp->FpFlags & table->FvDefBit) {
						printf("    %s: %s\n",
						    table->FvName,
						    SetFieldValueToStr(fp,
						    table, table->FvName,
						    NULL, 0));
					}
				}
			}
			if (fp->FpMask != 0) {
				ino_st_t mask;
				ino_st_t status;
				char	attr[32];
				char	*p;

				mask.bits = fp->FpMask;
				status.bits = fp->FpStatus;
				if (mask.b.bof_online |
				    mask.b.release |
				    mask.b.nodrop) {
					p = attr;
					if (!(status.b.bof_online |
					    status.b.release |
					    status.b.nodrop)) {
						*p++ = 'd';
					}
					if (status.b.release) {
						*p++ = 'a';
					}
					if (status.b.nodrop) {
						*p++ = 'n';
					}
					if (status.b.bof_online) {
						if (fp->FpPartial != 0) {
							snprintf(p,
							    sizeof (attr) -
							    Ptrdiff(p, attr),
							    "s%d",
							    fp->FpPartial);
							p += strlen(p);
						} else {
							*p++ = 'p';
						}
					}
					*p++ = '\0';
					printf("    release: %s\n", attr);
				}
				if (mask.b.stage_all | mask.b.direct) {
					p = attr;
					if (!(status.b.stage_all |
					    status.b.direct)) {
						*p++ = 'd';
					}
					if (status.b.stage_all) {
						*p++ = 'a';
					}
					if (status.b.direct) {
						*p++ = 'n';
					}
					*p++ = '\0';
					printf("    stage: %s\n", attr);
				}
			}

			if (fp->FpFlags & FP_noarch) {
				continue;
			}
			/*
			 * Print out copy information.
			 */
			for (copy = 0; copy < MAX_ARCHIVE; copy++) {
				int	copyBit;

				copyBit = 1 << copy;
				if (!(fp->FpCopiesReq & copyBit)) {
					continue;
				}
				/* copy: */
				printf("  %s %d ", GetCustMsg(4643), copy + 1);
				if (fp->FpCopiesRel & copyBit) {
					printf(" Release");
				}
				if (fp->FpCopiesNorel & copyBit) {
					printf(" Norelease");
				}
				printf(" arch_age: %s",
				    StrFromInterval(fp->FpArchAge[copy],
				    NULL, 0));
				if (fp->FpCopiesUnarch & copyBit) {
					printf(" unarch_age: %s",
					    StrFromInterval(
					    fp->FpUnarchAge[copy], NULL, 0));
				}
				printf("\n");
			}
		}
		printf("\n");
	}

	if (VsnPoolSize != 0) {
		struct VsnPool *vp, **vp_sort;
		struct ArchSet asd;
		size_t	offset;
		int	VsnPoolNumof;

		/*
		 * Count VSN pools.
		 * List the VSN pools in alphabetical order.
		 */
		VsnPoolNumof = 0;
		for (offset = 0; offset < VsnPoolSize; offset += size) {
			vp = (struct VsnPool *)(void *)((char *)VsnPoolTable +
			    offset);
			VsnPoolNumof++;
			size = sizeof (struct VsnPool) +
			    ((vp->VpNumof - 1) * sizeof (vp->VpVsnExp));
		}
		size = VsnPoolNumof * sizeof (struct VsnPool *);
		SamMalloc(vp_sort, size);
		i = 0;
		for (offset = 0; offset < VsnPoolSize; offset += size) {
			vp = (struct VsnPool *)(void *)((char *)VsnPoolTable +
			    offset);
			vp_sort[i++] = vp;
			size = sizeof (struct VsnPool) +
			    ((vp->VpNumof - 1) * sizeof (vp->VpVsnExp));
		}

		qsort(vp_sort, VsnPoolNumof, sizeof (struct VsnPool *),
		    cmp_vpn);
		memset(&asd, 0, sizeof (asd));

		/* VSN pools */
		printf("\n%s:\n", GetCustMsg(4629));
		for (i = 0; i < VsnPoolNumof; i++) {
			struct VsnPool *vp;

			vp = vp_sort[i];
			printf("%s", vp->VpName);
			/*
			 * Make an "archive set" that references the pool.
			 */
			strncpy(asd.AsName, vp->VpName,
			    sizeof (asd.AsName)-1);
			strncpy(asd.AsMtype, vp->VpMtype,
			    sizeof (asd.AsMtype)-1);
			/* media: */
			printf(" %s %s", GetCustMsg(4640), asd.AsMtype);
			asd.AsVsnDesc = VD_pool | Ptrdiff(vp, VsnPoolTable);
			if (strcmp(asd.AsMtype, "dk") == 0) {
				asd.AsFlags |= AS_disk_archive;
				printDiskVols(&asd);
			} else if (strcmp(asd.AsMtype, "cb") == 0) {
				asd.AsFlags |= AS_honeycomb;
				printDiskVols(&asd);
			} else {
				asd.AsFlags = 0;
				printVsns(&asd);
			}
		}
		SamFree(vp_sort);
	}
	printArchiveSets();
}


/*
 * List the volumes that match for the set.
 */
static void
printVsns(
	struct ArchSet *as)
{
	fsize_t	mediaSpace;
	int	numVsns;

	mediaSpace = 0;
	for (numVsns = 0; /* Terminated inside */; numVsns++) {
		struct VolInfo vi;

		if (GetRmArchiveVol(as, numVsns, NULL, NULL, &vi) == -1) {
			break;
		}
		mediaSpace += vi.VfSpace;
		if (!(ListOptions & LO_vsn)) {
			continue;
		}
		if (numVsns == 0) {
			/* Volumes: */
			printf(" %s\n", GetCustMsg(4646));
		}
		printf("   %s\n", vi.VfVsn);
	}
	if (numVsns == 0) {
		/* No volumes available */
		printf("  ** %s **\n", GetCustMsg(4333));
	} else {
		/* Total space available: */
		printf(" %s %7s\n", GetCustMsg(4647),
		    StrFromFsize(mediaSpace, 1, NULL, 0));
	}
}


/*
 * List the disk volumes or honeycomb silos that match for the set.
 */
static void
printDiskVols(
	struct ArchSet *as)
{
	struct DiskVolsDictionary *diskVols;
	int	numDiskVols;
	int availDiskVols;
	fsize_t	mediaSpace;
	struct DiskVolumeInfo *dv;

	diskVols = DiskVolsNewHandle(program_name, DISKVOLS_VSN_DICT, 0);
	if (diskVols == NULL) {
		printf("  ** No destinations available. **\n");
	}

	mediaSpace = 0;
	availDiskVols = 0;

	for (numDiskVols = 0; /* Terminated inside */; numDiskVols++) {
		int rval;
		struct VolInfo vi;

		rval = GetDkArchiveVol(as, diskVols, numDiskVols, &vi);

		if (rval == -1) {
			break;
		}

		(void) diskVols->Get(diskVols, vi.VfVsn, &dv);
		if (dv != NULL) {
			boolean_t accessible;

			if (as->AsFlags & AS_disk_archive) {
				/* Disk archive media, skip honeycomb volume. */
				if (DISKVOLS_IS_HONEYCOMB(dv)) {
					continue;
				}
			} else {
				/* Honeycomb archive media, skip disk volume. */
				if (DISKVOLS_IS_DISK(dv)) {
					continue;
				}
			}

			if (DISKVOLS_IS_HONEYCOMB(dv)) {
				accessible = B_TRUE;
			} else {
				accessible = SamrftIsAccessible(dv->DvHost,
				    dv->DvPath);
			}
			if (accessible == B_TRUE) {
				if (ListOptions & LO_vsn) {
					if (availDiskVols == 0) {
						/* Volumes: */
						printf(" %s\n",
						    GetCustMsg(4646));
					}

					printf("   %s ", vi.VfVsn);
					if (DISKVOLS_IS_HONEYCOMB(dv)) {
						printf("(%s ",
						    HONEYCOMB_DEVNAME);
						if (dv->DvPort == -1) {
							printf("%s)",
							    dv->DvAddr);
						} else {
							printf("%s:%d)",
							    dv->DvAddr,
							    dv->DvPort);
						}
					} else {
						if (dv->DvFlags & DV_remote) {
							printf("(%s:%s)",
							    dv->DvHost,
							    dv->DvPath);
						} else {
							printf("(%s)",
							    dv->DvPath);
						}
					}
					printf("\n");
				}
				availDiskVols++;
				mediaSpace += vi.VfSpace;
			}
		}
	}

	if (availDiskVols == 0) {
		/* No volumes available */
		printf("  ** %s **\n", GetCustMsg(4333));
	} else {
		/* Total space available */
		printf(" %s %7s\n", GetCustMsg(4647),
		    StrFromFsize(mediaSpace, 1, NULL, 0));
	}
	(void) DiskVolsDeleteHandle(DISKVOLS_VSN_DICT);
}


/*
 * Process 'priority' for SetFieldValueToStr().
 */
/*ARGSUSED0*/
static char *
paramsPriorityTostr(
	void *v,
	char *buf,
	int bufsize)
{
	struct fieldVals *table;
	uint32_t *fldDef;
	uint32_t flags;
	uint32_t flagsd;

	flags = as->AsPrFlags;
	flagsd = asd->AsPrFlags | ArchSetTable[0].AsPrFlags;
	for (table = Priorities; table->FvName != NULL; table++) {
		if (table->FvType == DEFBITS) {
			fldDef = (uint32_t *)(void *)((char *)as +
			    table->FvLoc);
			continue;
		}
		if (fldDef != NULL && ((flags | flagsd) & table->FvDefBit)) {
			char	*deflt;
			char	*name;

			name = table->FvName;
			/*
			 * Indicate allsets values with '.'.
			 */
			if (flags & table->FvDefBit) {
				deflt = " ";
			} else {
				deflt = ".";
			}
			printf("   %s%s: %s\n", deflt, name,
			    SetFieldValueToStr(as, table, table->FvName,
			    NULL, 0));
		}
	}
	return ("");
}


/*
 * Process 'reserve' for SetFieldValueToStr().
 */
static char *
paramsReserveTostr(
	void *v,
	char *buf,
	int bufsize)
{
	short	*val = (short *)v;
	int	i;

	/*
	 * There are easier ways, but this validates the table.
	 */
	i = 1;
	*buf = '\0';
	if (*val & Reserves[i].RsValue) {
		strncat(buf, Reserves[i].RsName, bufsize-1);
	}
	strncat(buf,  "/", bufsize-1);
	i++;
	if (*val & Reserves[i].RsValue) {
		strncat(buf, Reserves[i].RsName, bufsize-1);
	}
	i++;
	if (*val & Reserves[i].RsValue) {
		strncat(buf, Reserves[i].RsName, bufsize-1);
	}
	i++;
	if (*val & Reserves[i].RsValue) {
		strncat(buf, Reserves[i].RsName, bufsize-1);
	}
	strncat(buf,  "/", bufsize-1);
	i++;
	if (*val & Reserves[i].RsValue) {
		strncat(buf, Reserves[i].RsName, bufsize-1);
	}
	return (buf);
}


/*
 * Process 'after' for SetFieldValueToStr().
 */
static char *
propAfterTostr(
	void *v,
	char *buf,
	int bufsize)
{
	struct tm tm;
	time_t	tv = *(time_t *)v;

	strftime(buf, bufsize, "%Y-%m-%dT%T", localtime_r(&tv, &tm));
	return (buf);
}


/*
 * Process 'group' for SetFieldValueToStr().
 */
static char *
propGroupTostr(
	void *v,
	char *buf,
	int bufsize)
{
	struct group *gr;
	gid_t	gid = *(gid_t *)v;

	if ((gr = getgrgid(gid)) != NULL) {
		return (gr->gr_name);
	}
	snprintf(buf, bufsize, "%d", (int)gid);
	return (buf);
}


/*
 * Process 'name' for SetFieldValueToStr().
 */
/* ARGSUSED1 */
static char *
propNameTostr(
	void *v,
	char *buf,
	int bufsize)
{
	return ((char *)v);
}


/*
 * Process 'user' for SetFieldValueToStr().
 */
static char *
propUserTostr(
	void *v,
	char *buf,
	int bufsize)
{
	struct passwd *pw;
	uid_t	uid = *(uid_t *)v;

	if ((pw = getpwuid(uid)) != NULL) {
		return (pw->pw_name);
	}
	snprintf(buf, bufsize, "%d", (int)uid);
	return (buf);
}


/*
 * Compare archive set names.
 */
static int
cmp_asn(
	const void *p1,
	const void *p2)
{
	struct ArchSet *a1 = *(struct ArchSet **)p1;
	struct ArchSet *a2 = *(struct ArchSet **)p2;

	return (strcmp(a1->AsName, a2->AsName));
}


/*
 * Compare VSN pool names.
 */
static int
cmp_vpn(
	const void *p1,
	const void *p2)
{
	struct VsnPool *a1 = *(struct VsnPool **)p1;
	struct VsnPool *a2 = *(struct VsnPool **)p2;

	return (strcmp(a1->VpName, a2->VpName));
}
