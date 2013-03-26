# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.1l from Makefile.am

# Copyright (C) 1994, 1995, 1996 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /opt/SUNWsamfs
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = /opt/SUNWsamfs
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

INSTALL = /usr/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,
YACC = bison -y
GENCAT = 
PERL = /opt/csw/bin/perl
MV = /usr/bin/mv
GMSGFMT = /usr/bin/gmsgfmt
CC = gcc
RANLIB = ranlib
LN = /usr/bin/ln
RM = /usr/bin/rm

noinst_LIBRARIES = libfu.a

EXTRA_DIST = mkdir.c rmdir.c getline.c mktime.c strftime.c getgroups.c

INCLUDES = -I.. -I$(srcdir) -I../intl

libfu_a_SOURCES = getdate.y posixtm.y getopt.c getopt1.c \
argmatch.c backupfile.c dirname.c filemode.c \
full-write.c getversion.c idcache.c \
isdir.c long-options.c makepath.c modechange.c path-concat.c \
safe-read.c save-cwd.c savedir.c stripslash.c userspec.c xgetcwd.c \
xmalloc.c xstrdup.c xstrtol.c xstrtoul.c yesno.c

libfu_a_LIBADD =  regex.o euidaccess.o group-member.o rpmatch.o mktime.o strftime.o error.o obstack.o fsusage.o mountlist.o 
libfu_a_DEPENDENCIES = $(libfu_a_LIBADD)

noinst_HEADERS = argmatch.h backupfile.h error.h fnmatch.h fsusage.h \
getdate.h getline.h getopt.h group-member.h long-options.h \
makepath.h modechange.h mountlist.h path-concat.h pathmax.h \
save-cwd.h xstrtol.h xstrtoul.h

BUILT_SOURCES = getdate.c posixtm.c
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
LIBRARIES =  $(noinst_LIBRARIES)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
CPPFLAGS = 
LDFLAGS = 
LIBS = -lgen
libfu_a_OBJECTS =  getdate.o posixtm.o getopt.o getopt1.o argmatch.o \
backupfile.o dirname.o filemode.o full-write.o getversion.o idcache.o \
isdir.o long-options.o makepath.o modechange.o path-concat.o \
safe-read.o save-cwd.o savedir.o stripslash.o userspec.o xgetcwd.o \
xmalloc.o xstrdup.o xstrtol.o xstrtoul.o yesno.o
AR = ar
INTERLOCK = ./interlock
YLWRAP = ./ylwrap
CFLAGS = -g -O2
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(CC) $(LDFLAGS) -o $@
HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  Makefile.am Makefile.in alloca.c basename.c error.c \
error.h euidaccess.c fileblocks.c fnmatch.c fsusage.c ftruncate.c \
getdate.c getgroups.c getline.c group-member.c interlock memcmp.c \
memcpy.c memset.c mkdir.c mktime.c mountlist.c obstack.c obstack.h \
posixtm.c regex.c regex.h rename.c rmdir.c rpmatch.c rx.c rx.h stpcpy.c \
strcasecmp.c strdup.c strftime.c strndup.c strstr.c strtol.c strtoul.c \
ylwrap


PACKAGE = fileutils
VERSION = 3.16

DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) \
	$(TEXINFOS) $(MANS) $(EXTRA_DIST)

TAR = tar
SOURCES = $(libfu_a_SOURCES)
OBJECTS = $(libfu_a_OBJECTS)

default: all

.SUFFIXES:
.SUFFIXES: .c .o .y
$(srcdir)/Makefile.in: #M# Makefile.am $(top_srcdir)/configure.in $(ACLOCAL) 
	cd $(top_srcdir) && automake --gnits lib/Makefile

Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-noinstLIBRARIES:

clean-noinstLIBRARIES:
	test -z "$(noinst_LIBRARIES)" || rm -f $(noinst_LIBRARIES)

distclean-noinstLIBRARIES:

maintainer-clean-noinstLIBRARIES:

.c.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	rm -f *.o core

clean-compile:

distclean-compile:
	rm -f *.tab.c

maintainer-clean-compile:
$(libfu_a_OBJECTS): ../config.h

libfu.a: $(libfu_a_OBJECTS) $(libfu_a_DEPENDENCIES)
	rm -f libfu.a
	$(AR) cru libfu.a $(libfu_a_OBJECTS) $(libfu_a_LIBADD)
	$(RANLIB) libfu.a
.y.c:
	$(SHELL) $(INTERLOCK) =yacclockdir $(YLWRAP) "$(YACC)" y.tab.c $*.c y.tab.h $*.h -- $(YFLAGS) $<

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
alloca.o alloca.lo: alloca.c ../config.h
argmatch.o argmatch.lo: argmatch.c ../config.h
backupfile.o backupfile.lo: backupfile.c ../config.h backupfile.h
basename.o basename.lo: basename.c ../config.h
dirname.o dirname.lo: dirname.c ../config.h
error.o error.lo: error.c ../config.h error.h
euidaccess.o euidaccess.lo: euidaccess.c ../config.h
fileblocks.o fileblocks.lo: fileblocks.c ../config.h
filemode.o filemode.lo: filemode.c ../config.h
fnmatch.o fnmatch.lo: fnmatch.c ../config.h fnmatch.h
fsusage.o fsusage.lo: fsusage.c ../config.h fsusage.h
ftruncate.o ftruncate.lo: ftruncate.c ../config.h
full-write.o full-write.lo: full-write.c ../config.h
getdate.o getdate.lo: getdate.c ../config.h getdate.h
getgroups.o getgroups.lo: getgroups.c ../config.h
getline.o getline.lo: getline.c ../config.h
getopt.o getopt.lo: getopt.c ../config.h getopt.h
getopt1.o getopt1.lo: getopt1.c ../config.h getopt.h
getversion.o getversion.lo: getversion.c ../config.h backupfile.h
group-member.o group-member.lo: group-member.c ../config.h \
	group-member.h
idcache.o idcache.lo: idcache.c ../config.h
isdir.o isdir.lo: isdir.c ../config.h
long-options.o long-options.lo: long-options.c ../config.h getopt.h \
	long-options.h
makepath.o makepath.lo: makepath.c ../config.h makepath.h
memcmp.o memcmp.lo: memcmp.c ../config.h
memcpy.o memcpy.lo: memcpy.c ../config.h
memset.o memset.lo: memset.c
mkdir.o mkdir.lo: mkdir.c ../config.h
mktime.o mktime.lo: mktime.c ../config.h
modechange.o modechange.lo: modechange.c ../config.h modechange.h
mountlist.o mountlist.lo: mountlist.c ../config.h mountlist.h
obstack.o obstack.lo: obstack.c obstack.h ../config.h
path-concat.o path-concat.lo: path-concat.c ../config.h
posixtm.o posixtm.lo: posixtm.c ../config.h
regex.o regex.lo: regex.c ../config.h
rename.o rename.lo: rename.c ../config.h
rmdir.o rmdir.lo: rmdir.c ../config.h
rpmatch.o rpmatch.lo: rpmatch.c ../config.h
rx.o rx.lo: rx.c ../config.h
safe-read.o safe-read.lo: safe-read.c ../config.h
save-cwd.o save-cwd.lo: save-cwd.c ../config.h save-cwd.h error.h
savedir.o savedir.lo: savedir.c ../config.h
stpcpy.o stpcpy.lo: stpcpy.c ../config.h
strcasecmp.o strcasecmp.lo: strcasecmp.c ../config.h
strdup.o strdup.lo: strdup.c ../config.h
strftime.o strftime.lo: strftime.c ../config.h
stripslash.o stripslash.lo: stripslash.c ../config.h
strndup.o strndup.lo: strndup.c ../config.h
strstr.o strstr.lo: strstr.c
strtol.o strtol.lo: strtol.c ../config.h
strtoul.o strtoul.lo: strtoul.c strtol.c ../config.h
userspec.o userspec.lo: userspec.c ../config.h
xgetcwd.o xgetcwd.lo: xgetcwd.c ../config.h pathmax.h
xmalloc.o xmalloc.lo: xmalloc.c ../config.h error.h
xstrdup.o xstrdup.lo: xstrdup.c ../config.h
xstrtol.o xstrtol.lo: xstrtol.c ../config.h xstrtol.h
xstrtoul.o xstrtoul.lo: xstrtoul.c xstrtol.c ../config.h xstrtol.h
yesno.o yesno.lo: yesno.c ../config.h

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


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
