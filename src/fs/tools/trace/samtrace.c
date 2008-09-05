/*
 * samtrace.c  - Dump SAM-FS trace buffer.
 *
 * Samtrace dumps the contents of the trace buffer for the
 * mounted SAM-FS file system.
 *
 * Syntax:
 *	samtrace [-d corefile -n namelist] [-k #] [-O file] [-I file] \
 *				[-b bufs] -c -f [-i file] [-p seconds]
 *				[-t] [-s] [-v] [-V] [-T ticks]
 *
 * where:
 *  -d corefile The name of the corefile containing an
 *    image of system memory.  If none is
 *    specified the default is to use the /dev/mem
 *    and /dev/kmem files for the running system.
 *
 *  -n namelist The name of the namelist file corresponding
 *    to the corefile.  If none is specified
 *    the default is to use /dev/ksyms from the
 *    running system.
 *
 *  -k # Sets corefile and namelist to unix.# and vmcore.#.
 *
 *  -s Dumps the sam-amld command queue.  Includes -v output.
 *
 *  -v verbose option, excluding inode free and hash chains.
 *
 *  -V Verbose option, including inode free and hash chains.
 *		Includes -v output.
 *
 *  -t Suppress trace output.  Only table pointers are printed.
 *		Typically -t would be used along with -v or -V.
 *
 *  -f Decode flags in trace.
 *
 *  -O file Output raw, unformatted trace data to file.
 *
 *  -I file Read raw, unformatted trace data from file.
 *
 *
 * Continuous trace options:
 *
 *  -c file	continuous trace
 *		[incompatible w/ -d, -n, -k, -s, -v, -v, -I, -O]
 *		--> Not available on some Linux platforms
 *
 *  -b #	allocate # trace bufs per CPU (3 <= # <= 64, default=5)
 *
 *  -i file	read continuous raw trace file (and output text)
 *
 *  -p #	stop continuous tracing after # seconds
 *
 *  -T #	query for trace buffers every # ticks
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

#pragma ident "$Revision: 1.167 $"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#ifdef sun
#include <kvm.h>
#endif
#if (KERNEL_MAJOR < 5)
#include <nlist.h>
#endif
#include <string.h>
#include <strings.h>
#include <time.h>

#include <sys/types.h>
#include <sys/time.h>
#ifdef sun
#include <thread.h>
#include <synch.h>
#include <sys/processor.h>
#include <sys/procset.h>
#include <sys/buf.h>
#include <sys/vfs.h>
#include <sys/rwlock.h>
#endif

#include <pthread.h>

#ifdef sun
#include <sys/vnode.h>
#endif
#include <sys/mman.h>

#ifdef linux
#include <getopt.h>
#endif

#define	NEEDMSGS 1
#include "trace.h"
#undef NEEDMSGS

#ifdef sun
#define	_KERNEL
#include <sys/dnlc.h>
#undef _KERNEL
#include <sys/priocntl.h>
#include <sys/rtpriocntl.h>
#endif

#ifdef linux
#include "sam/samioc.h"
#endif

#include "sam/syscall.h"
#include "sam/types.h"
#include "sam/lib.h"

#define	SAM_QUOTA_KERNEL_STRUCTURES
#include "inode.h"
#include "mount.h"
#ifdef sun
#include "quota.h"
#endif
#include "amld.h"
#include "global.h"
#include "scd.h"
#include "samhost.h"

#define	MUTEX_OWNER_PTR(owner_lock) (owner_lock)

#if !defined(S_ISREG)
#include <pub/stat.h>
#endif

#ifdef linux
/*
 * Kernel mode flags used by the SAM-QFS protocol
 */
#define	FREAD   0x01
#define	FWRITE  0x02
#endif


#if !defined(_kernel)
#define	SAM_ITOV(ip)		((vnode_t *)(ip)->vnode)
#endif

#ifndef O_LARGEFILE
#define	O_LARGEFILE	0
#endif

struct bittext {
	uint64_t mask;
	char *s;
};
#ifdef sun
uint32_t num_duplist;		/* max # of inodes for dup list */
uint32_t dup_length;
struct dup_chk {
	dev_t	dev;
	sam_ino_t ino;
} *dup_list;

static void print_global_tbl(sam_global_tbl_t *gt, sam_global_tbl_t *addr);
static int set_realtime_priority(void);
static int dup_inode_check(dev_t dev, sam_ino_t ino);
#else
static void print_global_tbl(trace_global_tbl_t *gt, trace_global_tbl_t *addr);
#endif
static void print_mount(sam_mount_t *mount, sam_mount_t *addr);
#ifdef sun
static void print_vfs(vfs_t *vfs, vfs_t *addr);
static void print_samamld_tbl(samamld_cmd_table_t *si,
				samamld_cmd_table_t *addr);
static void print_chains(sam_global_tbl_t *gt, sam_global_tbl_t *addr);
#endif
static void print_sam_scd_table(struct sam_syscall_daemon *tbl,
				struct sam_syscall_daemon *addr);
static void print_sam_trace(sam_trace_tbl_t **trace_tables, int num_cpus);
static void print_trace_message(int event, sam_tr_t p1, sam_tr_t p2,
				sam_tr_t p3);
static void dump_sam_trace(char *lfn, sam_trace_tbl_t **trace_tables,
				int num_cpus);
static void undump_sam_trace(char *lfn);
static void strip_decode(char *msg, char *buf);
static void do_decode(char *msg, char *buf, sam_tr_t p1, sam_tr_t p2,
				sam_tr_t p3);
static void do_decode_one(char **msgp, char **pp, uint64_t value);
static char *decode_d(uint64_t value);
static char *decode_e(uint64_t value);
static char *shorterror(uint64_t value);
static char *decodemask(uint64_t value, sam_flagtext_t *fltextp);
static char *decode_m(uint64_t value);
static char *decode_M(uint64_t value);
static char *decode_o(uint64_t value);
static char *decode_f(uint64_t value);
static char *decode_F(uint64_t value);
static char *decode_s(uint64_t value);
static char *decode_S(uint64_t value);
static char *decode_l(uint64_t value);
static char *decode_L(uint64_t value);
static char *decode_c(uint64_t value);
static char *decode_a(uint64_t value);
#ifdef sun
static char *decode_t(uint64_t value);
static char *decode_T(uint64_t value);
static char *decode_p(uint64_t value);
#endif
static char *decode_P(uint64_t value);
static char *decode_r(uint64_t value);
#ifdef sun
static char *decode_R(uint64_t value);
#endif
static char *decode_q(uint64_t value);
static char *decode_z(uint64_t value);

static char *decode_fi_config(uint64_t value);

static void print_quota(sam_mount_t *mount);
static void print_quota_entry(struct sam_quot *qp);
static void quota_flags_cvt(int flags, char *buf);

static void sam_continuous_trace(int cpus, int tracebytes, char *lfn);
static void sam_continuous_xlate(char *lfn);


char *program_name = "samtrace";   /* Used by error() */

#ifdef linux
int	kvm_fd;
#else
kvm_t	*kvm_fd;			/* Kernel memory file descriptor */
#endif
samamld_cmd_table_t	samsi;	/* samamld_cmd table */
struct sam_syscall_daemon sam_scd_table[SCD_MAX];
sam_trace_tbl_t	**traces;	/* SAM-FS trace table */
sam_trace_tbl_t	**trace_addrs;
int	trace_count;


/*
 * Values related to the number of per-CPU trace buffers allocated
 * for continuous trace.
 */
#define			TR_NBUF_MIN		3
#define			TR_NBUF_DEF		5
#define			TR_NBUF_MAX		64

int	sam_amld = 0;		/* lowercase s sam_amld cmd queue */
int	verbose = 0;		/* lowercase v medium verbose */
int	Verbose = 0;		/* uppercase V full verbose */
int	tracesuppress = 0;	/* lowercase t no trace */
int	Decode = 0;		/* lowercase f flag decode */
int	Dump = 0;		/* dump disk trace */
int	Undump = 0;		/* read disk trace */
int Cdump = 0;			/* dump continuous trace to disk file */
int Cundump = 0;		/* read continuous trace disk file */
int StopAfter = 0;		/* stop continuous trace when? */
int TraceInterval = 0;		/* set ticks between trace calls */
int TrNbuf = TR_NBUF_DEF;	/* Number of trace buffers/CPU (cont. trace) */

#define		OPT_VB		0x1

/*
 * All the symbol table addresses and values that we need to look
 * up so that we can read the data we want out of the kernel.
 */
#ifdef sun
vfs_t *rootvfs;				/* Root vfs pointer address */
vfsops_t *samfs_vfsops;			/* SAM-FS operations table */
#endif
sam_trace_tbl_t **samfs_trace;		/* SAM-FS trace table header address */
int trace_cpus;				/* Number of SAM-FS trace tables */
int trace_bytes;			/* Size of SAM-FS trace table */
hrtime_t trace_scale;			/* Time scale for traces */
#ifdef sun
sam_global_tbl_t *gt;			/* SAM-FS global table (address) */
sam_global_tbl_t samgt;			/* SAM-FS global table */
#else
trace_global_tbl_t *gt;			/* SAM-FS global table (address) */
trace_global_tbl_t samgt;		/* SAM-FS global table */
#endif
samamld_cmd_table_t *si;		/* samamld_cmd table */
struct sam_syscall_daemon *scd;		/* kernel addr of sam_scd_table */
sam_trace_info_t info;			/* for ioctl call out of the kernel */

#ifdef sun
/*
 * A table to drive the initialization of the above variables.
 */
struct syminfo {
	char *name;		/* name of desired symbol */
	int flag;		/* OPT_VB ==> only get if verbose is set */
	void *value;		/* location to put symbol's addr (if !NULL) */
	void *data;		/* location to put symbol's data (if !NULL) */
	uint_t size;		/* size of symbol */
} ksym[] = {
	{ "rootvfs",	0,	NULL,	(void *)&rootvfs, sizeof (rootvfs)},
	{ "samfs_vfsopsp",	0,	(void *)&samfs_vfsops, NULL, 0},
	{ "samfs_trace", 0, NULL, (void *)&samfs_trace, sizeof (samfs_trace)},
	{ "samfs_trace_table_cpus", 0, NULL, (void *)&trace_cpus,
					sizeof (trace_cpus)},
	{ "samfs_trace_table_bytes", 0, NULL, (void *)&trace_bytes,
					sizeof (trace_bytes)},
	{ "samfs_trace_time_scale", 0, NULL, (void *)&trace_scale,
					sizeof (trace_scale)},
	{ "samgt", 0,	(void *)&gt,	(void *)&samgt, sizeof (samgt)},
	{ "si_cmd_table",	0, NULL,	(void *)&si, sizeof (si)},
	{ "sam_scd_table",	OPT_VB, (void *)&scd,	NULL, 0},
	{ NULL, 0, (ulong_t)NULL, NULL, 0}
};
#endif

#ifdef linux
int
#else
void
#endif
main(int argc, char *argv[])
{
#ifdef sun
	struct nlist k_nlist[sizeof (ksym)/sizeof (struct syminfo)];
#endif
	char *ksyms = NULL;			/* Namelist file name */
	char *kmem = NULL;			/* Core file name */
	char *tfile = NULL;			/* Raw trace file name */
	int argerr = 0;				/* Argument error flag */
	int i, n, k;

	/*
	 * Flavors:
	 *	samtrace [-d corefile -n namelist] [-s] [-v] [-V] [-f]
	 *	samtrace -k # [-s] [-v] [-V] [-f]
	 *	samtrace -O file
	 *	samtrace -I file [-f]
	 *	samtrace -c file [-b nbufs] [-p seconds] [-T ticks]
	 *		--> samtrace -c unavailable on some Linux platforms
	 *	samtrace -i file [-f]
	 */

#define	OPT_STR		"b:c:d:fi:I:k:n:O:p:stT:vV"

	while ((k = getopt(argc, argv, OPT_STR)) != EOF) {
		switch (k) {
		/* -b Nbuf [set buffers/CPU for cont. dump] */
		case 'b':
			TrNbuf = atoi(optarg);
			if (TrNbuf < TR_NBUF_MIN || TrNbuf > TR_NBUF_MAX) {
				fprintf(stderr,
				    "%s: -b nbufs: requires %d <= nbufs "
				    "<= %d [def=%d]\n",
				    argv[0], TR_NBUF_MIN, TR_NBUF_MAX,
				    TR_NBUF_DEF);
				exit(1);
			}
			break;
		/* -c file [continuous trace] */
		case 'c':
			Cdump = 1;
			tfile = optarg;
			break;
		/* -i file [format+print from continuous trace] */
		case 'i':
			Cundump = 1;
			tfile = optarg;
			break;
		/* -p # [continuous trace; stop after # seconds] */
		case 'p':
			StopAfter = atoi(optarg);
			break;
		/* -T # [continuous trace; get buf every # ticks] */
		case 'T':
			TraceInterval = atoi(optarg);
			break;
		/* -O file */
		case 'O':
			Dump = 1;
			tfile = optarg;
			break;
		/* -I file */
		case 'I':
			Undump = 1;
			tfile = optarg;
			break;
#ifdef sun
		/* -d corefile */
		case 'd':
			kmem = optarg;
			break;
		/* -n namelist */
		case 'n':
			ksyms = optarg;
			break;
		/* -s implies -v */
		case 's':
			sam_amld = 1;
			verbose = 1;
			break;
#endif
		/* -v */
		case 'v':
			verbose = 1;
			break;
		/* -V implies -v */
		case 'V':
			Verbose = 1;
			verbose = 1;
			break;
		/* -t */
		case 't':
			tracesuppress = 1;
			break;
#ifdef sun
		/* -k */
		case 'k':
			ksyms = malloc(6 + strlen(optarg));
			strcpy(ksyms, "unix.");
			strcat(ksyms, optarg);
			kmem = malloc(8 + strlen(optarg));
			strcpy(kmem, "vmcore.");
			strcat(kmem, optarg);
			break;
#endif
		/* -f */
		case 'f':
			Decode = 1;
			break;
		/* error */
		case '?':
			argerr++;
			break;
		}
	}

	if (argerr || (Dump && Undump) ||
	    (Cdump && (Dump || Undump || Cundump)) ||
	    (Cundump && (Dump || Undump || Cdump)) ||
	    (Dump && (kmem || ksyms || sam_amld || verbose || Decode)) ||
	    (Cdump && (kmem || ksyms || sam_amld || verbose || Decode)) ||
	    (Undump && (kmem || ksyms || sam_amld || verbose)) ||
	    (Cundump && (kmem || ksyms || sam_amld || verbose)) ||
	    ((Cundump || Dump || Undump) && (StopAfter || TraceInterval))) {
		fprintf(stderr, "Usage:\n\t"
		    "%s [-d corefile -n namelist] [-s] [-v] [-V] [-t] [-f]\n",
		    argv[0]);
		fprintf(stderr, "\t%s -k # [-s] [-v] [-V] [-f]\n", argv[0]);
		fprintf(stderr, "\t%s -O file\n", argv[0]);
		fprintf(stderr, "\t%s -I file [-f]\n", argv[0]);
		fprintf(stderr, "\t%s -c file [-b nbufs] [-p seconds] "
		    "[-T ticks]\n",
		    argv[0]);
		fprintf(stderr, "\t%s -i file [-f]\n", argv[0]);
		exit(1);
	}

	if (Undump) {
		undump_sam_trace(tfile);
		exit(0);
	}

	if (Cundump) {
		sam_continuous_xlate(tfile);
		exit(0);
	}

#ifdef linux
	kvm_fd = open(SAMSYS_CDEV_NAME, O_RDONLY);
	if (kvm_fd < 0) {
		printf("Couldn't open %s for reading, is SUNWqfs "
		    "module loaded?\n",
		    SAMSYS_CDEV_NAME);
		exit(1);
	}
	if (sam_syscall(SC_trace_info, &info, sizeof (info))) {
		perror("Sam trace info syscall failed");
		exit(1);
	}
	trace_cpus = info.cpus;
	trace_bytes = info.bytes;
	trace_scale = info.scale;
/*
 *	printf("info: cpus = %d, bytes = %d, scale = %d\n",
 *			trace_cpus, trace_bytes, trace_scale);
 */
	if (sam_syscall(SC_trace_global, &samgt, sizeof (samgt))) {
		perror("Sam trace global syscall failed");
		exit(1);
	}
	gt = 0;
#else
	kvm_fd = kvm_open(ksyms, kmem, NULL, O_RDONLY, program_name);

	if (kvm_fd == NULL) {
		printf("Couldn't open kernel files.\n");
		exit(1);
	}

	/*
	 * Generate a name list from the ksym list of symbols
	 */
	memset(&k_nlist, 0, sizeof (k_nlist));
	for (i = 0; ksym[i].name != NULL; i++) {
		k_nlist[i].n_name = ksym[i].name;
	}
	k_nlist[i].n_name = NULL;

	if (kvm_nlist(kvm_fd, k_nlist) < 0) {
		printf("Namelist lookup error.\n");
		exit(1);
	}

	/*
	 * Got the symbols.  Dispatch the values, and read up the
	 * requested entries and dispose them to their targets.
	 */
	for (i = 0; ksym[i].name != NULL; i++) {
		if (k_nlist[i].n_value == 0) {
			printf("Symbol '%s' not found.\n", ksym[i].name);
			continue;
		}

		if ((ksym[i].flag & OPT_VB) && !verbose) {
			continue;
		}

		if (ksym[i].value != NULL) {
			*(ulong_t *)ksym[i].value = k_nlist[i].n_value;
		}

		if (ksym[i].data != NULL) {
			k = kvm_kread(kvm_fd, k_nlist[i].n_value,
			    (char *)ksym[i].data, ksym[i].size);
			if (k < 0) {
				printf("Couldn't read '%s' value at %lx "
				    "of length %d\n",
				    ksym[i].name, k_nlist[i].n_value,
				    ksym[i].size);
			}
		}
	}
#endif

	if (Cdump) {
		if (StopAfter) {
			alarm(StopAfter);
		}
		sam_continuous_trace(trace_cpus, trace_bytes, tfile);
		/* no return; exit by signal only */
		exit(0);
	}

#ifdef sun
	if (!Dump) {
		printf("rootvfs:\t%8p\n", rootvfs);
		printf("samfs_vfsops:\t%8p\n", samfs_vfsops);
		printf("samfs_trace:\t%8p\n", samfs_trace);
	}
#endif

	if (verbose) {
		sam_mount_t *samfs_mp = NULL;	/* Active mount structs */
		sam_mount_t *samfs_stale = NULL; /* Staled mount structs */
		sam_mount_t *mp;
#ifdef sun
		vfs_t vfs;
#endif

		samfs_mp = samgt.mp_list;
		samfs_stale = samgt.mp_stale;
		print_global_tbl(&samgt, gt);
#ifdef sun
		if (si != NULL) {
			k = kvm_kread(kvm_fd, (ulong_t)si, (char *)&samsi,
			    sizeof (samamld_cmd_table_t));
			if (k < 0) {
				printf("Couldn't read SAM-FS "
				    "samamld_cmd table at %p\n", si);
			} else {
				print_samamld_tbl(&samsi,
				    (samamld_cmd_table_t *)si);
			}
		}

		/*
		 * Filesystem daemon table.
		 */
		if (kvm_kread(kvm_fd, (ulong_t)scd, (char *)sam_scd_table,
		    sizeof (sam_scd_table)) < 0) {
			printf("Couldn't read SAM-FS sam_scd table at %p\n",
			    scd);
		} else {
			print_sam_scd_table(sam_scd_table, scd);
		}
#endif
		if (samfs_mp == NULL) {
			printf("No SAM-FS mount entry found at %p\n",
			    samfs_mp);
		}
#ifdef sun
		while (samfs_mp != NULL) {
			sam_mount_t mount;

			k = kvm_kread(kvm_fd, (ulong_t)samfs_mp,
			    (char *)&mount,
			    sizeof (sam_mount_t));
			if (k < 0) {
				printf("Couldn't read SAM-FS base mount "
				    "table at %p\n",
				    samfs_mp);
				break;
			}
			n = mount.mt.fs_count;
			mp = malloc(sizeof (sam_mount_t) +
			    ((n - 1)*sizeof (struct samdent)));
			if (mp == NULL) {
				printf("Couldn't allocate memory for mount "
				    "table.\n");
				break;
			} else {
				k = kvm_kread(kvm_fd, (ulong_t)samfs_mp,
				    (char *)mp,
				    (sizeof (sam_mount_t) +
				    ((n-1)*sizeof (struct samdent))));
				if (k < 0) {
					printf("Couldn't read SAM-FS mount "
					    "table at %p\n",
					    samfs_mp);
					free(mp);
					break;
				}
			}
			if ((mp->mt.fi_status & FS_MOUNTED) &&
			    mp->mi.m_vfsp != NULL) {
				k = kvm_kread(kvm_fd, (ulong_t)mp->mi.m_vfsp,
				    (char *)&vfs,
				    sizeof (vfs_t));
				if (k < 0) {
					printf("Couldn't read vfs entry "
					    "at %p\n", mp->mi.m_vfsp);
					free(mp);
					break;
				} else if (vfs.vfs_fstype == samgt.fstype) {
					print_vfs(&vfs, mp->mi.m_vfsp);
				}
			}
			print_mount(mp, samfs_mp);
			print_quota(mp);
			samfs_mp = mp->ms.m_mp_next;
			free(mp);
		}
#endif
		if (samfs_stale == NULL) {
			printf("No SAM-FS stale mount entries found\n");
		} else {
			printf("\n        * * * *   STALE MOUNT "
			    "STRUCTS   * * * *\n\n");
		}
#ifdef sun
		while (samfs_stale != NULL) {
			sam_mount_t mount;

			k = kvm_kread(kvm_fd, (ulong_t)samfs_stale,
			    (char *)&mount,
			    sizeof (sam_mount_t));
			if (k < 0) {
				printf("Couldn't read SAM-FS stale mount "
				    "table at %p\n",
				    samfs_stale);
				break;
			}
			n = mount.mt.fs_count;
			mp = malloc(sizeof (sam_mount_t) +
			    ((n - 1)*sizeof (struct samdent)));
			if (mp == NULL) {
				printf("Couldn't allocate memory for stale "
				    "mount table.\n");
				break;
			} else {
				k = kvm_kread(kvm_fd, (ulong_t)samfs_stale,
				    (char *)mp,
				    (sizeof (sam_mount_t) +
				    ((n-1)*sizeof (struct samdent))));
				if (k < 0) {
					printf("Couldn't read SAM-FS stale "
					    "mount table at %p\n",
					    samfs_stale);
					free(mp);
					break;
				}
			}
			if (mp->mi.m_vfsp != NULL) {
				k = kvm_kread(kvm_fd, (ulong_t)mp->mi.m_vfsp,
				    (char *)&vfs,
				    sizeof (vfs_t));
				if (k < 0) {
					printf("Couldn't read vfs entry "
					    "at %p\n", mp->mi.m_vfsp);
					free(mp);
					break;
				} else if (vfs.vfs_fstype == samgt.fstype) {
					print_vfs(&vfs, mp->mi.m_vfsp);
				}
			}
			print_mount(mp, samfs_stale);
			print_quota(mp);
			samfs_stale = mp->ms.m_mp_next;
			free(mp);
		}
#endif
	}

	if (trace_bytes < sizeof (sam_trace_tbl_t)) {
		printf("SAM-FS trace table size of %d is too small!\n",
		    trace_bytes);
		exit(1);
	}

	if (trace_cpus <= 0) {
		printf("SAM-FS trace cpu count of %d is too small!\n",
		    trace_cpus);
		exit(1);
	}

	trace_addrs = malloc(trace_cpus * sizeof (sam_trace_tbl_t *));
	if (trace_addrs == NULL) {
		printf("Couldn't allocate memory for SAM-FS trace header "
		    "(%d bytes)\n",
		    trace_cpus * sizeof (sam_trace_tbl_t *));
		exit(1);
	}

	traces = malloc(trace_cpus * sizeof (sam_trace_tbl_t *));
	if (traces == NULL) {
		printf("Couldn't allocate memory for SAM-FS trace table "
		    "(%d bytes)\n",
		    trace_cpus * sizeof (sam_trace_tbl_t *));
		exit(1);
	}

#ifdef linux
	if (sam_syscall(SC_trace_addr_data, trace_addrs,
	    sizeof (sam_trace_tbl_t *)*trace_cpus)) {
		perror("Sam trace addr data syscall failed");
		exit(1);
	}
#else
	k = kvm_kread(kvm_fd, (ulong_t)samfs_trace, (char *)trace_addrs,
	    trace_cpus * sizeof (sam_trace_tbl_t *));
	if (k < 0) {
		printf("Couldn't read SAM-FS trace header at %p\n",
		    samfs_trace);
		exit(1);
	}
#endif

	{
		int i, j;

		/*
		 * Preallocate space so that we can capture the traces more
		 * quickly.  This reduces the chances that we'll get
		 * non-overlapping traces on a busy system.
		 */

		j = 0;

		for (i = 0; i < trace_cpus; i++) {
			if (trace_addrs[i] != NULL) {
				traces[j] = malloc(trace_bytes);
				if (traces[j] == NULL) {
					printf("Couldn't allocate %d bytes "
					    "for trace table %d\n",
					    trace_bytes, j);
					exit(1);
				}
				bzero(traces[j], trace_bytes);

				j++;
			}
		}

		j = 0;

		for (i = 0; i < trace_cpus; i++) {
			if (trace_addrs[i] != NULL) {
#ifdef linux
				traces[j]->t_cpu = i;
				if (sam_syscall(SC_trace_tbl_data,
				    traces[j], trace_bytes)) {
					perror("Sam trace tbl data syscall "
					    "failed");
					exit(1);
				}
#else
				k = kvm_kread(kvm_fd,
				    (ulong_t)trace_addrs[i], traces[j],
				    trace_bytes);
				if (k < 0) {
					printf("Couldn't read trace table "
					    "at %p\n", trace_addrs[i]);
					exit(1);
				}
#endif

				j++;
			}
		}

		trace_count = j;
	}

	if (!tracesuppress) {
		if (trace_count != 0) {
			if (Dump) {
				dump_sam_trace(tfile, traces, trace_count);
			} else {
				print_sam_trace(traces, trace_count);
			}
		} else {
			printf("Trace buffer is empty.\n");
		}
	}

#ifdef linux
	close(kvm_fd);
#else
	kvm_close(kvm_fd);
#endif
	exit(0);
}


/*
 * ---- print_trace - Print SAM-FS trace buffer.
 *
 * Print the contents of the SAM-FS trace buffer.
 *
 * We have one trace buffer for each CPU.  Each buffer is ordered by
 * time, but there is no relative ordering, so we need to merge them.
 *
 * Typically we expect 'runs' of several events on one CPU, rather than fully
 * random interleaving.  We can use this to improve the speed of the merge,
 * by remembering which buffer we most recently used, as well as the minimum
 * timestamp across all remaining buffers.
 */

#define	EARLIER(l, r)							\
	(((l)->secs < (r)->secs) || (((l)->secs == (r)->secs) &&	\
		((l)->nsecs < (r)->nsecs)))

static void
print_sam_trace(
	sam_trace_tbl_t **trace_tables,  /* SAM-FS trace table */
	int num_cpus)
{
	int i, cpu;
	struct trace_index {
		int next;
		struct sns {
			uint32_t secs;
			uint32_t nsecs;
		} next_time;
		boolean_t used;
		boolean_t finished;
	} *index;
	int min_index, next_index;

	/*
	 * Allocate memory for housekeeping.  For each CPU, we keep track of
	 * where we are in its trace ("next"), and the timestamp associated
	 * with that trace entry ("next_time").
	 */

	index = malloc(num_cpus * sizeof (struct trace_index));
	if (index == NULL) {
		printf("Couldn't allocate %d bytes for internal trace "
		    "table index\n",
		    num_cpus * sizeof (struct trace_index));
		exit(1);
	}

	/*
	 * Find the beginning of the trace.  Compute the scaling factor for
	 * subsequent events.  Note that "beginning" here is the minimum across
	 * all trace buffers.
	 */

	{
		long start;

		start = trace_tables[0]->t_basetime.tv_sec;
		for (cpu = 1; cpu < num_cpus; cpu++) {
			if (trace_tables[cpu]->t_basetime.tv_sec < start) {
				start = trace_tables[cpu]->t_basetime.tv_sec;
			}
		}

		printf("Trace begins at: %s", ctime(&start));
		printf("date  hh:mm:dd.ms  p thread   m vfsp      "
		    "event     message\n");

	}

	/*
	 * Set up our index across all traces.
	 *
	 * Each CPU's trace is a circular buffer, starting at t_in.  Entries
	 * which have not yet been filled will have a time of zero; skip past
	 * these.
	 */

	for (cpu = 0; cpu < num_cpus; cpu++) {
		int in;

		in = trace_tables[cpu]->t_in;
		i = 0;
		while (trace_tables[cpu]->t_ent[in].t_time == 0) {
			in = (in + 1) % trace_tables[cpu]->t_limit;
			if (i++ > trace_tables[cpu]->t_limit) {
				in = -1;	/* mark as finished */
				break;		/* out of while loop */
			}
		}
		index[cpu].next = in;
		if (in >= 0) {
			index[cpu].used = 0;
			index[cpu].finished = 0;
		} else {
			index[cpu].used = 1;
			index[cpu].finished = 1;
		}
	}

	/*
	 * Normalize all times.  We replace the hrtime_t found in each trace
	 * entry with a <seconds,nanoseconds> pair in absolute time.  This
	 * makes our comparisons a lot easier as we do the merge later on.
	 */

	{
		double scale;

		scale = trace_scale / 1.0E9;

		for (cpu = 0; cpu < num_cpus; cpu++) {
			if (!index[cpu].finished) {
				sam_trace_tbl_t *tb;
				int l;

				tb = trace_tables[cpu];
				l = tb->t_limit;
				for (i = 0; i < l; i++) {
					hrtime_t *pt, t;
					struct sns *ps;
					long long event_raw_nsecs;
					long event_delta_secs;
					long event_delta_nsecs;
					long event_abs_secs, event_abs_nsecs;

					pt = &tb->t_ent[i].t_time;
					ps = (struct sns *)pt;

					t = *pt;

					if (t == 0) {
						/*
						 * End of buf, done with
						 * for loop.
						 */
						break;
					}

					/*
					 * abs time = base time + (event
					 * ticks - base ticks) * scale.  Note
					 * that 'event ticks - base ticks'
					 * can be negative since we reset the
					 * baseline each time we wrap around.
					 */

					event_raw_nsecs = (long long)((t -
					    tb->t_baseticks) *
					    scale);
					if (event_raw_nsecs >= 0) {
						event_delta_secs =
						    event_raw_nsecs /
						    1000000000;
						event_delta_nsecs =
						    event_raw_nsecs %
						    1000000000;
					} else {
						event_delta_secs =
						    event_raw_nsecs /
						    1000000000;
						event_delta_nsecs = -
						    (-event_raw_nsecs %
						    1000000000);
					}

					event_abs_secs =
					    tb->t_basetime.tv_sec +
					    event_delta_secs;
#ifdef sun
					event_abs_nsecs =
					    tb->t_basetime.tv_nsec
					    + event_delta_nsecs;
#elif defined(linux)
					event_abs_nsecs =
					    (tb->t_basetime.tv_usec * 1000)
					    + event_delta_nsecs;
#endif
					if (event_abs_nsecs < 0) {
						event_abs_secs--;
						event_abs_nsecs += 1000000000;
					} else if (event_abs_nsecs >
					    1000000000) {
						event_abs_secs++;
						event_abs_nsecs -= 1000000000;
					}

					ps->secs = event_abs_secs;
					ps->nsecs = event_abs_nsecs;
				}
			}
		}
	}

	/*
	 * Get the starting time for each buffer.
	 */

	{
	for (cpu = 0; cpu < num_cpus; cpu++) {
		if (!index[cpu].finished) {
			index[cpu].next_time = *(struct sns *)
			    &trace_tables[cpu]->t_ent[index[cpu].next].t_time;
		}
	}
	}

	/*
	 * Find the minimum, and next-to-minimum, times across all buffers.
	 * We could do this in a single pass, but since it only runs once,
	 * it's a lot easier to code and understand as a two-pass process.
	 */

	{
		boolean_t have_min;

		have_min = FALSE;
		min_index = -1;

		/* Pass 1: Identify minimum. */

		for (cpu = 0; cpu < num_cpus; cpu++) {
			if (!index[cpu].finished) {
				if (!have_min) {
					have_min = TRUE;
					min_index = cpu;
				} else {
					if (EARLIER(&index[cpu].next_time,
					    &index[min_index].next_time)) {
						min_index = cpu;
					}
				}
			}
		}

		/* If no minimum found, then no CPUs have trace data. */

		if (!have_min) {
			printf("No trace data found.\n");
			return;
		}

		/* Pass 2: Identify next-to-minimum. */

		have_min = FALSE;
		next_index = -1;

		for (cpu = 0; cpu < num_cpus; cpu++) {
			if ((cpu != min_index) && (!index[cpu].finished)) {
				if (!have_min) {
					have_min = TRUE;
					next_index = cpu;
				} else {
					if (EARLIER(&index[cpu].next_time,
					    &index[next_index].next_time)) {
						next_index = cpu;
					}
				}
			}
		}
	}

	/*
	 * The main loop is essentially a merge of the sorted trace buffers.
	 */

	for (;;) {
		sam_trace_ent_t *t;
		struct tm *tm;
		boolean_t use_next;

		/*
		 * Get the next element from the current-minimum-time trace
		 * buffer.
		 *
		 * If no more, mark this buffer as finished.
		 *
		 * If this trace is later than that in the next-minimum-time
		 * buffer, make the next-min-time buffer current, and
		 * recompute a new next-minimum buffer.
		 */

		if (min_index < 0) {	/* Finished with last trace buffer? */
			break;
		}

		use_next = FALSE;

		t = &trace_tables[min_index]->t_ent[index[min_index].next];

		if (index[min_index].finished) {
			use_next = TRUE;
		} else if (t->t_time == 0) {
			index[min_index].finished = TRUE;
			use_next = TRUE;
		} else if (next_index >= 0) {
			if (EARLIER(&index[next_index].next_time,
						(struct sns *)(&t->t_time))) {
				use_next = TRUE;
			}
		}

		if (use_next) {
			boolean_t have_min;

			index[min_index].next_time = *((struct sns *)t);
			min_index = next_index;

			have_min = FALSE;
			next_index = -1;

			for (cpu = 0; cpu < num_cpus; cpu++) {
				if ((cpu != min_index) &&
				    (!index[cpu].finished)) {
					if (!have_min) {
						have_min = TRUE;
						next_index = cpu;
					} else {
					if (EARLIER(&index[cpu].next_time,
					    &index[next_index].next_time)) {
						next_index = cpu;
					}
					}
				}
			}

			/* ret to top of loop to process next buffer */
			continue;
		}

		/*
		 * If this is the first time we've printed an entry from this
		 * CPU's trace buffer, print a line in the trace to make it
		 * clear that any earlier events are not included.
		 */

		if (!index[min_index].used) {
			printf("(CPU %d trace begins)\n",
			    trace_tables[min_index]->t_cpu);
			index[min_index].used = TRUE;
		}

		{
			time_t tsecs;

			tsecs = ((struct sns *)t)->secs;
			tm = localtime(&tsecs);
		}
		{
			long msecs = ((struct sns *)t)->nsecs/1000000;
			printf("%02d/%02d %02d:%02d:%02d.%03ld ",
			    (tm->tm_mon + 1), tm->tm_mday,
			    tm->tm_hour, tm->tm_min, tm->tm_sec,
			    msecs);
		}

		printf("%x=%p ", trace_tables[min_index]->t_cpu, t->t_thread);
		printf("%x-%012p%c ", t->t_mount, t->t_addr,
		    (int)t->t_event >= T_SAM_MAX_VFS ? ' ' : '*');

		print_trace_message(t->t_event, t->t_p1, t->t_p2, t->t_p3);

		printf("\n");

		/* Advance ptr into trace buffer, and check for end of buf. */

		index[min_index].next =
		    (index[min_index].next + 1) %
		    trace_tables[min_index]->t_limit;

		if (index[min_index].next == trace_tables[min_index]->t_in) {
			index[min_index].finished = TRUE;
		}
	}
}

#undef EARLIER

/*
 * ---- print_trace_message - Print trace message.
 *  Print the trace message in decoded form.
 */

static void
print_trace_message(
	int event,
	sam_tr_t p1,
	sam_tr_t p2,
	sam_tr_t p3)
{
	char *msg;
	char buf[8192];	/* want this really big to allow lots of decode! */

	if (event > T_SAM_MAX) {
		printf("Unknown entry=%x: p1=" SAMTRACEX ", p2="
		    SAMTRACEX ", p3="
		    SAMTRACEX "\n",	event, p1, p2, p3);
		return;
	}

	msg = sam_trace_msg[event];

	/*
	 * Messages with a string encode the string within the parameters.
	 */

	if (strstr(msg, "%s") != NULL) {
		union {
			struct {
				sam_tr_t p1;
				sam_tr_t p2;
				sam_tr_t p3;
			} p;
			char s[25];	/* one extra byte for a terminator */
		} u;

		u.p.p1 = p1;
		u.p.p2 = p2;
		u.p.p3 = p3;
		u.s[24] = '\0';
		printf(msg, u.s);
		return;
	}

	/*
	 * If no ^ found in string, no special decoding required.
	 */

	if (strchr(msg, '^') == NULL) {
		printf(msg, p1, p2, p3);
		return;
	}

	/*
	 * If flag decoding is disabled, just strip the decode bits and print
	 * the message.
	 */

	if (!Decode) {
		strip_decode(msg, buf);
		printf(buf, p1, p2, p3);
		return;
	}

	/*
	 * Decode and print the string.
	 */

	do_decode(msg, buf, p1, p2, p3);
	printf(buf, p1, p2, p3);
}

/*
 * ---- strip_decode - Strip extra decoding bits from a message.
 */

static void
strip_decode(
	char *msg,
	char *buf)
{
	char *p;

	p = buf;
	while (*msg != '\0') {
		if (*msg != '^') {
			*p++ = *msg++;
			continue;
		}
		if (*++msg == '^') {
			msg++;
			*p++ = '^';
		} else {
			if (*msg != '\0') {
				while ((*msg++ != ']') && (*msg != '\0'))
					;
			}
		}
	}
	*p = '\0';
}


/*
 * ---- do_decode - Go through a message string, substituting decoded values.
 *
 * Go through the string, looking for any occurrence of ^.
 * Count occurrences of non-doubled % as we go.
 *
 * When we find ^, a doubled ^^ turns into a single ^.
 *
 * Otherwise, we expect to find either ^[...] or ^#[...] where # is 1-3.
 *   (other forms are invalid)
 *
 * Replace this construct.  Any characters other than $ are passed
 * through unchanged.  A doubled $$ becomes a single $.  Otherwise
 * we expect to find $x where x is a decoding key.  Replace $x by
 * the result of decoding the parameter whose index is either # or
 * the number of non-doubled % found in the string.
 *
 * When we are all done, we will have a new string which gets passed
 * through printf as usual.
 */

static void
do_decode(
	char *msg,
	char *buf,
	sam_tr_t p1,
	sam_tr_t p2,
	sam_tr_t p3)
{
	char *p;
	int param;
	int seen_one;

	p = buf;
	param = 0;
	seen_one = 0;
	while (*msg != '\0') {
		char c;

		if (*msg != '^') {
			if (*msg == '%') {
				if (!seen_one) {
					param++;
				} else {
					param--;
				}
				seen_one ^= 1;
			} else {
				seen_one = 0;
			}
			*p++ = *msg++;
			continue;
		}

		c = *++msg;
		if (c == '^') {
			msg++;
			*p++ = '^';
		} else if (c == '\0') {
			break;
		} else {
			char index;
			sam_tr_t value;

			if (c != '[') {
				index = c - '0';
				c = *++msg;
			} else {
				index = (char)param;
			}

			/*
			 * Now:
			 *   msg points at the hopefully-present '['
			 *   p points to the end of the buffer
			 *   index contains which parameter to substitute
			 */

			switch (index) {
			case 1:
				value = p1;
				break;
			case 2:
				value = p2;
				break;
			case 3:
				value = p3;
				break;
			default:
				value = -1;
				break;
			}

			do_decode_one(&msg, &p, value);
		}
	}
	*p = '\0';
}

static void
do_decode_one(
	char **msgp,
	char **pp,
	uint64_t value)
{
	char *msg, *p;

	msg = *msgp;
	p = *pp;

	if (*msg != '[') {
		printf("BADLY FORMED MESSAGE\n");
		return;
	}

	msg++;

	while (*msg != ']') {
		char *sub;

		if (*msg != '$') {
			*p++ = *msg++;
			continue;
		}
		msg++;
		switch (*msg++) {
		case 'd':
			sub = decode_d(value);
			break;
		case 'e':
			sub = decode_e(value);
			break;
		case 'm':
			sub = decode_m(value);
			break;
		case 'M':
			sub = decode_M(value);
			break;
		case 'o':
			sub = decode_o(value);
			break;
		case 'f':
			sub = decode_f(value);
			break;
		case 'F':
			sub = decode_F(value);
			break;
		case 's':
			sub = decode_s(value);
			break;
		case 'S':
			sub = decode_S(value);
			break;
		case 'l':
			sub = decode_l(value);
			break;
		case 'L':
			sub = decode_L(value);
			break;
		case 'c':
			sub = decode_c(value);
			break;
		case 'a':
			sub = decode_a(value);
			break;
#ifdef sun
		case 't':
			sub = decode_t(value);
			break;
		case 'T':
			sub = decode_T(value);
			break;
		case 'p':
			sub = decode_p(value);
			break;
#endif
		case 'P':
			sub = decode_P(value);
			break;
		case 'r':
			sub = decode_r(value);
			break;
#ifdef sun
		case 'R':
			sub = decode_R(value);
			break;
#endif
		case 'z':
			sub = decode_z(value);
			break;
		case 'q':
			sub = decode_q(value);
			break;
		default:
			sub = "[BAD DECODE KEY]";
			break;
		}
		strcpy(p, sub);
		p += strlen(sub);
	}

	*msgp = ++msg;
	*pp = p;
}

static char *
decode_d(uint64_t value)
{
	switch (value) {
	case 0: return (" DOK");
	case 1: return (" DNOCACHE");
	case 2: return (" DFOUND");
	case 3: return (" DNOENT");
	case 4: return (" DTOOBIG");
	case 5: return (" DNOMEM");
	default: return (" ???");
	}
}

static char *
decode_e(uint64_t value)
{
	static char buf[256];

	if (value == 0)
		return ("");

	buf[0] = ' ';
	strcpy(buf+1, shorterror(value));	/* could use strerror */
	return (buf);
}

static char *
shorterror(uint64_t value)
{
	static struct tt {
		char *s;
		int v;
	} tbl[] = {
		{"EPERM", 1},
		{"ENOENT", 2},
		{"ESRCH", 3},
		{"EINTR", 4},
		{"EIO", 5},
		{"ENXIO", 6},
		{"E2BIG", 7},
		{"ENOEXEC", 8},
		{"EBADF", 9},
		{"ECHILD", 10},
		{"EAGAIN", 11},
		{"ENOMEM", 12},
		{"EACCES", 13},
		{"EFAULT", 14},
		{"ENOTBLK", 15},
		{"EBUSY", 16},
		{"EEXIST", 17},
		{"EXDEV", 18},
		{"ENODEV", 19},
		{"ENOTDIR", 20},
		{"EISDIR", 21},
		{"EINVAL", 22},
		{"ENFILE", 23},
		{"EMFILE", 24},
		{"ENOTTY", 25},
		{"ETXTBSY", 26},
		{"EFBIG", 27},
		{"ENOSPC", 28},
		{"ESPIPE", 29},
		{"EROFS", 30},
		{"EMLINK", 31},
		{"EPIPE", 32},
		{"EDOM", 33},
		{"ERANGE", 34},
		{"ENOMSG", 35},
		{"EIDRM", 36},
		{"ECHRNG", 37},
		{"EL2NSYNC", 38},
		{"EL3HLT", 39},
		{"EL3RST", 40},
		{"ELNRNG", 41},
		{"EUNATCH", 42},
		{"ENOCSI", 43},
		{"EL2HLT", 44},
		{"EDEADLK", 45},
		{"ENOLCK", 46},
		{"ECANCELED", 47},
		{"ENOTSUP", 48},
		{"EDQUOT", 49},
		{"EBADE", 50},
		{"EBADR", 51},
		{"EXFULL", 52},
		{"ENOANO", 53},
		{"EBADRQC", 54},
		{"EBADSLT", 55},
		{"EDEADLOCK", 56},
		{"EBFONT", 57},
		{"EOWNERDEAD", 58},
		{"ENOTRECOVERABLE", 59},
		{"ENOSTR", 60},
		{"ENODATA", 61},
		{"ETIME", 62},
		{"ENOSR", 63},
		{"ENONET", 64},
		{"ENOPKG", 65},
		{"EREMOTE", 66},
		{"ENOLINK", 67},
		{"EADV", 68},
		{"ESRMNT", 69},
		{"ECOMM", 70},
		{"EPROTO", 71},
		{"ELOCKUNMAPPED", 72},
		{"ENOTACTIVE", 73},
		{"EMULTIHOP", 74},
		{"EBADMSG", 77},
		{"ENAMETOOLONG", 78},
		{"EOVERFLOW", 79},
		{"ENOTUNIQ", 80},
		{"EBADFD", 81},
		{"EREMCHG", 82},
		{"ELIBACC", 83},
		{"ELIBBAD", 84},
		{"ELIBSCN", 85},
		{"ELIBMAX", 86},
		{"ELIBEXEC", 87},
		{"EILSEQ", 88},
		{"ENOSYS", 89},
		{"ELOOP", 90},
		{"ERESTART", 91},
		{"ESTRPIPE", 92},
		{"ENOTEMPTY", 93},
		{"EUSERS", 94},
		{"ENOTSOCK", 95},
		{"EDESTADDRREQ", 96},
		{"EMSGSIZE", 97},
		{"EPROTOTYPE", 98},
		{"ENOPROTOOPT", 99},
		{"EPROTONOSUPPORT", 120},
		{"ESOCKTNOSUPPORT", 121},
		{"EOPNOTSUPP", 122},
		{"EPFNOSUPPORT", 123},
		{"EAFNOSUPPORT", 124},
		{"EADDRINUSE", 125},
		{"EADDRNOTAVAIL", 126},
		{"ENETDOWN", 127},
		{"ENETUNREACH", 128},
		{"ENETRESET", 129},
		{"ECONNABORTED", 130},
		{"ECONNRESET", 131},
		{"ENOBUFS", 132},
		{"EISCONN", 133},
		{"ENOTCONN", 134},
		{"ESHUTDOWN", 143},
		{"ETOOMANYREFS", 144},
		{"ETIMEDOUT", 145},
		{"ECONNREFUSED", 146},
		{"EHOSTDOWN", 147},
		{"EHOSTUNREACH", 148},
		{"EALREADY", 149},
		{"EINPROGRESS", 150},
		{"ESTALE", 151},
	};
	int i;

	for (i = 0; i < sizeof (tbl)/sizeof (struct tt); i++) {
		if (value == tbl[i].v) {
			return (tbl[i].s);
		}
	}

	return ("(UNKNOWN ERROR)");
}

static char *
decodemask(
	uint64_t value,
	sam_flagtext_t *fltextp)
{
	int i;
	int first;
	static char buf[8192];

	strcpy(buf, " [");
	first = 1;
	for (i = 63; i >= 0; i--) {
		uint64_t m;

		m = (((uint64_t)1) << i);
		if (value & m) {
			int j, done;
			sam_flagtext_t *bb;

			done = 0;
			for (j = 0, bb = fltextp; !done; j++, bb++) {
				if (bb->flagmask == 0) {
					done = 1;
				} else if (bb->flagmask == m) {
					if (!first) {
						strcat(buf, " ");
					} else {
						first = 0;
					}
					strcat(buf, bb->flagtext);
					value &= ~m;
					break;
				}
			}
		}
	}

	if (value != 0) {
		char left[18];

		if (value & ~0xFFFFFFFF) {
			sprintf(left, "$%16llx", value);
		} else {
			sprintf(left, "$%08llx", value);
		}
		if (!first) {
			strcat(buf, " ");
		}
		strcat(buf, left);
	}

	strcat(buf, "]");
	return (buf);
}

static char *
decode_m(uint64_t value)
{
	static char buf[20];

	sprintf(buf, " (0%03llo)", value);	/* should we decode further? */
	return (buf);
}

static char *
decode_M(uint64_t value)
{
	static char buf[256];
	int k;
	char *kind;

	k = (value >> 12) & 0xF;

	switch (k) {
	case 0: kind = "?0"; break;
	case 1: kind = "FIFO"; break;
	case 2: kind = "CHR"; break;
	case 3: kind = "?3"; break;
	case 4: kind = "dir"; break;
	case 5: kind = "?5"; break;
	case 6: kind = "BLK"; break;
	case 7: kind = "?7"; break;
	case 8: kind = "file"; break;
	case 9: kind = "?9"; break;
	case 10: kind = "LNK"; break;
	case 11: kind = "?11"; break;
	case 12: kind = "SOCK"; break;
	case 13: kind = "?13"; break;
	case 14: kind = "REQ"; break;
	case 15: kind = "?15"; break;
	}

	sprintf(buf, " (%s, 0%03llo)", kind, value & 0xFFF);
	return (buf);
}

static char *
decode_o(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ FREAD, "READ" },
		{ FWRITE, "WRITE" },
		{ FNDELAY, "NDELAY" },
		{ FAPPEND, "APPEND" },
#ifdef sun
		{ FSYNC, "SYNC" },
		{ FDSYNC, "DSYNC" },
		{ FRSYNC, "RSYNC" },
		{ FOFFMAX, "OFFMAX" },
#endif
		{ FNONBLOCK, "NONBLOCK" },
#ifdef sun
		{ FCREAT, "CREAT" },
		{ FTRUNC, "TRUNC" },
		{ FEXCL, "EXCL" },
		{ FNOCTTY, "NOCTTY" },
#endif
#ifdef FXATTR
		{ FXATTR, "XATTR" },
#endif
		{ FASYNC, "ASYNC" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_f(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ SAM_BUSY, "BUSY" },
		{ SAM_FREE, "FREE" },
		{ SAM_HASH, "HASH" },
		{ SAM_AP_LEASE, "AP_LEASE" },
		{ SAM_STAGING, "STAGING" },
		{ SAM_UNLOADING, "UNLOADING" },
		{ SAM_WRITE_MODE, "WRITE_MODE" },
		{ SAM_INACTIVATE, "INACTIVATE" },
		{ SAM_PURGE_PEND, "PURGE_PEND" },
		{ SAM_STALE, "STALE" },
		{ SAM_RM_OPENED, "RM_OPENED" },
		{ SAM_STAGE_N, "STAGE_N" },
		{ SAM_NORECLAIM, "NORECLAIM" },
		{ SAM_DIRECTIO, "DIRECTIO" },
		{ SAM_STAGE_ALL, "STAGE_ALL" },
		{ SAM_QWRITE, "QWRITE" },
		{ SAM_NOWAITSPC, "NOWAITSPC" },
		{ SAM_STAGE_P, "STAGE_P" },
		{ SAM_STAGE_PAGES, "STAGE_PAGES" },
		{ SAM_ARCHIVE_W, "ARCHIVE_W" },
		{ SAM_ABR, "ABR" },
		{ SAM_STAGE_DIRECTIO, "STAGE_DIRECTIO" },
		{ SAM_ARCH_DIRECT, "ARCH_DIRECT" },
		{ SAM_HOLD_BLOCKS, "HOLD_BLOCKS" },
		{ SAM_POSITIONING, "POSITIONING" },
		{ SAM_VALID_MTIME, "VALID_MTIME" },
		{ SAM_DIRTY, "DIRTY" },
		{ SAM_UPDATED, "UPDATED" },
		{ SAM_ACCESSED, "ACCESSED" },
		{ SAM_CHANGED, "CHANGED" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}


static char *
decode_F(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ 0x01, "LOOKUP_DIR" },
		{ 0x02, "LOOKUP_XATTR" },
		{ 0x04, "CREATE_XATTR_DIR" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_s(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ 0x80000000, "ACL" },
		{ 0x40000000, "DFACL" },
		{ 0x20000000, "STRIPE_GROUP" },
		{ 0x10000000, "STRIPE_WIDTH" },
		{ 0x08000000, "ARCHIVE_A" },
		{ 0x04000000, "SEG_FILE" },
		{ 0x02000000, "SEG_INO" },
		{ 0x01000000, "WORM_RDONLY" },
		{ 0x00800000, "WORM_ATTR" },
		{ 0x00400000, "INCONSISTENT_ARCHIVE" },
		{ 0x00200000, "DIRECTIO" },
		{ 0x00100000, "CONCURRENT_ARCHIVE" },
		{ 0x00080000, "DIRECT_MAP" },
		{ 0x00040000, "STAGE_FAILED" },
		{ 0x00020000, "SEGMENT" },
		{ 0x00010000, "META" },
		{ 0x00008000, "OFFLINE" },
		{ 0x00004000, "PEXTENTS" },
		{ 0x00002000, "ARCHNODROP" },
		{ 0x00001000, "ARCHDONE" },
		{ 0x00000800, "ON_LARGE" },
		{ 0x00000400, "CS_GEN" },
		{ 0x00000200, "CS_USE" },
		{ 0x00000100, "CS_VAL" },
		{ 0x00000080, "STAGE_ALL" },
		{ 0x00000040, "NOARCH" },
		{ 0x00000020, "BOF_ONLINE" },
		{ 0x00000010, "DAMAGED" },
		{ 0x00000008, "DIRECT" },
		{ 0x00000004, "NODROP" },
		{ 0x00000002, "RELEASE" },
		{ 0x00000001, "REMEDIA" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_S(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ FS_MOUNTED, "MOUNTED" },
		{ FS_MOUNTING, "MOUNTING" },
		{ FS_UMOUNT_IN_PROGRESS, "UMOUNTING" },
		{ FS_SERVER, "SERVER" },
		{ FS_CLIENT, "CLIENT" },
		{ FS_NODEVS, "NODEVS" },
		{ FS_SAM, "SAM" },
		{ FS_LOCK_WRITE, "LOCK_WRITE" },
		{ FS_LOCK_NAME, "LOCK_NAME" },
		{ FS_LOCK_RM_NAME, "LOCK_RM_NAME" },
		{ FS_LOCK_HARD, "LOCK_HARD" },
		{ FS_SRVR_DOWN, "SRVR_DOWN" },
		{ FS_SRVR_BYTEREV, "SRVR_BYTEREV" },
		{ FS_SRVR_DONE, "SRVR_DONE" },
		{ FS_CLNT_DONE, "CLNT_DONE" },
		{ FS_FREEZING, "FREEZING" },
		{ FS_FROZEN, "FROZEN" },
		{ FS_THAWING, "THAWING" },
		{ FS_RESYNCING, "RESYNCING" },
		{ FS_RELEASING, "RELEASING" },
		{ FS_STAGING, "STAGING" },
		{ FS_ARCHIVING, "ARCHIVING" },
		{ FS_OSDT_MOUNTED, "OSDT_MOUNTED" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_l(uint64_t value)
{
	switch (value) {
	case LTYPE_read: return (" READ");
	case LTYPE_write: return (" WRITE");
	case LTYPE_append: return (" APPEND");
	case LTYPE_truncate: return (" TRUNCATE");
	case LTYPE_frlock: return (" FRLOCK");
	case LTYPE_stage: return (" STAGE");
	case LTYPE_open: return (" OPEN");
	case LTYPE_mmap: return (" MMAP");
	default: return (" ???");
	}
}

static char *
decode_L(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ 1 << LTYPE_read, "READ" },
		{ 1 << LTYPE_write, "WRITE" },
		{ 1 << LTYPE_append, "APPEND" },
		{ 1 << LTYPE_truncate, "TRUNCATE" },
		{ 1 << LTYPE_frlock, "FRLOCK" },
		{ 1 << LTYPE_stage, "STAGE" },
		{ 1 << LTYPE_open, "OPEN" },
		{ 1 << LTYPE_mmap, "MMAP" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_c(uint64_t value)
{
	static struct tt {
		int m;
		int n;
		char *s;
	} tbl[] = {
		{ SAM_CMD_MOUNT, MOUNT_init, " MOUNT_init" },
		{ SAM_CMD_MOUNT, MOUNT_status, " MOUNT_status" },
		{ SAM_CMD_MOUNT, MOUNT_failinit, " MOUNT_failinit" },
		{ SAM_CMD_MOUNT, MOUNT_resync, " MOUNT_resync" },
		{ SAM_CMD_MOUNT, MOUNT_failover, " MOUNT_failover" },
		{ SAM_CMD_MOUNT, MOUNT_faildone, " MOUNT_faildone" },
		{ SAM_CMD_MOUNT, MOUNT_config, " MOUNT_config" },
		{ SAM_CMD_LEASE, LEASE_get, " LEASE_get" },
		{ SAM_CMD_LEASE, LEASE_remove, " LEASE_remove" },
		{ SAM_CMD_LEASE, LEASE_reset, " LEASE_reset" },
		{ SAM_CMD_LEASE, LEASE_relinquish, " LEASE_relinquish" },
		{ SAM_CMD_NAME, NAME_create, " NAME_create" },
		{ SAM_CMD_NAME, NAME_remove, " NAME_remove" },
		{ SAM_CMD_NAME, NAME_mkdir, " NAME_mkdir" },
		{ SAM_CMD_NAME, NAME_rmdir, " NAME_rmdir" },
		{ SAM_CMD_NAME, NAME_link, " NAME_link" },
		{ SAM_CMD_NAME, NAME_rename, " NAME_rename" },
		{ SAM_CMD_NAME, NAME_symlink, " NAME_symlink" },
		{ SAM_CMD_NAME, NAME_acl, " NAME_acl" },
		{ SAM_CMD_NAME, NAME_lookup, " NAME_lookup" },
		{ SAM_CMD_INODE, INODE_getino, " INODE_getino" },
		{ SAM_CMD_INODE, INODE_fsync_wait, " INODE_fsync_wait" },
		{ SAM_CMD_INODE, INODE_fsync_nowait, " INODE_fsync_nowait" },
		{ SAM_CMD_INODE, INODE_setabr, " INODE_setabr" },
		{ SAM_CMD_INODE, INODE_setattr, " INODE_setattr" },
		{ SAM_CMD_INODE, INODE_stage, " INODE_stage" },
		{ SAM_CMD_INODE, INODE_cancel_stage, " INODE_cancel_stage" },
		{ SAM_CMD_INODE, INODE_samattr, " INODE_samattr" },
		{ SAM_CMD_INODE, INODE_samarch, " INODE_samarch" },
		{ SAM_CMD_INODE, INODE_samaid, " INODE_samaid" },
		{ SAM_CMD_INODE, INODE_putquota, " INODE_putquota" },
		{ SAM_CMD_BLOCK, BLOCK_getbuf, " BLOCK_getbuf" },
		{ SAM_CMD_BLOCK, BLOCK_fgetbuf, " BLOCK_fgetbuf" },
		{ SAM_CMD_BLOCK, BLOCK_getino, " BLOCK_getino" },
		{ SAM_CMD_BLOCK, BLOCK_getsblk, " BLOCK_getsblk" },
		{ SAM_CMD_BLOCK, BLOCK_vfsstat, " BLOCK_vfsstat" },
		{ SAM_CMD_BLOCK, BLOCK_wakeup, " BLOCK_wakeup" },
		{ SAM_CMD_BLOCK, BLOCK_panic, " BLOCK_panic" },
		{ SAM_CMD_BLOCK, BLOCK_quota, " BLOCK_quota" },
		{ SAM_CMD_BLOCK, BLOCK_vfsstat_v2, " BLOCK_vfsstat_v2" },
		{ SAM_CMD_WAIT, WAIT_lease, " WAIT_lease" },
		{ SAM_CMD_CALLOUT, CALLOUT_action, " CALLOUT_action" },
		{ SAM_CMD_CALLOUT, CALLOUT_stage, " CALLOUT_stage" },
		{ SAM_CMD_CALLOUT, CALLOUT_acl, " CALLOUT_acl" },
		{ SAM_CMD_CALLOUT, CALLOUT_flags, " CALLOUT_flags" },
		{ SAM_CMD_CALLOUT, CALLOUT_relinquish_lease,
			" CALLOUT_relinquish_l" },
		{ SAM_CMD_NOTIFY, NOTIFY_lease, " NOTIFY_lease" },
		{ SAM_CMD_NOTIFY, NOTIFY_lease_expire,
			" NOTIFY_lease_expire" },
		{ SAM_CMD_NOTIFY, NOTIFY_dnlc, " NOTIFY_dnlc" },
		{ SAM_CMD_NOTIFY, NOTIFY_getino, " NOTIFY_getino" },
		{ SAM_CMD_NOTIFY, NOTIFY_panic, " NOTIFY_panic" }
	};

	int i, c, s;

	c = (value >> 16);
	s = (value & 0xFFFF);

	for (i = 0; i < sizeof (tbl)/sizeof (struct tt); i++) {
		if ((c == tbl[i].m) && (s == tbl[i].n)) {
			return (tbl[i].s);
		}
	}

	return (" (UNKNOWN COMMAND)");
}

static char *
decode_a(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ SR_STALE_INDIRECT, "STALE_INDIRECT" },
		{ SR_DIRECTIO_ON, "DIRECTIO_ON" },
		{ SR_SYNC_PAGES, "SYNC_PAGES" },
		{ SR_INVAL_PAGES, "INVAL_PAGES" },
		{ SR_WAIT_LEASE, "WAIT_LEASE" },
		{ SR_WAIT_FRLOCK, "WAIT_FRLOCK" },
		{ SR_SET_SIZE, "SET_SIZE" },
		{ SR_NOTIFY_FRLOCK, "NOTIFY_FRLOCK" },
		{ SR_FORCE_SIZE, "FORCE_SIZE" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

#ifdef sun
static char *
decode_t(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ ATTR_UTIME, "UTIME" },
		{ ATTR_EXEC, "EXEC" },
		{ ATTR_COMM, "COMM" },
		{ ATTR_HINT, "HINT" },
		{ ATTR_REAL, "REAL" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_T(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ AT_TYPE, "TYPE" },
		{ AT_MODE, "MODE" },
		{ AT_UID, "UID" },
		{ AT_GID, "GID" },
		{ AT_FSID, "FSID" },
		{ AT_NODEID, "NODEID" },
		{ AT_NLINK, "NLINK" },
		{ AT_SIZE, "SIZE" },
		{ AT_ATIME, "ATIME" },
		{ AT_MTIME, "MTIME" },
		{ AT_CTIME, "CTIME" },
		{ AT_RDEV, "RDEV" },
		{ AT_BLKSIZE, "BLKSIZE" },
		{ AT_NBLOCKS, "NBLOCKS" },
#ifdef AT_SEQ
		{ AT_SEQ, "SEQ" },
#else
		{ AT_VCODE, "VCODE" },
#endif
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_p(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ B_BUSY, "B_BUSY" },
		{ B_DONE, "B_DONE" },
		{ B_ERROR, "B_ERROR" },
		{ B_PAGEIO, "B_PAGEIO" },
		{ B_PHYS, "B_PHYS" },
		{ B_READ, "B_READ" },
		{ B_WRITE, "B_WRITE" },
#ifdef B_KERNBUF
		{ B_KERNBUF, "B_KERNBUF" },
#endif
		{ B_WANTED, "B_WANTED" },
		{ B_AGE, "B_AGE" },
		{ B_ASYNC, "B_ASYNC" },
		{ B_DELWRI, "B_DELWRI" },
		{ B_STALE, "B_STALE" },
		{ B_DONTNEED, "B_DONTNEED" },
		{ B_REMAPPED, "B_REMAPPED" },
		{ B_FREE, "B_FREE" },
		{ B_INVAL, "B_INVAL" },
		{ B_FORCE, "B_FORCE" },
		{ B_NOCACHE, "B_NOCACHE" },
		{ B_TRUNC, "B_TRUNC" },
		{ B_SHADOW, "B_SHADOW" },
		{ B_RETRYWRI, "B_RETRYWRI" },
#ifdef B_FAILFAST
		{ B_FAILFAST, "B_FAILFAST" },
#endif
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}
#endif

static char *
decode_P(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ PROT_READ, "PROT_READ" },
		{ PROT_WRITE, "PROT_WRITE" },
		{ PROT_EXEC, "PROT_EXEC" },
		{ 0x08, "PROT_USER" },	/* not defined if no _KERNEL */
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

static char *
decode_r(uint64_t value)
{
	switch (value) {
	case 0: return (" READ");
	case 1: return (" WRITE");
	default: return (" ???");
	}
}

#ifdef sun
static char *
decode_R(uint64_t value)
{
	switch (value) {
	case S_OTHER: return (" S_OTHER");
	case S_READ: return (" S_READ");
	case S_WRITE: return (" S_WRITE");
	case S_EXEC: return (" S_EXEC");
	case S_CREATE: return (" S_CREATE");
	default: return (" ???");
	}
}
#endif

static char *
decode_q(uint64_t value)
{
	switch (value) {
	case SAM_READ: return (" READ");
	case SAM_READ_PUT: return (" READ_PUT");
	case SAM_RD_DIRECT_IO: return (" RD_DIRECT_IO");
	case SAM_WRITE: return (" WRITE");
	case SAM_WR_DIRECT_IO: return (" WR_DIRECT_IO");
	case SAM_FORCEFAULT: return (" FORCEFAULT");
	case SAM_WRITE_MMAP: return (" WRITE_MMAP");
	case SAM_WRITE_BLOCK: return (" WRITE_BLOCK");
	case SAM_WRITE_SPARSE: return (" WRITE_SPARSE");
	case SAM_ALLOC_BLOCK: return (" ALLOC_BLOCK");
	case SAM_ALLOC_ZERO: return (" ALLOC_ZERO");
	default: return (" ???");
	}
}

static char *
decode_z(uint64_t value)
{
	return (decode_e(value & 0xFFFF)); /* just ignore error line number */
}

/*
 * The following are used internally, but not available to messages.
 */

#ifdef sun
static char *
decode_fi_config(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ MT_SHARED_MO, "MT_SHARED_MO" },
		{ MT_MH_WRITE, "MT_MH_WRITE" },
		{ MT_SAM_ENABLED, "MT_SAM_ENABLED" },
		{ MT_TRACE, "MT_TRACE" },
		{ MT_QWRITE, "MT_QWRITE" },
		{ MT_DIRECTIO, "MT_DIRECTIO" },
		{ MT_SOFTWARE_RAID, "MT_SOFTWARE_RAID" },
		{ MT_SHARED_WRITER, "MT_SHARED_WRITER" },
		{ MT_SHARED_READER, "MT_SHARED_READER" },
		{ MT_WORM, "MT_WORM" },
		{ MT_SYNC_META, "MT_SYNC_META" },
		{ MT_NFSASYNC, "MT_NFSASYNC" },
		{ MT_OLD_ARCHIVE_FMT, "MT_OLD_ARCHIVE_FMT" },
		{ MT_QUOTA, "MT_QUOTA" },
		{ MT_GFSID, "MT_GFSID" },
		{ MT_HWM_ARCHIVE, "MT_HWM_ARCHIVE" },
		{ MT_SHARED_SOFT, "MT_SHARED_SOFT" },
		{ MT_SHARED_BG, "MT_SHARED_BG" },
		{ MT_REFRESH_EOF, "MT_REFRESH_EOF" },
		{ MT_ARCHIVE_SCAN, "MT_ARCHIVE_SCAN" },
		{ MT_ABR_DATA, "MT_ABR_DATA" },
		{ MT_DMR_DATA, "MT_DMR_DATA" },
		{ MT_ZERO_DIO_SPARSE, "MT_ZERO_DIO_SPARSE" },
		{ MT_CONSISTENT_ATTR, "MT_CONSISTENT_ATTR" },
		{ MT_WORM_LITE, "MT_WORM_LITE" },
		{ MT_WORM_EMUL, "MT_WORM_EMUL" },
		{ MT_EMUL_LITE, "MT_EMUL_LITE" },
		{ MT_CDEVID, "CDEVID" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

/*
 * The following are used internally, but not available to messages.
 */

static char *
decode_fi_config1(uint64_t value)
{
	static sam_flagtext_t tbl[] = {
		{ MC_SHARED_FS, "MC_SHARED_FS" },
		{ MC_SHARED_MOUNTING, "MC_SHARED_MOUNTING" },
		{ MC_MISMATCHED_GROUPS, "MC_MISMATCHED_GROUPS" },
		{ MC_SMALL_DAUS, "MC_SMALL_DAUS" },
		{ MC_MR_DEVICES, "MC_MR_DEVICES" },
		{ MC_MD_DEVICES, "MC_MD_DEVICES" },
		{ MC_STRIPE_GROUPS, "MC_STRIPE_GROUPS" },
		{ MC_CLUSTER_MGMT, "MC_CLUSTER_MGMT" },
		{ MC_CLUSTER_FASTSW, "MC_CLUSTER_FASTSW" },
		{ MC_OBJECT_FS, "MC_OBJECT_FS" },
		{ MC_SAM_DB, "MC_SAM_DB" },
		{ MC_NOXATTR, "MC_NOXATTR" },
		{ 0, "" }
	};

	return (decodemask(value, tbl));
}

/*
 * ---- print_vfs - Print vfs entry.
 *  Print the contents of the vfs entry.
 */

static void
print_vfs(
	vfs_t *vfs,	/* vfs entry to print */
	vfs_t *addr)   /* Physical address of vfs */
{
	printf("vfs:\t%p\n", addr);
	printf("\t%12p vfs_op\n", vfs->vfs_op);
	printf("\t%12p vfs_next\n", vfs->vfs_next);
	printf("\t%12p vfs_vnodecovered\n", vfs->vfs_vnodecovered);
	printf("\t%12lx vfs_dev\n", vfs->vfs_dev);
	printf("\t%12p vfs_data\n", vfs->vfs_data);
	printf("\t%12p vfs_list\n", vfs->vfs_list);
	printf("\t%12p vfs_mntopts\n", vfs->vfs_mntopts);
	printf("\t%12p vfs_resource\n", vfs->vfs_resource);
	printf("\t%12p vfs_mntpt\n", vfs->vfs_mntpt);
	printf("\t%08x vfs_flag\n", vfs->vfs_flag);
	printf("\t%08x vfs_count\n", vfs->vfs_count);
	printf("\t%08x vfs_fstype (%d)\n", vfs->vfs_fstype, vfs->vfs_fstype);
	printf("\t%08x/%08x vfs_fsid\n",
	    vfs->vfs_fsid.val[0], vfs->vfs_fsid.val[1]);
	printf("\t%08lx vfs_bcount\n", vfs->vfs_bcount);
	printf("\t%08lx vfs_mtime\n", vfs->vfs_mtime);
}
#endif

/*
 * ---- print_global_tbl - Print global entry.
 *  Print the contents of the global entry.
 */

static void
print_global_tbl(
#ifdef sun
	sam_global_tbl_t *gt,   /* SAM-FS global table */
	sam_global_tbl_t *addr   /* SAM-FS global table */
#else
	trace_global_tbl_t *gt,   /* SAM-FS global table */
	trace_global_tbl_t *addr   /* SAM-FS global table */
#endif
)
{
	void **lock;

	printf("samgt:\t%p\n",		addr);
#ifdef sun
	printf("\t%012p ihashhead\n", gt->ihashhead);
	printf("\t%012p ihashlock\n", gt->ihashlock);
	printf("\t%012p ifreehead.forw\n", gt->ifreehead.free.forw);
	printf("\t%012p ifreehead.back\n", gt->ifreehead.free.back);
#endif
	printf("\t%012p mp_list\n", gt->mp_list);
	printf("\t%012p mp_stale\n", gt->mp_stale);
#ifdef sun
	printf("\t%012p samaio_vp\n", gt->samaio_vp);
	printf("\t%012p buf_freelist\n", gt->buf_freelist);
	lock = (void **)&gt->ifreelock;
	printf("\t%012p ifreelock owner\n", MUTEX_OWNER_PTR(*lock));
	lock = (void **)&gt->buf_mutex;
	printf("\t%012p buf_mutex owner\n", MUTEX_OWNER_PTR(*lock));
	lock = (void **)&gt->global_mutex;
	printf("\t%012p global_mutex owner\n", MUTEX_OWNER_PTR(*lock));
#endif
	printf("\t%08x num_fs_configured\n", gt->num_fs_configured);
	printf("\t%08x num_fs_mounted\n", gt->num_fs_mounted);
	printf("\t%08x num_fs_mounting\n", gt->num_fs_mounting);
	printf("\t%08x num_fs_syncing\n", gt->num_fs_syncing);
#ifdef sun
	printf("\t%08x nhino         (%d)\n", gt->nhino, gt->nhino);
	printf("\t%08x ninodes       (%d)\n", gt->ninodes, gt->ninodes);
#endif
	printf("\t%08x inocount      (%d)\n", gt->inocount, gt->inocount);
	printf("\t%08x inofree       (%d)\n", gt->inofree, gt->inofree);
	printf("\t%08x fstype        (%d)\n", gt->fstype, gt->fstype);
	printf("\t%08x buf_wait      (%d)\n", gt->buf_wait, gt->buf_wait);
	printf("\t%08x amld_pid      (%d)\n", gt->amld_pid, gt->amld_pid);
#ifdef linux
	printf("\t%08x meta_minor    (%d)\n", gt->meta_minor, gt->meta_minor);
#endif
	printf("\t%08x schedule_flags(%d)\n", gt->schedule_flags,
	    gt->schedule_flags);
	printf("\t%08x schedule_count(%d)\n", gt->schedule_count,
	    gt->schedule_count);
#ifdef sun
	if (Verbose) {

		num_duplist = (gt->inocount > gt->inofree) ?
		    gt->inocount : gt->inofree;
		if (num_duplist == 0) {
			num_duplist = gt->ninodes;
		}
		dup_list = malloc(sizeof (struct dup_chk) * num_duplist);
		if (dup_list == NULL) {
			printf("Couldn't allocate memory for inode "
			    "duplicate array.\n");
		}
		print_chains(gt, addr);
		if (dup_list) {
			free(dup_list);
		}
	}
#endif
}

#ifdef sun
/*
 * ---- print_samamld_tbl - Print samamld table.
 *  Print the contents of the samamld table.
 */

static void
print_samamld_tbl(
	samamld_cmd_table_t *si,	/* SAM-FS samamld table */
	samamld_cmd_table_t *addr)	/* SAM-FS samamld table */
{
	void **lock;
	samamld_cmd_queue_t *next;
	samamld_cmd_queue_t cmd;
	samamld_cmd_queue_t cmdarray[SAM_AMLD_CMD_BUF];
	uint_t *xsi;
	caddr_t naddr;
	int i, k, tblsize, quesize, quebufsize;
	int count;

	if (sam_amld) {
		printf("\n");
		tblsize = sizeof (samamld_cmd_table_t);
		quesize = sizeof (samamld_cmd_queue_t);
		quebufsize = sizeof (samamld_cmd_queue_t) * SAM_AMLD_CMD_BUF;
		printf("sizeof samamld_cmd_table_t:\t\t\t%08x\t(%d)\n",
		    tblsize, tblsize);
		printf("sizeof samamld_cmd_queue_t:\t\t\t%08x\t(%d)\n",
		    quesize, quesize);
		printf("sizeof samamld_cmd_queue_t * %d:\t%08x\t(%d)\n",
		    SAM_AMLD_CMD_BUF, quebufsize, quebufsize);
	}

	printf("\n");
	printf("si_cmd_table:\t%p\n", addr);
	printf("\t%p cmd_buffers\n", si->cmd_buffers);
	printf("\t%p cmd_buffer_free_list\n", si->cmd_buffer_free_list);
	printf("\t%08x cmd_buffer_free_list_count\n",
	    si->cmd_buffer_free_list_count);
	lock = (void **)&si->queue_mutex;
	printf("\t%12p queue_mutex\n", MUTEX_OWNER_PTR(*lock));
	lock = (void **)&si->samamld_cmd_queue_hdr.cmd_queue_mutex;
	printf("\t%12p hdr.cmd_queue_mutex\n", MUTEX_OWNER_PTR(*lock));
	lock = (void **)&si->samamld_cmd_queue_hdr.cmd_lockout_mutex;
	printf("\t%12p hdr.cmd_lockout_mutex\n", MUTEX_OWNER_PTR(*lock));
	printf("\t%08x hdr.cmd_lock_flag\n",
	    si->samamld_cmd_queue_hdr.cmd_lock_flag);
	printf("\t%08lx hdr.cmd_queue_timeout\n",
	    si->samamld_cmd_queue_hdr.cmd_queue_timeout);
	printf("\t%12p hdr.front\n", si->samamld_cmd_queue_hdr.front);
	printf("\t%12p hdr.end\n", si->samamld_cmd_queue_hdr.end);
	printf("\n");

	if (sam_amld) {
		printf("raw samamld_cmd_table_t:\t%p\n", addr);
		xsi = (uint_t *)si;
		for (i = 0; i < tblsize; i += 4*sizeof (int)) {
			int o[4], j;

			for (j = 0; j < 4; j++) {
				o[j] = *xsi++;
			}
			printf("\t%08x \t%08x \t%08x \t%08x\n",
			    o[0], o[1], o[2], o[3]);
		}
		printf("\n");
	}

	next = si->samamld_cmd_queue_hdr.front;
	if (next != NULL) {
		printf("cmd_queue:\t%p\n", next);
	} else {
		printf("cmd_queue:\tempty\n");
	}
	printf("\n");

	if (sam_amld) {
		count = SAM_AMLD_CMD_BUF;
		i = 1;
		while ((next != NULL) && count--) {
			k = kvm_kread(kvm_fd, (sam_tr_t)next,
			    (char *)&cmd, quesize);
			if (k < 0) {
				printf("Couldn't read SAM-FS "
				    "samamld_cmd_queue_t at %p\n",
				    next);
				break;
			} else {
				printf("cmd entry %d:\t%p\n", i, next);
				lock = (void **)&cmd.cmd_mutex;
				printf("\t%p cmd_mutex\n",
				    MUTEX_OWNER_PTR(*lock));
				printf("\t%08x qcmd_error\n", cmd.qcmd_error);
				printf("\t%08x qcmd_wait\n", cmd.qcmd_wait);
				printf("\t%08x blk_flag\n", cmd.blk_flag);
				printf("\t%08x cmd\n", cmd.cmd.cmd);
				printf("\t%p forward\n", cmd.forward);
				printf("\t%p back\n", cmd.back);
				printf("\n");
			}
			next = cmd.back;
			i++;
		}
	}

	next = si->cmd_buffer_free_list;
	if (next != NULL) {
		printf("free_queue:\t%p\n", next);
	} else {
		printf("free_queue:\tempty\n");
	}
	printf("\n");

	if (sam_amld) {
		count = SAM_AMLD_CMD_BUF;
		i = 1;
		while ((next != NULL) && count--) {
			k = kvm_kread(kvm_fd, (sam_tr_t)next,
			    (char *)&cmd, quesize);
			if (k < 0) {
				printf("Couldn't read SAM-FS "
				    "samamld_cmd_queue_t at %p\n",
				    next);
				break;
			} else {
				printf("free entry %d:\t%p\n", i, next);
				lock = (void **)&cmd.cmd_mutex;
				printf("\t%p cmd_mutex\n",
				    MUTEX_OWNER_PTR(*lock));
				printf("\t%08x qcmd_error\n", cmd.qcmd_error);
				printf("\t%08x qcmd_wait\n", cmd.qcmd_wait);
				printf("\t%08x blk_flag\n", cmd.blk_flag);
				printf("\t%08x cmd\n", cmd.cmd.cmd);
				printf("\t%12p forward\n", cmd.forward);
				printf("\t%12p back\n", cmd.back);
				printf("\n");
			}
			next = cmd.forward;
			i++;
		}
	}

	if (sam_amld) {
		next = si->cmd_buffers;
		naddr = (caddr_t)next;
		if (next != NULL) {
			printf("raw samamld_cmd_queue_t:\t%p\n",
			    si->cmd_buffers);
			k = kvm_kread(kvm_fd, (sam_tr_t)next,
			    (char *)&cmdarray,
			    quebufsize);
			if (k < 0) {
				printf("Couldn't read SAM-FS "
				    "samamld_cmd_queue_t at %p\n",
				    next);
				next = NULL;
			}
		} else {
			printf("entire command buffer:\tempty\n");
		}
		printf("\n");
		count = SAM_AMLD_CMD_BUF;
		i = 0;
		while ((next != NULL) && count--) {
			printf("raw cmd entry %d:\t%p\n", i+1, naddr);
			lock = (void **)&cmdarray[i].cmd_mutex;
			printf("\t%p cmd_mutex\n", MUTEX_OWNER_PTR(*lock));
			printf("\t%08x qcmd_error\n", cmdarray[i].qcmd_error);
			printf("\t%08x qcmd_wait\n", cmdarray[i].qcmd_wait);
			printf("\t%08x blk_flag\n", cmdarray[i].blk_flag);
			printf("\t%08x cmd\n", cmdarray[i].cmd.cmd);
			printf("\t%p forward\n", cmdarray[i].forward);
			printf("\t%p back\n", cmdarray[i].back);
			printf("\n");
			naddr += quesize;
			i++;
		}
	}
}

static void
print_quota(sam_mount_t *mount)		/* quota table's mount entry */
{
	int i, k, hashed = 0, avail = 0;
	struct sam_quot quota, *qnext, *first;
	struct sam_quot *qhashtable[SAM_QUOTA_HASH_SIZE];

	if (mount->mi.m_quota_hash == NULL) {
		printf("Quotas not enabled on fs '%s'.\n", mount->mt.fi_name);
		return;
	}
	k = kvm_kread(kvm_fd, (ulong_t)mount->mi.m_quota_hash,
	    (char *)&qhashtable, sizeof (struct sam_quot *) *
	    SAM_QUOTA_HASH_SIZE);

	if (k < 0) {
		printf("Cannot read quota hash table at %p\n",
		    mount->mi.m_quota_hash);
		return;
	}
	for (i = 0; i < SAM_QUOTA_HASH_SIZE; i++) {
		qnext = qhashtable[i];
		if (qnext != NULL) {
			printf("quota_hash[0x%x]: %p\n", i, qnext);
			while (qnext != NULL) {
				hashed++;
				k = kvm_kread(kvm_fd, (ulong_t)qnext,
				    (char *)&quota, sizeof (struct sam_quot));
				if (k < 0) {
					printf("Cannot read next hash "
					    "quota entry at %p\n", qnext);
					break;
				}
				print_quota_entry(&quota);
				qnext = quota.qt_hash.next;
			}
		}
	}

	first = qnext = mount->mi.m_quota_avail;
	printf("Quota available list: %p\n", qnext);
	do {
		k = kvm_kread(kvm_fd, (ulong_t)qnext,
		    (char *)&quota, sizeof (struct sam_quot));
		if (k < 0) {
			printf("Cannot read next avail quota entry at %p\n",
			    qnext);
			break;
		}
		avail++;
		print_quota_entry(&quota);
		qnext = quota.qt_avail.next;
	} while (qnext != first);

	printf("Quota: %d hashed entries, %d-1 available entries\n",
	    hashed, avail);
}

static char *quota_type[SAM_QUOTA_MAX] = { "a", "g", "u" };

static char *quota_flags[] = {
	"busy",		"dirty",	NULL,		NULL,
	"overob",	"overtb",	"overof",	"overtf",
	"avail",	"hashed",	NULL,		NULL,
	"anchor",	NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL,
	NULL,		NULL,		NULL,		NULL
};

/*
 * Print an in-core quota entry.  Doesn't print out the actual
 * in-use, limits &c, but does print out all the pointers,
 * indices, flags and timers.
 */
static void
print_quota_entry(struct sam_quot *qp)
{
	char type[24];
	char qentflags[64]; /* longer than all entries from quota_flags[] */

	if (qp->qt_type >= 0 && qp->qt_type < SAM_QUOTA_MAX &&
	    quota_type[qp->qt_type] != NULL) {
		strncpy(type, quota_type[qp->qt_type], sizeof (type));
	} else {
		sprintf(type, "[BAD] typ=0x%x", qp->qt_type);
	}
	quota_flags_cvt(qp->qt_flags, qentflags);
	printf("  %s vp = %12p/%05x refs=%05x time=%08x flags = %s\n",
	    type, qp->qt_vp, qp->qt_index,
	    qp->qt_ref, (int)qp->qt_lastref, qentflags);
	printf("    hash next = %12p; avail next/prev = %12p/%12p\n",
	    qp->qt_hash.next, qp->qt_avail.next, qp->qt_avail.prev);
}

static void
quota_flags_cvt(int flags, char *buf)
{
	int i;
	char *p = buf;

	for (i = 0; i < 8*sizeof (flags); i++) {
		if ((1 << i) & flags) {
			if (p != buf) {
				*p++ = ' ';
			}
			if (quota_flags[i] == NULL) {
				sprintf(buf, "%010x", flags);
				return;
			}
			p += sprintf(p, "%s", quota_flags[i]);
		}
	}
}
#endif

/*
 * ---- print_sam_scd_table - Print sam_scd table.
 */

static void
print_sam_scd_table(
	struct sam_syscall_daemon *tbl,
	struct sam_syscall_daemon *addr) /* addr in kernel sam_scd_table */
{
	printf("\n");
	printf("sam_scd_table:\t%p\n", addr);
	printf("\t%04x/%04x stageall.put_wait/size\n",
	    tbl[SCD_stageall].put_wait, tbl[SCD_stageall].size);
	printf("\t%08x stageall.fseq\n", tbl[SCD_stageall].cmd.stageall.fseq);
	printf("\t%08x stageall.id.ino\n",
	    tbl[SCD_stageall].cmd.stageall.id.ino);
	printf("\t%08x stageall.id.gen\n",
	    tbl[SCD_stageall].cmd.stageall.id.gen);
	printf("\n");
}

#ifdef sun
#define	DUP_STR " DUP"
#define	CORRUPT_STR " HDR CORRUPT"
/*
 * ---- print_chains - Print free and hash chains.
 *  Print the contents of the free and hash chains.
 */

static void
print_chains(
	sam_global_tbl_t *gt,   /* SAM-FS global table */
	sam_global_tbl_t *addr) /* SAM-FS global table */
{
	void **lock;
	struct sam_node *forw;
	struct sam_node inode;
	sam_ihead_t *hash;
	kmutex_t *hashlock;
	struct sam_ihead *hp;
	kmutex_t *lp;
	offset_t *free;
	int k, i;
	int first, count;
	int verbose_time;
	offset_t temp;
	time_t temptime;
	char inode_errs[sizeof (CORRUPT_STR) + sizeof (DUP_STR)];
	char *inode_errp = inode_errs;
	vnode_t *kvp;
	vnode_t vnode;
	vnode_t *vp = &vnode;

	if (gt->nhino <= 0 || gt->nhino > SAM_NHINO_MAX) {
		printf("Inode hash table size unreasonable:  %x\n", gt->nhino);
		return;
	}
	hash = malloc(sizeof (sam_ihead_t) * gt->nhino);
	if (hash == NULL) {
		printf("Couldn't allocate memory for inode hash array.\n");
		return;
	}
	hashlock = malloc(sizeof (kmutex_t) * gt->nhino);
	if (hashlock == NULL) {
		printf("Couldn't allocate memory for inode hash-lock "
		    "array.\n");
		return;
	}
	printf("ifreehead:\t%p\n", addr);
	free = (offset_t *)(void *)addr;
	forw = gt->ifreehead.free.forw;
	if ((offset_t)forw % 8) {
		printf("WARNING: Corrupted free forw pointer: %p\n", forw);
		temp = (offset_t)forw;
		temp >>= 3;
		temp <<= 3;
		forw = (struct sam_node *)temp;
		printf("WARNING: Corrected free forw pointer: %p\n", forw);
	}
	dup_length = 0;				/* start a new dup list */
	while (forw != (sam_node_t *)free) {
		k = kvm_kread(kvm_fd, (ulong_t)forw,
		    (char *)&inode, sizeof (struct sam_node));
		if (k < 0) {
			printf("Couldn't read SAM-FS inode at %p\n", forw);
			break;
		} else {
			char seg[12];

			*inode_errp = '\0';
			seg[0] = '\0';
			if (S_ISSEGS(&inode.di)) {
				sprintf(seg, ", S%d",
				    inode.di.rm.info.dk.seg.ord + 1);
			} else if (S_ISSEGI(&inode.di)) {
				sprintf(seg, ", S0");
			}

			if (dup_list != NULL) {
				if (dup_inode_check(inode.dev,
				    inode.di.id.ino) != 0) {
					strcat(inode_errp, DUP_STR);
				}
			}
			if (inode.pad0 != PAD0_CONTENTS ||
			    inode.pad1 != PAD1_CONTENTS) {
				strcat(inode_errp, CORRUPT_STR);
			}

			kvp = SAM_ITOV(&inode);
			k = kvm_kread(kvm_fd, (ulong_t)kvp, (char *)&vnode,
			    sizeof (vnode_t));
			if (k < 0) {
				printf("Couldn't read SAM-FS vnode at %p\n",
				    kvp);
				break;
			}
			printf("    %p (%d.%d,%d.%d)\t%x cnt=%d pgs=%12p "
			    "flgs=%x%s\n",
			    forw, inode.di.id.ino, inode.di.id.gen,
			    inode.di.parent_id.ino, inode.di.parent_id.gen,
			    inode.di.mode, vp->v_count,
			    vp->v_pages, inode.flags.bits, seg);
		}
		if (forw == inode.chain.free.forw) {
			printf("Loop detected in free chain, skipping\n");
			break;
		}
		forw = inode.chain.free.forw;
		if ((offset_t)forw % 8) {
			printf("WARNING: Corrupted free forw pointer: %p\n",
			    forw);
			temp = (offset_t)forw;
			temp >>= 3;
			temp <<= 3;
			forw = (struct sam_node *)temp;
			printf("WARNING: Corrected free forw pointer: %p\n",
			    forw);
		}
	}

	k = kvm_kread(kvm_fd, (ulong_t)gt->ihashhead,
	    (char *)&hash[0], sizeof (sam_ihead_t) * gt->nhino);
	if (k < 0) {
		printf("Couldn't read SAM-FS hash pointer table at %p\n",
		    gt->ihashhead);
		return;
	}
	hp = (struct sam_ihead *)gt->ihashhead;
	lp = (kmutex_t *)gt->ihashlock;

	k = kvm_kread(kvm_fd, (ulong_t)gt->ihashlock,
	    (char *)&hashlock[0], sizeof (kmutex_t) * gt->nhino);
	if (k < 0) {
		printf("Couldn't read SAM-FS hash lock pointer table at %p\n",
		    gt->ihashlock);
		return;
	}

	count = 0;
	for (i = 0; i < gt->nhino; i++, hp++, lp++, count++) {
		verbose_time = 0;
		forw = hash[i].forw;
		if ((offset_t)forw % 8) {
			printf("WARNING: Corrupted hash forw pointer: %p\n",
			    forw);
			verbose_time = 1;
			temp = (offset_t)forw;
			temp >>= 3;
			temp <<= 3;
			forw = (struct sam_node *)temp;
			printf("WARNING: Corrected hash forw pointer: %p\n",
			    forw);
		}
		first = 1;
		dup_length = 0;			/* start a new dup list */
		while (forw != (sam_node_t *)(void *)hp) {
			*inode_errp = '\0';
			k = kvm_kread(kvm_fd, (ulong_t)forw,
			    (char *)&inode, sizeof (struct sam_node));
			if (k < 0) {
				printf("Couldn't read SAM-FS inode at %p\n",
				    forw);
				break;
			} else {
				char seg[8];
				strcpy(seg, "  ");
				if (S_ISSEGS(&inode.di)) {
					sprintf(seg, "S%d",
					    inode.di.rm.info.dk.seg.ord + 1);
				} else if (S_ISSEGI(&inode.di)) {
					sprintf(seg, "S%d", 0);
				}
				if (first) {
					first = 0;
					lock = (void **)&hashlock[i];
					if (MUTEX_OWNER_PTR(*lock)) {
						printf("ihashhead %d:\t%p\t"
						    "%p LOCK %p\n",
						    i, hp, lp,
						    MUTEX_OWNER_PTR(*lock));
					} else {
						printf("ihashhead %d:\t%p\t"
						    "%p \n",
						    i, hp, lp);
					}
				}

				kvp = SAM_ITOV(&inode);
				k = kvm_kread(kvm_fd, (ulong_t)kvp,
				    (char *)&vnode, sizeof (vnode_t));
				if (k < 0) {
					printf("Couldn't read SAM-FS vnode "
					    "at %p\n", kvp);
					break;
				}
				if (dup_list != NULL) {
					if (dup_inode_check(inode.dev,
					    inode.di.id.ino) != 0) {
						strcat(inode_errp, DUP_STR);
					}
				}
				if (inode.pad0 != PAD0_CONTENTS ||
				    inode.pad1 != PAD1_CONTENTS) {
					strcat(inode_errp, CORRUPT_STR);
				}
				/*
				 * Print common inode stuff.
				 */
				printf("    %p (%d.%d,%d.%d)\t%x cnt=%d "
				    "pgs=%12p flgs=%x",
				    forw, inode.di.id.ino, inode.di.id.gen,
				    inode.di.parent_id.ino,
				    inode.di.parent_id.gen,
				    inode.di.mode, vp->v_count,
				    vp->v_pages, inode.flags.bits);


				lock = (void **)&vp->v_lock;
				if (*lock) {
					printf(", %s LOCK %p%s\n", seg,
					    *lock, inode_errp);
				} else {
					if (inode.di.rm.size <
					    inode.cl_allocsz) {
						printf(", %d, %lld",
						    inode.cl_hold_blocks,
						    inode.cl_allocsz);
					}
					printf(" %s\n", inode_errp);
					if (inode.cl_flock) {
						sam_cl_flock_t *fptr, fbuf;

						fptr = inode.cl_flock;

	/* N.B. Bad indentation here to meet cstyle requirements */
			while (fptr) {
				k = kvm_kread(kvm_fd, (ulong_t)fptr,
				    (char *)&fbuf,
				    sizeof (sam_cl_flock_t));
				if (k < 0) {
					printf("Couldn't read SAM-FS flock "
					    "at %p\n",
					    fptr);
					break;
				} else {
					printf("    FLOCK %llx, %llx, "
					    "%ld, %x\n",
					    fbuf.flock.l_start,
					    fbuf.flock.l_len,
					    fbuf.flock.l_pid,
					    fbuf.flock.l_sysid);
				}
				fptr = fbuf.cl_next;
			}

					}
				}

				if (verbose_time) {
					/*
					 * temptime avoids runtime invalid
					 * address alignment.
					 */
					temptime =
					    inode.di.access_time.tv_sec;
					printf("\t\taccess_time: (%lx) %s",
					    temptime,
					    ctime((time_t *)& temptime));
					temptime =
					    inode.di.modify_time.tv_sec;
					printf("\t\tmodify_time: (%lx) %s",
					    temptime,
					    ctime((time_t *)& temptime));
					temptime =
					    inode.di.change_time.tv_sec;
					printf("\t\tchange_time: (%lx) %s",
					    temptime,
					    ctime((time_t *)& temptime));
					temptime = inode.di.creation_time;
					printf("\t\tcreate_time: (%lx) %s",
					    temptime,
					    ctime((time_t *)& temptime));
					temptime = inode.di.attribute_time;
					printf("\t\tattrib_time: (%lx) %s",
					    temptime,
					    ctime((time_t *)& temptime));
				}
			}
			if (forw == inode.chain.hash.forw) {
				printf("Loop detected in hash chain, "
				    "skipping\n");
				break;
			}
			forw = inode.chain.hash.forw;
			if ((offset_t)forw % 8) {
				printf("WARNING: Corrupted hash forw "
				    "pointer: %p\n", forw);
				verbose_time = 1;
				temp = (offset_t)forw;
				temp >>= 3;
				temp <<= 3;
				forw = (struct sam_node *)temp;
				printf("WARNING: Corrected hash forw "
				    "pointer: %p\n", forw);
			}
			if ((inode.di.version < SAM_MIN_INODE_VERSION) ||
			    (inode.di.version > SAM_MAX_INODE_VERSION)) {
				printf("WARNING: Inode version is %d\n",
				    inode.di.version);
			}
			if (count > gt->ninodes) {
				break;
			}
		}
	}
}


/*
 * ----- dup_inode_check - detect if any duplicate inodes exist.
 * dup_inode_check looks for a matching inode number and device on
 * a list. The free list gets long, so takes a bit of time. Since
 * all other inode entries are hashed by dev and inode number, these
 * lists are the only (and short) lists to search.
 */
static int
dup_inode_check(
	dev_t dev,
	sam_ino_t ino)
{
	uint32_t cdup;
	struct dup_chk *dupp;

	if (dup_list != NULL) {
		for (cdup = 0, dupp = dup_list; cdup < dup_length;
		    cdup++, dupp++) {
			if (dupp->dev == dev && dupp->ino == ino) {
				return (1);
			}
		}
		if (dup_length < num_duplist) {
			dupp->dev = dev;
			dupp->ino = ino;
			dup_length++;
		}
	}
	return (0);
}

/*
 * ---- print_mount - Print mount entry.
 *  Print the contents of the mount entry.
 */

static void
print_mount(
	sam_mount_t *mount,	/* mount entry to print */
	sam_mount_t *addr)	/* Physical address of mount */
{
	int bt;
	int i;
	int j;
	int ci;
	int k;
	struct samdent *dp;
	sam_fb_pool_t *fbp;
	void **lock, **lock1;
	int ord;
	struct sam_rel_blks *next;
	struct sam_rel_blks rel_blk;

	printf("mount:\t%p\n", addr);
	printf("\t%s fs name\n", mount->mt.fi_name);
	printf("\t%s mnt_point\n", mount->mt.fi_mnt_point);
	printf("\t%8x fs version\n", mount->mt.fi_version);
	printf("\t%8x fam_set type\n", mount->mt.fi_type);
	printf("\t%8x maxphys type\n", mount->mi.m_maxphys);
	printf("\t%12p mp_next\n", mount->ms.m_mp_next);
	printf("\t%12p m_fsev_buf\n", mount->ms.m_fsev_buf);
	printf("\t%12p next\n", mount->mi.m_next);
	printf("\t%12p invalp\n", mount->mi.m_invalp);
	printf("\t%12p prealloc\n", mount->mi.m_prealloc);
	printf("\t%12p inodir\n", mount->mi.m_inodir);
	printf("\t%12p inoblk\n", mount->mi.m_inoblk);
	printf("\t%12p vn_root\n", mount->mi.m_vn_root);
	printf("\t%12p vfsp\n", mount->mi.m_vfsp);
	printf("\t%12p sblk\n", mount->mi.m_sbp);
	printf("\t%12p quota_avail\n", mount->mi.m_quota_avail);
	printf("\t%12p quota_hash_pp\n", mount->mi.m_quota_hash);
	printf("\t%12p quota_ip[0] (.quota_a)\n", mount->mi.m_quota_ip[0]);
	printf("\t%12p quota_ip[1] (.quota_g)\n", mount->mi.m_quota_ip[1]);
	printf("\t%12p quota_ip[2] (.quota_u)\n", mount->mi.m_quota_ip[2]);
	printf("\t%08x schedule_flags\n", mount->mi.m_schedule_flags);
	printf("\t%08x schedule_count\n", mount->mi.m_schedule_count);
	printf("\t%08x inval_count\n", mount->mi.m_inval_count);
	printf("\t%08x sync_meta\n", mount->mt.fi_sync_meta);
	printf("\t%08x status%s\n", mount->mt.fi_status,
	    (Decode) ? decode_S(mount->mt.fi_status) : "");
	printf("\t%08x config%s\n", mount->mt.fi_config,
	    (Decode) ? decode_fi_config(mount->mt.fi_config) : "");
	printf("\t%08x config1%s\n", mount->mt.fi_config1,
	    (Decode) ? decode_fi_config1(mount->mt.fi_config1) : "");
	printf("\t%08x stripe\n", mount->mt.fi_stripe[0]);
	printf("\t%08x mm_stripe\n", mount->mt.fi_stripe[1]);
	printf("\t%08x obj_width\n", mount->mt.fi_obj_width);
	printf("\t%08x obj_depth\n", mount->mt.fi_obj_depth);
	printf("\t%08x obj_depth_shift\n", mount->mt.fi_obj_depth_shift);
	printf("\t%012llx wr_throttle\n", mount->mt.fi_wr_throttle);
	printf("\t%08llx readahead   (%lld)\n",
	    mount->mt.fi_readahead, mount->mt.fi_readahead);
	printf("\t%08llx writebehind (%lld)\n",
	    mount->mt.fi_writebehind, mount->mt.fi_writebehind);
	printf("\t%08x rd_ino_buf_size\n", mount->mt.fi_rd_ino_buf_size);
	printf("\t%08x wr_ino_buf_size\n", mount->mt.fi_wr_ino_buf_size);
	printf("\t%08x flush_behind\n", mount->mt.fi_flush_behind);
	printf("\t%08x stage_flush_behind\n", mount->mt.fi_stage_flush_behind);
	printf("\t%08x stage_n_window\n", mount->mt.fi_stage_n_window);
	printf("\t%08x stage_retries\n", mount->mt.fi_stage_retries);
	printf("\t%dK partial\n", mount->mt.fi_partial);
	printf("\t%dK maxpartial\n", mount->mt.fi_maxpartial);
	printf("\t%08x partial_stage\n", mount->mt.fi_partial_stage);
	printf("\t%08x ext_bsize\n", mount->mt.fi_ext_bsize);
	printf("\t%08x fs_count\n", mount->mt.fs_count);
	printf("\t%08x mm_count\n", mount->mt.mm_count);
	printf("\t%012llx capacity\n", mount->mt.fi_capacity);
	printf("\t%012llx space free\n", mount->mt.fi_space);
	printf("\t%012llx hwm block count\n", mount->mi.m_high_blk_count);
	printf("\t%012llx lwm block count\n", mount->mi.m_low_blk_count);
	printf("\t%08x wm xmsg state\n", mount->mi.m_xmsg_state);
	printf("\t%08x wm xmsg time\n", mount->mi.m_xmsg_time);
	printf("\t%012lx fs full time\n", mount->mi.m_fsfull);
	printf("\t%012lx fs full msg\n", mount->mi.m_fsfullmsg);
	printf("\t%08x release time\n", mount->mi.m_release_time);
	printf("\t%012lx blk ran time\n", mount->mi.m_blkth_ran);
	printf("\t%012lx blk alloc time\n", mount->mi.m_blkth_alloc);
	printf("\t%08x min user inumber\n", mount->mi.m_min_usr_inum);

	lock = (void **)&mount->ms.m_synclock;
	printf("\t%012p synclock owner\n", MUTEX_OWNER_PTR(*lock));
	printf("\t%08x sblk time\n", mount->mi.m_sblk_fsid);
	printf("\t%08x sblk gen\n", mount->mi.m_sblk_fsgen);
	printf("\t%08x sblk_offset[0]\n", mount->mi.m_sblk_offset[0]);
	printf("\t%08x sblk_offset[1]\n", mount->mi.m_sblk_offset[1]);
	printf("\t%08x sblk version\n", mount->mi.m_sblk_version);
	printf("\t%08x bn shift\n", mount->mi.m_bn_shift);
	printf("\t%08x reserve .blocks bn\n", mount->mi.m_blk_bn);
	printf("\t%08x reserve .blocks ord\n", mount->mi.m_blk_ord);
	printf("\t%08x no_blocks count\n", mount->mi.m_no_blocks);
	printf("\t%08x no_inodes count\n", mount->mi.m_no_inodes);
	printf("\t%08x wait_write count\n", mount->mi.m_wait_write);
	printf("\t%08x rmseqno\n", mount->mi.m_rmseqno);
	printf("\t%08x inode.wait\n", mount->mi.m_inode.wait);
	printf("\t%08x inode.busy\n", mount->mi.m_inode.busy);
	printf("\t%08x inode.flag\n", mount->mi.m_inode.flag);
	printf("\t%08x inode.state\n", mount->mi.m_inode.state);
	printf("\t%08x block.wait\n", mount->mi.m_block.wait);
	printf("\t%08x block.busy\n", mount->mi.m_block.busy);
	printf("\t%08x block.flag\n", mount->mi.m_block.flag);
	printf("\t%08x block.state\n", mount->mi.m_block.state);

	printf("\t%08x hostid\n", mount->ms.m_hostid);
	printf("\t%08llx sblk failed\n", mount->ms.m_sblk_failed);
	printf("\t%08x syscall_cnt\n", mount->ms.m_syscall_cnt);
	printf("\t%08x sysc_dfhold\n", mount->ms.m_sysc_dfhold);

#ifdef sun
	printf("\t%08d tsd_key\n", mount->ms.m_tsd_key);
	printf("\t%08d tsd_leasekey\n", mount->ms.m_tsd_leasekey);
#endif
	printf("\t%llu message list items\n", mount->ms.m_sharefs.queue.items);
	printf("\t%llu items high water mark\n",
	    mount->ms.m_sharefs.queue.wmark);
	printf("\t%llu items processed\n",
	    mount->ms.m_sharefs.queue.current_item);

	if (SAM_IS_SHARED_FS(mount)) {
		struct client_entry *clnt = NULL;
		struct client_entry **clnti = NULL;
		struct sam_msg_array *mep;
		struct sam_client_msg *msgp;

		printf("\t****SHARED FILE SYSTEM\n");
		printf("\tclient: %s\n", mount->ms.m_cl_hname);
		printf("\tserver: %s\n", mount->mt.fi_server);
		printf("\t%08x clnt_seqno\n", mount->ms.m_clnt_seqno);
		printf("\t%012llx minallocsz\n", mount->mt.fi_minallocsz);
		printf("\t%012llx maxallocsz\n", mount->mt.fi_maxallocsz);
		printf("\t%d min_pool\n", mount->mt.fi_min_pool);
		printf("\t%d meta_timeo\n", mount->mt.fi_meta_timeo);
		printf("\t%d rdlease\n", mount->mt.fi_lease[0]);
		printf("\t%d wrlease\n", mount->mt.fi_lease[1]);
		printf("\t%d aplease\n", mount->mt.fi_lease[2]);
		printf("\t%d cl_sock_flags\n", mount->ms.m_cl_sock_flags);
		printf("\t%d client_ord\n", mount->ms.m_client_ord);
		printf("\t%d server_ord\n", mount->ms.m_server_ord);
		printf("\t%d prev_srvr_ord\n", mount->ms.m_prev_srvr_ord);
		printf("\t%d maxord\n", mount->ms.m_maxord);
		printf("\t%d involuntary\n", mount->ms.m_involuntary);
		printf("\t%d max_clients\n", mount->ms.m_max_clients);
		printf("\t%d no_clients\n", mount->ms.m_no_clients);
		printf("\t%d sharefs.no_threads\n",
		    mount->ms.m_sharefs.no_threads);
		printf("\t%016llx server_tags\n", mount->ms.m_server_tags);
		printf("\t%d sharefs.busy\n", mount->ms.m_sharefs.busy);
		printf("\t%d sharefs.put_wait\n",
		    mount->ms.m_sharefs.put_wait);
		printf("\t%d nsocks\n", mount->ms.m_cl_nsocks);
		printf("\t%d dfhold\n", mount->ms.m_cl_dfhold);

		if (mount->ms.m_clienti != NULL) {
			if (mount->ms.m_no_clients > SAM_MAX_SHARED_HOSTS) {
				printf("Warning: large client count "
				    "(%d/%d) \n",
				    mount->ms.m_no_clients,
				    SAM_MAX_SHARED_HOSTS);
			} else {
				printf("\t   clients:  %d\n",
				    mount->ms.m_no_clients);
			}
			if (mount->ms.m_max_clients > SAM_MAX_SHARED_HOSTS) {
				printf("Warning: large client max (%d/%d) \n",
				    mount->ms.m_max_clients,
				    SAM_MAX_SHARED_HOSTS);
			} else {
				printf("\t   max client:  %d\n",
				    mount->ms.m_max_clients);
			}
			/*
			 * Space for the host table index array.
			 */
			clnti = malloc(SAM_INCORE_HOSTS_INDEX_SIZE *
			    sizeof (client_entry_t *));
			if (clnti == NULL) {
				printf("Cannot allocate space for client "
				    "table index array (%d)\n",
				    mount->ms.m_max_clients);
				goto skipclient;
			}
			bzero((char *)clnti,
			    SAM_INCORE_HOSTS_INDEX_SIZE *
			    sizeof (struct client_entry *));

			/*
			 * Space for a chunk of the incore client host table.
			 */
			clnt = malloc(SAM_INCORE_HOSTS_TABLE_INC *
			    sizeof (struct client_entry));

			if (clnt == NULL) {
				printf("Cannot allocate space for client "
				    "array (%d)\n",
				    mount->ms.m_max_clients);
				free(clnti);
				goto skipclient;
			}
			bzero((char *)clnt,
			    SAM_INCORE_HOSTS_TABLE_INC *
			    sizeof (struct client_entry));

			k = kvm_kread(kvm_fd, (ulong_t)mount->ms.m_clienti,
			    (char *)clnti, SAM_INCORE_HOSTS_INDEX_SIZE *
			    sizeof (struct client_entry *));
			if (k < 0) {
				printf("Couldn't read SAM-FS server "
				    "client index array at %p\n",
				    mount->ms.m_clienti);
			} else {
				sam_msg_array_t *ptr;
				mep = malloc(sizeof (sam_msg_array_t));
				printf("\tORD HOSTID   Mnt File  \t "
				    "minseqno\t  nmsg     cnt tags\n");


	/* N.B. Bad indentation here to meet cstyle requirements */

	i = 0;
	for (j = 0; j < SAM_INCORE_HOSTS_INDEX_SIZE; j++) {
		struct client_entry *chunkp = clnti[j];

		if (chunkp == NULL) {
			continue;
		}

		k = kvm_kread(kvm_fd, (ulong_t)chunkp,
		    (char *)clnt,
		    SAM_INCORE_HOSTS_TABLE_INC *
		    sizeof (struct client_entry));
		if (k < 0) {
			printf("Couldn't read SAM-FS server "
			    "client host table chunk at %p\n",
			    chunkp);
			break;
		}

		for (ci = 0; ci < SAM_INCORE_HOSTS_TABLE_INC; ci++) {
			struct client_entry *clp = &clnt[ci];
			if (clp->hname[0] || clp->hostid || clp->cl_sh.sh_fp) {
				printf("\t%d   %08x %p %-16d %-8d %03d %-8x\n",
				    i+1, clp->hostid, clp->cl_sh.sh_fp,
				    clp->cl_min_seqno, clp->cl_nomsg,
				    clp->fs_count, clp->cl_tags);
				printf("\t    %s\n", clp->hname);

				ptr = (sam_msg_array_t *)
				    clp->queue.list.list_head.list_next;
				for (; clp->cl_nomsg; clp->cl_nomsg--) {
					k = kvm_kread(kvm_fd, (ulong_t)ptr,
					    (char *)mep,
					    sizeof (sam_msg_array_t));
					if (k < 0) {
						printf("Couldn't read SAM-FS "
						    "server msg"
						    " array at %p "
						    "for %s\n",
						    ptr, clp->hname);
							break;
					} else {
						ptr = (sam_msg_array_t *)
						    mep->node.list_next;
						printf("\t    %08d %p %p  "
						    "%d  %x %d   %d"
						    "   %d.%d\n",
						    mep->seqno, ptr,
						    mep->node.list_prev,
						    mep->client_ord,
						    (mep->command<<16)|
						    mep->operation,
						    mep->active,
						    mep->error,
						    mep->id.ino,
						    mep->id.gen);
					}
				}
			}
			i++;
			if (i >= mount->ms.m_max_clients) {
				goto clientsdone;
			}
		}
	}

			}
clientsdone:
			free(mep);
			free(clnt);
			free(clnti);
		}

skipclient:
		printf("\t%d server_wait\n", mount->ms.m_cl_server_wait);
		printf("\t%d active_ops\n", mount->ms.m_cl_active_ops);
		printf("\t%d wait_frozen\n", mount->mi.m_wait_frozen);
		printf("\t%p client FP\n", mount->ms.m_cl_sh.sh_fp);
		if (Verbose) {
			sam_client_msg_t msg;

			msgp = mount->ms.m_cl_head;
			printf("CLMSG cmd seqno err wait done "
			    "msg         next\n");
			while (msgp) {
				k = kvm_kread(kvm_fd, (ulong_t)msgp,
				    (char *)&msg,
				    sizeof (struct sam_client_msg));
				if (k < 0) {
					printf("Couldn't read SAM-FS client "
					    "msg at %p\n", msgp);
					break;
				}
				printf("    %x\t%d\t%d\t%d\t%p\t%p\n",
				    (msg.cl_command<<16)|msg.cl_operation,
				    msg.cl_seqno,
				    msg.cl_error, msg.cl_done,
				    msg.cl_msg, msg.cl_next);
				msgp = msg.cl_next;
			}
		}
		printf("\t%p cl_thread\n", mount->ms.m_cl_thread);
		printf("\t%p mount addr\n", addr);
		printf("\t%p server lease head\n", &addr->mi.m_sr_lease_chain);
		printf("\t%p server lease forw\n",
		    mount->mi.m_sr_lease_chain.forw);
		printf("\t%p server lease back\n",
		    mount->mi.m_sr_lease_chain.back);

		if (Verbose) {
			sam_lease_ino_t *leasep;

			leasep = mount->mi.m_sr_lease_chain.forw;
			while (leasep !=
			    (sam_lease_ino_t *)(void *)
			    &addr->mi.m_sr_lease_chain) {
				int nl;
				int no_clients;
				sam_lease_ino_t *lp;
				ulong_t inode_addr;
				struct sam_lease_ino lease;
				struct sam_node inode;

				k = kvm_kread(kvm_fd, (ulong_t)leasep,
				    (char *)&lease,
				    sizeof (struct sam_lease_ino));
				if (k < 0) {
					printf("Couldn't read SAM-FS "
					    "server lease list at %p\n",
					    leasep);
					break;
				}
				no_clients = lease.no_clients;
				lp = malloc(sizeof (sam_lease_ino_t) +
				    ((no_clients - 1)*
				    sizeof (struct sam_client_lease)));
				if (lp == NULL) {
					printf("Couldn't allocate memory "
					    "for server lease list.\n");
					break;
				} else {
					k = kvm_kread(kvm_fd,
					    (ulong_t)leasep, (char *)lp,
					    (sizeof (sam_lease_ino_t) +
					    ((no_clients-1)*
					    sizeof (struct sam_client_lease))));
					if (k < 0) {
						printf("Couldn't read "
						    "SAM-FS server lease "
						    "list at %p\n",
						    (void *)leasep);
						free(lp);
						break;
					}
				}

				inode_addr = (ulong_t)(struct sam_node *)
				    (lease.ip);
				k = kvm_kread(kvm_fd, inode_addr,
				    (char *)&inode,
				    sizeof (struct sam_node));
				if (k < 0) {
					printf("Couldn't read SAM-FS "
					    "inode at %p\n",
					    (void *)inode_addr);
					break;
				}

				printf("\t**** lease %p ip %p ino.gen "
				    "%d.%d no_cl %d\n",
				    (void *)leasep, (void *)inode_addr,
				    inode.di.id.ino, inode.di.id.gen,
				    lease.no_clients);
				for (nl = 0; nl < no_clients; nl++) {
					printf("\t\t %2d. ord %02d "
					    "lease %02x wt %02x flag %02x"
					" %lx %lx %lx %lx %lx %lx %lx%s\n",
					    nl, lp->lease[nl].client_ord,
					    lp->lease[nl].leases,
					    lp->lease[nl].wt_leases,
					    lp->lease[nl].actions,
					    lp->lease[nl].time[0],
					    lp->lease[nl].time[1],
					    lp->lease[nl].time[2],
					    lp->lease[nl].time[3],
					    lp->lease[nl].time[4],
					    lp->lease[nl].time[5],
					    lp->lease[nl].time[6],
					    (Decode) ?
					    decode_L(lp->lease[nl].leases) :
					    "");
				}
				free(lp);
				leasep = lease.lease_chain.forw;
			}
		}

		printf("\t%p client lease head\n", &addr->mi.m_cl_lease_chain);
		printf("\t%p client lease forw\n",
		    mount->mi.m_cl_lease_chain.forw);
		printf("\t%p client lease back\n",
		    mount->mi.m_cl_lease_chain.back);

		if (Verbose) {
			sam_nchain_t *leasep;

			leasep = mount->mi.m_cl_lease_chain.forw;
			while (leasep !=
			    (sam_nchain_t *)(void *)
			    &addr->mi.m_cl_lease_chain) {
				ulong_t inode_addr;
				sam_nchain_t chain;
				struct sam_node inode;
				int j;

				k = kvm_kread(kvm_fd, (ulong_t)leasep,
				    (char *)&chain,
				    sizeof (struct sam_nchain));
				if (k < 0) {
					printf("Couldn't read SAM-FS "
					    "client lease list at %p\n",
					    leasep);
					break;
				}

				inode_addr = (ulong_t)(struct sam_node *)
				    (chain.node);
				k = kvm_kread(kvm_fd, inode_addr,
				    (char *)&inode,
				    sizeof (struct sam_node));
				if (k < 0) {
					printf("Couldn't read SAM-FS "
					    "inode at %lx\n", inode_addr);
					break;
				}

				printf("\t**** ip %p ino.gen %d.%d\n",
				    inode_addr, inode.di.id.ino,
				    inode.di.id.gen);
				printf(
				    "\t lease %02x sv %02x usage %d "
				    "%d %d%s\n",
				    inode.cl_leases, inode.cl_saved_leases,
				    inode.cl_leaseused[0],
				    inode.cl_leaseused[1],
				    inode.cl_leaseused[2],
				    (Decode) ? decode_L(inode.cl_leases) : "");
				printf("\t\t   gen");
				for (j = 0; j < SAM_MAX_LTYPE; j++) {
					printf(" %x", inode.cl_leasegen[j]);
				}
				printf("\n");
				printf("\t\t   exp");
				for (j = 0; j < SAM_MAX_LTYPE; j++) {
					printf(" %llx", inode.cl_leasetime[j]);
				}
				printf("\n");
				if (inode.cl_short_leases != 0) {
					printf("\t\t short %x\n",
					    inode.cl_short_leases);
				}
				leasep = chain.forw;
			}
		}
	}

	next = mount->mi.m_next;
	if (next) {
		printf("\trelease block list:\n");
		while (next) {
			int k;
			k = kvm_kread(kvm_fd, (ulong_t)next,
			    (char *)&rel_blk,
			    sizeof (struct sam_rel_blks));
			if (k < 0) {
				printf("Couldn't read SAM-FS rel_blk "
				    "at %p\n", (void *)next);
				break;
			} else {
				printf("\t(%d.%d) len=%d\n", rel_blk.id.ino,
				    rel_blk.id.gen, (uint_t)rel_blk.length);
			}
			next = rel_blk.next;
		}
	}

	printf("\t  DD       MM     ROUND ROBIN SETTINGS\n");
	printf("\t%08x %08x dk_max\n", mount->mi.m_dk_max[DD],
	    mount->mi.m_dk_max[MM]);
	printf("\t%08x %08x dk_start\n", mount->mi.m_dk_start[DD],
	    mount->mi.m_dk_start[MM]);
	printf("\t%08x %08x unit\n", mount->mi.m_unit[DD],
	    mount->mi.m_unit[MM]);

	printf("\t  DD SM    DD LG  DATA DAU SETTINGS\n");
	printf("\t%012llx %012llx mask\n",
	    mount->mi.m_dau[DD].mask[SM], mount->mi.m_dau[DD].mask[LG]);
	printf("\t%012llx %012llx seg\n",
	    mount->mi.m_dau[DD].seg[SM], mount->mi.m_dau[DD].seg[LG]);
	printf("\t%08x %08x size\n",
	    mount->mi.m_dau[DD].size[SM], mount->mi.m_dau[DD].size[LG]);
	printf("\t%08x %08x wsize\n",
	    mount->mi.m_dau[DD].wsize[SM], mount->mi.m_dau[DD].wsize[LG]);
	printf("\t%08x %08x shift\n",
	    mount->mi.m_dau[DD].shift[SM], mount->mi.m_dau[DD].shift[LG]);
	printf("\t%08x %08x dif_shift\n",
	    mount->mi.m_dau[DD].dif_shift[SM],
	    mount->mi.m_dau[DD].dif_shift[LG]);
	printf("\t%08x %08x blocks\n",
	    mount->mi.m_dau[DD].blocks[SM],
	    mount->mi.m_dau[DD].blocks[LG]);
	printf("\t%08x %08x kblocks\n",
	    mount->mi.m_dau[DD].kblocks[SM],
	    mount->mi.m_dau[DD].kblocks[LG]);
	printf("\t%08x          sm_blkcount\n",
	    mount->mi.m_dau[DD].sm_blkcount);
	printf("\t%08llx          sm_bmask\n", mount->mi.m_dau[DD].sm_bmask);
	printf("\t%08x          sm_bits\n", mount->mi.m_dau[DD].sm_bits);
	printf("\t%08x          sm_off\n", mount->mi.m_dau[DD].sm_off);

	if (mount->mt.mm_count) {
		printf("\t  MM SM    MM LG  META DAU SETTINGS\n");
		printf("\t%012llx %012llx mask\n",
		    mount->mi.m_dau[MM].mask[SM],
		    mount->mi.m_dau[MM].mask[LG]);
		printf("\t%012llx %012llx seg\n",
		    mount->mi.m_dau[MM].seg[SM], mount->mi.m_dau[MM].seg[LG]);
		printf("\t%08x %08x size\n",
		    mount->mi.m_dau[MM].size[SM],
		    mount->mi.m_dau[MM].size[LG]);
		printf("\t%08x %08x wsize\n",
		    mount->mi.m_dau[MM].wsize[SM],
		    mount->mi.m_dau[MM].wsize[LG]);
		printf("\t%08x %08x shift\n",
		    mount->mi.m_dau[MM].shift[SM],
		    mount->mi.m_dau[MM].shift[LG]);
		printf("\t%08x %08x dif_shift\n",
		    mount->mi.m_dau[MM].dif_shift[SM],
		    mount->mi.m_dau[MM].dif_shift[LG]);
		printf("\t%08x %08x blocks\n",
		    mount->mi.m_dau[MM].blocks[SM],
		    mount->mi.m_dau[MM].blocks[LG]);
		printf("\t%08x %08x kblocks\n",
		    mount->mi.m_dau[MM].kblocks[SM],
		    mount->mi.m_dau[MM].kblocks[LG]);
		printf("\t%08x          sm_blkcount\n",
		    mount->mi.m_dau[MM].sm_blkcount);
		printf("\t%08llx          sm_bmask\n",
		    mount->mi.m_dau[MM].sm_bmask);
		printf("\t%08x          sm_bits\n",
		    mount->mi.m_dau[MM].sm_bits);
		printf("\t%08x          sm_off\n",
		    mount->mi.m_dau[MM].sm_off);
	}

	for (i = 0; i < mount->mt.fs_count; i++) {
		dp = &mount->mi.m_fs[i];
		printf("\t  DEVICE %d\n", i);
		printf("\t    %s \n", dp->part.pt_name);
		printf("\t    %08x eq\n", dp->part.pt_eq);
		printf("\t    %08x type\n", dp->part.pt_type);
		printf("\t    %04x%04x command/state\n",
		    dp->command, dp->part.pt_state);
		printf("\t    %012llx size\n", dp->part.pt_size);
		printf("\t    %012llx capacity\n", dp->part.pt_capacity);
		printf("\t    %012llx space\n", dp->part.pt_space);
		printf("\t    %04x%04x opened/skip_ord\n",
		    dp->opened, dp->skip_ord);
		printf("\t    %04x%04x busy/map_empty\n",
		    dp->busy, dp->map_empty);
		printf("\t    %08x dt\n", dp->dt);
		printf("\t    %012lx dev\n", dp->dev);
		printf("\t    %012p special_vn\n", dp->svp);
		printf("\t    %012p object_handle\n", dp->oh);
		printf("\t    %08x error\n", dp->error);
		printf("\t    %08x next_ord\n", dp->next_ord);
		printf("\t    %08x num_group\n", dp->num_group);
		printf("\t    %08x modified\n", dp->modified);
		printf("\t    %08x next_dau\n", dp->next_dau);
		printf("\t    %08x system\n", dp->system);
		printf("\t    %012p SM block\n", dp->block[0]);
		printf("\t    %012p LG block\n", dp->block[1]);
	}
	lock = (void **)&mount->mi.m_block.mutex;
	lock1 = (void **)&mount->mi.m_block.put_mutex;
	printf(
	"\tMS %p lock, %04x wait, %04x flag %04x state %04x "
	    "put_wait %p put.lck\n",
	    MUTEX_OWNER_PTR(*lock), mount->mi.m_block.wait,
	    mount->mi.m_block.flag, mount->mi.m_block.state,
	    mount->mi.m_block.put_wait, MUTEX_OWNER_PTR(*lock1));
	for (ord = 0; ord < mount->mt.fs_count; ord++) {
		struct sam_block block;
		struct sam_block *bp;

		printf("\n\tEQ%d %s\n", mount->mi.m_fs[ord].part.pt_eq,
		    (mount->mi.m_fs[ord].part.pt_type == DT_META ? "MM" :
		    (is_osd_group(mount->mi.m_fs[ord].part.pt_type) ?
		    "ox" : "DD")));
		for (i = 0; i < SAM_MAX_DAU; i++) {
			int out = 0;
			if (mount->mi.m_fs[ord].block[i] == NULL) {
				continue;
			}
			printf("\t%s block type------------\n",
			    (i == 0 ? "SM" : "LG"));
			k = kvm_kread(kvm_fd,
			    (ulong_t)mount->mi.m_fs[ord].block[i],
			    (char *)&block, sizeof (struct sam_block));
			if (k < 0) {
				printf("Couldn't read SAM-FS block buffer "
				    "at %p\n",
				    mount->mi.m_fs[ord].block[i]);
				continue;
			}
			bp = malloc(sizeof (struct sam_block) +
			    (block.limit-1)*sizeof (sam_bn_t));
			if (bp == NULL) {
				printf("Couldn't allocate memory for "
				    "block buffer.\n");
				continue;
			} else {
				k = kvm_kread(kvm_fd,
				    (ulong_t)mount->mi.m_fs[ord].block[i],
				    (char *)bp,
				    sizeof (struct sam_block) +
				    (block.limit-1)*sizeof (sam_bn_t));
				if (k < 0) {
					printf("Couldn't read SAM-FS "
					    "block buffer at %p\n",
					    mount->mi.m_fs[ord].block[i]);
					free(bp);
					continue;
				}
			}
			printf("\tord=%04d in=%04d out=%04d fill=%04d "
			    "limit=%04d\n",
			    bp->ord, bp->in, bp->out, bp->fill, bp->limit);
			while (out < block.limit) {
				int o[4], j;
				for (j = 0; j < 4; j++) {
					o[j] = out++;
				}
				printf("\t%5d \t%08x \t%08x \t%08x \t%08x\n",
				    o[0],
				    bp->bn[o[0]], bp->bn[o[1]],
				    bp->bn[o[2]], bp->bn[o[3]]);
			}
			free(bp);
		}
	}
	for (bt = 0; bt < SAM_MAX_DAU; bt++) {
		fbp = &mount->mi.m_fb_pool[bt];
		for (k = 0; k < SAM_NO_FB_ARRAY; k++) {
			lock = (void **)&fbp->array[k].fb_mutex;
			printf("\t%08x   FB %s count, %p owner\n",
			    fbp->array[k].fb_count, ((bt == SM) ? "SM" : "LG"),
			    MUTEX_OWNER_PTR(*lock));
			for (i = 0; i < fbp->array[k].fb_count; i++) {
				printf("\t    %01x bt %03x ord, %8x bn\n",
				    bt, fbp->array[k].fb_ord[i],
				    fbp->array[k].fb_bn[i]);
			}
		}
	}
}
#endif

/*
 * ---- dump_sam_trace - Write a trace file.
 */

static void
dump_sam_trace(
	char *lfn,
	sam_trace_tbl_t **trace_tables,
	int num_cpus)
{
	int fd;
	int i;

	if ((fd = open(lfn, O_WRONLY | O_CREAT | O_LARGEFILE, 0644)) < 0) {
		goto error;
	}

	if (write(fd, &trace_scale, sizeof (trace_scale)) !=
	    sizeof (trace_scale)) {
		goto error;
	}

	if (write(fd, &num_cpus, sizeof (num_cpus)) != sizeof (num_cpus)) {
		goto error;
	}

	if (write(fd, &trace_bytes, sizeof (trace_bytes)) !=
	    sizeof (trace_bytes)) {
		goto error;
	}

	for (i = 0; i < num_cpus; i++) {
		if (write(fd, trace_tables[i], trace_bytes) != trace_bytes) {
			goto error;
		}
	}

	if (close(fd) < 0) {
		goto error;
	}

	return;

error:
	fprintf(stderr, "Couldn't write disk trace file: %s\n", lfn);
	exit(1);
}

/*
 * ---- undump_sam_trace - Read a trace file and output its contents.
 */

static void
undump_sam_trace(char *lfn)
{
	int fd;
	int num_cpus;
	int i;

	if ((fd = open(lfn, O_RDONLY | O_LARGEFILE)) < 0) {
		goto error;
	}

	if (read(fd, &trace_scale, sizeof (trace_scale)) !=
	    sizeof (trace_scale)) {
		goto error;
	}

	if (read(fd, &num_cpus, sizeof (num_cpus)) != sizeof (num_cpus)) {
		goto error;
	}

	if (read(fd, &trace_bytes, sizeof (trace_bytes)) !=
	    sizeof (trace_bytes)) {
		goto error;
	}

	traces = malloc(num_cpus * sizeof (sam_trace_tbl_t *));
	if (traces == NULL) {
		fprintf(stderr,
		    "Couldn't allocate memory for SAM-FS trace "
		    "table (%d bytes)\n",
		    num_cpus * sizeof (sam_trace_tbl_t *));
		exit(1);
	}

	for (i = 0; i < num_cpus; i++) {
		traces[i] = malloc(trace_bytes);
		if (traces[i] == NULL) {
			fprintf(stderr, "Couldn't allocate %d bytes "
			    "for trace table %d\n",
			    trace_bytes, i);
			exit(1);
		}

		if (read(fd, traces[i], trace_bytes) != trace_bytes) {
			goto error;
		}
	}

	(void) close(fd);

	print_sam_trace(traces, num_cpus);

	return;

error:
	fprintf(stderr, "Couldn't read disk trace file: %s\n", lfn);
	exit(1);
}


/*
 *
 *  Continuous trace functions
 *
 */
#ifdef linux
#define		mutex_t				pthread_mutex_t
#define		cond_t				pthread_cond_t

#define		mutex_init(m, t, a)		pthread_mutex_init(m, a)
#define		mutex_lock(m)			pthread_mutex_lock(m)
#define		mutex_unlock(m)			pthread_mutex_lock(m)

#define		cond_init(c, t, a)		pthread_cond_init(c, a)
#define		cond_wait(c, m)			pthread_cond_wait(c, m)
#define		cond_signal(c)			pthread_cond_signal(c)


static int
set_realtime_priority()
{
	struct sched_param parms;

	bzero(&parms, sizeof (parms));
	parms.sched_priority = 1;
	if (sched_setscheduler(0, SCHED_FIFO, &parms) != 0) {
		fprintf(stderr,
		    "Unable to set LWP scheduling priority; errno=%d\n",
		    errno);
		return (-1);
	}
	return (0);
}

#ifdef _GNU_SOURCE

#if (KERNEL_MAJOR == 4)
#define	sam_sched_setaffinity(pid, sz, cset)	sched_setaffinity(pid, cset)
#endif /* KERNEL_MAJOR == 4 */

#if (KERNEL_MAJOR == 6)
#define	sam_sched_setaffinity(pid, sz, cset)	\
	sched_setaffinity(pid, sz, cset)
#endif

static int
cpu_bind_lwp(int cpu)
{
	cpu_set_t cset;

	CPU_ZERO(&cset);
	CPU_SET(cpu, &cset);
	if (sam_sched_setaffinity(0, sizeof (cset), &cset) != 0) {
		fprintf(stderr,
		    "LWP not bound to processor %d; errno=%d\n",
		    cpu, errno);
		return (-1);
	}
	return (0);
}
#else
static int
cpu_bind_lwp(int cpu)
{
	size_t mask;

	mask = (1 << cpu);
	if (sched_setaffinity(0, sizeof (mask), &mask) != 0) {
		fprintf(stderr,
		    "LWP not bound to processor %d; errno=%d\n",
		    cpu, errno);
		return (-1);
	}
	return (0);
}
#endif /* _GNU_SOURCE */
#endif	/* linux */


#ifdef sun
static int
set_realtime_priority()
{
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	rtparms_t	*rtparmsp = (rtparms_t *)pcparms.pc_clparms;

	bzero(&pcinfo, sizeof (pcinfo));
	strncpy(pcinfo.pc_clname, "RT", sizeof (pcinfo.pc_clname));
	if (priocntl(0, 0, PC_GETCID, (caddr_t)&pcinfo) < 0) {
		fprintf(stderr,
		    "priocntl(PC_GETCID) failed, unable to set "
		    "RT priority; errno=%d\n",
		    errno);
		return (-1);
	}
	bzero(&pcparms, sizeof (pcparms));
	pcparms.pc_cid = pcinfo.pc_cid;
	rtparmsp->rt_pri = RT_NOCHANGE;
	rtparmsp->rt_tqsecs = (ulong_t)RT_NOCHANGE;
	rtparmsp->rt_tqnsecs = RT_NOCHANGE;
	if (priocntl(P_PID, getpid(), PC_SETPARMS, (caddr_t)&pcparms) != 0) {
		fprintf(stderr,
		    "priocntl(PC_SETPARMS) failed, unable to "
		    "set RT priority;"
		    " errno=%d\n", errno);
		return (-1);
	}
	return (0);
}


/*
 * ---- cpu_bind_lwp
 *
 * Bind the calling LWP to the named CPU.
 */
static int
cpu_bind_lwp(int cpu)
{
	if (processor_bind(P_LWPID, P_MYID, cpu, NULL) < 0) {
		fprintf(stderr,
		    "LWP not bound to processor %d; errno=%d\n",
		    cpu, errno);
		return (-1);
	}
	return (0);
}
#endif	/* sun */


/*
 * ---- sam_continuous_xlate
 *
 * Read a continuous trace file and dump its contents.
 */
static void
sam_continuous_xlate(char *lfn)
{
	int i, n, fd;
	sam_trace_ent_t tbuf[2048];

	if ((fd = open(lfn, O_RDONLY | O_LARGEFILE)) < 0) {
		fprintf(stderr, "Couldn't open disk trace file: %s\n", lfn);
		exit(1);
	}

	while ((n = read(fd, &tbuf[0], sizeof (tbuf))) > 0) {
		for (i = 0; (i+1) * sizeof (tbuf[0]) <= n; i++) {
			if (tbuf[i].t_event != (ushort_t)-1) {
				printf("%16lld ", (long long)tbuf[i].t_time);
				printf("%x=%p ",
				    tbuf[i].t_pad, tbuf[i].t_thread);
				printf("%3x-%012p%c ",
				    tbuf[i].t_mount, tbuf[i].t_addr,
				    (int)tbuf[i].t_event >= T_SAM_MAX_VFS ?
				    ' ' : '*');
				print_trace_message(tbuf[i].t_event,
				    tbuf[i].t_p1, tbuf[i].t_p2, tbuf[i].t_p3);
				printf("\n");
			} else {
				printf("----- Overflow: missing trace "
				    "enties (CPU %d) -----\n",
				    tbuf[i].t_pad);
			}
		}
	}
	if (n < 0) {
		fprintf(stderr, "Read error in disk trace file '%s': %s",
		    lfn, strerror(errno));
		(void) close(fd);
		exit(1);
	}
	(void) close(fd);
}


/*
 * Number of trace buffer entries in the output write buffer.
 * This is written whenever the buffer is full or there is no
 * trace data available on some CPU queue.
 */
#define			TR_OBUF_SIZE	2048

/*
 * Trace & task queue.  Holds all the information needed by a
 * trace reader thread for the thread to get trace info from
 * a particular CPU.  Also contains buffers from which the
 * main thread pulls entries.
 */
struct sam_traceq {
	int tq_cpu;			/* CPU to read traces from */
	int tq_bufsiz;			/* len of tqb_buf trace bufs (bytes) */
	int tq_bufx;			/* idx of tq[] buf consumer is using */
	struct tqb {
		sam_trace_tbl_t *tqb_buf; /* trace buffer from kernel */
		int tqb_next;		/* next trace element in this buf */
		int tqb_valid;		/* true if valid data in buf */
		int tqb_overflow;	/* true if buf overrun */
	} tq[TR_NBUF_MAX];
	mutex_t tq_lock;		/* lock on sam_traceq struct */
	cond_t tq_get_cv;		/* to wake up producer on this queue */
	cond_t *tq_put_cv;		/* to wake up consumer of this queue */
};


/*
 * ---- thrReadTrace
 *
 * Read CPU trace buffers from the kernel.  There are N threads
 * executing this code (1 thread per cpu).  Each reads up a
 * buffer's worth at a time into the next of a set of buffers.
 * The main thread is signaled whenever a new buffer of data
 * is available, and it signals this thread whenever it frees
 * up a buffer.
 */
void *
thrReadTrace(void *p)
{
	struct sam_traceq *tkp = (struct sam_traceq *)p;
	struct tqb *tqbp;
	sam_trace_tbl_t *ltp;
	int e, ibuf = 0;

	cpu_bind_lwp(tkp->tq_cpu);
	set_realtime_priority();
	for (;;) {
		/*
		 * Wait for empty buffer
		 */
		tqbp = &tkp->tq[ibuf];
		ltp = tqbp->tqb_buf;

		mutex_lock(&tkp->tq_lock);
		/* reader still using this buf? */
		while (tqbp->tqb_valid) {
			cond_wait(&tkp->tq_get_cv, &tkp->tq_lock);
		}
		mutex_unlock(&tkp->tq_lock);

		ltp->t_cpu = tkp->tq_cpu;
		ltp->t_baseticks = TraceInterval;
		e = sam_syscall(SC_trace_tbl_wait, ltp, sizeof (*ltp));
		mutex_lock(&tkp->tq_lock);
		tqbp->tqb_overflow = 0;
		if (e < 0) {
			if (errno == EOVERFLOW) {
				tqbp->tqb_overflow = 1;
			} else {
				fprintf(stderr, "Error return from "
				    "trace (%d)\n", errno);
				break;
			}
		}
		tqbp->tqb_next = 0;		/* start at first entry */
		tqbp->tqb_valid = 1;		/* good data */
		cond_signal(tkp->tq_put_cv);	/* notify reader */
		mutex_unlock(&tkp->tq_lock);
		ibuf = ++ibuf % TrNbuf;		/* next buffer */
	}
	/* NOTREACHED */
	return (NULL);
}


/*
 * ---- sam_continuous_trace
 *
 * Main routine for continuous trace.  Forks off one thread per CPU,
 * each of which has a sam_traceq.  The sam_traceq contains a set of
 * trace buffers to get trace info, plus a lock and condition variables
 * with which to pass data back to the main thread.  The main thread
 * then reads up these buffers, and when there is data available in
 * all of them, writes out the oldest entry.
 */
/* ARGSUSED */
static void
sam_continuous_trace(
	int trace_cpus,
	int tracebytes,
	char *filename)
{
	int i, fd, error;
	pthread_attr_t attr;
	struct sam_traceq *tba;
	cond_t mcv;
	pthread_t *tids;
	sam_trace_ent_t obuf[TR_OBUF_SIZE];
	int odx = 0;
	/*
	 * trace_cpus == max # of configurable CPUs.
	 * cpus == CPUs actually configured.
	 */
	int cpus = sysconf(_SC_NPROCESSORS_CONF);


	if ((fd = open(filename, O_CREAT | O_APPEND | O_WRONLY, 0644)) < 0) {
		fprintf(stderr, "can't create output file (%d)\n", errno);
		exit(1);
	}

	/*
	 * Allocate a taskq/traceq for each CPU in the system.
	 * Allocate a thread ID for each task.  Initialize the
	 * global lock(s).
	 */
	tba = malloc(cpus * sizeof (*tba));
	if (!tba) {
		fprintf(stderr, "No trace buffer space available\n");
		exit(1);
	}
	bzero(tba, cpus * sizeof (*tba));
	tids = malloc(cpus * sizeof (*tids));
	if (!tids) {
		fprintf(stderr, "No trace buffer tids space available\n");
		exit(1);
	}
	bzero(tids, cpus * sizeof (*tids));
	cond_init(&mcv, USYNC_THREAD, NULL);

	/*
	 * Initialize the taskq/traceq for each thread.
	 */
	for (i = 0; i < cpus; i++) {
		int j;

		tba[i].tq_cpu = i;
		tba[i].tq_bufsiz = tracebytes;
		tba[i].tq_bufx = 0;
		cond_init(&tba[i].tq_get_cv, USYNC_THREAD, NULL);
#ifdef linux
		{
			/*
			 * 'pthread_mutex_t fmutex =
			 * PTHREAD_MUTEX_INITIALIZER;'
			 * should work, but doesn't.  The ERRORCHECK version
			 * does.
			 * The pthread_mutexattr_settype() should similarly
			 * use PTHREAD_MUTEX_FAST_NP instead of
			 * PTHREAD_MUTEX_ERRORCHECK_NP, but the ERRORCHECK
			 * version works...
			 */
			pthread_mutex_t fmutex =
			    PTHREAD_ERRORCHECK_MUTEX_INITIALIZER_NP;
			pthread_mutexattr_t mattr;

			tba[i].tq_lock = fmutex;
			pthread_mutexattr_init(&mattr);
			pthread_mutexattr_settype(&mattr,
			    PTHREAD_MUTEX_ERRORCHECK_NP);
			pthread_mutex_init(&tba[i].tq_lock, &mattr);
		}
#endif
#ifdef sun
		mutex_init(&tba[i].tq_lock, USYNC_THREAD, NULL);
#endif
		tba[i].tq_put_cv = &mcv;

		for (j = 0; j < TrNbuf; j++) {
			if ((tba[i].tq[j].tqb_buf =
			    malloc(tba[i].tq_bufsiz)) == NULL) {
				fprintf(stderr,
				    "No trace buffer space "
				    "available (%d/%d)\n", i, j);
				exit(1);
			}
		}
	}

	for (i = 0; i < cpus; i++) {
		pthread_attr_init(&attr);
		pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
		error = pthread_create(&tids[i], &attr, thrReadTrace,
		    (void *)&tba[i]);
		if (error) {
			fprintf(stderr,
			    "pthread_create returned error %d/%d\n",
			    error, errno);
			exit(1);
		}
	}
	/*
	 * Threads are off and running.  They'll signal the main
	 * thread (executing here) whenever they get more data to
	 * process.
	 */
	for (;;) {
		int anyempty, n;

getbuf:
		anyempty = 0;
		for (i = 0; i < cpus; i++) {
			int buf;

			mutex_lock(&tba[i].tq_lock);
			buf = tba[i].tq_bufx;
			if (!tba[i].tq[buf].tqb_valid) {
				anyempty = 1;
				break;
			}
			if (tba[i].tq[buf].tqb_overflow) {
				bzero(&obuf[odx], sizeof (obuf[odx]));
				/* Missing entries */
				obuf[odx].t_event = (ushort_t)-1;
				obuf[odx].t_pad = i;	/* record CPU # */
				if (++odx >= TR_OBUF_SIZE) {
					/*
					 * Full output buffer.  Write it out.
					 */
					if ((n = write(fd, obuf,
					    sizeof (obuf))) !=
					    sizeof (obuf)) {
						fprintf(stderr,
						    "short write "
						    "(%d/%d)\n",
						    n, sizeof (obuf));
						exit(1);
					}
					odx = 0;
				}
				tba[i].tq[buf].tqb_overflow = 0;
			}
			mutex_unlock(&tba[i].tq_lock);
		}
		if (anyempty) {
			mutex_unlock(&tba[i].tq_lock);
			/*
			 * If anything's in the output buffer, write it out.
			 * Then wait for something to show up in the empty
			 * input buffer we found.
			 *
			 * Either way, go back and rescan the input buffers
			 * when we're done.
			 */
			if (odx == 0) {
				mutex_lock(&tba[i].tq_lock);
				cond_wait(&mcv, &tba[i].tq_lock);
				mutex_unlock(&tba[i].tq_lock);
			} else {
				if ((n = write(fd, obuf,
				    odx * sizeof (obuf[0]))) !=
				    odx * sizeof (obuf[0])) {
					fprintf(stderr, "short write "
					    "(%d/%d)\n", n, sizeof (obuf));
					exit(1);
				}
				odx = 0;
			}
			continue;
		}

		/*
		 * Every traceq has at least one buffer w/ one trace entry in
		 * it.  Start copying data from the input buffers to the output
		 * buffer, merging to keep the output data in sorted order.  If
		 * entries share timestamps, round-robin between the queues.
		 */
		for (;;) {
			int buf, next;
			hrtime_t min, m;

			buf = tba->tq_bufx;
			next = tba->tq[buf].tqb_next;
			min = tba->tq[buf].tqb_buf->t_ent[next].t_time;
			for (i = 1; i < cpus; i++) {
				buf = tba[i].tq_bufx;
				next = tba[i].tq[buf].tqb_next;
				m = tba[i].tq[buf].tqb_buf->t_ent[next].t_time;
				if (m < min) {
					min = m;
				}
			}
			/*
			 * 'min' holds the earliest timestamp remaining
			 * in the queues.
			 */

			for (i = 0; i < cpus; i++) {
				int buf, next;

				buf = tba[i].tq_bufx;
				next = tba[i].tq[buf].tqb_next;

	/* N.B. Bad indentation here to meet cstyle requirements */
		if (tba[i].tq[buf].tqb_buf->t_ent[next].t_time == min) {
			obuf[odx] = tba[i].tq[buf].tqb_buf->t_ent[next];
			obuf[odx].t_pad = i;		/* record CPU # */
			tba[i].tq[buf].tqb_next = ++next;
			if (++odx >= TR_OBUF_SIZE) {
				/*
				 * Full output buffer.  Write it out.
				 */
				if (write(fd, obuf, sizeof (obuf)) !=
				    sizeof (obuf)) {
					fprintf(stderr,
					    "short write (%d/%d)\n",
					    n, sizeof (obuf));
					exit(1);
				}
				odx = 0;
			}
			if (next >= tba[i].tq[buf].tqb_buf->t_limit) {
				/*
				 * Buffer emptied.  Mark it invalid,
				 * switch bufs, and signal producer.
				 */
				mutex_lock(&tba[i].tq_lock);
				tba[i].tq[buf].tqb_valid = 0;
				tba[i].tq_bufx =  ++tba[i].tq_bufx % TrNbuf;
				cond_signal(&tba[i].tq_get_cv);
				mutex_unlock(&tba[i].tq_lock);
				goto getbuf;
			}
		}

			}
		}
	}
}
