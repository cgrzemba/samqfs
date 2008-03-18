# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.1n from Makefile.am

# Copyright (C) 1994, 1995, 1996 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.

# Makefile for GNU tar sources.
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

SAMINCLUDE = -I$(DEPTH)/include

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = ${SAMFS_TOOLS}
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
INTLDEPS = ../intl/$(OBJ_DIR)/libintl.a
INTLLIBS = ../intl/$(OBJ_DIR)/libintl.a
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
YACC = bison -y

AUTOMAKE_OPTIONS = gnits ansi2knr
bin_PROGRAMS = tar
libexec_PROGRAMS =
EXTRA_PROGRAMS = rmt

noinst_HEADERS = arith.h common.h rmt.h system.h tar.h
rmt_SOURCES = rmt.c
tar_SOURCES = arith.c buffer.c compare.c create.c delete.c extract.c \
incremen.c list.c mangle.c misc.c names.c open3.c rtapelib.c tar.c update.c

INCLUDES = -I../intl -I$(top_srcdir)/lib ${SAMINCLUDE}
LDADD = ../lib/$(OBJ_DIR)/libtar.a ../intl/$(OBJ_DIR)/libintl.a
localedir = $(prefix)/share/locale
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES =
X_PROGRAMS =  $(bin_PROGRAMS) $(libexec_PROGRAMS)
PROGRAMS = $(addprefix $(OBJ_DIR)/, $(X_PROGRAMS))

DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS =
LDFLAGS =
LIBS = -lsocket -lnsl
o = o
ANSI2KNR =
X_rmt_OBJECTS =  rmt.$o
rmt_OBJECTS =  $(addprefix $(OBJ_DIR)/, $(X_rmt_OBJECTS))
rmt_LDADD = $(LDADD)
rmt_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libtar.a
rmt_LDFLAGS =
X_tar_OBJECTS =  arith.$o buffer.$o compare.$o create.$o delete.$o \
extract.$o incremen.$o list.$o mangle.$o misc.$o names.$o open3.$o \
rtapelib.$o tar.$o update.$o
tar_OBJECTS =  $(addprefix $(OBJ_DIR)/, $(X_tar_OBJECTS))
tar_LDADD = $(LDADD)
tar_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libtar.a
tar_LDFLAGS =
CFLAGS = -g -O2 ${VERS}  $(DEFS) $(INCLUDES) $(CERRWARN)
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(CC) $(LDFLAGS) -o $@
HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  ChangeLog Makefile.am Makefile.in ansi2knr.1 ansi2knr.c

DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
SOURCES = $(rmt_SOURCES) $(tar_SOURCES)
OBJECTS = $(rmt_OBJECTS) $(tar_OBJECTS)

include $(DEPTH)/mk/targets.mk

default: all

.SUFFIXES:
.SUFFIXES: .c .o ._c ._o
$(srcdir)/Makefile.in: Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4)
	cd $(top_srcdir) && $(AUTOMAKE) --gnits src/Makefile

Makefile: #SUN# $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(bindir)/`echo $$p|sed '$(transform)'`; \
	done

mostlyclean-libexecPROGRAMS:

clean-libexecPROGRAMS:
	test -z "$(libexec_PROGRAMS)" || rm -f $(libexec_PROGRAMS)

distclean-libexecPROGRAMS:

maintainer-clean-libexecPROGRAMS:

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(libexecdir)
	@list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo "  $(INSTALL_PROGRAM) $$p $(libexecdir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(libexecdir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-libexecPROGRAMS:
	$(NORMAL_UNINSTALL)
	list='$(libexec_PROGRAMS)'; for p in $$list; do \
	  rm -f $(libexecdir)/`echo $$p|sed '$(transform)'`; \
	done

$(OBJ_DIR)/.c.o:
	$(COMPILE) -c -o $@ $<

mostlyclean-compile:
	rm -rf *.o core $(OBJ_DIR)

clean-compile:

distclean-compile:
	rm -f *.tab.c

maintainer-clean-compile:

$(OBJ_DIR)/ansi2knr: $(OBJ_DIR)/ansi2knr.o
	$(LINK) $(OBJ_DIR)/ansi2knr.o $(LIBS)

$(OBJ_DIR)/ansi2knr.o: $(CONFIG_HEADER)

mostlyclean-krextra:

clean-krextra:

distclean-krextra:
	rm -f ansi2knr

maintainer-clean-krextra:

$(OBJ_DIR)/._c._o:
	@echo "$(COMPILE) -c $(OBJ_DIR)/$@ $<"
	@rm -f _$*.c
	@ln $< _$*.c && $(COMPILE) -c $(OBJ_DIR)/$@ _$*.c && mv $(OBJ_DIR)/_$*.o $@ && rm _$*.c

-o $(OBJ_DIR)/$@ ._c.l_o:
	@echo "$(LIBTOOL) --mode=compile $(COMPILE) -c -o $(OBJ_DIR)/$@ $<"
	@rm -f _$*.c
	@ln $< _$*.c && $(LIBTOOL) --mode=compile $(COMPILE) -c -o $(OBJ_DIR)/$@ _$*.c && mv -o $(OBJ_DIR)/_$*.lo $@ && rm _$*.c

mostlyclean-kr:
	rm -f *._o *.l_o *._c _*.c _*.o

clean-kr:

distclean-kr:
	rm -f ansi2knr

maintainer-clean-kr:

$(OBJ_DIR)/rmt: $(rmt_OBJECTS) $(rmt_DEPENDENCIES)
	@rm -f rmt
	$(LINK) $(rmt_LDFLAGS) $(rmt_OBJECTS) $(rmt_LDADD) $(LIBS)

$(OBJ_DIR)/tar: $(tar_OBJECTS) $(tar_DEPENDENCIES)
	@rm -f tar
	$(LINK) $(tar_LDFLAGS) $(tar_OBJECTS) $(tar_LDADD) $(LIBS)

arith._c: arith.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/arith.c > arith.tmp && mv arith.tmp $@
buffer._c: buffer.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/buffer.c > buffer.tmp && mv buffer.tmp $@
compare._c: compare.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/compare.c > compare.tmp && mv compare.tmp $@
create._c: create.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/create.c > create.tmp && mv create.tmp $@
delete._c: delete.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/delete.c > delete.tmp && mv delete.tmp $@
extract._c: extract.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/extract.c > extract.tmp && mv extract.tmp $@
incremen._c: incremen.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/incremen.c > incremen.tmp && mv incremen.tmp $@
list._c: list.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/list.c > list.tmp && mv list.tmp $@
mangle._c: mangle.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/mangle.c > mangle.tmp && mv mangle.tmp $@
misc._c: misc.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/misc.c > misc.tmp && mv misc.tmp $@
names._c: names.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/names.c > names.tmp && mv names.tmp $@
open3._c: open3.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/open3.c > open3.tmp && mv open3.tmp $@
rmt._c: rmt.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/rmt.c > rmt.tmp && mv rmt.tmp $@
rtapelib._c: rtapelib.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/rtapelib.c > rtapelib.tmp && mv rtapelib.tmp $@
tar._c: tar.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/tar.c > tar.tmp && mv tar.tmp $@
update._c: update.c $(ANSI2KNR)
	$(ANSI2KNR) $(srcdir)/update.c > update.tmp && mv update.tmp $@

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

subdir = src
distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
arith.$o arith.l$o: arith.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h
buffer.$o buffer.l$o: buffer.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h ..//lib/fnmatch.h common.h tar.h arith.h \
	..//lib/modechange.h rmt.h
compare.$o compare.l$o: compare.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h \
	rmt.h
create.$o create.l$o: create.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h
delete.$o delete.l$o: delete.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h \
	rmt.h
extract.$o extract.l$o: extract.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h
incremen.$o incremen.l$o: incremen.c system.h ../config.h \
	..//lib/pathmax.h ..//lib/error.h common.h tar.h arith.h \
	..//lib/modechange.h
list.$o list.l$o: list.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h
mangle.$o mangle.l$o: mangle.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h
misc.$o misc.l$o: misc.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h ..//lib/backupfile.h rmt.h common.h tar.h \
	arith.h ..//lib/modechange.h
names.$o names.l$o: names.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h ..//lib/fnmatch.h common.h tar.h arith.h \
	..//lib/modechange.h
open3.$o open3.l$o: open3.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h
rmt.$o rmt.l$o: rmt.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h
rtapelib.$o rtapelib.l$o: rtapelib.c system.h ../config.h \
	..//lib/pathmax.h ..//lib/error.h rmt.h
tar.$o tar.l$o: tar.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h ..//lib/getopt.h common.h tar.h arith.h \
	..//lib/modechange.h ..//lib/backupfile.h
update.$o update.l$o: update.c system.h ../config.h ..//lib/pathmax.h \
	..//lib/error.h common.h tar.h arith.h ..//lib/modechange.h

info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-binPROGRAMS install-libexecPROGRAMS
	@$(NORMAL_INSTALL)

install-data:
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-binPROGRAMS uninstall-libexecPROGRAMS

all: $(ANSI2KNR) $(PROGRAMS) $(HEADERS) Makefile

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install
installdirs:
	$(mkinstalldirs)  $(bindir) $(libexecdir)

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
mostlyclean:  mostlyclean-binPROGRAMS mostlyclean-libexecPROGRAMS \
		mostlyclean-compile mostlyclean-krextra mostlyclean-kr \
		mostlyclean-tags mostlyclean-generic

clean:  clean-binPROGRAMS clean-libexecPROGRAMS clean-compile \
		clean-krextra clean-kr clean-tags clean-generic \
		mostlyclean

distclean:  distclean-binPROGRAMS distclean-libexecPROGRAMS \
		distclean-compile distclean-krextra distclean-kr \
		distclean-tags distclean-generic clean
	rm -f config.status

maintainer-clean:  maintainer-clean-binPROGRAMS \
		maintainer-clean-libexecPROGRAMS \
		maintainer-clean-compile maintainer-clean-krextra \
		maintainer-clean-kr maintainer-clean-tags \
		maintainer-clean-generic distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default mostlyclean-binPROGRAMS distclean-binPROGRAMS \
clean-binPROGRAMS maintainer-clean-binPROGRAMS uninstall-binPROGRAMS \
install-binPROGRAMS mostlyclean-libexecPROGRAMS \
distclean-libexecPROGRAMS clean-libexecPROGRAMS \
maintainer-clean-libexecPROGRAMS uninstall-libexecPROGRAMS \
install-libexecPROGRAMS mostlyclean-compile distclean-compile \
clean-compile maintainer-clean-compile mostlyclean-krextra \
distclean-krextra clean-krextra maintainer-clean-krextra mostlyclean-kr \
distclean-kr clean-kr maintainer-clean-kr tags mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir info dvi \
installcheck install-exec install-data install uninstall all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean

$(OBJ_DIR)/tar.o: tar.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c -o $@ $(srcdir)/tar.c

$(OBJ_DIR)/tar._o: tar._c
	@rm -f _tar.c
	@ln tar._c _tar.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c -o $@ _tar.c
	@mv _tar.o $@
	@rm _tar.c

$(OBJ_DIR)/rmt.o: rmt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c -o $@ $(srcdir)/rmt.c

$(OBJ_DIR)/rmt._o: rmt._c
	@rm -f _rmt.c
	@ln rmt._c _rmt.c
	$(COMPILE) -DLOCALEDIR=\"$(localedir)\" -c -o $@ _rmt.c
	@mv $(OBJ_DIR)/_rmt.o $@
	@rm _rmt.c

# SUN targets:
MODIFIED_SRCS = common.h buffer.c compare.c create.c extract.c incremen.c \
	list.c misc.c update.c
SRCS = ansi2knr.c arith.c buffer.c compare.c create.c delete.c extract.c \
	incremen.c list.c mangle.c misc.c names.c open3.c rmt.c rtapelib.c tar.c \
	update.c Makefile.am Makefile.in

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
.NOTPARALLEL:

include $(DEPTH)/mk/depend.mk
