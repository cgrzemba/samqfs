# $Revision: 1.14 $

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

include $(DEPTH)/mk/common.mk

PROG = samu
PROG_SRC = \
		command.c \
		dis_archiver.c \
		dis_catalog.c \
		dis_client.c \
		dis_config.c \
		dis_device.c \
		dis_diskvols.c \
		dis_fs.c \
		dis_inode.c \
		dis_kstat.c \
		dis_license.c \
		dis_mount.c \
		dis_ms.c \
		dis_prequeue.c \
		dis_preview.c \
		dis_queue.c \
		dis_remote.c \
		dis_remove.c \
		dis_sector.c \
		dis_sense.c \
		dis_shm.c \
		dis_spm.c \
		dis_staging.c \
		dis_trace.c \
		dis_udt.c \
		memdis.c \
		samu.c \
		utility.c

DEPCFLAGS += -I$(INCLUDE)/pub/$(OBJ_DIR) -I$(DB_INCLUDE)

PROG_LIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamapi -lsam -lsamcat -lsamfs -lsamut -lsamconf -lsamrft $(LIBSO) $(THRLIBS) -lsocket -lnsl -lcurses -lkvm -lkstat -ladm $(DB_LIB)

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsamapi -lsam -lsamcat -lsamfs -lsamut -lsamconf -lsamrft

include $(DEPTH)/mk/targets.mk

install: 	all
	$(INSTALL) $(SYSINST) $(OBJ_DIR)/$(PROG) $(ADMDEST)
	rm -f $(ADMDEST)/samcmd
	ln -s $(ADMDEST)/samu $(ADMDEST)/samcmd

include $(DEPTH)/mk/depend.mk
