# $Revision: 1.9 $

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

#	multiprog.mk - target definition
#
#	The following variables can be defined in a Makefile:
#
#		MPROGS		executable program (can be a list)
#		<progname>_SRC	program source list
#		<progname>_LIB	program library list for linking
#
#	multiprog.mk must be included after targets.mk

###############################################################################
#	Handle the MPROGS variable. MPROGS designates a list of executables that
#	are compiled from the sources in <progname>_SRC.

STRIP_PROGS = $(strip $(MPROGS))
.PHONY: $(STRIP_PROGS)

define PROGS_template
ifneq ($$(STRIP_PROGS),)
$(1)_OBJS = $$($(1)_SRC:%.c=$(OBJ_DIR)/%.o)

PROGS_SRC += $$($(1)_SRC)

$(OBJ_DIR)/$(1):		$$($(1)_OBJS)
	$(CC) $$(CFLAGS) -o $$@ $$(LDFLAGS) $$($(1)_OBJS) $$($(1)_LIBS)

endif
endef

$(foreach prog,$(STRIP_PROGS),$(eval $(call PROGS_template,$(prog))))

$(STRIP_PROGS):		$(addprefix $(OBJ_DIR)/, $(MPROGS)) 


###############################################################################

all:	$(STRIP_PROGS)
