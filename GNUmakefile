# $Revision: 1.23 $

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

DEPTH = .

include $(DEPTH)/mk/common.mk

BUILD_64BIT ?= yes

#   all is defined here so it is the default.
.PHONY: all config set_config
ifneq ($(SPARCV9), yes)
ifneq ($(AMD64), yes)
all:    info
endif
endif

ifeq ($(MAKECMDGOALS), clobberall)
DIRS = include src pkg pkg-linux
else
ifeq ($(OS), SunOS)
DIRS = include src pkg
else
DIRS = include src pkg-linux
endif
endif

#   CSCOPE defines
CSDIR=.
CSDIRS=include src mk
CSPATHS=$(CSDIRS:%=$(CSDIR)/%)
CSINCS=$(CSPATHS:%=-I%)
CSFLAGS=-b

include $(DEPTH)/mk/targets.mk

#   CVS checkout at a tag removes the lib directory.  Make a new one.
.INIT:
	@if [ ! -d lib ] ; then /bin/mkdir lib; fi
	@/bin/date "+Version $(SAMQFS_VERSION) (%d/%m/20%y)" > \
		src/samqfsui/html/fr/versionDate.txt
	@/bin/date "+Version $(SAMQFS_VERSION) (%m/%d/20%y)" > \
		src/samqfsui/html/en/versionDate.txt


#   Set up the default configuration.
#
#   Note that this target needs to be defined prior to including
#   common.mk to avoid picking up the existing definitions in
#   CONFIG.mk.

config: set_config

set_config:
	$(MAKE) -C mk $(MAKECMDGOALS)
	$(MAKE) info

clean:	clean_lib
clean_lib:
	rm -rf lib/$(OBJ_DIR)

clobberall:	clobber_lib cscope.clean copyright.clean cstyle.clean
clobber_lib:
	rm -rf lib/obj
	rm -f $(DEPTH)/MARKER
	$(MAKE) -C mk $(MAKECMDGOALS)

cscope: cscope.out

cscope.out: cscope.files
ifeq ($(OS), SunOS)
	@-rm -f cscope.out
	$(CSCOPE) $(CSFLAGS)
	@echo "=============================================="
else
	$(error "Target 'cscope.out' is not supported on Linux.")
endif

cscope.files:
ifeq ($(OS), SunOS)
	@-rm -f cscope.files
	@echo "=============================================="
	echo $(CSINCS) > cscope.files
	-find $(CSPATHS) -name CVS -prune -o -type f \( -name '*.[Ccshlxy]' -o -name GNUmakefile -o -name '*.mk' -o -name '*.hs' -o -name '*.kbuild' \) -print | grep -v "\/obj\/" >> cscope.files
else
	$(error "Target 'cscope.files' is not supported on Linux.")
endif

cscope.clean:
	-rm -f cscope.files cscope.out

check: cstyle copyright

cstyle:
ifeq ($(OS), SunOS)
	@-rm -f cstyle.out cstyle.files cstyle.temp
	@echo "=============================================="
	@echo "$(CSTYLETREE) > cstyle.files"
	@$(CSTYLETREE)
	@echo "cstyle warning count: \c"
	@/bin/wc -l cstyle.files
	@/bin/cat cstyle.files
	@echo "=============================================="
else
	$(error "Target 'cstyle' is not supported on Linux.")
endif

cstyle.clean:
	-rm -f cstyle.out cstyle.files cstyle.temp

copyright:
ifeq ($(OS), SunOS)
	@-rm -f copyright.files
	@echo "=============================================="
	$(COPYRIGHT) > copyright.files
	@echo "copyright warning count: \c"
	@/bin/egrep -v '^\*' copyright.files | /bin/wc -l
	@/bin/cat copyright.files
	@echo "=============================================="
else
	$(error "Target 'copyright' is not supported on Linux.")
endif

copyright.clean:
	-rm -f copyright.files

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
.NOTPARALLEL:

include $(DEPTH)/mk/depend.mk
