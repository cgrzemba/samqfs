# $Revision: 1.13 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

PROG = fsconfig
PROG_SRC = fsconfig.c

ifeq ($(OS), Linux)
INCFLAGS = $(OSFLAGS) -I$(LINUX_INCLUDE) -I$(INCLUDE) -I../../include -I../../include/$(OBJ_DIR)
else
INCFLAGS = -I$(INCLUDE) -I../../include -I../../include/$(OBJ_DIR)
endif
DEPCFLAGS = $(INCFLAGS) $(VERS) $(EFI_SUPPORT) $(METADATA_SERVER) $(DEBUGCDEFS) $(THRCOMP)

PROG_LIBS = -L ../../lib/$(OBJ_DIR) -L $(DEPTH)/lib/$(OBJ_DIR) \
	$(STATIC_OPT) -lfscmd $(DYNAMIC_OPT) -lsamut -lsamconf $(LIBSO) $(EFI_LIBS)

ifeq ($(OS), Linux)
LDFLAGS += ../../lib/$(OBJ_DIR)/setsyscall.o
PROG_LIBS += -ldl
endif

LNOPTS = -amnuxs -Dlint
LNLIBS = -L ../../lib/$(OBJ_DIR) -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
