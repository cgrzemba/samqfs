# $Revision: 1.22 $

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

include $(DEPTH)/mk/common.mk

DIRS = \
	sam \
	samconf \
	samspm \
	samut

ifeq ($(OS), SunOS)
BDB = bdb
SAMCAT = samcat
SAMRFT = samrft
DIRS += \
	bdb \
	fsmdb \
	mysql \
	samrft \
	samcat \
	samfs \
	samapi \
	samdb \
	sammig \
	ssl \
	curl \
	csn \
	sammgmt \
	sammgmtjni \
	pax_hdr
endif

include $(DEPTH)/mk/targets.mk

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
sam:		$(SAMCAT) samconf samspm samut
samspm:		samconf
samfs:		sam samut $(SAMCAT)
sammgmt:	sam samut $(SAMCAT) samfs samapi fsmdb csn
samrft:		samspm
samut:		samconf $(SAMRFT) $(BDB)
samcat samapi:	samut
sammgmt/stkapi:	sammgmt
sammgmtjni:	sammgmt
csn:		samut curl ssl
pax_hdr:	samut
samdb:		samut mysql

include $(DEPTH)/mk/depend.mk
