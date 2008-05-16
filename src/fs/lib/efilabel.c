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

#pragma ident "$Revision: 1.13 $"

#include <sys/efi_partition.h>
#include <dlfcn.h>

/*
 * Globals used for calling the EFI library, when present.
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
