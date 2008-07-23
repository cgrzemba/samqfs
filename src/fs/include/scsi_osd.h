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

#ifndef	_SCSI_OSD_H
#define	_SCSI_OSD_H


#pragma ident	"%Z%%M%	%I%	%E% SMI"


#include <sys/byteorder.h>

#ifdef	__cplusplus
extern "C" {
#endif


/* byte-boundary packing is essential here */
#pragma pack(1)

/*
 * SCSI Object-Based Storage (OSD) commands and data
 * as per T10 OSD specification Revision 10 (30 July 2004)
 *
 * IMPLEMENTATION NOTE: In order to avoid ambiguity and confusion, this
 * implementation attempts to keep structure member names as close as
 * possible to the field names used in the T10 spec.  In particular, the
 * use of arbitrary abbreviations is deliberately kept to a minimum.
 */


/* Generic T10 OSD Definitions: NOV 2007 */


/*
 * Format for Data-In Buffer and Data-Out Buffer offset fields.
 *
 * The offset field format is a 32-bit quantity that defines the upper 4 bits
 * as a power-of-two exponent and the remaining 28 bits as the mantissa. The
 * actual byte offset is computed as follows:
 *
 *   offset = mantissa * (2 ^ (exponent + 8))
 */

typedef union osd_offset_field_format {

	/* Use to set all bits to the 'not in use' value (0xFFFFFFFF) */
	uint32_t	off_not_in_use;

	struct {
		/* Byte 0: Exponent and upper 4 bits of mantissa */
#if defined(_BIT_FIELDS_LTOH)
		uint8_t	off_u_mantissa0	: 4,	/* 4-bit MSB */
			off_u_exponent	: 4;
#else
		uint8_t	off_u_exponent	: 4,
			off_u_mantissa0	: 4;	/* 4-bit MSB */
#endif
		/* Bytes 1-3: lower 24 bits of mantissa */
		uint8_t	off_u_mantissa1;
		uint8_t	off_u_mantissa2;
		uint8_t	off_u_mantissa3;	/* LSB */
	} off_u;

} osd_offset_field_format_t;


#define	off_exponent	off_u.off_u_exponent
#define	off_mantissa0	off_u.off_u_mantissa0
#define	off_mantissa1	off_u.off_u_mantissa1
#define	off_mantissa2	off_u.off_u_mantissa2
#define	off_mantissa3	off_u.off_u_mantissa3


/*
 * Required value when an 'offset' field is not in use.
 */
#define	OSD_OFFSET_FIELD_NOT_IN_USE		0xFFFFFFFF

#define	OSD_SET_OFFSET_FIELD_UNUSED(ofp)				\
	SCSI_WRITE32(&(ofp)->off_not_in_use, OSD_OFFSET_FIELD_NOT_IN_USE)

#define	OSD_SET_OFFSET_FIELD_ZERO(ofp)					\
	SCSI_WRITE32(&(ofp)->off_not_in_use, 0)

#define	OSD_OFFSET_CHECK_MASK(exp)					\
	(((1ULL << (exp)) - 1) | (~((1ULL << (28 + (exp))) - 1)))

/*
 * Returns OSD_FAILURE if exponent is not in the range [8, 23] or if
 * value cannot be exactly represented using this exponent.
 */
#define	OSD_OFFSET_IS_VALID(exponent, value)				\
	((((((exponent) - 8) & ~0xF) == 0) &&				\
	(((value) & OSD_OFFSET_CHECK_MASK(exponent)) == 0)) ?		\
	OSD_SUCCESS : OSD_FAILURE)

#define	OSD_SET_OFFSET_FIELD(ofp, exponent, value)			\
	(ofp)->off_exponent = (uint8_t)((exponent) - 8);		\
	(ofp)->off_mantissa3 = (uint8_t)((value) >> (exponent));	\
	(ofp)->off_mantissa2 = (uint8_t)((value) >> ((exponent) + 8));	\
	(ofp)->off_mantissa1 = (uint8_t)((value) >> ((exponent) + 16));	\
	(ofp)->off_mantissa0 = (uint8_t)((value) >> ((exponent) + 24));



/*
 * Set an attribute using CDB fields.
 * Bytes 52-79 in the OSD CDB.
 */
typedef struct osd_cdbfield_attribute {
	/*
	 * These specify one attribute value to be set.  A zero for the
	 * opa_set_attributes_page value specifies that no attribute value
	 * is to be set.
	 */
	uint32_t oca_set_attributes_page;
	uint32_t oca_set_attribute_number;

	/* Specifies the number of bytes in the attribute to be set. */
	uint16_t oca_set_attribute_length;

	uint8_t	oca_attribute_value[18];
} osd_cdbfield_attribute_t;

/*
 * Page-oriented get and set attributes CDB parameters.
 * Bytes 52-79 in the OSD CDB.
 */
typedef struct osd_page_attribute {
	/*
	 * Attributes page number to be retrieved. A zero specifies
	 * that no attributes page is to be retrieved.
	 */
	uint32_t opa_get_attributes_page;

	/*
	 * Specifies the number of bytes allocated to receive the retrieved
	 * attributes page. An insufficient length will cause the retrieved
	 * data to be truncated without generating an error.
	 */
	uint32_t opa_get_attributes_allocation_length;

	/*
	 * Specifies the byte offset of the first Data-In Buffer byte to
	 * contain the retrieved attributes page.
	 */
	osd_offset_field_format_t	opa_retrieved_attributes_offset;

	/*
	 * These specify one attribute value to be set.  A zero for the
	 * opa_set_attributes_page value specifies that no attribute value
	 * is to be set.
	 */
	uint32_t opa_set_attributes_page;
	uint32_t opa_set_attribute_number;

	/* Specifies the number of bytes in the attribute to be set. */
	uint32_t opa_set_attribute_length;

	/*
	 * Specifies the byte offset of the first Data-Out Buffer byte
	 * containing the value of the attribute to be set.
	 */
	osd_offset_field_format_t	opa_set_attributes_offset;

} osd_page_attribute_t;


/*
 * List-oriented get and set attributes CDB parameters.
 * Bytes 52-79 in the OSD CDB.
 */
typedef struct osd_list_attribute {
	/*
	 * Defines the byte length of the get attributes list that specifies
	 * one or more attributes values to be retrieved. A zero specifies
	 * that no get attributes list is provided with the command.
	 */
	uint32_t ola_get_attributes_list_length;

	/*
	 * Specifies the byte offset of the first Data-Out Buffer byte
	 * containing the get attributes list.
	 */
	osd_offset_field_format_t	ola_get_attributes_list_offset;

	/*
	 * Specifies the number of bytes allocated to receive the retrieved
	 * attributes list. An insufficient length will cause the retrieved
	 * page to be truncated without generating an error.
	 */
	uint32_t ola_get_attributes_allocation_length;

	/*
	 * Specifies the byte offset of the first Data-In Buffer byte to
	 * contain the retrieved attributes list.
	 */
	osd_offset_field_format_t	ola_retrieved_attributes_offset;

	/*
	 * Specifies the length of the set attributes list that specifies one
	 * or more attributes values to be set. A zero specifies that no get
	 * attributes list is provided with the command.
	 */
	uint32_t ola_set_attributes_list_length;

	/*
	 * Specifies the byte offset of the first Data-Out Buffer byte
	 * containing the set attributes list.
	 */
	osd_offset_field_format_t	ola_set_attributes_list_offset;

} osd_list_attribute_t;





/*
 * USER (User) capability object descriptor.
 * Bytes 60-103 of the OSD capability format (osd_capability_format_t).
 * Byte offset within osd_capability_format_t (not in osd_cdb_t)
 */
typedef struct osd_user_object_descriptor_format {
	uint32_t	ucod_policy_access_tag;		/* Bytes 60-63 */
	uint16_t	ucod_boot_epoch;		/* Bytes 64-65 */
	uint8_t		ucod_reserved[6];		/* Bytes 66-71 */
	uint64_t	ucod_allowed_partition_id;	/* Bytes 72-79 */
	uint64_t	ucod_allowed_user_object_id;	/* Bytes 80-87 */
	uint64_t	ucod_allowed_range_length;	/* Bytes 88-95 */
	uint64_t	ucod_allowed_range_start_byte_addr;   /* Bytes 96-103 */
} osd_user_object_descriptor_format_t;


/*
 * PAR (Partition) capability object descriptor.
 * Bytes 60-103 of the OSD capability format (osd_capability_format_t).
 * Byte offset within osd_capability_format_t (not in osd_cdb_t)
 */
typedef struct osd_partition_descriptor_format {
	uint32_t	pcod_policy_access_tag;		/* Bytes 60-63 */
	uint16_t	pcod_boot_epoch;		/* Bytes 64-65 */
	uint8_t		pcod_reserved66[6];		/* Bytes 66-71 */
	uint64_t	pcod_allowed_partition_id;	/* Bytes 72-79 */
	uint8_t		pcod_reserved80[24];		/* Bytes 80-103 */
} osd_partition_descriptor_format_t;


/*
 * COLL (Collection) capability object descriptor.
 * Bytes 60-103 of the OSD capability format (osd_capability_format_t).
 * Byte offset within osd_capability_format_t (not in osd_cdb_t)
 */
typedef struct osd_collection_descriptor_format {
	uint32_t	ccod_policy_access_tag;		/* Bytes 60-63 */
	uint16_t	ccod_boot_epoch;		/* Bytes 64-65 */
	uint8_t		ccod_reserved66[6];		/* Bytes 66-71 */
	uint64_t	ccod_allowed_partition_id;	/* Bytes 72-79 */
	uint64_t	ccod_allowed_coll_object_id;    /* Bytes 80-87 */
	uint8_t		ccod_reserved88[16];		/* Bytes 88-103 */
} osd_collection_descriptor_format_t;


/*
 * OSD capability format.  Bytes 80-183 in the OSD CDB.
 * (Note: the byte offsets in the comments below are the offset within this
 * structure, not the offset within the CDB. This is intended to match the
 * presentation in the T10 spec.)
 */
typedef struct osd_capability_format {

#if defined(_BIT_FIELDS_LTOH)
	uint8_t	ocap_capability_format	: 4,		/* Byte 0 */
		ocap_reserved0		: 4;

	uint8_t	ocap_integrity_check_value_algorithm	/* Byte 1 */
					: 4,
		ocap_key_version	: 4;

	uint8_t	ocap_security_method	: 4,		/* Byte 2 */
		ocap_reserved2		: 4;

#else

	uint8_t	ocap_reserved0		: 4,		/* Byte 0 */
		ocap_capability_format	: 4;

	uint8_t	ocap_key_version	: 4,		/* Byte 1 */
		ocap_integrity_check_value_algorithm
					: 4;

	uint8_t	ocap_reserved2		: 4,		/* Byte 2 */
		ocap_security_method	: 4;
#endif

	uint8_t	ocap_reserved3;				/* Byte 3 */
	uint8_t ocap_capability_expiration_time[6];	/* Bytes 4-9 */
	uint8_t	ocap_audit[20];				/* Bytes 10-29 */
	uint8_t	ocap_capability_discriminator[12];	/* Bytes 30-41 */
	uint8_t	ocap_object_created_time[6];		/* Bytes 42-47 */
	uint8_t	ocap_object_type;			/* Byte 48 */

#if defined(_BIT_FIELDS_LTOH)
	uint8_t	ocap_permissions_bit_mask_append	: 1, /* Byte 49 */
		ocap_permissions_bit_mask_obj_mgmt	: 1,
		ocap_permissions_bit_mask_remove	: 1,
		ocap_permissions_bit_mask_create	: 1,
		ocap_permissions_bit_mask_set_attr	: 1,
		ocap_permissions_bit_mask_get_attr	: 1,
		ocap_permissions_bit_mask_write		: 1,
		ocap_permissions_bit_mask_read		: 1;

	uint8_t	ocap_permissions_bit_mask_reserved50	: 3, /* Byte 50 */
		ocap_permissions_bit_mask_query		: 1,
		ocap_permissions_bit_mask_mobject	: 1,
		ocap_permissions_bit_mask_polsec	: 1,
		ocap_permissions_bit_mask_global	: 1,
		ocap_permissions_bit_mask_dev_mgmt	: 1;
#else
	uint8_t	ocap_permissions_bit_mask_read		: 1, /* Byte 49 */
		ocap_permissions_bit_mask_write		: 1,
		ocap_permissions_bit_mask_get_attr	: 1,
		ocap_permissions_bit_mask_set_attr	: 1,
		ocap_permissions_bit_mask_create	: 1,
		ocap_permissions_bit_mask_remove	: 1,
		ocap_permissions_bit_mask_obj_mgmt	: 1,
		ocap_permissions_bit_mask_append	: 1;

	uint8_t	ocap_permissions_bit_mask_dev_mgmt	: 1, /* Byte 50 */
		ocap_permissions_bit_mask_global	: 1,
		ocap_permissions_bit_mask_polsec	: 1,
		ocap_permissions_bit_mask_mobject	: 1,
		ocap_permissions_bit_mask_query		: 1,
		ocap_permissions_bit_mask_reserved50	: 3;
#endif

	uint8_t	ocap_permissions_bit_mask_reserved51;	/* Byte 51 */
	uint8_t	ocap_permissions_bit_mask_reserved52;	/* Byte 52 */
	uint8_t	ocap_permissions_bit_mask_reserved53;	/* Byte 53 */

	uint8_t	ocap_reserved54;			/* Byte 54 */

#if defined(_BIT_FIELDS_LTOH)
	uint8_t	ocal_reserved55			: 4,	/* Byte 55 */
		ocap_object_descriptor_type	: 4;
#else
	uint8_t	ocap_object_descriptor_type	: 4,	/* Byte 55 */
		ocal_reserved55			: 4;
#endif

	uint32_t ocap_allowed_attributes_access;	/* Bytes 56-59 */

	union {						/* Bytes 60-103 */
		uint8_t	ocap_u_object_descriptor[44];
		osd_user_object_descriptor_format_t
			ocap_u_object_descriptor_user;
		osd_partition_descriptor_format_t
			ocap_u_object_descriptor_par;
		osd_collection_descriptor_format_t
			ocap_u_object_descriptor_coll;
	} ocap_u;

} osd_capability_format_t;


#define	ocap_object_descriptor		ocap_u.ocap_u_object_descriptor
#define	ocap_object_descriptor_user	ocap_u.ocap_u_object_descriptor_user
#define	ocap_object_descriptor_par	ocap_u.ocap_u_object_descriptor_par
#define	ocap_object_descriptor_coll	ocap_u.ocap_u_object_descriptor_coll


/*
 * Values for the ocap_capability_format field above.
 */
#define	OSD_CAPABILITY_FORMAT_NO_CAPABILITY	0x0
#define	OSD_CAPABILITY_FORMAT_OSD1		0x1
#define	OSD_CAPABILITY_FORMAT_OSD2		0x2


/*
 * Values for the ocap_object_type field above.
 */
#define	OSD_OBJECT_TYPE_ROOT		0x01
#define	OSD_OBJECT_TYPE_PARTITION	0x02
#define	OSD_OBJECT_TYPE_COLLECTION	0x40
#define	OSD_OBJECT_TYPE_USER		0x80


/*
 * Values for the ocap_object_descriptor_type field above.
 */
#define	OSD_OBJECT_DESCRIPTOR_TYPE_NONE	0x00	/* Ignored */
#define	OSD_OBJECT_DESCRIPTOR_TYPE_USER	0x01	/* User */
#define	OSD_OBJECT_DESCRIPTOR_TYPE_PAR	0x02	/* Partition */
#define	OSD_OBJECT_DESCRIPTOR_TYPE_COLL	0x03	/* Collection */



/*
 * OSD Request nonce format.
 */
typedef struct osd_request_nonce_format {
	uint8_t	ornf_timestamp[6];	/* Bytes 0-5 */
	uint8_t	ornf_random_number[6];	/* Bytes 6-11 */
} osd_request_nonce_format_t;


/*
 * Security parameters format for the OSD CDB.
 * Bytes 184-223 in the OSD CDB.
 */
typedef struct osd_security_parameters {

	/* Bytes 184-203 in the OSD CDB */
	uint8_t		osp_request_integrity_check_value[20];

	/* Bytes 204-215 in the OSD CDB */
	osd_request_nonce_format_t
		osp_request_nonce;

	/* Bytes 216-219 in the OSD CDB */
	osd_offset_field_format_t
		osp_data_in_integrity_check_value_offset;

	/* Bytes 220-223 in the OSD CDB */
	osd_offset_field_format_t
		osp_data_out_integrity_check_value_offset;

} osd_security_parameters_t;



/*
 * Structs for the service-action specific fields in bytes 16-51 of
 * the OSD CDB. Some of the OSD service actions also make use of certain
 * bits in bytes 10 & 11 of the OSD CDB. Such bit definitions, if any,
 * are given after each service-action specific struct.
 */


/*
 * Bytes 16-51 in the CDB, for access to 64-bit fields that are not
 * located at 64-bit offsets from the base of the CDB.
 */
#ifdef L8R
/* Change the following struct to use 64bit values */
typedef struct osd_cmd_sa {
	uint8_t		ocmd_sa[20];		/* CDB bytes 16-35 */
	uint32_t	ocmd_sa0;		/* CDB bytes 36-39 */
	uint32_t	ocmd_sa1;		/* CDB bytes 40-43 */
	uint32_t	ocmd_sa2;		/* CDB bytes 44-47 */
	uint32_t	ocmd_sa3;		/* CDB bytes 48-51 */
} osd_cmd_sa_t;
#endif /* L8R */

typedef struct osd_cmd_sa {
	uint8_t		ocmd_sa[20];		/* CDB bytes 16-35 */
	uint64_t	ocmd_sa0;		/* CDB bytes 36-43 */
	uint64_t	ocmd_sa2;		/* CDB bytes 44-51 */
} osd_cmd_sa_t;


/*
 * Macros to set 64-bit fields in the CDB at offsets that are not a
 * multiple of 64 bits.
 */
#define	OSD_WRITE64_AT_CDB_OFFSET_36(cdbp, val64)			\
	SCSI_WRITE64(&((cdbp)->ocdb_sa_u.ocdb_u_sa.ocmd_sa0), val64);

#define	OSD_WRITE64_AT_CDB_OFFSET_44(cdbp, val64)			\
	SCSI_WRITE64(&((cdbp)->ocdb_sa_u.ocdb_u_sa.ocmd_sa2), val64);


/*
 * Macro to set 64-bit field at a pointer offset that is not a
 * multiple of 64 bits.
 */
#define	OSD_WRITE64_AT_ADDR(p64, val64)					\
{									\
	uint32_t *p32 = (uint32_t *)(p64);				\
	*p32 = ((uint32_t)((val64) >> 32));				\
	*(p32 + 1) = ((uint32_t)(BMASK_32(val64)));			\
}

/*
 * Bytes 16-51 in the OSD CDB for a generic command
 * READ, WRITE, CREATE_AND_WRITE, FLUSH, CLEAR, PUNCH are all generic
 * Defining a separate struct (instead of typedef'ing) to maintain granularity
 */
typedef struct osd_cmd_generic {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_length;			/* Bytes 32-39 */
	uint64_t	ocmd_starting_byte_address;	/* Bytes 40-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_generic_t;


/*
 * Bytes 16-51 in the OSD CDB for the APPEND command.
 */
typedef struct osd_cmd_append {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_length;			/* Bytes 32-39 */
	uint8_t		ocmd_reserved[12];		/* Bytes 40-51 */
} osd_cmd_append_t;

/*
 * Bytes 16-51 for the CLEAR command.
 */
typedef struct osd_cmd_clear {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_clear_length;		/* Bytes 32-39 */
	uint64_t	ocmd_starting_byte_address;	/* Bytes 40-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_clear_t;

/*
 * Bytes 16-51 in the OSD CDB for the CREATE command.
 */
typedef struct osd_cmd_create {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_requested_user_object_id;	/* Bytes 24-31 */
	uint16_t	ocmd_number_of_user_objects;	/* Bytes 32-33 */
	uint8_t		ocmd_reserved[18];		/* Bytes 34-51 */
} osd_cmd_create_t;


/*
 * Bytes 16-51 in the OSD CDB for the CREATE AND WRITE command.
 */
typedef struct osd_cmd_create_and_write {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_requested_user_object_id;	/* Bytes 24-31 */
	uint64_t	ocmd_length;			/* Bytes 32-39 */
	uint64_t	ocmd_starting_byte_address;	/* Bytes 40-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_create_and_write_t;


/*
 * Bytes 16-51 in the OSD CDB for the CREATE COLLECTION command.
 */
typedef struct osd_cmd_create_collection {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_requested_collection_object_id; /* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_create_collection_t;


/*
 * Bytes 16-51 in the OSD CDB for the CREATE PARTITION command.
 */
typedef struct osd_cmd_create_partition {
	uint64_t	ocmd_requested_partition_id;	/* Bytes 16-23 */
	uint8_t		ocmd_reserved[28];		/* Bytes 24-51 */
} osd_cmd_create_partition_t;


/*
 * FLUSH SCOPE bits in the CDB for the FLUSH, FLUSH COLLECTION, FLUSH OSD,
 * and FLUSH PARTITION commands.  The flush scope bits are bits 0-1 in the
 * OPTIONS BYTE (byte 10) of the OSD CDB.
 */
#define	OSD_FLUSH_SCOPE_USER_DATA_AND_ATTRIBUTES	0x0
#define	OSD_FLUSH_SCOPE_USER_ATTRIBUTES_ONLY		0x1
#define	OSD_FLUSH_SCOPE_USER_DATA_RANGE_AND_ATTRIBUTES	0x2
#define	OSD_FLUSH_SCOPE_MASK				0x3


/*
 * Bytes 16-51 in the OSD CDB for the FLUSH command.
 */
typedef struct osd_cmd_flush {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_length;			/* Bytes 32-39 */
	uint64_t	ocmd_starting_byte_address;	/* Bytes 40-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_flush_t;


/*
 * Bytes 16-51 in the OSD CDB for the FLUSH COLLECTION command.
 */
typedef struct osd_cmd_flush_collection {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_collection_object_id;	/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_flush_collection_t;

/* The FLUSH SCOPE bits defined for the FLUSH COLLECTION command. */
#define	OSD_FLUSH_COLLECTION_SCOPE_USER_OBJECTS			0x0
#define	OSD_FLUSH_COLLECTION_SCOPE_ATTRIBUTES_ONLY		0x1
#define	OSD_FLUSH_COLLECTION_SCOPE_USER_OBJECTS_AND_ATTRIBUTES	0x2


/*
 * Bytes 16-51 in the OSD CDB for the FLUSH OSD command.
 */
typedef struct osd_cmd_flush_osd {
	uint8_t		ocmd_reserved[36];		/* Bytes 16-51 */
} osd_cmd_flush_osd_t;

/*
 * The FLUSH SCOPE bits defined for the FLUSH OSD command.
 * See the T10 spec for complete details of the 0x2 scope value.
 */
#define	OSD_FLUSH_OSD_SCOPE_PARTITION_LIST			0x0
#define	OSD_FLUSH_OSD_SCOPE_ROOT_OBJECT_ATTRIBUTES_ONLY		0x1
#define	OSD_FLUSH_OSD_SCOPE_ALL					0x2


/*
 * Bytes 16-51 in the OSD CDB for the FLUSH PARTITION command.
 */
typedef struct osd_cmd_flush_partition {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint8_t		ocmd_reserved[28];		/* Bytes 24-51 */
} osd_cmd_flush_partition_t;

/*
 * The FLUSH SCOPE bits defined for the FLUSH PARTITION command.
 * See the T10 spec for complete details of the 0x2 scope value.
 */
#define	OSD_FLUSH_PARTITION_SCOPE_USER_OBJECTS_AND_COLLECTIONS	0x0
#define	OSD_FLUSH_PARTITION_SCOPE_ATTRIBUTES_ONLY		0x1
#define	OSD_FLUSH_PARTITION_SCOPE_ALL				0x2


/*
 * Bytes 16-51 in the OSD CDB for the FORMAT OSD command.
 */
typedef struct osd_cmd_format_osd {
	uint8_t		ocmd_reserved16[16];		/* Bytes 16-31 */
	uint64_t	ocmd_formatted_capacity;	/* Bytes 32-39 */
	uint8_t		ocmd_reserved44[8];		/* Bytes 44-51 */
} osd_cmd_format_osd_t;


/*
 * Bytes 16-51 in the OSD CDB for the GET ATTRIBUTES command.
 */
typedef struct osd_cmd_get_attributes {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_get_attributes_t;


/*
 * Bytes 16-51 in the OSD CDB for the GET MEMBER ATTRIBUTES command.
 */
typedef struct osd_cmd_get_member_attributes {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_collection_object_id;	/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_get_member_attributes_t;


/*
 * Bytes 16-51 in the OSD CDB for the LIST command.
 */

#define	ocdb_list_attr	ocdb_reserved11_6

typedef struct osd_cmd_list {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint8_t		ocmd_reserved[8];		/* Bytes 24-31 */
	uint64_t	ocmd_allocation_length;		/* Bytes 32-39 */
	uint64_t	ocmd_initial_object_id;		/* Bytes 40-47 */
	uint32_t	ocmd_list_identifier;		/* Bytes 48-51 */
} osd_cmd_list_t;

/*
 * The LIST command uses bits 0-3 of the Command Specific Options field
 * in byte 11 of the OSD CDB to specify the sort order.  (All unspecified
 * values are reserved.)
 */
#define	OSD_LIST_SORT_ORDER_ASCENDING_NUMERIC		0x0

/*
 * LIST command parameter data format.
 * The actual parameter data starts at byte 24 (after this struct).
 */
typedef struct osd_list_parameter_data {
	uint64_t	olpd_additional_length;		/* Bytes 0-7 */
	uint64_t	olpd_continuation_object_id;	/* Bytes 8-15 */
	uint32_t	olpd_list_identifier;		/* Bytes 16-19 */
	uint8_t		olpd_reserved[3];		/* Bytes 20-22 */
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		olpd_obsolete	: 1,		/* Byte 23 */
			olpd_lstchg	: 1,
			olpd_object_descriptor_format	: 6;
#else
	uint8_t		olpd_object_descriptor_format	: 6,	/* Byte 23 */
			olpd_lstchg	: 1,
			olpd_obsolete	: 1;
#endif
} osd_list_parameter_data_t;


/*
 * Bytes 16-51 in the OSD CDB for the LIST COLLECTION command.
 */
typedef struct osd_cmd_list_collection {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_collection_object_id;	/* Bytes 24-31 */
	uint64_t	ocmd_allocation_length;		/* Bytes 32-39 */
	uint64_t	ocmd_initial_object_id;		/* Bytes 40-47 */
	uint32_t	ocmd_list_identifier;		/* Bytes 48-51 */
} osd_cmd_list_collection_t;


/*
 * LIST COLLECTION command parameter data format.
 * The actual parameter data starts at byte 24 (after this struct).
 */
typedef struct osd_list_collection_parameter_data {
	uint64_t	olcpd_additional_length;	/* Bytes 0-7 */
	uint64_t	olcpd_continuation_object_id;	/* Bytes 8-15 */
	uint32_t	olcpd_list_identifier;		/* Bytes 16-19 */
	uint8_t		olcpd_reserved[3];		/* Bytes 20-22 */
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		olcpd_obsolete		: 1,	/* Byte 23 */
			olcpd_lstchg		: 1,
			olcpd_object_descriptor_format	: 6;
#else
	uint8_t		olcpd_object_descriptor_format	: 6,	/* Byte 23 */
			olcpd_lstchg		: 1,
			olcpd_obsolete		: 1;
#endif
} osd_list_collection_parameter_data_t;


/*
 * Object Structure Check command
 */
typedef struct osd_cmd_object_structure_check {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint8_t		ocmd_reserved[28];		/* Bytes 24-51 */
} osd_cmd_object_structure_check_t;


/*
 * Bytes 16-51 in the OSD CDB for the PERFORM SCSI COMMAND command.
 */
typedef struct osd_cmd_perform_scsi_command {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint8_t		ocmd_request_cdb[16];		/* Bytes 32-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_perform_scsi_command_t;


/*
 * Bytes 16-51 in the OSD CDB for the PERFORM TASK MANAGEMENT FUNCTION cm.
 */
typedef struct osd_cmd_perform_task_management_function {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint16_t	ocmd_allocation_length;		/* Bytes 32-33 */
	uint8_t		ocmd_reserved34[5];		/* Bytes 34-38 */
	uint8_t		ocmd_task_management_function;	/* Byte 39 */
	uint64_t	ocmd_task_tag;			/* Bytes 40-47 */
	uint8_t		ocmd_reserved48[4];		/* Bytes 48-51 */
} osd_cmd_perform_task_management_function_t;

/* Task Management Operations for the PERFORM TASK MANAGEMENT FUNCTION cmd */
#define	OSD_ABORT_TASK		0x01
#define	OSD_ABORT_TASK_SET	0x02
#define	OSD_CLEAR_TASK_SET	0x04
#define	OSD_LOGICAL_UNIT_RESET	0x08
#define	OSD_I_T_NEXUS_RESET	0x10
#define	OSD_CLEAR_ACA		0x40
#define	OSD_QUERY_TASK		0x80
#define	OSD_QUERY_TASK_SET	0x81
#define	OSD_QUERY_UA		0x82

/*
 * Perform Task Mangement Function parameter data format
 * Name shortened for some brevity.
 */
typedef struct osd_task_management_func_data {
	uint16_t	additional_length;	/* Bytes 0-1 (0006h) */
	uint8_t		reserved[2];		/* Bytes 2-3 */
	uint8_t		service_response;	/* Byte 4 */
	/* See SAM-4 for info on "Additional Response Information" */
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		sense_key	: 4,	/* Byte 5 */
			uade_depth	: 2,
			reserved5_7	: 2;
#else
	uint8_t		reserved5_7	: 2,	/* Byte 5 */
			uade_depth	: 2,
			sense_key	: 4;
#endif
	uint8_t		asc;			/* Byte 6 */
	uint8_t		ascq;			/* Byte 7 */
} osd_task_management_func_data_t;

#define	OSD_UADE_NUM_UA_ERRORS_UNKNOWN	0x0	/* # UA+deferred errs unknown */
#define	OSD_UADE_NUM_UA_ERRORS_EQ_ONE	0x1	/* # of UA+deferred is 1 */
#define	OSD_UADE_NUM_UA_ERRORS_GT_ONE	0x2	/* # of UA+deferred is >1 */

/* Service response codes - See SAM-4 */
#define	OSD_SVC_RESP_FUNC_COMPLETE	0x00
#define	OSD_SVC_RESP_FUNC_SUCCEEDED	0x05
#define	OSD_SVC_RESP_FUNC_REJECTED	0x08
#define	OSD_SVC_RESP_INCORRECT_LUN	0x09

/*
 * Bytes 16-51 in the OSD CDB for the PUNCH command.
 */
typedef struct osd_cmd_punch {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_punch_length;		/* Bytes 32-39 */
	uint64_t	ocmd_starting_byte_address;	/* Bytes 40-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_punch_t;

/*
 * Bytes 16-51 in the OSD CDB for the QUERY command.
 */
typedef struct osd_cmd_query {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_collection_object_id;	/* Bytes 24-31 */
	uint64_t	ocmd_allocation_length;		/* Bytes 32-39 */
	uint8_t		ocmd_reserved[8];		/* Bytes 40-47 */
	uint32_t	ocmd_list_length;		/* Bytes 48-51 */
} osd_cmd_query_t;

/*
 * Query list format
 */
typedef struct osd_query_list_format {
	uint8_t		query_type;			/* Byte 0 */
	uint8_t		reserved[3];			/* Bytes 1-3 */
	/* Followed by >= 1 query criteria entries */
} osd_query_list_format_t;

#define	OSD_QUERY_TYPE_MATCH_ANY	0x00
#define	OSD_QUERY_TYPE_MATCH_ALL	0x01

typedef struct osd_query_criteria_entry {
	uint16_t	reserved;			/* Bytes 0-1 */
	uint16_t	entry_length;			/* Bytes 2-3 */
	uint32_t	attributes_page;		/* Bytes 4-7 */
	uint32_t	attribute_number;		/* Bytes 8-11 */
	uint16_t	min_attribute_value_length;	/* Bytes 12-13 */
	/* min_attribute_value				   Bytes 14-m */
	/* uint16_t max_attribute_value_length;		   Bytes m+1-m+3 */
	/* max_attribute_value				   Bytes m+3-n */
} osd_query_criteria_entry_t;

typedef struct osd_matches_list_data {
	uint64_t	additional_length;		/* Bytes 0-7 */
	uint32_t	reserved8;			/* Bytes 8-11 */
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		reserved12_0		: 2,	/* Byte 12 (0x21) */
			object_descriptor_format: 6;
#else
	uint8_t		object_descriptor_format: 6,	/* Byte 12 (0x21) */
			reserved12_0		: 2;
#endif
	/* Followed by object descriptor list entries */
} osd_matches_list_data_t;


/*
 * Bytes 16-51 in the OSD CDB for the READ MAP command.
 */
typedef struct osd_cmd_read_map {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_allocation_length;		/* Bytes 32-39 */
	uint64_t	ocmd_data_map_byte_offset;	/* Bytes 40-47 */
	uint16_t	ocmd_requested_map_type;	/* Bytes 48-49 */
	uint8_t		ocmd_reserved[2];		/* Bytes 50-51 */
} osd_cmd_read_map_t;

#define	OSD_READ_MAP_TYPE_ALL		0x0000		/* All map type vals */
#define	OSD_READ_MAP_TYPE_WRITTEN_DATA	0x0001		/* WRITTEN_DATA vals */
#define	OSD_READ_MAP_TYPE_DATA_HOLE	0x0002		/* DATA_HOLE vals */
#define	OSD_READ_MAP_TYPE_DAMAGED_DATA	0x0003		/* DAMAGED_DATA vals */
#define	OSD_READ_MAP_TYPE_DAMAGED_ATTRS	0x8000		/* DAMAGED_ATTRS vals */

#define	OSD_MAP_DESC_WRITTEN_DATA	OSD_READ_MAP_TYPE_WRITTEN_DATA
#define	OSD_MAP_DESC_DATA_HOLE		OSD_READ_MAP_TYPE_DATA_HOLE
#define	OSD_MAP_DESC_DAMAGED_DATA	OSD_READ_MAP_TYPE_DAMAGED_DATA
#define	OSD_MAP_DESC_DAMAGED_ATTRS	OSD_READ_MAP_TYPE_DAMAGED_ATTRS

typedef struct osd_map_descriptor {
	uint16_t		reserved;		/* Bytes 0-1 */
	uint16_t		descriptor_type;	/* Bytes 2-3 */
	uint32_t		data_length;		/* Bytes 4-7 */
	uint64_t		byte_offset;		/* Bytes 8-15 */
} osd_map_descriptor_t;

typedef struct osd_read_map_data {
	uint64_t		additional_length;	/* Bytes 0-7 */
	osd_map_descriptor_t	map_descriptor[1];	/* Bytes 8-23 ... */
} osd_read_map_data_t;


/*
 * Bytes 16-51 in the OSD CDB for the READ command.
 */
typedef struct osd_cmd_read {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_length;			/* Bytes 32-39 */
	uint64_t	ocmd_starting_byte_address;	/* Bytes 40-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_read_t;


/*
 * Bytes 16-51 in the OSD CDB for the REMOVE command.
 */
typedef struct osd_cmd_remove {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_remove_t;


/*
 * Bytes 16-51 in the OSD CDB for the REMOVE COLLECTION command.
 */
typedef struct osd_cmd_remove_collection {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_collection_object_id;	/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_remove_collection_t;

/*
 * The REMOVE COLLECTION command uses bit 0 of the Command Specific Options
 * field in byte 11 of the OSD CDB as the FCR (force collection removal) bit.
 */
#define	OSD_REMOVE_COLLECTION_FCR	0x1


/*
 * Bytes 16-51 in the OSD CDB for the REMOVE MEMBER OBJECTS command.
 */
typedef struct osd_cmd_remove_member_objects {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_collection_object_id;	/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_remove_member_objects_t;

/*
 * Bytes 16-51 in the OSD CDB for the REMOVE PARTITION command.
 */
typedef struct osd_cmd_remove_partition {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint8_t		ocmd_reserved24[28];		/* Bytes 24-51 */
} osd_cmd_remove_partition_t;


/*
 * Bytes 16-51 in the OSD CDB for the SET ATTRIBUTES command.
 */
typedef struct osd_cmd_set_attributes {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_set_attributes_t;


/*
 * Definition of CDB byte 11 for the SET KEY command.
 */

typedef struct ocdb_set_key_cs_opts {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t	ocdb_key_to_set			: 2,
		ocdb_rsvd2			: 2,
		ocdb_getset_cdbfmt		: 2,
		ocdb_rsvd6			: 2;
#elif defined(_BIT_FIELDS_HTOL)
	uint8_t	ocdb_rsvd6			: 2,
		ocdb_getset_cdbfmt		: 2,
		ocdb_rsvd2			: 2,
		ocdb_key_to_set			: 2;
#else
#error	One of _BIT_FIELDS_LTOH or _BIT_FIELDS_HTOL must be defined
#endif
} ocdb_set_key_cs_options_t;

/*
 * Bytes 16-51 in the OSD CDB for the SET KEY command.
 */
typedef struct osd_cmd_set_key {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		ocmd_key_version	: 4,	/* Byte 24 */
			ocmd_reserved		: 4;
#else
	uint8_t		ocmd_reserved		: 4,	/* Byte 24 */
			ocmd_key_version	: 4;
#endif
	uint8_t		ocmd_key_identifier[7];		/* Bytes 25-31 */
	uint8_t		ocmd_seed[20];			/* Bytes 32-51 */
} osd_cmd_set_key_t;

/*
 * The SET KEY command uses bits 0-1 of the Command Specific Options field
 * in byte 11 of the OSD CDB as the KEY TO SET value.
 */
#define	OSD_KEY_TO_SET_ROOT		0x1
#define	OSD_KEY_TO_SET_PARTITION	0x2
#define	OSD_KEY_TO_SET_WORKING		0x3

/*
 * Reuse the SET_KEY version of CDB byte 11 since DH_STEP is the same
 * bit field as ocdb_key_to_set.
 */

#define	ocdb_dh_step			ocdb_key_to_set

/*
 * Bytes 16-51 in the OSD CDB for the SET MASTER KEY command.
 */
typedef struct osd_cmd_set_master_key {
	uint8_t		ocmd_reserved16[8];		/* Bytes 16-23 */
	uint8_t		ocmd_dh_group;			/* Byte 24 */
	uint8_t		ocmd_key_identifier[7];		/* Bytes 25-31 */
	uint32_t	ocmd_parameter_list_length;	/* Bytes 32-35 */
	uint32_t	ocmd_allocation_length;		/* Bytes 36-39 */
	uint8_t		ocmd_reserved40[12];		/* Bytes 40-51 */
} osd_cmd_set_master_key_t;

/*
 * The SET MASTER KEY command uses bits 0-1 of the Command Specific Options
 * field in byte 11 of the OSD CDB as the Diffie-Hellman step value.
 * (All unspecified values are reserved.)
 */
#define	OSD_DH_STEP_SEED_EXCHANGE		0x0
#define	OSD_DH_STEP_CHANGE_MASTER_KEY		0x1


/*
 * Seed exchange device server DH_data format (response to the SET MASTER KEY
 * command with the OSD_DH_STEP_SEED_EXCHANGE step).
 *
 * Bytes 0-3 are the osde_response_length (n-3).
 * Bytes 4-n are the device server DH_DATA.
 */
typedef struct osd_dh_data_seed_exchange {
	uint32_t	odse_response_length;	/* Bytes 0-3 */
} osd_dh_data_seed_exchange_t;


/*
 * Change Master Key DH_data format (response to the SET MASTER KEY command
 * with the OSD_DH_STEP_CHANGE_MASTER_KEY step).
 *
 * Bytes 0-3 are the osmkc_application_client_data_length (k-3).
 * Bytes 4-k are the application client DH_DATA.
 * Bytes k+1 thru k+4  are the osmks_device_server_data_length (n-(k+4)).
 * Bytes k+4 thru n are the device server DH_DATA.
 */
typedef struct osd_dh_data_set_master_key_client {
	/* Bytes 0-3 */
	uint32_t	osmkc_application_client_data_length;
} osd_dh_data_set_master_key_client_t;

typedef struct osd_dh_data_set_master_key_server {
	/* Bytes (k+1) thru (k+4) */
	uint32_t	osmks_device_server_data_length;
} osd_dh_data_set_master_key_server_t;

/*
 * Bytes 16-51 in the OSD CDB for the SET MEMBER ATTRIBUTES command.
 */
typedef struct osd_cmd_set_member_attributes {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_collection_object_id;	/* Bytes 24-31 */
	uint8_t		ocmd_reserved[20];		/* Bytes 32-51 */
} osd_cmd_set_member_attributes_t;


/*
 * Bytes 16-51 in the OSD CDB for the WRITE command.
 */
typedef struct osd_cmd_write {
	uint64_t	ocmd_partition_id;		/* Bytes 16-23 */
	uint64_t	ocmd_user_object_id;		/* Bytes 24-31 */
	uint64_t	ocmd_length;			/* Bytes 32-39 */
	uint64_t	ocmd_starting_byte_address;	/* Bytes 40-47 */
	uint8_t		ocmd_reserved[4];		/* Bytes 48-51 */
} osd_cmd_write_t;


typedef union ocdb_options {
#if defined(_BIT_FIELDS_LTOH)
		uint8_t	ocdb_u_options_byte;
		uint8_t	ocdb_u_options_isolation	: 3,
			ocdb_u_options_fua		: 1,
			ocdb_u_options_dpo		: 1,
			ocdb_u_options_reserved		: 3;
#else
		uint8_t	ocdb_u_options_byte;
		uint8_t	ocdb_u_options_reserved		: 3,
			ocdb_u_options_dpo		: 1,
			ocdb_u_options_fua		: 1,
			ocdb_u_options_isolation	: 3;
#endif	/* _BIT_FIELDS_LTOH */
} ocdb_options_u_t;

typedef union ocdb_cs_options_u {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t	ocdb_u_cs_options_byte;
	uint8_t	ocdb_u_command_specific_options	: 4,
		ocdb_u_getset_cdbfmt		: 2,
		ocdb_u_reserved11_6		: 1,
		ocdb_u_reserved11_7		: 1;
#else
	uint8_t	ocdb_u_cs_options_byte;
	uint8_t	ocdb_u_reserved11_7		: 1,
		ocdb_u_reserved11_6		: 1,
		ocdb_u_getset_cdbfmt		: 2,
		ocdb_u_command_specific_options	: 4;
#endif
} ocdb_cs_options_u_t;

/*
 * Basic SCSI OSD CDB format.
 * Based upon SPC-3 variable length CDB format.
 */
typedef	struct osd_cdb	{

	/* Byte 0: Operation Code (see below) */
	uint8_t	ocdb_operation_code;

	/* Byte 1: Control Byte, as per SAM-3 */
	uint8_t	ocdb_control;

	/* Bytes 2-6: T10 reserved bytes */
	uint8_t	ocdb_reserved2[5];

	/* Byte 7: Additional CDB length. */
	uint8_t	ocdb_additional_cdb_length;

	/* Bytes 8-9: Service Action Code: 16-bit, big-endian. */
	uint16_t ocdb_service_action;

	/* Byte 10: Service Action Options */
	ocdb_options_u_t ocdb_options;

	/* Byte 11: Additional command-specific options. */
	ocdb_cs_options_u_t ocdb_cs_options;

	/* Byte 12: timestamps control */
	uint8_t	ocdb_timestamps_control;

	/* Bytes 13-15: T10 reserved bytes */
	uint8_t	ocdb_reserved13[3];

	/*
	 * Bytes 16-51: The content of these varies depending upon the
	 * specific service action.
	 */
	union {
		osd_cmd_generic_t		ocdb_u_generic;
		osd_cmd_append_t		ocdb_u_append;
		osd_cmd_clear_t			ocdb_u_clear;
		osd_cmd_create_t		ocdb_u_create;
		osd_cmd_create_and_write_t	ocdb_u_create_and_write;
		osd_cmd_create_collection_t	ocdb_u_create_collection;
		osd_cmd_create_partition_t	ocdb_u_create_partition;
		osd_cmd_flush_t			ocdb_u_flush;
		osd_cmd_flush_collection_t	ocdb_u_flush_collection;
		osd_cmd_flush_osd_t		ocdb_u_flush_osd;
		osd_cmd_flush_partition_t	ocdb_u_flush_partition;
		osd_cmd_format_osd_t		ocdb_u_format_osd;
		osd_cmd_get_attributes_t	ocdb_u_get_attributes;
		osd_cmd_get_member_attributes_t	ocdb_u_get_member_attributes;
		osd_cmd_list_t			ocdb_u_list;
		osd_cmd_list_collection_t	ocdb_u_list_collection;
		osd_cmd_object_structure_check_t ocdb_u_object_structure_check;
		osd_cmd_perform_scsi_command_t	ocdb_u_perform_scsi_command;
		osd_cmd_perform_task_management_function_t
		    ocdb_u_perform_task_management_function;
		osd_cmd_punch_t			ocdb_u_punch;
		osd_cmd_query_t			ocdb_u_query;
		osd_cmd_read_t			ocdb_u_read;
		osd_cmd_read_map_t		ocdb_u_read_map;
		osd_cmd_remove_t		ocdb_u_remove;
		osd_cmd_remove_collection_t	ocdb_u_remove_collection;
		osd_cmd_remove_member_objects_t	ocdb_u_remove_member_objects;
		osd_cmd_remove_partition_t	ocdb_u_remove_partition;
		osd_cmd_set_attributes_t	ocdb_u_set_attributes;
		osd_cmd_set_key_t		ocdb_u_set_key;
		osd_cmd_set_master_key_t	ocdb_u_set_master_key;
		osd_cmd_set_member_attributes_t	ocdb_u_set_member_attributes;
		osd_cmd_write_t			ocdb_u_write;
		osd_cmd_sa_t			ocdb_u_sa;
	} ocdb_sa_u;

	/*
	 * Bytes 52-79: Get and Set Attributes parameters.
	 * (CDB field, Page format and List format)
	 */
	union {
		osd_cdbfield_attribute_t	ocdb_u_cdbfield_attributes;
		osd_page_attribute_t		ocdb_u_page_attributes;
		osd_list_attribute_t		ocdb_u_list_attributes;
	} ocdb_u_attr;

	/* Bytes 80-183: Capability */
	osd_capability_format_t			ocdb_capability;

	/* Bytes 184-223: Security parameters. */
	osd_security_parameters_t		ocdb_security_parameters;

} osd_cdb_t;

#define	ocdb_options_byte		ocdb_options.ocdb_u_options_byte
#define	ocdb_options_reserved		ocdb_options.ocdb_u_options_reserved
#define	ocdb_options_dpo		ocdb_options.ocdb_u_options_dpo
#define	ocdb_options_fua		ocdb_options.ocdb_u_options_fua
#define	ocdb_options_isolation		ocdb_options.ocdb_u_options_isolation

#define	ocdb_cs_options_byte		ocdb_cs_options.ocdb_u_cs_options_byte
#define	ocdb_getset_cdbfmt		ocdb_cs_options.ocdb_u_getset_cdbfmt

#define	ocdb_generic			ocdb_sa_u.ocdb_u_generic
#define	ocdb_append			ocdb_sa_u.ocdb_u_append
#define	ocdb_clear			ocdb_sa_u.ocdb_u_clear
#define	ocdb_create			ocdb_sa_u.ocdb_u_create
#define	ocdb_create_and_write		ocdb_sa_u.ocdb_u_create_and_write
#define	ocdb_create_collection		ocdb_sa_u.ocdb_u_create_collection
#define	ocdb_create_partition		ocdb_sa_u.ocdb_u_create_partition
#define	ocdb_flush			ocdb_sa_u.ocdb_u_flush
#define	ocdb_flush_collection		ocdb_sa_u.ocdb_u_flush_collection
#define	ocdb_flush_osd			ocdb_sa_u.ocdb_u_flush_osd
#define	ocdb_flush_partition		ocdb_sa_u.ocdb_u_flush_partition
#define	ocdb_format_osd			ocdb_sa_u.ocdb_u_format_osd
#define	ocdb_get_attributes		ocdb_sa_u.ocdb_u_get_attributes
#define	ocdb_get_member_attributes	ocdb_sa_u.ocdb_u_get_member_attributes
#define	ocdb_list			ocdb_sa_u.ocdb_u_list
#define	ocdb_list_collection		ocdb_sa_u.ocdb_u_list_collection
#define	ocdb_object_structure_check	ocdb_sa_u.ocdb_u_object_structure_check
#define	ocdb_perform_scsi_command	ocdb_sa_u.ocdb_u_perform_scsi_command
#define	ocdb_perform_task_management_function \
			ocdb_sa_u.ocdb_u_perform_task_management_function
#define	ocdb_punch			ocdb_sa_u.ocdb_u_punch
#define	ocdb_query			ocdb_sa_u.ocdb_u_query
#define	ocdb_read_map			ocdb_sa_u.ocdb_u_read_map
#define	ocdb_read			ocdb_sa_u.ocdb_u_read
#define	ocdb_remove			ocdb_sa_u.ocdb_u_remove
#define	ocdb_remove_collection		ocdb_sa_u.ocdb_u_remove_collection
#define	ocdb_remove_member_objects	ocdb_sa_u.ocdb_u_remove_member_objects
#define	ocdb_remove_partition		ocdb_sa_u.ocdb_u_remove_partition
#define	ocdb_set_attributes		ocdb_sa_u.ocdb_u_set_attributes
#define	ocdb_set_key			ocdb_sa_u.ocdb_u_set_key
#define	ocdb_set_master_key		ocdb_sa_u.ocdb_u_set_master_key
#define	ocdb_set_member_attributes	ocdb_sa_u.ocdb_u_set_member_attributes
#define	ocdb_write			ocdb_sa_u.ocdb_u_write


#define	ocdb_cdbfield_attributes	ocdb_u_attr.ocdb_u_cdbfield_attributes
#define	ocdb_page_attributes		ocdb_u_attr.ocdb_u_page_attributes
#define	ocdb_list_attributes		ocdb_u_attr.ocdb_u_list_attributes


/*
 * Primary OSD CDB opcode for the ocdb_operation_code field above.
 * All OSD CDBs must set this field to this value.
 */
#define	OSD_CDB_OPCODE			0x7F


/*
 * Constant for the ocdb_additional_cdb_length field above.
 * All OSD CDBs must set this field to this value.
 */
#define	OSD_ADDITIONAL_CDB_LENGTH	216


/* Capability Format and Security parameters field lengths in OSD CDB */
#define	OSD_CDB_CAPABILITY_LENGTH	(sizeof (osd_capability_format_t))
#define	OSD_CDB_SECURITY_LENGTH		(sizeof (osd_security_parameters_t))


/*
 * Service Action Codes for the ocdb_service_action field above.
 * M=Mandatory, O=Optional, Oc=Optional if collections are supported
 */
#define	OSD_APPEND				0x8887	/* M */
#define	OSD_CLEAR				0x8889	/* M */
#define	OSD_CREATE				0x8882	/* M */
#define	OSD_CREATE_AND_WRITE			0x8892	/* M */
#define	OSD_CREATE_COLLECTION			0x8895	/* Oc */
#define	OSD_CREATE_PARTITION			0x888B	/* M */
#define	OSD_FLUSH				0x8888	/* M */
#define	OSD_FLUSH_COLLECTION			0x889A	/* Oc */
#define	OSD_FLUSH_OSD				0x889C	/* M */
#define	OSD_FLUSH_PARTITION			0x889B	/* M */
#define	OSD_FORMAT_OSD				0x8881	/* O */
#define	OSD_GET_ATTRIBUTES			0x888E	/* M */
#define	OSD_GET_MEMBER_ATTRIBUTES		0x88A2	/* Oc */
#define	OSD_LIST				0x8883	/* M */
#define	OSD_LIST_COLLECTION			0x8897	/* Oc */
#define	OSD_OBJECT_STRUCTURE_CHECK		0x8880	/* M */
#define	OSD_PERFORM_SCSI_COMMAND		0x8F7C	/* M */
#define	OSD_PERFORM_TASK_MANAGEMENT_FUNCTION	0x8F7D	/* M */
#define	OSD_PUNCH				0x8884	/* M */
#define	OSD_QUERY				0x88A0	/* Oc */
#define	OSD_READ				0x8885	/* M */
#define	OSD_READ_MAP				0x88B1	/* M */
#define	OSD_REMOVE				0x888A	/* M */
#define	OSD_REMOVE_COLLECTION			0x8896	/* O (should be Oc?) */
#define	OSD_REMOVE_MEMBER_OBJECTS		0x88A1	/* Oc */
#define	OSD_REMOVE_PARTITION			0x888C	/* M */
#define	OSD_SET_ATTRIBUTES			0x888F	/* M */
#define	OSD_SET_KEY				0x8898	/* M */
#define	OSD_SET_MASTER_KEY			0x8899	/* M */
#define	OSD_SET_MEMBER_ATTRIBUTES		0x88A3	/* Oc */
#define	OSD_WRITE				0x8886	/* M */


/*
 * Defined bits for the ocdb_options_isolation field above.
 */
#define	OSD_ISOLATION_METHOD_DEFAULT	0x00
#define	OSD_ISOLATION_METHOD_NONE	0x01
#define	OSD_ISOLATION_METHOD_STRICT	0x02
#define	OSD_ISOLATION_METHOD_RANGE	0x04
#define	OSD_ISOLATION_METHOD_FUNCTIONAL	0x05
#define	OSD_ISOLATION_METHOD_VENDOR	0x07


/*
 * Defined values for the ocdb_getset_cdbfmt field above.
 */
#define	OSD_SET_1ATTR_WITH_CDB	0x1	/* CDB field Set format */
#define	OSD_GETSET_CDBFMT_PAGE	0x2	/* Page format */
#define	OSD_GETSET_CDBFMT_LIST	0x3	/* List format */


/*
 * Defined bits for the ocdb_timestamps_control field above.
 * Values 0x01 thru 0x7E and 0x80 thru 0xDF are reserved.
 * Values 0xE0 thru 0xFF are vendor-specific.
 */
#define	OSD_TIMESTAMP_CONTROL_DEFAULT	0x00	/* Default updates */
#define	OSD_TIMESTAMP_CONTROL_DISABLED	0x7F	/* No timestamp updates */




/*
 * SCSI Sense data descriptors
 */

#define	SENSE_DESC_OBJECT_IDENTIFICATION	0x06
#define	SENSE_DESC_RESPONSE_INTEGRITY_CHECK	0x07
#define	SENSE_DESC_ATTRIBUTE_IDENTIFICATION	0x08


/*
 * OSD Command function bits
 */
#define	OSD_CMDFUNC_VALIDATION		0x80000000
#define	OSD_CMDFUNC_CMD_CAP_V		0x20000000
#define	OSD_CMDFUNC_COMMAND		0x10000000
#define	OSD_CMDFUNC_IMP_ST_ATT		0x00100000
#define	OSD_CMDFUNC_SA_CAP_V		0x00002000
#define	OSD_CMDFUNC_SET_ATT		0x00001000
#define	OSD_CMDFUNC_GA_CAP_V		0x00000020
#define	OSD_CMDFUNC_GET_ATT		0x00000010


/*
 * Descriptor Type 0x06 (OSD Object Identification)
 */
typedef struct osd_error_identification_sense_data {
	uint8_t		oei_descriptor_type;
	uint8_t		oei_additional_length;
	uint8_t		oei_reserved0[6];
	uint32_t	oei_not_initiated_command_functions;
	uint32_t	oei_completed_command_functions;
	uint64_t	oei_partition_id;
	uint64_t	oei_object_id;
} osd_error_identification_sense_data_t;


/*
 * Descriptor Type 0x07 (OSD Response Integrity Check Value)
 */
typedef struct osd_response_integrity_check_value_sense_data {
	uint8_t		ori_descriptor_type;
	uint8_t		ori_affitional_length;
	uint8_t		ori_response_integrity_check_value[20];
} osd_response_integrity_check_value_sense_data_t;


/*
 * Descriptor Type 0x08 (OSD Attribute Identification)
 */
typedef struct osd_attribute_identification_sense_data {
	uint8_t		oai_descriptor_type;
	uint8_t		oai_additional_length;
	uint8_t		oai_reserved0[2];
} osd_attribute_identification_sense_data_t;


typedef struct osd_sense_data_attribute_descriptor {
	uint32_t	oda_attribute_page;
	uint32_t	oda_attribute_number;
} osd_sense_data_attribute_descriptor_t;




typedef struct osd_sense_data {
	uint8_t	osn_code;
	uint8_t	osn_key;
	uint8_t	osn_asc;
	uint8_t osn_ascq;
	uint8_t	osn_reserved0;
	uint8_t	osn_reserved1;
	uint8_t	osn_reserved2;
	uint8_t	osn_add_len;
	union	osn_un	*ies_desc;
} osd_sense_data_t;


#define	OSD_SENSE_KEY(p)	(p)->osn_key
#define	OSD_SENSE_ASC(p)	(p)->osn_asc
#define	OSD_SENSE_ASCQ(p)	(p)->osn_ascq
#define	OSD_SENSE_ASC_ASCQ(p)	(((p)->osn_asc << 8) | (p)->osn_ascq)



/*
 * OSD ASC/ASCQ Error codes, as per SPC-3
 */
#define	OSD_NO_ADDITIONAL_SENSE_INFORMATION			0x0000
#define	OSD_IO_PROCESS_TERMINATED				0x0006
#define	OSD_OPERATION_IN_PROGRESS				0x0016
#define	OSD_CLEANING_REQUESTED					0x0017
#define	OSD_LUN_NOT_READY_CAUSE_NOT_REPORTABLE			0x0400
#define	OSD_LUN_IS_IN_PROCESS_OF_BECOMING_READY			0x0401
#define	OSD_LUN_NOT_READY_INITIALIZING_COMMAND_REQUIRED		0x0402
#define	OSD_LUN_NOT_READY_MANUAL_INTERVENTION_REQUIRED		0x0403
#define	OSD_LUN_NOT_READY_FORMAT_IN_PROGRESS			0x0404
#define	OSD_LUN_NOT_READY_OPERATION_IN_PROGRESS			0x0407
#define	OSD_LUN_NOT_READY_SELF_TEST_IN_PROGRESS			0x0409
#define	OSD_LUN_NOT_ACCESSIBLE_ASYMMETRIC_ACCESS_STATE_TRANSITION	0x040A
#define	OSD_LUN_NOT_ACCESSIBLE_TARGET_PORT_IN_STANDBY_STATE 	0x040B
#define	OSD_LUN_NOT_ACCESSIBLE_TARGET_PORT_IN_UNAVAILABLE_STATE	0x040C
#define	OSD_LUN_NOT_READY_NOTIFY_ENABLE_SPINUP_REQUIRED		0x0411
#define	OSD_LUN_DOES_NOT_RESPOND_TO_SELECTION			0x0500
#define	OSD_LUN_COMMUNICATION_FAILURE				0x0800
#define	OSD_LUN_COMMUNICATION_TIME_OUT				0x0801
#define	OSD_LUN_COMMUNICATION_PARITY_ERROR			0x0802
#define	OSD_ERROR_LOG_OVERFLOW					0x0A00
#define	OSD_WARNING						0x0B00
#define	OSD_WARNING_SPECIFIED_TEMPERATURE_EXCEEDED		0x0B01
#define	OSD_WARNING_ENCLOSURE_DEGRADED				0x0B02
#define	OSD_WRITE_ERROR_UNEXPECTED_UNSOLICITED_DATA		0x0C0C
#define	OSD_WRITE_ERROR_NOT_ENOUGH_UNSOLICITED_DATA		0x0C0D
#define	OSD_INVALID_INFORMATION_UNIT				0x0E00
#define	OSD_INFORMATION_UNIT_TOO_SHORT				0x0E01
#define	OSD_INFORMATION_UNIT_TOO_LONG				0x0E02
#define	OSD_INVALID_FIELD_IN_COMMAND_INFORMATION_UNIT		0x0E03
#define	OSD_READ_ERROR_FAILED_RETRANSMISSION_REQUEST		0x1113
#define	OSD_PARAMETER_LIST_LENGTH_ERROR				0x1A00
#define	OSD_SYNCHRONOUS_DATA_TRANSFER_ERROR			0x1B00
#define	OSD_INVALID_COMMAND_OPERATION_CODE			0x2000
#define	OSD_INVALID_FIELD_IN_CDB				0x2400
#define	OSD_CDB_DECRYPTION_ERROR				0x2401
#define	OSD_SECURITY_AUDIT_VALUE_FROZEN				0x2404
#define	OSD_SECURITY_WORKING_KEY_FROZEN				0x2405
#define	OSD_NONCE_NOT_UNIQUE					0x2406
#define	OSD_NONCE_TIMESTAMP_OUT_OF_RANGE			0x2407
#define	OSD_LOGICAL_UNIT_NOT_SUPPORTED				0x2500
#define	OSD_INVALID_FIELD_IN_PARAMETER_LIST			0x2600
#define	OSD_PARAMETER_NOT_SUPPORTED				0x2601
#define	OSD_PARAMETER_VALUE_INVALID				0x2602
#define	OSD_INVALID_RELEASE_OF_PERSISTENT_RESERVATION		0x2604
#define	OSD_INVALID_DATA_OUT_BUFFER_INTEGRITY_CHECK_VALUE	0x260F
#define	OSD_NOT_READY_TO_READY_CHANGE_MEDIUM_MAY_HAVE_CHANGED	0x2800
#define	OSD_POWER_ON_RESET_OR_BUS_DEVICE_RESET_OCCURRED		0x2900
#define	OSD_POWER_ON_OCCURRED					0x2901
#define	OSD_SCSI_BUS_RESET_OCCURRED				0x2902
#define	OSD_BUS_DEVICE_RESET_FUNCTION_OCCURRED			0x2903
#define	OSD_DEVICE_INTERNAL_RESET				0x2904
#define	OSD_TRANSCEIVER_MODE_CHANGED_TO_SINGLE_ENDED		0x2905
#define	OSD_TRANSCEIVER_MODE_CHANGED_TO_LVD			0x2906
#define	OSD_I_T_NEXUS_LOSS_OCCURRED				0x2907
#define	OSD_PARAMETERS_CHANGED					0x2A00
#define	OSD_MODE_PARAMETERS_CHANGED				0x2A01
#define	OSD_ASYMMETRIC_ACCESS_STATE_CHANGED			0x2A06
#define	OSD_IMPLICIT_ASYMMETRIC_ACCESS_STATE_TRANSITION_FAILED	0x2A07
#define	OSD_PRIORITY_CHANGED					0x2A08
#define	OSD_COMMAND_SEQUENCE_ERROR				0x2C00
#define	OSD_PREVIOUS_BUSY_STATUS				0x2C07
#define	OSD_PREVIOUS_TASK_SET_FULL_STATUS			0x2C08
#define	OSD_PREVIOUS_RESERVATION_CONFLICT_STATUS		0x2C09
#define	OSD_PARTITION_OR_COLLECTION_CONTAINS_USER_OBJECTS	0x2C0A
#define	OSD_COMMANDS_CLEARED_BY_ANOTHER_INITIATOR		0x2F00
#define	OSD_CLEANING_FAILURE					0x3007
#define	OSD_FORMAT_COMMAND_FAILED				0x3101
#define	OSD_ENCLOSURE_FAILURE					0x3400
#define	OSD_ENCLOSURE_SERVICES_FAILURE				0x3500
#define	OSD_UNSUPPORTED_ENCLOSURE_FUNCTION			0x3501
#define	OSD_ENCLOSURE_SERVICES_UNAVAILABLE			0x3502
#define	OSD_ENCLOSURE_SERVICES_TRANSFER_FAILURE			0x3503
#define	OSD_ENCLOSURE_SERVICES_TRANSFER_REFUSED			0x3504
#define	OSD_ENCLOSURE_SERVICES_CHECKSUM_ERROR			0x3505
#define	OSD_ROUNDED_PARAMETER					0x3700
#define	OSD_READ_PAST_END_OF_USER_OBJECT			0x3B17
#define	OSD_LOGICAL_UNIT_HAS_NOT_SELF_CONFIGURED_YET		0x3E00
#define	OSD_LOGICAL_UNIT_FAILURE				0x3E01
#define	OSD_TIMEOUT_ON_LOGICAL_UNIT				0x3E02
#define	OSD_LOGICAL_UNIT_FAILED_SELF_TEST			0x3E03
#define	OSD_LOGICAL_UNIT_UNABLE_TO_UPDATE_SELF_TEST_LOG		0x3E04
#define	OSD_TARGET_OPERATING_CONDITIONS_HAVE_CHANGED		0x3F00
#define	OSD_MICROCODE_HAS_BEEN_CHANGED				0x3F01
#define	OSD_INQUIRY_DATA_HAS_CHANGED				0x3F03
#define	OSD_ECHO_BUFFER_OVERWRITTEN				0x3F0F
#define	OSD_MESSAGE_ERROR					0x4300
#define	OSD_INTERNAL_TARGET_FAILURE				0x4400
#define	OSD_SELECT_OR_RESELECT_FAILURE				0x4500
#define	OSD_SCSI_PARITY_ERROR					0x4700
#define	OSD_DATA_PHASE_CRC_ERROR_DETECTED			0x4701
#define	OSD_SCSI_PARITY_ERROR_DETECTED_DURING_ST_DATA_PHASE	0x4702
#define	OSD_INFORMATION_UNIT_iuCRC_ERROR_DETECTED		0x4703
#define	OSD_ASYNCHRONOUS_INFORMATION_PROTECTION_ERROR_DETECTED	0x4704
#define	OSD_PROTOCOL_SERVICE_CRC_ERROR				0x4705
#define	OSD_PHY_TEST_FUNCTION_IN_PROGRESS			0x4706
#define	OSD_INITIATOR_DETECTED_ERROR_MESSAGE_RECEIVED		0x4800
#define	OSD_INVALID_MESSAGE_ERROR				0x4900
#define	OSD_COMMAND_PHASE_ERROR					0x4A00
#define	OSD_DATA_PHASE_ERROR					0x4B00
#define	OSD_LOGICAL_UNIT_FAILED_SELF_CONFIGURATION		0x4C00
#define	OSD_OVERLAPPED_COMMANDS_ATTEMPTED			0x4E00
#define	OSD_QUOTA_ERROR						0x5507
#define	OSD_FAILURE_PREDICTION_THRESHOLD_EXCEEDED		0x5D00
#define	OSD_FAILURE_PREDICTION_THRESHOLD_EXCEEDED_FALSE		0x5DFF
#define	OSD_VOLTAGE_FAULT					0x6500
#define	OSD_SET_TARGET_PORT_GROUPS_COMMAND_FAILED		0x670A



/*
 * Information on OSD Attributes
 */



/*
 * List entry format for retriving attributes for an OSD object
 */
typedef struct osd_list_type_retrieve {
	uint32_t	oltr_attributes_page;
	uint32_t	oltr_attribute_number;
} osd_list_type_retrieve_t;


/*
 * List entry format for retrieved attributes and for setting attributes
 * for an OSD object.
 */
typedef struct osd_list_type_retset {
	uint32_t	oltrs_attributes_page;
	uint32_t	oltrs_attribute_number;
	uint16_t	oltrs_attribute_length;
} osd_list_type_retset_t;


/*
 * Multi-object retrieved attributes format.  Used for attributes to be
 * retrieved by:
 * CREATE command that creates more than one user object
 * GET_MEMBER_ATTRIBUTES
 * SET_MEMBER_ATTRIBUTES
 * LIST command with list_attr set to 1
 * LIST_COLLECTION command with list_attr set to 1
 */
typedef struct osd_list_type_retrieve_multi {
	uint64_t	oltrm_user_object_id;
	uint8_t		oltrm_object_type;
	uint8_t		oltrm_reserved[5];
	uint16_t	oltrm_attribute_list_length;
} osd_list_type_retrieve_multi_t;


/*
 * Attributes list format
 */
typedef struct osd_attributes_list {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		oal_list_type	: 4,	/* Byte 0 */
			oal_reserved0	: 4;
#else
	uint8_t		oal_reserved0	: 4,	/* Byte 0 */
			oal_list_type	: 4;
#endif
	uint8_t		oal_reserved1[3];	/* Bytes 1-3 */
	uint32_t	oal_list_length;	/* Bytes 4-7 */
} osd_attributes_list_t;


/* Values for oal_list_type field */
#define	OSD_LIST_TYPE_RETRIEVE		0x1	/* osd_list_type_retrieve_t */
#define	OSD_LIST_TYPE_RETSET		0x9	/* osd_list_type_retset_t */
#define	OSD_LIST_TYPE_RETRIEVE_MULTI	0xE	/* ...type_retrieve_multi_t */



/*
 * Attribute page identification format. (Attribute number 0x0)
 */
typedef struct osd_attribute_page_id {
	uint8_t	oapid_vendor_id[8];
	uint8_t	oapid_attributes_page_id[32];
} osd_attribute_page_id_t;



/*
 * OSD attributes page number ranges
 */
#define	OSD_ATTR_USER_BASE		0x0
#define	OSD_ATTR_USER_MAX		0x2FFFFFFF
#define	OSD_ATTR_PARTITION_BASE		0x30000000
#define	OSD_ATTR_PARTITION_MAX		0x5FFFFFFF
#define	OSD_ATTR_COLLECTION_BASE	0x60000000
#define	OSD_ATTR_COLLECTION_MAX		0x8FFFFFFF
#define	OSD_ATTR_ROOT_BASE		0x90000000
#define	OSD_ATTR_ROOT_MAX		0xBFFFFFFF
#define	OSD_ATTR_ANYTYPE_BASE		0xF0000000
#define	OSD_ATTR_ANYTYPE_MAX		0xFFFFFFFE
#define	OSD_ATTR_ALLREQ			0xFFFFFFFF


/*
 * Attributes page numbers defined in T10/1729D Revision 3 (OSD-2)
 */

#define	OSD_USER_OBJECT_DIRECTORY_PAGE		0x0
#define	OSD_USER_OBJECT_INFORMATION_PAGE	0x1
#define	OSD_USER_OBJECT_QUOTAS_PAGE		0x2
#define	OSD_USER_OBJECT_TIMESTAMPS_PAGE		0x3
#define	OSD_USER_OBJECT_COLLECTIONS_PAGE	0x4
#define	OSD_USER_OBJECT_POLICY_SECURITY_PAGE	0x5
#define	OSD_USER_OBJECT_ERROR_RECOVERY_PAGE	0x6

#define	OSD_PARTITION_DIRECTORY_PAGE		0x30000000
#define	OSD_PARTITION_INFORMATION_PAGE		0x30000001
#define	OSD_PARTITION_QUOTAS_PAGE		0x30000002
#define	OSD_PARTITION_TIMESTAMPS_PAGE		0x30000003
#define	OSD_PARTITION_ATTRIBUTES_ACCESS_PAGE	0x30000004
#define	OSD_PARTITION_POLICY_SECURITY_PAGE	0x30000005
#define	OSD_PARTITION_ERROR_RECOVERY_PAGE	0x30000006

#define	OSD_COLLECTION_DIRECTORY_PAGE		0x60000000
#define	OSD_COLLECTION_INFORMATION_PAGE		0x60000001
#define	OSD_COLLECTION_TIMESTAMPS_PAGE		0x60000003
#define	OSD_COLLECTION_POLICY_SECURITY_PAGE	0x60000005
#define	OSD_COLLECTION_ERROR_RECOVERY_PAGE	0x60000006

#define	OSD_ROOT_DIRECTORY_PAGE			0x90000000
#define	OSD_ROOT_INFORMATION_PAGE		0x90000001
#define	OSD_ROOT_QUOTAS_PAGE			0x90000002
#define	OSD_ROOT_TIMESTAMPS_PAGE		0x90000003
#define	OSD_ROOT_POLICY_SECURITY_PAGE		0x90000005
#define	OSD_ROOT_ERROR_RECOVERY_PAGE		0x90000006

#define	OSD_CURRENT_COMMAND_PAGE		0xFFFFFFFE


/*
 * Root Information Attributes Page (OSD_ROOT_INFORMATION_PAGE)
 */
#define	OSD_ROOT_INFO_PAGE_IDENTIFICATION			0x0
#define	OSD_ROOT_INFO_PAGE_IDENTIFICATION_LEN			40
#define	OSD_ROOT_INFO_OSD_SYSTEM_ID				0x3
#define	OSD_ROOT_INFO_OSD_SYSTEM_ID_LEN				20
#define	OSD_ROOT_INFO_VENDOR_IDENTIFICATION			0x4
#define	OSD_ROOT_INFO_VENDOR_IDENTIFICATION_LEN			8
#define	OSD_ROOT_INFO_PRODUCT_IDENTIFICATION			0x5
#define	OSD_ROOT_INFO_PRODUCT_IDENTIFICATION_LEN		16
#define	OSD_ROOT_INFO_PRODUCT_MODEL				0x6
#define	OSD_ROOT_INFO_PRODUCT_MODEL_LEN				32
#define	OSD_ROOT_INFO_PRODUCT_REVISION_LEVEL			0x7
#define	OSD_ROOT_INFO_PRODUCT_REVISION_LEVEL_LEN		4
#define	OSD_ROOT_INFO_PRODUCT_SERIAL_NUMBER			0x8
#define	OSD_ROOT_INFO_OSD_NAME					0x9
#define	OSD_ROOT_INFO_TOTAL_CAPACITY				0x80
#define	OSD_ROOT_INFO_TOTAL_CAPACITY_LEN			8
#define	OSD_ROOT_INFO_USED_CAPACITY				0x81
#define	OSD_ROOT_INFO_USED_CAPACITY_LEN				8
#define	OSD_ROOT_INFO_OBJECT_ACCESSIBILITY			0x83
#define	OSD_ROOT_INFO_OBJECT_ACCESSIBILITY_LEN			4
#define	OSD_ROOT_INFO_NUMBER_OF_PARTITIONS			0xC0
#define	OSD_ROOT_INFO_NUMBER_OF_PARTITIONS_LEN			8
#define	OSD_ROOT_INFO_CLOCK					0x100
#define	OSD_ROOT_INFO_CLOCK_LEN					6
#define	OSD_ROOT_INFO_DEFAULT_ISOLATION_METHOD			0x110
#define	OSD_ROOT_INFO_DEFAULT_ISOLATION_METHOD_LEN		1
#define	OSD_ROOT_INFO_SUPPORTED_ISOLATION_METHODS		0x111
#define	OSD_ROOT_INFO_SUPPORTED_ISOLATION_METHODS_LEN		32
#define	OSD_ROOT_INFO_DATA_ATOMICITY_GUARANTEE			0x120
#define	OSD_ROOT_INFO_DATA_ATOMICITY_GUARANTEE_LEN		8
#define	OSD_ROOT_INFO_DATA_ATOMICITY_ALIGNMENT			0x121
#define	OSD_ROOT_INFO_DATA_ATOMICITY_ALIGNMENT_LEN		8
#define	OSD_ROOT_INFO_ATTRIBUTES_ATOMICITY_GUARANTEE		0x122
#define	OSD_ROOT_INFO_ATTRIBUTES_ATOMICITY_GUARANTEE_LEN	8
#define	OSD_ROOT_INFO_DATA_ATTR_ATOMICITY_MULTIPLIER		0x123
#define	OSD_ROOT_INFO_DATA_ATTR_ATOMICITY_MULTIPLIER_LEN	1

typedef struct osd_supported_isolation_methods {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		vendor	: 1,
			rsvd6	: 1,
			func	: 1,
			range	: 1,
			rsvd3	: 1,
			strict	: 1,
			none	: 1,
			rsvd0	: 1;
#else
	uint8_t		rsvd0	: 1,
			none	: 1,
			strict	: 1,
			rsvd3	: 1,
			range	: 1,
			func	: 1,
			rsvd6	: 1,
			vendor	: 1;
#endif
	uint8_t		reserved[31];
} osd_supported_isolation_methods_t;


/*
 * Partition Information Attributes Page (OSD_PARTITION_INFORMATION_PAGE)
 */
#define	OSD_PARTITION_INFO_PAGE_IDENTIFICATION			0x0
#define	OSD_PARTITION_INFO_PAGE_IDENTIFICATION_LEN		40
#define	OSD_PARTITION_INFO_PARTITION_ID				0x01
#define	OSD_PARTITION_INFO_PARTITION_ID_LEN			8
#define	OSD_PARTITION_INFO_USERNAME				0x09
#define	OSD_PARTITION_INFO_USED_CAPACITY			0x81
#define	OSD_PARTITION_INFO_USED_CAPACITY_LEN			8
#define	OSD_PARTITION_INFO_OBJECT_ACCESSIBILITY			0x83
#define	OSD_PARTITION_INFO_OBJECT_ACCESSIBILITY_LEN		4
#define	OSD_PARTITION_INFO_NUMBER_OF_OBJECTS			0xC1
#define	OSD_PARTITION_INFO_NUMBER_OF_OBJECTS_LEN		8

/*
 * Partition Attributes Access Attributes Page (OSD_PARTITION_ATTRIBUTES_ACCESS)
 */
#define	OSD_PARTITION_AAA_PAGE_IDENTIFICATION			0x0
#define	OSD_PARTITION_AAA_PAGE_IDENTIFICATION_LEN		40

/*
 * Attributes Access Descriptor
 * The Allowed Attributes Access Attribute format consists of a variable
 * number of attributes access descriptors.
 */

typedef struct osd_partition_attributes_access_descriptor {
	uint32_t		attributes_page;
	uint32_t		attribute_number;
} osd_partition_attributes_access_descriptor_t;


/*
 * Collection Information Attributes Page (OSD_COLLECTION_INFORMATION_PAGE)
 */
#define	OSD_COLLECTION_INFO_PAGE_IDENTIFICATION			0x0
#define	OSD_COLLECTION_INFO_PAGE_IDENTIFICATION_LEN		40
#define	OSD_COLLECTION_INFO_PARTITION_ID			0x01
#define	OSD_COLLECTION_INFO_PARTITION_ID_LEN			8
#define	OSD_COLLECTION_INFO_COLLECTION_OBJECT_ID		0x02
#define	OSD_COLLECTION_INFO_COLLECTION_OBJECT_ID_LEN		8
#define	OSD_COLLECTION_INFO_USERNAME				0x09
#define	OSD_COLLECTION_INFO_USED_CAPACITY			0x81
#define	OSD_COLLECTION_INFO_USED_CAPACITY_LEN			8
#define	OSD_COLLECTION_INFO_OBJECT_ACCESSIBILITY		0x83
#define	OSD_COLLECTION_INFO_OBJECT_ACCESSIBILITY_LEN		4


/*
 * User Object Information Attributes Page (OSD_USER_OBJECT_INFORMATION_PAGE)
 */
#define	OSD_USER_OBJECT_INFO_PAGE_IDENTIFICATION		0x0
#define	OSD_USER_OBJECT_INFO_PAGE_IDENTIFICATION_LEN		40
#define	OSD_USER_OBJECT_INFO_PARTITION_ID			0x01
#define	OSD_USER_OBJECT_INFO_PARTITION_ID_LEN			8
#define	OSD_USER_OBJECT_INFO_USER_OBJECT_OBJECT_ID		0x02
#define	OSD_USER_OBJECT_INFO_USER_OBJECT_OBJECT_ID_LEN		8
#define	OSD_USER_OBJECT_INFO_USERNAME				0x09
#define	OSD_USER_OBJECT_INFO_USED_CAPACITY			0x81
#define	OSD_USER_OBJECT_INFO_USED_CAPACITY_LEN			8
#define	OSD_USER_OBJECT_INFO_LOGICAL_LENGTH			0x82
#define	OSD_USER_OBJECT_INFO_LOGICAL_LENGTH_LEN			8
#define	OSD_USER_OBJECT_INFO_OBJECT_ACCESSIBILITY		0x83
#define	OSD_USER_OBJECT_INFO_OBJECT_ACCESSIBILITY_LEN		4
#define	OSD_USER_OBJECT_INFO_ACTUAL_DATA_SPACE			0xD1
#define	OSD_USER_OBJECT_INFO_ACTUAL_DATA_SPACE_LEN		8 /* Or 0 */
#define	OSD_USER_OBJECT_INFO_RESERVED_DATA_SPACE		0xD2
#define	OSD_USER_OBJECT_INFO_RESERVED_DATA_SPACE_LEN		8 /* Or 0 */


/*
 * Root Quotas Attributes Page (OSD_ROOT_QUOTAS_PAGE)
 */
#define	OSD_ROOT_QUOTAS_PAGE_IDENTIFICATION			0x0
#define	OSD_ROOT_QUOTAS_PAGE_IDENTIFICATION_LEN			40
#define	OSD_ROOT_QUOTAS_DEFAULT_MAX_OBJECT_LENGTH		0x1
#define	OSD_ROOT_QUOTAS_DEFAULT_MAX_OBJECT_LENGTH_LEN		8
#define	OSD_ROOT_QUOTAS_PARTITION_CAPACITY_QUOTA		0x10001
#define	OSD_ROOT_QUOTAS_PARTITION_CAPACITY_QUOTA_LEN		8
#define	OSD_ROOT_QUOTAS_PARTITION_OBJECT_COUNT			0x10002
#define	OSD_ROOT_QUOTAS_PARTITION_OBJECT_COUNT_LEN		8
#define	OSD_ROOT_QUOTAS_PARTITION_COLLECTIONS_PER_USER_OBJECT	0x10081
#define	OSD_ROOT_QUOTAS_PARTITION_COLLECTIONS_PER_USER_OBJECT_LEN	4
#define	OSD_ROOT_QUOTAS_PARTITION_COUNT				0x20002
#define	OSD_ROOT_QUOTAS_PARTITION_COUNT_LEN			8

typedef struct osd_root_quotas_page {
	uint32_t	rqp_page_number;
	uint32_t	rqp_page_length;
	uint64_t	rqp_default_maximum_user_object_length;
	uint64_t	rqp_partition_capacity_quota;
	uint64_t	rqp_partition_object_count;
	uint32_t	rqp_partition_collections_per_user_object;
	uint64_t	rqp_partition_count;
} osd_root_quotas_page_t;


/*
 * Partition Quotas Attributes Page (OSD_PARTITION_QUOTAS_PAGE)
 */
#define	OSD_PARTITION_QUOTAS_PAGE_IDENTIFICATION		0x0
#define	OSD_PARTITION_QUOTAS_PAGE_IDENTIFICATION_LEN		40
#define	OSD_PARTITION_QUOTAS_DEFAULT_MAX_OBJECT_LENGTH		0x1
#define	OSD_PARTITION_QUOTAS_DEFAULT_MAX_OBJECT_LENGTH_LEN	8
#define	OSD_PARTITION_QUOTAS_CAPACITY_QUOTA			0x10001
#define	OSD_PARTITION_QUOTAS_CAPACITY_QUOTA_LEN			8
#define	OSD_PARTITION_QUOTAS_OBJECT_COUNT			0x10002
#define	OSD_PARTITION_QUOTAS_OBJECT_COUNT_LEN			8
#define	OSD_PARTITION_QUOTAS_COLLECTIONS_PER_USER_OBJECT	0x10081
#define	OSD_PARTITION_QUOTAS_COLLECTIONS_PER_USER_OBJECT_LEN	4

typedef struct osd_partition_quotas_page {
	uint32_t	pqp_page_number;
	uint32_t	pqp_page_length;
	uint64_t	pqp_default_maximum_user_object_length;
	uint64_t	pqp_capacity_quota;
	uint64_t	pqp_object_count;
	uint32_t	pqp_collections_per_user_object;
} osd_partition_quotas_page_t;


/*
 * User Object Quotas Attributes Page (OSD_USER_OBJECT_QUOTAS_PAGE)
 */
#define	OSD_USER_OBJECT_QUOTAS_PAGE_IDENTIFICATION		0x0
#define	OSD_USER_OBJECT_QUOTAS_PAGE_IDENTIFICATION_LEN		40
#define	OSD_USER_OBJECT_QUOTAS_MAXIMUM_OBJECT_LENGTH		0x1
#define	OSD_USER_OBJECT_QUOTAS_MAXIMUM_OBJECT_LENGTH_LEN	8

typedef struct osd_user_object_quotas_page {
	uint32_t	uqp_page_number;
	uint32_t	uqp_page_length;
	uint64_t	uqp_maximum_user_object_length;
} osd_user_object_quotas_page_t;



/*
 * Root Timestamps Attributes Page (OSD_ROOT_TIMESTAMPS_PAGE)
 */
#define	OSD_ROOT_TIMESTAMPS_PAGE_IDENTIFICATION			0x0
#define	OSD_ROOT_TIMESTAMPS_PAGE_IDENTIFICATION_LEN		40
#define	OSD_ROOT_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME		0x2
#define	OSD_ROOT_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME_LEN	6
#define	OSD_ROOT_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME		0x3
#define	OSD_ROOT_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME_LEN	6
#define	OSD_ROOT_TIMESTAMPS_TIMESTAMP_BYPASS			0xFFFFFFFE
#define	OSD_ROOT_TIMESTAMPS_TIMESTAMP_BYPASS_LEN		1

typedef struct osd_root_timestamps_page {
	uint32_t	rtp_page_number;
	uint32_t	rtp_page_length;
	uint8_t		rtp_attributes_accessed_time[6];
	uint8_t		rtp_attributes_modified_time[6];
	uint8_t		rtp_timestamp_bypass;
} osd_root_timestamps_page_t;



/*
 * Partition Timestamps Attributes Page (OSD_PARTITION_TIMESTAMPS_PAGE)
 */
#define	OSD_PARTITION_TIMESTAMPS_PAGE_IDENTIFICATION		0x0
#define	OSD_PARTITION_TIMESTAMPS_PAGE_IDENTIFICATION_LEN	40
#define	OSD_PARTITION_TIMESTAMPS_CREATED_TIME			0x1
#define	OSD_PARTITION_TIMESTAMPS_CREATED_TIME_LEN		6
#define	OSD_PARTITION_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME	0x2
#define	OSD_PARTITION_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME_LEN	6
#define	OSD_PARTITION_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME	0x3
#define	OSD_PARTITION_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME_LEN	6
#define	OSD_PARTITION_TIMESTAMPS_DATA_ACCESSED_TIME		0x4
#define	OSD_PARTITION_TIMESTAMPS_DATA_ACCESSED_TIME_LEN		6
#define	OSD_PARTITION_TIMESTAMPS_DATA_MODIFIED_TIME		0x5
#define	OSD_PARTITION_TIMESTAMPS_DATA_MODIFIED_TIME_LEN		6
#define	OSD_PARTITION_TIMESTAMPS_TIMESTAMP_BYPASS		0xFFFFFFFE
#define	OSD_PARTITION_TIMESTAMPS_TIMESTAMP_BYPASS_LEN		1

typedef struct osd_partition_timestamps_page {
	uint32_t	ptp_page_number;
	uint32_t	ptp_page_length;
	uint8_t		ptp_created_time[6];
	uint8_t		ptp_attributes_accessed_time[6];
	uint8_t		ptp_attributes_modified_time[6];
	uint8_t		ptp_data_accessed_time[6];
	uint8_t		ptp_data_modified_time[6];
	uint8_t		ptp_timestamp_bypass;
} osd_partition_timestamps_page_t;


/*
 * Collection Timestamps Attributes Page (OSD_COLLECTION_TIMESTAMPS_PAGE)
 */
#define	OSD_COLLECTION_TIMESTAMPS_PAGE_IDENTIFICATION		0x0
#define	OSD_COLLECTION_TIMESTAMPS_PAGE_IDENTIFICATION_LEN	40
#define	OSD_COLLECTION_TIMESTAMPS_CREATED_TIME			0x1
#define	OSD_COLLECTION_TIMESTAMPS_CREATED_TIME_LEN		6
#define	OSD_COLLECTION_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME	0x2
#define	OSD_COLLECTION_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME_LEN	6
#define	OSD_COLLECTION_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME	0x3
#define	OSD_COLLECTION_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME_LEN	6
#define	OSD_COLLECTION_TIMESTAMPS_DATA_ACCESSED_TIME		0x4
#define	OSD_COLLECTION_TIMESTAMPS_DATA_ACCESSED_TIME_LEN	6
#define	OSD_COLLECTION_TIMESTAMPS_DATA_MODIFIED_TIME		0x5
#define	OSD_COLLECTION_TIMESTAMPS_DATA_MODIFIED_TIME_LEN	6

typedef struct osd_collection_timestamps_page {
	uint32_t	ctp_page_number;
	uint32_t	ctp_page_length;
	uint8_t		ctp_created_time[6];
	uint8_t		ctp_attributes_accessed_time[6];
	uint8_t		ctp_attributes_modified_time[6];
	uint8_t		ctp_data_accessed_time[6];
	uint8_t		ctp_data_modified_time[6];
} osd_collection_timestamps_page_t;



/*
 * User Object Timestamps Attributes Page (OSD_USER_OBJECT_TIMESTAMPS_PAGE)
 */
#define	OSD_USER_OBJECT_TIMESTAMPS_PAGE_IDENTIFICATION		0x0
#define	OSD_USER_OBJECT_TIMESTAMPS_PAGE_IDENTIFICATION_LEN	40
#define	OSD_USER_OBJECT_TIMESTAMPS_CREATED_TIME			0x1
#define	OSD_USER_OBJECT_TIMESTAMPS_CREATED_TIME_LEN		6
#define	OSD_USER_OBJECT_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME	0x2
#define	OSD_USER_OBJECT_TIMESTAMPS_ATTRIBUTES_ACCESSED_TIME_LEN	6
#define	OSD_USER_OBJECT_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME	0x3
#define	OSD_USER_OBJECT_TIMESTAMPS_ATTRIBUTES_MODIFIED_TIME_LEN	6
#define	OSD_USER_OBJECT_TIMESTAMPS_DATA_ACCESSED_TIME		0x4
#define	OSD_USER_OBJECT_TIMESTAMPS_DATA_ACCESSED_TIME_LEN	6
#define	OSD_USER_OBJECT_TIMESTAMPS_DATA_MODIFIED_TIME		0x5
#define	OSD_USER_OBJECT_TIMESTAMPS_DATA_MODIFIED_TIME_LEN	6

typedef struct osd_user_object_timestamps_page {
	uint32_t	utp_page_number;
	uint32_t	utp_page_length;
	uint8_t		utp_created_time[6];
	uint8_t		utp_attributes_accessed_time[6];
	uint8_t		utp_attributes_modified_time[6];
	uint8_t		utp_data_accessed_time[6];
	uint8_t		utp_data_modified_time[6];
} osd_user_object_timestamps_page_t;


/*
 * User Object Collections Attributes Page (OSD_USER_OBJECT_COLLECTIONS_PAGE)
 */
#define	OSD_USER_OBJECT_COLLECTIONS_PAGE_IDENTIFICATION		0x0
#define	OSD_USER_OBJECT_COLLECTIONS_PAGE_IDENTIFICATION_LEN	40

typedef struct osd_user_object_collections_page {
	uint32_t	ucp_page_number;
	uint32_t	ucp_page_length;
	uint64_t	ucp_attribute_value;
} osd_user_object_collections_page_t;

typedef struct osd_user_object_collection_attributes_page {
	uint32_t	page_number;
	uint32_t	page_length;
	/* Followed by variable number of 64-bit pointers */
} osd_user_object_collection_attributes_page_t;


/*
 * Root Policy/Security Attributes Page (OSD_ROOT_POLICY_SECURITY_PAGE)
 */
#define	OSD_ROOT_POLICY_PAGE_IDENTIFICATION			0x0
#define	OSD_ROOT_POLICY_PAGE_IDENTIFICATION_LEN			40
#define	OSD_ROOT_POLICY_DEFAULT_SECURITY_METHOD			0x1
#define	OSD_ROOT_POLICY_DEFAULT_SECURITY_METHOD_LEN		1
#define	OSD_ROOT_POLICY_OLDEST_VALID_NONCE_LIMIT		0x2
#define	OSD_ROOT_POLICY_OLDEST_VALID_NONCE_LIMIT_LEN		6
#define	OSD_ROOT_POLICY_NEWEST_VALID_NONCE_LIMIT		0x3
#define	OSD_ROOT_POLICY_NEWEST_VALID_NONCE_LIMIT_LEN		6
#define	OSD_ROOT_POLICY_PARTITION_DEFAULT_SECURITY_METHOD	0x6
#define	OSD_ROOT_POLICY_PARTITION_DEFAULT_SECURITY_METHOD_LEN	1
#define	OSD_ROOT_POLICY_SUPPORTED_SECURITY_METHODS		0x7
#define	OSD_ROOT_POLICY_SUPPORTED_SECURITY_METHODS_LEN		2
#define	OSD_ROOT_POLICY_ADJUSTABLE_CLOCK			0x9
#define	OSD_ROOT_POLICY_ADJUSTABLE_CLOCK_LEN			6
#define	OSD_ROOT_POLICY_BOOT_EPOCH				0xA
#define	OSD_ROOT_POLICY_BOOT_EPOCH_LEN				2
#define	OSD_ROOT_POLICY_MASTER_KEY_IDENTIFIER			0x7FFD
#define	OSD_ROOT_POLICY_MASTER_KEY_IDENTIFIER_MAX_LEN		7
#define	OSD_ROOT_POLICY_ROOT_KEY_IDENTIFIER			0x7FFE
#define	OSD_ROOT_POLICY_ROOT_KEY_IDENTIFIER_MAX_LEN		7
#define	OSD_ROOT_POLICY_SUPPORTED_INTEGRITY_ALGORITHM		0x80000000
#define	OSD_ROOT_POLICY_SUPPORTED_INTEGRITY_ALGORITHM_LEN	1
#define	OSD_ROOT_POLICY_SUPPORTED_DH_GROUP_0			0x80000010
#define	OSD_ROOT_POLICY_SUPPORTED_DH_GROUP_0_LEN		1


typedef struct osd_root_policy_page {
	uint32_t	rps_page_number;
	uint32_t	rps_page_length;
	uint8_t		rps_default_security_method;
	uint8_t		rps_partition_default_security_method;
	uint8_t		rps_supported_security_methods[2];
	uint8_t		rps_oldest_valid_nonce_limit[6];
	uint8_t		rps_newest_valid_nonce_limit[6];
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		rps_rki_valid	: 1,
			rps_mki_valid	: 1,
			rps_reserved	: 6;
#else
	uint8_t		rps_reserved	: 6,
			rps_mki_valid	: 1,
			rps_rki_valid	: 1;
#endif
	uint8_t		rps_master_key_identifier[7];
	uint8_t		rps_root_key_identifier[7];
	uint8_t		rps_supported_integrity_algorithm[16];
	uint8_t		rps_supported_dh_group[16];
	uint8_t		reserved;
	uint16_t	rps_boot_epoch;
} osd_root_policy_page_t;


#define	OSD_SECURITY_METHOD_ALLDATA		0x08
#define	OSD_SECURITY_METHOD_CMDRSP		0x04
#define	OSD_SECURITY_METHOD_CAPKEY		0x02
#define	OSD_SECURITY_METHOD_NOSEC		0x01

#define	OSD_INTEGRITY_ALGORITHM_NONE		0x00
#define	OSD_INTEGRITY_ALGORITHM_HMAC_SHA1	0x01



/*
 * Partition Policy/Security Attributes Page
 * (OSD_PARTITION_POLICY_SECURITY_PAGE)
 */
#define	OSD_PARTITION_POLICY_PAGE_IDENTIFICATION		0x0
#define	OSD_PARTITION_POLICY_PAGE_IDENTIFICATION_LEN		40
#define	OSD_PARTITION_POLICY_DEFAULT_SECURITY_METHOD		0x1
#define	OSD_PARTITION_POLICY_DEFAULT_SECURITY_METHOD_LEN	1
#define	OSD_PARTITION_POLICY_OLDEST_VALID_NONCE			0x2
#define	OSD_PARTITION_POLICY_OLDEST_VALID_NONCE_LEN		6
#define	OSD_PARTITION_POLICY_NEWEST_VALID_NONCE			0x3
#define	OSD_PARTITION_POLICY_NEWEST_VALID_NONCE_LEN		6
#define	OSD_PARTITION_POLICY_REQUEST_NONCE_LIST_DEPTH		0x4
#define	OSD_PARTITION_POLICY_REQUEST_NONCE_LIST_DEPTH_LEN	2
#define	OSD_PARTITION_POLICY_FROZEN_WORKING_KEY_BIT_MASK	0x5
#define	OSD_PARTITION_POLICY_FROZEN_WORKING_KEY_BIT_MASK_LEN	2
#define	OSD_PARTITION_POLICY_PARTITION_KEY_IDENTIFIER		0x7FFF
#define	OSD_PARTITION_POLICY_PARTITION_KEY_IDENTIFIER_MAX_LEN	7
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_0		0x8000
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_1		0x8001
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_2		0x8002
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_3		0x8003
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_4		0x8004
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_5		0x8005
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_6		0x8006
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_7		0x8007
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_8		0x8008
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_9		0x8009
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_A		0x800A
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_B		0x800B
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_C		0x800C
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_D		0x800D
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_E		0x800E
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_F		0x800F
#define	OSD_PARTITION_POLICY_WORKING_KEY_IDENTIFIER_MAX_LEN	7
#define	OSD_PARTITION_POLICY_POLICY_ACCESS_TAG			0x40000001
#define	OSD_PARTITION_POLICY_POLICY_ACCESS_TAG_LEN		4
#define	OSD_PARTITION_POLICY_USER_OBJECT_POLICY_ACCESS_TAG	0x40000002
#define	OSD_PARTITION_POLICY_USER_OBJECT_POLICY_ACCESS_TAG_LEN	4

/*
 * Frozen working key bit mask attribute
 */
#define	OSD_WK00_FZN		0x0001
#define	OSD_WK01_FZN		0x0002
#define	OSD_WK02_FZN		0x0004
#define	OSD_WK03_FZN		0x0008
#define	OSD_WK04_FZN		0x0010
#define	OSD_WK05_FZN		0x0020
#define	OSD_WK06_FZN		0x0040
#define	OSD_WK07_FZN		0x0080
#define	OSD_WK08_FZN		0x0100
#define	OSD_WK09_FZN		0x0200
#define	OSD_WK0A_FZN		0x0400
#define	OSD_WK0B_FZN		0x0800
#define	OSD_WK0C_FZN		0x1000
#define	OSD_WK0D_FZN		0x2000
#define	OSD_WK0E_FZN		0x4000
#define	OSD_WK0F_FZN		0x8000


typedef struct osd_partition_policy_page {
	uint32_t	pps_page_number;
	uint32_t	pps_page_length;
	uint8_t		pps_reserved0[3];
	uint8_t		pps_default_security_method;
	uint8_t		pps_oldest_valid_nonce[6];
	uint8_t		pps_newest_valid_nonce[6];
	uint16_t	pps_requested_nonce_list_depth;
	uint16_t	pps_prozen_working_key_bit_mask;
	uint32_t	pps_policy_access_tag;
	uint32_t	pps_user_object_policy_access_tag;
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		pps_pki_valid	: 1,
			pps_reserved	: 7;

	uint8_t		pps_wki00_vld	: 1,
			pps_wki01_vld	: 1,
			pps_wki02_vld	: 1,
			pps_wki03_vld	: 1,
			pps_wki04_vld	: 1,
			pps_wki05_vld	: 1,
			pps_wki06_vld	: 1,
			pps_wki07_vld	: 1;

	uint8_t		pps_wki08_vld	: 1,
			pps_wki09_vld	: 1,
			pps_wki0A_vld	: 1,
			pps_wki0B_vld	: 1,
			pps_wki0C_vld	: 1,
			pps_wki0D_vld	: 1,
			pps_wki0E_vld	: 1,
			pps_wki0F_vld	: 1;
#else
	uint8_t		pps_reserved	: 7,
			pps_pki_valid	: 1;

	uint8_t		pps_wki07_vld	: 1,
			pps_wki06_vld	: 1,
			pps_wki05_vld	: 1,
			pps_wki04_vld	: 1,
			pps_wki03_vld	: 1,
			pps_wki02_vld	: 1,
			pps_wki01_vld	: 1,
			pps_wki00_vld	: 1;

	uint8_t		pps_wki0F_vld	: 1,
			pps_wki0E_vld	: 1,
			pps_wki0D_vld	: 1,
			pps_wki0C_vld	: 1,
			pps_wki0B_vld	: 1,
			pps_wki0A_vld	: 1,
			pps_wki09_vld	: 1,
			pps_wki08_vld	: 1;
#endif
	uint8_t		pps_partition_key_identifier[7];
	uint8_t		pps_working_key_identifier_[16][7];
} osd_partition_policy_page_t;



/*
 * Collection Policy/Security Attributes Page
 * (OSD_COLLECTION_POLICY_SECURITY_PAGE)
 */
#define	OSD_COLLECTION_POLICY_PAGE_IDENTIFICATION		0x0
#define	OSD_COLLECTION_POLICY_PAGE_IDENTIFICATION_LEN		40
#define	OSD_COLLECTION_POLICY_POLICY_ACCESS_TAG			0x40000001
#define	OSD_COLLECTION_POLICY_POLICY_ACCESS_TAG_LEN		4


typedef struct osd_collection_policy_page {
	uint32_t	cps_page_number;
	uint32_t	cps_page_length;
	uint32_t	cps_policy_access_tag;
} osd_collection_policy_page_t;



/*
 * User Object Policy/Security Attributes Page
 * (OSD_USER_OBJECT_POLICY_SECURITY_PAGE)
 */
#define	OSD_USER_OBJECT_POLICY_PAGE_IDENTIFICATION		0x0
#define	OSD_USER_OBJECT_POLICY_PAGE_IDENTIFICATION_LEN		40
#define	OSD_USER_OBJECT_POLICY_POLICY_ACCESS_TAG		0x40000001
#define	OSD_USER_OBJECT_POLICY_POLICY_ACCESS_TAG_LEN		4


typedef struct osd_user_object_policy_page {
	uint32_t	ups_page_number;
	uint32_t	ups_page_length;
	uint32_t	ups_policy_access_tag;
} osd_user_object_policy_page_t;

/*
 * Common structures for root, partition, and collection attributes.
 * Some fields are not universally applicable.  See the comment
 * after each field for applicability. (R=root, P=partition, C=collection)
 * The field prefix is 'X' implies context-sensitivity.  That is,
 * X_list could mean root, partition or collection list.
 */

typedef struct osd_damage_summary_attribute {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		X_list		: 1,	/* RPC */
			attr		: 1,	/* RPC */
			r_osc_rc	: 1,	/* R */
			p_osc_rc	: 1,	/* RP */
			rsvd		: 3,
			p_osc		: 1;	/* R */
#else
	uint8_t		p_osc		: 1,	/* R */
			rsvd		: 3,
			p_osc_rc	: 1,	/* RP */
			r_osc_rc	: 1,	/* R */
			attr		: 1,	/* RPC */
			X_list		: 1;	/* RPC */
#endif
} osd_damage_summary_attribute_t;

/*
 * The contained objects damage summary attribute applies to the root,
 * partition, and user attributes but not the collection attribute.
 */

typedef struct osd_contained_objects_damage_summary {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		c_data	: 1,
			c_attr	: 1,
			rsvd	: 6;
#else
	uint8_t		rsvd	: 6,
			c_attr	: 1,
			c_data	: 1;
#endif
} osd_contained_objects_damage_summary_t;

/*
 * This structure is used for root, partition and user error recovery
 * attributes pages.  The collection error recovery attributes page format
 * is different and is dsecribed below.
 */

typedef struct osd_error_recovery_attributes_page {
	uint32_t	era_page_number;			/* Bytes 0-3 */
	uint32_t	era_page_length;			/* Bytes 4-7 */
	uint64_t	era_number_damaged;			/* Bytes 8-15 */
	uint8_t		era_damage_summary;			/* Byte 16 */
	uint8_t		era_contained_objs_damage_summary;	/* Byte 17 */
	uint8_t		era_last_damaged_obj_data_time[6];	/* 18-23 */
	uint8_t		era_last_damaged_obj_attr_time[6];	/* 24-29 */
	uint8_t		era_last_damaged_contained_obj_time[6];	/* 30-35 */
} osd_error_recovery_attributes_page_t;

/*
 * Root Error Recovery Attributes Page format
 * (OSD_ROOT_ERROR_RECOVERY_PAGE)
 */

#define	OSD_ROOT_ERR_RECOVERY_PAGE_IDENTIFICATION			0x0
#define	OSD_ROOT_ERR_RECOVERY_PAGE_IDENTIFICATION_LEN			40
#define	OSD_ROOT_ERR_RECOVERY_ROOT_DAMAGE_SUMMARY			0x1
#define	OSD_ROOT_ERR_RECOVERY_ROOT_DAMAGE_SUMMARY_LEN			1
#define	OSD_ROOT_ERR_RECOVERY_CONTAINED_OBJS_DAMAGE_SUMMARY		0x2
#define	OSD_ROOT_ERR_RECOVERY_CONTAINED_OBJS_DAMAGE_SUMMARY_LEN		1
#define	OSD_ROOT_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME		0x3
#define	OSD_ROOT_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME_LEN		6
#define	OSD_ROOT_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME		0x4
#define	OSD_ROOT_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME_LEN		6
#define	OSD_ROOT_ERR_RECOVERY_LAST_DAMAGED_CONTAINED_OBJ_TIME		0x5
#define	OSD_ROOT_ERR_RECOVERY_LAST_DAMAGED_CONTAINED_OBJ_TIME_LEN	6
#define	OSD_ROOT_ERR_RECOVERY_NUMBER_DAMAGED_PARTITIONS			0x6
#define	OSD_ROOT_ERR_RECOVERY_NUMBER_DAMAGED_PARTITIONS_LEN		6

/*
 * Partition Error Recovery Attributes Page format
 * (OSD_PARTITION_ERROR_RECOVERY_PAGE)
 */

#define	OSD_PART_ERR_RECOVERY_PAGE_IDENTIFICATION			0x0
#define	OSD_PART_ERR_RECOVERY_PAGE_IDENTIFICATION_LEN			40
#define	OSD_PART_ERR_RECOVERY_DAMAGE_SUMMARY				0x1
#define	OSD_PART_ERR_RECOVERY_DAMAGE_SUMMARY_LEN			1
#define	OSD_PART_ERR_RECOVERY_CONTAINED_OBJS_DAMAGE_SUMMARY		0x2
#define	OSD_PART_ERR_RECOVERY_CONTAINED_OBJS_DAMAGE_SUMMARY_LEN		1
#define	OSD_PART_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME		0x3
#define	OSD_PART_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME_LEN		6
#define	OSD_PART_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME		0x4
#define	OSD_PART_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME_LEN		6
#define	OSD_PART_ERR_RECOVERY_LAST_DAMAGED_CONTAINED_OBJ_TIME		0x5
#define	OSD_PART_ERR_RECOVERY_LAST_DAMAGED_CONTAINED_OBJ_TIME_LEN	6
#define	OSD_PART_ERR_RECOVERY_NUMBER_DAMAGED_OBJECTS			0x6
#define	OSD_PART_ERR_RECOVERY_NUMBER_DAMAGED_OBJECTS_LEN		6

/*
 * Collection Error Recovery Attributes Page format
 * (OSD_COLLECTION_ERROR_RECOVERY_PAGE)
 */

#define	OSD_COLL_ERR_RECOVERY_PAGE_IDENTIFICATION			0x0
#define	OSD_COLL_ERR_RECOVERY_PAGE_IDENTIFICATION_LEN			40
#define	OSD_COLL_ERR_RECOVERY_DAMAGE_SUMMARY				0x1
#define	OSD_COLL_ERR_RECOVERY_DAMAGE_SUMMARY_LEN			1
#define	OSD_COLL_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME		0x3
#define	OSD_COLL_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME_LEN		6
#define	OSD_COLL_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME		0x4
#define	OSD_COLL_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME_LEN		6

typedef struct osd_collection_damage_summary_attribute {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		c_list	: 1,
			attr	: 1,
			rsvd	: 6;
#else
	uint8_t		rsvd	: 6,
			attr	: 1,
			c_list	: 1;
#endif
} osd_collection_damage_summary_attribute_t;

typedef struct osd_collection_error_recovery_attributes_page {
	uint32_t	era_page_number;			/* Bytes 0-3 */
	uint32_t	era_page_length;			/* Bytes 4-7 */
	uint8_t		era_damage_summary;			/* Byte 8 */
	uint8_t		era_reserved;				/* Byte 9 */
	uint8_t		era_last_damaged_obj_data_time[6];	/* 10-15 */
	uint8_t		era_last_damaged_obj_attr_time[6];	/* 16-21 */
} osd_collection_error_recovery_attributes_page_t;

/*
 * User Object Error Recovery Attributes Page format
 * (OSD_USER_OBJECT_ERROR_RECOVERY_PAGE)
 */

#define	OSD_USER_OBJ_ERR_RECOVERY_PAGE_IDENTIFICATION			0x0
#define	OSD_USER_OBJ_ERR_RECOVERY_PAGE_IDENTIFICATION_LEN		40
#define	OSD_USER_OBJ_ERR_RECOVERY_DAMAGE_SUMMARY			0x1
#define	OSD_USER_OBJ_ERR_RECOVERY_DAMAGE_SUMMARY_LEN			1
#define	OSD_USER_OBJ_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME		0x3
#define	OSD_USER_OBJ_ERR_RECOVERY_LAST_DAMAGED_OBJ_DATA_TIME_LEN	6
#define	OSD_USER_OBJ_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME		0x4
#define	OSD_USER_OBJ_ERR_RECOVERY_LAST_DAMAGED_OBJ_ATTR_TIME_LEN	6

typedef struct osd_user_object_damage_summary_attribute {
#if defined(_BIT_FIELDS_LTOH)
	uint8_t		data	: 1,
			attr	: 1,
			rsvd	: 6;
#else
	uint8_t		rsvd	: 6,
			attr	: 1,
			data	: 1;
#endif
} osd_user_object_damage_summary_attribute_t;

typedef osd_collection_error_recovery_attributes_page_t \
    osd_user_object_error_recovery_attributes_page_t;

/*
 * Current Command Attributes Page (OSD_CURRENT_COMMAND_PAGE)
 */
#define	OSD_CURRENT_COMMAND_PAGE_IDENTIFICATION			0x0
#define	OSD_CURRENT_COMMAND_PAGE_IDENTIFICATION_LEN		40
#define	OSD_CURRENT_COMMAND_RESPONSE_INTEGRITY_CHECK_VALUE	0x1
#define	OSD_CURRENT_COMMAND_RESPONSE_INTEGRITY_CHECK_VALUE_LEN	20
#define	OSD_CURRENT_COMMAND_OBJECT_TYPE				0x2
#define	OSD_CURRENT_COMMAND_OBJECT_TYPE_LEN			1
#define	OSD_CURRENT_COMMAND_PARTITION_ID			0x3
#define	OSD_CURRENT_COMMAND_PARTITION_ID_LEN			8
#define	OSD_CURRENT_COMMAND_OBJECT_ID				0x4
#define	OSD_CURRENT_COMMAND_OBJECT_ID_LEN			8
#define	OSD_CURRENT_COMMAND_STARTING_BYTE_ADDRESS_OF_APPEND	0x05
#define	OSD_CURRENT_COMMAND_STARTING_BYTE_ADDRESS_OF_APPEND_LEN	8


typedef struct osd_current_command_page {
	uint32_t	ccp_page_number;
	uint32_t	ccp_page_length;
	uint8_t		ccp_response_integrity_check_value[20];
	uint8_t		ccp_object_type;
	uint8_t		ccp_reserved[3];
	uint64_t	ccp_partition_id;
	uint64_t	ccp_object_id;
	uint64_t	ccp_starting_byte_address_of_append;
} osd_current_command_page_t;


typedef struct osd_null_atributes_page {
	uint32_t	nap_page_number;
	uint32_t	nap_page_length;
} osd_null_atributes_page_t;

#pragma pack()

#ifdef	__cplusplus
}
#endif

#endif	/* _SCSI_OSD_H */
