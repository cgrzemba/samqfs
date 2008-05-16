# $Revision: 1.12 $

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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

SRC_VPATH = $(DEPTH)/src/fs/lib $(DEPTH)/src/lib/sammgmt/restore
vpath %c $(SRC_VPATH)

BUILD_64BIT = yes

PROG = samindexdump
PROG_SRC = samindexdump.c \
		bswap.c \
		cmn_csd_funcs.c

DEPCFLAGS += -I$(DEPTH) -I$(DEPTH)/src/fs/include \
		-I$(DEPTH)/src/fs/cmd/dump-restore -D_REENTRANT
PROG_LIBS += -lz

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
