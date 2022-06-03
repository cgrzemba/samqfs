/*
 * import.c
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

#pragma ident "$Revision: 1.25 $"


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <libgen.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define	DEC_INIT

#include "sam/types.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/device.h"
#include "aml/fifo.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "sam/custmsg.h"
#include <dlfcn.h>
#if !defined(_NoACSLS_)
#include "acssys.h"
#include "acsapi.h"
#else
#define POOL int
#endif

/* globals */
shm_alloc_t              master_shm, preview_shm;

static void	Usage(void);
static void    *map_sym(void *, char *);

#if !defined(_NoACSLS_)
#define	ACS_STAT(t)	char *(*t)(STATUS)
#define	ACS_QP(t)	STATUS (*t)(SEQ_NO, POOL[], ushort_t)
#define	ACS_QS(t)	STATUS (*t)(SEQ_NO, POOL[], ushort_t)
#define	ACS_RSP(t)	STATUS (*t)(int, SEQ_NO *, REQ_ID *, \
				ACS_RESPONSE_TYPE *, void *)
#define	ACS_SA(t)	STATUS (*t)(char *)
#define	ACS_SS(t)	STATUS (*t)(SEQ_NO, LOCKID, POOL, \
				VOLRANGE[], BOOLEAN, ushort_t)
#define	ACS_PORTNUM	"ACSAPI_SSI_SOCKET"
#define	ACS_DEF_PORTNUM 50004
#endif

#define	FIFO_PATH	"/FIFO_CMD"

static char	env_acs_portnum[80];

int
main(int argc, char **argv)
{
	int	shmid, fifo_fd;
	int	audit_eod = 0, strange = 0, media = 0;
	void	*memory;
	char	fifo_file[MAXPATHLEN];
	char	*volser = NULL, *barcode = NULL;

	dev_ent_t	*device;
	dev_ptr_tbl_t	*dev_ptr_tbl;
	shm_ptr_tbl_t	*shm_ptr_tbl;
	sam_cmd_fifo_t	cmd_block;

	int	pool_count = -1, l_opt = 0;
	void   *api_handle;
	POOL    what_pool = -1;

#if !defined(_NoACSLS_)
	ACS_STAT(dl_acs_status);
	ACS_QP(dl_acs_query_pool);
	ACS_QS(dl_acs_query_scratch);
	ACS_RSP(dl_acs_response);
	ACS_SA(dl_acs_set_access);
	ACS_SS(dl_acs_set_scratch);
#endif

	CustmsgInit(0, NULL);

	program_name = (char *)basename(argv[0]);

#if !defined(DEBUG)
	if (geteuid() != 0) {
		fprintf(stderr, catgets(catfd, SET, 100,
		    "%s may be run only by super-user.\n"),
		    program_name);
		exit(1);
	}
#endif
	argc--;
	argv++;
	if (argc <= 0) {
		Usage();
		/* NOTREACHED */
	}
	while (argc > 1) {
		if (strcmp(*argv, "-v") == 0) {
			argc--;
			argv++;
			if (argc < 2) {
				Usage();
				/* NOTREACHED */
			}
			volser = strdup(*argv);
			argc--;
			argv++;
		} else if (strcmp(*argv, "-b") == 0) {
			argc--;
			argv++;
			if (argc < 2) {
				Usage();
				/* NOTREACHED */
			}
			barcode = strdup(*argv);
			argc--;
			argv++;
		} else if (strcmp(*argv, "-m") == 0) {
			argc--;
			argv++;
			if (argc < 2) {
				Usage();
				/* NOTREACHED */
			}
			if ((media = media_to_device(*argv)) == -1) {
				fprintf(stderr,
				    catgets(catfd, SET, 2755,
				    "Unknown media type %s.\n"), *argv);
				exit(1);
			}
			argc--;
			argv++;
		} else if (strcmp(*argv, "-e") == 0) {
			argc--;
			argv++;
			audit_eod = TRUE;
		} else if ((strcmp(*argv, "-n") == 0) ||
		    (strcmp(*argv, "-N") == 0)) {
			argc--;
			argv++;
			strange = TRUE;
		} else if (strcmp(*argv, "-s") == 0) {
			argc--;
			argv++;
			if (argc < 2) {
				Usage();
				/* NOTREACHED */
			}
			what_pool = atoi(*argv);
			if (what_pool < 0) {
				Usage();
				/* NOTREACHED */
			}
			argc--;
			argv++;
		} else if (strcmp(*argv, "-c") == 0) {
			argc--;
			argv++;
			if (argc < 2) {
				Usage();
				/* NOTREACHED */
			}
			pool_count = atoi(*argv);
			if (pool_count < 0) {
				Usage();
				/* NOTREACHED */
			}
			argc--;
			argv++;
		} else if (strcmp(*argv, "-l") == 0) {
			l_opt = 1;
			argc--;
			argv++;
		} else {
			fprintf(stderr,
			    catgets(catfd, SET, 2760,
			    "Unknown option %s.\n"),
			    *argv);
			Usage();
			/* NOTREACHED */
		}
	}

	if (volser && (what_pool > 0 || pool_count > 0)) {
		fprintf(stderr,
		    catgets(catfd, SET, 2255,
		    "Scratch pools and specifying vsn are "
		    "mutually exclusive.\n"));
		Usage();
		/* NOTREACHED */
	}
	if (l_opt && (pool_count == 0 || what_pool < 0)) {
		fprintf(stderr, catgets(catfd, SET, 13215,
		    "%s: -l option only allowed if importing by pools.\n"),
		    program_name);
		Usage();
		/* NOTREACHED */
	}
	if (audit_eod && strange) {
		fprintf(stderr,
		    catgets(catfd, SET, 13205,
		    "%s: -e and -n are mutually exclusive.\n"),
		    program_name);
		exit(1);
	}
#if !defined(_NoACSLS_)
	if (what_pool >= 0 || pool_count > 0) {
		if ((api_handle =
		    dlopen("libapi.so", RTLD_NOW | RTLD_GLOBAL)) == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 2483,
			    "The shared object library %s cannot "
			    "be loaded: %s"), "libapi.so", dlerror());
			fprintf(stderr, "\n");
			exit(1);
		}
		dl_acs_status =
		    (ACS_STAT()) map_sym(api_handle, "acs_status");
		dl_acs_query_pool =
		    (ACS_QP()) map_sym(api_handle, "acs_query_pool");
		dl_acs_query_scratch =
		    (ACS_QS()) map_sym(api_handle, "acs_query_scratch");
		dl_acs_response =
		    (ACS_RSP()) map_sym(api_handle, "acs_response");
		dl_acs_set_access =
		    (ACS_SA()) map_sym(api_handle, "acs_set_access");
		dl_acs_set_scratch =
		    (ACS_SS()) map_sym(api_handle, "acs_set_scratch");
		if (what_pool >= 0 && pool_count < 0)
			pool_count = 10;

		if (pool_count > 0 && what_pool < 0)
			what_pool = 0;
	}
#endif
	if (argc != 1) {
		Usage();
		/* NOTREACHED */
	}
	memset(&cmd_block, 0, sizeof (cmd_block));

	if ((shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 2244, "SAM_FS is not running.\n"));
		exit(1);
	}
	if ((memory = shmat(shmid, (void *) NULL, 0444)) == (void *) -1) {
		fprintf(stderr,
		    catgets(catfd, SET, 568,
		    "Cannot attach shared memory segment.\n"));
		exit(1);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)memory;
	sprintf(fifo_file, "%s" FIFO_PATH,
	    ((char *)memory + shm_ptr_tbl->fifo_path));

	dev_ptr_tbl =
	    (dev_ptr_tbl_t *)((char *)memory + shm_ptr_tbl->dev_table);
	cmd_block.eq = atoi(*argv);

	if (cmd_block.eq < 0 || cmd_block.eq > dev_ptr_tbl->max_devices) {
		fprintf(stderr,
		    catgets(catfd, SET, 80,
		    "%d is not a valid equipment ordinal.\n"),
		    cmd_block.eq);
		exit(1);
	}
	if (dev_ptr_tbl->d_ent[cmd_block.eq] == 0) {
		fprintf(stderr,
		    catgets(catfd, SET, 80,
		    "%d is not a valid equipment ordinal.\n"),
		    cmd_block.eq);
		exit(1);
	}
	device =
	    (dev_ent_t *)((char *)memory +
	    (int)dev_ptr_tbl->d_ent[cmd_block.eq]);

	if (device->equ_type != DT_HISTORIAN && !device->status.b.ready) {
		fprintf(stderr,
		    catgets(catfd, SET, 853, "Device %d is not ready.\n"),
		    cmd_block.eq);
		exit(1);
	}
	if (!IS_ROBOT(device)) {
		fprintf(stderr,
		    catgets(catfd, SET, 79,
		    "%d is not a robotic device.\n"),
		    cmd_block.eq);
		exit(1);
	}
	if (device->equ_type != DT_HISTORIAN && barcode != NULL) {
		fprintf(stderr,
		    catgets(catfd, SET, 517,
		    "barcode may only be specified for the historian.\n"));
		exit(1);
	}
	if (!strange && (device->equ_type != DT_HISTORIAN && media != 0)) {
		fprintf(stderr,
		    catgets(catfd, SET, 1643,
		"media type may only be specified for the historian.\n"));
		exit(1);
	}
	if (device->equ_type == DT_HISTORIAN && audit_eod) {
		fprintf(stderr,
		    catgets(catfd, SET, 487,
		    "audit_eod (-e) is not allowed for the historian.\n"));
		exit(1);
	}
	cmd_block.magic = CMD_FIFO_MAGIC;
	cmd_block.slot = ROBOT_NO_SLOT;
	cmd_block.part = 0;
	if (device->equ_type == DT_HISTORIAN) {
		if (media == 0) {
			fprintf(stderr,
			    catgets(catfd, SET, 1645,
			"media type must be supplied for the historian.\n"));
			exit(1);
		}
		cmd_block.cmd = CMD_FIFO_ADD_VSN;
		cmd_block.media = media;
		if (volser != NULL)
			strncpy(cmd_block.vsn, volser, sizeof (vsn_t));
		if (barcode != NULL) {
			cmd_block.flags |= ADDCAT_BARCODE;
			memccpy(cmd_block.info, barcode, '\0', 127);
		}
		if (volser == NULL && barcode == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 2873,
			    "VSN and/or barcode must be supplied for "
			    "import into the historian.\n"));
			exit(1);
		}
	} else if (volser == NULL && what_pool < 0) {
		if (device->type == DT_GRAUACI || device->type == DT_STKAPI ||
		    device->type == DT_IBMATL || device->type == DT_SONYPSC) {
			fprintf(stderr,
			    catgets(catfd, SET, 1328,
			    "Import to GRAU, STK, SONY and IBM libraries "
			    "must be by vsn or scratch pool(STK only).\n"));
			exit(1);
		}
		cmd_block.cmd = CMD_FIFO_IMPORT;
		if (audit_eod)
			cmd_block.flags |= CMD_IMPORT_AUDIT;

		if (strange)
			cmd_block.flags |= CMD_IMPORT_STRANGE;
	}
#if !defined(_NoACSLS_)
           else if (what_pool >= 0) {
		int		err;
		void	   *buffer, *buffer2;
		POOL	    pools[MAX_ID];
		SEQ_NO	  sequence = 0, resp_seq;
		REQ_ID	  resp_req;
		VOLRANGE	vol_ids[MAX_ID];
		ACS_RESPONSE_TYPE type;
		ACS_QUERY_POL_RESPONSE *qp_resp;
		QU_POL_STATUS  *qp_status;

		if (device->type != DT_STKAPI) {
			fprintf(stderr,
			    catgets(catfd, SET, 2254,
			    "Scratch pool only supported on STK.\n"));
			exit(1);
		}
		if ((buffer = (void *) malloc(4096)) == NULL ||
		    (buffer2 = (void *) malloc(4096)) == NULL) {
			fprintf(stderr,
			    catgets(catfd, SET, 1606,
			    "malloc: %s\n"), strerror(errno));
			exit(1);
		}
		/*
		 * Set the environment variable for the port
		 * number sam-stkd is using to talk to ACSLS.
		 */
		if (device->dt.rb.port_num == 0) {
			sprintf(env_acs_portnum, "%s=%d",
			    ACS_PORTNUM, ACS_DEF_PORTNUM);
		} else {
			sprintf(env_acs_portnum, "%s=%d",
			    ACS_PORTNUM, device->dt.rb.port_num);
		}

		putenv(env_acs_portnum);

		if (device->vsn[0] != '\0')	/* if access set */
			dl_acs_set_access(&device->vsn[0]);

		pools[0] = what_pool;
		if ((err = dl_acs_query_pool(sequence, pools, 1))) {
			fprintf(stderr,
			    catgets(catfd, SET, 339,
			    "acs_query_pool failed: %s\n"),
			    dl_acs_status(err));
			exit(1);
		}
		do {
			if ((err = dl_acs_response(-1, &resp_seq,
			    &resp_req, &type, buffer)) != STATUS_SUCCESS) {
				fprintf(stderr,
				    catgets(catfd, SET, 353,
				    "acs_response:failure %s\n"),
				    dl_acs_status(err));
				exit(1);
			}
			if (resp_seq != sequence) {
				fprintf(stderr,
				    catgets(catfd, SET, 352,
				    "acs_response: wrong sequence.\n"));
				exit(1);
			}
		} while (type != RT_FINAL);

		qp_resp = (ACS_QUERY_POL_RESPONSE *) buffer;
		if (qp_resp->query_pol_status != STATUS_SUCCESS) {
			fprintf(stderr,
			    catgets(catfd, SET, 341,
			    "acs_query_pool: status error: %s\n"),
			    dl_acs_status(qp_resp->query_pol_status));
			exit(1);
		}
		if (qp_resp->count != 1) {
			fprintf(stderr,
			    catgets(catfd, SET, 340,
			    "acs_query_pool: count error: %d.\n"),
			    qp_resp->count);
			exit(1);
		}
		qp_status = &qp_resp->pool_status[0];
		if (qp_status->pool_id.pool != what_pool) {
			fprintf(stderr,
			    catgets(catfd, SET, 342,
			    "acs_query_pool: wrong pool %d.\n"),
			    qp_status->pool_id.pool);
			exit(1);
		}
		if (qp_status->volume_count < pool_count) {
			fprintf(stderr,
			    catgets(catfd, SET, 2910,
			    "Warning: Only %d volumes left in pool.\n"),
			    qp_status->volume_count);
			pool_count = qp_status->volume_count;
		}
		while (pool_count > 0) {
			int		ii, jj;
			QU_SCR_STATUS  *qs_status;
			ACS_QUERY_SCR_RESPONSE *qs_resp;
			ACS_SET_SCRATCH_RESPONSE *ss_resp;

			sequence++;
			if ((err = dl_acs_query_scratch(sequence, pools, 1))) {
				fprintf(stderr,
				    catgets(catfd, SET, 343,
				    "acs_query_scratch failed: %s\n"),
				    dl_acs_status(err));
				exit(1);
			}
			do {
				if ((err = dl_acs_response(-1, &resp_seq,
				    &resp_req, &type, buffer)) !=
				    STATUS_SUCCESS) {
					fprintf(stderr,
					    catgets(catfd, SET, 349,
					    "acs_response(qu_scr):failure "
					    "%s\n"), dl_acs_status(err));
					exit(1);
				}
				if (resp_seq != sequence) {
					fprintf(stderr,
					    catgets(catfd, SET, 348,
					    "acs_response(qu_scr): "
					    "wrong sequence.\n"));
					exit(1);
				}
			} while (type != RT_FINAL);

			qs_resp = (ACS_QUERY_SCR_RESPONSE *) buffer;
			if (qs_resp->count == 0) {
				fprintf(stderr,
				    catgets(catfd, SET, 344,
				    "acs_query_scratch: "
				    "No scratch volumes.\n"));
				exit(1);
			}
			if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {
				perror(catgets(catfd, SET, 1113,
				    "Unable to open command fifo:"));
				exit(1);
			}
			for (ii = 0, jj = (int)qs_resp->count;
			    ii < jj && pool_count > 0;
			    ii++) {
			/* N.B. Bad indentation to meet cstyle requirements */
			qs_status = &qs_resp->scr_status[ii];
			if (qs_status->status != STATUS_VOLUME_HOME) {
				fprintf(stderr,
				    catgets(catfd, SET, 2349,
				    "Skipping %s: not home\n"),
				    qs_status->vol_id);
				continue;
			}
			sequence++;
			memset(&vol_ids[0].endvol.external_label[0],
			    0, sizeof (VOLID));
			memset(&vol_ids[0].startvol.external_label[0],
			    0, sizeof (VOLID));
			strncpy(&vol_ids[0].startvol.external_label[0],
			    qs_status->vol_id.external_label,
			    sizeof (VOLID));
			strncpy(&vol_ids[0].endvol.external_label[0],
			    qs_status->vol_id.external_label,
			    sizeof (VOLID));
			if ((err = dl_acs_set_scratch(sequence, NO_LOCK_ID,
			    what_pool, vol_ids, FALSE, 1))) {
				fprintf(stderr,
				    catgets(catfd, SET, 354,
				    "acs_set_scratch failed: %s\n"),
				    dl_acs_status(err));
				exit(1);
			}
			do {
				if ((err = dl_acs_response(
				    -1, &resp_seq, &resp_req, &type,
				    buffer2)) != STATUS_SUCCESS) {
					fprintf(stderr,
					    catgets(catfd, SET, 351,
					    "acs_response(set_scr):"
					    "failure %s\n"),
					    dl_acs_status(err));
					exit(1);
				}
				if (resp_seq != sequence) {
					fprintf(stderr,
					    catgets(catfd, SET, 350,
					    "acs_response(set_scr): "
					    "wrong sequence.\n"));
					exit(1);
				}
			} while (type != RT_FINAL);

			ss_resp = (ACS_SET_SCRATCH_RESPONSE *) buffer2;
			if (ss_resp->set_scratch_status == STATUS_SUCCESS ||
			    ss_resp->set_scratch_status ==
			    STATUS_POOL_LOW_WATER) {
				if (ss_resp->vol_status[0] == STATUS_SUCCESS) {
					strncpy(cmd_block.vsn, ss_resp->
					    vol_id[0].external_label,
					    sizeof (vsn_t));
					cmd_block.cmd = CMD_FIFO_ADD_VSN;
					if (l_opt)
						fprintf(stdout, "%s\n",
						    cmd_block.vsn);
					write(fifo_fd, &cmd_block,
					    sizeof (sam_cmd_fifo_t));
					pool_count--;
				}
			} else {
				if (ss_resp->set_scratch_status !=
				    STATUS_VOLUME_ACCESS_DENIED) {
						fprintf(stderr,
						    "acs_set_scratch: %s.\n",
						    dl_acs_status(ss_resp->
						    set_scratch_status));
						exit(1);
					}
				}
			} /* end for */
			close(fifo_fd);
			if (pool_count == 0)
				exit(0);
		}
	}
#endif
        else {
		strncpy(cmd_block.vsn, volser, sizeof (vsn_t));
		if (audit_eod)
			cmd_block.flags |= CMD_ADD_VSN_AUDIT;

		if (strange)
			cmd_block.flags |= CMD_ADD_VSN_STRANGE;

		cmd_block.cmd = CMD_FIFO_ADD_VSN;
	}

	if ((fifo_fd = open(fifo_file, O_WRONLY)) < 0) {
		perror(catgets(catfd, SET, 1113,
		    "Unable to open command fifo:"));
		exit(1);
	}
	write(fifo_fd, &cmd_block, sizeof (sam_cmd_fifo_t));

	close(fifo_fd);
	return (0);
}


static void*
map_sym(void *handle, char *sym)
{
	void	   *ret;

	if ((ret = dlsym(handle, sym)) == NULL) {
		fprintf(stderr,
		    catgets(catfd, SET, 1048, "Error mapping symbol %s: %s\n"),
		    sym, dlerror());
		exit(1);
	}
	return (ret);
}


static void
Usage()
{
	fprintf(stderr,
	    catgets(catfd, SET, 13207,
	    "Usage: import [-v volser | -s pool -c num -l ] [-b barcode]"
	    "[-m type] [-e | -n] eq\n"));
	exit(1);
}
