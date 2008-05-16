/*
 * devinfo.h header file.
 *
 *    Contains all of the character names for the devices
 *    The models supported are defined here.
 *    The model consists of a vendor id and a product id.
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

#if !defined(_KERNEL)

#if !defined(SAM_DEVINFO_H)
#define	SAM_DEVINFO_H

#ifdef sun
#pragma ident "$Revision: 1.54 $"
#endif

#define	INQUIRY_CONF    SAM_CONFIG_PATH"/inquiry.conf"

#if !defined(DTYPE_DIRECT)
#define	DTYPE_DIRECT		0x00
#define	DTYPE_SEQUENTIAL	0x01
#define	DTYPE_PRINTER		0x02
#define	DTYPE_PROCESSOR		0x03
#define	DTYPE_WORM		0x04
#define	DTYPE_RODIRECT		0x05
#define	DTYPE_SCANNER		0x06
#define	DTYPE_OPTICAL		0x07
#define	DTYPE_CHANGER		0x08
#define	DTYPE_COMM		0x09
#endif


typedef enum {
	M_NOMODEL = 0,		/* No model defined */
	M_STK4280,		/* STK 4280 streaming tape */
	M_LMS4100,		/* LMS 4100 optical drive */
	M_LMS4500,		/* LMS 4500 optical drive */
	M_RSP2150,		/* Metrum rsp2150 */
	M_CYG1803,		/* Cygnet jukebox */
	M_LF4500,		/* LMS 4500 rapidchanger */
	M_EXB8505,		/* Exabyte 8505 tape drive */
	M_IBM0632,		/* IBM Optical disk drive */
	M_DOCSTOR,		/* DocuStore automated library */
	M_DLT2000,		/* DEC Digital linear tape */
	M_DLT2700,		/* DEC DLT Mini-library - robot */
	M_HPC1716,		/* HP erasable optical disk */
	M_HPC1710,		/* HP optical library */
	M_METD360,		/* Metrum D-360 Library */
	M_ARCHDAT,		/* Archive Python dat */
	M_METD28,		/* Metrum D-28 Library */
	M_ACL452,		/* ACL 4/52 */
	M_ACL2640,		/* ACL 2640 */
	M_ATLP3000,		/* ATL P3000, P4000, P7000 */
	M_GRAUACI,		/* GRAU through aci interface */
	M_EXB210,		/* Exabyte 210 */
	M_STKAPI,		/* STK api interface */
	M_ADIC448,		/* ADIC 448 */
	M_SPECLOG,		/* Spectralogic */
	M_STK9490,		/* STK 9490 */
	M_STKD3,		/* STK D3 */
	M_STK9840,		/* STK 9840 */
	M_STK9940,		/* STK 9940 */
	M_RMTSAM,		/* Remote Sam Device */
	M_RMTSAMC,		/* Remote Sam Client */
	M_RMTSAMS,		/* Remote Sam Server */
	M_IBM3590,		/* IBM3590 Tape */
	M_IBMATL,		/* IBM ATL interface */
	M_STK97XX,		/* STK through SCSI */
	M_FJNMXX,		/* Fujitsu NML270 and 250 libraries */
	M_IBM3570,		/* IBM3570 Tape */
	M_IBM3570C,		/* IBM3570 Media changer */
	M_SONYDTF,		/* Sony DTF 2120 */
	M_SONYAIT,		/* Sony AIT */
	M_SONYDMS,		/* Sony MDS changer */
	M_SONYCSM,		/* Sony CSM-20S Tape Library */
	M_SONYPSC,		/* Sony through PSC api */
	M_UNUSED2,		/* unused */
	M_FUJITSU_128,		/* Fujitsu M8100 128track drive */
	M_EXBM2,		/* Exabyte Mammoth-2 tape drive */
	M_PLASMON_D,		/* Plasmon D-Series DVD-RAM library */
	M_PLASMON_G,		/* Plasmon G-Series UDO/MO library */
	M_HITACHI,		/* Hitachi DVD-RAM drive */
	M_IBM3580,		/* IBM3580 Tape */
	M_ADIC1000,		/* ADIC Scalar 100 Library */
	M_EXBX80,		/* Exabyte X80 library */
	M_STKLXX,		/* STK L20/L40/L80, Sun L7/L8 Library */
	M_IBM3584,		/* IBM 3584 library */
	M_ADIC100,		/* ADIC Scalar 100 Library */
	M_HP_C7200,		/* HP C7200 Series Libraries */
	M_QUAL82xx,		/* Qualstar 82xx Library */
	M_ATL1500,		/* ATL 1500 and ATL 2500 Libraries */
	M_ODI_NEO,		/* Overland Data Inc. Neo Series Libraries */
	M_SONYSAIT,		/* SONY Super AIT */
	M_IBM3592,		/* IBM3592 Tape */
	M_PLASMON_UDO,		/* Plasmon UDO */
	M_STKTITAN,		/* STK Titanium drive */
	M_QUANTUMC4,		/* Sun C4/ Quantumm PX500 */
	M_HPSLXX,		/* Sun HP SL48 Library */
	M_LAST
} MODEL;

typedef struct inquiry_id {
	char	*vendor_id;
	char	*product_id;
	char	*samfs_id;
} inquiry_id_t;

typedef struct {
	char	*short_name;
	int	dt;
	MODEL	model;
	uchar_t	dtype;
	char	*long_name;
} sam_model_t;

#if defined(DEC_INIT)

inquiry_id_t  *inquiry_info = (inquiry_id_t *)NULL;

sam_model_t sam_model[] = {
	{"acl2640", DT_ACL2640, M_ACL2640, DTYPE_CHANGER,
		"ATL 2640, P1000, or Sun L1000 Library"},
	{"acl452",  DT_ACL452, M_ACL452, DTYPE_CHANGER,
		"ATL 4/52, 7100 or Sun 1800, 3500 Library"},
	{"atlp3000", DT_ATLP3000, M_ATLP3000, DTYPE_CHANGER,
		"ATL P3000, P4000, P7000 or Sun L11000 Library"},
	{"adic448", DT_ADIC448, M_ADIC448, DTYPE_CHANGER,
		"ADIC 448 Library"},
	{"adic1000", DT_ADIC1000, M_ADIC1000, DTYPE_CHANGER,
		"ADIC 1000 and 10K Library"},
	{"adic100", DT_ADIC100, M_ADIC100, DTYPE_CHANGER,
		"ADIC 100 Library"},
	{"archdat", DT_DAT, M_ARCHDAT, DTYPE_SEQUENTIAL,
		"Archive Python 4mm DAT"},
	{"atl1500", DT_ATL1500, M_ATL1500, DTYPE_CHANGER,
		"ATL L1500 and 2500 or Sun L25 and L100 Library"},
	{"cyg1803", DT_CYGNET, M_CYG1803, DTYPE_CHANGER,
		"Cygnet Jukebox 1803"},
	{"dlt2000", DT_LINEAR_TAPE, M_DLT2000, DTYPE_SEQUENTIAL,
		"DLT Tape" },
	{"dlt2700", DT_DLT2700, M_DLT2700, DTYPE_CHANGER,
		"DLT Tape Library" },
	{"docstor", DT_DOCSTOR, M_DOCSTOR, DTYPE_CHANGER,
		"DISC or Plasmon Library" },
	{"exb210",  DT_EXB210, M_EXB210, DTYPE_CHANGER,
		"Exabyte 210, Sun L280, or ATL L-series Library" },
	{"exb8505", DT_EXABYTE_TAPE, M_EXB8505, DTYPE_SEQUENTIAL,
		"Exabyte Tape" },
	{"exbm2",   DT_EXABYTE_M2_TAPE, M_EXBM2, DTYPE_SEQUENTIAL,
		"Exabyte Mammoth-2 Tape" },
	{"exbx80",  DT_EXBX80, M_EXBX80, DTYPE_CHANGER,
		"Exabyte X80 Library" },
	{"grauaci", DT_GRAUACI, M_GRAUACI, DTYPE_CHANGER,
		"GRAU Library " },
	{"hpc1716", DT_ERASABLE, M_HPC1716, DTYPE_DIRECT,
		"HP Erasable Optical" },
	{"hpc1716", DT_ERASABLE, M_HPC1716, DTYPE_OPTICAL,
		"HP Erasable Optical" },
	{"hitachi", DT_ERASABLE, M_HITACHI, DTYPE_RODIRECT,
		"Hitachi DVD-RAM" },
	{"plasmonUDO", DT_PLASMON_UDO, M_PLASMON_UDO, DTYPE_OPTICAL,
		"Plasmon UDO Optical" },
	{"hpc7200", DT_HP_C7200, M_HP_C7200, DTYPE_CHANGER,
		"Sun StorEDGE L9/20/40/60 or HP C7200 series" },
	{"hpoplib", DT_HPLIBS, M_HPC1710, DTYPE_CHANGER,
		"HP Optical Library" },
	{"ibmatl",  DT_IBMATL, M_IBMATL, DTYPE_CHANGER,
		"IBM ATL Library" },
	{"ibm0632", DT_MULTIFUNCTION, M_IBM0632, DTYPE_OPTICAL,
		"IBM Optical" },
	{"ibm3570", DT_3570, M_IBM3570, DTYPE_SEQUENTIAL,
		"IBM 3570 Tape" },
	{"ibm3570c", DT_3570C, M_IBM3570C, DTYPE_CHANGER,
		"IBM 3570 Changer" },
	{"ibm3580", DT_IBM3580, M_IBM3580, DTYPE_SEQUENTIAL,
		"IBM LTO 3580, Seagate Viper or HP Ultrium Tape" },
	{"ibm3584", DT_IBM3584, M_IBM3584, DTYPE_CHANGER,
		"IBM 3584 Library" },
	{"ibm3590", DT_3590, M_IBM3590, DTYPE_SEQUENTIAL,
		"IBM 3590 Tape" },
	{"ibm3592", DT_3592, M_IBM3592, DTYPE_SEQUENTIAL,
		"IBM 3592 Tape" },
	{"lms4100", DT_WORM_OPTICAL_12, M_LMS4100, DTYPE_WORM,
		"LMS 4100"},
	{"lms4500", DT_WORM_OPTICAL_12, M_LMS4500, DTYPE_CHANGER,
		"LMS 4500 Changer" },
	{"metd28",  DT_METD28, M_METD28, DTYPE_CHANGER,
		"Mountain Gate D-28 Library" },
	{"metd360", DT_METD360, M_METD360, DTYPE_CHANGER,
		"Metrum D-360 Library" },
	{"odi_neo", DT_ODI_NEO, M_ODI_NEO, DTYPE_CHANGER,
		"Overland Data Inc. Neo Series Library"},
	{"plasmond", DT_PLASMON_D, M_PLASMON_D, DTYPE_CHANGER,
		"Plasmon D-Series DVD-RAM Library" },
	{"plasmong", DT_PLASMON_G, M_PLASMON_G, DTYPE_CHANGER,
		"Plasmon G-Series UDO/MO library"},
	{"quantumc4", DT_QUANTUMC4, M_QUANTUMC4, DTYPE_CHANGER,
		"Sun C4/Quantum PX500 library" },
	{"rap4500", DT_LMS4500, M_LF4500, DTYPE_CHANGER,
		"LMS RapidChanger 4500" },
	{"rmtsam", DT_PSEUDO_RD, M_RMTSAM, DTYPE_SEQUENTIAL,
		"Remote Sam device"},
	{"rmtsamc", DT_PSEUDO_SC, M_RMTSAMC, DTYPE_CHANGER,
		"Remote Sam Client"},
	{"rmtsams", DT_PSEUDO_SS, M_RMTSAMS, DTYPE_CHANGER,
		"Remote Sam Server"},
	{"rsp2150", DT_VIDEO_TAPE, M_RSP2150, DTYPE_SEQUENTIAL,
		"Metrum VHS tape" },
	{"speclog", DT_SPECLOG, M_SPECLOG, DTYPE_CHANGER,
		"SpectraLogic Library" },
	{"qual82xx", DT_QUAL82xx, M_QUAL82xx, DTYPE_CHANGER,
		"Qualstar 42xx, 62xx, 82xx library" },
	{"sonydms", DT_SONYDMS, M_SONYDMS, DTYPE_CHANGER,
		"Sony DMF or DMS Changer" },
	{"sonydtf", DT_SONYDTF, M_SONYDTF, DTYPE_SEQUENTIAL,
		"Sony DTF 2120" },
	{"sonyait", DT_SONYAIT, M_SONYAIT, DTYPE_SEQUENTIAL,
		"Sony AIT Tape" },
	{"sonysait", DT_SONYSAIT, M_SONYSAIT, DTYPE_SEQUENTIAL,
		"Sony Super AIT Tape" },
	{"stk4280", DT_SQUARE_TAPE, M_STK4280, DTYPE_SEQUENTIAL,
		"STK 4280 Tape"},
	{"stk9490", DT_9490, M_STK9490, DTYPE_SEQUENTIAL,
		"STK 9490 Tape"},
	{"stkapi",  DT_STKAPI, M_STKAPI, DTYPE_CHANGER,
		"STK ACSLS Library" },
	{"stkd3",   DT_D3, M_STKD3, DTYPE_SEQUENTIAL,
		"STK D3 Tape"},
	{"stk97xx", DT_STK97XX, M_STK97XX, DTYPE_CHANGER,
		"STK 97XX or Sun L700/L180 Library" },
	{"fujitsu_nm", DT_FJNMXX, M_FJNMXX, DTYPE_CHANGER,
		"Fujitsu NM2XX Libraries" },
	{"stk9840", DT_9840, M_STK9840, DTYPE_SEQUENTIAL,
		"STK 9840 Tape"},
	{"stk9940", DT_9940, M_STK9940, DTYPE_SEQUENTIAL,
		"STK 9940 Tape"},
	{"stklxx", DT_STKLXX, M_STKLXX, DTYPE_CHANGER,
		"STK L20/L40/L80 or Sun L7/L8 Library" },
	{"stktitan", DT_TITAN, M_STKTITAN, DTYPE_SEQUENTIAL,
		"STK Titanium Tape"},
	{"sonypsc", DT_SONYPSC, M_SONYPSC, DTYPE_CHANGER,
		"Sony PSC Library" },
	{"fujitsu_128", DT_FUJITSU_128, M_FUJITSU_128, DTYPE_SEQUENTIAL,
		"Fujitsu M8100 128track Tape" },
	{"sonycsm", DT_SONYCSM, M_SONYCSM, DTYPE_CHANGER,
		"Sony CSM-20s Tape Library" },
	{"hpslxx", DT_HPSLXX, M_HPSLXX, DTYPE_CHANGER,
		"HP SL48 Tape Library" },
	{(char *)NULL, 0, 0, 0, (char *)NULL}
};

char *dev_sync[] = {
	"no", "yes", NULL
};

#else	/* DEC_INIT */
extern inquiry_id_t  *inquiry_info;
extern sam_model_t  sam_model[];
#endif	/* DEC_INIT */

#endif  /* SAM_DEVINFO_H */

#endif /* _KERNEL */
