# $Revision: 1.14 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or https://illumos.org/license/CDDL.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at pkg/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

#
# Build kernel symbol table entries for mdb's use.  This should
# only be defined in GNUmakefiles that define MODULE_SRC.
#
BUILD_STABS = yes

MODULE = samioc
MODULE_SRC = samioc.c

DEPCFLAGS += $(KERNFLAGS) $(VERS)

CFLAGS_CSTD  = $(KERNFLAGS) -xO3
CFLAGS_GCC  = $(KERNFLAGS) -O3
CLFAGS += $(CFLAGS_$(COMPILER))

LD = ld
LDFLAGS = -r

LNOPTS += -n
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
