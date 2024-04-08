/*
 * efilabel.c - access to a disk's EFI label, when OS support is present
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
 * or https://illumos.org/license/CDDL.
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

#pragma ident "$Revision: 1.14 $"

#include <sys/efi_partition.h>
#include <sys/vtoc.h>
#include <sys/dkio.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


/*
 * Globals used for calling the EFI library, when present.
 */

/*
* normally this is the interface for dynloaded libefi.so
* but efi_alloc_and_read do not work on zvol so here some stecial stuff added to workaround this 
* https://www.illumos.org/issues/12339
*
*/

static int efi_library_checked = 0;
static int efi_library_present = 0;
static void *efi_library_handle = NULL;

typedef int (*efi_alloc_and_read_func_t) (int, struct dk_gpt **);
typedef void (*efi_free_func_t) (struct dk_gpt *);

static efi_alloc_and_read_func_t efi_alloc_and_read_func = NULL;
static efi_free_func_t efi_free_func = NULL;

/*
 * -----  is_efi_present
 *  Test whether the EFI libraries exist on the running system.  Also
 *  prepare global variables used to call into the library.
 */

int
is_efi_present(void)
{
	if (efi_library_checked)
		return (efi_library_present);

	efi_library_checked = 1;
	efi_library_present = 0;

	efi_library_handle = (void *) dlopen("libefi.so", RTLD_LAZY);
	if (efi_library_handle == NULL)
		return (0);

	efi_alloc_and_read_func =
	    (efi_alloc_and_read_func_t)dlsym(efi_library_handle,
	    "efi_alloc_and_read");
	if (efi_alloc_and_read_func == NULL)
		return (0);

	efi_free_func =
	    (efi_free_func_t)dlsym(efi_library_handle, "efi_free");
	if (efi_free_func == NULL)
		return (0);

	efi_library_present = 1;

	return (1);
}

/*
 * -----  call_efi_alloc_and_read
 *  Call efi_alloc_and_read().
 *  Caller is responsible for ensuring that is_efi_present() has returned true.
 */

int
call_efi_alloc_and_read(int fd, struct dk_gpt **efi_vtoc)
{
	return ((*efi_alloc_and_read_func) (fd, efi_vtoc));
}

/*
 * -----  call_efi_free
 *  Call efi_free().
 *  Caller is responsible for ensuring that is_efi_present() has returned true.
 */

void
call_efi_free(struct dk_gpt *efi_vtoc)
{
	(*efi_free_func) (efi_vtoc);
}


#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/dkio.h>
#include <sys/vtoc.h>
#include <sys/mnttab.h>
#include <sys/param.h>
#include <sys/efi_partition.h>
#include <sys/crc32.h>
#include <errno.h>
#include <string.h>
#include <strings.h>

/*
 * the number of blocks the EFI label takes up (round up to nearest
 * block)
 */
#define	NBLOCKS(p, l)	(1 + ((((p) * (int)sizeof (efi_gpe_t))  + \
((l) - 1)) / (l)))

static struct uuid_to_ptag {
	struct uuid	uuid;
	ushort_t	p_tag;
} conversion_array[] = {
	{ EFI_UNUSED, V_UNASSIGNED },
	{ EFI_BOOT, V_BOOT },
	{ EFI_ROOT, V_ROOT },
	{ EFI_SWAP, V_SWAP },
	{ EFI_USR, V_USR },
	{ EFI_BACKUP, V_BACKUP },
	{ EFI_VAR, V_VAR },
	{ EFI_HOME, V_HOME },
	{ EFI_ALTSCTR, V_ALTSCTR },
	{ EFI_RESERVED, V_RESERVED },
	{ EFI_SYSTEM, V_SYSTEM },		/* V_SYSTEM is 0xc */
	{ EFI_LEGACY_MBR, 0x10 },
	{ EFI_SYMC_PUB, 0x11 },
	{ EFI_SYMC_CDS, 0x12 },
	{ EFI_MSFT_RESV, 0x13 },
	{ EFI_DELL_BASIC, 0x14 },
	{ EFI_DELL_RAID, 0x15 },
	{ EFI_DELL_SWAP, 0x16 },
	{ EFI_DELL_LVM, 0x17 },
	{ EFI_DELL_RESV, 0x19 },
	{ EFI_AAPL_HFS, 0x1a },
	{ EFI_AAPL_UFS, 0x1b },
	{ EFI_AAPL_ZFS, 0x1c },
	{ EFI_AAPL_APFS, 0x1d },
	{ EFI_BIOS_BOOT, V_BIOS_BOOT },		/* V_BIOS_BOOT is 0x18 */
	{ EFI_FREEBSD_BOOT,  V_FREEBSD_BOOT },
	{ EFI_FREEBSD_SWAP, V_FREEBSD_SWAP },
	{ EFI_FREEBSD_UFS, V_FREEBSD_UFS },
	{ EFI_FREEBSD_VINUM, V_FREEBSD_VINUM },
	{ EFI_FREEBSD_ZFS, V_FREEBSD_ZFS },
	{ EFI_FREEBSD_NANDFS, V_FREEBSD_NANDFS }
};

int efi_debug = 1;

static unsigned int crc32_tab[] = { CRC32_TABLE };
unsigned int efi_crc32(const unsigned char *s, unsigned int len)
{
	unsigned int crc32val;

	CRC32(crc32val, s, len, -1U, crc32_tab);

	return (crc32val ^ -1U);
}

static int
check_label(int fd, dk_efi_t *dk_ioc)
{
	efi_gpt_t		*efi;
	uint_t			crc;

	if (ioctl(fd, DKIOCGETEFI, dk_ioc) == -1) {
		switch (errno) {
		case EIO:
			return (VT_EIO);
		default:
			return (VT_ERROR);
		}
	}
	efi = dk_ioc->dki_data;
	if (efi->efi_gpt_Signature != LE_64(EFI_SIGNATURE)) {
		if (efi_debug)
			(void) fprintf(stderr,
			    "Bad EFI signature: 0x%llx != 0x%llx\n",
			    (long long)efi->efi_gpt_Signature,
			    (long long)LE_64(EFI_SIGNATURE));
		return (VT_EINVAL);
	}

	/*
	 * check CRC of the header; the size of the header should
	 * never be larger than one block
	 */
	crc = efi->efi_gpt_HeaderCRC32;
	efi->efi_gpt_HeaderCRC32 = 0;

	if (((len_t)LE_32(efi->efi_gpt_HeaderSize) > dk_ioc->dki_length) ||
	    crc != LE_32(efi_crc32((unsigned char *)efi, LE_32(efi->efi_gpt_HeaderSize)))) {
		if (efi_debug)
			(void) fprintf(stderr,
			    "Bad EFI CRC: 0x%x != 0x%x\n",
			    crc, LE_32(efi_crc32((unsigned char *)efi, LE_32(efi->efi_gpt_HeaderSize))));
		return (VT_EINVAL);
	}

	return (0);
}

int
efi_read(int fd)
{
	int			i, j, size, length;
	int			label_len;
	int			rval = 0;
	struct dk_minfo		disk_info;
	dk_efi_t		dk_ioc;
	efi_gpt_t		*efi;
	efi_gpe_t		*efi_parts;
	uint32_t		user_length;
    uint32_t        nparts;
    struct dk_gpt   *vtoc;

    nparts = EFI_MIN_ARRAY_SIZE / sizeof (efi_gpe_t);
	length = (int) sizeof (struct dk_gpt) + (int) sizeof (struct dk_part) * (nparts - 1);
	if ((vtoc = calloc(1, length)) == NULL)
		return (VT_ERROR);
    vtoc->efi_nparts = nparts; /* 128 */

	/* get the LBA size */
	if (ioctl(fd, DKIOCGMEDIAINFO, (caddr_t)&disk_info) == -1) {
		if (efi_debug) {
			(void) fprintf(stderr,
			    "assuming LBA 512 bytes %d\n",
			    errno);
		}
		disk_info.dki_lbsize = DEV_BSIZE;
	}
	if (disk_info.dki_lbsize == 0) {
		if (efi_debug) {
			(void) fprintf(stderr,
			    "efi_read: assuming LBA 512 bytes\n");
		}
		disk_info.dki_lbsize = DEV_BSIZE;
	}
	/*
	 * Read the EFI GPT to figure out how many partitions we need
	 * to deal with.
	 */
	dk_ioc.dki_lba = 1;
	if (NBLOCKS(vtoc->efi_nparts, disk_info.dki_lbsize) < 34) {
		label_len = EFI_MIN_ARRAY_SIZE + disk_info.dki_lbsize;
	} else {
		label_len = vtoc->efi_nparts * (int) sizeof (efi_gpe_t) +
		    disk_info.dki_lbsize;
		if (label_len % disk_info.dki_lbsize) {
			/* pad to physical sector size */
			label_len += disk_info.dki_lbsize;
			label_len &= ~(disk_info.dki_lbsize - 1);
		}
	}
    /* label_len = 16896 */
    void * data = calloc(1, label_len);
	if (data == NULL)
		return (VT_ERROR);
    efi = dk_ioc.dki_data = data;
    dk_ioc.dki_data_64 = (uint64_t)(uintptr_t) data;
	dk_ioc.dki_length = disk_info.dki_lbsize;
	user_length = vtoc->efi_nparts;
	if ((rval = check_label(fd, &dk_ioc)) == VT_EINVAL) {
		if ((efi_debug))
				(void) fprintf(stderr, "DKIOCGETEFI ioctl error %s\n", strerror(errno));
		
	} else if (rval == 0) {

		dk_ioc.dki_lba = LE_64(efi->efi_gpt_PartitionEntryLBA);
		/* LINTED */
		dk_ioc.dki_data = (efi_gpt_t *)((char *)dk_ioc.dki_data
		    + disk_info.dki_lbsize);
		dk_ioc.dki_length = label_len - disk_info.dki_lbsize;
		rval = ioctl(fd, DKIOCGETEFI, &dk_ioc);

	}

	if (rval < 0) {
		free(efi);
		return (rval);
	}

	/* LINTED -- always longlong aligned */
	efi_parts = (efi_gpe_t *)(((char *)efi) + disk_info.dki_lbsize);

	/*
	 * Assemble this into a "dk_gpt" struct for easier
	 * digestibility by applications.
	 */
	vtoc->efi_version = LE_32(efi->efi_gpt_Revision);
	vtoc->efi_nparts = LE_32(efi->efi_gpt_NumberOfPartitionEntries);
	vtoc->efi_part_size = LE_32(efi->efi_gpt_SizeOfPartitionEntry);
	vtoc->efi_lbasize = disk_info.dki_lbsize;
	vtoc->efi_last_lba = disk_info.dki_capacity - 1;
	vtoc->efi_first_u_lba = LE_64(efi->efi_gpt_FirstUsableLBA);
	vtoc->efi_last_u_lba = LE_64(efi->efi_gpt_LastUsableLBA);
	vtoc->efi_altern_lba = LE_64(efi->efi_gpt_AlternateLBA);
	UUID_LE_CONVERT(vtoc->efi_disk_uguid, efi->efi_gpt_DiskGUID);

	/*
	 * If the array the user passed in is too small, set the length
	 * to what it needs to be and return
	 */
	if (user_length < vtoc->efi_nparts) {
		return (VT_EINVAL);
	}

	for (i = 0; i < vtoc->efi_nparts; i++) {

		UUID_LE_CONVERT(vtoc->efi_parts[i].p_guid,
		    efi_parts[i].efi_gpe_PartitionTypeGUID);

		for (j = 0;
		    j < sizeof (conversion_array)
		    / sizeof (struct uuid_to_ptag); j++) {

			if (bcmp(&vtoc->efi_parts[i].p_guid,
			    &conversion_array[j].uuid,
			    sizeof (struct uuid)) == 0) {
				vtoc->efi_parts[i].p_tag =
				    conversion_array[j].p_tag;
				break;
			}
		}
		if (vtoc->efi_parts[i].p_tag == V_UNASSIGNED)
			continue;
		vtoc->efi_parts[i].p_flag =
		    LE_16(efi_parts[i].efi_gpe_Attributes.PartitionAttrs);
		vtoc->efi_parts[i].p_start =
		    LE_64(efi_parts[i].efi_gpe_StartingLBA);
		vtoc->efi_parts[i].p_size =
		    LE_64(efi_parts[i].efi_gpe_EndingLBA) -
		    vtoc->efi_parts[i].p_start + 1;
		for (j = 0; j < EFI_PART_NAME_LEN; j++) {
			vtoc->efi_parts[i].p_name[j] =
			    (uchar_t)LE_16(
			    efi_parts[i].efi_gpe_PartitionName[j]);
		}

		UUID_LE_CONVERT(vtoc->efi_parts[i].p_uguid,
		    efi_parts[i].efi_gpe_UniquePartitionGUID);
	}
	free(efi);
    size = vtoc->efi_parts[i-1].p_size;
    free(vtoc);
	return (size);
}
