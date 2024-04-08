#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at pkg/OPENSOLARIS.LICENSE
# or https://illumos.org/license/CDDL.
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

# $Revision: 1.12 $

DEPTH = ../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

_dummy = $(shell [ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR))

INCS =  catalog.h devstat.h lib.h mig.h rminfo.h    \
	san.h sam_errno.h samapi.h samrpc.h stat.h version.h

HOST = $(shell hostname)
DATETIME = $(shell date '+%Y-%m-%d %H:%M:%S')
X_UNAME = $(shell uname -a )
UNAME = $(shell uname -a | sed -e's:/:-:g' )

include $(DEPTH)/mk/targets.mk

.INIT: $(OBJ_DIR)/pub/version.h

$(OBJ_DIR)/pub/version.h: $(DEPTH)/MARKER version.hs
	[ -d $(OBJ_DIR)/pub ] || mkdir -p $(OBJ_DIR)/pub
	/bin/rm -f $(OBJ_DIR)/pub/version.h ;\
	sed -e 's/%samqfs_version%/$(SAMQFS_VERSION)/' \
		-e 's/%level%/$(MARKER)/' -e 's/%host%/$(HOST)/' \
		-e 's/%datetime%/$(DATETIME)/' -e 's/%uname%/$(UNAME)/' \
		version.hs >$(OBJ_DIR)/pub/version.h ;\
	chmod -w $(OBJ_DIR)/pub/version.h

clean:

install:	install_more
install_more:
	@ for f in $(INCS); do                      \
		echo $$f;                               \
		$(INSTALL) $(DATAINST) $$f $(INCDEST);  \
	done
	$(INSTALL) $(DATAINST) samrpc.h $(RPCDEST)/include

include $(DEPTH)/mk/depend.mk
