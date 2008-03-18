/*
 * devnm.h - SAM-FS device mnemonics.
 *
 *  Description:
 *    Data loaded 2-character device mnemonics.  If DEC_INIT is
 *    defined, the mnemonic table data will be included.
 *
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

#ifndef _SAM_DEVNM_H
#define	_SAM_DEVNM_H

#ifdef sun
#pragma ident "$Revision: 1.52 $"
#endif


typedef struct {	/* Device to mnemonic table */
	char	*nm;	/* Two character mnemonic */
	short	mf;	/* Media flag */
	dtype_t	dt;	/* Device type */
} dev_nm_t;

/* For media table, mf = true if generic. */
/* For device table, mf = true if removable media device. */



/* ----- Two character device type class/media mnemonics. */

#if defined(DEC_INIT)


/*
 * NOTE - these are indexed by type .....
 * nm = dev_nmtp[un->type & DT_MEDIA_MASK];
 */

/*
 * NOTE also that d2 support is removed at 4.1. The "xx" media type
 * is available for use by a new media type.
 */

char *dev_nmtp[] = {
	"tp", "vt", "st", "xt", "lt", "dt", "se", "d3", "d2", "ib", "i7",
	"so", "at", "sg", "fd", "xm", "sf", "li", "sa", "m2", "ti", NULL
};

#define	MT_CNT ((sizeof (dev_nmtp)/sizeof (char *)) - 1)

char *dev_nmod[] = {
	"od", "o2", "wo", "mo", "mf", "pu", NULL
};

#define	OD_CNT ((sizeof (dev_nmod)/sizeof (char *)) - 1)

/* Define all possible third party names */
char *dev_nmtr[] = {
	"z0", "z1", "z2", "z3", "z4", "z5", "z6", "z7", "z8", "z9",
	"za", "zb", "zc", "zd", "ze", "zf", "zg", "zh", "zi", "zj",
	"zk", "zl", "zm", "zn", "zo", "zp", "zq", "zr", "zs", "zt",
	"zu", "zv", "zw", "zx", "zy", "zz"
};

#define	TP_CNT (sizeof (dev_nmtr)/sizeof (char *))

/* Define all possible striped group disks - g0 - g127 */

char *dev_nmsg[] = {
"g0  ", "g1  ", "g2  ", "g3  ", "g4  ", "g5  ", "g6  ", "g7  ", "g8  ", "g9  ",
"g10 ", "g11 ", "g12 ", "g13 ", "g14 ", "g15 ", "g16 ", "g17 ", "g18 ", "g19 ",
"g20 ", "g21 ", "g22 ", "g23 ", "g24 ", "g25 ", "g26 ", "g27 ", "g28 ", "g29 ",
"g30 ", "g31 ", "g32 ", "g33 ", "g34 ", "g35 ", "g36 ", "g37 ", "g38 ", "g39 ",
"g40 ", "g41 ", "g42 ", "g43 ", "g44 ", "g45 ", "g46 ", "g47 ", "g48 ", "g49 ",
"g50 ", "g51 ", "g52 ", "g53 ", "g54 ", "g55 ", "g56 ", "g57 ", "g58 ", "g59 ",
"g60 ", "g61 ", "g62 ", "g63 ", "g64 ", "g65 ", "g66 ", "g67 ", "g68 ", "g69 ",
"g70 ", "g71 ", "g72 ", "g73 ", "g74 ", "g75 ", "g76 ", "g77 ", "g78 ", "g79 ",
"g80 ", "g81 ", "g82 ", "g83 ", "g84 ", "g85 ", "g86 ", "g87 ", "g88 ", "g89 ",
"g90 ", "g91 ", "g92 ", "g93 ", "g94 ", "g95 ", "g96 ", "g97 ", "g98 ", "g99 ",
"g100", "g101", "g102", "g103", "g104", "g105", "g106", "g107", "g108", "g109",
"g110", "g111", "g112", "g113", "g114", "g115", "g116", "g117", "g118", "g119",
"g120", "g121", "g122", "g123", "g124", "g125", "g126", "g127" };

#define	SG_CNT (sizeof (dev_nmsg)/sizeof (char *))

int dev_nmsg_size = sizeof (dev_nmsg) / sizeof (dev_nmsg[0]);

#endif	/* DEC_INIT */

#ifdef DEC_INIT

char *dev_nmmd[] = {
	"dk", "md", "mm", "mr", "cb", "ob", "??", "??", NULL
};

char *dev_nmfs[] = {
	"fs", "ms", "ma", "mb", "moa", "??", "??", "??", NULL
};

/* NOTE: This table must be in synch with the DT_xxx defines in devstat.h */

char *dev_nmrb[] = {
	"rb", "rc", "cy", "ds", "hp", "ml", "me", "me", "me", "ac", "ac", "ac",
	"gr", "eb", "sk", "ad", "sl", "im", "s9", "?1", "ic", "dm", "cs",
	"?2", "ac", "pd", "pe", "as", "e8", "sn", "il", "ae", "h4", "hc", "q8",
	"al", "ov", "pg", "c4", NULL
};

char *dev_nmps[] = {
	"ps", "si", "sc", "ss", "rd", "hy", NULL
};

/* Device mnemonic table: */

dev_nm_t dev_nm2dt[] = {
	{"fs",  0, DT_FAMILY_SET},
	{"ms",  0, DT_DISK_SET },
	{"ma",  0, DT_META_SET },
	{"mb",	0, DT_META_OBJECT_SET },
	{"moa",	0, DT_META_OBJ_TGT_SET },
	{"rb",  0, DT_ROBOT },
	{"rc",  0, DT_LMS4500 },
	{"ml",  0, DT_DLT2700 },
	{"ds",  0, DT_DOCSTOR },
	{"hc",	0, DT_HP_C7200 },
	{"hp",  0, DT_HPLIBS },
	{"cy",  0, DT_CYGNET },
	{"me",  0, DT_METRUM_LIB },
	{"me",  0, DT_METD28 },
	{"me",  0, DT_METD360 },
	{"ac",  0, DT_ACL_LIB },
	{"ac",  0, DT_ACL452 },
	{"ac",  0, DT_ACL2640 },
	{"ac",  0, DT_ATLP3000 },
	{"dm",  0, DT_SONYDMS },
	{"cs",  0, DT_SONYCSM },
	{"pe",  0, DT_SONYPSC },
	{"gr",  0, DT_GRAUACI },
	{"?2",  0, DT_UNUSED2 },
	{"eb",  0, DT_EXB210 },
	{"e8",  0, DT_EXBX80 },
	{"sk",  0, DT_STKAPI },
	{"s9",  0, DT_STK97XX },
	{"sn",  0, DT_STKLXX },
	{"?1",  0, DT_UNUSED1 },
	{"h4",	0, DT_HPSLXX },
	{"ic",  0, DT_3570C },
	{"im",  0, DT_IBMATL },
	{"il",  0, DT_IBM3584 },
	{"ae",  0, DT_ADIC100 },
	{"ad",  0, DT_ADIC448 },
	{"al",  0, DT_ATL1500 },
	{"as",  0, DT_ADIC1000 },
	{"sl",  0, DT_SPECLOG },
	{"q8",  0, DT_QUAL82xx },
	{"ov",  0, DT_ODI_NEO },
	{"dk",  0, DT_DISK },
	{"md",  0, DT_DATA },
	{"mm",  0, DT_META },
	{"mr",  0, DT_RAID },
	{"ob",  0, DT_OBJECT },
	{"tp",  1, DT_TAPE },
	{"vt",  1, DT_VIDEO_TAPE },
	{"st",  1, DT_SQUARE_TAPE },
	{"se",  1, DT_9490 },
	{"xt",  1, DT_EXABYTE_TAPE },
	{"xm",  1, DT_EXABYTE_M2_TAPE },
	{"lt",  1, DT_LINEAR_TAPE },
	{"dt",  1, DT_DAT },
	{"xx",  1, DT_xx },
	{"d3",  1, DT_D3 },
	{"fd",  0, DT_FUJITSU_128 },
	{"pd",  0, DT_PLASMON_D },
	{"pg",  0, DT_PLASMON_G },
	{"c4",  0, DT_QUANTUMC4 },
	{"sg",  1, DT_9840 },
	{"sf",  1, DT_9940 },
	{"ib",  1, DT_3590 },
	{"m2",  1, DT_3592 },
	{"i7",  1, DT_3570 },
	{"li",  1, DT_IBM3580 },
	{"so",  1, DT_SONYDTF },
	{"at",  1, DT_SONYAIT },
	{"sa",  1, DT_SONYSAIT },
	{"ti",  1, DT_TITAN },
	{"od",  1, DT_OPTICAL },
	{"wo",  1, DT_WORM_OPTICAL },
	{"mo",  1, DT_ERASABLE },
	{"pu",	1, DT_PLASMON_UDO },
	{"o2",  1, DT_WORM_OPTICAL_12},
	{"mf",  0, DT_MULTIFUNCTION },
	{"sc",  0, DT_PSEUDO_SC },
	{"ss",  0, DT_PSEUDO_SS },
	{"rd",  1, DT_PSEUDO_RD },
	{"hy",  1, DT_HISTORIAN },
	{"za",  1, DT_THIRD_PARTY },
	{"zb",  1, DT_THIRD_PARTY },
	{"zc",  1, DT_THIRD_PARTY },
	{"zd",  1, DT_THIRD_PARTY },
	{"ze",  1, DT_THIRD_PARTY },
	{"zf",  1, DT_THIRD_PARTY },
	{"zg",  1, DT_THIRD_PARTY },
	{"zh",  1, DT_THIRD_PARTY },
	{"zi",  1, DT_THIRD_PARTY },
	{"zj",  1, DT_THIRD_PARTY },
	{"zk",  1, DT_THIRD_PARTY },
	{"zl",  1, DT_THIRD_PARTY },
	{"zm",  1, DT_THIRD_PARTY },
	{"zn",  1, DT_THIRD_PARTY },
	{"zo",  1, DT_THIRD_PARTY },
	{"zp",  1, DT_THIRD_PARTY },
	{"zq",  1, DT_THIRD_PARTY },
	{"zr",  1, DT_THIRD_PARTY },
	{"zs",  1, DT_THIRD_PARTY },
	{"zt",  1, DT_THIRD_PARTY },
	{"zu",  1, DT_THIRD_PARTY },
	{"zv",  1, DT_THIRD_PARTY },
	{"zw",  1, DT_THIRD_PARTY },
	{"zx",  1, DT_THIRD_PARTY },
	{"zy",  1, DT_THIRD_PARTY },
	{"zz",  1, DT_THIRD_PARTY },
	{"z0",  1, DT_THIRD_PARTY },
	{"z1",  1, DT_THIRD_PARTY },
	{"z2",  1, DT_THIRD_PARTY },
	{"z3",  1, DT_THIRD_PARTY },
	{"z4",  1, DT_THIRD_PARTY },
	{"z5",  1, DT_THIRD_PARTY },
	{"z6",  1, DT_THIRD_PARTY },
	{"z7",  1, DT_THIRD_PARTY },
	{"z8",  1, DT_THIRD_PARTY },
	{"z9",  1, DT_THIRD_PARTY },
	{ NULL, 0, 0 }
};

/*  Media mnemonic table:    */

dev_nm_t    dev_nm2mt[] = {
	{"tp",  1, DT_TAPE },
	{"vt",  0, DT_VIDEO_TAPE },
	{"st",  0, DT_SQUARE_TAPE },
	{"se",  0, DT_9490 },
	{"xt",  0, DT_EXABYTE_TAPE },
	{"xm",  0, DT_EXABYTE_M2_TAPE },
	{"lt",  0, DT_LINEAR_TAPE },
	{"dt",  0, DT_DAT },
	{"xx",  0, DT_xx },
	{"d3",  0, DT_D3 },
	{"fd",  0, DT_FUJITSU_128 },
	{"sg",  0, DT_9840 },
	{"sf",  0, DT_9940 },
	{"ib",  0, DT_3590 },
	{"m2",  0, DT_3592 },
	{"i7",  0, DT_3570 },
	{"li",  0, DT_IBM3580 },
	{"so",  0, DT_SONYDTF },
	{"at",  0, DT_SONYAIT },
	{"ti",  0, DT_TITAN },
	{"sa",  0, DT_SONYSAIT },
	{"od",  1, DT_OPTICAL },
	{"o2",  0, DT_WORM_OPTICAL_12},
	{"wo",  0, DT_WORM_OPTICAL },
	{"mo",  0, DT_ERASABLE },
	{"pu",	0, DT_PLASMON_UDO },
	{"dk",  0, DT_DISK },
	{"cb",  0, DT_STK5800 },
	{"za",  0, DT_THIRD_PARTY },
	{"zb",  0, DT_THIRD_PARTY },
	{"zc",  0, DT_THIRD_PARTY },
	{"zd",  0, DT_THIRD_PARTY },
	{"ze",  0, DT_THIRD_PARTY },
	{"zf",  0, DT_THIRD_PARTY },
	{"zg",  0, DT_THIRD_PARTY },
	{"zh",  0, DT_THIRD_PARTY },
	{"zi",  0, DT_THIRD_PARTY },
	{"zj",  0, DT_THIRD_PARTY },
	{"zk",  0, DT_THIRD_PARTY },
	{"zl",  0, DT_THIRD_PARTY },
	{"zm",  0, DT_THIRD_PARTY },
	{"zn",  0, DT_THIRD_PARTY },
	{"zo",  0, DT_THIRD_PARTY },
	{"zp",  0, DT_THIRD_PARTY },
	{"zq",  0, DT_THIRD_PARTY },
	{"zr",  0, DT_THIRD_PARTY },
	{"zs",  0, DT_THIRD_PARTY },
	{"zt",  0, DT_THIRD_PARTY },
	{"zu",  0, DT_THIRD_PARTY },
	{"zv",  0, DT_THIRD_PARTY },
	{"zw",  0, DT_THIRD_PARTY },
	{"zx",  0, DT_THIRD_PARTY },
	{"zy",  0, DT_THIRD_PARTY },
	{"zz",  0, DT_THIRD_PARTY },
	{"z0",  0, DT_THIRD_PARTY },
	{"z1",  0, DT_THIRD_PARTY },
	{"z2",  0, DT_THIRD_PARTY },
	{"z3",  0, DT_THIRD_PARTY },
	{"z4",  0, DT_THIRD_PARTY },
	{"z5",  0, DT_THIRD_PARTY },
	{"z6",  0, DT_THIRD_PARTY },
	{"z7",  0, DT_THIRD_PARTY },
	{"z8",  0, DT_THIRD_PARTY },
	{"z9",  0, DT_THIRD_PARTY },
	{ NULL, 0, 0 }
};
#endif	/* DEC_INIT */

extern  char *dev_nmmd[];
extern  char *dev_nmtp[];
extern  char *dev_nmsg[];
extern  int dev_nmsg_size;
extern  char *dev_nmod[];
extern  char *dev_nmfs[];
extern  char *dev_nmrb[];
extern  char *dev_nmps[];
extern  char *dev_nmtr[];
extern  dev_nm_t dev_nm2dt[];
extern  dev_nm_t dev_nm2mt[];

#endif	/* _SAM_DEVNM_H */
