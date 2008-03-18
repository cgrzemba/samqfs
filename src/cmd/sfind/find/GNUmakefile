# $Revision: 1.10 $

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

# GPL Notice
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; version 2.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software Foundation,
#      Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#      For the avoidance of doubt, except that if any license choice other
#      than GPL or LGPL is available it will apply instead, Sun elects to
#      use only the General Public License version 2 (GPLv2) at this time
#      for any software where a choice of GPL license versions is made
#      available with the language indicating that GPLv2 or any later
#      version may be used, or where a choice of which version of the GPL
#      is applied is otherwise unspecified.

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

PROG = find
PROG_SRC = \
	find.c \
	fstype.c \
	parser.c \
	pred.c \
	tree.c \
	util.c \
	version.c

DEFS =  -DHAVE_STRING_H -DDIRENT \
	-DDIRENT=1 -DFSTYPE_STATVFS=1 -DSTDC_HEADERS=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DHAVE_LIMITS_H=1 -DHAVE_STRERROR=1 \
	-DHAVE_VPRINTF=1 -DHAVE_ALLOCA_H=1 -DHAVE_TZNAME=1
ifeq ($(OS), Linux)
DEFS +=  -DMAJOR_IN_SYSMACROS=1 -I$(LINUX_INCLUDE) $(OSFLAGS)
else
DEFS +=  -DMAJOR_IN_MKDEV=1
endif

DEPCFLAGS = $(DEFS) -I../lib -I$(TARG_INC)$(DEPTH)/include
PROG_LIBS = ../lib/$(OBJ_DIR)/libfind.a -L $(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut -lsamconf $(LIBSO)
ifeq ($(OS), SunOS)
PROG_LIBS += -lgen -lintl
endif

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
