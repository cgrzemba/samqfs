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

/*
 *
 * quota.c -- set quotas, display quotas, maybe other stuff
 *
 * quota <options> file
 *
 *   or
 *
 * quota -Q quotafile index <options>
 *
 * legal options:
 *  -h				print help info
 *	-a			select file's admin quota stats
 *	-g			select file's group quota stats
 *	-u			select file's user quota stats
 *	-A #			select admin quota stats on file's FS.
 *	-G group		select group quota stats on file's FS.
 *	-U user			select user quota stats on file's FS.
 *	-e			print output in editable (command) format
 *
 *	-Q #			use file as writeable quota file,
 *				    index # instead of kernel
 *	-k			print/accept all storage quantities in
 *				    units of 1024 bytes
 *
 *  Modify quota entries:
 *
 *	-t {int}:{scope}			set soft limit grace period
 *	-x {"clear"|"expire"|"reset"|int}:{scope}
 *						clear, reset, expire, or
 *						    set soft limit grace expiry
 *	-f {count}:{type}:{scope}	set file count limit
 *	-b {count}:{type}:{scope}	set block count limit
 *
 *	-i				zero all limit fields when setting
 *					    values
 *	-w				suppress warnings from
 *					    -x/-z/:inuse: options
 *	-z				zero all fields when setting values
 *					    (DEBUG only)
 *	-p				print (after edits)
 *
 *
 *	int
 *		Intervals are counted internally in seconds; any value
 *		between 0 and 2^31-1 (~68 years) are permitted.  For input,
 *		specifiers 'w' (weeks), 'd' (days), 'h' (hours),
 *		'm' (minutes), or 's' (seconds) are permitted.
 *		E.g., 1w3d12h specifies 10+1/2 days.
 *
 *	type
 *		Specifies the type of limit or counter involved.  's' or 'soft'
 *		specifies the soft limit, 'h' or 'hard' specifies the hard
 *		limit, and 'u' or 'use' specifies the in-use counter.  (This
 *		last is normally set only by fsck and system administration
 *		tools.)
 *
 *	scope
 *		Specifies the scope of the limit.  'o' or 'online' specifies
 *		the online limit or counter, and 't' or 'total' specifies the
 *		total limit or counter.
 *
 *	count
 *		Specifies a number of items.  This is either a number of
 *		512-byte blocks (1024-byte blocks if the -k option is
 *		present) or a number of files.
 *		Multipliers are permitted:  'k' or 'K' specifies * 1000,
 *		'm' or 'M' * 1,000,000, 'g' or 'G' * 1,000,000,000,
 *		't' or 'T' * 10^12, and 'p' or 'P' * 10^15.
 *
 * If no modification options are selected, then any quota entry (entries)
 * selected will be printed.
 *
 * Intervals ('int' above) may be specified in weeks, days, hours, minutes,
 * and seconds, e.g., 2w3d12h30m12s would specify 17 days, 12:30:12 (from
 * 'now' for -bx or -fx).
 */

#pragma ident "$Revision: 1.26 $"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mnttab.h>
#include <sys/time.h>
#include <sys/errno.h>

/* SAM-FS headers. */
#define DEC_INIT
#include "sam/types.h"
#include "sam/param.h"
#include "pub/stat.h"
#include "sam/quota.h"
#include "sam/lib.h"

#define		MAX_QUOTA_CMDS		512
#define		MAX_QUOTA_OPS		64
#define		MAX_QUOTA_FILES		512

#define		streq(x, y)		(strcmp((x), (y)) == 0)

char *Program;		/* argv[0] in convenient place */
long Now;			/* current time (in seconds) */
int Err = 0;		/* Global error status */
int Danger;			/* We were asked to change an in-use field */

/*
 * Basic program operation:
 *
 *	(1) Parse arguments into following structures.
 *	(2) For each record in RecordList, do:
 *		(a), initialize a quota record according to iflag
 *		(b) "execute" the QuotaOperations list on the record.
 *		(c) Write the record back.
 *		(d) Print the record if we didn't do (3) and (4) or -p was set.
 */

char *FileList[MAX_QUOTA_FILES];
int NFiles = 0;

struct QuotaRecord {
	int type;	/* SAM_QUOTA_{ADMIN,GROUP,USER}* or SAM_QUOTA_FILE* */
	int index;	/* -1 if unspecified */
} RecordList[MAX_QUOTA_CMDS];
int NQuotaRecs = 0;

/*
 * A quota structure wherein all the "in use" fields are filled with
 * 1's.  We can clear all the remaining fields in a quota structure
 * by ANDing this structure with it.  This structure also corresponds
 * with the INIT_LIMITS_ZERO initializer below.
 */
struct sam_dquot limit_quotas = {
	{			/* online values */
		{			/* files */
			~0LL,			/* in use */
			0LL,			/* soft limit */
			0LL,			/* hard limit */
		},
		{			/* blocks */
			~0LL,			/* in use */
			0LL,			/* soft limit */
			0LL,			/* hard limit */
		}
	},
	{			/* total values */
		{			/* files */
			~0LL,			/* in use */
			0LL,			/* soft limit */
			0LL,			/* hard limit */
		},
		{			/* blocks */
			~0LL,			/* in use */
			0LL,			/* soft limit */
			0LL,			/* hard limit */
		}
	},
	0,			/* online grace period */
	~0,			/* online enforcement becomes active */
	0,			/* total grace period */
	~0,			/* total enforcement becomes active */
	0LL,			/* unused */
	0LL			/* unused */
};

#define	INIT_FROM_RECORD	0	/* get quota record from system */
#define	INIT_LIMITS_ZERO	1	/* get usage fields from system */
#define	INIT_ALL_ZERO		2	/* get nothing from system */

#ifdef DEBUG
int dflag = 0;		/* debug */
#endif
int eflag = 0;		/* dump quota info output in editable format */
int iflag = INIT_FROM_RECORD;	/* quota entry source (see INIT_* above) */
int kflag = 0;		/* count blocks in 1024-byte units */
int pflag = 0;		/* print, even if modification options given */
int qflag = 0;		/* do ops on given files instead of using syscall */
int wflag = 1;		/* warn on -x/-z/:inuse: options -- 1 ==> warn */
int tflag = 1;		/* print "total" counts and limits, too */
int OptFlags = 0;	/* all argument flags */

/*
 *	bits in OptFlags.
 */
#define		OPTa		0x00001
#define		OPTb		0x00002
#define		OPTd		0x00004
#define		OPTe		0x00008
#define		OPTf		0x00010
#define		OPTg		0x00020
#define		OPTi		0x00080
#define		OPTk		0x00100
#define		OPTp		0x00200
#define		OPTt		0x00400
#define		OPTu		0x00800
#define		OPTw		0x01000
#define		OPTx		0x02000
#define		OPTz		0x04000
#define		OPTA		0x08000
#define		OPTG		0x10000
#define		OPTO		0x20000
#define		OPTU		0x80000
#define		OPTMASK_Set	(OPTb | OPTf | OPTi | OPTt | OPTx | OPTz)

/*
 *	bits in ParseEl structure.
 */
#define		TYPE_SOFT		0x1
#define		TYPE_HARD		0x2
#define		TYPE_INUSE		0x4
#define		TYPE_MASK		(TYPE_SOFT|TYPE_HARD|TYPE_INUSE)

#define		SCOPE_ONLINE	0x10
#define		SCOPE_TOTAL		0x20
#define		SCOPE_MASK		(SCOPE_ONLINE|SCOPE_TOTAL)

/*
 * Sub-categories of ARG_EXPIRE.
 *
 * Could be put in least-significant nybble if more bits
 * are needed, since they and TYPE_* are mutually exclusive.
 */
#define		EXP_CLEAR		0x100
#define		EXP_RESET		0x200
#define		EXP_EXPIRE		0x400
#define		EXP_MASK		(EXP_CLEAR|EXP_RESET|EXP_EXPIRE)

#define		ARG_BLOCK	0x1000
#define		ARG_FILE	0x2000
#define		ARG_GRACE	0x4000
#define		ARG_EXPIRE	0x8000
#define		ARG_MASK	(ARG_BLOCK|ARG_FILE|ARG_GRACE|ARG_EXPIRE)

struct ParseEl {
	char *str;
	int flags;
} QModifiers[] = {
	{ "s",		TYPE_SOFT },
	{ "soft",	TYPE_SOFT },
	{ "h",		TYPE_HARD },
	{ "hard",	TYPE_HARD },
	{ "u",		TYPE_INUSE },
	{ "inuse",	TYPE_INUSE },
	{ "o",		SCOPE_ONLINE },
	{ "online",	SCOPE_ONLINE },
	{ "t",		SCOPE_TOTAL },
	{ "total",	SCOPE_TOTAL },
	{ NULL, 0 }
};

#define		OP_SET			0x10000
#define		OP_INC			0x20000
#define		OP_DEC			0x40000
#define		OP_RESET		0x80000

struct QuotaOperation {
	int op;
	long long value;
} OperationList[MAX_QUOTA_OPS];
int NQuotaOps = 0;


/*
 * Random utilities
 */
void BLogAnd(char *, char *, int n);	/* logically AND two arrays */
int Ask(char *, char);			/* Query user for y/n answer */
static void TimePrint(long);		/* print seconds in AwBdChDmEs fmt */

/*
 * This proc adds to the file list
 */
void ProcFile(char *arg);			/* file ... */

/*
 * These procs add entries to the RecordList
 */
void ProcAdminArg(char *);		/* -A # */
void ProcGroupArg(char *);		/* -G user/GID */
void ProcUserArg(char *);		/* -U user/UID */
void AddRecord(int, int);

/*
 * These procs add entries to the OperationList
 */
void ProcBlockArg(char *);	/* -b {count}{multiplier}:{type}:{scope} */
void ProcFileArg(char *);	/* -f {count}{multiplier}:{type}:{scope} */
void ProcTimeArg(char *);	/* -t {int}:{scope} */
void ProcExpireArg(char *);	/* -x {int}|clear|reset|expire:{scope} */

/*
 * Things to interpret the data structures that
 * all the arg processing set up.
 */
void ProcessFiles(void);
void ProcessRecords(char *);
void ProcessOperations(struct sam_dquot *, char *, int, int);
void ProcessQuotaOp(struct sam_dquot *, struct QuotaOperation *);

void AddOperation(int, long long);
void AddBlockOp(int, long long);
void AddFileOp(int, long long);
void AddGraceOp(int, long long);
void AddExpireOp(int, long long);
void AddPrimOp(int, long long);

int ParseCount(char *, long long *, int *);
int ParseWhen(char *, int *, int *);


void
Help(int xstatus)
{
	fprintf(stderr,
"\n"
"  %s:\n"
"	%s <options> [file ... ]\n"
"	\n"
"  Legal options:\n"
"	-h			print help info\n"
"	-a			select file's admin quota stats\n"
"	-g			select file's group quota stats\n"
"	-u			select file's user quota stats\n"
"	-e			print in editable format\n"
"\n"
"\n", Program, Program);

	if (getuid() == 0) {
		fprintf(stderr,
"\n"
"	-A #			select admin # quota stats on file's FS.\n"
"	-G group		select group # quota stats on file's FS.\n"
"	-U user			select user # quota stats on file's FS.\n"
"\n"
"  Modify quota entries: (super-user only)\n"
"\n"
"	-t {int}:{scope}\n"
"			- set grace period on soft limit\n"
"	-x {\"clear\"|\"reset\"|\"expire\"|int}:{scope}\n"
"			- clear, reset, expire, or set grace expiry\n"
"	-b {count}:{type}:{scope}	set block usage limit or counter\n"
"	-f {count}:{type}:{scope}	set file usage limit or counter\n"
"	-i			zero all limit fields when setting values\n"
#ifdef DEBUG
"	-w			suppress warnings from -x/-z/:inuse: options\n"
"	-z			zero all fields when setting values\n"
#else	/* DEBUG */
"	-w			suppress warnings from -x/:inuse: options\n"
#endif	/* DEBUG */
"\n"
"	If no modification options are selected, then any quota entry "
	"(entries)\n"
"	selected will be printed.\n"
"\n"
"	Intervals ('int' above) may be specified in weeks, days, hours, "
	"minutes,\n"
"	and seconds, e.g., 2w3d12h30m12s would specify 17 days, 12:30:12 "
	"(from\n"
"	'now' for the -x option).\n");
	}
	exit(xstatus);
}

/*
 * Return true if all of the string's chars are digits.
 */
int
NumericStr(char *s)
{
	while (*s) {
		if (*s < '0' || *s > '9')
			return (0);
		s++;
	}
	return (1);
}

static int	HdrPrinted = 0;

static void
PrintQuotaHdr(void)
{
	if (HdrPrinted)
		return;

	if (tflag) {
		if (eflag) {
			printf("# Type  ID\n");
			printf(
"#                Online Limits                          Total   Limits\n");
			printf(
"#            soft            hard                    soft            hard\n");
			printf("# Files\n");
			printf("# Blocks\n");
			printf("# Grace Periods\n");
			printf("#\n");
		} else {
			printf(
"                                 Online Limits                "
			    "Total Limits\n");
			printf(
"        Type    ID    In Use     Soft     Hard    In Use     "
			    "Soft     Hard\n");
		}
	} else {
		if (eflag) {
			printf("# Type  ID\n");
			printf("#                  Limits\n");
			printf("#            soft            hard\n");
			printf("# Files\n");
			printf("# Blocks\n");
			printf("# Grace Periods\n");
			printf("#\n");
		} else {
			printf("                                    Limits\n");
			printf("        Type    ID    In Use     "
			    "Soft     Hard\n");
		}
	}
	HdrPrinted = 1;
}

static void
PrintQuotaEntry(struct sam_dquot *dq, char *arg, int type, int index)
{
	int kfactor = kflag ? 2 : 1;

#ifdef DEBUG
	if (dflag) {
		fprintf(stderr,
		    "command_print('%s', dq=%lx, arg='%s', type=%d, idx=%d)\n",
		    Program, (long)dq, arg, type, index);
	}
#endif	/* DEBUG */

	PrintQuotaHdr();

	if (eflag) {
		if (tflag) {
			printf("%s -%c %d %s\\\n",
			    Program, "zAGUxQ"[type+1], index,
			    kflag ? "-k" : "");
			printf("      -f %8lld:s:o -f %8lld:h:o         "
			    "-f %8lld:s:t -f %8lld:h:t \\\n",
			    dq->dq_folsoft, dq->dq_folhard,
			    dq->dq_ftotsoft, dq->dq_ftothard);
			printf("      -b %8lld:s:o -b %8lld:h:o         "
			    "-b %8lld:s:t -b %8lld:h:t \\\n",
			    dq->dq_bolsoft/kfactor, dq->dq_bolhard/kfactor,
			    dq->dq_btotsoft/kfactor, dq->dq_btothard/kfactor);
			printf("                 -t  ");
			TimePrint(dq->dq_ol_grace);
			printf(":o                               -t ");
			TimePrint(dq->dq_tot_grace);
			printf(":t");
			if (dq->dq_ol_enforce || dq->dq_tot_enforce) {
				printf(" \\\n      -x ");
				if (dq->dq_ol_enforce) {
					if (dq->dq_ol_enforce > Now) {
						TimePrint(
						    dq->dq_ol_enforce-Now);
						printf(":o ");
					} else {
						printf("expire:o ");
					}
				} else {
					printf("clear  ");
				}
				printf("                      -x ");
				if (dq->dq_tot_enforce) {
					if (dq->dq_tot_enforce > Now) {
						TimePrint(
						    dq->dq_tot_enforce-Now);
						printf(":t ");
					} else {
						printf("expire:t ");
					}
				} else {
					printf("clear  ");
				}
			}
			printf("   %s\n", arg);
		} else {
			printf("%s -%c %d %s\\\n",
			    Program, "zAGUxQ"[type+1], index,
			    kflag ? "-k" : "");
			printf("      -f %8lld:s -f %8lld:h \\\n",
			    dq->dq_folsoft, dq->dq_folhard);
			printf("      -b %8lld:s -b %8lld:h \\\n",
			    dq->dq_bolsoft/kfactor, dq->dq_bolhard/kfactor);
			printf("                 -t  ");
			TimePrint(dq->dq_ol_grace);
			if (dq->dq_ol_enforce) {
				printf(" \\\n      -x ");
				if (dq->dq_ol_enforce) {
					if (dq->dq_ol_enforce > Now) {
						TimePrint(
						    dq->dq_ol_enforce-Now);
					} else {
						printf("expire ");
					}
				} else {
					printf("clear  ");
				}
			}
			printf("   %s\n", arg);
		}
	} else {
		if (tflag) {
			char c1, c2;

			printf("%s\n", arg);
			c1 = c2 = ' ';
			if (!QUOTA_SANE(dq)) {
				c1 = c2 = '!';
			} else if (!QUOTA_INF(dq)) {
				if (dq->dq_folused > dq->dq_folhard) {
					c1 = '!';
				} else if (dq->dq_folused > dq->dq_folsoft) {
					c1 = '+';
					if (dq->dq_ol_enforce &&
					    Now > dq->dq_ol_enforce)
						c1 = '*';
				}
				if (dq->dq_ftotused > dq->dq_ftothard) {
					c2 = '!';
				} else if (dq->dq_ftotused > dq->dq_ftotsoft) {
					c2 = '+';
					if (dq->dq_ol_enforce &&
					    Now > dq->dq_ol_enforce)
						c2 = '*';
				}
			}
			printf("Files  %5s %5d  %8lld%c%8lld %8lld  "
			    "%8lld%c%8lld %8lld",
			    type == SAM_QUOTA_MAX ? "  -  " :
			    quota_types[type], index,
			    dq->dq_folused, c1, dq->dq_folsoft, dq->dq_folhard,
			    dq->dq_ftotused, c2, dq->dq_ftotsoft,
			    dq->dq_ftothard);
			printf("\n");
			c1 = c2 = ' ';
			if (!QUOTA_SANE(dq)) {
				c1 = c2 = '!';
			} else if (!QUOTA_INF(dq)) {
				if (dq->dq_bolused > dq->dq_bolhard) {
					c1 = '!';
				} else if (dq->dq_bolused > dq->dq_bolsoft) {
					c1 = '+';
					if (dq->dq_ol_enforce &&
					    Now > dq->dq_ol_enforce)
						c1 = '*';
				}
				if (dq->dq_btotused > dq->dq_btothard) {
					c2 = '!';
				} else if (dq->dq_btotused > dq->dq_btotsoft) {
					c2 = '+';
					if (dq->dq_tot_enforce &&
					    Now > dq->dq_tot_enforce)
						c2 = '*';
				}
			}
			printf("Blocks %5s %5d  %8lld%c%8lld %8lld  "
			    "%8lld%c%8lld %8lld",
			    type == SAM_QUOTA_MAX ? "  -  " :
			    quota_types[type], index,
			    dq->dq_bolused/kfactor, c1,
			    dq->dq_bolsoft/kfactor, dq->dq_bolhard/kfactor,
			    dq->dq_btotused/kfactor, c2,
			    dq->dq_btotsoft/kfactor, dq->dq_btothard/kfactor);
			printf("\nGrace period                    ");
			TimePrint(dq->dq_ol_grace);
			printf("                          ");
			TimePrint(dq->dq_tot_grace);
			printf("\n");
			if (index && dq->dq_ol_enforce) {
				if (dq->dq_ol_enforce > Now) {
					printf("---> Warning:  online soft "
					    "limits to be enforced in ");
					TimePrint(dq->dq_ol_enforce-Now);
					printf("\n");
				} else {
					printf("---> Online soft limits "
					    "under enforcement (since ");
					TimePrint(Now-dq->dq_ol_enforce);
					printf(" ago)\n");
				}
			}
			if (index && dq->dq_tot_enforce) {
				if (dq->dq_tot_enforce > Now) {
					printf("---> Warning:  total soft "
					    "limits to be enforced in ");
					TimePrint(dq->dq_tot_enforce-Now);
					printf("\n");
				} else {
					printf("---> Total soft limits under "
					    "enforcement (since ");
					TimePrint(Now-dq->dq_tot_enforce);
					printf(" ago)\n");
				}
			}
			if (!QUOTA_SANE(dq)) {
				printf("---> Quota values inconsistent; "
				    "zero quotas in effect.\n");
			} else if (index == 0 || QUOTA_INF(dq)) {
				printf("---> Infinite quotas in effect.\n");
			}
		} else {
			char c1;

			printf("%s\n", arg);
			c1 = ' ';
			if (!QUOTA_SANE(dq)) {
				c1 = '!';
			} else if (!QUOTA_INF(dq)) {
				if (dq->dq_folused > dq->dq_folhard) {
					c1 = '!';
				} else if (dq->dq_folused > dq->dq_folsoft) {
					c1 = '+';
					if (dq->dq_ol_enforce &&
					    Now > dq->dq_ol_enforce)
						c1 = '*';
				}
			}
			printf("Files  %5s %5d  %8lld%c%8lld %8lld",
			    type == SAM_QUOTA_MAX ? "  -  " :
			    quota_types[type], index,
			    dq->dq_folused, c1, dq->dq_folsoft,
			    dq->dq_folhard);
			printf("\n");
			c1 = ' ';
			if (!QUOTA_SANE(dq)) {
				c1 = '!';
			} else if (!QUOTA_INF(dq)) {
				if (dq->dq_bolused > dq->dq_bolhard) {
					c1 = '!';
				} else if (dq->dq_bolused > dq->dq_bolsoft) {
					c1 = '+';
					if (dq->dq_ol_enforce &&
					    Now > dq->dq_ol_enforce)
						c1 = '*';
				}
			}
			printf("Blocks %5s %5d  %8lld%c%8lld %8lld",
			    type == SAM_QUOTA_MAX ? "  -  " :
			    quota_types[type], index,
			    dq->dq_bolused/kfactor, c1,
			    dq->dq_bolsoft/kfactor, dq->dq_bolhard/kfactor);
			printf("\nGrace period                    ");
			TimePrint(dq->dq_ol_grace);
			printf("\n");
			if (index && dq->dq_ol_enforce) {
				if (dq->dq_ol_enforce > Now) {
					printf("---> Warning:  online soft "
					    "limits to be enforced in ");
					TimePrint(dq->dq_ol_enforce-Now);
					printf("\n");
				} else {
					printf("---> Online soft limits "
					    "under enforcement (since ");
					TimePrint(Now-dq->dq_ol_enforce);
					printf(" ago)\n");
				}
			}
			if (!QUOTA_SANE(dq)) {
				printf("---> Quota values inconsistent; "
				    "zero quotas in effect.\n");
			} else if (index == 0 || QUOTA_INF(dq)) {
				printf("---> Infinite quotas in effect.\n");
			}
		}
	}
}

static void
ReadSamFSes(void)
{
	struct flock lb;
	struct mnttab mntent;
	FILE *mntfd;

	lb.l_type = F_RDLCK;
	lb.l_whence = 0;
	lb.l_start = 0;
	lb.l_len = 0;

	if ((mntfd = fopen(MNTTAB, "r")) == NULL) {
		fprintf(stderr,
		    "%s:  Can't open mount table (%s).\n", Program, MNTTAB);
		Err++;
		return;
	}

	if (fcntl(fileno(mntfd), F_SETLKW, &lb) < 0) {
		fclose(mntfd);
		fprintf(stderr,
		    "%s:  Cannot lock mount table ('%s') for read.\n",
		    Program, MNTTAB);
		Err++;
		return;
	}

	while (getmntent(mntfd, &mntent) == 0) {
		if (!mntent.mnt_special)
			continue;
		if (!mntent.mnt_mountp)
			continue;
		if (!mntent.mnt_fstype)
			continue;
		if (!streq(mntent.mnt_fstype, "samfs"))
			continue;
		FileList[NFiles] = strdup(mntent.mnt_mountp);
		NFiles++;
	}
	fclose(mntfd);
}

int
main(int ac, char *av[])
{
	int c;
	struct timeval tp;
#ifdef DEBUG
	char *optstr = "A:ab:def:G:ghikOpt:U:uwx:z";
#else	/* DEBUG */
	char *optstr = "A:ab:ef:G:ghikOpt:U:uwx:";
#endif	/* DEBUG */

	Program = av[0];

	gettimeofday(&tp, NULL);
	Now = tp.tv_sec;

	while ((c = getopt(ac, av, optstr)) != EOF) {
		switch (c) {
		case 'A':	/* arg (admin set) */
			OptFlags |= OPTA;
			ProcAdminArg(optarg);
			break;

		case 'a':
			OptFlags |= OPTa;
			AddRecord(SAM_QUOTA_ADMIN, -1);
			break;

		case 'b':	/* arg (block count) */
			OptFlags |= OPTb;
			ProcBlockArg(optarg);
			break;

#ifdef DEBUG
		case 'd':
			OptFlags |= OPTd;
			dflag++;
			break;
#endif	/* DEBUG */

		case 'e':
			OptFlags |= OPTe;
			eflag++;
			pflag++;
			break;

		case 'f':	/* arg (file count) */
			OptFlags |= OPTf;
			ProcFileArg(optarg);
			break;

		case 'G':	/* arg (group ID) */
			OptFlags |= OPTG;
			ProcGroupArg(optarg);
			break;

		case 'g':
			OptFlags |= OPTg;
			AddRecord(SAM_QUOTA_GROUP, getegid());
			break;

		case 'h':	/* help.  No return */
			Help(0);
			break;

		case 'i':
			OptFlags |= OPTi;
			iflag = INIT_LIMITS_ZERO;
			break;

		case 'k':
			OptFlags |= OPTk;
			kflag++;
			break;

		case 'O':	/* Don't print total counts */
			OptFlags |= OPTO;
			tflag = 0;
			break;

		case 'p':
			OptFlags |= OPTp;
			pflag++;
			break;

		case 't':	/* arg (grace period) */
			OptFlags |= OPTt;
			ProcTimeArg(optarg);
			break;

		case 'U':	/* arg (user ID) */
			OptFlags |= OPTU;
			ProcUserArg(optarg);
			break;

		case 'u':
			OptFlags |= OPTu;
			AddRecord(SAM_QUOTA_USER, geteuid());
			break;

		case 'w':
			OptFlags |= OPTw;
			wflag = 0;
			break;

		case 'x':	/* arg (expiry) */
			OptFlags |= OPTx;
			ProcExpireArg(optarg);
			break;

#ifdef DEBUG
		case 'z':
			OptFlags |= OPTz;
			iflag = INIT_ALL_ZERO;
			break;
#endif	/* DEBUG */

		default:
			Help(1);
		}
	}

	for (; optind < ac; optind++) {
		ProcFile(av[optind]);
	}

	/*
	 *	Check options for validity.
	 */
#define	TWOBITS(a, b, c) ((a & (b | c)) == (b | c))

	if (OptFlags & OPTMASK_Set) {
		if (NFiles == 0 || (OptFlags & (OPTa | OPTg | OPTu))) {
			fprintf(stderr,
			    "%s:  The options require a file.\n", Program);
			Help(1);
		}

	} else {	/* just reporting */
	}
	if (TWOBITS(OptFlags, OPTa, OPTA) || TWOBITS(OptFlags, OPTg, OPTG) ||
	    TWOBITS(OptFlags, OPTu, OPTU)) {
		fprintf(stderr, "%s:  Incorrect option usage.\n", Program);
		Help(1);
	}

	if (!Err)
		ProcessFiles();
	return (Err ? 10 : 0);
}

void
ProcessFiles(void)
{
	int i;

	if (NQuotaRecs == 0) {
		if (NFiles != 0)
			AddRecord(SAM_QUOTA_ADMIN, -1);
		AddRecord(SAM_QUOTA_GROUP, -1);
		AddRecord(SAM_QUOTA_USER, -1);
	}
	if (NFiles == 0 && NQuotaOps == 0)
		ReadSamFSes();
	for (i = 0; i < NFiles && !Err; i++)
		ProcessRecords(FileList[i]);
}


void
ProcessRecords(char *file)
{
	int i, fd;
	struct sam_dquot dq;
	int index, type, r;
	extern int errno;

	for (i = 0; i < NQuotaRecs && !Err; i++) {
		/*
		 * Get a file descriptor thru which to access the quota record
		 */
		type = RecordList[i].type;
		index = RecordList[i].index;

		fd = open(file, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr,
			    "%s:  Cannot open '%s'\n", Program, file);
			Err++;
			return;
		}

		/*
		 * Initialize the quota record appropriately.
		 */
		if (iflag == INIT_ALL_ZERO) {
			if (wflag &&
			    !Ask("Resetting All In-Use Fields:  continue? ",
			    'n')) {
				Err++;
				return;
			}
			bzero((char *)&dq, sizeof (dq));
		} else {
			if (index < 0) {
				if (access(file, W_OK) == 0) {
					struct sam_stat stbuf;

					if (sam_stat(file, &stbuf,
					    sizeof (stbuf)) != 0) {
						fprintf(stderr,
						    "%s:  Cannot stat '%s'\n",
						    Program, file);
						Err++;
						close(fd);
						return;
					}
					switch (type) {
					case SAM_QUOTA_ADMIN:
						index = stbuf.admin_id;
						break;
					case SAM_QUOTA_GROUP:
						index = stbuf.st_gid;
						break;
					case SAM_QUOTA_USER:
						index = stbuf.st_uid;
						break;
					}
				} else {
					switch (type) {
					case SAM_QUOTA_ADMIN:
						index = 0;
						break;
					case SAM_QUOTA_GROUP:
						index = getgid();
						break;
					case SAM_QUOTA_USER:
						index = getuid();
						break;
					}
				}
			}
			r = sam_get_quota_entry_by_index(fd, type, index, &dq);
			if (r < 0) {
				switch (errno) {
				case ENOENT:
					fprintf(stderr,
					    "%s: No %s quota entry for "
					    "'%s'.\n",
					    Program, quota_types[type], file);
					close(fd);
					continue;
				case EACCES:
					fprintf(stderr,
					    "%s: Couldn't get %s quota entry"
					    " for '%s' (no permission)\n",
					    Program, quota_types[type], file);
					close(fd);
					continue;
				case ENOTTY:
					fprintf(stderr,
					    "%s: Quotas not enabled"
					    " on '%s' filesystem.\n",
					    Program, file);
					close(fd);
					continue;
				default:
					fprintf(stderr,
					    "%s: Couldn't get %s quota entry"
					    " for file '%s' (%s)\n",
					    Program, quota_types[type],
					    file, strerror(errno));
				}
				Err++;
				close(fd);
				return;
			}
		}
		if (iflag == INIT_LIMITS_ZERO) {
			BLogAnd((char *)&dq, (char *)&limit_quotas,
			    sizeof (dq));
		}
		Danger = 0;
		ProcessOperations(&dq, file, type, index);
		if ((iflag != INIT_FROM_RECORD || NQuotaOps > 0) && !Err) {
			if (Danger)
				r = sam_putall_quota_entry(fd, type,
				    index, &dq);
			else
				r = sam_put_quota_entry(fd, type, index, &dq);
			if (r < 0) {
				switch (errno) {
				case EPERM:
				case EACCES:
					fprintf(stderr,
					    "%s:  Permission denied\n",
					    Program);
					break;
				default:
					fprintf(stderr,
					    "%s:  sam_put_quota_entry(fd=%d, "
					    "type=%d, index=%d, &dq) "
					    "failed:  errno=%d\n",
					    Program, fd, type, index, errno);
				}
				Err++;
			} else if (NQuotaOps == 0 || pflag || eflag) {
				/*
				 * Print updated quota entry.
				 */
				r = sam_get_quota_entry_by_index(fd, type,
				    index, &dq);
				if (r < 0) {
					switch (errno) {
					case ENOENT:
						fprintf(stderr,
						    "%s: No %s quota entry "
						    "for '%s'.\n",
						    Program,
						    quota_types[type], file);
						close(fd);
						continue;
					case EACCES:
						fprintf(stderr,
						    "%s: Couldn't get "
						    "updated %s quota entry"
						    " for '%s' "
						    "(no permission)\n",
						    Program,
						    quota_types[type], file);
						close(fd);
						continue;
					case ENOTTY:
						fprintf(stderr,
						    "%s: Quotas not enabled"
						    " on '%s' filesystem.\n",
						    Program, file);
						close(fd);
						continue;
					default:
						fprintf(stderr,
						    "%s: Couldn't get %s "
						    "updated quota entry"
						    " for file '%s' (%s)\n",
						    Program,
						    quota_types[type], file,
						    strerror(errno));
					}
					close(fd);
					continue;
				}
				PrintQuotaEntry(&dq, file, type, index);
			}
		} else {
			if (!Err) {
				PrintQuotaEntry(&dq, file, type, index);
			}
		}
		close(fd);
	}
}

void
ProcessOperations(
	struct sam_dquot *dqp,
	char *name,
	int type, int index)
{
	int i;

	for (i = 0; i < NQuotaOps && !Err; i++) {
		ProcessQuotaOp(dqp, &OperationList[i]);
	}
}

/*
 * This is just brute force, and it doesn't handle the
 * OP_INC or OP_DEC operations at all.  Ought to be
 * rewritten to handle this; then shell syntax like:
 *
 * if `squota -f +25:o:t -b +10m:o:t -X .`
 * then
 *   echo 'Starting job'
 *   runjob...
 * else
 *   echo 'This job would run over quota'
 *   exit 1
 * fi
 *
 * could be legal, make sense, and maybe be highly useful.
 */
void
ProcessQuotaOp(struct sam_dquot *dq, struct QuotaOperation *op)
{
	long long l;

	l = op->value;
	switch (op->op) {
	case OP_SET|ARG_FILE|SCOPE_ONLINE|TYPE_SOFT:
		dq->dq_folsoft = l;
		break;
	case OP_SET|ARG_FILE|SCOPE_ONLINE|TYPE_HARD:
		dq->dq_folhard = l;
		break;
	case OP_SET|ARG_FILE|SCOPE_ONLINE|TYPE_INUSE:
		Danger++;
		dq->dq_folused = l;
		break;
	case OP_SET|ARG_FILE|SCOPE_TOTAL|TYPE_SOFT:
		dq->dq_ftotsoft = l;
		break;
	case OP_SET|ARG_FILE|SCOPE_TOTAL|TYPE_HARD:
		dq->dq_ftothard = l;
		break;
	case OP_SET|ARG_FILE|SCOPE_TOTAL|TYPE_INUSE:
		Danger++;
		dq->dq_ftotused = l;
		break;

	case OP_SET|ARG_BLOCK|SCOPE_ONLINE|TYPE_SOFT:
		dq->dq_bolsoft = l;
		break;
	case OP_SET|ARG_BLOCK|SCOPE_ONLINE|TYPE_HARD:
		dq->dq_bolhard = l;
		break;
	case OP_SET|ARG_BLOCK|SCOPE_ONLINE|TYPE_INUSE:
		Danger++;
		dq->dq_bolused = l;
		break;
	case OP_SET|ARG_BLOCK|SCOPE_TOTAL|TYPE_SOFT:
		dq->dq_btotsoft = l;
		break;
	case OP_SET|ARG_BLOCK|SCOPE_TOTAL|TYPE_HARD:
		dq->dq_btothard = l;
		break;
	case OP_SET|ARG_BLOCK|SCOPE_TOTAL|TYPE_INUSE:
		Danger++;
		dq->dq_btotused = l;
		break;

	case OP_SET|ARG_GRACE|SCOPE_ONLINE:
		dq->dq_ol_grace = (int)l;
		break;
	case OP_SET|ARG_GRACE|SCOPE_TOTAL:
		dq->dq_tot_grace = (int)l;
		break;

	case OP_SET|ARG_EXPIRE|SCOPE_ONLINE:
		Danger++;
		dq->dq_ol_enforce = (sam_time_t)l;
		break;
	case OP_SET|ARG_EXPIRE|SCOPE_TOTAL:
		Danger++;
		dq->dq_tot_enforce = (sam_time_t)l;
		break;

	case OP_RESET|ARG_EXPIRE|SCOPE_ONLINE:
		Danger++;
		dq->dq_ol_enforce = (sam_time_t)(Now + dq->dq_ol_grace);
		break;
	case OP_RESET|ARG_EXPIRE|SCOPE_TOTAL:
		Danger++;
		dq->dq_tot_enforce = (sam_time_t)(Now + dq->dq_tot_grace);
		break;

	default:
		fprintf(stderr, "%s:  unrecognized QuotaOp: '%#x'\n",
		    Program, op->op);
		Err++;
	}
}


void
ProcFile(char *arg)
{
	FileList[NFiles++] = arg;
}

/*
 * These procs add entries to the RecordList
 */

/*
 * -A #
 */
void
ProcAdminArg(char *arg)
{
	if (NumericStr(arg))
		AddRecord(SAM_QUOTA_ADMIN, atoi(arg));
	else {
		fprintf(stderr, "%s:  unrecognized admin ID: '%s'\n",
		    Program, arg);
		Err++;
	}
}

/*
 * -G user/GID
 */
void
ProcGroupArg(char *grp)
{
	struct group *pg;

	pg = getgrnam(grp);
	if (pg)
		AddRecord(SAM_QUOTA_GROUP, pg->gr_gid);
	else if (NumericStr(grp))
		AddRecord(SAM_QUOTA_GROUP, atoi(grp));
	else {
		fprintf(stderr, "%s:  unrecognized group name or ID: '%s'\n",
		    Program, grp);
		Err++;
	}
}

/*
 * -U user/UID
 */
void
ProcUserArg(char *user)
{
	struct passwd *pw;

	pw = getpwnam(user);
	if (pw)
		AddRecord(SAM_QUOTA_USER, pw->pw_uid);
	else if (NumericStr(user))
		AddRecord(SAM_QUOTA_USER, atoi(user));
	else {
		fprintf(stderr, "%s:  unrecognized user name or ID: '%s'\n",
		    Program, user);
		Err++;
	}
}

void
AddRecord(int type, int index)
{
	if (NQuotaRecs >= MAX_QUOTA_CMDS) {
		fprintf(stderr,
		    "%s:  too many -A, -G, or -U arguments\n", Program);
		Err++;
		return;
	}
	RecordList[NQuotaRecs].type = type;
	RecordList[NQuotaRecs].index = index;
	NQuotaRecs++;
}

/*
 * These procs add entries to the OperationList
 */

/*
 * -b {count}{multiplier}:{type}:{scope}
 */
void
ProcBlockArg(char *arg)
{
	long long nblocks;
	int flags;

	if (ParseCount(arg, &nblocks, &flags)) {
		fprintf(stderr, "Ill-formed argument: -b '%s'\n", arg);
		Err++;
		return;
	}
	if (kflag)
		nblocks *= 2;	/* count is 1024-byte blocks, not 512-byte */
	flags |= ARG_BLOCK;
	AddOperation(flags, nblocks);
}

/*
 * -f {count}{multiplier}:{type}:{scope}
 */
void
ProcFileArg(char *arg)
{
	long long nfiles;
	int flags;

	if (ParseCount(arg, &nfiles, &flags)) {
		fprintf(stderr, "Ill-formed argument: -f '%s'\n", arg);
		Err++;
		return;
	}
	flags |= ARG_FILE;
	AddOperation(flags, nfiles);
}

/*
 * -t {int}:{scope}
 */
void
ProcTimeArg(char *arg)
{
	int when, flags;

	if (ParseWhen(arg, &when, &flags)) {
		fprintf(stderr, "Ill-formed interval: -t '%s'\n", arg);
		Err++;
		return;
	}
	if (flags & EXP_MASK) {
		fprintf(stderr, "Ill-formed interval: -t '%s' "
		    "-- clear/reset/expire don't apply\n", arg);
		Err++;
		return;
	}
	flags |= ARG_GRACE;
	AddOperation(flags, when);
}

/*
 * -x {int}|clear|reset|expire:{scope}
 */
void
ProcExpireArg(char *arg)
{
	int when, flags;

	if (ParseWhen(arg, &when, &flags)) {
		fprintf(stderr, "Ill-formed argument: -x '%s'\n", arg);
		Err++;
		return;
	}
	flags |= ARG_EXPIRE;
	AddOperation(flags, when);
}

/*
 * The following functions parse the compound arguments of the sorts
 * -f countMult:type:scope
 * -b countMult:type:scope
 * -t interval:scope
 * -x {expire_opt,interval}:scope
 *
 * where:
 *		countMult = {0-9}{0-9}*{k,K,m,M,g,G,t,T}*
 *		type = {s,soft,h,hard,u,inuse}
 *		scope = {o,online,t,total}
 *		interval = {0-9}{0-9}*{w,d,h,m,s}{{0-9}{0-9}*{w,d,h,m,s}}*
 *		expire_opt = {clear,reset,expire}
 */

/*
 * Parse an argument of the form N:opt:opt,
 * allowing multipliers for N (k *= 1000, m *= 1000*1000, g *= ...)
 *
 * e.g., 120M:soft:online
 *
 * also allows specifiers like 120M:soft:hard:online
 */
int
ParseCount(char *arg, long long *count, int *flags)
{
	long long t;
	int f;
	char *s;
	struct ParseEl *p;

	t = 0;
	for (s = strtok(arg, ":,./"); s != NULL; s++) {
		if (*s >= '0' && *s <= '9') {
			t = 10*t + (*s - '0');
		} else
			break;
	}
	for (; *s; s++) {
		switch (*s) {
		case 'k':
		case 'K':
			t *= 1000LL;
			break;
		case 'm':
		case 'M':
			t *= 1000LL * 1000LL;
			break;
		case 'g':
		case 'G':
			t *= 1000LL * 1000LL * 1000LL;
			break;
		case 't':
		case 'T':
			t *= 1000LL * 1000LL * 1000LL * 1000LL;
			break;
		case 'p':
		case 'P':
			t *= 1000LL * 1000LL * 1000LL * 1000LL * 1000LL;
			break;
		default:
			return (-1);
		}
	}

	f = 0;
	for (s = strtok(NULL, ":,./"); s != NULL; s = strtok(NULL, ":,./")) {
		for (p = &QModifiers[0]; p->str != NULL; p++) {
			if (streq(p->str, s)) {
				f |= p->flags;
				break;
			}
		}
		if (p->str == NULL) {
			fprintf(stderr,
			    "%s:  ParseCount:  unrecognized specifier:  '%s'\n",
			    Program, s);
			return (-1);
		}
	}
	*flags = f;
	if ((f & (SCOPE_TOTAL | SCOPE_ONLINE)) == 0) {
		*flags |= SCOPE_TOTAL | SCOPE_ONLINE;
	}
	*count = t;
	return (0);
}

/*
 * Parse an argument of the form interval:opt,
 * where interval can include week, day, hour, minute, ... specifiers,
 * e.g., 1w3d12h6m:online
 */
#define		WEEK2SECS	(7*24*60*60)
#define		DAY2SECS	(24*60*60)
#define		HOUR2SECS	(60*60)
#define		MINUTE2SECS	(60)

int
ParseWhen(char *arg, int *count, int *flags)
{
	long long i, t;
	int f, g;
	struct ParseEl *p;
	char *s;

	f = g = 0;
	i = t = 0;
	s = strtok(arg, ":,./");
	if (streq(s, "clear")) {
		f = EXP_CLEAR;
	} else if (streq(s, "reset")) {
		f = EXP_RESET;
	} else if (streq(s, "expire")) {
		f = EXP_EXPIRE;
	} else {
		while (*s != '\0') {
			if (*s >= '0' && *s <= '9') {
				i = 10*i + (*s - '0');
			} else {
				switch (*s) {
				case 'W':
				case 'w':
					i *= WEEK2SECS;
					break;
				case 'D':
				case 'd':
					i *= DAY2SECS;
					break;
				case 'H':
				case 'h':
					i *= HOUR2SECS;
					break;
				case 'M':
				case 'm':
					i *= MINUTE2SECS;
					break;
				case 'S':
				case 's':
					break;
				default:
					return (-1);
				}
				t += i;
				i = 0;
			}
			s++;
		}
		t += i;
	}

	for (s = strtok(NULL, ":,./"); s != NULL; s = strtok(NULL, ":,./")) {
		for (p = &QModifiers[0]; p->str != NULL; p++) {
			if (streq(p->str, s)) {
				g |= p->flags;
				break;
			}
		}
		if (p->str == NULL) {
			fprintf(stderr,
			    "%s:  ParseWhen:  unrecognized specifier '%s'\n",
			    Program, s);
			return (-1);
		}
	}

	if (g & TYPE_MASK) {
		fprintf(stderr,
		    "%s:  ParseWhen:  Inappropriate type specifier\n",
		    Program);
		return (-1);
	}
	*count = t;
	*flags = f | g;
	if ((*flags & (SCOPE_TOTAL | SCOPE_ONLINE)) == 0) {
		*flags |= SCOPE_TOTAL | SCOPE_ONLINE;
	}
	return ((t == *count) ? 0 : -1);			/* overflow? */
}

/*
 * Print n seconds in 'w d h m s' format.
 */
static void
TimePrint(long n)
{
	long r;

	if (n == 0) {
		printf("0s");
		return;
	}
	if ((r = (n / WEEK2SECS)) != 0) {
		printf("%ldw", r);
		n -= r * WEEK2SECS;
	}
	if ((r = (n / DAY2SECS)) != 0) {
		printf("%ldd", r);
		n -= r * DAY2SECS;
	}
	if ((r = (n / HOUR2SECS)) != 0) {
		printf("%ldh", r);
		n -= r * HOUR2SECS;
	}
	if ((r = (n / MINUTE2SECS)) != 0) {
		printf("%ldm", r);
		n -= r * MINUTE2SECS;
	}
	if ((r = n) != 0) {
		printf("%lds", r);
		n -= r;
	}
}

/*
 * Convert parsed argument into one or more operations
 */
void
AddOperation(int flags, long long n)
{
	switch (flags&ARG_MASK) {
	case ARG_BLOCK:
		AddBlockOp(flags, n);
		break;
	case ARG_FILE:
		AddFileOp(flags, n);
		break;
	case ARG_GRACE:
		AddGraceOp(flags, n);
		break;
	case ARG_EXPIRE:
		AddExpireOp(flags, n);
		break;
	default:
		fprintf(stderr,
		    "%s:  bad command argument (%#x)\n", Program, flags);
		Err++;
		return;
	}
}

#define		CLRBIT(x)		((x)&(x-1))
#define		GETBIT(x)		((x)^CLRBIT(x))

/*
 * -b countMult:type:scope
 *
 * Explode these kinds of flags into full sets of primitive
 * operations, e.g., -b 10m:s,h:o,t expands into four operations:
 * { 10m:s:o + 10m:h:o + 10m:s:t + 10m:h:t }.
 */
void
AddBlockOp(int flags, long long n)
{
	int TypeMask, ScopeMask;
	int TypeBit, ScopeBit, opbase;

	opbase = OP_SET|ARG_BLOCK;

	ScopeMask = flags&SCOPE_MASK;
	for (ScopeBit = GETBIT(ScopeMask); ScopeBit != 0;
	    ScopeBit = GETBIT(ScopeMask)) {
		TypeMask = flags&TYPE_MASK;
		for (TypeBit = GETBIT(TypeMask); TypeBit != 0;
		    TypeBit = GETBIT(TypeMask)) {
			switch (ScopeBit|TypeBit) {
			case SCOPE_ONLINE|TYPE_INUSE:
			case SCOPE_TOTAL|TYPE_INUSE:
				if (wflag &&
				    !Ask("Setting In-Use Field:  continue? ",
				    'n')) {
					Err++;
					return;
				}
				AddPrimOp(opbase|ScopeBit|TypeBit, n);
				break;
			case SCOPE_ONLINE|TYPE_SOFT:
			case SCOPE_ONLINE|TYPE_HARD:
			case SCOPE_TOTAL|TYPE_SOFT:
			case SCOPE_TOTAL|TYPE_HARD:
				AddPrimOp(opbase|ScopeBit|TypeBit, n);
				break;
			default:
				fprintf(stderr,
				    "%s:  AddBlockOp:  bad option %#x\n",
				    Program, ScopeBit|TypeBit);
				Err++;
			}
			TypeMask &= ~TypeBit;
		}
		ScopeMask &= ~ScopeBit;
	}
}

/*
 * Explode -f N:s,h:o,t ops in fashion similar to -b above.
 */
void
AddFileOp(int flags, long long n)
{
	int TypeMask, ScopeMask;
	int TypeBit, ScopeBit, opbase;

	opbase = OP_SET|ARG_FILE;

	ScopeMask = flags&SCOPE_MASK;
	for (ScopeBit = GETBIT(ScopeMask); ScopeBit != 0;
	    ScopeBit = GETBIT(ScopeMask)) {
		TypeMask = flags&TYPE_MASK;
		for (TypeBit = GETBIT(TypeMask); TypeBit != 0;
		    TypeBit = GETBIT(TypeMask)) {
			switch (ScopeBit|TypeBit) {
			case SCOPE_ONLINE|TYPE_INUSE:
			case SCOPE_TOTAL|TYPE_INUSE:
				if (wflag &&
				    !Ask("Setting In-Use Field:  continue? ",
				    'n')) {
					Err++;
					return;
				}
				AddPrimOp(opbase|ScopeBit|TypeBit, n);
				break;
			case SCOPE_ONLINE|TYPE_SOFT:
			case SCOPE_ONLINE|TYPE_HARD:
			case SCOPE_TOTAL|TYPE_SOFT:
			case SCOPE_TOTAL|TYPE_HARD:
				AddPrimOp(opbase|ScopeBit|TypeBit, n);
				break;
			default:
				fprintf(stderr,
				    "%s:  AddFilekOp:  bad option %#x\n",
				    Program, ScopeBit|TypeBit);
				Err++;
			}
			TypeMask &= ~TypeBit;
		}
		ScopeMask &= ~ScopeBit;
	}
}

/*
 * Expand -g interval:o,t into primitive ops.
 */
void
AddGraceOp(int flags, long long n)
{
	int ScopeMask, ScopeBit, opbase;

	ScopeMask = flags&SCOPE_MASK;

	if ((flags&(TYPE_MASK|EXP_MASK)) != 0) {
		fprintf(stderr, "%s:  AddGraceOp: Invalid flags -- %#x\n",
		    Program, flags);
		Err++;
		return;
	}

	opbase = OP_SET|ARG_GRACE;
	for (ScopeBit = GETBIT(ScopeMask); ScopeBit != 0;
	    ScopeBit = GETBIT(ScopeMask)) {
		switch (ScopeBit) {
		case SCOPE_ONLINE:
		case SCOPE_TOTAL:
			AddPrimOp(opbase|ScopeBit, n);
			break;
		default:
			fprintf(stderr, "%s:  AddGraceOp:  bad option %#x\n",
			    Program, ScopeBit);
			Err++;
		}
		ScopeMask &= ~ScopeBit;
	}
}

/*
 * Expand -t xxx:o,t into primitive ops.
 */
void
AddExpireOp(int flags, long long n)
{
	int ScopeMask, ScopeBit;
	int ExpMask, opbase;

	ScopeMask = flags&SCOPE_MASK;
	ExpMask = flags&EXP_MASK;

	if ((flags&TYPE_MASK) != 0) {
		fprintf(stderr, "%s:  AddGraceOp: Invalid flags -- %#x\n",
		    Program, flags);
		Err++;
		return;
	}
	if (ExpMask != 0 && n != 0) {
		fprintf(stderr,
		    "%s:  Interval incompatible with clear/reset/expire.\n",
		    Program);
		Err++;
		return;
	}
	if (CLRBIT(ExpMask) != 0) {
		fprintf(stderr,
		    "%s:  Only one of clear/reset/expire is permitted.\n",
		    Program);
		Err++;
		return;
	}
	if (wflag && !Ask("Setting Grace Timer:  continue? ", 'n')) {
		Err++;
		return;
	}

	opbase = ARG_EXPIRE;
	switch (ExpMask) {
	case 0:
		opbase |= OP_SET;
		n += Now;
		break;
	case EXP_CLEAR:
		opbase |= OP_SET;
		n = 0;
		break;
	case EXP_RESET:
		opbase |= OP_RESET;
		n = 0;
		break;
	case EXP_EXPIRE:
		opbase |= OP_SET;
		n = Now;
		break;
	default:
		fprintf(stderr, "%s:  Bad ExpMask %#x\n", Program, ExpMask);
		Err++;
		return;
	}

	for (ScopeBit = GETBIT(ScopeMask); ScopeBit != 0;
	    ScopeBit = GETBIT(ScopeMask)) {
		switch (ScopeBit) {
		case SCOPE_ONLINE:
		case SCOPE_TOTAL:
			AddPrimOp(opbase|ScopeBit, n);
			break;
		default:
			fprintf(stderr, "%s:  AddGraceOp:  Bad ScopeBit %#x\n",
			    Program, ScopeBit);
			Err++;
		}
		ScopeMask &= ~ScopeBit;
	}
}

void
AddPrimOp(int op, long long val)
{
	if (NQuotaOps >= MAX_QUOTA_OPS) {
		fprintf(stderr, "%s:  too many command arguments\n", Program);
		Err++;
		return;
	}
	OperationList[NQuotaOps].op = op;
	OperationList[NQuotaOps].value = val;
	NQuotaOps++;
}

/*
 * Logically 'AND' two structures together, replacing the first.
 */
void
BLogAnd(char *p1, char *p2, int n)
{
	for (; n--; p1++, p2++) {
		*p1 = *p1 & *p2;
	}
}

int
Ask(char *msg, char def)
{
	int i, n;
	int defret = (def == 'y' ? TRUE : FALSE);
	char answ[120];

	if (!isatty(0))
		return (TRUE);
	printf("%s", msg);
	fflush(stdout);
	n = read(0, answ, sizeof (answ));
	for (i = 0; i < n; i++) {
		if (answ[i] == ' ' || answ[i] == '\t')
			continue;
		if (tolower(answ[i]) == 'n')
			return (FALSE);
		if (tolower(answ[i]) == 'y')
			return (TRUE);
		return (defret);
	}
	return (defret);
}
