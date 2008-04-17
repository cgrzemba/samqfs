# $Revision: 1.15 $

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

PROG = mount
PROG_SRC = mount.c

ifeq ($(OS), Linux)
INCFLAGS = $(OSFLAGS) -I$(INCLUDE)/sam/$(OBJ_DIR) -I$(LINUX_INCLUDE) -I../../include -I../../include/$(OBJ_DIR) -I$(INCLUDE)
SCRIPT = samfs.shared.linux
LADM = 
else
INCFLAGS = -I$(INCLUDE)/sam/$(OBJ_DIR) -I../../include -I$(INCLUDE)
SCRIPT = samfs.shared
LADM = -ladm
endif
DEPCFLAGS = $(INCFLAGS) $(VERS) $(METADATA_SERVER) $(DEBUGCDEFS)

LDFLAGS += ../../lib/$(OBJ_DIR)/setsyscall.o
PROG_LIBS = $(STATIC_OPT) -L ../../lib/$(OBJ_DIR) -lfscmd \
	-L $(DEPTH)/lib/$(OBJ_DIR) -lsamut -lsamconf $(DYNAMIC_OPT) \
	$(LADM) -ldl

ifeq ($(OS), SunOS)
PROG_LIBS += -lsysevent -lnvpair
endif

LNOPTS = -mnuxs -Dlint
LNLIBS = -L ../../lib/$(OBJ_DIR) -lfscmd \
		-L $(DEPTH)/lib/$(OBJ_DIR) -lsamut -lsamconf

include $(DEPTH)/mk/targets.mk

install:	install_scripts

install_scripts:
	$(INSTALL) $(SYSINST) samd $(ADMDEST)
	$(INSTALL) $(SYSINST) $(SCRIPT) /etc/init.d/$(SCRIPT)

include $(DEPTH)/mk/depend.mk
