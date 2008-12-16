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

#	targets.mk - target definition
#
#	The following variables can be defined in a Makefile:
#
#		MLIBS		  	program library name list 
#		<libname>_SRC	program library source list
#		<libname>_LIB	program library list for linking 
#
#	multilib.mk must be included after targets.mk


###############################################################################
#	Handle the MLIBS variable. MLIBS designates a list of libraries that
#	are compiled from the sources in <libname>_SRC.

STRIP_LIBS = $(strip $(MLIBS))
.PHONY: $(STRIP_LIBS)

CFLAGS += $(SHARED_CFLAGS) 

#
# Generate both static (.a) and shared (.so) formats
#
ALL_LIBS = $(addsuffix .a, $(STRIP_LIBS)) $(addsuffix .so, $(STRIP_LIBS))
$(STRIP_LIBS):  $(addprefix $(OBJ_DIR)/lib, $(ALL_LIBS))

define LIBS_template
ifneq ($$(STRIP_LIBS), )

LIBS_SRC += $$($(1)_SRC)

$(1)_OBJS := $$($(1)_SRC:%.c=$(OBJ_DIR)/%.o)

$(OBJ_DIR)/lib$(1).a:   $$($(1)_OBJS)
	rm -f $$@
	ar ruc $$@ $$^
	@if [ ! -z "$(LIB_HOME)" ]; then \
		echo rm -f $(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).a; \
		rm -f $(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).a; \
		echo ln -s $(LIB_HOME)/$$(OBJ_DIR)/lib$(1).a $(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).a; \
		ln -s $(LIB_HOME)/$$(OBJ_DIR)/lib$(1).a $(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).a; \
	fi

$(OBJ_DIR)/lib$(1).so:  $$($(1)_OBJS)
	rm -f $$@
	$(CC) $(SHARED_CFLAGS) -o $$@ $$(LDFLAGS) $$^ $($(1)_LIBS)
	@if [ ! -z "$(LIB_HOME)" ]; then \
		echo rm -f $$(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).so; \
		rm -f $$(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).so; \
		echo ln -s $$(LIB_HOME)/$$(OBJ_DIR)/lib$(1).so $$(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).so; \
		ln -s $$(LIB_HOME)/$$(OBJ_DIR)/lib$(1).so $$(DEPTH)/lib/$$(OBJ_DIR)/lib$(1).so; \
	fi

endif
endef

$(foreach lib,$(STRIP_LIBS),$(eval $(call LIBS_template,$(lib))))


###############################################################################

all:	$(STRIP_LIBS)
