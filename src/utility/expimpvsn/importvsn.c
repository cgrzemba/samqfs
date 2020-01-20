/* from remote_sam/server/vsn_list.c */
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
static char *_SrcFile = __FILE__; /* Using __FILE__ makes duplicate strings */

#include <stdio.h>
#include <thread.h>
#include <synch.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <regex.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "sam/types.h"
#include "sam/param.h"
#include "aml/device.h"
#include "aml/shm.h"
#include "sam/defaults.h"
#include "aml/remote.h"
#include "aml/proto.h"
#include "sam/lib.h"
#include "aml/catalog.h"
#include "aml/catlib.h"
#include "sam/sam_malloc.h"
#include "sam/sam_trace.h"
#include "sam/custmsg.h"

#include "server.h"

shm_alloc_t master_shm;

/* copy content of ce to found->entries[] */
int
add_vsn_entry(
	rmt_sam_client_t *client,
	rmt_entries_found_t *found,
	struct CatalogEntry *ce,
	uint_t flags)
{
	int rval;
	rmt_sam_vsn_entry_t *target;

	target = &found->entries[found->count++];

	memset(target, 0, sizeof (*target));
	memcpy(&target->ce, ce, sizeof (struct CatalogEntry));

	target->upd_flags = flags;

	Trace(TR_MISC, "[%s] Update vsn '%s' upd flags: %d",
	    TrNullString(client->host_name),
	    TrNullString(target->ce.CeVsn), target->upd_flags);
	Trace(TR_MISC, "\tstatus: 0x%x capacity: %lld ptoc: %lld",
	    target->ce.CeStatus, target->ce.CeCapacity, target->ce.m.CePtocFwa);

	Trace(TR_DEBUG, "\tmedia: '%s' slot: %d part: %d",
	    TrNullString(target->ce.CeMtype),
	    target->ce.CeSlot, target->ce.CePart);
	Trace(TR_DEBUG, "\tspace: %lld block size: %d",
	    target->ce.CeSpace, target->ce.CeBlockSize);
	Trace(TR_DEBUG, "\tlabel time: %d mod time: %d mount time: %d",
	    target->ce.CeLabelTime, target->ce.CeModTime,
	    target->ce.CeMountTime);
	Trace(TR_DEBUG, "\tbar code: '%s'",
	    TrNullString(target->ce.CeBarCode));

	rval = 0;
	if (found->count == RMT_SEND_VSN_COUNT) {
		rval = flush_list(client, found);
	}

	return (rval);
}

/* read memory mapped catalog file and copies searched vsn to found->entries */
void
imp_vsn_list(
        rmt_vsn_equ_list_t *next_equ,
        rmt_sam_client_t *client)
{
        int i;
        int n_entries;
        struct CatalogEntry *list;
        rmt_entries_found_t *found;
        dev_ent_t *this_un;

        this_un = next_equ->un;
        Trace(TR_MISC, "[%s] Import VSN list eq: %d",
            TrNullString(client->host_name), this_un->eq);

        (void) CatalogSync();
        list = CatalogGetEntriesByLibrary(this_un->eq, &n_entries);
        

        (void) CatalogSync();
}

static void *
ShmatSamfs(
	int	mode)	/* O_RDONLY = read only, read/write otherwise */
{
    shm_ptr_tbl_t *shm_ptr_tbl;
	/*
 	 * Access SAM-FS shared memory segment.
	 */
	if ((master_shm.shmid = shmget(SHM_MASTER_KEY, 0, 0)) < 0) {
		return (NULL);
	}
	mode = (O_RDONLY == mode) ? SHM_RDONLY : 0;
	master_shm.shared_memory = shmat(master_shm.shmid, NULL, mode);
	if (master_shm.shared_memory == (void *)-1) {
		return (NULL);
	}
	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
	if (strcmp(shm_ptr_tbl->shm_block.segment_name,
	    SAM_SEGMENT_NAME) != 0) {
		errno = ENOCSI;
		return (NULL);
	}
	return (shm_ptr_tbl);
}

#define USEAGE  "usage: %s [-c <lib_eq>] <vsn-file>\n\timport vsn from file\n" 

int 
main(int argc, char *argv[]) 
{
    shm_ptr_tbl_t *shm_ptr_tbl;
    dev_ent_t un;

    rmt_vsn_equ_list_t equ;
    char hostname[MAXHOSTNAMELEN] = "unstable11s";
    media_t  media = 529; /* "li" */
    rmt_sam_client_t client;
    regex_t re;
    int c, index;
    equ_t cat_eq = 40;
    char *vsn_pat;
    
    opterr = 0;

    while ((c = getopt (argc, argv, "c:h")) != -1)
    switch (c)
    {
        case 'c':
        cat_eq = atoi(optarg);
        break;
      case 'h':
      case '?':
          fprintf (stderr, USEAGE, argv[0]);
          exit(1);
      default:
        abort ();
    }
    if (argv[optind] == NULL){
        fprintf (stderr, USEAGE, argv[0]);
        exit(1);
    }
    vsn_pat = argv[optind];
    gethostname(hostname, sizeof(hostname));

    CustmsgInit(1, NULL);
    /* fprintf (stderr, "my catalog version: %d\n", CF_VERSION);  */

    /*
     * Prepare tracing.
     */
    TraceInit(program_name, TI_rmtserver | TR_MPLOCK);
    Trace(TR_PROC, "prog started");

    /*
    shm_ptr_tbl = ShmatSamfs(O_RDONLY);
    if (master_shm.shared_memory == (void *) -1) {
            SysError(HERE, "Unable to attach master shared memory segment");
            exit(1);
    }
   */
   
    /* gets the Catalogs via CSR_GetInfo from sam_catservd */
    if (CatalogInit("SamApi") == -1) {
            LibFatal(CatalogInit, "");
    }
    /* catalogs now available in Catalogs[0..] */

    un.eq = cat_eq;
    equ.un = &un;
    client.first_equ = &equ;
    client.host_name = hostname;
    
    imp_vsn_list(&equ, &client);
}
