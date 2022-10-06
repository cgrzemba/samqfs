# $Revision: 1.13 $

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

DEPTH = ../../../..

# devfsadm is a 32bit command!
override AMD64 = no
override BUILD_64BIT = no
include $(DEPTH)/mk/common.mk

LIB = samaio_link
LIB_SRC = \
		samaio_link.c

CFLAGS = -I. -I.. -I$(INCLUDE) -DnoDEBUG -DNOSAMFS $(VERS) $(PLATFLAGS) $(CERRWARN)

LNFLAGS = -I. -I.. -I$(INCLUDE) -DnoDEBUG -DNOSAMFS $(VERS)
LNOPTS = -a -u -m -x -Dlint
LNLIBS =

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
