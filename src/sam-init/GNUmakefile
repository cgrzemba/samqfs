# $Revision: 1.15 $

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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

PROG = sam-amld
PROG_SRC = \
		amld.c \
		child.c \
		defaults.c \
		fifos.c \
		fifo_cmds.c \
		fifo_fs.c \
		mkshm.c \
		previewcmd.c \
		server.c \
		samfm_service.c

DEPCFLAGS += $(THRCOMP) -I$(DEPTH)/include/pub/$(OBJ_DIR) -I/usr/include/libxml2
PROG_LIBS = ../fs/lib/$(OBJ_DIR)/libfscmd.a \
	-L $(DEPTH)/lib/$(OBJ_DIR) $(STATIC_OPT) -lsamspm $(LIBSO) \
	$(DYNAMIC_OPT) -lsamcat -lsamfs -lsamut -lsamconf \
	-lsocket -lnsl -lgen -ldl $(THRLIBS) -lposix4 -ladm

LNOPTS = $(CMDS_LFLAGS32)
LNOPTS += -Dlint
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamcat -lsamfs -lsamut -lsamconf

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
