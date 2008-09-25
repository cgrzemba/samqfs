/*
 * common.h
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

#ifndef _COMMON_H
#define	_COMMON_H

#pragma ident "$Revision: 1.18 $"

#include "aml/device.h"
#include "aml/preview.h"
#include "aml/scsi.h"
#include "../lib/librobots.h"

/* Macro to delete trailing blanks in string a of length l */

#define	DTB(a, l) { register uchar_t *t; for (t = ((a)+((l)-1)); \
	(t >= (a))&&(((*t) == 0x20)||(!isprint(*t))); t--)(*t) = 0x00; }

/* Some nice macros */

/* is the supplied element in the range of storage elements */
#define	IS_STORAGE(l, e) is_storage(l, e)

/* caculate the slot number given element address */
#define	SLOT_NUMBER(l, e) slot_number(l, e)

/* given the slot number, return the element address */
#define	ELEMENT_ADDRESS(l, i) element_address(l, i)

/* is the given slot number the flip side of the storage element */
#define	IS_FLIPPED(l, i)	((l)->status.b.two_sided ? ((i) & 0x1) : 0)

/* Bar code known values */

	/*
	 * For metrum.
	 * Barcode is unreadable
	 * No barcode and no physical check
	 * No barcode put physical check
	 */

#define	METRUM_BC_UNREAD	"*************"
#define	METRUM_BC_NOCHK		"+++++++++++++"
#define	METRUM_BC_NOBARC	"-------------"
#define	METRUM_BC_SIZE		13	/* Number of bytes to compare */

/*
 * Tables and structurs internal to the every media changer
 * For all of these structs, the main mutex is used for locking
 * all fields except list management and catalog access.
 * For these fields, use the mutex accociated with the fields.
 * The body of the struct is defined here, media changer specific
 * stuff should be defined in the media changers .h file.
 */

/* drive_state - One for each drive in a library. */
#define	COMMON_DRIVE_STATE \
	mutex_t	mutex; \
	union { \
    struct { \
	uint_t \
	offline    : 1, \
	except	: 1, \
	access	: 1, \
	full	: 1, \
	d_st_invert	: 1, \
	valid	: 1, \
	bar_code   : 1, \
	cln_inprog : 1; \
	}b; \
    uint_t  bits; \
	}status; \
	thread_t	thread; \
	int	    open_fd; \
	uchar_t	 bar_code[BARCODE_LEN + 1]; \
	ushort_t	new_slot; \
	dev_ent_t		*un; \
	preview_t		*preview; \
	struct library_s	*library; \
	struct drive_state_s  *previous; \
	struct drive_state_s  *next; \
	char	    *samst_name; \
	mutex_t	 list_mutex; \
	cond_t	  list_condit; \
	int		active_count; \
	robo_event_t    *first;
/* End of COMMON_DRIVE_STATE */

/* xport_state.  Information about the state of a transport element. */
#define	COMMON_XPORT_STATE \
	mutex_t	mutex; \
	cond_t	condit; \
	union { \
    struct { \
	ushort_t \
	rotate	: 1, \
	mail_box   : 1, \
	except	: 1, \
	full	: 1, \
	invert	: 1, \
	valid	: 1, \
	bar_code   : 1; \
	}b; \
    ushort_t  bits; \
	}status; \
	thread_t	thread; \
	uchar_t	 bar_code[BARCODE_LEN + 1]; \
	struct library_s	*library; \
	struct xport_state_s *previous; \
	struct xport_state_s *next; \
	mutex_t	 list_mutex; \
	cond_t	  list_condit; \
	int		active_count; \
	robo_event_t    *first;
/* End of COMMON_XPORT_STATE */

/*
 *  iport_state. Information on the import/export element.
 *
 *  The open and tray bits are only used to support the
 *  Plasmon DVD-RAM library's I/E drawer.
 *	  open: equals one if I/E drawer is open
 *    tray: equals one if tray exists in I/E drawer
 */
#define	COMMON_IPORT_STATE \
	mutex_t	mutex; \
	union { \
    struct { \
	uint_t \
	inenab	: 1, \
	exenab	: 1, \
	access	: 1, \
	except	: 1, \
	impexp	: 1, \
	full	: 1, \
	invert	: 1, \
	valid	: 1, \
	bar_code   : 1, \
	open	: 1, \
	tray	: 1; \
	}b; \
    uint_t   bits; \
	}status; \
	thread_t	thread; \
	uchar_t	 bar_code[BARCODE_LEN + 1]; \
	struct library_s	*library; \
	struct iport_state_s *previous; \
	struct iport_state_s *next; \
	struct xport_state_s *xport; \
	mutex_t	 list_mutex; \
	cond_t	  list_condit; \
	int		active_count; \
	robo_event_t    *first;
/* End of COMMON_IPORT_STATE */

/*
 * library_t. Information about the state of the media changer itlself
 */

#define	COMMON_LIBRARY_STATE \
	mutex_t	 mutex; \
	dev_ent_t	*un; \
	drive_state_t   *drive; \
	xport_state_t   *transports; \
	iport_state_t   *import; \
	iport_state_t   *im_ele; \
	iport_state_t   *ex_ele; \
	drive_state_t   *index; \
	int		audit_index; \
	int		countdown; \
	int		drives_auditing; \
	int		mesqid; \
	int		audit_tab_len; \
	int		catalog_fd; \
	int		preview_count; \
	equ_t	   eq; \
	union { \
    struct { \
	uint_t \
	two_sided  : 1,\
	except	: 1, \
		passthru   : 1; \
	}b; \
    uint_t   bits; \
	}status; \
	ushort_t	 ele_dest_len; \
	uchar_t	  chk_req; \
	mutex_t	 list_mutex; \
	cond_t	  list_condit; \
	mutex_t	 free_mutex; \
	int		free_count; \
	int		active_count; \
	robo_event_t    *free; \
	robo_event_t    *first;
/* End of COMMON_LIBRARY_STATE */


/* thread command structure */

typedef struct {
	struct library_s    *library;
	robo_event_t *event;
}robot_threaded_cmd;

#endif /* _COMMON_H */
