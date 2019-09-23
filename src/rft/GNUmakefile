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

DEPTH = ../..

include $(DEPTH)/mk/common.mk

PROG = rftd
PROG_SRC = \
		command.c \
		crew.c \
		rft.c \
		log.c \
		readcmd.c \
		worker.c

PROG_LIBS = $(STATIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) -lsamspm $(DYNAMIC_OPT) \
	-lsamut -lsam -lsamcat -lsamconf -lsamfs -lsamapi \
	-lsocket -lnsl -ldl -lgen \
	$(THRLIBS) $(LIBSO)

DEPCFLAGS += -I$(INCLUDE)/aml/$(OBJ_DIR) $(THRCOMP) $(OSDEPCFLAGS)

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamut -lsamcat -lsamconf \
	-lsamfs -lsamapi

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
