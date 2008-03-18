# $Revision: 1.14 $

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

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

ifeq ($(OS), SunOS)
#
# Build kernel symbol table entries for mdb's use.  This should
# only be defined in GNUmakefiles that define MODULE_SRC.
#
BUILD_STABS = yes
include ./Build.mk
else
ifeq ($(KERNEL_MAJOR),4)
include ./Build.mk
else

# linux 2.6 kernel build
all:
	@DEBUG=$(DEBUG) $(MAKE) -f Makefile.kbuild
	cp $(OBJ_DIR)/SUNWqfs.ko  $(OBJ_DIR)/SUNWqfs.ko.tmp
	./squish.pl $(OBJ_DIR)/SUNWqfs.ko.tmp $(OBJ_DIR)/SUNWqfs.ko $(OBJ_DIR)


clean:
	@DEBUG=$(DEBUG) $(MAKE) -f Makefile.kbuild clean

clobber:
	@DEBUG=$(DEBUG) $(MAKE) -f Makefile.kbuild clean

clobberall:
	@DEBUG=$(DEBUG) $(MAKE) -f Makefile.kbuild clean

endif

endif
