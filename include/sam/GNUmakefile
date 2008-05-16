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

# $Revision: 1.13 $

DEPTH = ../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

#
# Need an obj directory for platform specific destination
#
__dummy := $(shell [ -d $(OBJ_DIR)/sam ] || mkdir -p $(OBJ_DIR)/sam )

DIRS = fs

include $(DEPTH)/mk/targets.mk

.INIT: $(OBJ_DIR)/sam/defaults.hc $(OBJ_DIR)/sam/dpparams.hc $(OBJ_DIR)/sam/mount.hc

clean:		clean_more
clean_more:
	@/bin/rm -f *.hc

$(OBJ_DIR)/sam/defaults.hc: $(DEPTH)/include/sam/mkhc.awk defaults.cfg
	$(AWK) -f $(DEPTH)/include/sam/mkhc.awk defaults.cfg $@

$(OBJ_DIR)/sam/dpparams.hc: $(DEPTH)/include/sam/mkhc.awk dpparams.cfg
	$(AWK) -f $(DEPTH)/include/sam/mkhc.awk dpparams.cfg $@

$(OBJ_DIR)/sam/mount.hc: $(DEPTH)/include/sam/mkhc.awk mount.cfg
	$(AWK) -v setfield_const="SETFIELD_DEFS_`uname -s`" -f mkhc.awk mount.cfg $@

include $(DEPTH)/mk/depend.mk
