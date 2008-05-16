# $Revision: 1.15 $

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

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

PROG = lockfs
PROG_SRC = lockfs.c

INCFLAGS = -I../../include -I../../include/$(OBJ_DIR) -I$(INCLUDE)
DEPCFLAGS = $(INCFLAGS) $(VERS) $(METADATA_SERVER) $(DEBUGCDEFS)

LIBSO = $(LIBSO_OPT) /usr/lib/fs/samfs
PROG_LIBS = $(STATIC_OPT) -L ../../lib/$(OBJ_DIR) -lfscmd \
	-L $(DEPTH)/lib/$(OBJ_DIR) -lsamut $(DYNAMIC_OPT) -lsamconf $(LIBSO)

DESTDIR = /usr/lib/fs/samfs
LINKS = samlockfs

LNOPTS = -amnuxs -Dlint
LNLIBS = -L ../../lib/$(OBJ_DIR) -lfscmd \
		-L $(DEPTH)/lib/$(OBJ_DIR) -lsamut -lsamconf

include $(DEPTH)/mk/targets.mk

install:	softlink

softlink:	$(OBJ_DIR)/$(PROG)
	@for f in $(LINKS); do \
		echo rm -f $(ADMDEST)/$$f; \
		echo ln -s $(DESTDIR)/$(PROG) $(ADMDEST)/$$f; \
		rm -f $(ADMDEST)/$$f; \
		ln -s $(DESTDIR)/$(PROG) $(ADMDEST)/$$f; \
	done

include $(DEPTH)/mk/depend.mk
