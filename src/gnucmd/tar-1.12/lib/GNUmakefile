# $Id: GNUmakefile,v 1.4 2007/10/08 15:46:44 dr161460 Exp $
# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.1n from Makefile.am

# Copyright (C) 1994, 1995, 1996 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.

# Makefile for GNU tar library.
# Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

DEPTH = ../../../..
include $(DEPTH)/mk/common.mk

__dummy := $(shell [ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR) )

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

pkgdatadir = $(datadir)/tar
pkglibdir = $(libdir)/tar
pkgincludedir = $(includedir)/tar

top_builddir = ..

ACLOCAL = ../missing aclocal
AUTOCONF = ../missing autoconf
AUTOMAKE = ../missing automake
AUTOHEADER = ../missing autoheader

INSTALL = .././install-sh -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = true
PRE_INSTALL = true
POST_INSTALL = true
NORMAL_UNINSTALL = true
PRE_UNINSTALL = true
POST_UNINSTALL = true
CATALOGS =  de.gmo fr.gmo it.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo sl.gmo sv.gmo
CATOBJEXT = .gmo
CC = ${GCC}
DATADIRNAME = share
ECHO_C = \c
ECHO_N = 
GENCAT = 
GMOFILES =  de.gmo fr.gmo it.gmo ko.gmo nl.gmo no.gmo pl.gmo pt.gmo sl.gmo sv.gmo
GMSGFMT = /bin/msgfmt
GT_NO = 
GT_YES = #YES#
INSTOBJEXT = .mo
INTLDEPS = ../intl/$(OBJ)_DIR)/libintl.a
INTLLIBS = ../intl/$(OBJ)_DIR)/libintl.a
INTLOBJS = $(GETTOBJS)
MAKEINFO = makeinfo
MKINSTALLDIRS = ./mkinstalldirs
MSGFMT = /bin/msgfmt
PACKAGE = tar
POFILES =  de.po fr.po it.po ko.po nl.po no.po pl.po pt.po sl.po sv.po
POSUB = po
RANLIB = ranlib
RMT = rmt
U = 
USE_INCLUDED_LIBINTL = yes
USE_NLS = yes
VERSION = 1.12
YACC = $(BISON_KLUDGE) -y

AUTOMAKE_OPTIONS = gnits

noinst_LIBRARIES = $(OBJ_DIR)/libtar.a

# not used on build on GCC
EXTRA_DIST.SunOS = \
alloca.c fileblocks.c fnmatch.c ftruncate.c execlp.c gmalloc.c \
memset.c mkdir.c modechange.h rename.c rmdir.c stpcpy.c strstr.c

noinst_HEADERS = \
argmatch.h backupfile.h error.h fnmatch.h getopt.h getdate.h getpagesize.h \
pathmax.h

libtar_a_SOURCES = \
argmatch.c backupfile.c error.c getdate.y getopt.c getopt1.c getversion.c \
modechange.c msleep.c xgetcwd.c xmalloc.c xstrdup.c

INCLUDES = -I.. -I$(srcdir) -I../intl

libtar_a_LIBADD =   $(OBJ_DIR)/gmalloc.o
libtar_a_DEPENDENCIES = $(libtar_a_LIBADD)
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
LIBRARIES =  $(noinst_LIBRARIES)

DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS = 
LDFLAGS += 
LIBS = -lsocket -lnsl
X_libtar_a_OBJECTS =  argmatch.o backupfile.o error.o getdate.o getopt.o \
getopt1.o getversion.o modechange.o msleep.o xgetcwd.o xmalloc.o \
xstrdup.o
libtar_a_OBJECTS = $(addprefix $(OBJ_DIR)/, $(X_libtar_a_OBJECTS))
AR = ar
CFLAGS = -g -O2 $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CERRWARN)
COMPILE = $(CC) $(CFLAGS)
LINK = $(CC) $(LDFLAGS) -o $(OBJ_DIR)/$@
HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  ChangeLog Makefile.am Makefile.in alloca.c basename.c \
dirname.c execlp.c fileblocks.c fnmatch.c ftruncate.c $(OBJ_DIR)/getdate.c \
gmalloc.c memset.c mkdir.c rename.c rmdir.c strstr.c

DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
SOURCES = $(libtar_a_SOURCES)
OBJECTS = $(libtar_a_OBJECTS)

include $(DEPTH)/mk/targets.mk

default: all

.SUFFIXES:
.SUFFIXES: .c .o .y
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnits lib/Makefile

Makefile: #SUN# $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

mostlyclean-noinstLIBRARIES:

clean-noinstLIBRARIES:
	test -z "$(noinst_LIBRARIES)" || rm -f $(noinst_LIBRARIES)

distclean-noinstLIBRARIES:

maintainer-clean-noinstLIBRARIES:

$(OBJ_DIR)/.c.o:
	$(COMPILE) -c -o $(OBJ_DIR)/$@ $<

mostlyclean-compile:
	rm -rf *.o core $(OBJ_DIR)

clean-compile:

distclean-compile:
	rm -f *.tab.c

maintainer-clean-compile:

$(OBJ_DIR)/libtar.a: $(libtar_a_OBJECTS) $(libtar_a_DEPENDENCIES)
	rm -f $@
	$(AR) cru $@ $(libtar_a_OBJECTS) $(libtar_a_LIBADD)
	$(RANLIB) $@
.y.c:
	$(YACC) $(YFLAGS) $< && mv y.tab.c $@
	if test -f y.tab.h; then \
	if cmp -s y.tab.h $*.h; then rm -f y.tab.h; else mv y.tab.h $*.h; fi; \
	else :; fi

tags: TAGS

ID: $(HEADERS) $(SOURCES)
	here=`pwd` && cd $(srcdir) && mkid -f$$here/ID $(SOURCES) $(HEADERS)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES)
	tags=; \
	here=`pwd`; \
	list='$(SUBDIRS)'; for subdir in $$list; do \
	  test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
	done; \
	test -z "$(ETAGS_ARGS)$(SOURCES)$(HEADERS)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $(SOURCES) $(HEADERS) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = lib
distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
$(OBJ_DIR)/alloca.o $(OBJ_DIR)/alloca.lo: alloca.c ../config.h
$(OBJ_DIR)/argmatch.o $(OBJ_DIR)/argmatch.lo: argmatch.c ../config.h
$(OBJ_DIR)/backupfile.o $(OBJ_DIR)/backupfile.lo: backupfile.c ../config.h backupfile.h
$(OBJ_DIR)/basename.o $(OBJ_DIR)/basename.lo: basename.c ../config.h
$(OBJ_DIR)/dirname.o $(OBJ_DIR)/dirname.lo: dirname.c ../config.h
$(OBJ_DIR)/error.o $(OBJ_DIR)/error.lo: error.c ../config.h error.h
$(OBJ_DIR)/execlp.o $(OBJ_DIR)/execlp.lo: execlp.c ../config.h
$(OBJ_DIR)/fileblocks.o $(OBJ_DIR)/fileblocks.lo: fileblocks.c ../config.h
$(OBJ_DIR)/fnmatch.o $(OBJ_DIR)/fnmatch.lo: fnmatch.c ../config.h fnmatch.h
$(OBJ_DIR)/ftruncate.o $(OBJ_DIR)/ftruncate.lo: ftruncate.c ../config.h
$(OBJ_DIR)/getdate.o $(OBJ_DIR)/getdate.lo: $(OBJ_DIR)/getdate.c ../config.h getdate.h
$(OBJ_DIR)/getopt.o $(OBJ_DIR)/getopt.lo: getopt.c ../config.h getopt.h
$(OBJ_DIR)/getopt1.o $(OBJ_DIR)/getopt1.lo: getopt1.c ../config.h getopt.h
$(OBJ_DIR)/getversion.o $(OBJ_DIR)/getversion.lo: getversion.c ../config.h backupfile.h
$(OBJ_DIR)/gmalloc.o $(OBJ_DIR)/gmalloc.lo: gmalloc.c ../config.h
$(OBJ_DIR)/memset.o $(OBJ_DIR)/memset.lo: memset.c
$(OBJ_DIR)/mkdir.o $(OBJ_DIR)/mkdir.lo: mkdir.c ../config.h
$(OBJ_DIR)/modechange.o $(OBJ_DIR)/modechange.lo: modechange.c ../config.h modechange.h
$(OBJ_DIR)/msleep.o $(OBJ_DIR)/msleep.lo: msleep.c ../config.h
$(OBJ_DIR)/rename.o $(OBJ_DIR)/rename.lo: rename.c ../config.h
$(OBJ_DIR)/rmdir.o $(OBJ_DIR)/rmdir.lo: rmdir.c ../config.h
$(OBJ_DIR)/strstr.o $(OBJ_DIR)/strstr.lo: strstr.c
$(OBJ_DIR)/xgetcwd.o $(OBJ_DIR)/xgetcwd.lo: xgetcwd.c ../config.h pathmax.h
$(OBJ_DIR)/xmalloc.o $(OBJ_DIR)/xmalloc.lo: xmalloc.c ../config.h error.h
$(OBJ_DIR)/xstrdup.o $(OBJ_DIR)/xstrdup.lo: xstrdup.c ../config.h

info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: 
	@$(NORMAL_INSTALL)

install-data: 
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: 

all: $(LIBRARIES) $(HEADERS) Makefile

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install
installdirs:

mostlyclean-generic:
	test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	rm -f Makefile $(DISTCLEANFILES)
	rm -f config.cache config.log stamp-h
	test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-noinstLIBRARIES mostlyclean-compile \
		mostlyclean-tags mostlyclean-generic

clean:  clean-noinstLIBRARIES clean-compile clean-tags clean-generic \
		mostlyclean

distclean:  distclean-noinstLIBRARIES distclean-compile distclean-tags \
		distclean-generic clean
	rm -f config.status

maintainer-clean:  maintainer-clean-noinstLIBRARIES \
		maintainer-clean-compile maintainer-clean-tags \
		maintainer-clean-generic distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default mostlyclean-noinstLIBRARIES distclean-noinstLIBRARIES \
clean-noinstLIBRARIES maintainer-clean-noinstLIBRARIES \
mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile tags mostlyclean-tags distclean-tags \
clean-tags maintainer-clean-tags distdir info dvi installcheck \
install-exec install-data install uninstall all installdirs \
mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean

# Say $(srcdir), so GNU make does not report an ambiguity with the .y.c rule.
$(OBJ_DIR)/getdate.c: getdate.y
	@echo Expect 13 shift/reduce conflicts...
	cp $< $(OBJ_DIR)
	cd $(OBJ_DIR) && \
	  $(YACC) $(YFLAGS) getdate.y; \
	  mv -f y.tab.c getdate.c

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

include $(DEPTH)/mk/depend.mk
