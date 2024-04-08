# $Revision: 1.11 $

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

DEPTH = ../../..

include $(DEPTH)/mk/common.mk

BUILD_64BIT = yes

#
# Need an obj directory for trace.h destination
#
__dummy := $(shell [ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR))

.INIT: $(OBJ_DIR)/trace.h

$(OBJ_DIR)/trace.h: tracemsgs.hs trace.hs
	rm -f $(OBJ_DIR)/z1 $(OBJ_DIR)/z2 $(OBJ_DIR)/z3 $(OBJ_DIR)/trace.h
	sed -e '/^#/d' tracemsgs.hs >$(OBJ_DIR)/z1
	# Separate tracemsgs.hs columns into 2 files.
	# Change %d and %x to use #defined strings to handle 32-bit/64-bit.
	cut <$(OBJ_DIR)/z1 -f1 | sed 's/$$/,/' >$(OBJ_DIR)/z2
	cut <$(OBJ_DIR)/z1 -f2 -d'"' | \
	sed -e 's/^/"/' -e 's/$$/",/' \
		-e 's/%d/"SAMTRACED"/g' -e 's/%x/"SAMTRACEX"/g' \
		>$(OBJ_DIR)/z3
	# Insert the files into the enums and messages array.
	sed -e '/T_NULL/r $(OBJ_DIR)/z2' -e '/"No message"/r $(OBJ_DIR)/z3' trace.hs >$(OBJ_DIR)/trace.h
	rm -f $(OBJ_DIR)/z1 $(OBJ_DIR)/z2 $(OBJ_DIR)/z3
	chmod ugo-w $(OBJ_DIR)/trace.h

include $(DEPTH)/mk/targets.mk

include $(DEPTH)/mk/depend.mk
