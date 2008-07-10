/*
 *	ino_ext.h - SAM-FS file system multi-volume inode extension.
 *
 *	Defines the structure of multi-volume inode extension.
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

#ifndef	_SAM_FS_INO_EXT_H
#define	_SAM_FS_INO_EXT_H

#ifdef sun
#pragma ident "$Revision: 1.28 $"
#endif

#include "acl.h"

/*
 * ----- Mode for inode extension types
 */
#define	S_IFMVA 0xff00		/* Multivolume archive extension inode */
#define	S_IFSLN 0xfe00		/* Symbolic link name inode extension */
#define	S_IFRFA 0xfd00		/* Removable media file attr inode ext */
#define	S_IFHLP 0xfc00		/* Hard link parent inode extension */
#define	S_IFACL 0xfb00		/* Access control list inode extension */
#define	S_IFOBJ 0xfa00		/* Object striping array inode extension */


/* ----- Inode header for inode extension types. */

#define	S_IFEXT 0xff00		/* Mode mask for inode extensions */
#define	S_IFORD 0x00ff		/* Extent ordinal in mode field */

#define	EXT_HDR_ERR_DP(eip, eid, dp)					\
		((((eip)->hdr.version != (dp)->version) ||		\
		((eip)->hdr.id.ino != (eid).ino) ||			\
		((eip)->hdr.id.gen != (eid).gen) ||			\
		((eip)->hdr.file_id.ino != (dp)->id.ino) ||		\
		((eip)->hdr.file_id.gen != (dp)->id.gen)))

#define	EXT_HDR_ERR_DP(eip, eid, dp)					\
		((((eip)->hdr.version != (dp)->version) ||		\
		((eip)->hdr.id.ino != (eid).ino) ||			\
		((eip)->hdr.id.gen != (eid).gen) ||			\
		((eip)->hdr.file_id.ino != (dp)->id.ino) ||		\
		((eip)->hdr.file_id.gen != (dp)->id.gen)))

#define	EXT_HDR_ERR(eip, eid, ip)					\
		((((eip)->hdr.version != (ip)->di.version) ||		\
		((eip)->hdr.id.ino != (eid).ino) ||			\
		((eip)->hdr.id.gen != (eid).gen) ||			\
		((eip)->hdr.file_id.ino != (ip)->di.id.ino) ||		\
		((eip)->hdr.file_id.gen != (ip)->di.id.gen)))

#define	EXT_MODE_ERR(eip, emode) (((eip)->hdr.mode & S_IFEXT) != (emode))

#define	EXT_1ST_ORD(eip)		(((eip)->hdr.mode & S_IFORD) == 1)

typedef struct sam_inode_ext_hdr {
	sam_mode_t	mode;		/* Mode and type of file */
	int32_t		version;	/* Inode layout version */
	sam_id_t	id;		/* Unique id: i-num/gen */
	sam_id_t	file_id;	/* Unique base inode id: i-num/gen */
	sam_id_t	next_id;	/* next ext list id: i-num/gen */
} sam_inode_ext_hdr_t;


/*
 * Inode extension for archive of multiple VSNs.
 */
#define	S_ISMVA(mode)  (((mode)&0xff00) == S_IFMVA)

#define	MAX_VSNS_IN_INO	(8)

typedef struct sam_vsn_array {
	struct sam_vsn_section section[MAX_VSNS_IN_INO];
} sam_vsn_array_t;

/* Inode version 1 multivolume archive extension */
struct sam_mv1_inode {
	int		copy;		/* Archive copy number */
	int		n_vsns;		/* Count of sections */
	struct sam_vsn_array vsns;	/* Vsn section array */
};

/* Current inode version (2) multivolume archive extension */

typedef struct sam_mva_inode {
	sam_time_t	creation_time;	/* Time inode extension created */
	sam_time_t	change_time;	/* Time extension info last changed */
	int32_t		fill[2];	/* Reserved */
	int32_t		copy;		/* Copy number */
	int32_t		t_vsns;		/* Archive vsns for this copy */
	int32_t		n_vsns;		/* Count of sections in ext */
	char		pad[4];
	struct sam_vsn_array vsns;	/* Vsn section array */
} sam_mva_inode_t;

/* ----- Ioctl for setting multivolume archive information. */

struct sam_setarch {
	sam_archive_info_t	 ar;			/* Archive copy */
	union {
		struct sam_vsn_section *ptr;	/* Multivol vsn information */
		uint32_t  p32;
		uint64_t  p64;
	} vp;
};

#ifdef	_KERNEL
int sam_set_multivolume(sam_node_t *bip, struct sam_vsn_section **vsnpp,
					int copy, int t_vsns, sam_id_t *fid);
int sam_get_multivolume(sam_node_t *bip, struct sam_vsn_section **vsnpp,
						int copy, int *size);
void sam_fix_mva_inode_ext(sam_node_t *ip, int copy, sam_id_t *eidp);
#endif	/* _KERNEL */


/*
 * Inode extension for symbolic link name
 */
#define	S_ISSLN(mode)  (((mode)&0xff00) == S_IFSLN)

#define	MAX_SLN_CHARS_IN_INO (464)

typedef struct sam_sln_inode {
	sam_time_t	creation_time;	/* Time inode extension created */
	sam_time_t	change_time;	/* Time extension info last changed */
	int32_t		fill;		/* Reserved */
	int32_t		n_chars;	/* Number of symlink chars in inode */
	uchar_t		chars[MAX_SLN_CHARS_IN_INO];	/* Symlk name chars */
} sam_sln_inode_t;


/*
 * Inode extension for removable media file attribute list
 */
#define	S_ISRFA(mode)  (((mode)&0xff00) == S_IFRFA)

typedef struct sam_rfa_inode {
	sam_time_t	creation_time;	/* Time inode extension created */
	sam_time_t	change_time;	/* Time extension last changed */
	sam_resource_file_t info;	/* Removable media file info */
} sam_rfa_inode_t;

typedef struct sam_rfv_inode {
	int		ord;		/* Ordinal of 1st section in ext */
	int		n_vsns;		/* Count of sections in ext */
	struct sam_vsn_array vsns;	/* Vsn section array */
} sam_rfv_inode_t;


/*
 * Inode extension for hard link parent list
 */
#define	S_ISHLP(mode)  (((mode)&0xff00) == S_IFHLP)

#define	MAX_HLP_IDS_IN_INO	(58)

typedef struct sam_hlp_inode {
	sam_time_t	creation_time;	/* Time inode extension created */
	sam_time_t	change_time;	/* Time extension last changed */
	int32_t		fill;		/* Reserved */
	int32_t		n_ids;		/* Num parent ids in inode */
	sam_id_t	ids[MAX_HLP_IDS_IN_INO]; /* Hard link parent id list */
} sam_hlp_inode_t;


/*
 * Inode extension for access control list
 */
#define	S_ISACL(mode)  (((mode)&0xff00) == S_IFACL)

#define	MAX_ACL_ENTS_IN_INO	(38)

typedef struct sam_acl_inode {
	sam_time_t	creation_time;	/* Time inode extension created */
	int32_t		fill;		/* Reserved */
	int32_t		t_acls;		/* Total of regular acl entries */
	int32_t		t_dfacls;	/* Total of default acl entries */
	int32_t		n_acls;		/* Regular acl entries this ext. */
	int32_t		n_dfacls;	/* Default acl entries this ext. */
	sam_acl_t	ent[MAX_ACL_ENTS_IN_INO]; /* Access ctrl list ents */
} sam_acl_inode_t;


/*
 * Inode extension for object striping array
 * See sam_obj_layout_t in ino.h for struct
 */
#define	S_ISOBJ(mode)  (((mode)&0xff00) == S_IFOBJ)

typedef struct sam_osd_ext_inode {
	uint64_t		obj_id[SAM_MAX_OSD_EXTENT];
	ushort_t		ord[SAM_MAX_OSD_EXTENT];
} sam_osd_ext_inode_t;


/*
 * ----- Composite inode extension type
 */
#define	S_ISEXT(mode)  ((S_ISMVA(mode)) || \
						(S_ISSLN(mode)) || \
						(S_ISRFA(mode)) || \
						(S_ISHLP(mode)) || \
						(S_ISACL(mode)) || \
						(S_ISOBJ(mode)))

struct sam_inode_ext {
	struct sam_inode_ext_hdr	hdr;	/* Inode extension header */
	union {
		struct sam_mv1_inode	mv1;	/* Multivol archive ext (v1) */
		struct sam_mva_inode	mva;	/* Multivol arch extension */
		struct sam_sln_inode	sln;	/* Symlink name extension */
		struct sam_rfa_inode	rfa;	/* Resource file attr ext */
		struct sam_rfv_inode	rfv;	/* Resource file vsns ext */
		struct sam_hlp_inode	hlp;	/* Hard link parent ext */
		struct sam_acl_inode	acl;	/* Access control list ext */
		struct sam_osd_ext_inode obj;	/* Object stripe extension */
	} ext;
};

union sam_ino_ext {
	char	i[SAM_ISIZE];
	struct sam_inode_ext	inode;
};

#endif /* _SAM_FS_INO_EXT_H */
