# $Revision: 1.16 $

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

# GPL Notice
#      This program is free software; you can redistribute it and/or modify
#      it under the terms of the GNU General Public License as published by
#      the Free Software Foundation; version 2.
#
#      This program is distributed in the hope that it will be useful,
#      but WITHOUT ANY WARRANTY; without even the implied warranty of
#      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#      GNU General Public License for more details.
#
#      You should have received a copy of the GNU General Public License
#      along with this program; if not, write to the Free Software Foundation,
#      Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#      For the avoidance of doubt, except that if any license choice other
#      than GPL or LGPL is available it will apply instead, Sun elects to
#      use only the General Public License version 2 (GPLv2) at this time
#      for any software where a choice of GPL license versions is made
#      available with the language indicating that GPLv2 or any later
#      version may be used, or where a choice of which version of the GPL
#      is applied is otherwise unspecified.

DEPTH = ../../../..

include $(DEPTH)/mk/common.mk

SRC_VPATH = $(OBJ_DIR)
vpath %.c $(SRC_VPATH)

LIB = fu
LIB_SRC = \
	getdate.c \
	posixtm.c \
	argmatch.c \
	backupfile.c \
	basename.c \
	dirname.c \
	eaccess.c \
	error.c \
	filemode.c \
	fsusage.c \
	getopt.c \
	getopt1.c \
	getversion.c \
	idcache.c \
	isdir.c \
	makepath.c \
	modechange.c \
	mountlist.c \
	stripslash.c \
	userspec.c \
	xgetcwd.c \
	xmalloc.c \
	xstrdup.c \
	yesno.c \

ifeq ($(OS), Linux)
LIB_SRC += fnmatch.c
else
LIB_SRC += \
	alloca.c \
	fileblocks.c \
	fnmatch.c \
	ftruncate.c \
	mktime.c \
	stpcpy.c \
	strdup.c \
	strstr.c
endif

YACC = yacc

DEFS = -DCONFIG_BROKETS -DHAVE_CONFIG_H -DSUNW $(METADATA_SERVER)
ifeq ($(OS), Linux)
INCS = -I$(LINUX_INCLUDE)
endif
INCS += -I$(DEPTH)/include -I.. -I.
DEPCFLAGS = $(OSFLAGS) $(DEFS) $(INCS)

# valist deprecated 
CERRWARN += -erroff=E_DEPRECATED_ATT

ifeq ($(OS), Linux)
CFLAGS += $(DEBUGFLAGS) -O
else
CFLAGS_CSTD = $(DEBUGFLAGS) -O -xc99=none
CFLAGS_GCC = $(DEBUGFLAGS) -O
CFLAGS += $(CFLAGS_$(COMPILER))
endif

LNOPTS = -u -x -Dlint
LNFLAGS = $(DEFS) $(INCS)  -DMVDIR="\"$(libdir)/mvdir\""

include $(DEPTH)/mk/targets.mk

# Since this directory contains two parsers, we have to be careful to avoid
# running two $(YACC)s during parallel makes.  See below.
$(OBJ_DIR)/getdate.c: getdate.y
	@echo expect 9 shift/reduce conflicts
	cd $(OBJ_DIR); \
	$(YACC) ../../getdate.y; \
	mv -f y.tab.c getdate.c; \
	chmod -w getdate.c

# Make the rename atomic, in case sed is interrupted and later rerun.
# The artificial dependency on getdate.c keeps the two parsers from being
# built in parallel.  Enforcing this little bit of sequentiality lets
# everyone (even those without bison) still run mostly parallel builds.
$(OBJ_DIR)/posixtm.c: posixtm.y $(OBJ_DIR)/getdate.c
	cd $(OBJ_DIR); \
	$(YACC) ../../posixtm.y; \
	mv -f y.tab.c posixtm.tab.c; \
	sed -e 's/yy/zz/g' posixtm.tab.c > tposixtm.c; \
	mv -f tposixtm.c posixtm.c; \
	rm -f posixtm.tab.c; \
	chmod -w posixtm.c

$(OBJ_DIR)/rename.o: rename.c
	$(CC) -c $(CPPFLAGS) -DMVDIR="\"$(libdir)/mvdir\"" $(DEFS) \
	-I.. -I. $(CFLAGS) -o $@ ./rename.c

include $(DEPTH)/mk/depend.mk
