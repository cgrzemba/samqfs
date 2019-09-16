# $Revision: 1.41 $

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

#	sun.mk - definitions for a Sun build environment

#	uname -p output:
#	32-bit sparc: sparc
#	64-bit sparc: sparc
#	32-bit amd64: i386
#	64-bit amd64: i386

#	isainfo -k output:
#	32-bit sparc: sparc
#	64-bit sparc: sparcv9
#	32-bit amd64: i386
#	64-bit amd64: amd64

OS_REVISION := $(shell uname -r)
OS_VERSION := $(shell uname -v)
PLATFORM := $(shell uname -p)
ISA_KERNEL := $(shell isainfo -k)
OS_RELEASE := $(shell  uname -v | cut -d. -f1,2)
OS_RELEASE_MINOR := $(patsubst illumos-%,0,$(shell  uname -v | cut -d. -f2))

#
# set sparc/i386 specific options
#
# PLATFLAGS := -Xa
ifeq ($(PLATFORM), sparc)
	ifeq ($(SPARCV9), yes)
		64DIR := /sparcv9
		ISA_TARGET := sparcv9
		PLATFLAGS += -m64
	else
		ISA_TARGET := sparc
		PLATFLAGS += -m32
	endif
	KERNFLAGS := -xregs=no%float
endif
ifeq ($(PLATFORM), i386)
	ifeq ($(AMD64), yes)
		64DIR := /amd64
		ISA_TARGET := amd64
		ifeq ($(OS_REVISION), 5.10)
			PLATFLAGS += -xarch=amd64 -Ui386 -U__i386
		else
		ifeq ($(OS_REVISION), 5.11)
			PLATFLAGS += -m64 -Ui386 -U__i386
		else
		$(error "Unknown Solaris version $(OS_REVISION)")
		endif
		endif
		KERNFLAGS := -xmodel=kernel -Wu,-save_args
	else
		ISA_TARGET := i386
		PLATFLAGS += -m32
	endif
endif

# GCC = $(SAMFS_TOOLS)/gcc/4.4.4/bin/gcc
GCC = /usr/gcc/7/bin/gcc

ifneq (,$(filter $(OS_RELEASE),11.3 11.4))
  OSDEPCFLAGS  = -DORACLE_SOLARIS
  OSDEPCFLAGS += -D__USE_SUNOS_SOCKETS__
  OSDEPCFLAGS += -D__USE_DRAFT6_PROTOTYPES__
  COMPILER = CSTD
else
  COMPILER = GCC
endif

#
# turn compiler warnings into errors
#
CERRWARN_CSTD = -errtags=yes -errwarn=%all
CERRWARN_GCC = -Wall -Wno-unknown-pragmas -Wno-format
CERRWARN = $(CERRWARN_$(COMPILER))

KERNFLAGS_CSTD = -D_KERNEL -m64 -xmodel=kernel -xregs=no%float -DKERNEL_MINOR=$(OS_RELEASE_MINOR)
KERNFLAGS_GCC = -D_KERNEL -D_ELF64 -m64 -mcmodel=kernel -mno-red-zone -ffreestanding -nodefaultlibs -DKERNEL_MINOR=$(OS_RELEASE_MINOR)

KERNFLAGS = $(KERNFLAGS_$(COMPILER))

#
# Solaris common commands
#
PERL = /usr/bin/perl
BISON_KLUDGE = /usr/bin/bison

AWK = /usr/bin/nawk
INSTALL = /usr/bin/ginstall
MAKEDEPEND = /usr/openwin/bin/makedepend

CMDECHO = /usr/bin/echo
CMDMCS = /usr/ccs/bin/mcs
CMDWHOAMI = /usr/bin/whoami

MSGDEST = $(BASEDIR)/usr/lib/locale/C/LC_MESSAGES

COPYRIGHT = $(SAMFS_TOOLS)/bin/copyright

CSTYLE = $(SAMFS_TOOLS)/bin/cstyle
CSTYLETREE = $(SAMFS_TOOLS)/bin/cstyletree

#
# Solaris version specific commands
#
OS_ARCH := $(OS)_$(OS_RELEASE)_$(ISA_TARGET)
VERS := $(shell $(CMDECHO) -DSOL`uname -r | cut -d . -f1-2 | tr . _` )

ifneq (,$(filter $(OS_RELEASE),11.3 11.4))
	OS_VERS := Solaris 11
	CC = /opt/solarisstudio12.4/bin/cc
	LINT = /opt/solarisstudio12.4/bin/lint
	CSCOPE = /opt/solarisstudio12.4/bin/cscope
	LD = /opt/solarisstudio12.4/bin/cc
else ifeq ($(OS_REVISION), 5.11)
	OS_VERS := Solaris 11
	CC = $(GCC) 
	LINT = /opt/sunstudio12.1/bin/lint
	CSCOPE = /opt/sunstudio12.1/bin/cscope
	LD = $(GCC) 
#	CC = /opt/solarisstudio12.3/bin/cc
#	LINT = /opt/solarisstudio12.3/bin/lint
#	CSCOPE = /opt/solarisstudio12.3/bin/cscope
#	CC = $(SAMFS_TOOLS)/SS12/bin/cc
#	LINT = $(SAMFS_TOOLS)/SS12/bin/lint
#	CSCOPE = $(SAMFS_TOOLS)/SS12/bin/cscope
else
$(error "Unknown Solaris version $(OS_REVISION)")
endif

#
# Kernel Stabs (Symbol TABle entrieS) support.  Each Solaris release
# should use the corresponding Solaris ON tools.
#
ifeq ($(OS_REVISION), 5.10)
	CTFCONVERT = $(SAMFS_TOOLS)/on10-tools/bin/$(PLATFORM)/ctfconvert
	CTFCVTFLAGS = -l $(SAMQFS_VERSION)
	CTFCONVERT_CMD = $(CTFCONVERT) $(CTFCVTFLAGS) $@

	CTFMERGE = $(SAMFS_TOOLS)/on10-tools/bin/$(PLATFORM)/ctfmerge
	CTFMRGFLAGS = -l $(SAMQFS_VERSION)
	CTFMERGE_CMD = cd $(OBJ_DIR); $(CTFMERGE) $(CTFMRGFLAGS) -o $(MODULE) $(MODULE_OBJS_BASE)
else
ifeq ($(OS_REVISION), 5.11)
	CTFCONVERT = $(SAMFS_TOOLS)/onbld/bin/$(PLATFORM)/ctfconvert
	CTFCVTFLAGS = -l $(SAMQFS_VERSION)
	CTFCONVERT_CMD = $(CTFCONVERT) $(CTFCVTFLAGS) $@

	CTFMERGE = $(SAMFS_TOOLS)/onbld/bin/$(PLATFORM)/ctfmerge
	CTFMRGFLAGS = -l $(SAMQFS_VERSION)
	CTFMERGE_CMD = cd $(OBJ_DIR); $(CTFMERGE) $(CTFMRGFLAGS) -o $(MODULE) $(MODULE_OBJS_BASE)
else
$(error "Unknown Solaris version $(OS_REVISION)")
endif
endif

#
# This is to enable the build of the SunCluster agent for SAM-QFS.
# This requires that SUNWscdev be installed on the build machine.
# To force the build, set BUILD_SC_AGENT to yes.
#
ifeq ($(COMPLETE), yes)
	BUILD_SC_AGENT = yes
endif
ifeq ($(shell [ ! -d /usr/cluster/include ] || echo "yes"), yes)
	BUILD_SC_AGENT = yes
endif

#
# Set additional debug options (generate symbols for all architectures)
#
DEBUGCFLAGS += -g
ifeq ($(PLATFORM), sparc)
	# The "-Wc,-Qiselect-T1" enables tail-call optimization when using "-g".
	DEBUGCFLAGS += -Wc,-Qiselect-T1
endif

#
# New java setup rules
#
# JBASE = /usr/java
# JBASE = /usr/jdk/instances/openjdk1.8.0
JBASE = /usr/jdk/instances/jdk1.7.0
JPKG = $(DEPTH)/pkg/vendor/java
JBASEINC = $(JBASE)/include
JBASEBIN = $(JBASE)/bin
JSOLINC = $(JBASE)/include/solaris
J3LIB = $(DEPTH)/src/java/jcbwt.jar
JRETAR = $(JPKG)/jre.tar
JCLSPATH = $(JBASE)/lib/classes.zip:$(J3LIB):$(DEPTH)/src/java:.
#JDEBUG = -deprecation
JFLAGS = -classpath $(JCLSPATH)
JC = $(JBASEBIN)/javac
JP = $(JBASEBIN)/javap
JH = $(JBASEBIN)/javah
JAR = $(JBASEBIN)/jar
JD = $(JBASEBIN)/javadoc
ANT = /usr/bin/ant

#
# Berkeley DB definitions
#
ifeq ($(PLATFORM), sparc)
	ifeq ($(SPARCV9), yes)
		DB_ARCH = solaris_sparcv9
	else
		DB_ARCH = solaris_sparc
	endif
endif
ifeq ($(PLATFORM), i386)
	ifeq ($(AMD64), yes)
		DB_ARCH = amd_64
	else
		DB_ARCH = i386
	endif
endif
DB_VERSION = V4/4.5.20
DB_INCLUDE = $(DEPTH)/src/lib/bdb/$(DB_VERSION)/$(OS_ARCH)/include
DB_LIB = -ldb-4.5

#
# MySQL DB definitions
#
ifeq ($(PLATFORM), i386)
	MYSQL_ARCH = i386
else
	MYSQL_ARCH = sparc
endif
MYSQL_VERSION = V5/5.0.51a
MYSQL_INCLUDE = -I$(DEPTH)/src/lib/mysql/$(MYSQL_VERSION)/$(MYSQL_ARCH)/include
MYSQL_LIB = -lmysqlclient
MYSQL_LIB_R = -lmysqlclient_r

#
# StorageTek 5800 API definitions
#
# ifeq ($(PLATFORM), sparc)
# 	HC_ARCH = sol_10_sparc
# endif
# ifeq ($(PLATFORM), i386)
# 	HC_ARCH = sol_10_x86
# endif
# HC_VERSION = StorageTek5800_SDK_1_1_74
# HC_INCLUDE = -I$(DEPTH)/src/lib/honeycomb/$(HC_VERSION)/include -I$(DEPTH)/src/lib/honeycomb/$(HC_VERSION)/$(HC_ARCH)/include
# HC_LIB = -lstk5800

#
# mcs definitions
#
MCSOPT = $(shell echo "-c -a '@(=)Copyright (c) `/bin/date +%Y`, Sun Microsystems, Inc.  All Rights Reserved' -a '@(=)Built `/bin/date +%D` by `$(CMDWHOAMI)` on `/bin/uname -n` `/bin/uname -s` `/bin/uname -r`.'" | /bin/tr = '\043')
MCS = $(CMDMCS) $(MCSOPT)

#
# libraries to link with when using threads
#
THRLIBS = -lpthread -lthread

METADATA_SERVER = -DMETADATA_SERVER

# not build IBM Tivoly support
NO_BUILD_SANERGY = -D_NoTIVOLI_
# not build ST5800 and ACSLS
NO_BUILD_STK = -D_NoSTK_
# not build OSD
NO_BUILD_OSD = -D_NoOSD_

#
# Final default CFLAGS settings
#
LIBSO_OPT = -R
STATIC_OPT_CSTD = -Bstatic
STATIC_OPT_GCC = -Wl,-Bstatic
STATIC_OPT = $(STATIC_OPT_$(COMPILER))

DYNAMIC_OPT_CSTD = -Bdynamic
DYNAMIC_OPT_GCC = -Wl,-Bdynamic
DYNAMIC_OPT = $(DYNAMIC_OPT_$(COMPILER))

SHARED_CFLAGS_CSTD = -G -K PIC
SHARED_CFLAGS_GCC = -shared -fPIC
# SHARED_CFLAGS = $(SHARED_CFLAGS_$(COMPILER))
SHARED_CFLAGS = $(SHARED_CFLAGS_$(COMPILER))

DEPCFLAGS = -I$(INCLUDE) $(VERS) $(METADATA_SERVER) $(NO_BUILD_SANERGY) $(NO_BUILD_STK) $(NO_BUILD_OSD)

