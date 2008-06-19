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
# Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#
#    SAM-QFS_notice_end

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

PROG = copy
PROG_SRC = \
		archive_read.c \
		checksum.c \
		circular_io.c \
		copy.c \
		copyfile.c \
		disk_cache.c \
		dkstage.c \
		double_buffer.c \
		error_retry.c \
		filesys.c \
		hcstage.c \
		log.c \
		rmstage.c \
		stage.c \
		stage_reqs.c

DEPCFLAGS += -I../include $(HC_INCLUDE) $(THRCOMP)

SYSLIBS = -ldl -lgen $(THRLIBS) -lsocket -lnsl
SAMLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamcat -lsamut -lsam -lsamconf \
	-lsamfs -lsamapi -lsamrft
PROG_LIBS = -L ../lib/$(OBJ_DIR) -lstager $(SAMLIBS) $(HC_LIB) \
	$(SYSLIBS) $(LIBSO)

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L ../lib/$(OBJ_DIR) -lstager $(SAMLIBS)


include $(DEPTH)/mk/targets.mk

install: all
	$(INSTALL) $(SYSINST) $(OBJ_DIR)/$(PROG) $(ADMDEST)/sam-stagerd_$(PROG)

include $(DEPTH)/mk/depend.mk
