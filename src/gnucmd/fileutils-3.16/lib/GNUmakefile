# $Id: GNUmakefile,v 1.5 2007/10/08 15:46:41 dr161460 Exp $
# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.1l from Makefile.am

# Copyright (C) 1994, 1995, 1996 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.

DEPTH = ../../../..
include $(DEPTH)/mk/common.mk
__dummy := $(shell [ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR) )

SHELL = /bin/sh

#  The interlock and ylwrap scripts failed occasionally due to permissions.
.NO_PARALLEL:

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

pkgdatadir = $(datadir)/fileutils
pkglibdir = $(libdir)/fileutils
pkgincludedir = $(includedir)/fileutils

top_builddir = ..

INSTALL = .././install-sh -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,
ifeq ($(PLATFORM), sparc)
YACC = $(BISON_KLUDGE) -y
else
YACC = bison -y
endif
GENCAT = 
PERL = /usr/local/bin/perl
MV = /bin/mv
GMSGFMT = /bin/msgfmt
CC = ${GCC}
RANLIB = ranlib
LN = /bin/ln
RM = /bin/rm

noinst_LIBRARIES = $(OBJ_DIR)/libfu.a

EXTRA_DIST = mkdir.c rmdir.c getline.c mktime.c strftime.c getgroups.c

INCLUDES = -I.. -I$(srcdir) -I../intl

libfu_a_SOURCES = getdate.y posixtm.y getopt.c getopt1.c \
argmatch.c backupfile.c dirname.c filemode.c \
full-write.c getversion.c idcache.c \
isdir.c long-options.c makepath.c modechange.c path-concat.c \
safe-read.c save-cwd.c savedir.c stripslash.c userspec.c xgetcwd.c \
xmalloc.c xstrdup.c xstrtol.c xstrtoul.c yesno.c

ifeq ($(OS), Linux)
X_libfu_a_LIBADD =  regex.o euidaccess.o group-member.o rpmatch.o strndup.o strftime.o error.o obstack.o fsusage.o mountlist.o
else
X_libfu_a_LIBADD =  regex.o euidaccess.o group-member.o rpmatch.o stpcpy.o strndup.o strftime.o getline.o error.o obstack.o fsusage.o mountlist.o 
endif
libfu_a_DEPENDENCIES = $(libfu_a_LIBADD)
libfu_a_LIBADD = $(addprefix $(OBJ_DIR)/, $(X_libfu_a_LIBADD))

noinst_HEADERS = argmatch.h backupfile.h error.h fnmatch.h fsusage.h \
getdate.h getline.h getopt.h group-member.h long-options.h \
makepath.h modechange.h mountlist.h path-concat.h pathmax.h \
save-cwd.h xstrtol.h xstrtoul.h

BUILT_SOURCES = $(OBJ_DIR)/getdate.c $(OBJ_DIR)/posixtm.c
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
LIBRARIES =  $(noinst_LIBRARIES)

DEFS = -m32 -DHAVE_CONFIG_H $(OSDEPCFLAGS) 

ifeq ($(OS), Linux)
DEFS += -DCONFIG_BROKETS -DSUNW ${OSFLAGS}
INCLUDES += -I$(LINUX_INCLUDE) 
endif

CPPFLAGS = 
LDFLAGS += 
LIBS = -lgen
X_libfu_a_OBJECTS =  getdate.o posixtm.o getopt.o getopt1.o argmatch.o \
backupfile.o dirname.o filemode.o full-write.o getversion.o idcache.o \
isdir.o long-options.o makepath.o modechange.o path-concat.o \
safe-read.o save-cwd.o savedir.o stripslash.o userspec.o xgetcwd.o \
xmalloc.o xstrdup.o xstrtol.o xstrtoul.o yesno.o
libfu_a_OBJECTS = $(addprefix $(OBJ_DIR)/, $(X_libfu_a_OBJECTS))
AR = ar
INTERLOCK = ../../interlock
YLWRAP = ../../ylwrap
CFLAGS = -g -O2 $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CERRWARN)
COMPILE = $(CC) $(CFLAGS)
LINK = $(CC) $(LDFLAGS) -o $(OBJ_DIR)/$@
HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  Makefile.am Makefile.in alloca.c basename.c error.c \
error.h euidaccess.c fileblocks.c fnmatch.c fsusage.c ftruncate.c \
$(OBJ_DIR)/getdate.c getgroups.c getline.c group-member.c interlock memcmp.c \
memcpy.c memset.c mkdir.c mktime.c mountlist.c obstack.c obstack.h \
$(OBJ_DIR)/posixtm.c regex.c regex.h rename.c rmdir.c rpmatch.c rx.c rx.h stpcpy.c \
strcasecmp.c strdup.c strftime.c strndup.c strstr.c strtol.c strtoul.c \
ylwrap

PACKAGE = fileutils
VERSION = 3.16

DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) \
	$(TEXINFOS) $(MANS) $(EXTRA_DIST)

TAR = tar
SOURCES = $(libfu_a_SOURCES)
OBJECTS = $(libfu_a_OBJECTS)

include $(DEPTH)/mk/targets.mk

.INIT:
	chmod 755 interlock
	chmod 755 ylwrap

default: all

.SUFFIXES:
.SUFFIXES: .c .o .y
$(srcdir)/Makefile.in: #M# Makefile.am $(top_srcdir)/configure.in $(ACLOCAL) 
	cd $(top_srcdir) && automake --gnits lib/Makefile

Makefile: #SUN# $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

mostlyclean-noinstLIBRARIES:

clean-noinstLIBRARIES:
	test -z "$(noinst_LIBRARIES)" || rm -f $(noinst_LIBRARIES)

distclean-noinstLIBRARIES:

maintainer-clean-noinstLIBRARIES:

$(OBJ_DIR)/.c.o:
	$(COMPILE) -c -o $@ $<

mostlyclean-compile:
	rm -rf *.o core $(OBJ_DIR)

clean-compile:
	test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)

distclean-compile:
	rm -f *.tab.c

maintainer-clean-compile:
$(libfu_a_OBJECTS): ../config.h

$(OBJ_DIR)/libfu.a: $(libfu_a_OBJECTS) $(libfu_a_DEPENDENCIES)
	rm -f $@
	$(AR) cru $@ $(libfu_a_OBJECTS) $(libfu_a_LIBADD)
	$(RANLIB) $@

$(OBJ_DIR)/getdate.c:  getdate.y
	cp getdate.y $(OBJ_DIR)
	cd $(OBJ_DIR); $(SHELL) $(INTERLOCK) =yacclockdir $(YLWRAP) "$(YACC)" y.tab.c getdate.c y.tab.h getdate.h -- $(YFLAGS) $<

$(OBJ_DIR)/posixtm.c:  posixtm.y
	cp posixtm.y $(OBJ_DIR)
	cd $(OBJ_DIR); $(SHELL) $(INTERLOCK) =yacclockdir $(YLWRAP) "$(YACC)" y.tab.c posixtm.c y.tab.h posixtm.h -- $(YFLAGS) $<

#.y.c:
#	$(SHELL) $(INTERLOCK) =yacclockdir $(YLWRAP) "$(YACC)" y.tab.c $*.c y.tab.h $*.h -- $(YFLAGS) $<

tags: TAGS

ID: $(HEADERS) $(SOURCES)
	here=`pwd` && cd $(srcdir) && mkid -f$$here/ID $(SOURCES) $(HEADERS)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES)
	tags=; \
	here=`pwd`; \
	list="$(SUBDIRS)"; for subdir in $$list; do \
	  test -f $$subdir/TAGS && tags="$$tags -i $$here/$$subdir/TAGS"; \
	done; \
	test -z "$(ETAGS_ARGS)$(SOURCES)$(HEADERS)$$tags" \
	  || cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $(SOURCES) $(HEADERS) -o $$here/TAGS

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
$(OBJ_DIR)/alloca.o alloca.lo: alloca.c ../config.h
$(OBJ_DIR)/argmatch.o argmatch.lo: argmatch.c ../config.h
$(OBJ_DIR)/backupfile.o backupfile.lo: backupfile.c ../config.h backupfile.h
$(OBJ_DIR)/basename.o basename.lo: basename.c ../config.h
$(OBJ_DIR)/dirname.o dirname.lo: dirname.c ../config.h
$(OBJ_DIR)/error.o error.lo: error.c ../config.h error.h
$(OBJ_DIR)/euidaccess.o euidaccess.lo: euidaccess.c ../config.h
$(OBJ_DIR)/fileblocks.o fileblocks.lo: fileblocks.c ../config.h
$(OBJ_DIR)/filemode.o filemode.lo: filemode.c ../config.h
$(OBJ_DIR)/fnmatch.o fnmatch.lo: fnmatch.c ../config.h fnmatch.h
$(OBJ_DIR)/fsusage.o fsusage.lo: fsusage.c ../config.h fsusage.h
$(OBJ_DIR)/ftruncate.o ftruncate.lo: ftruncate.c ../config.h
$(OBJ_DIR)/full-write.o full-write.lo: full-write.c ../config.h
$(OBJ_DIR)/getdate.o $(OBJ_DIR)/getdate.lo: $(OBJ_DIR)/getdate.c ../config.h getdate.h
$(OBJ_DIR)/getgroups.o getgroups.lo: getgroups.c ../config.h
$(OBJ_DIR)/getline.o getline.lo: getline.c ../config.h
$(OBJ_DIR)/getopt.o getopt.lo: getopt.c ../config.h getopt.h
$(OBJ_DIR)/getopt1.o getopt1.lo: getopt1.c ../config.h getopt.h
$(OBJ_DIR)/getversion.o getversion.lo: getversion.c ../config.h backupfile.h
$(OBJ_DIR)/group-member.o group-member.lo: group-member.c ../config.h \
	group-member.h
$(OBJ_DIR)/idcache.o idcache.lo: idcache.c ../config.h
$(OBJ_DIR)/isdir.o isdir.lo: isdir.c ../config.h
$(OBJ_DIR)/long-options.o long-options.lo: long-options.c ../config.h getopt.h \
	long-options.h
$(OBJ_DIR)/makepath.o makepath.lo: makepath.c ../config.h makepath.h
$(OBJ_DIR)/memcmp.o memcmp.lo: memcmp.c ../config.h
$(OBJ_DIR)/memcpy.o memcpy.lo: memcpy.c ../config.h
$(OBJ_DIR)/memset.o memset.lo: memset.c
$(OBJ_DIR)/mkdir.o mkdir.lo: mkdir.c ../config.h
$(OBJ_DIR)/mktime.o mktime.lo: mktime.c ../config.h
$(OBJ_DIR)/modechange.o modechange.lo: modechange.c ../config.h modechange.h
$(OBJ_DIR)/mountlist.o mountlist.lo: mountlist.c ../config.h mountlist.h
$(OBJ_DIR)/obstack.o obstack.lo: obstack.c obstack.h ../config.h
$(OBJ_DIR)/path-concat.o path-concat.lo: path-concat.c ../config.h
$(OBJ_DIR)/posixtm.o posixtm.lo: $(OBJ_DIR)/posixtm.c ../config.h
$(OBJ_DIR)/regex.o regex.lo: regex.c ../config.h
$(OBJ_DIR)/rename.o rename.lo: rename.c ../config.h
$(OBJ_DIR)/rmdir.o rmdir.lo: rmdir.c ../config.h
$(OBJ_DIR)/rpmatch.o rpmatch.lo: rpmatch.c ../config.h
$(OBJ_DIR)/rx.o rx.lo: rx.c ../config.h
$(OBJ_DIR)/safe-read.o safe-read.lo: safe-read.c ../config.h
$(OBJ_DIR)/save-cwd.o save-cwd.lo: save-cwd.c ../config.h save-cwd.h error.h
$(OBJ_DIR)/savedir.o savedir.lo: savedir.c ../config.h
$(OBJ_DIR)/stpcpy.o stpcpy.lo: stpcpy.c ../config.h
$(OBJ_DIR)/strcasecmp.o strcasecmp.lo: strcasecmp.c ../config.h
$(OBJ_DIR)/strdup.o strdup.lo: strdup.c ../config.h
$(OBJ_DIR)/strftime.o strftime.lo: strftime.c ../config.h
$(OBJ_DIR)/stripslash.o stripslash.lo: stripslash.c ../config.h
$(OBJ_DIR)/strndup.o strndup.lo: strndup.c ../config.h
$(OBJ_DIR)/strstr.o strstr.lo: strstr.c
$(OBJ_DIR)/strtol.o strtol.lo: strtol.c ../config.h
$(OBJ_DIR)/strtoul.o strtoul.lo: strtoul.c strtol.c ../config.h
$(OBJ_DIR)/userspec.o userspec.lo: userspec.c ../config.h
$(OBJ_DIR)/xgetcwd.o xgetcwd.lo: xgetcwd.c ../config.h pathmax.h
$(OBJ_DIR)/xmalloc.o xmalloc.lo: xmalloc.c ../config.h error.h
$(OBJ_DIR)/xstrdup.o xstrdup.lo: xstrdup.c ../config.h
$(OBJ_DIR)/xstrtol.o xstrtol.lo: xstrtol.c ../config.h xstrtol.h
$(OBJ_DIR)/xstrtoul.o xstrtoul.lo: xstrtoul.c xstrtol.c ../config.h xstrtol.h
$(OBJ_DIR)/yesno.o yesno.lo: yesno.c ../config.h

info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: 
	$(NORMAL_INSTALL)

install-data: 
	$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: 

all: $(LIBRARIES) $(BUILT_SOURCES) $(HEADERS) Makefile

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

# SUN targets:
MODIFIED_SRCS =
SRCS = alloca.c argmatch.c backupfile.c basename.c dirname.c error.c \
	euidaccess.c fileblocks.c filemode.c fnmatch.c fsusage.c ftruncate.c \
	full-write.c $(OBJ_DIR)/getdate.c getline.c getdate.y getopt.c \
	getopt1.c getversion.c group-member.c idcache.c \
	isdir.c long-options.c makepath.c memcmp.c memset.c mkdir.c modechange.c \
	mountlist.c obstack.c path-concat.c posixtm.c \
	regex.c rename.c rmdir.c rpmatch.c rx.c \
	safe-read.c save-cwd.c savedir.c stpcpy.c strdup.c strftime.c \
	stripslash.c strndup.c strstr.c strtol.c strtoul.c userspec.c xgetcwd.c \
	xmalloc.c xstrdup.c xstrtol.c xstrtoul.c yesno.c \
	Makefile.in posixtm.y 

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

include $(DEPTH)/mk/depend.mk
