# $Revision: 1.18 $

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

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

ifeq ($(OS), SunOS)
# SunOS

MPROGS = samtrace trace
samtrace_SRC = samtrace.c
trace_SRC	 = trace.c

CFLAGS_CSTD += -xO2 -mt -lpthread
CFLAGS_GCC += -t -lpthread
CFLAGS_$(COMPILER) = $(CFLAGS_$(COMPILER))

else
# Linux

PROG = samtrace
PROG_SRC = samtrace.c

LDFLAGS += -lpthread
CFLAGS += -D_GNU_SOURCE

endif
# End ifeq ($(OS), SunOS)

DEPCFLAGS += -I../../include -I../../include/$(OBJ_DIR)

PROG_LIBS = $(LIBSO) -L$(DEPTH)/lib/$(OBJ_DIR) -lsamconf
samtrace_LIBS = $(PROG_LIBS) -L /usr/lib$(64DIR) -lelf -lgen -lkvm -lc
trace_LIBS	  = $(PROG_LIBS) -L /usr/lib$(64DIR) -lelf -lgen -lkvm -lc

LNOPTS = -numxs -erroff=E_BAD_FORMAT_STR2 -Dlint
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
