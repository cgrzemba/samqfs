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
#ifndef	_ACS_MEDIA_H
#define	_ACS_MEDIA_H

#pragma ident   "$Revision: 1.8 $"

#include "pub/mgmt/sqm_list.h"
#include "pub/mgmt/types.h"
#include "mgmt/util.h"
#include "acssys.h"
#include "acsapi.h"


/*
 * ACSLS software keeps a System defined list of supported media
 * types in $ACS_HOME/data/internal/mixed_media/media_types.dat
 *
 * See also: A table of media types in src/robots/stk/init.c. This
 * is a copy from $ACS_HOME/data/internal/mixed_media/media_types.dat
 * but with only 41 entries, this must be the ones that SAM has
 * qualified. SAM gets the media type from the inquiry command.
 *
 * Depending on the release of ACSLS and the 'PUTs' (storagetek term),
 * the acsls query returns the media type as a string, i.e. LTO-100G
 * On newer releases, that have the PUT0602 applied, the media type
 * is returned as an index. (no doc available, just observations)
 *
 * To check the PUT, log into the ACSLS machine as user acsss,
 * and type "pkginfo -l STKacsls". The resulting output will
 * describe the exact version details, including PUTs. If it is
 * simply 7.1 with no mention of PUTs applied, the release is
 * out-dated.
 *
 * This table also maps the stk media type to the 2 letter media type
 * in SAM's terminology.
 *
 * e.g.
 * From L8500 (PUT0602S for ACSLS, release 7.1.0):-
 *
 * ACSSA> query volume 000379
 * Identifier  Status          Current Location        Type
 * 000379      home              0, 4, 7, 0, 0         LTO-400G
 * <data>
 *   <r>
 *	<f maxlen=\"6\">000379</f>
 *	<f maxlen=\"3\">0</f>
 *	<f maxlen=\"3\">4</f>
 *	<f maxlen=\"5\">7</f>
 *	<f maxlen=\"3\">0</f>
 *	<f maxlen=\"6\">0</f>
 *	<f maxlen=\"5\">0</f>
 *	<f maxlen=\"8\">home</f>
 *	<f maxlen=\"8\">31</f>
 *	<f maxlen=\"7\">data</f>
 *   </r>
 * <data>
 * So from the above XML, '31' maps to LTO-400G which inturn maps
 * to 'li' in SAM's terminology
 *
 * From L180 (no PUTs applied, release 7.1.0):-
 *
 * ACSSA> query volume 000379
 * Identifier  Status          Current Location        Type
 * AE5535      home              0, 0, 1, 5, 0         LTO-100G
 * <data>
 *   <r>
 *	<f maxlen=\"6\">AE5535</f>
 *	<f maxlen=\"3\">0</f>
 *	<f maxlen=\"3\">0</f>
 *	<f maxlen=\"5\">1</f>
 *	<f maxlen=\"3\">5</f>
 *	<f maxlen=\"6\">0</f>
 *	<f maxlen=\"5\">0</f>
 *	<f maxlen=\"8\">home</f>
 *	<f maxlen=\"8\">LTO-100G</f>
 *	<f maxlen=\"7\">data</f>
 *   </r>
 * <data>
 * So from the above XML, stk media type is LTO-100G which maps to
 * 'li' in SAM's terminology
 *
 */
static struct acslsMediaMap {
	int	index;	/* index into the table - for better readability */
	char	stk_mtype[9];	/* stk media type name */
	char	sam_mtype[3];	/* sam media type */
	fsize_t capacity;	/* capacity in kb */
} ACSLS_MAP[] = {
	{0,	"3480",		"st",	(1024 * 210)},
	{1,	"3490E",	"st",	(1024 * 800)},
	{2,	"DD3A",		"d3",	(1024 * 1024 * 10)},
	{3,	"DD3B",		"d3",	(1024 * 1024 * 25)},
	{4,	"DD3C",		"d3",	(1024 * 1024 * 50)},
	{5,	"DD3D",		"cn",	0},	/* DD3 cleaning tape */
	{6,	"DLTIII",	"lt",	(1024 * 1024 * 10)},
	{7,	"DLTIV",	"lt",	(1024 * 1024 * 20)},
	{8,	"DLTIIIXT",	"lt",	(1024 * 1024 * 15)},
	{9,	"STK1R",	"sg",	(1024 * 1024 * 20)},
	{10,	"STK1U",	"sg",	0},
	{11,	"EECART",	"cn",	(1024 * 1024 * 1.6)},
	{12,	"JLABEL",	"99",	0},
	{13,	"STK2P",	"sf",	(1024 * 1024 * 60)},
	{14,	"STK2W",	"sf",	0},
	{15,	"KLABEL",	"99",	0},
	{16,	"LTO-100G",	"li",	(1024 * 1024 * 100)},
	{17,	"LTO-50G",	"li",	(1024 * 1024 * 50)},
	{18,	"LTO-35G",	"li",	(1024 * 1024 * 35)},
	{19,	"LTO-10G",	"li",	(1024 * 1024 * 10)},
	{20,	"LTO-CLN2",	"cn",	0},
	{21,	"LTO-CLN3",	"cn",	0},
	{22,	"LTO-CLN1",	"cn",	0},
	{23,	"SDLT",		"lt",	(1024 * 1024 * 110)},
	{24,	"VIRTUAL",	"99",	0},
	{25,	"LTO-CLNU",	"cn",	0},
	{26,	"LTO-200G",	"li",	(1024 * 1024 * 200)},
	{27,	"SDLT-2",	"lt",	(1024 * 1024 * 300)},
	{28,	"T10000T1",	"ti",	(1024 * 1024 * 500)},
	{29,	"T10000TS",	"ti",	(1024 * 1024 * 120)},
	{30,	"T10000CT",	"ti",	(1024 * 1024 * 0)},
	{31,	"LTO-400G",	"li",	(1024 * 1024 * 400)},
	{32,	"LTO-400W",	"li",	(1024 * 1024 * 400)},
	{33,    "reserved",	"99",	0},
	{34,	"SDLT-S1",	"lt",	(1024 * 1024 * 800)},
	{35,	"SDLT-S2",	"lt",	(1024 * 1024 * 800)},
	{36,	"SDLT-S3",	"lt",	(1024 * 1024 * 800)},
	{37,	"SDLT-S4",	"lt",	(1024 * 1024 * 800)},
	{38,	"SDLT-4",	"lt",	(1024 * 1024 * 800)},
	{39,	"STK1Y",	"sg",	(1024 * 1024 * 800)},
	{40,	"LTO-800G",	"li",	(1024 * 1024 * 800)},
	{41,	"LTO-800W",	"li",	(1024 * 1024 * 800)},
	/* carry over from R45 old code */
	/* do not use this except to match against the stk_mtype */
	{-1,	"9840",		"sg", (1024 * 1024 * 20)}, /* ~ STK1R */
	{-2,	"T9940A",	"sf", (1024 * 1024 * 60)}, /* ~ STK2P */
};
#define	MAX_MEDIA_INDEX 41 /* from media_dat */
#define	ACSLS_MAP_ENTRY_COUNT 44 /* 0-41 -1, -2 */

typedef struct acs_svr {
	char		acsls_host[MAXHOSTNAMELEN];
	int		acsls_port;
	char		ssi_host[MAXHOSTNAMELEN];
	int		ssi_port;
	int		csi_port;
	pthread_t	tid;
} acs_svr_t;


typedef struct acs_drive {
	DRIVEID	id;
	char	type[10];
	char	serial_num[33];
	char	status[10];
	char	state[11];
	char	volume[7];
	LOCKID	lock;
	char	condition[15];
} acs_drive_t;

typedef struct acs_lsm {
	LSMID	id;
	char	status[32];
	char	state[32];
	int	free_cells;
	char	serial_num[64];
} acs_lsm_t;

typedef struct acs_vol {
	char	name[EXTERNAL_LABEL_SIZE + 1];
	PANELID	panel_id;
	int	row;
	int	col;
	int	pool_id;
	char	status[129];
	char	mtype[129];
	char	vtype[129];	/* data, cleaning */
} acs_vol_t;


#define	BUFSIZE	256

#define	SSI_PID_FILE	VAR_DIR"/ssi_pid"

/* Copied from src/robots/stk/init.c */
#define	ACS_HOSTNAME		"CSI_HOSTNAME"
#define	ACS_SSIHOST		"SSI_HOSTNAME"
#define	ACS_ACCESS		"ACSAPI_USER_ID"
#define	ACS_PORTNUM		"ACSAPI_SSI_SOCKET"
#define	ACS_SSI_INET_PORT	"SSI_INET_PORT"
#define	ACS_CSI_HOSTPORT	"CSI_HOSTPORT"


#define	SCRATCH_POOL_ID	"scratch_pool_id"
#define	START_VSN	"start_vsn"
#define	END_VSN	"end_vsn"
#define	VSN_EXPRESSIONS	"vsn_expression"
#define	LSM	"lsm"
#define	PANEL	"panel"
#define	START_ROW	"start_row"
#define	END_ROW	"end_row"
#define	START_COL	"start_col"
#define	END_COL	"end_col"
#define	FILTER_TYPE	"filter_type"
#define	EQU_TYPE	"equ_type"
#define	ACCESS_DATE	"access_date"

static int
get_devpath_entries(stk_host_info_t *stk_host, sqm_lst_t *mcf_paths,
	hashtable_t **h_stk_devpaths, hashtable_t **h_drives);
static int stkmtype2sammtype(char *stk_mtype, char *sam_mtype);
static int
get_vol_with_uniq_mtype(sqm_lst_t *lst_duplicates, sqm_lst_t **lst);

static int vol_in_vol_access_list(char *member, sqm_lst_t *list);

static void
free_list_of_stk_volume(sqm_lst_t *stk_volume_list);

/*
 * The following functions st_XXX and their utilities are copied from
 * src/robots/vendor_supplied/stk/src/client/t_cdriver.c
 */
static char *st_rttostr(ACS_RESPONSE_TYPE k);
static void st_show_ack_res(SEQ_NO s, unsigned char mopts, unsigned
	char eopts,
	LOCKID lid,
	ACS_RESPONSE_TYPE type,
	SEQ_NO seq_nmbr,
	STATUS status,
	ALIGNED_BYTES rbuf[MAX_MESSAGE_SIZE / sizeof (ALIGNED_BYTES)]);
static void st_show_int_resp_hdr(
	ACS_RESPONSE_TYPE type,
	SEQ_NO seq_nmbr,
	SEQ_NO s,
	STATUS status);
static int st_show_final_resp_hdr(
	ACS_RESPONSE_TYPE type,
	SEQ_NO seq_nmbr,
	SEQ_NO s,
	STATUS status);

static BOOLEAN no_variable_part(STATUS status);
static char *decode_mopts(unsigned char msgopt, char *str_buffer);
static char *decode_vers(long vers, char *buffer);
static char *decode_eopts(unsigned char extopt, char *str_buffer);

int get_acs_library_cfg(stk_host_info_t *stk_host, sqm_lst_t *mcf_paths,
    sqm_lst_t **res_lst);

#endif /* ACS_MEDIA_H */
