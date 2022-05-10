/*
 *	init.c - initialize the library and all its elements
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

#pragma ident "$Revision: 1.63 $"

static char *_SrcFile = __FILE__;

#include <ctype.h>
#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/mtio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "driver/samst_def.h"
#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/robots.h"
#include "sam/nl_samfs.h"
#include "stk.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"


/*	structs */
struct media_index {
	int index;
	int capacity;
	struct media_index *next;
};

/*
 * Define media capacity in units of 1024
 */
#define	STK_MEDIA_CAP_COUNT 58
uint64_t default_cap[STK_MEDIA_CAP_COUNT] = {
	(1024 * 210),			/* 3480	 (210 meg) */
	(1024 * 800),			/* 3490E (800 meg) */
	(1024 * 1024 * 10),		/* DD3A	 (10 gig) */
	(1024 * 1024 * 25),		/* DD3B	 (25 gig) */
	(1024 * 1024 * 50),		/* DD3C	 (50 gig) */
	(1024 * 1024 * 0),		/* DD3D Cleaning tape */
	(1024 * 1024 * 10),		/* DLTIII(10 gig) */
	(1024 * 1024 * 20),		/* DLTIV (20 gig) */
	(1024 * 1024 * 15),		/* DLTIIIXT(15 gig) */
	(1024 * 1024 * 20),		/* STK1R (9840 20 gig) */
	(1024 * 1024 * 0),		/* STK1U (9840 cleaning tape */
	(uint64_t)(1024 * 1024 * 1.6),		/* EECART (9490 EE, 1.6G) */
	(1024 * 1024 * 0),		/* JLABEL (foreign label) */
	(1024 * 1024 * 60),		/* STK2P T9940A (60 gig) */
	(1024 * 1024 * 0),		/* STK2W T9940A cleaning tape */
	(1024 * 1024 * 0),		/* KLABEL (unsupported type) */
	(1024 * 1024 * 100),		/* LTO-100G (100 gig) */
	(1024 * 1024 * 50),		/* LTO-50G (50 gig) */
	(1024 * 1024 * 35),		/* LTO-35G (35 gig) */
	(1024 * 1024 * 10),		/* LTO-10G (10 gig) */
	(1024 * 1024 * 0),		/* LTO-CLN2 cleaning tape */
	(1024 * 1024 * 0),		/* LTO-CLN3 cleaning tape */
	(1024 * 1024 * 0),		/* LTO-CLN1 cleaning tape */
	(1024 * 1024 * 110),		/* SDLT super dlt (110 gig) */
	(1024 * 1024 * 0),		/* Virtual */
	(1024 * 1024 * 0),		/* LTO-CLNU cleaning tape */
	(1024 * 1024 * 200),		/* LTO-200G (200 gig) */
	(1024 * 1024 * 312),		/* SDLT-2 (312 gig uncompressed) */
	(1024 * 1024 * 500),		/* T10000T1  (500 gig) */
	(1024 * 1024 * 120),		/* T10000TS (120 gig) */
	(1024 * 1024 * 0),		/* T10000CT Titanium cleaning tape */
	(1024 * 1024 * 400),		/* LTO-400G */
	(1024 * 1024 * 400),		/* LTO-400W */
	(1024 * 1024 * 0),		/* reserved */
	(1024 * 1024 * 800),		/* SDLT-S1 */
	(1024 * 1024 * 800),		/* SDLT-S2 */
	(1024 * 1024 * 800),		/* SDLT-S3 */
	(1024 * 1024 * 800),		/* SDLT-S4 */
	(1024 * 1024 * 800),		/* SDLT-4 */
	(1024 * 1024 * 800),		/* STK1Y */
	(1024 * 1024 * 800),		/* LTO-800G */
	(1024 * 1024 * 800),		/* LT0-800W */
	(1024 * 1024 * 1000),		/* T10000T2 */
        (1024 * 1024 * 1000),   	/* T10000TT */
        (1024 * 1024 * 0),   		/* T10000CC */
        (1024 * 1024 * 1536),  		/* LTO-1.5T */
        (1024 * 1024 * 1536),  		/* LTO-1.5W */
        (1024 * 1024 * 0),   		/* T10000CL */
        ((uint64_t)1024 * 1024 * 2560),  		/* LTO-2.5T */
        ((uint64_t)1024 * 1024 * 2560),  		/* LTO-2.5W */
        ((uint64_t)1024 * 1024 * 1024 * 6),   	/* LTO-6.4T */
        ((uint64_t)1024 * 1024 * 1024 * 6),   	/* LTO-6.4W */
        ((uint64_t)1024 * 1024 * 1024 *12),   	/* LTO-12T */
        ((uint64_t)1024 * 1024 * 1024 *12),   	/* LTO-12W */
        ((uint64_t)1024 * 1024 * 5000),   	/* T10000TA */
        ((uint64_t)1024 * 1024 * 1024 * 9),   	/* LTO-9T */
        ((uint64_t)1024 * 1024 * 1024 * 18),   	/* LTO-18T */
        ((uint64_t)1024 * 1024 * 1024 * 18),   	/* LTO-18W */
};

/*	function prototypes */
int init_drives(library_t *, dev_ptr_tbl_t *);
void start_helper(library_t *);
static void start_ssi(library_t *library);

/*	globals */
extern shm_alloc_t master_shm, preview_shm;
extern void set_catalog_tape_media(library_t *library);

/*	environment var definitions */
#define	ACS_HOSTNAME "CSI_HOSTNAME"
#define	ACS_SSIHOST	"SSI_HOSTNAME"
#define	ACS_PORTNUM "ACSAPI_SSI_SOCKET"
#define	ACS_SSI_INET_PORT "SSI_INET_PORT"
#define	ACS_CSI_HOSTPORT "CSI_HOSTPORT"

/*	default values */
#define	ACS_DEF_PORTNUM 50004
#define	ACS_DEF_HOSTNAME "localhost"
#define	ACS_DEF_SSI_INET_PORT 0
#define	ACS_DEF_CSI_HOSTPORT 0

/*	environment variables */
static char env_acs_hostname[128];
static char env_acs_ssihost[128];
static char env_acs_portnum[80];
static char env_acs_ssi_inet_port[80];
static char env_acs_csi_hostport[80];

static char hostname[128];


/*
 * initialize - initialize the library
 * exit -
 *	  0 - ok
 *	 !0 - failed
 */
int
initialize(library_t *library, dev_ptr_tbl_t *dev_ptr_tbl)
{
	int 	i, fatal = 0;
	int 	high_index = STK_MEDIA_CAP_COUNT - 1;
	int 	tmp_lsm;
	char 	*ent_pnt = "initialize";
	char 	*line, *tmp;
	char 	*l_mess = library->un->dis_mes[DIS_MES_NORM];
	char 	*lc_mess = library->un->dis_mes[DIS_MES_NORM];
	char 	*err_fmt = catgets(catfd, SET, 9120, "%s: Syntax error in stk"
	    " configuration file line %d.");
	FILE 	*open_str;
	drive_state_t 		*drive;
	struct media_index 	*media_p, *media_indexs;
	struct CatalogHdr 	*ch;
	SEQ_NO 				sequence = (SEQ_NO) 1;
	struct	stat		buf;

	media_indexs = media_p =
	    (struct media_index *)
	    malloc_wait(sizeof (struct media_index), 2, 0);
	memset(media_indexs, 0, sizeof (struct media_index));

	/*
	 * Initialize the port number to zero. It will
	 * later be used by import command if importing by pools.
	 *
	 * Initialize capid to -1 to indicate that no cap has
	 * been defined (yet) in the params file for one step export.
	 */
	library->un->dt.rb.port_num = 0;
	library->un->dt.rb.capid = ROBOT_NO_SLOT;

	if ((open_str = fopen(library->un->name, "r")) == NULL) {
		sam_syslog(LOG_CRIT, catgets(catfd, SET, 9117,
		    "%s: Unable to open"
		    " configuration file(%s): %m."),
		    ent_pnt, library->un->name);
		memccpy(lc_mess, catgets(catfd, SET, 9118,
		    "unable to open configuration file"),
		    '\0', DIS_MES_LEN);
		return (-1);
	}

	/*
	 * Set hasam_running flag if HASAM_RUN_FILE exists.
	 */
	library->hasam_running = FALSE;

	if (stat(HASAM_RUN_FILE, &buf) == 0) {
		library->hasam_running = TRUE;
	}

	if (CatalogInit("stk") == -1) {
		sam_syslog(LOG_ERR, "%s",
		    catgets(catfd, SET, 2364,
		    "Catalog initialization failed!"));
		exit(1);
	}

	i = 0;
	line = malloc_wait(200, 2, 0);
	mutex_lock(&library->un->mutex);

	if (init_drives(library, dev_ptr_tbl)) {
		mutex_unlock(&library->un->mutex);
		return (-1);
	}

	library->index = library->drive;

	gethostname(hostname, 128);

	sprintf(env_acs_hostname, "%s=%s", ACS_HOSTNAME, ACS_DEF_HOSTNAME);
	sprintf(env_acs_ssihost, "%s=%s", ACS_SSIHOST, hostname);
	sprintf(env_acs_portnum, "%s=%d", ACS_PORTNUM, ACS_DEF_PORTNUM);
	sprintf(env_acs_ssi_inet_port, "%s=%d", ACS_SSI_INET_PORT,
	    ACS_DEF_SSI_INET_PORT);
	sprintf(env_acs_csi_hostport, "%s=%d", ACS_CSI_HOSTPORT,
	    ACS_DEF_CSI_HOSTPORT);

	while (fgets(memset(line, 0, 200), 200, open_str) != NULL) {
		char *open_paren;
		i++;

		if (*line == '#' || strlen(line) == 0 || *line == '\n')
			continue;

		if (*line == '(') {
			sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
			continue;
		}

		if ((tmp = strchr(line, '#')) != NULL)
			memset(tmp, 0, (200 - (tmp - line)));

		/*
		 * If open paren found, put a zero byte on top of it
		 * so the first strtok won't find any = within the parens.
		 */
		if ((open_paren = strchr(line, '(')) != NULL)
			*open_paren = '\0';

		if ((tmp = strtok(line, "= \t\n")) == NULL)
			continue;

		if (strcasecmp(tmp, "access") == 0) {
			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}
			if (strlen(tmp) > (sizeof (vsn_t) + 9 + 17 +
			    sizeof (SAM_CDB_LENGTH)))

				sam_syslog(LOG_INFO,
				    catgets(catfd, SET, 9119,
				    "%s: access id too long - ignored."),
				    ent_pnt);
			else
				memcpy(&library->un->vsn, tmp, strlen(tmp) + 1);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "setting access to %s.",
				    &library->un->vsn);
			continue;
		} else if (strcasecmp(tmp, "capacity") == 0) {
			int this_index, value, first = TRUE;

			if (open_paren == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			open_paren++;

			while (TRUE) {
				char *param;

				if (first)
					tmp = strtok(open_paren, " ,=:\t)");
				else
					tmp = strtok(NULL, " ,=:\t)");

				first = FALSE;
				if (tmp == NULL)
					break;

				if (*tmp == '\n' || *tmp == '\0')
					break;

				if ((param = strtok(NULL, " ,=:\t)")) == NULL) {
					sam_syslog(LOG_INFO, err_fmt,
					    ent_pnt, i);
					break;
				}

				if (isalpha(*param) || *param == '\0') {
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9121,
					    "%s: STK configuration error"
					    " line %d: Index error."),
					    ent_pnt, i);
					continue;
				}

				if (((this_index = atoi(tmp)) < 0) ||
				    this_index > 63) {

					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9121,
					    "%s: STK configuration error"
					    " line %d: Index error."),
					    ent_pnt, i);
					continue;
				}

				if (high_index < this_index)
					high_index = this_index;

				if ((value = atoi(param)) >= 0) {
					if (DBG_LVL(SAM_DBG_DEBUG))
						sam_syslog(LOG_DEBUG,
						    "stk capacity index"
						    " %d-%dk.",
						    this_index, value);
					media_p->index = this_index;
					media_p->capacity = value;
					media_p->next =
					    (struct media_index *)
					    malloc_wait(
					    sizeof (struct media_index),
					    2, 0);
					memset(media_p->next, 0,
					    sizeof (struct media_index));
					media_p = media_p->next;
				} else {
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9121,
					    "%s: STK configuration error"
					    " line %d: Index error."),
					    ent_pnt, i);
					continue;
				}
			}
		} else if (strcasecmp(tmp, "hostname") == 0) {

			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			sprintf(env_acs_hostname, "%s=%s", ACS_HOSTNAME, tmp);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "setting hostname to %s", tmp);

			continue;
		} else if (strcasecmp(tmp, "ssihost") == 0) {

			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			sprintf(env_acs_ssihost, "%s=%s", ACS_SSIHOST, tmp);
			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG,
				    "setting ssiname to %s", tmp);
			}
			continue;
		} else if (strcasecmp(tmp, "ssi_inet_port") == 0) {

			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			if ((atoi(tmp) != 0) &&
			    ((atoi(tmp) < 1024) || (atoi(tmp) > 65535))) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			sprintf(env_acs_ssi_inet_port, "%s=%s",
			    ACS_SSI_INET_PORT, tmp);

			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG,
				    "setting ssi_inet_port to %s", tmp);
			}
			continue;
		} else if (strcasecmp(tmp, "csi_hostport") == 0) {

			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			if ((atoi(tmp) != 0) &&
			    ((atoi(tmp) < 1024) || (atoi(tmp) > 65535))) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			sprintf(env_acs_csi_hostport, "%s=%s",
			    ACS_CSI_HOSTPORT, tmp);

			if (DBG_LVL(SAM_DBG_DEBUG)) {
				sam_syslog(LOG_DEBUG,
				    "setting csi_hostport to %s", tmp);
			}
			continue;
		} else if (strcasecmp(tmp, "portnum") == 0) {

			if ((tmp = strtok(NULL, "= \t\n")) == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			sprintf(env_acs_portnum, "%s=%d",
			    ACS_PORTNUM, atoi(tmp));
			/*
			 * Save the port number in dev_ent_t. It needs to be
			 * available for the import command to import by pools.
			 */
			library->un->dt.rb.port_num = atoi(tmp);
			if (DBG_LVL(SAM_DBG_DEBUG))
				sam_syslog(LOG_DEBUG,
				    "setting port number to %d", atoi(tmp));
			continue;
		} else if (strcasecmp(tmp, "capid") == 0) {
			int parts = 0, value;

			if (open_paren == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			open_paren++;

			while (parts < 3) {
				char *param;

				if (parts == 0) {
					tmp = strtok(open_paren, " ,=:\t)");
				} else
					tmp = strtok(NULL, " ,=:\t)");

				if (tmp == NULL) {
					sam_syslog(LOG_INFO,
					    err_fmt, ent_pnt, i);
					break;
				}

				if ((param = strtok(NULL, " ,=:\t)")) == NULL) {
					sam_syslog(LOG_INFO, err_fmt,
					    ent_pnt, i);
					break;
				}

				if (strcasecmp(tmp, "acs") == 0) {
					if (isalpha(*param) ||
					    *param == '\0') {

						sam_syslog(LOG_INFO,
						    catgets(catfd, SET, 9122,
						"%s: ACS specification error"
						" line %d."),
						    ent_pnt, i);
						continue;
					}
					value = atoi(param);
					library->capid.lsm_id.acs = value;
				} else if (strcasecmp(tmp, "lsm") == 0) {
					if (isalpha(*param) ||
					    *param == '\0') {

						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9124, "%s: LSM"
						    " specification error"
						    " line %d."),
						    ent_pnt, i);
						continue;
					}
					value = atoi(param);
					library->capid.lsm_id.lsm = value;
				} else if (strcasecmp(tmp, "cap") == 0) {

					if (isalpha(*param) ||
					    *param == '\0') {
						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9133, "%s: CAP"
						    " specification error"
						    " line %d."),
						    ent_pnt, i);
						continue;
					}
					value = atoi(param);
					library->capid.cap = value;
					library->un->dt.rb.capid = value;
				} else
					sam_syslog(LOG_INFO, catgets(catfd,
					    SET, 9130,
					    "%s:Unknown keyword(%s) line"
					    " %d."),
					    ent_pnt, tmp, i);

				parts++;
			}
		} else if (*tmp == '/') {
			int 	parts = 0, value, shared_drive = 0;
			char 	*rmtname;
			char 	*shared;
			drive_state_t 	*drive;
			DRIVE 	acs_drive = MAX_DRIVE + 1;
			PANEL 	acs_panel = MAX_PANEL + 1;
			LSM 	acs_lsm = MAX_LSM + 1;
			ACS 	acs_acs = MAX_ACS + 1;

			if (open_paren == NULL) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			open_paren++;
			rmtname = tmp;

			while (parts < 4) {
				char *param;

				if (parts == 0)
					tmp = strtok(open_paren, " ,=:\t");
				else
					tmp = strtok(NULL, " ,=:\t");

				if (tmp == NULL) {
					sam_syslog(LOG_INFO, err_fmt,
					    ent_pnt, i);
					break;
				}

				if ((param = strtok(NULL, " ,=:\t")) == NULL) {
					sam_syslog(LOG_INFO, err_fmt,
					    ent_pnt, i);
					break;
				}

				if (strcasecmp(tmp, "acs") == 0) {
					if (isalpha(*param) ||
					    *param == '\0') {
						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9122, "%s: ACS"
						    " specification error"
						    " line %d."),
						    ent_pnt, i);
						continue;
					}
					value = atoi(param);
					if (value < MIN_ACS ||
					    value > MAX_ACS) {

						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9123, "%s: ACS range"
						    " error line %d."),
						    ent_pnt, i);
						continue;
					}
					acs_acs = value;
				} else if (strcasecmp(tmp, "lsm") == 0) {
					if (isalpha(*param) || *param == '\0') {
						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9124, "%s: LSC"
						    " specification error"
						    " line %d."),
						    ent_pnt, i);
						continue;
					}

					value = atoi(param);
					if (value < MIN_LSM ||
					    value > MAX_LSM) {

						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9125, "%s: LSM range"
						    " error line %d."),
						    ent_pnt, i);
						continue;
					}

					acs_lsm = value;
				} else if (strcasecmp(tmp, "panel") == 0) {
					if (isalpha(*param) || *param == '\0') {
						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9126, "%s: PANEL"
						    " specification error"
						    " line %d."),
						    ent_pnt, i);
						continue;
					}

					value = atoi(param);
					if (value < MIN_PANEL ||
					    value > MAX_PANEL) {

						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9127, "%s: PANEL range"
						    " error line %d."),
						    ent_pnt, i);
						continue;
					}

					acs_panel = value;
				} else if (strcasecmp(tmp, "drive") == 0) {

					if (isalpha(*param) ||
					    *param == '\0') {

						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9128, "%s: DRIVE"
						    " specification error"
						    " line %d."),
						    ent_pnt, i);
						continue;
					}

					value = atoi(param);

					if (value < MIN_DRIVE ||
					    value > MAX_DRIVE) {

						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9129, "%s: DRIVE range"
						    " error line %d."),
						    ent_pnt, i);

						continue;
					}

					acs_drive = value;
				} else if (strcasecmp(tmp, ")") == 0) {
					break;
				} else
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9130,
					    "%s:Unknown keyword(%s) line"
					    " %d."),
					    ent_pnt, tmp, i);
				parts++;
			}

			if (parts != 4) {
				sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
				continue;
			}

			tmp = strtok(NULL, " ,=:\t)");
			if (tmp != NULL) {
				if (strncasecmp(tmp, "shared", 6) == 0) {
					shared_drive = B_TRUE;
				} else {
					sam_syslog(LOG_INFO,
					    catgets(catfd, SET, 9130,
					    "%s:Unknown keyword(%s) line"
					    " %d."), ent_pnt, tmp, i);
				}
			}

			for (drive = library->drive;
			    drive != NULL;
			    drive = drive->next) {

				if (strcmp(drive->un->name, rmtname) == 0) {
					if (shared_drive) {
						drive->un->flags = DVFG_SHARED;
						sam_syslog(LOG_INFO,
						    catgets(catfd, SET, 9147,
						"Drive %d is a shared drive."),
						    drive->un->eq);
					}

					if (acs_lsm == (MAX_LSM + 1) ||
					    acs_acs == (MAX_ACS + 1) ||
					    acs_panel ==
					    (MAX_PANEL + 1) ||
					    acs_drive ==
					    (MAX_DRIVE + 1)) {
						drive->un->state = DEV_DOWN;
						drive->status.b.offline = TRUE;
						sam_syslog(LOG_INFO,
						    catgets(catfd, SET,
						    9131,
						    "%s: Drive %d set"
						    " down due to"
						    " configuration error"),
						    ent_pnt, drive->un->eq);
						break;
					} else {
						drive->drive_id.drive
						    = acs_drive;
						drive->
						    drive_id.panel_id.panel
						    = acs_panel;
						drive->
						    drive_id.panel_id.lsm_id.lsm
						    = acs_lsm;
						drive->
						    drive_id.panel_id.lsm_id.acs
						    = acs_acs;
						break;
					}
				}
			}

			if (drive == NULL) {
				sam_syslog(LOG_INFO,
				    catgets(catfd, SET, 9132,
				    "%s: Cannot find aci drive name"
				    " line %d."),
				    ent_pnt, i);
				fatal++;
			}
			continue;
		} else {
			sam_syslog(LOG_INFO, err_fmt, ent_pnt, i);
			continue;
		}
	}
	fclose(open_str);

	/* now set the environment variables */
	putenv(env_acs_hostname);
	putenv(env_acs_ssihost);
	putenv(env_acs_portnum);

	/* fire up ssi */
	start_ssi(library);

	library->media_capacity.count = high_index + 1;
	i = (high_index + 1) * sizeof (int);
	library->media_capacity.capacity =
	    (int *)malloc_wait(i, 2, 0);
	memset(library->media_capacity.capacity, 0, i);
	media_p = media_indexs;

	while (TRUE) {
		void *hold;

		if (media_p->next == NULL) {
			free(media_p);
			break;
		}

		*(library->media_capacity.capacity + media_p->index)
		    = media_p->capacity;
		hold = media_p;
		media_p = media_p->next;
		free(hold);
	}

	/* Fill in based on the stk defaults for media 0 - 7 */
	for (i = 0; i <= high_index; i++) {
		if (i < STK_MEDIA_CAP_COUNT)
			if (*(library->media_capacity.capacity + i) == 0)
				*(library->media_capacity.capacity + i)
				    = default_cap[i];

		if (*(library->media_capacity.capacity + i))
			sam_syslog(LOG_INFO, catgets(catfd, SET, 9143,
			    "Capacity for media type index %d = %dK."),
			    i, *(library->media_capacity.capacity + i));
	}

	tmp_lsm = library->drive->drive_id.panel_id.lsm_id.lsm;
	library->status.b.passthru = 0;

	for (drive = library->drive; drive != NULL; drive = drive->next) {

		if (drive->drive_id.panel_id.lsm_id.lsm != tmp_lsm) {
			library->status.b.passthru = 1;
		}

		if (drive->drive_id.drive == (MAX_DRIVE + 1) &&
		    drive->drive_id.panel_id.panel ==
		    (MAX_PANEL + 1) &&
		    drive->drive_id.panel_id.lsm_id.lsm ==
		    (MAX_LSM + 1) &&
		    drive->drive_id.panel_id.lsm_id.acs ==
		    (MAX_ACS + 1)) {
			drive->un->state = DEV_DOWN;
			drive->status.b.offline = TRUE;
			memccpy(drive->un->dis_mes[DIS_MES_CRIT],
			    catgets(catfd, SET, 9145,
			    "not in configuration file"),
			    '\0', DIS_MES_LEN);
			sam_syslog(LOG_ERR,
			    catgets(catfd, SET, 9144,
			    "%s: No entry for drive %d in"
			    " configuration file."),
			    ent_pnt, drive->un->eq);
		}
	}

	if (fatal) {
		sprintf(l_mess, "Initialization failed.");
		return (-1);
	}

	/* Get the catalog media type from the first active drive */
	set_catalog_tape_media(library);

	if (!(is_optical(library->un->media)) &&
	    !(is_tape(library->un->media))) {
		sam_syslog(LOG_INFO, catgets(catfd, SET, 9357,
		    "Unable to determine media type."
		    " Library daemon exiting."));
		if (library->ssi_pid > 0) {
			kill(library->ssi_pid, 9);
		}
		exit(1);
	}

	ch = CatalogGetHeader(library->eq);
	library->un->status.bits |= (DVST_PRESENT | DVST_REQUESTED);
	if ((sam_atomedia(ch->ChMediaType) == DT_OPTICAL) ||
	    (sam_atomedia(ch->ChMediaType) == DT_WORM_OPTICAL_12) ||
	    (sam_atomedia(ch->ChMediaType) == DT_WORM_OPTICAL) ||
	    (sam_atomedia(ch->ChMediaType) == DT_ERASABLE) ||
	    (sam_atomedia(ch->ChMediaType) == DT_MULTIFUNCTION))
		library->status.b.two_sided = TRUE;
	else
		library->status.b.two_sided = FALSE;

	(void) CatalogSetCleaning(library->eq);

	mutex_unlock(&library->un->mutex);

	/* allocate the free list */
	library->free = init_list(ROBO_EVENT_CHUNK);
	library->free_count = ROBO_EVENT_CHUNK;
	mutex_init(&library->free_mutex, USYNC_THREAD, NULL);
	mutex_init(&library->list_mutex, USYNC_THREAD, NULL);
	cond_init(&library->list_condit, USYNC_THREAD, NULL);

	library->transports = malloc_wait(sizeof (xport_state_t), 5, 0);
	(void) memset(library->transports, 0, sizeof (xport_state_t));

	mutex_lock(&library->transports->mutex);
	library->transports->library = library;
	library->transports->first = malloc_wait(sizeof (robo_event_t), 5, 0);
	(void) memset(library->transports->first, 0, sizeof (robo_event_t));
	library->transports->first->type = EVENT_TYPE_INTERNAL;
	library->transports->first->status.bits = REST_FREEMEM;
	library->transports->first->request.internal.command = ROBOT_INTRL_INIT;
	library->transports->active_count = 1;
	library->helper_pid = -1;
	memccpy(l_mess, catgets(catfd, SET, 9151,
	    "waiting for helper to start"), '\0', DIS_MES_LEN);

	start_helper(library);
	{
		char *MES_9152 = catgets(catfd, SET, 9152,
		    "helper running as pid %d");
		char *mes = (char *)malloc_wait(strlen(MES_9152) + 15, 5, 0);

		sprintf(mes, MES_9152, library->helper_pid);
		memccpy(l_mess, mes, '\0', DIS_MES_LEN);
		free(mes);
	}

	library->transports->sequence = sequence;
	if (thr_create(NULL, MD_THR_STK, &transport_thread,
	    (void *)library->transports,
	    (THR_NEW_LWP | THR_BOUND | THR_DETACHED),
	    &library->transports->thread)) {
		sam_syslog(LOG_ERR,
		    "Unable to create thread transport_thread: %m.");
		library->transports->thread = (thread_t)(- 1);
	}

	mutex_unlock(&library->transports->mutex);
	mutex_unlock(&library->mutex);

	/* free the drive threads */
	for (drive = library->drive; drive != NULL; drive = drive->next)
		mutex_unlock(&drive->mutex);

	return (0);
}


int
init_drives(library_t *library, dev_ptr_tbl_t *dev_ptr_tbl)
{
	int i;
	dev_ent_t *un;
	drive_state_t *drive, *nxt_drive;

	drive = NULL;
	/*
	 * For each drive, build the drive state structure,
	 * put the init request on the list and start a thread with a new lwp
	 */
	for (i = 0; i <= dev_ptr_tbl->max_devices; i++)
		if (dev_ptr_tbl->d_ent[i] != 0) {
			un = (dev_ent_t *)SHM_REF_ADDR(dev_ptr_tbl->d_ent[i]);
			if (un->fseq == library->eq && !IS_ROBOT(un)) {
				nxt_drive = malloc_wait(sizeof (drive_state_t),
				    5, 0);
				nxt_drive->previous = drive;
				if (drive != NULL)
					drive->next = nxt_drive;
				else
					library->drive = nxt_drive;
				drive = nxt_drive;
				(void) memset(drive, 0, sizeof (drive_state_t));
				drive->library = library;
				drive->open_fd = -1;
				drive->un = un;
				*(U_ID(drive->drive_id)) = 0xffffffff;
				/* hold lock until ready */
				mutex_lock(&drive->mutex);
				memset(&drive->un->vsn, 0, sizeof (vsn_t));
				drive->new_slot = ROBOT_NO_SLOT;
				drive->un->slot = ROBOT_NO_SLOT;
				library->countdown++;
				drive->active_count = 1;
				/*
				 * We can end up here with un->active greater
				 * than zero after the daemon has core dumped.
				 * This will cause a hang in clear_drive
				 * waiting for the drive to become inactive.
				 */
				drive->un->active = 0;
				drive->first = malloc_wait(
				    sizeof (robo_event_t), 5, 0);
				(void) memset(drive->first, 0,
				    sizeof (robo_event_t));
				drive->first->type = EVENT_TYPE_INTERNAL;
				drive->first->status.bits = REST_FREEMEM;
				drive->first->request.internal.command =
				    ROBOT_INTRL_INIT;
				if (thr_create(NULL, MD_THR_STK,
				    &drive_thread, (void *)drive,
				    (THR_NEW_LWP | THR_BOUND |
				    THR_DETACHED),
				    &drive->thread)) {
					sam_syslog(LOG_ERR,
					    "Unable to create thread"
					    " drive_thread: %m.");
					drive->status.b.offline = TRUE;
					drive->thread = (thread_t)(- 1);
				}
			}
		}
	return (0);
}


/*
 * init_drive - initialize drive
 */
void
init_drive(drive_state_t *drive)
{
	char *ent_pnt = "init_drive";
	char *d_mess = drive->un->dis_mes[DIS_MES_NORM];
	char *dc_mess = drive->un->dis_mes[DIS_MES_CRIT];

	mutex_lock(&drive->mutex);
	mutex_lock(&drive->un->mutex);
	drive->un->scan_tid = thr_self();

	memccpy(d_mess, catgets(catfd, SET, 9065, "initializing"),
	    '\0', DIS_MES_LEN);
	if (drive->un->state < DEV_IDLE) {
		int 	local_open, err, drive_status, drive_state;
		int 	down_it = FALSE;
		char 	*dev_name;

		drive->un->status.bits |= DVST_REQUESTED;
		drive->status.b.full = FALSE;

		mutex_unlock(&drive->mutex);
		mutex_unlock(&drive->un->mutex);

		err = query_drive(drive->library, drive,
		    &drive_status, &drive_state);
		if (DBG_LVL(SAM_DBG_DEBUG))
			sam_syslog(LOG_DEBUG,
			    "from query drive(%d:%d).",
			    err, drive_state);

		mutex_lock(&drive->mutex);
		mutex_lock(&drive->un->mutex);

		/*
		 * If the drive is not online or the query failed,
		 * then down the drive
		 */
		if (err == STATUS_SUCCESS && drive_state == STATE_ONLINE) {

			switch (drive_status) {

			case STATUS_DRIVE_IN_USE:
				drive->status.b.full = TRUE;
				/* FALLTHROUGH */
			case STATUS_DRIVE_AVAILABLE:
				break;

			default:
				down_it = TRUE;
				break;
			}
		} else
			down_it = TRUE;

		if (down_it) {
			memccpy(dc_mess,
			    catgets(catfd, SET, 9153,
			    "drive set down due to"
			    " ACS reported state"), '\0', DIS_MES_LEN);
			sam_syslog(LOG_INFO,
			    catgets(catfd, SET, 9154,
			    "%s: Drive (%d) set down:"
			    " ACS reported state."),
			    ent_pnt, drive->un->eq);
			drive->un->state = DEV_DOWN;
			drive->status.b.offline = TRUE;
			drive->un->status.bits &= ~DVST_REQUESTED;
			mutex_unlock(&drive->mutex);
			mutex_unlock(&drive->un->mutex);
			mutex_lock(&drive->library->mutex);
			drive->library->countdown--;
			mutex_unlock(&drive->library->mutex);
			return;
		}

		if (IS_TAPE(drive->un))
			ChangeMode(drive->un->name, SAM_TAPE_MODE);

		if (IS_OPTICAL(drive->un))
			dev_name = &drive->un->name[0];
		else
			dev_name = samst_devname(drive->un);

		drive->un->status.bits |= DVST_READ_ONLY;
		local_open = open_unit(drive->un, dev_name, 1);
		clear_driver_idle(drive, local_open);

		mutex_lock(&drive->un->io_mutex);

		if (scsi_cmd(local_open, drive->un, SCMD_TEST_UNIT_READY, 10)
		    >= 0)

			drive->status.b.full = TRUE;

		if (drive->un->vsn[0] != '\0') {
			drive->status.b.full = TRUE;
		}

		close_unit(drive->un, &local_open);
		if (dev_name != &drive->un->dt.tp.samst_name[0])
			free(dev_name);

		mutex_unlock(&drive->un->io_mutex);
		mutex_unlock(&drive->un->mutex);

		if (drive->status.b.full) {
			if (!(drive->un->flags & DVFG_SHARED)) {
				if (clear_drive(drive))
					down_drive(drive, SAM_STATE_CHANGE);
			} else {
				struct CatalogEntry ced;
				struct CatalogEntry *ce = &ced;

				/*
				 * For shared drives, only unload the drive
				 * if this volume belongs to this SAM-FS
				 * (i.e., we find this volume in our catalog)
				 */
				if (((ce = CatalogGetCeByMedia(
				    sam_mediatoa(drive->un->type),
				    drive->un->vsn, &ced)) != NULL) &&
				    (!(ce->CeStatus & CES_occupied)) &&
				    (ce->CeEq == drive->library->eq)) {

					if (clear_drive(drive))
						down_drive(drive,
						    SAM_STATE_CHANGE);
				} else {
					drive->status.b.full = FALSE;
					memccpy(d_mess,
					    catgets(catfd, SET, 9206,
					    "empty"), '\0',
					    DIS_MES_LEN);
				}
			}
		}

		mutex_unlock(&drive->mutex);
		mutex_lock(&drive->un->mutex);
		if (drive->open_fd >= 0)
			close_unit(drive->un, &drive->open_fd);
		drive->un->status.bits &= ~(DVST_READ_ONLY | DVST_REQUESTED);
		mutex_unlock(&drive->un->mutex);
	} else {
		mutex_unlock(&drive->mutex);
		drive->un->status.bits &= ~(DVST_READ_ONLY | DVST_REQUESTED);
		mutex_unlock(&drive->un->mutex);
		*d_mess = '\0';
	}

	mutex_lock(&drive->library->mutex);
	drive->library->countdown--;
	mutex_unlock(&drive->library->mutex);
	if (drive->un->state < DEV_IDLE)
		*dc_mess = *d_mess = '\0';
}


/*
 *	re_init_library -
 */
int
re_init_library(library_t *library)
{
	return (0);
}


void
start_helper(library_t *library)
{
	char 	*ent_pnt = "start_helper";
	pid_t 	pid = -1;
	char 	path[512], *lc_mess = library->un->dis_mes[DIS_MES_CRIT];

	sprintf(path, "%s/sam-stk_helper", SAM_EXECUTE_PATH);

	while (pid < 0) {
		int fd;

		/* Set non-standard files to close on exec. */
		for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
			(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
		}

		if ((pid = fork1()) == 0) {	/* we are the child */
			char shmid[12], equ[12];

			setgid(0);	/* clear special group id */

			sprintf(shmid, "%#d", master_shm.shmid);
			sprintf(equ, "%#d", library->eq);
			execl(path, "sam-stk_helper", shmid, equ, NULL);
			/* Cant translate since the fd is closed */
			sprintf(lc_mess, "helper did not start: %s",
			    error_handler(errno));
			_exit(1);
		}

		if (pid < 0)
			sam_syslog(LOG_ERR,
			    "%s: Unable to start sam-stk_helper:%s.",
			    ent_pnt, error_handler(errno));

		sleep(5);
	}

	library->helper_pid = pid;
}


static void
start_ssi(library_t *library)
{
	char 	*ent_pnt = "start_ssi";
	pid_t 	pid = -1;
	char 	path[512], *lc_mess = library->un->dis_mes[DIS_MES_CRIT];

	sprintf(path, "%s/ssi.sh", SAM_SCRIPT_PATH);

	while (pid < 0) {
		if ((pid = fork1()) == 0) {	/* we are the child */
			int 	fd;
			char 	shmid[12], equ[12], pshmid[12];

			setgid(0);	/* clear special group id */

			/* Set non-standard files to close on exec. */
			for (fd = STDERR_FILENO + 1; fd < OPEN_MAX; fd++) {
				(void) fcntl(fd, F_SETFD, FD_CLOEXEC);
			}

			sprintf(shmid, "%#d", master_shm.shmid);
			sprintf(pshmid, "%#d", preview_shm.shmid);
			sprintf(equ, "%#d", getpid());
			execl(path, "ssi.sh", shmid, pshmid, equ, NULL);

			/* Can't translate since the fd is closed */
			sprintf(lc_mess, "ssi.sh did not start: %s",
			    error_handler(errno));
			_exit(1);
		}

		if (pid < 0)
			sam_syslog(LOG_ERR,
			    "%s: Unable to start ssi.sh:%s.",
			    ent_pnt, error_handler(errno));

		sleep(5);
	}

	library->ssi_pid = pid;
}
