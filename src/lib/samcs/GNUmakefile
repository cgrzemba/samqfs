# $Revision: 1.13 $

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
LIB_HOME = $(DEPTH)/src/lib/samcs

include $(DEPTH)/mk/common.mk

LIB = samcs
LIB_SRC = \
		csempty.c \
		csmd5.c \
		cssha1.c \
		cssimple.c \
		csuser.c \
		readcsum.c \
		writecsum.c

DEPCFLAGS += -I$(OBJ_DIR) $(THRCOMP) $(OSDEPCFLAGS)
# LIB_LIBS +=  -L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut -lsamcat $(LIBSO) \
# 	-ldl -lthread -lsysevent -lnvpair

ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif
LNLIBS =

include $(DEPTH)/mk/targets.mk

EXTRA_CFLAGS_GCC = -O4
EXTRA_CFLAGS_CSTD = xO4 -xunroll=8

include $(DEPTH)/mk/depend.mk
