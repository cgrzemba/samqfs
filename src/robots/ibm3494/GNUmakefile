# $Revision: 1.10 $

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

SRC_VPATH = ../common
vpath %.c $(SRC_VPATH)

PROG = ibm3494
PROG_SRC =  \
	clear.c \
	delay_resp.c \
	ibm_drive.c \
	ibm_misc.c \
	init.c	\
	main.c \
	man_work.c \
	media_move.c \
	message.c \
	transport.c	\
	work_cmds1.c

#
# Additional source files from ../common
#
PROG_SRC += \
	misc.c \
	down.c \
	drive.c \
	drive1.c \
	work_cmds.c \
	chk_drives.c \
	common_scsi.c

DEPCFLAGS += $(THRCOMP)
LDFLAGS += ../../../lib/$(OBJ_DIR)/libibm.o
PROG_LIBS = -lsocket $(STATIC_OPT) -L ../lib/$(OBJ_DIR) -lrobots \
	$(DYNAMIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) \
	-lsamfs -lsamut -lsamcat -lsam $(THRLIBS) -lgen $(LIBSO)

LNOPTS = -a -Dlint 
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
