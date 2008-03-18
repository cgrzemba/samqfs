/*
 * license - manage license
 *
 *	Solaris 2.x Sun Storage & Archiving Management File System
 */

/*
 *    SAM-QFS_notice_begin
 *
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License")
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#pragma ident "$Revision: 1.19 $"

/*
 * NOTE NOTE NOTE NOTE NOTE NOTE  NOTE NOTE NOTE
 *
 * These files (check_license.c encrypt.c license.c) are intended to
 * be included with the pieces of samfs that need access to the license
 * data.  It is done this way for security reasons.  By including these
 * files, rather than compiling and linking, we can make all the routines
 * static, which makes them much more difficult for a hacker to find.
 *
 */

#include <ctype.h>
#include <sys/systeminfo.h>
#include <sam/names.h>
#include <sam/devinfo.h>
#ifdef _sparc_
#include <sys/openpromio.h>
#endif
#include <pub/version.h>

/* size recommended by sysinfo(2) */
#define	BUFFER_SIZE		257

static uint_t get_hostid();
static int read_licenses(sam_license_t_33**, sam_media_license_t_33**, int *);
static int verify_hostid(sam_license_t_33*, sam_media_license_t_33*, int);
static int verify_expiration(sam_license_t_33*);
static void lic_to_string(ulong_t *, char *);
static void string_to_lic(char *, ulong_t *);
static char *get_dev_name(int);
static char *get_media_name(int);

#include <license/encrypt.c>
#include <license/check_license.c>

static uint_t
get_hostid() {
	uint_t hostid;
	char si_data[120];

	(void) sysinfo(SI_HW_SERIAL, si_data, 20);
	hostid = strtoul(si_data, NULL, 10);

#if defined(lint)
/* Eliminate "warning: static unused" messages. */
(void) verify_hostid(NULL, NULL, 0);
(void) check_license(NULL, NULL, NULL);
(void) is_demo(NULL);
(void) is_trial(NULL);
(void) is_special(NULL);
(void) is_expiring(NULL);
(void) is_non_expiring(NULL);
(void) remote_sam_client_enabled(NULL);
(void) remote_sam_server_enabled(NULL);
(void) migkit_enabled(NULL);
(void) fast_fs_enabled(NULL);
(void) qfs_stand_alone_enabled(NULL);
(void) db_features_enabled(NULL);
(void) strange_tape_enabled(NULL);
(void) shared_san_enabled(NULL);
(void) segment_enabled(NULL);
(void) strange_tape_enabled(NULL);
(void) sharedfs_enabled(NULL);
#endif /* defined(lint) */

	return (hostid);
}

static int
read_licenses(
sam_license_t_33 **system_license,
sam_media_license_t_33 **media_licenses,
int *num_licenses)
{
	struct stat sbuf;
	int lsize;
	char *p;
	int nlines;
	char kbuf[64];
	sam_media_license_t_33 *mlp;
	ulong_t encrypted[4];
	uint_t	check_sum;
	int i;
	int license_fd = -1;
	char *lbuf = NULL;
	int r = RL_OK;

	*system_license = NULL;
	*media_licenses = NULL;
	if (stat(SAM_CONFIG_PATH"/"SAM_LICENSE_FILE, &sbuf) < 0) {
		r = RL_CANT_STAT;
		goto out;
	}
	if (sbuf.st_mtim.tv_sec > time(NULL)) {
		r = RL_MOD_TIME;
		goto out;
	}
	lsize = sbuf.st_size;
	if ((license_fd =
	    open(SAM_CONFIG_PATH"/"SAM_LICENSE_FILE, O_RDONLY)) < 0) {
		r = RL_CANT_OPEN;
		goto out;
	}
	lbuf = (char *)malloc_wait(lsize+1, 4, 0);
	if (read(license_fd, lbuf, lsize) != lsize) {
		r = RL_CANT_READ;
		goto out;
	}
	(void) close(license_fd);
	license_fd = -1;

	lbuf[lsize] = '\0';
	p = lbuf;
	nlines = 0;
	while (p) {
		p = strchr(p, '\n');
		if (p) {
			nlines++;
			while ('\n' == *p)
				p++;
		}
	}

	*system_license = (sam_license_t_33 *)
	    malloc_wait(sizeof (sam_license_t_33), 4, 0);
	if (nlines - 1 > 0) {
		*media_licenses = (sam_media_license_t_33 *)
		    malloc_wait(sizeof (sam_media_license_t_33) * (nlines-1),
		    4, 0);
	}
	memcpy(kbuf, lbuf, 32);
	string_to_lic(kbuf, encrypted);
	decrypt(encrypted, (ulong_t *)*system_license, crypt_33);
	CHECK_SUM(*system_license,
	    sizeof (sam_license_t_33) - sizeof (uint_t), check_sum);
	if (check_sum != (*system_license)->check_sum) {
		r = RL_LIC_DATA;
		goto out;
	}

	p = strchr(lbuf, '\n');
	p++;
	mlp = *media_licenses;
	for (i = 0; i < nlines-1; i++) {
		memcpy(kbuf, p, 32);
		string_to_lic(kbuf, encrypted);
		decrypt(encrypted, (ulong_t *)mlp, crypt_33);
		CHECK_SUM(mlp,
		    sizeof (sam_media_license_t_33) - sizeof (uint_t),
		    check_sum);
		if (mlp->check_sum != check_sum) {
			r = RL_LIC_DATA;
			goto out;
		}
		p = strchr(p, '\n');
		if (p == NULL)
			break;
		p++;
		mlp++;
	}
	*num_licenses = nlines - 1;

out:
	if (lbuf != NULL) {
		free(lbuf);
	}
	if (license_fd >= 0) {
		(void) close(license_fd);
	}
	if (r != RL_OK) {
		if (*system_license != NULL) {
			free(*system_license);
			*system_license = NULL;
		}
		if (*media_licenses != NULL) {
			free(*media_licenses);
			*media_licenses = NULL;
		}
	}
	return (r);
}

static int
verify_hostid(sam_license_t_33 *system_license,
		sam_media_license_t_33 *media_licenses,
		int nml) {
	int i;
	sam_media_license_t_33 *mlp = media_licenses;

	uint_t hostid = get_hostid();

	if (system_license->hostid != hostid) {
		return (VH_NO_MATCH);
	}
	for (i = 0; i < nml; i++) {
		if (mlp->hostid != hostid) {
			return (VH_NO_MATCH);
		}
		mlp++;
	}
	return (VH_OK);
}

static int
verify_expiration(sam_license_t_33 *system_license) {
	time_t now;
	time_t time_left;

	if ((system_license->license.lic_u.b.license_type == NON_EXPIRING) ||
		(system_license->license.lic_u.b.license_type == QFS_SPECIAL)) {
		return (VE_NO_EXPIRE);
	}

	now = time(NULL);

	if (now > system_license->exp_date) {
		return (VE_EXPIRED);
	}

	time_left = system_license->exp_date - now;
	return (time_left);
}

static char *
get_dev_name(int dev_type)  {
	sam_model_t *mp;

	mp = sam_model;
	while (mp->long_name) {
		if (mp->dt == dev_type) {
			return (mp->long_name);
		}
		mp++;
	}
	return ("unknown media library");
}


static void
lic_to_string(
ulong_t *lic,
char *string)
{
	sprintf(string, "%8.8lx%8.8lx%8.8lx%8.8lx",
	    lic[0], lic[1], lic[2], lic[3]);
}

static void
string_to_lic(char *string, ulong_t *lic) {
	int i1, i2;
	ulong_t tmp;

	for (i1 = 0; i1 < 4; i1++) {
		tmp = 0;
		for (i2 = 0; i2 < 2 * sizeof (ulong_t); i2++) {
			/*
			 * Convert string byte-by-byte to avoid swapping
			 * problems with int's on little-endian machines.
			 */
			tmp <<= 4;
			if (isxdigit(*string)) {
				if (*string <= '9') {
					tmp |= *string & 0x0f;
				} else {
					tmp |= ((*string - 1) & 0x0f) + 0x0a;
				}
				string++;
			} else {
				string++;
				break;
			}
		}
		if (tmp == ULONG_MAX) {
			*lic = 0;
		} else {
			*lic = tmp;
		}
		lic++;
	}
}

static char *
get_media_name(int type) {
	dev_nm_t *media_nm = dev_nm2mt;

	while (media_nm->nm) {
		if (media_nm->dt == type) {
			return (media_nm->nm);
		}
		media_nm++;
	}
	return ("unknown media");
}
