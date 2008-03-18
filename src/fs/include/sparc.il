! Extracted from Solaris 9 kernel source by ABR 2003-07-14.
!
! Copyright (c) 1992-1998 by Sun Microsystems, Inc.
! All rights reserved.
!
#ident	"@(#)sparc.il	1.8	99/01/05 SMI"
!
! In-line functions for sparc kernels.
!

! return current thread pointer

	.inline	threadp,0
	.register %g7, #scratch
	mov	%g7, %o0
	.end

! return caller

	.inline	caller,0
	mov	%i7, %o0
	.end

! return callee

	.inline	callee,0
	mov	%o7, %o0
	.end

! atomic load-double and store-double for 32-bit operation

	.inline load_double,4
	ldd     [%o0],%o0
	.end

	.inline store_double,12
	std     %o0,[%o2]
	.end
