# $Revision: 1.6 $

#    SAM-QFS_notice_begin
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
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

#
# define source list for 'make depend'
#
ifeq ($(DEPEND_SRC), )
ifeq ($(strip $(SRC_VPATH)), )
	X_DEP_SRC = $(PROG_SRC) $(LIB_SRC) $(MODULE_SRC) $(PROGS_SRC) $(LIBS_SRC)
else
	#
	# if we're using vpath, we need to figure out where all
	# the source actually lives.
	#
	X_DEP_SRC = $(foreach file, $(PROG_SRC) $(LIB_SRC) $(MODULE_SRC) $(PROGS_SRC) $(LIBS_SRC), \
					$(foreach dir, . $(SRC_VPATH), \
						$(wildcard $(dir)/$(file))))
endif
DEPEND_SRC = $(strip $(X_DEP_SRC))
endif

depend:	.INIT $(STRIP_DIRS)

ifneq ($(DEPEND_SRC), )
depend: 	$(DEPFILE) 

$(DEPFILE): 	GNUmakefile $(DEPEND_SRC) 
		@touch $(DEPFILE); \
		echo $(MAKEDEPEND) -f $(DEPFILE) -p$(OBJ_DIR)/ -- ${DEPCFLAGS} -- $(DEPEND_SRC); \
		$(MAKEDEPEND) -f $(DEPFILE) -p$(OBJ_DIR)/ -- -DMAKEDEPEND ${DEPCFLAGS} -- $(DEPEND_SRC)

X_DEPFILE_EXISTS = [ ! -f $(DEPFILE) ] || echo yes
DEPFILE_EXISTS = $(shell $(X_DEPFILE_EXISTS))
ifeq ($(DEPFILE_EXISTS), yes)
include $(DEPFILE)
endif
else
depend: $(STRIP_DIRS)
endif
