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

# $Revision: 1.14 $

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

SRCDIR = $(DEPTH)/src/fs/include

INCS = acl.h amld.h arfind.h bswap.h debug.h dirent.h ino.h ino_ext.h inode.h \
	ioblk.h macros.h quota.h rmedia.h samhost.h sblk.h sblk_mgmt.h scd.h \
	share.h validation.h

#
# Add any generated .h's here.  These get placed in an OBJ_DIR as
# they could differ based on the platform they were generated on.
#
PLATFORM_INCS = trace.h
PLATFORM_DEPTH = ../../../..
PLATFORM_DIR = $(OBJ_DIR)/sam/fs

#
# Need an obj directory for platform specific destination
#
__dummy := $(shell [ -d $(PLATFORM_DIR) ] || mkdir -p $(PLATFORM_DIR))


ifeq ($(OS), Linux)
INCS += macros_linux.h
else
INCS += macros_solaris.h
endif

include $(DEPTH)/mk/targets.mk


.INIT:
	-for f in $(INCS) ; do \
		if [ ! -f $$f ]; then \
			ln -s $(SRCDIR)/$$f $$f 2>/dev/null; \
		fi \
	done
	-for f in $(PLATFORM_INCS) ; do \
		ln -fs $(PLATFORM_DEPTH)/$(SRCDIR)/$(OBJ_DIR)/$$f $(PLATFORM_DIR); \
	done

clobberall:  remove_links
remove_links:
	-rm -f $(INCS)

include $(DEPTH)/mk/depend.mk
