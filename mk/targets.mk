# $Revision: 1.19 $

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

#	targets.mk - target definition
#
#	The following variables can be defined in a Makefile:
#
#		DIRS		sub directories (can be a list)
#		PROG		executable program
#		PROG_SRC	program source list
#		PROG_LIBS	program library list (for linking)
#		LIB			library
#		LIB_SRC		library source list
#		LIB_LIBS	library list for linking
#		MODULE		kernel module
#		MODULE_SRC	kernel module source
#		MODULE_LIBS	kernel module library list (for linking)
#
#	The following targets and dependencies are defined:
#
#		all			$(DIRS) $(PROG) $(LIB) $(MODULE)
#		clean		$(DIRS)
#		clobber		$(DIRS)
#		clobberall	$(DIRS)
#		install
#		depend
#		lint		$(DIRS)
#
#	targets.mk should be included after all definitions and includes

.PRECIOUS:  .o %.o $(OBJ_DIR)/%.o

.PHONY:	all clean clobber clobberall install lint info depend .INIT

###############################################################################
# Make sure OBJ_DIRs exists when required

X_CHECK_OBJDIR = \
	if [ ! -z "$(LIB)" -o ! -z "$(MLIBS)" -o ! -z "$(PROG)" \
		  -o ! -z "$(MPROGS)" -o ! -z "$(MODULE)" ]; then \
			[ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR); \
	fi
__dummy := $(shell $(X_CHECK_OBJDIR))

X_CHECK_LIBDIR = \
	if [ ! -z "$(LIB_HOME)" -o ! -z "$(JAR_HOME)" ]; then \
			[ -d $(DEPTH)/lib/$(OBJ_DIR) ] || mkdir -p $(DEPTH)/lib/$(OBJ_DIR); \
	fi
__dummy := $(shell $(X_CHECK_LIBDIR))

#X_CHECK_JARDIR = \
#	if [ ! -z "$(JAR_HOME)" ]; then \
#			[ -d $(DEPTH)/jar/$(OBJ_DIR) ] || mkdir -p $(DEPTH)/jar/$(OBJ_DIR); \
#	fi
#__dummy := $(shell $(X_CHECK_JARDIR))

#
# Are we building multiple libraries from a single makefile?
#
ifneq ($(MLIBS), )
include $(DEPTH)/mk/multilib.mk
endif

#
# Are we building multiple programs from a single makefile?
#
ifneq ($(MPROGS), )
include $(DEPTH)/mk/multiprog.mk
endif

###############################################################################
#	Handle the DIRS variable. This is a list of directories to
#	enter and make.

STRIP_DIRS = $(strip $(DIRS))
ifneq ($(STRIP_DIRS),)
.PHONY:	$(STRIP_DIRS)

.WAIT:
	echo "Waiting for background processes to finish..."; \
	wait `jobs -p`

$(STRIP_DIRS):
	$(MAKE) -C $@ $(MAKECMDGOALS)
endif

###############################################################################
#	Handle the PROG variable. PROG designates an executable that
#	is compiled from the sources in PROG_SRC. (See also MPROGS in multiprog.mk)

STRIP_PROG = $(strip $(PROG))
ifneq ($(STRIP_PROG),)
.PHONY: $(STRIP_PROG)

PROG_OBJS = $(PROG_SRC:%.c=$(OBJ_DIR)/%.o)

#
# Create a variable with the path to the dependency libraries.
# This only catches changes to existing libraries.  If the library
# doesn't exist, this does nothing.
X_PROG_LIBDEP	:= $(foreach lib, $(patsubst -l%, lib%.so, $(PROG_LIBS)),\
			$(foreach dir, $(patsubst -L%, %,$(PROG_LIBS)),\
				$(wildcard $(dir)/$(lib))))
DEPLIBS = $(strip $(X_PROG_LIBDEP))

$(OBJ_DIR)/$(PROG):	$(PROG_OBJS) $(DEPLIBS)
	$(LD) -o $@ $(LDFLAGS) $(PROG_OBJS) $(PROG_LIBS)
	$(MCS) $@

$(STRIP_PROG):	$(addprefix $(OBJ_DIR)/, $(PROG))

endif

###############################################################################
#	Handle the LIB variable. LIB designates a library that
#	is compiled from the sources in LIB_SRC. (See also MLIBS in multilib.mk)

STRIP_LIB = $(strip $(LIB))
ifneq ($(STRIP_LIB), )
.PHONY: $(STRIP_LIB)

CFLAGS += $(SHARED_CFLAGS)

LIB_OBJS = $(LIB_SRC:%.c=$(OBJ_DIR)/%.o)

#
# Create a variable with the path to the dependency libraries.
# This only catches changes to existing libraries.  If the library
# doesn't exist, this does nothing.
#
X_LIB_LIBDEP	:= $(foreach lib, $(patsubst -l%, lib%.a, $(LIB_LIBS)),\
			$(foreach dir, $(patsubst -L%, %,$(LIB_LIBS)),\
				$(wildcard $(dir)/$(lib))))

DEPLIBS = $(strip $(X_LIB_LIBDEP))

# Generate both static (.a) and shared (.so) formats
#
ALL_LIBS = $(addsuffix .a, $(STRIP_LIB)) $(addsuffix .so, $(STRIP_LIB))
$(STRIP_LIB):  $(addprefix $(OBJ_DIR)/lib, $(ALL_LIBS))

$(OBJ_DIR)/lib%.a:   $(LIB_OBJS) $(DEPLIBS)
	-rm -f $@
	ar ruc $@ $(LIB_OBJS)
	-ranlib $@
	-@if [ ! -z "$(LIB_HOME)" ]; then \
		echo rm -f $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
		rm -f $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
		echo ln -s $(LIB_HOME)/$(OBJ_DIR)/$(notdir $@) $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
		ln -fs $(LIB_HOME)/$(OBJ_DIR)/$(notdir $@) $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
	fi

$(OBJ_DIR)/lib%.so:   $(LIB_OBJS) $(DEPLIBS)
	-rm -f $@
	$(LD) $(SHARED_CFLAGS) -o $@ $(LDFLAGS) $(LIB_OBJS) $(LIB_LIBS)
	-@if [ ! -z "$(LIB_HOME)" ]; then \
		echo rm -f $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
		rm -f $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
		echo ln -s $(LIB_HOME)/$(OBJ_DIR)/$(notdir $@) $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
		ln -fs $(LIB_HOME)/$(OBJ_DIR)/$(notdir $@) $(DEPTH)/lib/$(OBJ_DIR)/$(notdir $@); \
	fi

endif

###############################################################################
#	Handle the MODULE variable. MODULE designates the name of the kernel module
#	and is compiled from the sources in MODULE_SRC.

STRIP_MODULE = $(strip $(MODULE))
ifneq ($(STRIP_MODULE),)
.PHONY: $(STRIP_MODULE)

X_MODULE_OBJS = $(MODULE_SRC:%.c=$(OBJ_DIR)/%.o)
MODULE_OBJS = $(X_MODULE_OBJS:%.S=$(OBJ_DIR)/%.o)

#
# We need to execute the ctfmerge command in OBJ_DIR to prevent a race
# condition with an intermediate temporary output file.  _BASE is used
# for this purpose.
#
X_MODULE_OBJS_BASE = $(MODULE_SRC:%.c=%.o)
MODULE_OBJS_BASE = $(X_MODULE_OBJS_BASE:%.S=%.o)

$(OBJ_DIR)/$(MODULE):	$(MODULE_OBJS)
	-rm -f $@
	ld $(LDFLAGS) -o $@ $(MODULE_OBJS)
ifeq ($(BUILD_STABS), yes)
	$(CTFMERGE_CMD)
endif
	$(MCS) $@

$(STRIP_MODULE):	$(addprefix $(OBJ_DIR)/, $(MODULE))
endif

###############################################################################
# setup general java compile rules here
#
.SUFFIXES: .mid .class .java
.java.class $(OBJ_DIR)/%.class:	%.java
	$(JC) $(JFLAGS) $(JDEBUG) $<

.class.mid:
	$(JP) $(JFLAGS) -s -p $(JDEPTH).$* > $*.mid

.class.h:
	$(JH) $(JFLAGS) $<

#
# Always create the fsmgmtjni.jar in the OBJ_DIR
# This should work during multiple builds simultaneously
# pkg/samqfsuiproto_header is changed to pick the fsmgmtjni.jar from
# the appropriate OBJ_DIR
#
# From R 4.5.q the fsmgmtjni.jar only get created in
# src/lib/sammgmtjni/##OBJ_DIR##
# Temporarily retain code to remove the jar from
# src/lib/sammgmtjni (as created by previous builds)
#
%.jar: $(JAVAOBJS)
	@rm -f $@
	@echo "creating jar file ($@)..."
	@cd $(OBJ_DIR); \
	$(JAVA_HOME)/bin/jar cf $@ $(patsubst $(OBJ_DIR)/%,%,$(JAVAOBJS))
	@echo done

java_hdrs:	$(JAVAOBJS)
	$(JH) $(JFLAGS) $(JAVAHDRS:.class=)

java_lib:    $(JAVAOBJS) java_hdrs $(LIB).jar


###############################################################################

.INIT:

all:	.INIT $(STRIP_DIRS) $(JAVA_TARGETS) $(STRIP_LIB) $(STRIP_PROG) $(STRIP_MODULE)

#
# Make sure .INIT gets called before anything else
#
$(JAVA_TARGETS) $(STRIP_LIB) $(STRIP_PROG) $(STRIP_MODULE): .INIT


clean:	$(STRIP_DIRS)
	-rm -rf $(OBJ_DIR) *.ln

clobber:	clean

clobberall:	$(STRIP_DIRS)
	-rm -rf $(OBJ_BASE) *.ln

install:

$(OBJ_DIR)/%.o:		%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
ifeq ($(BUILD_STABS), yes)
	$(CTFCONVERT_CMD)
endif

$(OBJ_DIR)/%.o:		%.S
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $< -o $@
ifeq ($(BUILD_STABS), yes)
	$(CTFCONVERT_CMD)
endif

%.cat:	$(CATMSGS)
	-rm -f             $@
	/usr/bin/gencat   $@ $<
	/usr/bin/chmod -w $@

ifneq ($(DEPLIBS),)
$(DEPLIBS):
	# Need to add something here to build dependent libs
	@echo "Warning: one or more of the required libraries do not exist."
	@echo "Run 'gmake' at the top level to rebuild the libraries."
	exit 1
#	cd $(DEPTH)/src/lib; \
#	$(MAKE) DEBUG=$(DEBUG) COMPLETE=$(COMPLETE); \
#	if [ ! -f $@ ]; then \
#		exit 1; \
#	fi
endif


MARKER = $(shell cat $(DEPTH)/MARKER)

info:	$(DEPTH)/MARKER
	@echo
	@echo "BUILD PARAMETERS"
	@echo "           DEBUG: " $(DEBUG)
	@echo "        COMPLETE: " $(COMPLETE)
	@echo
	@echo "BUILD ENVIROMENT"
	@echo "     GUI_VERSION: " $(GUI_VERSION)
	@echo "  SAMQFS_VERSION: " $(SAMQFS_VERSION)
	@echo "          MARKER: " $(MARKER)
	@echo "        HOSTNAME: " $(HOSTNAME)
	@echo "              OS: " $(OS)
	@echo "     OS_REVISION: " $(OS_REVISION)
	@echo "        PLATFORM: " $(PLATFORM)
	@echo "      ISA_KERNEL: " $(ISA_KERNEL)
	@echo "         OBJ_DIR: " $(OBJ_DIR)

$(DEPTH)/MARKER:
	@if [ "$(shell /bin/basename $(shell cd $(DEPTH); /bin/pwd))" = "sam-qfs" ] ; then	\
		echo "$(shell /bin/basename $(shell cd $(DEPTH)/..; /bin/pwd))" > $@;	\
	else	\
		echo "$(shell /bin/basename $(shell cd $(DEPTH); /bin/pwd))" > $@;	\
	fi

#
# Lint target.  Automatically generate the list of source files
# based on $(PROG_SRC) $(LIB_SRC) $(MODULE_SRC) $(PROGS_SRC) $(LIBS_SRC)
# settings.  Take into account any vpath settings.
#
ifeq ($(LNSRC), )
ifeq ($(strip $(SRC_VPATH)), )
X_LINT_SRC = $(PROG_SRC) $(LIB_SRC) $(MODULE_SRC) $(PROGS_SRC) $(LIBS_SRC)
else
	#
	# if we're using vpath, we need to figure out where all
	# the source actually lives.
	#
	# WARNING:	This does not strip out files found in vpath having the same name.  If
	#			there are multiple files with the same name, then override this by
	#			setting LNSRC to the exact list of files to be used in your GNUmakefile.
	#
X_LINT_SRC = $(foreach file, $(PROG_SRC) $(LIB_SRC) $(MODULE_SRC) $(PROGS_SRC) $(LIBS_SRC), \
		$(foreach dir, . $(SRC_VPATH),  $(wildcard $(dir)/$(file))))
endif
LNSRC = $(strip $(X_LINT_SRC))
endif

ifeq ($(LNFLAGS), )
LNFLAGS = $(DEPCFLAGS)
endif

ifneq ($(LIB), )
LNOPTS += -o$(LIB)_$(OS_ARCH)
endif

lint:	.INIT $(STRIP_DIRS)
ifneq ($(LNSRC), )
	$(LINT) $(LNOPTS) $(LNFLAGS) $(LNSRC) $(LNLIBS)
ifneq ($(LIB), )
	-rm -f $(OBJ_DIR)/llib-l$(LIB).ln
	-mv -f llib-l$(LIB)_$(OS_ARCH).ln $(OBJ_DIR)/llib-l$(LIB).ln
endif
ifneq ($(LIB_HOME), )
	-rm -f $(DEPTH)/lib/$(OBJ_DIR)/llib-l$(LIB).ln
	-ln -s $(DEPTH)/src/lib/$(LIB)/$(OBJ_DIR)/llib-l$(LIB).ln $(DEPTH)/lib/$(OBJ_DIR)/llib-l$(LIB).ln
endif
endif

#
# Handle sparcv9 builds in those GNUmakefiles with BUILD_64BIT=yes
# Clear DIRS so that we don't recursively build sparcv9.
#
ifeq ($(PLATFORM), sparc)
ifeq ($(BUILD_64BIT), yes)
ifneq ($(SPARCV9), yes)
.INIT all clean clobber depend install:	sparcv9
sparcv9:
	$(MAKE) DIRS= SPARCV9=yes $(MAKECMDGOALS)
endif
endif
endif


#
# Handle amd64 builds in those GNUmakefiles with BUILD_64BIT=yes
# Clear DIRS so that we don't recursively build amd64.
#
ifeq ($(PLATFORM), i386)
ifeq ($(BUILD_64BIT), yes)
ifneq ($(AMD64), yes)
.INIT all clean clobber depend install:	amd64
amd64:
	$(MAKE) DIRS= AMD64=yes $(MAKECMDGOALS)
endif
endif
endif

ifneq ($(OS), SunOS)
#
# Special top level target to build Linux client ISO image
#
.PHONY:	linuxpkg
linuxpkg:
	$(MAKE) -C pkg-linux iso
endif
