/*
 * readshm
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
 *
 *    SAM-QFS_notice_end
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define	ROBOTS_MAIN
#define	MAIN
#define	NEWALARM

#define DEC_INIT
#include "sam/types.h"
#include "sam/custmsg.h"
#include "sam/param.h"
#include "aml/shm.h"
#include "aml/message.h"
#include "generic/generic.h"
#include "sam/devnm.h"
#include "sam/lib.h"
#include "sam/nl_samfs.h"
#include "robots.h"

/* function prototypes */

/* some globals */

int	number_robots, got_sigchld = FALSE;
pid_t	mypid;
char	*fifo_path;
message_request_t *message;
shm_alloc_t master_shm, preview_shm;

#define is_family(a)              (((a) & DT_CLASS_MASK) == DT_FAMILY_SET)
#define         ELEMENT_DESCRIPTOR_LENGTH   (48 + 4)


const char *states[] = {"ON", "NOALLOC", "RO", "IDLE", "UNAVAIL", "OFF", "DOWN"};

const char *disk_type[] = {"DISK",              /*    0x100  Disk storage */
							"DATA",             /* Disk storage for sm/lg daus */
							"META",				/* Disk storage for meta data */
							"RAID"};				/* Disk storage for raid data */

const char *tape_type[] = {"TAPE",              /*    0x200   Tape */
							"VIDEO_TAPE",       /* VHS Video tape */
							"SQUARE_TAPE",      /* Square tape (3480) */
							"EXABYTE_TAPE",     /* 8mm Exabyte tape */
							"LINEAR_TAPE", /* Digital linear tape */
							"DAT",         /* 4mm dat (Python) */
							"9490",   /* Square tape (9440) */
							"D3",    /* D3 */
							"notape", /* currently unused and available */
							"3590",   /* IBM 3590 */
							"3570",   /* IBM 3570 */
							"SONYDTF", /* Sony DTF 2120 */
							"SONYAIT", /* Sony AIT */
							"9840", 			/* STK 9840 */
							"FUJITSU_128",    /* Fujitsu Diana-4 128track drive */
							"EXABYTE_M2", /* 8mm Mammoth-2 Exabyte tape */
							"DT_9940",	/* STK 9940 */
							"IBM3580", /* IBM LTO 3580 */
							"SONYSAIT", /* SONY Super AIT */
							"3592",	/* IBM 3592 and TS1120 drives */
							"TITAN" }; /* STK Titanium drive */

int
main(int argc, char **argv)
{
	int		count, stk_found = 0;
	shm_ptr_tbl_t		*shm_ptr_tbl;
	dev_ent_t	*device_entry;
	dev_ptr_tbl_t *dev_ptr_tbl;
	library_t *library;
	robotic_device_t *rb;
	program_name = "readshm";

	program_name = "readshm";
	if ( argc < 3) {
		printf ("%s <master_id> <preview_id> <libeq>\n", *argv);
		return 1;
	}
		
	argv++;
	master_shm.shmid = atoi(*argv);
	argv++;
	preview_shm.shmid = atoi(*argv);

	mypid = getpid();
	if ((master_shm.shared_memory = shmat(master_shm.shmid, NULL, 0774)) ==
	    (void *) -1)
		exit(2);

	if ((preview_shm.shared_memory =
	    shmat(preview_shm.shmid, NULL, 0774)) ==
	    (void *) -1)
		exit(3);

	library = (library_t *)calloc(1, sizeof (library_t));
	if (argc == 4){
		argv++;
		library->eq = atoi(*argv);
	}

	shm_ptr_tbl = (shm_ptr_tbl_t *)master_shm.shared_memory;
        printf("master_shm.shared_memory: %llx\n", shm_ptr_tbl);
	device_entry = (dev_ent_t *)SHM_REF_ADDR(
		    ((shm_ptr_tbl_t *)master_shm.shared_memory)->first_dev);
        printf("first_dev: %llx\n\n", device_entry);
	printf("%8s %12s %49s %3s %9s %17s %5s %16s %8s %12s %s\n","set", "rdn", "device", "eq", "vendor", "product", "rev", "serial", "type", "space", "state");
	for (count = 0;
	    device_entry != NULL;
	    device_entry = (dev_ent_t *)SHM_REF_ADDR(device_entry->next))
	{
		const char* type;
		if (is_disk(device_entry->type)) 
			type = disk_type[device_entry->type-DT_DISK];
		else
			if (is_tape(device_entry->type))
				type = tape_type[device_entry->type-DT_TAPE];
			else
				if (is_robot(device_entry->type)){
					type = "ROBOT";
					if (library->eq == 0)
						library->eq = device_entry->eq;
				}
				else
					if (is_family(device_entry->type))
						type = "FAMILY";	
					else
						type = "other";
		

		if (TRUE || IS_ROBOT(device_entry) || device_entry->type == DT_PSEUDO_SC || device_entry->type == DT_PSEUDO_SS) {
			count++;
			/* stks need the ssi running */

			if (!stk_found && device_entry->type == DT_STKAPI) {
				stk_found = TRUE;
				count++;
			}
		}
<<<<<<< HEAD
		printf("%8s ",device_entry->set);
		printf("%12llx ",device_entry->st_rdev);
		printf("%49s ",device_entry->name);
		printf("%3d ",device_entry->eq);
		printf("%9s ",device_entry->vendor_id);
		printf("%17s ",device_entry->product_id);
		printf("%5s ",device_entry->revision);
		printf("%16s ",device_entry->serial);
		printf("%8s ",type);
		printf("%12lu ",device_entry->space);
		printf("%s\n",states[device_entry->state]);
=======
                int i;
 		printf("%8s ",device_entry->set);
 		printf("%12llx ",device_entry->st_rdev);
 		printf("%49s ",device_entry->name);
 		printf("%3d ",device_entry->eq);
 		printf("%9s ",device_entry->vendor_id);
 		printf("%17s ",device_entry->product_id);
 		printf("%5s ",device_entry->revision);
 		printf("%16s ",device_entry->serial);
 		printf("%8s ",type);
 		printf("%12lu ",device_entry->space);
 		printf("%s\n",states[device_entry->state]);
/*
			printf("%8s %12llx %49s %3d %9s %17s %5s %16s %8s %12lu %s\n", 
				device_entry->set,
				device_entry->st_rdev,
				device_entry->name, 
				device_entry->eq, 
				device_entry->vendor_id, 
				device_entry->product_id, 
				device_entry->revision, 
				device_entry->serial, 
				type, 
				device_entry->space,
				states[device_entry->state]);
*/
>>>>>>> 6a4028a5 (print device values in sepearate printf's)
	}
	printf("\nfound %d devices\n", count);

	dev_ptr_tbl = (dev_ptr_tbl_t *)SHM_REF_ADDR(
            ((shm_ptr_tbl_t *)master_shm.shared_memory)->dev_table);
        printf("dev_table: %llx\n", dev_ptr_tbl);

        /* LINTED pointer cast may result in improper alignment */
	library->un = (dev_ent_t *)SHM_REF_ADDR(
            dev_ptr_tbl->d_ent[library->eq]);
        printf("library dev: %llx\n", library->un);

	library->ele_dest_len = ELEMENT_DESCRIPTOR_LENGTH;

	rb = (robotic_device_t *)&library->un->dt;
	printf ("\nlibrary set %s, device %s, catalog %s, bits %x, %s\n", 
		library->un->set, 
		library->un->name, 
		library->un->dt.rb.name, 
		library->un->dt.rb.status.bits,
		library->un->dt.rb.status.b.barcodes == 1 ? "has barcode reader" : "");
	/* common_init(library->un); */

	fifo_path = strdup(SHM_REF_ADDR(shm_ptr_tbl->fifo_path));
	printf("fifo_path: %s\n", fifo_path);
        message = (message_request_t *)SHM_REF_ADDR(
                    ((shm_ptr_tbl_t *)master_shm.shared_memory)->scan_mess);
	free(library);
}
