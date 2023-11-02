# $Revision: 1.11 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
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
LIB_HOME = $(DEPTH)/src/lib/samapi

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

LIB = samapi
LIB_SRC = \
	sam_audit.c \
	sam_chmed.c \
	sam_clear_request.c \
	sam_errno.c \
	sam_export.c \
	sam_import.c \
	sam_label.c \
	sam_live.c \
	sam_load.c \
	sam_move.c \
	sam_preview.c \
	sam_reserve.c \
	sam_set_fs_attr.c \
	sam_set_state.c \
	sam_settings.c \
	sam_unload.c \
	sam_utils.c

DEPCFLAGS += $(THRCOMP)
LIB_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut -lsamcat -lsamfs $(LIBSO) -lgen -lintl

ifeq ($(SPARCV9), yes)
LNOPTS = $(LIBS_LFLAGS64)
else
LNOPTS = $(LIBS_LFLAGS32)
endif
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
