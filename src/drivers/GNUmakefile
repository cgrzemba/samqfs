# $Revision: 1.16 $

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

DEPTH = ../..

# Suppress debug mode if it is enabled. Comment out the line to
# re-enable.
DEBUG_OFF = yes

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

#
# Build kernel symbol table entries for mdb's use.  This should
# only be defined in GNUmakefiles that define MODULE_SRC.
#
BUILD_STABS = yes

MODULE = samst
MODULE_SRC = samst.c

DATAFILES = samst.conf

DIRS =	samaio devfsadm

DEPCFLAGS += -D_KERNEL -DNOSAMFS

CFLAGS += $(KERNFLAGS)

LDFLAGS = -r

LNFLAGS = -I$(INCLUDE) -D_KERNEL -DNOSAMFS $(VERS)
LNOPTS += -n
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
