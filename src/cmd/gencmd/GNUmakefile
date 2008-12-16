# $Revision: 1.12 $

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

include $(DEPTH)/mk/common.mk

PROG = gencmd
PROG_SRC = gencmd.c

USRLINKS = release stage ssum setfa
ADMLINKS = unarchive damage undamage rearch unrearch

DEPCFLAGS += $(CHECKSUM)
PROG_LIBS = -L$(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut -lsamconf $(LIBSO)

ifeq ($(OS), SunOS)
PROG_LIBS += -lgen -lintl
endif

LNOPTS = $(CMDS_LFLAGS32)
LNLIBS = -L$(DEPTH)/lib/$(OBJ_DIR) -lsam -lsamut -lsamconf

include $(DEPTH)/mk/targets.mk

install:	all
	$(INSTALL) $(SYSINST) gencmd $(USRDEST)/archive
	for f in $(USRLINKS); do                        \
		rm -f $(USRDEST)/$$f;                       \
		ln -s $(USRDEST)/archive $(USRDEST)/$$f;    \
	done
	for f in $(ADMLINKS); do                        \
		rm -f $(ADMDEST)/$$f;                       \
		ln -s $(USRDEST)/archive $(ADMDEST)/$$f;    \
	done

include $(DEPTH)/mk/depend.mk
