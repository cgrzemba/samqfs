# $Revision: 1.8 $

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

PROG = nrecycler

DATAFILES = nrecycler.sh

PROG_SRC = 		\
	cmd.c		\
	crew.c		\
	csd.c		\
	dat.c		\
	device.c	\
	fs.c		\
	log.c		\
	media.c		\
	recycler.c

DEPCFLAGS += -I../include $(THRCOMP)

DIRS = ../lib

SAMLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamapi -lsamcat -lsamfs \
	-lsamut -lsamconf -lsamrft
PROG_LIBS =  $(SAMLIBS) $(LIBSO) $(THRLIBS) -lz -lgen -lintl -L ../lib/$(OBJ_DIR) -lrecycler

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = $(SAMLIBS)

include $(DEPTH)/mk/targets.mk

install: all
	$(INSTALL) $(SYSINST) $(OBJ_DIR)/$(PROG) $(ADMDEST)/sam-$(PROG)

include $(DEPTH)/mk/depend.mk
