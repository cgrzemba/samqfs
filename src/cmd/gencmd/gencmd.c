/*
 * ---- gencmd - General file control command.
 *
 * gencmd contains the various file control commands.
 *
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

#pragma ident "$Revision: 1.57 $"

/* Feature test switches. */
	/* None. */

#define	MAIN

/* ANSI C headers. */
#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* POSIX headers. */
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <sys/shm.h>

/* OS headers. */
#include <libgen.h>
#include <sys/param.h>

/* SAM-FS headers. */
#include <sam/types.h>
#include <pub/lib.h>
#include <pub/stat.h>
#include <pub/sam_errno.h>
#include <sam/lib.h>
#include <sam/param.h>
#include <sam/syscall.h>
#include <sam/quota.h>
#include <sam/checksum.h>
#include <sam/nl_samfs.h>
#include <sam/custmsg.h>

/* Private functions. */
static void Archive(void);
static void ChgArch(void);
static void ExArch(void);
static void Release(void);
static void Stage(void);
static void Ssum(void);
static void Setfa(void);
static void Segment(void);
static int rearch(const char *path, const char *opns);
static int damage(const char *path, const char *opns);
static int unarchive(const char *path, const char *opns);
static int exarchive(const char *path, const char *opns);
static int undamage(const char *path, const char *opns);
static int unrearch(const char *path, const char *opns);
static int chkfile(void);
static void adddir(char *name);
static void dodir(char *name);
static int get_length(char *optarg, offset_t *length);
static void prerror(int quit, int prerrno, char *fmt, ...);

extern int sam_ssum(const char *name, const char *opns);

/* Private data. */
static struct sam_archive_copy_arg args;
static int which_dofile = 0;	/* 0/1 => dofile/dofile_no_options used */
static int (*dofile)(const char *name, const char *opns);
static int (*dofile_no_options)(const char *name);
static void (*init)(void);
static struct sam_stat sb;

static char cwd[MAXPATHLEN + 4];	/* Current full path name */
static char fullpath[MAXPATHLEN + 4];	/* Current full path name */
static char *base_name = fullpath;	/* Position in fullpath of base name */
static char *dir_names = NULL;		/* Unprocessed directory base names */
static char opns[SAM_MAX_OPS_LEN], *opn = opns;	/* File operations */
static int copy = -1;				/* Archive copy */
static int dn_size = 0;				/* Current size of dir_names */
static int exit_status = 0;			/* Exit status */

static struct {
	char *program_name;		/* program name as invoked */
	char *opts;				/* option string for getopts */
	void (*init)(void);		/* initialization function to call */
	/* action function to call */
	int (*dofile)(const char *path, const char *opns);
} cmd_table[] = {
	{ "archive",	"Cc:dfInrwW",	Archive,	sam_archive },
	{ "damage",	"ac:fm:Morv:",	ChgArch,	damage },
	{ "release",	"adfnprs:V",	Release,	sam_release },
	{ "ssum",	"defgGru",	Ssum,		sam_ssum },
	{ "stage",	"ac:dfnprVwx",	Stage,		sam_stage },
#if !defined(DEBUG)
	{ "unarchive",	"c:fm:Morv:",	ChgArch,	unarchive },
#else
	{ "unarchive",	"c:Ffm:Morv:",	ChgArch,	unarchive },
#endif
	{ "exarchive",	"c:fMr",	ExArch,		exarchive },
	{ "rearch",	"ac:fm:Morv:",	ChgArch,	rearch },
	{ "undamage",	"c:fm:Mrv:",	ChgArch,	undamage },
	{ "unrearch",	"c:fm:Mrv:",	ChgArch,	unrearch },
	{ "setfa",	"A:BDdfg:h:l:L:o:qrs:v:V",	Setfa,	sam_setfa },
	{ "segment",	"dfl:rs:V",		Segment,	sam_segment },
};
int n_cmd_table_entries = sizeof (cmd_table)/sizeof (cmd_table[0]);

/* Options. */
static int Default = FALSE;
static int quiet = FALSE;
static int recursive = FALSE;
static int a_opt = FALSE;
static int A_opt = FALSE;
static int B_opt = FALSE;
static int C_opt = FALSE;
static int D_opt = FALSE;
static int e_opt = FALSE;
static int g_opt = FALSE;
static int G_opt = FALSE;
static int h_opt = FALSE;
static int i_opt = FALSE;	/* not an external option */
static int I_opt = FALSE;
static int l_opt = FALSE;
static int L_opt = FALSE;
static int M_opt = FALSE;
static int n_opt = FALSE;
static int o_opt = FALSE;
static int p_opt = FALSE;
static int q_opt = FALSE;
static int s_opt = FALSE;
static int u_opt = FALSE;
static int v_opt = FALSE;
static int V_opt = FALSE;
static int w_opt = FALSE;
static int W_opt = FALSE;
static int x_opt = FALSE;


/* Static data. */
static int algo;
static int partial;
static int stripe_group;
static int stripe_width;
static offset_t length;
static offset_t allocahead;

char *program_name;

int
main(
	int argc,		/* Number of arguments */
	char *argv[])	/* Argument pointer list */
{
	extern int optind;
	char *opts = NULL;
	int c;
	int i;

	CustmsgInit(0, NULL);

	/*
	 * Determine action.
	 */
	program_name = basename(argv[0]);
	for (i = 0; i < n_cmd_table_entries; i++) {
		if (strcmp(program_name, cmd_table[i].program_name) == 0) {
			opts = cmd_table[i].opts;
			init = cmd_table[i].init;
			dofile = cmd_table[i].dofile;
			break;
		}
	}

	/*
	 * Program invoked with improper name - couldn't determine action.
	 */
	if (opts == NULL) {
		fprintf(stderr,
		    catgets(catfd, SET, 715,
		    "%s: must only be invoked as one of:\n"), argv[0]);
		fprintf(stderr, "     ");
		for (i = 0; i < n_cmd_table_entries; i++) {
			fprintf(stderr, "%s%s", cmd_table[i].program_name,
			    (i == n_cmd_table_entries-1) ? "\n" : ", ");
		}
		exit(2);
	}

	/*
	 * Process arguments.
	 */
	while ((c = getopt(argc, argv, opts)) != EOF) {
		switch (c) {
		case 'a':
			a_opt = TRUE;
			break;

		case 'A':
			A_opt = TRUE;
			allocahead = -1;
			if (!optarg || get_length(optarg, &allocahead) < 0) {
				prerror(0, 0, catgets(catfd, SET, 5105,
				    "Bad or missing -A argument"));
				exit_status++;
			}
			break;

		case 'B':
			B_opt = TRUE;
			break;

		case 'c':
			if (optarg) {
				copy = strtoll(optarg, NULL, 0) - 1;
				if (copy >= 0 && copy < MAX_ARCHIVE) {
					args.copies |= 1 << copy;
					args.ncopies++;
					if (args.ncopies == 2) {
						args.dcopy = copy;
					}
				} else {
					prerror(0, 0, catgets(catfd, SET, 729,
					    "copy must be: 1 <= c <= %d"),
					    MAX_ARCHIVE);
				}
			}
			break;

		case 'C':
			C_opt = TRUE;
			break;

		case 'd':
			Default = TRUE;
			break;

		case 'D':
			D_opt = TRUE;
			break;

		case 'e':
			if (getuid() != 0) {
				prerror(0, 0, catgets(catfd, SET, 5111,
				    "You must be root to set data"
				    " verification"));
			} else {
				e_opt = TRUE;
			}
			break;

		case 'f':
			quiet = TRUE;
			break;

		case 'F':
			args.flags |= SU_force;
			break;

		case 'g':
			g_opt = TRUE;
			if (optarg) {
				stripe_group = atoi(optarg);
			} else {
				stripe_group = 0;
			}
			break;

		case 'h':
			h_opt = TRUE;
			if (optarg) {
				stripe_width = atoi(optarg);
			} else {
				stripe_width = 0;
			}
			break;

		case 'G':
			G_opt = TRUE;
			break;

		case 'I':
			I_opt = TRUE;
			break;

		case 'l':
		case 'L':
			if (c == 'l') {
				l_opt = TRUE;
			} else {
				L_opt = TRUE;
			}
			if (l_opt && L_opt) {
				prerror(1, 0, catgets(catfd, SET, 5102,
				    "-l and -L are mutually exclusive."));
			}
			if (!optarg || get_length(optarg, &length) < 0) {
				prerror(0, 0, catgets(catfd, SET, 5106,
				    "Bad or missing -l or -L argument"));
				exit_status++;
			}
			break;

		case 'm':
			if ((args.media = sam_atomedia(optarg)) == 0) {
				prerror(0, 0, catgets(catfd, SET, 2753,
				    "unknown media"));
			}
			break;

		case 'M':
			M_opt = TRUE;
			break;

		case 'n':
			n_opt = TRUE;
			break;

		case 'o':
			if (dofile == sam_setfa) {
				o_opt = TRUE;
				if (optarg) {
					stripe_group = atoi(optarg);
				} else {
					stripe_group = 0;
				}
			} else {
				args.flags |= SU_online;
			}
			break;

		case 'p':
			p_opt = TRUE;
			break;

		case 'q':
			q_opt = TRUE;
			break;

		case 'r':
			recursive = TRUE;
			if (getcwd(cwd, sizeof (cwd)) == NULL) {
				prerror(1, 1, catgets(catfd, SET, 580,
				    "cannot determine cwd."));
			}
			break;

		case 's':
			s_opt = TRUE;
			if (optarg) {
				partial = atoi(optarg);
			} else {
				exit_status++;
			}
			break;

		case 'u':
			u_opt = TRUE;
			break;

		case 'v':
			if (dofile == sam_setfa) {
				if (!optarg ||
				    get_length(optarg, &length) < 0) {
					prerror(0, 0, catgets(catfd, SET, 5109,
					    "Bad or missing %c argument"),
					    'v');
					exit_status++;
				} else {
					v_opt = TRUE;
				}
			} else {
				strncpy(args.vsn, optarg, sizeof (args.vsn));
			}
			break;

		case 'V':
			V_opt = TRUE;
			break;

		case 'w':
			w_opt = TRUE;
			break;

		case 'W':
			W_opt = TRUE;
			break;

		case 'x':
			x_opt = TRUE;
			break;

		case '?':
		default:
			exit_status++;
		}
	}

	if (optind == argc) {
		exit_status++;	/* No file name */
	}
	if (exit_status != 0) {
		char optstr[128];
		char *optp;
		int optl, optn;

		optp = &optstr[0];
		optn = sizeof (optstr);
		optl = snprintf(optp, optn, "%s ", program_name);
		optn -= optl;
		optp += optl;
		while ((c = *opts++) != '\0') {
			if (*opts != ':') {
				optl = snprintf(optp, optn, " [-%c]", c);
			} else {
				optl = snprintf(optp, optn, " [-%c x]", c);
				opts++;
			}
			optn -= optl;
			optp += optl;
		}
		fprintf(stderr, catgets(catfd, SET, 13001, "Usage: %s %s\n"),
		    optstr, catgets(catfd, SET, 13031, "file ..."));
		exit(2);
	}

	/*
	 * Set up action.
	 */
	memset(opns, 0, sizeof (opns));
	(*init)();

	/*
	 * Process all file/directory names.
	 */
	while (optind < argc) {
		char *name = argv[optind++];
		int stat_err;

		strncpy(fullpath, name, sizeof (fullpath)-2);
		if ((stat_err = sam_lstat(fullpath, &sb, sizeof (sb))) < 0) {
			if (dofile == sam_setfa) {
				if ((open(fullpath, O_CREAT, 0666)) < 0) {
					prerror(1, errno,
					    catgets(catfd, SET, 574,
					    "cannot create %s"), fullpath);
				}
				stat_err =
				    sam_lstat(fullpath, &sb, sizeof (sb));
			}
		}
		if (stat_err < 0) {
			prerror(0, 1, catgets(catfd, SET, 2247,
			    "sam_stat error for %s"), fullpath);
			continue;
		}

		/*
		 * Individual file error checking
		 */
		if (!SS_ISSAMFS(sb.attr)) {
			prerror(0, 0, catgets(catfd, SET, 259,
			    "%s: Not a SAM-FS file."), fullpath);
			continue;
		}

		if (chkfile() != 0) {
			continue;
		}

		if ((which_dofile == 0) ? dofile(name, opns)
		    : dofile_no_options(name)) {
			prerror(0, 1, "%s: %s", name, opns);
			if (!S_ISDIR(sb.st_mode)) {
				continue;
			}
		}
		if (S_ISDIR(sb.st_mode) && recursive) {
			base_name = fullpath;
			dodir(name);
			if (chdir(cwd) < 0) {
				prerror(1, 1, catgets(catfd, SET, 3038,
				    "cannot chdir to %s"), cwd);
			}
		}
	}
	return (exit_status);
}


/*
 *	Archive.
 */
static void
Archive(void)
{
	if (args.ncopies != 0) {
		int		n;

		if (C_opt) {
			prerror(2, 0, catgets(catfd, SET, 13210,
			    "%s and %s are mutually exclusive"), "c", "C");
		}
		if (n_opt) {
			prerror(2, 0, catgets(catfd, SET, 13210,
			    "%s and %s are mutually exclusive"), "c", "n");
		}
		if (W_opt) {
			prerror(2, 0, catgets(catfd, SET, 13210,
			    "%s and %s are mutually exclusive"), "c", "W");
		}
		*opn++ = 'c';
		for (n = 0; n < MAX_ARCHIVE; n++) {
			if (args.copies & (1 << n)) {
				*opn++ = '1' + n;
			}
		}
	}
	if (Default)  *opn++ = 'd';
	if (C_opt) *opn++ = 'C';
	if (I_opt)  *opn++ = 'I';
	if (n_opt)  *opn++ = 'n';
	if (w_opt && W_opt) {
		prerror(2, 0, catgets(catfd, SET, 13210,
		    "%s and %s are mutually exclusive"), "w", "W");
	}
	if (w_opt || W_opt) {
		const char *wstr;

		wstr = (w_opt) ? "w" : "W";
		if (Default) {
			prerror(2, 0, catgets(catfd, SET, 13210,
			    "%s and %s are mutually exclusive"), wstr, "d");
		}
		if (n_opt) {
			prerror(2, 0, catgets(catfd, SET, 13210,
			    "%s and %s are mutually exclusive"), wstr, "n");
		}
		*opn++ = wstr[0];
	}
	if (!Default && !n_opt && !C_opt) {
		i_opt = TRUE;
		*opn++ = 'i';
	}
}


/*
 *	Release.
 */
static void
Release(void)
{
	if (Default)  *opn++ = 'd';
	if (a_opt)  *opn++ = 'a';
	if (n_opt) {
		if (a_opt) {
			prerror(2, 0, catgets(catfd, SET, 5108,
			    "cannot specify -n with -a."));
		}
		*opn++ = 'n';
	}
	if (p_opt)  *opn++ = 'p';
	if (s_opt) {
		char num[32];
		char *pnum = num;
		int n, i;

		sprintf(num, "s%d", partial);
		n = strlen(num);
		for (i = 0; i < n; i++) *opn++ = *pnum++;
	}
	if (!Default && !a_opt && !n_opt && !p_opt && !s_opt) {
		i_opt = TRUE;
		*opn++ = 'i';
	}
}


/*
 *	Stage.
 */
static void
Stage(void)
{
	if (w_opt && (a_opt || Default || n_opt)) {
		prerror(2, 0, catgets(catfd, SET, 641,
		    "cannot specify -w with -a, -d, or -n."));
	}
	if (n_opt && a_opt) {
		prerror(2, 0, catgets(catfd, SET, 643,
		    "-a, -n are mutually exclusive."));
	}
	if (Default)  *opn++ = 'd';
	if (a_opt)  *opn++ = 'a';
	if (n_opt)  *opn++ = 'n';
	if (p_opt)  *opn++ = 'p';
	if (w_opt)  *opn++ = 'w';
	if (x_opt)  {
		which_dofile = 1;
		dofile_no_options = sam_cancelstage;
	}
	if (copy >= 0) {
		if (args.copies & ~(1 << copy)) {
			prerror(2, 0, catgets(catfd, SET, 642,
			    "cannot specify more than one copy"));
		}
		*opn++ = '1' + copy;
	}
	if (!Default && !a_opt && !n_opt && !p_opt) {
		i_opt = TRUE;
		*opn++ = 'i';
	}
}


/*
 *	Damage, Unarchive, Rearch, Undamage, and Unrearch.
 */
static void
ChgArch(void)
{
	if (*args.vsn != '\0' && args.media == 0) {
		prerror(2, 0, catgets(catfd, SET, 314,
		    "-m must be specified if using -v"));
	}
	if (args.copies == 0 && args.media == 0) {
		prerror(2, 0, catgets(catfd, SET, 986,
		    "Either copy or VSN/media must be specified"));
	}
	if (args.copies == 0)  args.copies = (1 << MAX_ARCHIVE) - 1;
	if (a_opt)  args.flags |= SU_archive;
	if (M_opt)  args.flags |= SU_meta;
}


/*
 *	ExArch
 */
static void
ExArch(void)
{
	if (args.ncopies != 2) {
		prerror(2, 0, catgets(catfd, SET, 5021,
		    "Source copy and Destination copy must be specified"));
	}
	if (M_opt)  args.flags |= SU_meta;
	args.copies &= ~(1 << args.dcopy);	/* Remove destination copy */
}


/*
 *	Ssum.
 */
static void
Ssum(void)
{
	/* Check algorithm specified */
	if ((algo >= CS_FUNCS) && (algo < CS_USER)) {
		prerror(2, 0, catgets(catfd, SET, 1395,
		    "Invalid algorithm specified."));
	}

	if (Default)  *opn++ = 'd';
	if (g_opt)  *opn++ = 'g';
	if (u_opt)  *opn++ = 'u';
	if (G_opt)  *opn++ = 'G';
	if (e_opt) {
		if (!g_opt)	*opn++ = 'g';
		if (!u_opt)	*opn++ = 'u';
		*opn++ = 'e';
	}
	if ((g_opt || u_opt || e_opt) && !a_opt) {
		algo = CS_SIMPLE;
	}
	if (g_opt || u_opt || a_opt || e_opt) {
		sprintf(opn, "%d", algo);
	}
}


/*
 *	Setfa.
 */
static void
Setfa(void)
{
	char num[32];
	char *pnum = num;
	int n, i;

	if (Default)  *opn++ = 'd';
	if (B_opt)  *opn++ = 'B';
	if (D_opt)  *opn++ = 'D';
	if (q_opt)  *opn++ = 'q';
	if (g_opt) {
		sprintf(num, "g%d", stripe_group);
		n = strlen(num);
		for (i = 0; i < n; i++) {
			*opn++ = *pnum++;
		}
	}
	if (A_opt) {
		if (allocahead < SAM_ONE_MEGABYTE) {
			prerror(1, 0, catgets(catfd, SET, 5103,
			    "allocahead must be greater than 1 megabyte"));
		} else {
			sprintf(num, "A%lld", allocahead);
			pnum = num;
			n = strlen(num);
			for (i = 0; i < n; i++) {
				*opn++ = *pnum++;
			}
		}
	}
	if (l_opt) {
		sprintf(num, "l%lld", length);
		pnum = num;
		n = strlen(num);
		for (i = 0; i < n; i++) {
			*opn++ = *pnum++;
		}
	}
	if (L_opt) {
		sprintf(num, "L%lld", length);
		pnum = num;
		n = strlen(num);
		for (i = 0; i < n; i++) {
			*opn++ = *pnum++;
		}
	}
	if (s_opt) {
		sprintf(num, "s%d", partial);
		pnum = num;
		n = strlen(num);
		for (i = 0; i < n; i++) {
			*opn++ = *pnum++;
		}
	}
	if (o_opt) {
		sprintf(num, "o%d", stripe_group);
		n = strlen(num);
		for (i = 0; i < n; i++) {
			*opn++ = *pnum++;
		}
	}
	if (h_opt) {
		sprintf(num, "h%d", stripe_width);
		pnum = num;
		n = strlen(num);
		for (i = 0; i < n; i++) {
			*opn++ = *pnum++;
		}
	}
	if (v_opt) {
		sprintf(num, "v%lld", length);
		pnum = num;
		n = strlen(num);
		for (i = 0; i < n; i++) {
			*opn++ = *pnum++;
		}
	}
}


/*
 *	Segment.
 */
static void
Segment(void)
{
	char num[32];
	char *pnum = num;
	int n, i;

	if (Default)  *opn++ = 'd';
	if (l_opt) {
		if (length < SAM_ONE_MEGABYTE) {
			prerror(1, 0, catgets(catfd, SET, 5104,
			    "segment size must be greater than 1 megabyte"));
		}
		sprintf(num, "l%lld", length);
		n = strlen(num);
		for (i = 0; i < n; i++) *opn++ = *pnum++;
	}
	if (s_opt) {
		sprintf(num, "s%d", partial);
		pnum = num;
		n = strlen(num);
		for (i = 0; i < n; i++) *opn++ = *pnum++;
	}
}


/*
 *	Damage archive copy.
 */
/* ARGSUSED1 */
static int
damage(const char *path, const char *opns)
{
	if (a_opt)  args.flags |= SU_archive;
	args.operation = OP_damage;
	args.path.ptr = path;
	return (sam_syscall(SC_archive_copy, &args, sizeof (args)));
}


/*
 *	Rearchive archive copy.
 */
/* ARGSUSED1 */
static int
rearch(const char *path, const char *opns)
{
	args.operation = OP_rearch;
	args.path.ptr = path;
	return (sam_syscall(SC_archive_copy, &args, sizeof (args)));
}


/*
 *	Unarchive file.
 */
/* ARGSUSED1 */
static int
unarchive(const char *path, const char *opns)
{
	args.operation = OP_unarchive;
	args.path.ptr = path;
	return (sam_syscall(SC_archive_copy, &args, sizeof (args)));
}


/*
 *	Exarchive file.
 */
/* ARGSUSED1 */
static int
exarchive(const char *path, const char *opns)
{
	args.operation = OP_exarchive;
	args.path.ptr = path;
	return (sam_syscall(SC_archive_copy, &args, sizeof (args)));
}


/*
 *	Undamage/Unstale archive copy.
 */
/* ARGSUSED1 */
static int
undamage(const char *path, const char *opns)
{
	int copy, mask, stale = 0;

	for (copy = 0, mask = 1; copy < MAX_ARCHIVE; copy++, mask += mask) {
		if (args.copies & mask) {
			if (sb.copy[copy].flags & CF_STALE) {
				stale++;
			}
		}
	}
	args.operation = OP_undamage;
	args.path.ptr = path;
	return (sam_syscall(SC_archive_copy, &args, sizeof (args)));
}


/*
 *	Unrearchive archive copy.
 */
/* ARGSUSED1 */
static int
unrearch(const char *path, const char *opns)
{
	args.operation = OP_unrearch;
	args.path.ptr = path;
	return (sam_syscall(SC_archive_copy, &args, sizeof (args)));
}


/*
 *	Descend through the directories, starting at "name".
 *	Call dofile() for each directory entry.
 *	Save each directory name, and process at the end.
 */
static void
dodir(char *name)
{
	struct dirent *dirp;
	DIR *dp;
	size_t dn_mark, dn_next;
	char *prev_base;

	strcpy(base_name, name);

	/*
	 * Change to the new directory.
	 * Extend the full path.
	 */
	if ((dp = opendir(name)) == NULL) {
		prerror(0, 1, catgets(catfd, SET, 3039,
		    "cannot read directory %s"), fullpath);
		return;
	}
	if (chdir(name) == -1 && chdir(fullpath) == -1) {
		prerror(0, 1, catgets(catfd, SET, 3038,
		    "cannot chdir to %s"), fullpath);
		(void) closedir(dp);
		return;
	}
	prev_base = base_name;
	base_name += strlen(name);
	if (base_name[-1] != '/') {
		*base_name++ = '/';
	}
	*base_name = '\0';

	/*
	 * Mark the directory name stack.
	 */
	dn_mark = dn_next = dn_size;

	while ((dirp = readdir(dp)) != NULL) {
		/* ignore dot and dot-dot */
		if (strcmp(dirp->d_name, ".") == 0 ||
		    strcmp(dirp->d_name, "..") == 0) {
			continue;
		}

		strcpy(base_name, dirp->d_name);
		if (sam_lstat(dirp->d_name, &sb, sizeof (sb)) < 0) {
			prerror(0, 1, catgets(catfd, SET, 2247,
			    "sam_lstat error for %s"), fullpath);
			continue;
		}

		/*
		 * Individual file error checking.
		 */
		if (chkfile() != 0) {
			continue;
		}

		if (((which_dofile == 0) ? dofile(dirp->d_name, opns)
		    : dofile_no_options(dirp->d_name)) < 0) {
			prerror(0, 1, "%s", fullpath);
			if (!S_ISDIR(sb.st_mode)) {
				continue;
			}
		}
		if (S_ISDIR(sb.st_mode)) {
			adddir(dirp->d_name);
		}
	}

	if (closedir(dp) < 0) {
		prerror(0, 1, catgets(catfd, SET, 3040,
		    "cannot close directory %s"), fullpath);
	}

	/*
	 * Process all the directories found.
	 */
	while (dn_next < dn_size) {
		char *name;

		name = &dir_names[dn_next];
		dn_next += strlen(name) + 1;
		dodir(name);
	}
	dn_size = dn_mark;

	base_name = prev_base;
	*base_name = '\0';
	if (chdir("..") < 0 && chdir(fullpath) < 0) {
		prerror(0, 1, catgets(catfd, SET, 572,
		    "cannot chdir to \"..\" %s"), fullpath);
	}
}


/*
 *	int get_length(IN char *optarg, OUT offset_t *length)
 *
 * Convert a -A #, -l #, or -L # option to a byte count.
 *
 *	optarg is a pointer to the command line argument,
 *	*length is filled in with the option's value, if valid.
 *
 * suffixes k/m/g specify multipliers 2^10, 2^20, 2^30 resp.
 * If someone is really ambitious, suffixes t/p/e could
 * also be supported (tera - 2^40, peta - 2^50, exa - 2^60).
 *
 * Returns < 0 on error.
 */
static int
get_length(char *optarg, offset_t *length)
{
	char *p;
	offset_t value = 0, multiplier = 1;

	p = optarg;
	if (!isdigit(*p)) {
		return (-1);
	}
	while (*p != '\0' && isdigit(*p)) {
		value = (10 * value) + *p - '0';
		p++;
	}

	if (*p == 'k' || *p == 'K') {
		multiplier = 1024;
		p++;
	} else if (*p == 'm' || *p == 'M') {
		multiplier = 1024 * 1024;
		p++;
	} else if (*p == 'g' || *p == 'G') {
		multiplier = 1024 * 1024 * 1024;
		p++;
	}

	if (*p != '\0') {
		return (-1);
	}
	value *= multiplier;
	*length = value;
	return (0);
}


/*
 *	Add a directory name to table.
 */
static void
adddir(char *name)
{
	static int dn_limit = 0;
	size_t l;

	l = strlen(name) + 1;
	if (dn_size + l >= dn_limit) {
		/*
		 * No room in table for entry.  realloc() space for
		 * enlarged table.
		 */
		dn_limit += 5000;
		if ((dir_names = realloc(dir_names, dn_limit)) == NULL) {
			prerror(1, 0, catgets(catfd, SET, 628,
			    "Cannot realloc for directory names %d"),
			    dn_limit);
		}
	}
	memcpy(&dir_names[dn_size], name, l);
	dn_size += l;
}

/*
 *	Check operation for validity.
 */
static int
chkfile(void)
{
	if (init == ChgArch || init == ExArch) {
		if (M_opt) {
			if (!(S_ISDIR(sb.st_mode) || SS_ISREMEDIA(sb.attr) ||
			    SS_ISSEGMENT_F(sb.attr))) {
				/*
				 * not meta, cannot change archive
				 */
				return (1);
			}
		} else {
			if (S_ISDIR(sb.st_mode)) {
				if (recursive) {
					/*
					 * directory, keep going
					 */
					return (0);
				}
				prerror(1, 0, catgets(catfd, SET, 5107,
				    "%s is a directory but -M not specified"),
				    fullpath);
				/*
				 * directory, cannot change archive
				 */
				return (1);
			}
			if (SS_ISREMEDIA(sb.attr)) {
				/*
				 * other meta, cannot change archive
				 */
				return (1);
			}
		}
	} else if (dofile == sam_stage) {
		/*
		 * if immediate or partial
		 */
		if (i_opt == TRUE || p_opt == TRUE) {
			if (!S_ISDIR(sb.st_mode)) {
				if (!SS_ISSEGMENT_F(sb.attr)) {
					if (i_opt) {
						if (!SS_ISOFFLINE(sb.attr) ||
						    (SS_ISSTAGE_N(sb.attr) &&
						    !x_opt)) {
							return (1);
						}
					} else if (p_opt) {
						if (SS_ISPARTIAL(sb.attr)) {
							return (1);
						}
					}
				}
			} else if (!recursive) {
				/*
				 * directory, cannot be offline
				 */
				return (1);
			} else {
				/*
				 * directory, keep going
				 */
				return (0);
			}
			if (V_opt)
				printf(catgets(catfd, SET, 2398,
				    "Staging %s\n"), fullpath);
		}
	} else if (dofile == sam_release) {
		/*
		 * if immediate
		 */
		if (i_opt == TRUE) {
			if (S_ISDIR(sb.st_mode)) {
				if (!recursive) {
					/*
					 * directory, cannot release
					 */
					return (1);
				} else {
					/*
					 * directory, keep going
					 */
					return (0);
				}
			}
			if (SS_ISRELEASE_N(sb.attr))
				/*
				 * release never attribute set
				 */
				return (1);

			if (V_opt)
				printf(catgets(catfd, SET, 2085,
				    "Releasing %s\n"), fullpath);
		}
		if (S_ISLNK(sb.st_mode)) {
			/*
			 * symbolic link - skip file
			 */
			return (1);
		}
	} else if (dofile == sam_segment) {
		if (V_opt)
			printf(catgets(catfd, SET, 5100,
			    "Setting segment attributes for %s\n"), fullpath);
	} else if (dofile == sam_setfa) {
		if (V_opt)
			printf(catgets(catfd, SET, 5101,
			    "Setting file attributes for %s\n"), fullpath);
	}
	return (0);
}

/*
 *	Print error message.
 */
static void
prerror(int status, int prerrno, char *fmt, ...)
{
	char *msg;
	va_list ap;

	if (quiet) {
		if (status) {
			exit(status);
		}
		exit_status = EXIT_FAILURE;
		return;
	}
	fprintf(stderr, "%s: ", program_name);
	if (fmt != NULL) {
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	}
	if (prerrno) {
		int n = errno;
		int messagenum;

		if (SAM_ERRNO <= n && n < SAM_MAX_ERRNO) {
			messagenum = (n - SAM_ERRNO) + ERRNO_CATALOG;
			msg = catgets(catfd, SET, messagenum, " ");
			fprintf(stderr, ": %s", msg);
		} else if ((msg = strerror(n)) != NULL) {
			fprintf(stderr, ":  %s", msg);
		} else {
			fprintf(stderr, catgets(catfd, SET, 3041,
			    ": Unknown error %d"), n);
		}
	}
	fprintf(stderr, "\n");
	(void) fflush(stderr);
	if (status) {
		exit(status);
	}
	exit_status = EXIT_FAILURE;
}
