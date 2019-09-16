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
 * Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 *
 *    SAM-QFS_notice_end
 */

/* $Revision: 1.22 $ */

#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#ifdef sun
char	*ctime_r(const time_t *clock, char *buf, int buflen);
#endif /* sun */

#include <sys/vfs.h>

#define	BYTE_SWAP

#include <sam/param.h>
#include <sam/types.h>
#include <sam/sys_types.h>
#include <sam/quota.h>
#include <sam/resource.h>

#include <aml/remote.h>
#include <aml/catalog.h>

#include <sblk.h>
#include <share.h>
#include <samhost.h>
#include <dirent.h>
#include <ino_ext.h>
#include <acl.h>

#include <../src/fs/cmd/dump-restore/csd.h>
#include <../src/fs/cmd/dump-restore/old_resource.h>

#ifndef	MIN
#define	MIN(x, y)	(((x) < (y)) ? (x) : (y))
#endif /* MIN */

/*
 * This structure describes an element within some
 * object.  An object descriptor is a list of these,
 * terminated with a zero element size and count.
 */
enum printfn {
	pfn_object		= 0,
	pfn_hex			= 1,
	pfn_dec			= 2,
	pfn_udec		= 3,
	pfn_oct			= 4,
	pfn_char		= 5,
	pfn_str			= 6,
	pfn_sam_time		= 7,
	pfn_uid			= 8,
	pfn_gid			= 9,
	pfn_none
};

/*
 * All the stuff needed to print out a structure element,
 * and/or byte-swap it.  An array of these, sorted in element
 * offset order, and terminated with an elsize == 0, describes
 * a structure, esp. including how to print it and how to
 * byte-swap its elements.
 */
struct element_descriptor {
	size_t offset;				/* elem offset within struct */
	size_t elsize;				/* element size */
	size_t count;				/* number of elements */
	struct element_descriptor *eldesc;	/* desc, if not primitive */
	char *name;				/* print name for symbol */
	enum printfn printfn;			/* print function for symbol */
};

/*
 * A print descriptor.  An element descriptor after the potential
 * for recursion has been eliminated.  Elements must be ordered
 * by offset.
 */
struct print_descriptor {
	size_t offset;
	size_t elsize;
	size_t count;
	char *name;
	enum printfn printfn;
};


/*
 * Things needed to byte-swap an existing structure in-place.
 * No names, no print stuff, and no recursion.  Adjacent
 * elements of the same size are combined, and elements
 * that don't need byteswapping (padding bytes, char arrays)
 * are omitted.
 *
 * A copy descriptor is identical, but may include items with
 * elsize == 1 and does not need to include inter-element padding.
 */
struct swap_descriptor {
	size_t offset;
	size_t elsize;
	size_t count;
};

struct descriptor_desc {
	char *name;				/* object's name */
	struct element_descriptor *desc;	/* object's descriptor */
	size_t	size;				/* object's size */
};

void print_copy_descriptor(char *name, struct element_descriptor *);

static int sam_byte_swap(struct element_descriptor *, void *, size_t);
static int bswap(void *, size_t, size_t);

struct element_descriptor *explode_descriptor(char *name,
					struct element_descriptor *ep,
					struct element_descriptor *r);
struct element_descriptor *compress_descriptor(
					char *, struct element_descriptor *);
struct element_descriptor *copy_object(struct element_descriptor *);

void print_object(struct element_descriptor *, void *, size_t);
void print_full_descriptor(char *, struct element_descriptor *, size_t);
void print_swap_descriptor(char *, struct element_descriptor *, size_t);
void print_swap_compressed_descriptor(char *,
				struct element_descriptor *, size_t);

void print_hex(struct element_descriptor *, void *, size_t);
void print_dec(struct element_descriptor *, void *, size_t);
void print_udec(struct element_descriptor *, void *, size_t);
void print_oct(struct element_descriptor *, void *, size_t);
void print_char(struct element_descriptor *, void *, size_t);
void print_str(struct element_descriptor *, void *, size_t);
void print_sam_time(struct element_descriptor *, void *, size_t);
void print_uid(struct element_descriptor *, void *, size_t);
void print_gid(struct element_descriptor *, void *, size_t);
void print_none(struct element_descriptor *, void *, size_t);

/*
 * Print function definitions.
 */
void (*printfns[])(struct element_descriptor *, void *, size_t) = {
	print_object,
	print_hex,
	print_dec,
	print_udec,
	print_oct,
	print_char,
	print_str,
	print_sam_time,
	print_uid,
	print_gid,
	print_none
};


#define	START_DESC(x)	struct element_descriptor x ## _descriptor[] = {
#define	REMAP(s, t, n, c) \
	{ offsetof(s, n), 1, sizeof (t), NULL, #n, pfn_char }
#define	ATOMIC(s, t, n, c) \
	{ offsetof(s, n), sizeof (t), c, NULL, #n, pfn_hex }
#define	PRIMITIVE(s, t, n, c) \
	{ offsetof(s, n), sizeof (t), c, NULL, #n, pfn_hex }
#define	OBJECT(s, t, d, n, c) \
	{ offsetof(s, n), sizeof (t), c, d, #n, pfn_object }
#define	END_DESC(s)		{ sizeof (s), 0, 0, NULL, NULL, pfn_none } }

#define	STARTLIST	struct descriptor_desc descriptors[] = {
#define	LIST(n, d, s) { n, d, s }
#define	ENDLIST { "", NULL, 0 } };

#include "decls.c"



void check_descriptor(char *, struct element_descriptor *, size_t);

/*
 * command line flags
 */
int allflag = 0;
int copyflag = 0;
int printflag = 0;
int swapflag = 0;
int warnflag = 0;

int
main(int ac, char *av[])
{
	int c, i;
	struct descriptor_desc *dcp;
	char buf[16384];
	bzero(buf, sizeof (buf));

	while ((c = getopt(ac, av, "achpsw")) != EOF) {
		switch (c) {
		case 'a':
			allflag = 1;
			copyflag = 1;
			printflag = 1;
			swapflag = 1;
			break;

		case 'c':
			copyflag = 1;
			break;

		case 'h':
			printf("Usage:\n\t%s [-achps]\nall, copy, help, print, swap, warn", av[0]);
			exit(0);
			/* NOTREACHED */

		case 'p':
			printflag = 1;
			break;

		case 's':
			swapflag = 1;
			break;

		case 'w':
			warnflag = 1;
			break;

		default:
			printf("Unrecognized flag - '%c'\n", c);
			printf("Usage:\n\t%s [-achps]\n", av[0]);
			exit(1);
		}
	}

	for (i = 0; descriptors[i].desc != NULL; i++) {
		dcp = &descriptors[i];
		check_descriptor(dcp->name, dcp->desc, dcp->size);
		if (printflag) {
			print_copy_descriptor(dcp->name, dcp->desc);
		}
		if (copyflag) {
			print_full_descriptor(dcp->name, dcp->desc, dcp->size);
		}
		if (swapflag) {
			print_swap_descriptor(dcp->name, dcp->desc, dcp->size);
		}
	}

	if (allflag) {
		short i = 0x0102;
		int k = 0x12131415;
		long long m = 0x2122232425262728LL;

		print_object(sam_sbinfo_descriptor, (void *)buf, sizeof (buf));

		print_object(sam_sbord_descriptor, (void *)buf, sizeof (buf));

		print_object(sam_san_header_descriptor, (void *)buf,
		    sizeof (buf));

		bswap((void *)&i, sizeof (short), 1);
		bswap((void *)&k, sizeof (int), 1);
		bswap((void *)&m, sizeof (long long), 1);

		printf("0x0102 -> %#x, 0x12131415 -> %#x, "
		    "0x2122232425262728 -> %#llx\n",
		    (int)i, k, m);

		{
			struct element_descriptor *r, *s;

			print_copy_descriptor("sbinfo", sam_sbinfo_descriptor);
			r = explode_descriptor("swap_sbinfo",
			    sam_sbinfo_descriptor, NULL);
			print_copy_descriptor("exp_sbinfo", r);
			s = compress_descriptor("exp_sbinfo", r);
			print_copy_descriptor("cmp_exp_sbinfo", s);
			free(r);
			for (i = 0; i < sizeof (buf); i++) {
				buf[i] = i;
			}
			sam_byte_swap(s, (void *)buf, sizeof (buf));
			printf("\n\nsbinfo (byte-swapped)\n");
			print_object(sam_sbinfo_descriptor, (void *)buf,
			    sizeof (buf));
			printf("\n\n");
			free(s);
		}

		for (i = 0; i < sizeof (buf); i++) {
			buf[i] = i;
		}
		print_object(sam_disk_inode_descriptor, (void *)buf,
		    sizeof (buf));

		{
			struct element_descriptor *r, *s;

			print_copy_descriptor("disk_inode",
			    sam_disk_inode_descriptor);
			r = explode_descriptor("swap_disk_inode",
			    sam_disk_inode_descriptor, NULL);
			print_copy_descriptor("exp_disk_inode", r);
			s = compress_descriptor("exp_disk_inode", r);
			print_copy_descriptor("cmp_exp_disk_inode", s);
			free(r);

			for (i = 0; i < sizeof (buf); i++) {
				buf[i] = i;
			}
			sam_byte_swap(s, (void *)buf, sizeof (buf));
			printf("\n\ndisk inode (byte-swapped)\n");
			print_object(sam_disk_inode_descriptor, (void *)buf,
			    sizeof (buf));
			printf("\n\n");
			free(s);
		}
	}
	return (0);
}


/*
 * Make a copy of an object descriptor and return it.
 */
struct element_descriptor *
copy_object(struct element_descriptor *ep)
{
	int i;
	struct element_descriptor *rp;

	rp = ep;
	for (i = 0; rp->elsize != 0; i++) {
		rp++;
	}

	rp = (struct element_descriptor *)malloc((i + 1) *
	    sizeof (struct element_descriptor));
	if (rp == NULL) {
		return (NULL);
	}
	bcopy((char *)ep, (char *)rp, (i + 1) *
	    sizeof (struct element_descriptor));
	return (rp);
}


/*
 * Print out a descriptor. (non-recursive)
 */
void
print_copy_descriptor(
	char *name,
	struct element_descriptor *ep)
{
	printf("struct element_descriptor %s_copy_descriptor[] =  {\n", name);
	while (ep->elsize != 0) {
		printf("\t{ %lld, %lld, %lld, %s NULL, %d, \"%s\" },\n",
		    (long long)ep->offset,
		    (long long)ep->elsize,
		    (long long)ep->count,
		    ep->eldesc ? "/* fn */" : "",
		    ep->printfn,
		    ep->name);
		ep++;
	}
	printf("\t{ 0, 0, 0, NULL, 0, NULL }\n");
	printf("};\n\n");
}


/*
 * Examine a descriptor; note any gaps or overlaps in it.
 * (Non-recursive)
 */
void
check_descriptor(
	char *name,
	struct element_descriptor *ep,
	size_t size)
{
	int offset = 0;

	while (ep->elsize != 0) {
		if (offset != ep->offset) {
			if (offset > ep->offset) {
				printf("ERROR -- element overlap in "
				    "structure\n");
			}
			if (warnflag) {
				printf("%s: gap prior to symbol '%s'; %d "
				    "bytes\n",
				    name, ep->name,
				    (int)ep->offset - (int)offset);
			}
		}
		if (ep->eldesc != NULL && ep->printfn != pfn_object) {
			printf("%s: object (%s) element print function "
			    "!= pfn_object\n",
			    name, ep->name);
		}
		switch (ep->elsize) {
		case 1:
		case 2:
		case 4:
		case 8:
			break;
		default:
			if (ep->eldesc == NULL && ep->elsize != 1) {
				printf("%s: primitive element (%s) has odd "
				    "size (%d)\n",
				    name, ep->name, ep->elsize);
			}
		}
		offset = ep->offset + ep->elsize * ep->count;
		ep++;
	}
	if (warnflag && offset != size) {
		printf("%s: structure size (%d) != structure descriptor "
		    "size (%d)\n",
		    name, size, offset);
	}
}


/*
 * Walk a list of element_descriptor elements, calling
 * the appropriate print functions in each case.
 */
void
print_object(struct element_descriptor *ep, void *obp, size_t len)
{
	for (; ep->elsize != 0; ep++) {
		int l, n;
		char *o;

		o = (char *)obp;
		o += ep->offset;
		l = len - ep->offset;
		for (n = 0; n < ep->count; n++) {
			if (ep->elsize > l) {
				printf("print_object:  object overrun\n");
				return;
			}
			if (ep->eldesc != NULL) {
				if (ep->count > 1) {
					printf("%s[%d] = {\n", ep->name, n);
				} else {
					printf("%s = {\n", ep->name);
				}
				(*printfns[ep->printfn])(ep->eldesc,
				    (void *)o, l);
				printf("}\n");
			} else {
				(*printfns[ep->printfn])(ep, (void *)o, l);
				break;
			}
			o += ep->elsize;
			l -= ep->elsize;
		}
	}
}


/*
 * Print out the object descriptor for a swap object.
 * This also notes any gaps in the structure and includes
 * extra entries for them.
 */
void
print_full_descriptor(char *name, struct element_descriptor *ep, size_t size)
{
	int offset;

	if (ep == NULL) {
		printf("%s { UNDEFINED }\n", name);
		return;
	}
	printf("struct element_descriptor %s_full_descriptor[] = {\n", name);
	offset = 0;
	while (ep->elsize != 0) {
		if (offset > size) {
			printf("ERROR -- DESCRIBED STRUCT LARGER THAN "
			    "STRUCT\n");
		}
		if (offset != ep->offset) {
			printf("\t{ %d, %d, %d, NULL, %d, \"%s\" },	"
			    "/* gap */\n",
			    (int)offset, 1, (int)ep->offset - (int)offset,
			    (int)pfn_none, "[pad]");
		}
		printf("\t{ %lld, %lld, %lld, %s NULL, %d, \"%s\" },\n",
		    (long long)ep->offset,
		    (long long)ep->elsize,
		    (long long)ep->count,
		    ep->eldesc ? "/* fn */" : "",
		    ep->printfn,
		    ep->name);
		offset = ep->offset + ep->elsize * ep->count;
		ep++;
	}
	if (offset != size) {
		if (offset > size) {
			printf("ERROR ERROR DESCRIBED STRUCT LARGER THAN "
			    "STRUCT\n");
		}
		printf("\t{ %d, %d, %d, NULL, %d, \"%s\" },	/* gap */\n",
		    (int)offset, 1, size - offset,
		    (int)pfn_none, "[pad]");
	}
	printf("\t{ 0, 0, 0, NULL, 0, NULL }\n");
	printf("};\n\n");
}


/*
 * Generate a descriptor suitable for doing the byteswap operations
 * on a structure in-place.  So we can ignore all the character
 * arrays (that don't need to be swapped (or copied)), and all
 * the padding, &c.  Print it.
 */
void
print_swap_descriptor(char *name, struct element_descriptor *ep, size_t size)
{
	struct element_descriptor *eep, *cep;

	eep = explode_descriptor(name, ep, NULL);
	if (eep == NULL) {
		printf("%s { UNDEFINED };\n", name);
		return;
	}
	check_descriptor(name, eep, size);
	cep = compress_descriptor(name, eep);
	print_swap_compressed_descriptor(name, cep, size);
}


/*
 * Print the compressed descriptor for an object.  This
 * ignores padding elements and joins adjacent elements
 * that have the same size.
 */
void
print_swap_compressed_descriptor(
	char *name,
	struct element_descriptor *xp,
	size_t size)
{
	int i;
	struct element_descriptor xpc[256];
	struct element_descriptor *enext, *ep;

	if (xp == NULL) {
		printf("%s = { UNDEFINED };\n", name);
		return;
	}
	for (i = 0; i < 256; i++) {
		bcopy((char *)&xp[i], (char *)&xpc[i],
		    sizeof (struct element_descriptor));
		if (xp[i].elsize == 0) {
			break;
		}
	}
	ep = &xpc[0];
	enext = &xpc[1];
	printf("\nstruct swap_descriptor %s_swap_descriptor[] = {\n", name);
	while (ep->elsize != 0) {
		if (ep->elsize == enext->elsize &&
		    ep->eldesc == enext->eldesc &&
		    ep->offset + (ep->elsize * ep->count) ==
		    enext->offset) {
			ep->count += enext->count;
		} else {
			/* no need to byte-swap chars */
			if (ep->elsize != 1) {
				printf("\t{ %lld, %lld, %lld },\n",
				    (long long)ep->offset,
				    (long long)ep->elsize,
				    (long long)ep->count);
			}
			ep = enext;
		}
		if (enext->elsize != 0) {
			enext++;
		}
	}
	printf("\t{ %lld, %lld, %lld },\n",
	    (long long)ep->offset, (long long)ep->elsize, (long long)ep->count);
	printf("};\n");
}


/*
 * Walk a list of element descriptors, unwinding any recursion
 * along the way.  If the last argument is NULL, allocate an
 * array to put the result in, otherwise use the last argument
 * to store the expanded result.  (Users should set the argument
 * to NULL; this routine calls itself recursively passing
 * non-NULL addresses.)
 */
struct element_descriptor *
explode_descriptor(
	char *name,
	struct element_descriptor *ep,
	struct element_descriptor *r)
{
	int n = 0;
	struct element_descriptor result[5000], *rp;

	if (r == NULL) {
		bzero((char *)result, sizeof (result));
		r = &result[0];
	}

	while (ep->elsize != 0) {
		if (ep->eldesc) {
			int i;

			for (i = 0; i < ep->count; i++) {
				explode_descriptor(ep->name, ep->eldesc,
				    &r[n]);
				while (r[n].elsize != 0) {
					r[n].offset +=
					    ep->offset + i * ep->elsize;
					n++;
					if (n >= 256) {
						printf("explode_descriptor: "
						    "overflow(2)\n");
						return (NULL);
					}
				}
			}
		} else {
			r[n++] = *ep;
			if (n >= 256) {
				printf("explode_descriptor: overflow(1)\n");
				return (NULL);
			}
		}
		ep++;
	}
	r[n++] = *ep;		/* copy termination record */
	if (n >= 256) {
		printf("explode_descriptor: overflow(3)\n");
		return (NULL);
	}
	if (r != &result[0]) {	/* recursive call? */
		return (r);
	}

	/*
	 * save result to malloc'd array and return.
	 */
	rp = (struct element_descriptor *)malloc(n *
	    sizeof (struct element_descriptor));
	if (rp) {
		bcopy((char *)&result[0], (char *)rp,
		    n * sizeof (struct element_descriptor));
	}
	return (rp);
}


/*
 * Walk an element descriptor list (non-recursive), combining
 * adjacent entries if their types have the same sizes.  Return
 * a copy of the new list.
 */
struct element_descriptor *
compress_descriptor(
	char *name,
	struct element_descriptor *ep)
{
	int n = 0;
	struct element_descriptor *enext = ep;
	struct element_descriptor result[256];

	if (ep == NULL) {
		return (NULL);
	}
	enext++;
	while (ep->elsize != 0) {
		if (ep->elsize == enext->elsize &&
		    ep->eldesc == enext->eldesc &&
		    ep->offset + (ep->elsize * ep->count) == enext->offset) {
			ep->count += enext->count;
		} else {
			if (ep->elsize != 1) { /* no need to byte-swap chars */
				result[n++] = *ep;
			}
			ep = enext;
		}
		if (enext->elsize != 0) {
			enext++;
		}
	}
	bcopy((char *)enext, (char *)&result[n], sizeof (result[n]));
	++n;

	/*
	 * save result to malloc'd array and return.
	 */
	enext = (struct element_descriptor *)malloc(n *
	    sizeof (struct element_descriptor));
	if (enext) {
		bcopy((char *)&result[0], (char *)enext,
		    n * sizeof (struct element_descriptor));
	}
	return (enext);
}


static inline void sam_bswap2(void *buf, size_t count);
static inline void sam_bswap4(void *buf, size_t count);
static inline void sam_bswap8(void *buf, size_t count);


/*
 * Byte swap 'count' objects of size 'size' pointed at by *obj.
 */
static int
bswap(void *obj, size_t size, size_t count)
{
	switch (size) {
	case 2:
		sam_bswap2(obj, count);
		break;

	case 4:
		sam_bswap4(obj, count);
		break;

	case 8:
		sam_bswap8(obj, count);
		break;

	default:
		printf("ERROR -- bswap object size (%d) not 2^n (n >= 1)\n",
		    size);
		return (1);
	}
	return (0);
}


/*
 * sam_byte_swap -- reorder bytes in a data structure
 *
 * Takes three arguments -- a descriptor listing the elements
 * that need reordering, a buffer address, and a length.  The
 * descriptor lists the elements in the buffer (usu. sorted by
 * offset), their sizes, and the number of consecutive elements.
 *
 * Each element is assumed to be naturally aligned -- if not,
 * then this code will break (core dump).  Each element's bytes
 * are reversed; if consecutive elements fit into whole, aligned
 * words (or half-words), we do words (half-words) at a time to
 * speed things.
 *
 * It's also possible that this code will be reworked at some
 * future date to support packing structures as well.  If that
 * happens, the descriptor structure will change substantially.
 *
 * This routine returns 0 on success, < 0 if the buffer is overrun
 * or data elements are non-power-of-2 sized.
 */
int
sam_byte_swap(
	struct element_descriptor *ep,
	void *buf,
	size_t len)
{
	int l;
	char *op;

	while (ep->elsize != 0) {
		op = (char *)buf + ep->offset;
		l = len - ep->offset;
		if (ep->count * ep->elsize > l) {
			printf("sam_byte_swap: ERROR -- object overrun\n");
			return (-1);
		}
		switch (ep->elsize) {
		case 2:
			sam_bswap2(op, ep->count);
			break;

		case 4:
			sam_bswap4(op, ep->count);
			break;

		case 8:
			sam_bswap8(op, ep->count);
			break;

		default:
			printf("sam_byte_swap: "
			    "ERROR -- object size (%d) not 2^n (n >= 1)\n",
			    ep->elsize);
			return (-1);
		}
		ep++;
	}
	return (0);
}

#ifdef _LP64
#define		PTR_32B_ALGND(p)		((((long)p) & 03) == 0)
#define		PTR_64B_ALGND(p)		((((long)p) & 07) == 0)
#else
#define		PTR_32B_ALGND(p)		((((int)p) & 03) == 0)
#define		PTR_64B_ALGND(p)		((((int)p) & 07) == 0)
#endif /* _LP64 */

/*
 * sam_bswapX -- byte swap consecutive data elements
 *
 * Byte swap consecutive data items of size X given
 * their address and a count.
 *
 * Test for alignment, and half-words (32 bits) or words (64 bits)
 * at a time if possible.  We use shift and mask to shuffle things
 * about.
 *
 * Mildly machine dependent.  Assumes that byte pointers' alignment
 * can be determined from their least-significant bits.
 */
static inline void
sam_bswap2(void *buf, size_t count)
{
	uint16_t *op = (uint16_t *)buf;

	while (count) {
		if (count >= 4 && PTR_64B_ALGND(op)) {
			uint64_t v, *vp = (uint64_t *)op;

			/* 4 for the price of 1 */
			v = *vp;
			/* swap even and odd bytes */
			v = ((v & 0xff00ff00ff00ffLL) << 8)
			    | ((v >> 8) & 0xff00ff00ff00ffLL);
			*vp = v;
			op += 4;
			count -= 4;
		} else if (count >= 2 && PTR_32B_ALGND(op)) {
			uint32_t v, *vp = (uint32_t *)op;

			/* 2 for the price of 1 */
			v = *vp;
			/* swap even and odd bytes */
			v = ((v & 0xff00ff) << 8) | ((v >> 8) & 0xff00ff);
			*vp = v;
			op += 2;
			count -= 2;
		} else {
			uint16_t v;

			v = *op;
			/* swap bytes */
			v = (v << 8) | (v >> 8);
			*op = v;
			op += 1;
			count -= 1;
		}
	}
}


static inline void
sam_bswap4(void *buf, size_t count)
{
	uint32_t *op = (uint32_t *)buf;

	while (count) {
		if (count >= 2 && PTR_64B_ALGND(op)) {
			uint64_t v, *vp = (uint64_t *)op;

			/* 2 for the price of 1 */
			v = *vp;
			/* swap 16-bit chunks */
			v = ((v & 0xffff0000ffffLL) << 16)
			    | ((v >> 16) & 0xffff0000ffffLL);
			/* swap bytes */
			v = ((v & 0xff00ff00ff00ffLL) << 8)
			    | ((v >> 8) & 0xff00ff00ff00ffLL);
			*vp = v;
			op += 2;
			count -= 2;
		} else {
			uint32_t v;

			v = *op;
			/* swap 16-bit chunks */
			v = (v << 16) | (v >> 16);
			/* swap bytes */
			v = ((v & 0xff00ff) << 8) | ((v >> 8) & 0xff00ff);
			*op = v;
			op += 1;
			count -= 1;
		}
	}
}


static inline void
sam_bswap8(void *buf, size_t count)
{
	uint64_t *op = (uint64_t *)buf;

	while (count) {
		uint64_t v;

		v = *op;
		/* swap 32-bit chunks */
		v = (v << 32) | (v >> 32);
		/* swap 16-bit chunks */
		v = ((v & 0xffff0000ffffLL) << 16)
		    | ((v >> 16) & 0xffff0000ffffLL);
		/* swap bytes */
		v = ((v & 0xff00ff00ff00ffLL) << 8)
		    | ((v >> 8) & 0xff00ff00ff00ffLL);
		*op = v;
		op += 1;
		count -= 1;
	}
}



/*
 * Print functions for the element descriptors.  Each of the
 * functions below takes an element descriptor, and prints
 * out ep->count objects of size ep->elsize pointed at by
 * obp in the appropriate format.  (hex, oct, dec, udec, ...)
 *
 * 'len' is the number of valid bytes remaining in the input
 * buffer; it is checked against the size/count for overflow.
 */

/*
 * Print Hexadecimal
 */
void
print_hex(struct element_descriptor *ep, void *obp, size_t len)
{
	int i;
	char *o = (char *)obp;

	printf("%s = ", ep->name);
	for (i = 0; i < ep->count; i++) {
		if (&o[ep->elsize] - (char *)obp > len) {
			printf("print_hex: object overrun");
			return;
		}
		switch (ep->elsize) {
		case 1:
			printf("%#x ", ((int)o[0]) & 0xff);
			break;
		case 2:
			printf("%#x ", ((int)((short *)o)[0]) & 0xffff);
			break;
		case 4:
			printf("%#llx ", (((int *)o)[0]) & 0xffffffffLL);
			break;
		case 8:
			printf("%#llx ", ((long long *)o)[0]);
			break;
		default:
			printf("print_hex: bad print object size (%d), "
			    "name = %s\n",
			    ep->elsize, ep->name);
		}
		o += ep->elsize;
	}
	printf("\n");
}


/*
 * Print decimal
 */
void
print_dec(struct element_descriptor *ep, void *obp, size_t len)
{
	int i;
	char *o = (char *)obp;

	printf("%s = ", ep->name);
	for (i = 0; i < ep->count; i++) {
		if (&o[ep->elsize] - (char *)obp > len) {
			printf("print_dec: object overrun");
			return;
		}
		switch (ep->elsize) {
		case 1:
			printf("%d ", ((int)o[0]) & 0xff);
			break;
		case 2:
			printf("%d ", ((int)((short *)o)[0]) & 0xffff);
			break;
		case 4:
			printf("%lld ", (((int *)o)[0]) & 0xffffffffLL);
			break;
		case 8:
			printf("%lld ", ((long long *)o)[0]);
			break;
		default:
			printf("print_dec: bad print object size (%d), "
			    "name = %s\n",
			    ep->elsize, ep->name);
		}
		o += ep->elsize;
	}
	printf("\n");
}


/*
 * Print unsigned decimal
 */
void
print_udec(struct element_descriptor *ep, void *obp, size_t len)
{
	int i;
	char *o = (char *)obp;

	printf("%s = ", ep->name);
	for (i = 0; i < ep->count; i++) {
		if (&o[ep->elsize] - (char *)obp > len) {
			printf("print_udec: object overrun");
			return;
		}
		switch (ep->elsize) {
		case 1:
			printf("%u ", ((int)o[0]) & 0xff);
			break;
		case 2:
			printf("%u ", ((int)((short *)o)[0]) & 0xffff);
			break;
		case 4:
			printf("%llu ", (((int *)o)[0]) & 0xffffffffLL);
			break;
		case 8:
			printf("%llu ", ((long long *)o)[0]);
			break;
		default:
			printf("print_udec: bad print object size (%d), "
			    "name = %s\n",
			    ep->elsize, ep->name);
		}
		o += ep->elsize;
	}
	printf("\n");
}


/*
 * Print octal
 */
void
print_oct(struct element_descriptor *ep, void *obp, size_t len)
{
	int i;
	char *o = (char *)obp;

	printf("%s = ", ep->name);
	for (i = 0; i < ep->count; i++) {
		if (&o[ep->elsize] - (char *)obp > len) {
			printf("print_oct: object overrun");
			return;
		}
		switch (ep->elsize) {
		case 1:
			printf("%o ", ((int)o[0]) & 0xff);
			break;
		case 2:
			printf("%o ", ((int)((short *)o)[0]) & 0xffff);
			break;
		case 4:
			printf("%llo ", (((int *)o)[0]) & 0xffffffffLL);
			break;
		case 8:
			printf("%llo ", ((long long *)o)[0]);
			break;
		default:
			printf("print_oct: bad print object size (%d), "
			    "name = %s\n",
			    ep->elsize, ep->name);
		}
		o += ep->elsize;
	}
	printf("\n");
}


/*
 * Print character array (don't stop for nulls).
 */
void
print_char(struct element_descriptor *ep, void *obp, size_t len)
{
	int i;
	char *cp = (char *)obp;

	if (ep->elsize != sizeof (char)) {
		printf("ERROR: print_char: element size != 1 (%d)\n",
		    ep->elsize);
		return;
	}
	if (ep->count > len) {
		printf("print_char: object overrun");
		return;
	}
	printf("%s = ", ep->name);
	for (i = 0; i < ep->count; i++, cp++) {
		if (*cp < 32) {
			printf("\\%#o", (int)*cp);
		} else {
			printf("%c", cp[i]);
		}
	}
	printf("\n");
}


/*
 * Print string (do stop for nulls)
 */
void
print_str(struct element_descriptor *ep, void *obp, size_t len)
{
	int count;
	char *cp = (char *)obp;
	char buf[129];

	count = MIN(sizeof (buf), len);
	count = MIN(count, ep->count);

	if (ep->elsize != sizeof (char)) {
		printf("ERROR: print_str: element size != 1 (%d)\n",
		    ep->elsize);
		return;
	}
	snprintf(buf, count, "%s = %s", ep->name, cp);
	buf[128] = 0;
	printf("%s\n", buf);
}


/*
 * Print out a sam_time value (seconds since 1970) in
 * date format.
 */
void
print_sam_time(struct element_descriptor *ep, void *obp, size_t len)
{
	time_t *tp = (time_t *)obp;
	int i;
	char buf[64];

	if (ep->elsize != sizeof (time_t)) {
		printf("ERROR: print_sam_time: element size (%d) != "
		    "time_t (%d)\n",
		    ep->elsize, sizeof (time_t));
		return;
	}
	printf("%s = ", ep->name);
	for (i = 0; i < ep->count; i++) {
		if ((char *)&tp[1] > (char *)obp + len) {
			printf("print_time: object overrun");
			return;
		}
#ifdef sun
		ctime_r(tp, buf, sizeof (buf));
#endif /* sun */
#ifdef linux
		ctime_r(tp, buf);
#endif /* linux */
		buf[24] = 0;
		printf("%s ", buf);
		tp++;
	}
	printf("\n");
}


/*
 * Print out a UID; just do it in decimal for now.
 */
void
print_uid(struct element_descriptor *ep, void *obp, size_t len)
{
	print_dec(ep, obp, len);
}


/*
 * Print out a GID; just do it in decimal for now.
 */
void
print_gid(struct element_descriptor *ep, void *obp, size_t len)
{
	print_dec(ep, obp, len);
}

/*
 * Dummy function for non-printing elements.
 */
/*ARGSUSED*/
void
print_none(struct element_descriptor *ep, void *obp, size_t len)
{
}
