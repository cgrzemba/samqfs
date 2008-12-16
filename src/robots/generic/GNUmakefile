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
OBJDEPTH = ../..

include $(DEPTH)/mk/common.mk

DIRS = scsi_error

SRC_VPATH = ../common
vpath %.c $(SRC_VPATH)

PROG = generic
PROG_SRC = \
	main.c \
	gen_drive.c \
	element.c \
	scsi_support.c \
	work_cmds1.c \
	message.c \
	man_work.c \
	transport.c \
	gen_misc.c \
	media_move.c \
	import.c \
	import2.c \
	clear.c \
	init.c \
	element2.c \
	grau_init.c \
	generic_init.c \
	api.c

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
PROG_LIBS = $(STATIC_OPT) -L ../lib/$(OBJ_DIR) -lrobots \
	$(DYNAMIC_OPT) -L $(DEPTH)/lib/$(OBJ_DIR) -lsamfs -lsamcat -lsamut \
	$(LIBSO) $(THRLIBS) -l gen -ldl

LNOPTS = -a -Dlint
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR)

ifneq ($(SAM_OPEN_SOURCE), yes)
ifeq ($(PLATFORM), sparc)
MPROGS = grau_helper
grau_helper_SRC = grau_helper.c


grau_helper_LIBS = $(PROG_LIBS) -L $(OBJ_DIR) -laci

ACILIB = libaci.so

.INIT:
	@-rm -f $(OBJ_DIR)/$(ACILIB)
	@ln -s $(DEPTH)/$(OBJDEPTH)/lib/$(OBJ_DIR)/$(ACILIB) `/bin/pwd`/$(OBJ_DIR)/$(ACILIB)

endif
endif

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
