/*
 * memdis.c - Memory display functions.
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

#pragma ident "$Revision: 1.19 $"


/* ANSI headers. */
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

/* POSIX headers. */
#include <fcntl.h>

/* Solaris headers. */
#include <kvm.h>

/* SAM-FS headers. */
#include "sam/types.h"
#include "sam/nl_samfs.h"
#include "sam/custmsg.h"

/* Local headers. */
#undef ERR
#include "samu.h"

/* Private data. */
static struct MemPars cmp;
static struct MemPars psmp;
static struct MemPars shmp;
static unsigned long long caddr = 0ULL;

#define	PREAD pread64

/*
 * Display memory.
 */
void
DisCmem(void)
{
	int kd;
	int n;

	if ((kd = open("/dev/kmem", O_RDONLY | SAM_O_LARGEFILE)) < 0) {
		Error(catgets(catfd, SET, 619, "Cannot open kernel memory."));
	}
	n = PREAD(kd, (char *)Buffer, sizeof (Buffer), (offset_t)caddr);
	close(kd);
	if (n < 0)
		Error(catgets(catfd, SET, 747, "Could not read memory %llx"),
		    caddr);
	cmp.base = Buffer;
	cmp.size = sizeof (Buffer);
	Mvprintw(0, 10, catgets(catfd, SET, 7004, "base: 0x%llx"), caddr);
	DisMem(&cmp, 2);
}


/*
 * Display initialization.
 */
boolean
InitCmem(void)
{
	if (Argc > 1) {
		caddr = strtoull(Argv[1], NULL, 16);
		cmp.offset = 0;
	} else if (caddr == 0ULL) {
		Error((const char *)GetCustMsg(7005));
	}
	return (TRUE);
}


/*
 * Keyboard processor for memory.
 */
boolean
KeyCmem(char key)
{
	return (KeyMem(&cmp, key));
}


/*
 * Display preview shared memory.
 */
void
DisPshmem(void)
{
	Mvprintw(0, 24, catgets(catfd, SET, 2339, "size: %d"), psmp.size);
	DisMem(&psmp, 2);
}


/*
 * Keyboard processor for preview shared memory.
 */
boolean
KeyPshem(char key)
{
	return (KeyMem(&psmp, key));
}


/*
 * Initialize preview shared memory.
 */
boolean
InitPshem(void)
{
	shm_ptr_tbl_t *shm;	/* Shared memory pointer table */

	if (Argc > 1)  psmp.offset = strtoul(Argv[1], NULL, 0);
	/* LINTED pointer cast may result in improper alignment */
	shm = (shm_ptr_tbl_t *)SHM_ADDR(preview_shm, 0);
	psmp.base = (uchar_t *)shm;
	psmp.size = shm->shm_block.size;
	return (TRUE);
}


/*
 * Display shared memory.
 */
void
DisShem(void)
{
	Mvprintw(0, 20, catgets(catfd, SET, 2339, "size: %d"), shmp.size);
	DisMem(&shmp, 2);
}


/*
 * Keyboard processor for shared memory display.
 */
boolean
KeyShem(char key)
{
	return (KeyMem(&shmp, key));
}


/*
 * Initialize shared memory display.
 */
boolean
InitShem(void)
{
	shm_ptr_tbl_t *shm;	/* Shared memory pointer table */

	if (Argc > 1)  shmp.offset = strtoul(Argv[1], NULL, 0);
	/* LINTED pointer cast may result in improper alignment */
	shm = (shm_ptr_tbl_t *)SHM_ADDR(master_shm, 0);
	shmp.base = (uchar_t *)shm;
	shmp.size = shm->shm_block.size;
	return (TRUE);
}


/*
 * Display memory.
 */
void
DisMem(struct MemPars *p, int ln)
{
	uchar_t *fwa;
	int l, ll;
	uint_t addr, size;

	addr = p->offset & ~3;
	fwa = p->base + addr;
	size = p->size;
	ll = LINES - 4;
	l = 0;
#if 0
	Mvprintw(ln++, 0,
	    "base: %8x, offset: %8x, size: %d", p->base, p->offset, size);
#endif
	while (ln < ll) {
		static char hex[] = "0123456789abcdef";
		char str[80], *s = str;
		int	n;

		if (addr >= size)
			return;
		switch (p->fmt) {
		case 0: {	/* 32 bits per field */
			uchar_t *a = fwa;
			uchar_t v;
			int f;	/* field */

			for (f = 1; f <= 4; f++) {

				for (n = 0; n <= 3; n++) {
					v = *a++;
					*(s + (n*2) + 1) = hex[v & 0xf];
					v >>= 4;
					*(s + (n*2)) = hex[v & 0xf];
				}
				s += 8;
				*s++ = ' ';
				if ((f & 3) == 0)  *s++ = ' ';
			}
		}
		break;

		case 1: {	/* 16 bits per field */
			uchar_t *a = fwa;
			uchar_t v;
			int f;	/* field */

			for (f = 1; f <= 8; f++) {

				for (n = 0; n <= 1; n++) {
					v = *a++;
					*(s + (n*2) + 1) = hex[v & 0xf];
					v >>= 4;
					*(s+(n*2)) = hex[v & 0xf];
				}
				s += 4;
				*s++ = ' ';
				if ((f & 3) == 0)  *s++ = ' ';
			}
		}
		break;

		case 2: {	/* 8 bits per field */
			uchar_t *a = (uchar_t *)fwa;
			int f;	/* field */

			for (f = 1; f <= 16; f++) {
				int v = *a++;

				*s++ = hex[(v >> 4) & 0xf];
				*s++ = hex[v & 0xf];
				*s++ = ' ';
				if ((f & 3) == 0)  *s++ = ' ';
			}
		}
		break;

		}

		for (n = 0; n < 16; n++) {
			char c;

			c = *fwa++ & 0x7F;
			*s++ = (isprint(c)) ? c : '.';
		}
		*s = '\0';
		Mvprintw(ln++, 0, "%08x  %s", addr, str);
		addr += 16;
		if ((++l & 7) == 0)  ln++;
	}
}


/*
 * Keyboard processor for shared memory.
 */
boolean
KeyMem(struct MemPars *p, char key)
{
	boolean fwd = TRUE;
	int l;

	l = LINES - 6;
	l -= (l / 8);
	l *= 16;

	switch (key) {

	case KEY_full_fwd:
		break;

	case KEY_full_bwd:
		fwd = FALSE;
		break;

	case KEY_half_bwd:
		fwd = FALSE;
	/* FALLTHROUGH */
	case KEY_half_fwd:
		l /= 2;
		break;

	case KEY_adv_fmt:
		if (++p->fmt > 2)  p->fmt = 0;
		return (TRUE);

	case '0':
		p->offset = 0;
		return (TRUE);

	case '1':
		p->fmt = 2;
		return (TRUE);

	case '2':
		p->fmt = 1;
		return (TRUE);

	case '4':
		p->fmt = 0;
		return (TRUE);

	default:
		return (FALSE);
	}
	if (!fwd) {
		if (p->offset > l)  p->offset -= l;
		else  p->offset = 0;
	} else  if (p->offset + l < p->size)  p->offset += l;
	return (TRUE);
}
