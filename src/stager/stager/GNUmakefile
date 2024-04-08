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

PROG = stager
PROG_SRC = \
		compose.c \
		control.c \
		device.c \
		error_retry.c \
		filesys.c \
		log.c \
		messages.c \
		readcmd.c \
		stage_reqs.c \
		rmedia.c \
		schedule.c \
		stager.c \
		stream.c \
		thirdparty.c

DEPCFLAGS += -I../include -I$(INCLUDE)/aml/$(OBJ_DIR) $(THRCOMP)

SYSLIBS = -ldl -lgen $(THRLIBS) -lsocket -lnsl
SAMLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamcat -lsamut -lsam -lsamconf -lsamfs -lsamapi -lsammig
PROG_LIBS =  -L ../lib/$(OBJ_DIR) -lstager $(SAMLIBS) $(SYSLIBS) $(LIBSO)

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L ../lib/$(OBJ_DIR) -lstager $(SAMLIBS)

include $(DEPTH)/mk/targets.mk

install: all
	$(INSTALL) $(SYSINST) $(OBJ_DIR)/$(PROG) $(ADMDEST)/sam-$(PROG)d

include $(DEPTH)/mk/depend.mk
