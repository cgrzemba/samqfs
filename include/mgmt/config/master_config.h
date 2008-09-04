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

#ifndef	_MASTER_CONFIG_H
#define	_MASTER_CONFIG_H

#pragma ident   "$Revision: 1.19 $"


/*
 * master_config.h
 * Used to read the device and family set configuration and
 * can be used directly to add to the configuration by inserting
 * base_dev_t structures into the mcf_cfg_t and calling write_mcf_cfg.
 *
 * The structures in this header map to the mcf file.
 */

#include <sys/types.h>

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/device.h"

#include "sam/types.h"

#define	EQ_INTERVAL	100
#define	EQ_ROUND	10

typedef struct mcf_cfg {
	sqm_lst_t	*mcf_devs;
	time_t	read_time;
} mcf_cfg_t;


/*
 * Given an equipment ordinal, get its base_dev_t structure.  Sets mcf_dev
 * to NULL and returns -1 if no device with the ordinal is found.
 */
int get_device_by_eq(const mcf_cfg_t *cfg, const equ_t eq,
	base_dev_t **ret_val);


/*
 *  Given a family set name, get a list of devices which belong to
 *  that family set.
 */
int get_family_set_devs(const mcf_cfg_t *cfg, const char *family_set,
	sqm_lst_t **mcf_dev_list);


/*
 * Given an equipment identifier, get its base_dev_t structure.  Sets mcf_dev
 * to NULL and returns -1 if no device with the eq_identifier is found.
 */
int get_mcf_dev(const mcf_cfg_t *cfg, const char *eq_identifier,
	base_dev_t **mcf_dev);


/*
 * A list of base_dev_t under family_set and state = DEV_ON e.g.
 * If family_set is non-null and does not exist -1 will be returned and
 *    samerrno will be set to SE_FAMILY_SET_DOES_NOT_EXIST.
 * If family_set is null all devices with the indicated state will be returned.
 */
int get_mcf_dev_list(const mcf_cfg_t *cfg, const char *family_set,
	const dstate_t state, sqm_lst_t **devs);

/*
 * Get the next count available eq_ord numbers.
 */
int get_available_eq_ord(const mcf_cfg_t *cfg, sqm_lst_t **ret_vals, int count);

/*
 * Get a list of devices of the same type, e.g. g10 or mm
 * If family_set is non-null and does not exist -1 will be returned and
 *    samerrno will be set to SE_FAMILY_SET_DOES_NOT_EXIST.
 * If family_set is null all devices with the indicated type will be returned.
 * If an invalid eq_type is specified samerrno is set to SE_INVALID_EQ_TYPE.
 */
int get_devices_by_type(const mcf_cfg_t *cfg, const char *family_set,
	const char *eq_type, sqm_lst_t **mcf_dev_list);


/*
 * A list of family set names.
 */
int get_family_set_names(const mcf_cfg_t *cfg, sqm_lst_t **family_sets);


/*
 * A list of family set names which are only for FS.
 */
int get_fs_family_set_names(const mcf_cfg_t *cfg, sqm_lst_t **fs);


/*
 * After getting license type, check if the eq_type is OK under sam-fs, qfs,
 * or sam-qfs. For example, g* cannot be used in sam-fs.
 */
int check_eq_type(const mcf_cfg_t *cfg, const char *eq_type,
		const char *licenese_fs, boolean_t *retval);


/*
 * Check eq_type under a family set. In qfs, md, mr, and g*
 * cannot be mixed.
 */
int check_fs_eq_types(const mcf_cfg_t *cfg, const char *family_set,
		boolean_t *eq_valid);


/*
 * returns a list of device types that are valid for the named file system.
 * takes into account the existing devices in the fs.
 */
int get_valid_eq_types(const mcf_cfg_t *cfg, const char *family_set,
		sqm_lst_t **valid_eq_types);


/*
 * Read the mcf configuration from the defualt location and
 * build the mcf_cfg_t structure.
 */
int read_mcf_cfg(mcf_cfg_t **mcf);


/*
 * Write the mcf configuration to the default location.
 *
 * If force is false and the configuration has been modified since
 * the mcf_cfg_t struture was read in this function will
 * set errno and return -1
 *
 * If force is true the mcf configuration will be written without
 * regard to its modification time.
 */
int write_mcf_cfg(ctx_t *ctx, mcf_cfg_t *mcf, boolean_t force);


/*
 * dump the configuration to the specified location.  If a file exists at
 * the specified location it will be overwritten.
 */
int dump_mcf(const mcf_cfg_t *mcf, const char *location);


/*
 * check the mcf_cfg_t for correctness.
 */
int verify_mcf_cfg(mcf_cfg_t *mcf);



/*
 * returns count striped group ids that are free for the named fs.
 * if the fs does not exist the first available striped group name will
 * be returned so g0. Does not return an error if none are available. This
 * condition must be externally checked and would be indicated by a return
 * list length of 0.
 */
int
get_available_striped_group_id(mcf_cfg_t *mcf, uname_t fs_name,
	sqm_lst_t **ret_list, int count, boolean_t object_group);


/*
 * nm_to_dtclass.c  - Check device mnemonic to be legal
 *
 * Description:
 *	Convert two character device mnemonic to device type.
 *	If the device is any of the scsi robots or optical devices,
 *	return the generic type for scsi robots or optical. (rb or od)
 *	If the device is a scsi tape and uses the standard st device
 *	driver, return the device type for standard tape. (tp)
 *
 * On entry:
 *	nm  = The mnemonic name for the device.
 *
 * Returns:
 *	The device type or 0 if the mnemonic is not recognized.
 *
 */
dtype_t nm_to_dtclass(char *);


/*
 * get a list of parsing_error_t structs from the most recent mcf parsing.
 */
int
get_master_config_parsing_errors(sqm_lst_t **l);


/*
 * free the mcf_cfg_t and all of its sub-structures.
 */
void free_mcf_cfg(mcf_cfg_t *mcf);


/*
 * function to copy a base_dev_t struct
 */
int dev_cpy(base_dev_t *dst, base_dev_t *src);


/*
 * Add devices to the mcf for the named family set.
 * Post conditions:
 * 1. Family set comment added includes current date.
 * 2. New devs added in order.
 * 3. mcf written and verified.
 * 4. Backup copy of pre-existing mcf has been made.
 * 5. samd config has NOT been called.
 */
int add_family_set(ctx_t *c, sqm_lst_t *devs);


/*
 * Append devices to the named family set.
 * Post conditions:
 * 1. Family set comment extended to include current date.
 * 2. New devs appended in order.
 * 3. mcf written and verified.
 * 4. Backup copy of pre-existing mcf has been made.
 * 5. samd config has NOT been called.
 */
int append_to_family_set(ctx_t *c, char *set_name, sqm_lst_t *base_devs);


/*
 * Remove the named family set from the mcf file.
 * When this function returns without error the following will be true:
 * 1. The family set devices will be gone from the mcf.
 * 2. Any fam-set-eq: comments will be gone.
 * 3. sam-fsd will have executed cleanly indicating no errors.
 * 4. Backup copy of pre-existing mcf has been made.
 * 5. samd config will NOT have been called.
 */
int remove_family_set(ctx_t *c, char *name);


/*
 * parse the mcf file. - function in parse_mcf.c
 */
int
parse_mcf(
	char *mcf_file_name,	/* file to parse */
	mcf_cfg_t **ret_val);	/* malloced return value */


#endif /* _MASTER_CONFIG_H */
