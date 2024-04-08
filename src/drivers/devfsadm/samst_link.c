/*
 * samst_link.c
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

#pragma ident	"@(#)samst_link.c	1.7	99/09/23 SMI"

#include <devfsadm.h>
#include <driver/samst_def.h>
#include <stdio.h>
#include <strings.h>
#include <stdlib.h>
#include <limits.h>

#define	SAMST_SUBPATH_MAX 30
#define	RM_STALE 0x01
#define	SAMST_LINK_RE	"^samst/c[0-9]+(t[0-9]+)?d[0-9]+$"
#define	SAMST_LINK_TO_UPPER(ch)\
	(((ch) >= 'a' && (ch) <= 'z') ? (ch - 'a' + 'A') : ch)

static int samst_callback_chan(di_minor_t minor, di_node_t node);
static int samst_callback_nchan(di_minor_t minor, di_node_t node);
static int samst_callback_wwn(di_minor_t minor, di_node_t node);
static int samst_callback_fabric(di_minor_t minor, di_node_t node);
static void samst_common(di_minor_t minor, di_node_t node, char *samst,
				int flags);
static char *samstctrl(di_node_t node, di_minor_t minor);
extern void rm_link_from_cache(char *devlink, char *physpath);


static devfsadm_create_t samst_cbt[] = {
	{ "samst", "ddi_samst", NULL,
	    TYPE_EXACT, ILEVEL_0, samst_callback_nchan
	},
	{ "samst", DDI_NT_SAMST_CHAN, NULL,
	    TYPE_EXACT, ILEVEL_0, samst_callback_chan
	},
	{ "samst", DDI_NT_SAMST_FABRIC, NULL,
		TYPE_EXACT, ILEVEL_0, samst_callback_fabric
	},
	{ "samst", DDI_NT_SAMST_WWN, NULL,
	    TYPE_EXACT, ILEVEL_0, samst_callback_wwn
	},
};

DEVFSADM_CREATE_INIT_V0(samst_cbt);

/*
 * HOT auto cleanup of samst not desired.
 */
static devfsadm_remove_t samst_remove_cbt[] = {
	{ "samst", SAMST_LINK_RE, RM_POST,
		ILEVEL_0, devfsadm_rm_all
	}
};

DEVFSADM_REMOVE_INIT_V0(samst_remove_cbt);

static int
samst_callback_chan(di_minor_t minor, di_node_t node)
{
	char *addr;
	char samst[20];
	uint_t targ;
	uint_t lun;

	addr = di_bus_addr(node);
	(void) sscanf(addr, "%X,%X", &targ, &lun);
	(void) sprintf(samst, "t%du%d", targ, lun);
	samst_common(minor, node, samst, 0);
	return (DEVFSADM_CONTINUE);

}

static int
samst_callback_nchan(di_minor_t minor, di_node_t node)
{
	char *addr;
	char samst[10];
	uint_t lun;

	addr = di_bus_addr(node);
	(void) sscanf(addr, "%X", &lun);
	(void) sprintf(samst, "u%d", lun);
	samst_common(minor, node, samst, 0);
	return (DEVFSADM_CONTINUE);

}

static int
samst_callback_wwn(di_minor_t minor, di_node_t node)
{
	char samst[10];
	int lun;
	int targ;
	int *intp;

	if (di_prop_lookup_ints(DDI_DEV_T_ANY, node,
	    "target", &intp) <= 0) {
		return (DEVFSADM_CONTINUE);
	}
	targ = *intp;
	if (di_prop_lookup_ints(DDI_DEV_T_ANY, node,
	    "lun", &intp) <= 0) {
		lun = 0;
	} else {
		lun = *intp;
	}
	(void) sprintf(samst, "t%du%d", targ, lun);

	samst_common(minor, node, samst, RM_STALE);

	return (DEVFSADM_CONTINUE);
}

static int
samst_callback_fabric(di_minor_t minor, di_node_t node)
{
	char samst[25];
	int lun;
	int count;
	int *intp;
	uchar_t *str;
	uchar_t *wwn;
	uchar_t ascii_wwn[17];

	if (di_prop_lookup_bytes(DDI_DEV_T_ANY, node, "port-wwn", &wwn) <= 0) {
		return (DEVFSADM_CONTINUE);
	}

	if (di_prop_lookup_ints(DDI_DEV_T_ANY, node, "lun", &intp) <= 0) {
		lun = 0;
	} else {
		lun = *intp;
	}

	for (count = 0, str = ascii_wwn; count < 8; count++, str += 2) {
		(void) sprintf((caddr_t)str, "%02x", wwn[count]);
	}
	*str = '\0';

	for (str = ascii_wwn; *str != '\0'; str++) {
		*str = SAMST_LINK_TO_UPPER(*str);
	}

	(void) sprintf(samst, "t%su%d", ascii_wwn, lun);

	samst_common(minor, node, samst, RM_STALE);

	return (DEVFSADM_CONTINUE);
}

/*
 * This function is called for every samst minor node.
 * Calls enumerate to assign a logical controller number, and
 * then devfsadm_mklink to make the link.
 */
static void
samst_common(di_minor_t minor, di_node_t node, char *samst, int flags)
{
	char l_path[PATH_MAX + 1];
	char stale_re[SAMST_SUBPATH_MAX];
	char *dir;
	char slice[4];
	char *mn;
	char *ctrl;

	if (strstr(mn = di_minor_name(minor), ",raw")) {
		dir = "samst";
	} else {
		return;
	}
	if (NULL == (ctrl = samstctrl(node, minor)))
		return;

	(void) strcpy(l_path, dir);
	(void) strcat(l_path, "/c");
	(void) strcat(l_path, ctrl);
	(void) strcat(l_path, samst);

	(void) devfsadm_mklink(l_path, node, minor, 0);

	if ((flags & RM_STALE) == RM_STALE) {
		(void) strcpy(stale_re, "^");
		(void) strcat(stale_re, dir);
		(void) strcat(stale_re, "/c");
		(void) strcat(stale_re, ctrl);
		(void) strcat(stale_re, "t[0-9]+u[0-9]+$");
		/*
		 * optimizations are made inside of devfsadm_rm_stale_links
		 * instead of before calling the function, as it always
		 * needs to add the valid link to the cache.
		 */
		devfsadm_rm_stale_links(stale_re, l_path, node, minor);
	}

	free(ctrl);
}


/* index of enumeration rule applicable to this module */
#define	RULE_INDEX	0

static char *
samstctrl(di_node_t node, di_minor_t minor)
{
	char path[PATH_MAX + 1];
	char *devfspath;
	char *buf, *mn;

	devfsadm_enumerate_t rules[3] = {
	    {"^samst$/^c([0-9]+)", 1, MATCH_PARENT},
	    {"^cfg$/^c([0-9]+)$", 1, MATCH_ADDR},
	    {"^scsi$/^.+$/^c([0-9]+)", 1, MATCH_PARENT}
	};

	mn = di_minor_name(minor);

	if ((devfspath = di_devfs_path(node)) == NULL) {
		return (NULL);
	}
	(void) strcpy(path, devfspath);
	(void) strcat(path, ":");
	(void) strcat(path, mn);
	di_devfs_path_free(devfspath);

	/*
	 * Use controller component of samst path
	 */
	if (devfsadm_enumerate_int(path, RULE_INDEX, &buf, rules, 3)) {
		/*
		 * Maybe failed because there are multiple logical controller
		 * numbers for a single physical controller.  If we use node
		 * name also in the match it should fix this and only find one
		 * logical controller.
		 * NOTE: Rules for controllers are not changed, as there is
		 * no unique controller number for them in this case.
		 *
		 * MATCH_UNCACHED flag is private to the "disks" and "sgen"
		 * modules. NOT to be used by other modules.
		 */
		rules[0].flags = MATCH_NODE | MATCH_UNCACHED; /* samsts */
		rules[2].flags = MATCH_NODE | MATCH_UNCACHED; /* generic scsi */
		if (devfsadm_enumerate_int(path, RULE_INDEX, &buf, rules, 3)) {
			return (NULL);
		}
	}

	return (buf);
}
