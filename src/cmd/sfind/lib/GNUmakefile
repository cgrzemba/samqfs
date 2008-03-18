# $Revision: 1.13 $

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

LIB = find
LIB_SRC = \
	dirname.c \
	error.c \
	filemode.c \
	getopt1.c \
	idcache.c \
	listfile.c \
	modechange.c \
	nextelem.c \
	savedir.c \
	xmalloc.c \
	xstrdup.c \
	fnmatch.c \
	fileblocks.c \
	strdup.c \
	strftime.c \
	strtol.c

ifeq ($(OS), SunOS)
LIB_SRC += \
	getopt.c \
	regex.c \
	stpcpy.c \
	strstr.c \
	alloca.c
endif

DEFS = \
	-DDIRENT=1 -DFSTYPE_STATVFS=1 -DSTDC_HEADERS=1 \
	-DHAVE_UNISTD_H=1 -DHAVE_STRING_H=1 -DHAVE_LIMITS_H=1 -DHAVE_STRERROR=1 \
	-DHAVE_VPRINTF=1 -DHAVE_ALLOCA_H=1 -DHAVE_TZNAME=1
ifeq ($(OS), Linux)
DEFS +=  -DMAJOR_IN_SYSMACROS=1 -I$(LINUX_INCLUDE) $(OSFLAGS)
else
DEFS +=  -DMAJOR_IN_MKDEV=1
endif

DEPCFLAGS = $(DEFS) -I. -I$(TARG_INC)$(DEPTH)/include

LNOPTS = -u -x -Dlint
LNFLAGS = $(DEFS)  -I. -I$(TARG_INC)$(DEPTH)/include

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
