# $Revision: 1.33 $

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

#	common.mk - common definitions
#
#	common.mk should be the first inclusion in all Makefiles.

# Default number of parallel jobs allowed
NJOBS = 4

.PHONY:	all

# Default target
all:

ifndef	DEPTH
$(error "DEPTH variable is undefined")
endif

ifneq ($(HAVE_internal_mk),yes)
-include $(DEPTH)/mk/include/internal.mk
endif
ifneq ($(HAVE_internal_mk),yes)
SAM_OPEN_SOURCE=yes
endif
include $(DEPTH)/mk/include/defaults.mk

SAMQFS_VERSION ?= $(shell git describe --tags --abbrev=0)
GUI_VERSION := 5.0.1

SHELL := /bin/sh
OS := $(shell uname -s)
HOSTNAME := $(shell hostname)

SAMFS_TOOLS = /opt

#
#     Unset LD_LIBRARY_PATH.
#
LD_LIBRARY_PATH =

#
#	Include OS, Revision, and architecture dependent definitions
#	for SunOS and Linux.
#
ifeq ($(OS), SunOS)
	include $(DEPTH)/mk/include/sun.mk
else
ifeq ($(OS), Linux)
	include $(DEPTH)/mk/include/linux.mk
else
$(error "Don't know anything about OS $(OS)")
endif
endif

WHOAMI = $(shell $(CMDWHOAMI))

# Installation directories.
BASEDIR := /
USRDEST := $(BASEDIR)/opt/SUNWsamfs/bin
ADMDEST := $(BASEDIR)/opt/SUNWsamfs/sbin
INCDEST := $(BASEDIR)/opt/SUNWsamfs/include
DOCDEST := $(BASEDIR)/opt/SUNWsamfs/doc
EXADEST := $(BASEDIR)/opt/SUNWsamfs/examples
MANDEST := $(BASEDIR)/opt/SUNWsamfs/man
LIBDEST := $(BASEDIR)/opt/SUNWsamfs/lib
RPCDEST := $(BASEDIR)opt/SUNWsamfs/client
TOOLDEST := $(BASEDIR)/opt/SUNWsamfs/tools
UTILDEST := $(BASEDIR)/opt/SUNWsamfs/util
JREDEST := $(BASEDIR)/opt/SUNWsamfs/jre
STKDEST := $(BASEDIR)/opt/SUNWsamfs/stk
FSDEST  := $(BASEDIR)/kernel/fs/$(64DIR)
SYSDEST  := $(BASEDIR)/kernel/sys/$(64DIR)
DRVDEST := $(BASEDIR)/kernel/drv/$(64DIR)
JLIBDEST := $(BASEDIR)/opt/SUNWsamfs/lib/java
ECTDEST := $(BASEDIR)/etc/opt/SUNWsamfs
VARDEST := $(BASEDIR)/var/opt/SUNWsamfs


OWNER := $(WHOAMI)
GROUP := staff
SYSMODE := 755
USERMODE := 755
DATAMODE := 644

SYSINST := -m $(SYSMODE) -g $(GROUP) -o $(OWNER)
USERINST := -m $(USERMODE) -g $(GROUP) -o $(OWNER)
DATAINST := -m $(DATAMODE) -g $(GROUP) -o $(OWNER)
MSGINST := -m 444 -g $(GROUP) -o $(OWNER)

#
# Default VPATH
#
VPATH = .

#
#	Destination for generated output
#
OBJ_BASE := obj
ifneq	($(DEBUG), yes)
	OBJ_DIR := $(OBJ_BASE)/$(OS_ARCH)
else
	OBJ_DIR := $(OBJ_BASE)/$(OS_ARCH)_DEBUG
endif

#
# Set additional debug options if needed
#
# "DEBUG = yes | no" may be specified globally
#
# "DEBUG_OFF = yes" may be specified in a make file to
# suppress debug mode for that make file only.
#
ifeq ($(DEBUG), yes)
	ifneq ($(DEBUG_OFF), yes)
		DEBUGCDEFS += -DDEBUG
	endif
endif

DEPCFLAGS += $(DEBUGCDEFS)

#
# Build with SAM_TRACE enabled
#
DEPCFLAGS += -DSAM_TRACE

ifeq ($(SAM_OPEN_SOURCE),yes)
#
# Build with SAM_OPEN_SOURCE
#
	DEPCFLAGS += -DSAM_OPEN_SOURCE
endif

#
# make depend output file
#
DEPFILE = $(OBJ_DIR)/.depend

#
# CFLAGS defines when using threads
#
THRCOMP = -D_REENTRANT

#
# Default sam-qfs include files
#
INCLUDE = $(DEPTH)/include

#
# Final default CFLAGS settings
#
CFLAGS = $(DEPCFLAGS) $(DEBUGCFLAGS) $(PLATFLAGS) $(CERRWARN)
LDFLAGS = $(PLATFLAGS) -B direct -z ignore
ifeq ($(SPARCV9), yes)
LIBSO = $(LIBSO_OPT)/opt/SUNWsamfs/lib/sparcv9:/opt/SUNWsamfs/lib
else
ifeq ($(AMD64), yes)
LIBSO = $(LIBSO_OPT)/opt/SUNWsamfs/lib/amd64
else
LIBSO = $(LIBSO_OPT)/opt/SUNWsamfs/lib
endif
endif

#
# default lint options
#
LNOPTS = -u -x -Dlint -Nlevel=2
#
CMDS_LFLAGS32 = -Dlint
CMDS_LFLAGS32 += -Xa
CMDS_LFLAGS32 += -nsxmuF
CMDS_LFLAGS32 += -errtags=yes
CMDS_LFLAGS32 += -erroff=E_BAD_PTR_CAST_ALIGN
CMDS_LFLAGS32 += -erroff=E_SUPPRESSION_DIRECTIVE_UNUSED
CMDS_LFLAGS32 += -erroff=E_SUSPICIOUS_COMPARISON
CMDS_LFLAGS32 += -erroff=E_CAST_UINT_TO_SIGNED_INT
CMDS_LFLAGS32 += -erroff=E_PASS_UINT_TO_SIGNED_INT
CMDS_LFLAGS32 += -erroff=E_INVALID_ANNOTATION_NAME
CMDS_LFLAGS32 += -erroff=E_OLD_STYLE_DECL_OR_BAD_TYPE
CMDS_LFLAGS32 += -erroff=E_NO_EXPLICIT_TYPE_GIVEN
CMDS_LFLAGS32 += -erroff=E_STATIC_UNUSED
CMDS_LFLAGS32 += -erroff=E_PTRDIFF_OVERFLOW
CMDS_LFLAGS32 += -erroff=E_ASSIGN_NARROW_CONV
#
CMDS_LFLAGS64 = -Dlint
CMDS_LFLAGS64 += -Xa
CMDS_LFLAGS64 += -nsxmuF
CMDS_LFLAGS64 += -errtags=yes
CMDS_LFLAGS64 += -m64
CMDS_LFLAGS64 += -erroff=E_BAD_PTR_CAST_ALIGN
CMDS_LFLAGS64 += -erroff=E_SUPPRESSION_DIRECTIVE_UNUSED
CMDS_LFLAGS64 += -erroff=E_SUSPICIOUS_COMPARISON
CMDS_LFLAGS64 += -erroff=E_CAST_UINT_TO_SIGNED_INT
CMDS_LFLAGS64 += -erroff=E_PASS_UINT_TO_SIGNED_INT
CMDS_LFLAGS64 += -erroff=E_INVALID_ANNOTATION_NAME
CMDS_LFLAGS64 += -erroff=E_OLD_STYLE_DECL_OR_BAD_TYPE
CMDS_LFLAGS64 += -erroff=E_NO_EXPLICIT_TYPE_GIVEN
CMDS_LFLAGS64 += -erroff=E_STATIC_UNUSED
CMDS_LFLAGS64 += -erroff=E_PTRDIFF_OVERFLOW
CMDS_LFLAGS64 += -erroff=E_ASSIGN_NARROW_CONV
CMDS_LFLAGS64 += -erroff=E_CAST_INT_TO_SMALL_INT
CMDS_LFLAGS64 += -erroff=E_CAST_INT_CONST_TO_SMALL_INT
CMDS_LFLAGS64 += -erroff=E_CAST_TO_PTR_FROM_INT
CMDS_LFLAGS64 += -erroff=E_ASSIGN_INT_TO_SMALL_INT
CMDS_LFLAGS64 += -erroff=E_ASSIGN_INT_FROM_BIG_CONST
CMDS_LFLAGS64 += -erroff=E_CONST_PROMOTED_UNSIGNED_LL
CMDS_LFLAGS64 += -erroff=E_CONST_PROMOTED_LONG_LONG
CMDS_LFLAGS64 += -erroff=E_CONST_TRUNCATED_BY_ASSIGN
CMDS_LFLAGS64 += -erroff=E_PASS_INT_FROM_BIG_CONST
CMDS_LFLAGS64 += -erroff=E_COMP_INT_WITH_LARGE_INT
CMDS_LFLAGS64 += -erroff=E_ASSIGN_UINT_TO_SIGNED_INT
CMDS_LFLAGS64 += -erroff=E_PASS_INT_TO_SMALL_INT
CMDS_LFLAGS64 += -erroff=E_PTR_CONV_LOSES_BITS
#
LIBS_LDEF32 = -errtags=yes 
LIBS_LDEF32 += -s 
LIBS_LDEF32 += -erroff=E_PTRDIFF_OVERFLOW
LIBS_LDEF32 += -erroff=E_ASSIGN_NARROW_CONV

LIBS_LFLAGS32 = -Dlint
LIBS_LFLAGS32 += -uaxs
LIBS_LFLAGS32 += -D_TS_ERRNO
LIBS_LFLAGS32 += $(LIBS_LDEF32)
#
LIBS_LFLAGS64 = -Dlint
LIBS_LFLAGS64 += -Xa
LIBS_LFLAGS64 += -nsxmuF
LIBS_LFLAGS64 += -errtags=yes
LIBS_LFLAGS64 += -Xarch=v9
LIBS_LFLAGS64 += $(LIBS_LDEF32)

