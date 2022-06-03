/*
 * samset.c
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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

#pragma ident "$Revision: 1.33 $"

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <thread.h>
#include <synch.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define	DEC_INIT

#include "sam/types.h"
#include "aml/shm.h"
#include "sam/param.h"
#include "aml/preview.h"
#include "aml/device.h"
#define	NEED_DL_NAMES
#include "aml/dev_log.h"
#undef NEED_DL_NAMES
#include "sam/defaults.h"
#include "aml/fifo.h"
#include "sam/lib.h"
#include "aml/proto.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

#define	DEV_ENT(a) ((dev_ent_t *)SHM_REF_ADDR(((dev_ptr_tbl_t *)SHM_REF_ADDR( \
((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table))->d_ent[(a)]))

/* function prototypes */
static void	attended(int, char **);
static void	alerts(int, char **);
static void	debug(int, char **);
static void	devlog(int, char **);
static void	export_med(int, char **);
static void	align_scsi_cmdbuf(int, char **);
static int	get_val(int, char **, char *);
static void	labels(int, char **);
static void	idle_unload(int, char **);
static void	shared_unload(int, char **);
static void	stale_time(int, char **);
static void	timeout(int, char **);
static void	remote_keepalive(int, char **);
static void	tapealert(int, char **);
static void	sef(int, char **);

static int	setlogflags(int, char **, int);
static void	set_tapealert_flags(dev_ent_t *, uchar_t);
static void	set_sef_flags(dev_ent_t *, uchar_t, int);
static void	show_debug(int, int, char *);
static void	show_export_media(void);
static void	show_device_info(void);
static void	sho_defaults(char *);
static void	ListDevlogEvents(int eq, char *msg, int flags);
static void	ListTapeAlertFlags(int eq, char *msg, uchar_t flags);
static void	ListSefFlags(int eq, char *msg, uchar_t flags, int interval);

static void	issue_fifo_cmd(sam_cmd_fifo_t *, int);

typedef enum setsam_type {
	setsam_media = 1,
	setsam_int
}		setsam_type_t;

typedef struct setsam_deflt {
	char	   *keyword;
	void	   *value;
	float	   factor;
	setsam_type_t   type;
	char	   *sec_word;
}		setsam_deflt_t;

typedef struct setsam_func {
	char	   *keyword;
	void	    (*func) (int, char **);
}		setsam_func_t;

static setsam_func_t functions[] =
{
	"attended", attended,
	"debug", debug,
	"devlog", devlog,
	"exported_media", export_med,
	"labels", labels,
	"idle_unload", idle_unload,
	"shared_unload", shared_unload,
	"stale_time", stale_time,
	"timeout", timeout,
	"remote_keepalive", remote_keepalive,
	"tapealert", tapealert,
	"sef", sef,
	"alerts", alerts,
	"align_scsi_cmdbuf", align_scsi_cmdbuf,
	NULL, NULL, 0
};

static int	is_root;
static shm_alloc_t master_shm;
static shm_ptr_tbl_t *shm_ptr_tbl;
static sam_defaults_t *defaults, tmp_dflt;
static dev_ptr_tbl_t *dev_tbl;
static char    *scanptr;	/* scratch for strtoul */

static setsam_deflt_t dflts[] =
{
	"idle_unload", &tmp_dflt.idle_unload, 1.0, setsam_int, "seconds",
	"shared_unload", &tmp_dflt.shared_unload, 1.0, setsam_int, "seconds",
	"stale_time", &tmp_dflt.stale_time, (1.0 / 60.0), setsam_int, "minutes",
	"timeout", &tmp_dflt.timeout, 1.0, setsam_int, "seconds",
	"remote_keepalive", &tmp_dflt.remote_keepalive, 1.0, setsam_int,
	    "seconds",
	NULL, NULL, 0, 0,
};

static char	pgm_name[MAXPATHLEN];

int
main(int argc, char **argv)
{
	int		shmid;
	setsam_func_t  *p_f;
	is_root = (getuid() == 0);
	strncpy(pgm_name, argv[0], sizeof (pgm_name));
	CustmsgInit(0, NULL);
	if ((shmid = shmget(SHM_MASTER_KEY, 0, 0464)) < 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 272, "%s: SAM_FS is not running.\n"),
		    pgm_name);
		exit(1);
	}
	if ((master_shm.shared_memory = shmat(shmid, (void *) NULL, 0464)) ==
	    (void *) -1) {
		fprintf(stderr,
		    catgets(catfd, SET, 211,
		    "%s: Cannot attach shared memory segment: %s\n"),
		    pgm_name, strerror(errno));
		exit(1);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	defaults = GetDefaultsRw();
	dev_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(shm_ptr_tbl->dev_table);
	if (defaults == NULL || dev_tbl == NULL) {
		fprintf(stderr,
		    catgets(catfd, SET, 2966,
		    "Wrong shared memory segment.\n"));
		exit(2);
	}
	memcpy(&tmp_dflt, defaults, sizeof (sam_defaults_t));
	if (argc == 1) {
		sho_defaults(NULL);
		exit(0);
	}
	argc--;
	argv++;
	for (p_f = functions; p_f->keyword != NULL; p_f++) {
		if (strcmp(*argv, p_f->keyword) == 0) {
			argc--;
			argv++;
			p_f->func(argc, argv);
			exit(0);
		}
	}

	fprintf(stderr,
	    catgets(catfd, SET, 282, "%s: unknown option\n"), pgm_name);
	return (1);
}

static void
debug(int argc, char **argv)
{
	char	   *msg;
	int		disp_help = FALSE;
	sam_debug_t	def, current, new;
	sam_dbg_strings_t *this_one;

	def = defaults->debug;
	current = shm_ptr_tbl->debug;
	new = current;

	if (argc == 0) {
		msg = catgets(catfd, SET, 813, "default: ");
		show_debug(def, 0, msg);

		msg = catgets(catfd, SET, 788, "current: ");
		show_debug(current, 0, msg);


		if (current != def) {
			uint_t	  differ = current ^ def;

			msg = catgets(catfd, SET, 901, "differ: ");
			show_debug(differ, current, msg);
		}
		exit(0);
	}
	while (argc > 0) {
		int		what = FALSE;
		char	   *start = *argv;
		if (**argv == '-') {
			what = TRUE;
			start++;
		}
		if (**argv == '+')
			start++;

		if (strcasecmp(start, "all") == 0) {
			if (what) {
				new = current & ~SAM_DBG_ALL;
			} else {
				new = current | SAM_DBG_ALL;
			}
		} else if (strcasecmp(start, "none") == 0) {
			new = SAM_DBG_NONE;
		} else if (strcasecmp(start, "default") == 0) {
			if (what) {
				new = current & ~def;
			} else {
				new = def;
			}
		} else {
			for (this_one = sam_dbg_strings;
			    this_one->string != NULL; this_one++) {
				if (this_one->bits == SAM_DBG_ALL ||
				    this_one->bits == SAM_DBG_NONE)
					continue;

				if (strcasecmp(this_one->string, start) == 0) {
					if (this_one->who == SAM_DBG_ROOT_SET &&
					    !is_root && !what) {
						printf(catgets(catfd, SET, 102,
						    "%s may be set only by"
						    " super-user.\n"),
						    start);
						break;
					}
					if (what)
						new &= ~this_one->bits;
					else
						new |= this_one->bits;
					break;
				}
			}
			if (this_one->string == NULL) {
				printf(catgets(catfd, SET, 2743,
				    "unknown debug option %s\n"),
				    start);
				disp_help = TRUE;
			}
		}
		argc--;
		argv++;
	}

	if (disp_help) {
		int		outlen = 9;
		printf(catgets(catfd, SET, 1885, "options: "));
		for (this_one = sam_dbg_strings; this_one->string != NULL;
		    this_one++) {
			if (this_one->bits == SAM_DBG_ALL ||
			    this_one->bits == SAM_DBG_NONE ||
			    ((this_one->who == SAM_DBG_ROOT_SET) && !is_root))
				continue;

			printf("%s ", this_one->string);
			outlen += (strlen(this_one->string) + 1);
			if (outlen > 70) {
				printf("\n\t");
				outlen = 8;
			}
		}
		if (outlen != 8)
			printf("\n");
		printf("\n");
		printf(catgets(catfd, SET, 1042,
		    "error in options, no options changed.\n"));
		exit(1);
	}
	shm_ptr_tbl->debug = new;
	exit(0);
}

static void
devlog(int argc, char **argv)
{
	uint_t	  ef, el, eq;
	dev_ent_t	*un;

	if (argc < 1) {
		fprintf(stderr, catgets(catfd, SET, 13001,
		    "Usage: %s %s\n"),
		    pgm_name, "devlog eq [events...]");
		exit(2);
	}
	if (strcmp(*argv, "all") == 0) {
		ef = 1;
		el = dev_tbl->max_devices;
		if (argc == 1)
			ListDevlogEvents(-1, "default", DL_def_events);
		for (eq = ef; eq <= el; eq++) {
			if ((un = DEV_ENT(eq)) == NULL)
				continue;
			if (setlogflags(argc, argv, eq)) {
				ListDevlogEvents(eq, "set to", un->log.flags);
			}
		}
	} else {
		eq = (uint_t)strtoul(*argv, &scanptr, 10);
		if (scanptr == *argv || *scanptr != '\0') {
			fprintf(stderr, catgets(catfd, SET, 2350,
			"%s is not a valid equipment ordinal.\n"), *argv);
			exit(2);
		}
		if (eq > dev_tbl->max_devices) {
			fprintf(stderr,
			    catgets(catfd, SET, 13024,
			    "%s: %s: equipment %s: out of range.\n"),
			    pgm_name, "devlog", *argv);
			exit(2);
		} else if ((un = DEV_ENT(eq)) == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 13025,
			    "%s: %s: equipment %s: not present.\n"),
			    pgm_name, "devlog", *argv);
			exit(2);
		}
		if (argc == 1)
			ListDevlogEvents(eq, "default", DL_def_events);
		if (setlogflags(argc, argv, eq)) {
			ListDevlogEvents(eq, "set to", un->log.flags);
		}
	}
}

static int
setlogflags(int argc, char **argv, int eq)
{
	int		an;
	int		flags;
	dev_ent_t	*un = DEV_ENT(eq);

	flags = un->log.flags;
	if (argc == 1) {
		ListDevlogEvents(eq, "current", flags);
		return (0);
	}
	for (an = 1; an < argc; an++) {
		enum DL_event   to;
		char	   *name;
		int		not;

		name = argv[an];
		if (*name != '-')
			not = FALSE;
		else {
			name++;
			not = TRUE;
		}

		for (to = 0; strcmp(name, DL_names[to]) != 0; to++) {
			if (to >= DL_MAX) {
				fprintf(stderr,
				    catgets(catfd, SET, 13026,
				    "%s: Unknown devlog event: %s\n"),
				    pgm_name, name);
				exit(2);
			}
		}
		if (DL_none == to) {
			flags = 0;
		} else if (not) {
			if (DL_all == to)
				flags &= ~DL_all_events;
			else if (DL_default == to)
				flags &= ~DL_def_events;
			else
				flags &= ~(1 << to);
		} else {
			if (DL_all == to)
				flags = DL_all_events;
			else if (DL_default == to)
				flags = DL_def_events;
			else
				flags |= 1 << to;
		}
	}
	un->log.flags = flags;
	return (1);
}

static void
show_debug(int which, int current, char *which_msg)
{
	int		outlen;
	sam_dbg_strings_t *this_one;

	if (which != 0) {
		outlen = 9;
		printf("%s", which_msg);
		for (this_one = sam_dbg_strings; this_one->string != NULL;
		    this_one++) {
			if (this_one->bits == SAM_DBG_ALL ||
			    this_one->bits == SAM_DBG_NONE ||
			    ((this_one->who == SAM_DBG_ROOT_SET) && !is_root))
				continue;

			if (current) {
				if (which & this_one->bits) {
					if (current & this_one->bits)
						printf("+%s ",
						    this_one->string);
					else
						printf("-%s ",
						    this_one->string);
					outlen +=
					    (strlen(this_one->string) + 2);
					if (outlen > 70) {
						printf("\n\t");
						outlen = 8;
					}
				}
			} else {
				if (which & this_one->bits) {
					printf("%s ", this_one->string);
					outlen +=
					    (strlen(this_one->string) + 1);
					if (outlen > 70) {
						printf("\n\t");
						outlen = 8;
					}
				}
			}
		}
		if (outlen != 8)
			printf("\n");
		printf("\n");
	}
}

static void
show_export_media(void)
{
	dev_ent_t	*device;

	device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	for (; device != NULL;
	    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
		if (IS_ROBOT(device) && device->type != DT_HISTORIAN) {
			printf(catgets(catfd, SET, 855,
			    "Device %d: exported_media is %s\n"),
			    device->eq,
			    device->dt.rb.status.b.export_unavail ?
			    catgets(catfd, SET, 2728, "unavailable") :
			    catgets(catfd, SET, 492, "available"));
		}
	}
}

static void
show_device_info(void)
{
	dev_ent_t	*device;

	device = (dev_ent_t *)SHM_REF_ADDR(
	    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);

	for (; device != NULL;
	    device = (dev_ent_t *)SHM_REF_ADDR(device->next)) {
		if ((IS_ROBOT(device) || IS_TAPE(device)) &&
		    device->type != DT_HISTORIAN) {
			printf("%s %d: ", catgets(catfd, SET, 13257, "device"),
			    device->eq);
			printf("%s ", catgets(catfd, SET, 13255, "tapealert"));
			if (device->tapealert & TAPEALERT_ENABLED) {
				printf("%s ",
				    catgets(catfd, SET, 13253, "on"));
				if (device->tapealert & TAPEALERT_SUPPORTED) {
					printf("(%s), ",
					    catgets(catfd, SET, 13251,
					    "supported"));
				} else {
					printf("(%s), ",
					    catgets(catfd, SET, 13252,
					    "unsupported"));
				}
			} else {
				printf("%s, ",
				    catgets(catfd, SET, 13254, "off"));
			}
			printf("%s ", catgets(catfd, SET, 13034, "sef"));
			if (IS_TAPE(device)) {
				if (device->sef_sample.state & SEF_ENABLED) {
					printf("%s ",
					    catgets(catfd, SET, 13253, "on"));
					if (device->sef_sample.interval ==
					    SEF_INTERVAL_ONCE) {
						printf("%s ",
						    catgets(catfd, SET,
						    13633, "once"));
					} else {
						printf("%ds ",
						    device->
						    sef_sample.interval);
					}
					if (device->sef_sample.state &
					    SEF_SUPPORTED) {
						printf("(%s)",
						    catgets(catfd, SET,
						    13251, "supported"));
					} else {
						printf("(%s)",
						    catgets(catfd, SET,
						    13252, "unsupported"));
					}
				} else {
					printf("%s", catgets(catfd, SET,
					    13254, "off"));
				}
			} else {
				printf("%s", catgets(catfd, SET, 13256,
				    "not applicable"));
			}
			printf("\n");
		}
	}
}

static void
sho_defaults(char *what)
{
	setsam_deflt_t *d_p = dflts;
	char	   *msg;

	for (; d_p->keyword != NULL; d_p++) {
		if (what != NULL && strcmp(what, d_p->keyword))
			continue;

		switch (d_p->type) {
		case setsam_media:
			printf("%s\n", sam_mediatoa(*(media_t *)d_p->value));
			break;

		case setsam_int:
			{
				int		value = *(int *)d_p->value;

				if (d_p->factor != 1.0)
					value =
					    (int)((float)value * d_p->factor);
				printf("%s %d %s\n", d_p->keyword, value,
				    d_p->sec_word == NULL ? "" : d_p->sec_word);
				break;
			}
		}
	}

	if (what != NULL)
		return;

	printf(catgets(catfd, SET, 101, "%s mode\n"),
	    (tmp_dflt.flags & DF_ATTENDED) ?
	    catgets(catfd, SET, 468, "attended") :
	    catgets(catfd, SET, 2726, "unattended"));
	printf(catgets(catfd, SET, 31003, "alerts are %s.\n"),
	    (tmp_dflt.flags & DF_ALERTS) ?
	    catgets(catfd, SET, 31004, "enabled") :
	    catgets(catfd, SET, 31005, "disabled"));
	if (tmp_dflt.flags & DF_ALIGN_SCSI_CMDBUF) {
		printf(catgets(catfd, SET, 101, "scsi_cmd %s\n"),
		    catgets(catfd, SET, 12022, "align_scsi_cmdbuf"));
	}

	show_export_media();
	printf(catgets(catfd, SET, 1489, "labels are %s"),
	    (tmp_dflt.flags & DF_LABEL_BARCODE) ?
	    catgets(catfd, SET, 519, "barcodes") :
	    catgets(catfd, SET, 2004, "read"));
	if (tmp_dflt.flags & DF_LABEL_BARCODE)
		printf(catgets(catfd, SET, 395, " and barcodes are %s order"),
		    (tmp_dflt.flags & DF_BARCODE_LOW) ?
		    catgets(catfd, SET, 1589, "low") :
		    catgets(catfd, SET, 1295, "high"));
	printf("\n");

	show_device_info();

	msg = catgets(catfd, SET, 789, "debug: ");
	show_debug(shm_ptr_tbl->debug, 0, msg);
}

static void
export_med(int argc, char **argv)
{
	int		unavail = FALSE, show = FALSE;
	uint_t	  eq;
	dev_ent_t	*device;

	if (argc < 1) {
		show_export_media();
		exit(0);
	}
	if (**argv == '-' || **argv == '+') {
		if (strcmp(*argv, "+u") == 0)
			unavail = TRUE;
		else if (strcmp(*argv, "-u") == 0)
			unavail = FALSE;
		else {
			fprintf(stderr, catgets(catfd, SET, 13001,
			    "Usage: %s %s\n"),
			    pgm_name, "exported_media [+u|-u] eq [eq2...]");
			exit(1);
		}
		argc--;
		argv++;
	} else
		show = TRUE;

	while (argc-- > 0) {
		eq = (uint_t)strtoul(*argv, &scanptr, 10);
		if (scanptr == *argv || *scanptr != '\0') {
			fprintf(stderr, catgets(catfd, SET, 2350,
			"%s is not a valid equipment ordinal.\n"), *argv);
			exit(2);
		}
		if (eq > dev_tbl->max_devices)
			fprintf(stderr,
			    catgets(catfd, SET, 13024,
			    "%s: %s: equipment %s: out of range.\n"),
			    pgm_name, "exported_media", *argv);
		else if ((device = DEV_ENT(eq)) == NULL)
			fprintf(stderr,
			    catgets(catfd, SET, 13025,
			    "%s: %s: equipment %s: not present.\n"),
			    pgm_name, "exported_media", *argv);
		else if (IS_ROBOT(device)) {
			sigset_t	block_all;

			if (show) {
				printf(catgets(catfd, SET, 855,
				    "Device %d: exported_media is %s\n"),
				    device->eq,
				    device->dt.rb.status.b.export_unavail ?
				    catgets(catfd, SET, 2728, "unavailable") :
				    catgets(catfd, SET, 492, "available"));
			} else {
				sigfillset(&block_all);
				sigprocmask(SIG_SETMASK, &block_all, NULL);
				mutex_lock(&device->mutex);
				device->dt.rb.status.b.export_unavail = unavail;
				mutex_unlock(&device->mutex);
			}
		}
		argv++;
	}
	exit(0);
}

static void
stale_time(int argc, char **argv)
{
	int		time;

	time = get_val(argc, argv, "stale_time");
	if (time < 0) {
		fprintf(stderr, catgets(catfd, SET, 2348,
		"%s is not a valid value for %s.\n"), *argv, "stale_time");
		exit(-2);
	}
	defaults->stale_time = time * 60;
	exit(0);
}

static void
timeout(int argc, char **argv)
{
	int		i;

	i = get_val(argc, argv, "timeout");
	if (i != 0 && i < 600) {
		fprintf(stderr, catgets(catfd, SET, 2348,
		    "%s is not a valid value for %s.\n"), *argv, "timeout");
		exit(-2);
	}
	defaults->timeout = i;
	exit(0);
}

static void
remote_keepalive(int argc, char **argv)
{
	int	i;

	i = get_val(argc, argv, "remote_keepalive");
	if (i < 0) {
		fprintf(stderr, catgets(catfd, SET, 2348,
		    "%s is not a valid value for %s.\n"),
		    *argv, "remote_keepalive");
		exit(-2);
	}
	defaults->remote_keepalive = i;
	exit(0);
}

static void
tapealert(int argc, char **argv)
{
	int32_t	 ef, el, eq;
	dev_ent_t	*un;
	int		i;
	uchar_t	 flags = 0;

	if (argc < 1) {
		fprintf(stderr, catgets(catfd, SET, 13001,
		    "Usage: %s %s\n"),
		    pgm_name, "tapealert eq_ord on|off");
		exit(2);
	}
	if (strcmp(argv[0], "all") == 0) {
		eq = -1;
	} else {
		eq = (uint_t)strtoul(argv[0], &scanptr, 10);
		if (scanptr == argv[0] || *scanptr != '\0') {
			fprintf(stderr, catgets(catfd, SET, 2350,
			"%s is not a valid equipment ordinal.\n"), argv[0]);
			exit(2);
		}
		if (eq > dev_tbl->max_devices) {
			fprintf(stderr,
			    catgets(catfd, SET, 13024,
			    "%s: %s: equipment %s: out of range.\n"),
			    pgm_name, "devlog", argv[0]);
			exit(2);
		} else if ((un = DEV_ENT(eq)) == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 13025,
			    "%s: %s: equipment %s: not present.\n"),
			    pgm_name, "devlog", argv[0]);
			exit(2);
		} else if (IS_HISTORIAN(un) || !(IS_TAPE(un) || IS_ROBOT(un))) {
			fprintf(stderr,
			    catgets(catfd, SET, 13249,
			"%s: equipment %d not tape or media changer.\n"),
			    pgm_name, eq);
			exit(2);
		}
	}

	if (argc == 1) {
		if (eq == -1) {
			ef = 1;
			el = dev_tbl->max_devices;

			for (eq = ef; eq <= el; eq++) {
				if ((un = DEV_ENT(eq)) == NULL)
					continue;
				if (!IS_HISTORIAN(un) && (IS_TAPE(un) ||
				    IS_ROBOT(un))) {
					ListTapeAlertFlags(eq,
					    catgets(catfd, SET, 13259,
					    "current"),
					    un->tapealert);
				}
			}
		} else {
			ListTapeAlertFlags(eq,
			    catgets(catfd, SET, 13259, "current"),
			    un->tapealert);
		}
		exit(0);
	}
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "on") == 0) {
			flags = TAPEALERT_ENABLED;
		} else if (strcmp(argv[i], "off") == 0) {
			flags &= ~TAPEALERT_ENABLED;
		} else if (strcmp(argv[i], "default") == 0) {
			flags = TAPEALERT_ENABLED;
		} else {
			fprintf(stderr, catgets(catfd, SET, 13246,
			    "%s: Unknown tapealert flag: %s\n"),
			    pgm_name, argv[i]);
			exit(2);
		}
	}

	if (eq == -1) {
		ef = 1;
		el = dev_tbl->max_devices;

		for (eq = ef; eq <= el; eq++) {
			if ((un = DEV_ENT(eq)) == NULL)
				continue;
			if (!IS_HISTORIAN(un) && (IS_TAPE(un) ||
			    IS_ROBOT(un))) {
				set_tapealert_flags(un, flags);
			}
		}
	} else if ((un = DEV_ENT(eq)) == NULL) {
		fprintf(stderr,
		    catgets(catfd, SET, 13025,
		    "%s: %s: equipment %s: not present.\n"),
		    pgm_name, "devlog", argv[0]);
		exit(2);
	} else {
		set_tapealert_flags(un, flags);
	}
}

static void
set_tapealert_flags(
		    dev_ent_t *un,
		    uchar_t flags)
{
	sam_cmd_fifo_t  cmd_block;

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq = un->eq;
	cmd_block.flags = flags;
	issue_fifo_cmd(&cmd_block, CMD_FIFO_TAPEALERT);
}

static void
sef(int argc, char **argv)
{
	int32_t	 ef, el, eq;
	dev_ent_t	*un;
	int		i;
	uchar_t	 flags = 0;
	boolean_t	valid = B_FALSE;
	int		interval;
	char	   *endptr;

	if (argc < 1) {
		fprintf(stderr, catgets(catfd, SET, 13001,
		    "Usage: %s %s\n"),
		    pgm_name, "sef eq_ord on|off interval");
		exit(2);
	}
	if (strcmp(argv[0], "all") == 0) {
		eq = -1;
	} else {
		eq = (uint_t)strtoul(argv[0], &scanptr, 10);
		if (scanptr == argv[0] || *scanptr != '\0') {
			fprintf(stderr, catgets(catfd, SET, 2350,
			"%s is not a valid equipment ordinal.\n"), argv[0]);
			exit(2);
		}
		if (eq > dev_tbl->max_devices) {
			fprintf(stderr,
			    catgets(catfd, SET, 13024,
			    "%s: %s: equipment %s: out of range.\n"),
			    pgm_name, "devlog", argv[0]);
			exit(2);
		} else if ((un = DEV_ENT(eq)) == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 13025,
			    "%s: %s: equipment %s: not present.\n"),
			    pgm_name, "devlog", argv[0]);
			exit(2);
		} else if (IS_HISTORIAN(un) || !IS_TAPE(un)) {
			fprintf(stderr,
			    catgets(catfd, SET, 13035,
			    "%s: equipment %d not tape.\n"),
			    pgm_name, eq);
			exit(2);
		}
	}

	if (argc == 1) {
		if (eq == -1) {
			ef = 1;
			el = dev_tbl->max_devices;

			for (eq = ef; eq <= el; eq++) {
				if ((un = DEV_ENT(eq)) == NULL)
					continue;

				if (!IS_HISTORIAN(un) && IS_TAPE(un)) {
					ListSefFlags(eq,
					    catgets(catfd, SET, 13259,
					    "current"),
					    un->sef_sample.state,
					    un->sef_sample.interval);
				}
			}
		} else {
			ListSefFlags(eq, catgets(catfd, SET, 13259, "current"),
			    un->sef_sample.state, un->sef_sample.interval);
		}
		exit(0);
	}
	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "on") == 0) {
			flags = SEF_ENABLED;
		} else if (strcmp(argv[i], "off") == 0) {
			flags &= ~SEF_ENABLED;
		} else if (strcmp(argv[i], "default") == 0) {
			flags = SEF_ENABLED;
			interval = SEF_INTERVAL_DEFAULT;
			valid = B_TRUE;
		} else {
			if (flags != SEF_ENABLED) {
				fprintf(stderr,
				    catgets(catfd, SET, 13634,
				    "%s: on required before interval\n"),
				    pgm_name);
				exit(2);
			} else if (strcmp(argv[i], "once") == 0) {
				interval = SEF_INTERVAL_ONCE;
			} else if (((interval =
			    strtol(argv[i], &endptr, 0)) == 0 &&
			    errno == EINVAL) ||
			    ((interval == LONG_MAX || interval == LONG_MIN) &&
			    errno == ERANGE) ||
			    *endptr != '\0' || interval < 300) {
				fprintf(stderr, catgets(catfd, SET, 13250,
				    "%s: Invalid sef interval: %s\n"),
				    pgm_name, argv[i]);
				exit(2);
			}
			valid = B_TRUE;
		}
	}

	if (eq == -1) {
		ef = 1;
		el = dev_tbl->max_devices;

		for (eq = ef; eq <= el; eq++) {
			if ((un = DEV_ENT(eq)) == NULL)
				continue;
			if (!IS_HISTORIAN(un) && IS_TAPE(un)) {
				set_sef_flags(un, flags,
				    (valid == B_TRUE ? interval :
				    un->sef_sample.interval));
			}
		}
	} else if ((un = DEV_ENT(eq)) == NULL) {
		fprintf(stderr,
		    catgets(catfd, SET, 13025,
		    "%s: %s: equipment %s: not present.\n"),
		    pgm_name, "devlog", argv[0]);
		exit(2);
	} else {
		set_sef_flags(un, flags,
		    (valid == B_TRUE ? interval : un->sef_sample.interval));
	}
}

static void
set_sef_flags(
		dev_ent_t *un,
		uchar_t flags,
		int interval)
{
	sam_cmd_fifo_t  cmd_block;

	memset(&cmd_block, 0, sizeof (cmd_block));
	cmd_block.eq = un->eq;
	cmd_block.flags = flags;
	cmd_block.value = interval;
	issue_fifo_cmd(&cmd_block, CMD_FIFO_SEF);
}

static int
get_val(int argc, char **argv, char *name)
{
	if (argc == 0) {
		sho_defaults(name);
		exit(0);
	} else {
		int		i;

		if (argc != 1) {
			fprintf(stderr, catgets(catfd, SET, 13017,
			    "Usage: %s %s nnn\n"), pgm_name, name);
			return (-1);
		}
		i = (int)strtoul(*argv, &scanptr, 10);
		if (scanptr == *argv || *scanptr != '\0') {
/* couldn't convert the input string to a base 10 number */
			fprintf(stderr, catgets(catfd, SET, 2348,
			"%s is not a valid value for %s.\n"), *argv, name);
			exit(-2);
		}
		return (i);
	}
/*NOTREACHED*/
}

static void
idle_unload(int argc, char **argv)
{
	int		i;

	i = get_val(argc, argv, "idle_unload");
	if (i < 0) {
		fprintf(stderr, catgets(catfd, SET, 2348,
		"%s is not a valid value for %s.\n"), *argv, "idle_unload");
		exit(-2);
	}
	defaults->idle_unload = i;
	exit(0);
}

static void
shared_unload(int argc, char **argv)
{
	int		i;

	i = get_val(argc, argv, "shared_unload");
	if (i < 0) {
		fprintf(stderr, catgets(catfd, SET, 2348,
		    "%s is not a valid value for %s.\n"),
		    *argv, "shared_unload");
		exit(-2);
	}
	defaults->shared_unload = i;
	exit(0);
}

static void
attended(int argc, char **argv)
{
	if (argc == 0)
		printf(catgets(catfd, SET, 2459,
		    "system is running in %s mode.\n"),
		    (defaults->flags & DF_ATTENDED) ?
		    catgets(catfd, SET, 468, "attended") :
		    catgets(catfd, SET, 2726, "unattended"));
	else {
		if (argc != 1) {
			fprintf(stderr, catgets(catfd, SET, 13001,
			"Usage: %s %s\n"), pgm_name, "attended [yes|no]");
			exit(1);
		}
		if (strcmp(*argv, "no") == 0)
			defaults->flags &= ~DF_ATTENDED;
		else if (strcmp(*argv, "yes") == 0)
			defaults->flags |= DF_ATTENDED;
		else {
			fprintf(stderr, catgets(catfd, SET, 13001,
			"Usage: %s %s\n"), pgm_name, "attended [yes|no]");
			exit(1);
		}
	}
	exit(0);
}

static void
alerts(int argc, char **argv)
{
	if (argc == 0)
		printf(catgets(catfd, SET, 31003, "alerts are %s.\n"),
		    (defaults->flags & DF_ALERTS) ?
		    catgets(catfd, SET, 31004, "enabled") :
		    catgets(catfd, SET, 31005, "disabled"));

	else {
		if (argc != 1) {
			fprintf(stderr, catgets(catfd, SET, 31006,
			    "Usage: %s %s\n"), pgm_name, "alerts [on|off]");
			exit(1);
		}
		if (strcmp(*argv, "off") == 0)
			defaults->flags &= ~DF_ALERTS;
		else if (strcmp(*argv, "on") == 0)
			defaults->flags |= DF_ALERTS;
		else {
			fprintf(stderr, catgets(catfd, SET, 31006,
			    "Usage: %s %s\n"), pgm_name, "alerts [on|off]");
			exit(1);
		}
	}
	exit(0);
}

static void
labels(int argc, char **argv)
{
	if (argc == 0)
		printf(catgets(catfd, SET, 2460,
		    "system is running with labels = %s\n"),
		    (defaults->flags & DF_LABEL_BARCODE) ?
		    (defaults->flags & DF_BARCODE_LOW) ?
		    catgets(catfd, SET, 521, "barcodes_low") :
		    catgets(catfd, SET, 519, "barcodes") :
		    catgets(catfd, SET, 2004, "read"));
	else {
		if (argc != 1) {
			fprintf(stderr, catgets(catfd, SET, 13001,
			    "Usage: %s %s\n"),
			    pgm_name,
			    "labels { barcodes | barcodes_low | read }");
			exit(1);
		}
		if ((strcmp(*argv, "barcodes") == 0) ||
		    (strcmp(*argv, "barcode") == 0)) {
			defaults->flags &= ~DF_BARCODE_LOW;
			defaults->flags |= DF_LABEL_BARCODE;
		} else if (strcmp(*argv, "read") == 0)
			defaults->flags &= ~(DF_LABEL_BARCODE | DF_BARCODE_LOW);
		else if (strcmp(*argv, "barcodes_low") == 0 ||
		    strcmp(*argv, "barcode_low") == 0)
			defaults->flags |= DF_LABEL_BARCODE | DF_BARCODE_LOW;
		else {
			fprintf(stderr, catgets(catfd, SET, 13001,
			    "Usage: %s %s\n"),
			    pgm_name,
			    "labels { barcodes | barcodes_low | read }");
			exit(1);
		}
	}
	exit(0);
}


/*
 * List the devlog events set in flags.
 */
static void
ListDevlogEvents(
		int eq,		/* equipment ordinal */
		char *msg,	/* message about events */
		int flags)
{				/* Devlog flags */
	int	first = TRUE;
	int	n;

	if (eq == -1) {
		printf("       %s: ", msg);
	} else {
		printf("eq %d: %s: ", eq, msg);
	}
	for (n = 0; n < DL_MAX; n++) {
		if (flags & 1 << n) {
			if (first)
				first = FALSE;
			else
				printf(",");
			printf(" %s", DL_names[n]);
		}
	}
	if (first)
		printf("NONE");
	printf("\n");
}

/*
 * List the TapeAlert flags.
 */
static void
ListTapeAlertFlags(
		int eq,		/* equipment ordinal */
		char *msg,	/* message about events */
		uchar_t flags)
{				/* TapeAlert flags */
	printf("eq %d %s: ", eq, msg);

	if (flags & TAPEALERT_ENABLED) {
		printf("%s", catgets(catfd, SET, 13253, "on"));
	} else {
		printf("%s", catgets(catfd, SET, 13254, "off"));
	}
	printf(" ");
	if (flags & TAPEALERT_SUPPORTED) {
		printf("%s %s", catgets(catfd, SET, 13258, "and"),
			catgets(catfd, SET, 13251, "supported"));
	} else {
		printf("%s %s", catgets(catfd, SET, 13258, "and"),
			catgets(catfd, SET, 13252, "unsupported"));
	}
	printf("\n");
}

/*
 * List the Sef flags.
 */
static void
ListSefFlags(
		int eq,		/* equipment ordinal */
		char *msg,	/* message about events */
		uchar_t flags,	/* sef flags */
		int interval)
{
	printf("eq %d %s: ", eq, msg);

	if (flags & SEF_ENABLED) {
		printf("%s", catgets(catfd, SET, 13253, "on"));
	} else {
		printf("%s", catgets(catfd, SET, 13254, "off"));
	}
	printf(" %s ", catgets(catfd, SET, 13258, "and"));
	if (flags & SEF_SUPPORTED) {
		printf(catgets(catfd, SET, 13251, "supported"));
	} else {
		printf(catgets(catfd, SET, 13252, "unsupported"));
	}
	if (interval == SEF_INTERVAL_ONCE) {
		printf(" %s %s\n", catgets(catfd, SET, 13033,
		    "with a sample rate of"),
		    catgets(catfd, SET, 13633, "once"));
	} else {
		printf(" %s %ds\n", catgets(catfd, SET, 13033,
		    "with a sample rate of"),
		    interval);
	}
}

#define	  FIFO_path	SAM_FIFO_PATH"/"CMD_FIFO_NAME

static void
issue_fifo_cmd(
		sam_cmd_fifo_t *cmd_block,
		int cmd)
{			/* FIFO command to issue */
	int	fifo_fd; /* File descriptor for FIFO */
	int	size;

	fifo_fd = open(FIFO_path, O_WRONLY | O_NONBLOCK);
	if (fifo_fd < 0)
		fprintf(stderr,
		    catgets(catfd, SET, 560, "Cannot open cmd FIFO"));
	cmd_block->magic = CMD_FIFO_MAGIC;
	cmd_block->cmd = cmd;

	size = write(fifo_fd, cmd_block, sizeof (sam_cmd_fifo_t));
	close(fifo_fd);
	if (size != sizeof (sam_cmd_fifo_t)) {
		fprintf(stderr,
		    catgets(catfd, SET, 1114, "Command FIFO write failed"));
	}
}

static void
align_scsi_cmdbuf(int argc, char **argv)
{
	if (argc == 0) {
		if (defaults->flags & DF_ALIGN_SCSI_CMDBUF) {
			printf(catgets(catfd, SET, 2459,
			    "system is running in %s mode.\n"),
			    catgets(catfd, SET, 12022, "align_scsi_cmdbuf"));
		}
	} else {
		if (argc != 1) {
			fprintf(stderr, catgets(catfd, SET, 13001,
			    "Usage: %s %s\n"), pgm_name,
			    "align_scsi_cmdbuf [yes|no]");
			exit(1);
		}
		if (strcmp(*argv, "no") == 0)
			defaults->flags &= ~DF_ALIGN_SCSI_CMDBUF;
		else if (strcmp(*argv, "yes") == 0)
			defaults->flags |= DF_ALIGN_SCSI_CMDBUF;
		else {
			fprintf(stderr, catgets(catfd, SET, 13001,
			    "Usage: %s %s\n"), pgm_name,
			    "align_scsi_cmdbuf [yes|no]");
			exit(1);
		}
	}
	exit(0);
}
