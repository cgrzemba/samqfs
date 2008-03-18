# $Id: GNUmakefile,v 1.4 2007/10/08 15:46:42 dr161460 Exp $
# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.1l from Makefile.am

# Copyright (C) 1994, 1995, 1996 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.

DEPTH = ../../../..
include $(DEPTH)/mk/common.mk

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

pkgdatadir = $(datadir)/fileutils
pkglibdir = $(libdir)/fileutils
pkgincludedir = $(includedir)/fileutils

top_builddir = ..

INSTALL = .././install-sh -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,
YACC = bison -y
GENCAT =
PERL = /usr/local/bin/perl
MV = /bin/mv
GMSGFMT = /bin/msgfmt
CC = ${GCC}
RANLIB = ranlib
LN = /bin/ln
RM = /bin/rm

man_MANS = chgrp.1 chmod.1 chown.1 cp.1 dd.1 df.1 dircolors.1 du.1 install.1 \
ln.1 ls.1 mkdir.1 mkfifo.1 mknod.1 mv.1 rm.1 rmdir.1 touch.1
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES =
MANS = chgrp.1 chmod.1 chown.1 cp.1 dd.1 df.1 dircolors.1 du.1 install.1 ln.1 ls.1 mkdir.1 mkfifo.1 mknod.1 mv.1 rm.1 rmdir.1 touch.1

NROFF = nroff
DIST_COMMON =  Makefile.am Makefile.in

PACKAGE = fileutils
VERSION = 3.16

DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) \
	$(TEXINFOS) $(MANS) $(EXTRA_DIST)

TAR = tar
default: all

.SUFFIXES:
$(srcdir)/Makefile.in: #M# Makefile.am $(top_srcdir)/configure.in $(ACLOCAL)
	cd $(top_srcdir) && automake --gnits man/Makefile

Makefile: #SUN# $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

install-man: $(MANS)
	$(NORMAL_INSTALL)
	$(mkinstalldirs) $(mandir)/man1
	sect=1;				\
	inst=`echo "ln" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/ln.1; then file=$(srcdir)/ln.1; \
	else file=ln.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "chgrp" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/chgrp.1; then file=$(srcdir)/chgrp.1; \
	else file=chgrp.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "ls" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/ls.1; then file=$(srcdir)/ls.1; \
	else file=ls.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "chmod" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/chmod.1; then file=$(srcdir)/chmod.1; \
	else file=chmod.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "mkdir" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/mkdir.1; then file=$(srcdir)/mkdir.1; \
	else file=mkdir.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "chown" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/chown.1; then file=$(srcdir)/chown.1; \
	else file=chown.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "mkfifo" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/mkfifo.1; then file=$(srcdir)/mkfifo.1; \
	else file=mkfifo.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "cp" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/cp.1; then file=$(srcdir)/cp.1; \
	else file=cp.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "mknod" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/mknod.1; then file=$(srcdir)/mknod.1; \
	else file=mknod.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "dd" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/dd.1; then file=$(srcdir)/dd.1; \
	else file=dd.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "mv" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/mv.1; then file=$(srcdir)/mv.1; \
	else file=mv.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "df" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/df.1; then file=$(srcdir)/df.1; \
	else file=df.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "rm" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/rm.1; then file=$(srcdir)/rm.1; \
	else file=rm.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "dircolors" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/dircolors.1; then file=$(srcdir)/dircolors.1; \
	else file=dircolors.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "rmdir" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/rmdir.1; then file=$(srcdir)/rmdir.1; \
	else file=rmdir.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "du" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/du.1; then file=$(srcdir)/du.1; \
	else file=du.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "touch" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/touch.1; then file=$(srcdir)/touch.1; \
	else file=touch.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst
	sect=1;				\
	inst=`echo "install" | sed '$(transform)'`.1; \
	if test -f $(srcdir)/install.1; then file=$(srcdir)/install.1; \
	else file=install.1; fi; \
	$(INSTALL_DATA) $$file $(mandir)/man$$sect/$$inst

uninstall-man:
	inst=`echo "ln" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "chgrp" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "ls" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "chmod" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "mkdir" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "chown" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "mkfifo" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "cp" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "mknod" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "dd" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "mv" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "df" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "rm" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "dircolors" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "rmdir" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "du" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "touch" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst
	inst=`echo "install" | sed '$(transform)'`.1; \
	rm -f $(mandir)/man1/$$inst

tags: TAGS
TAGS:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = man
distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec:
	$(NORMAL_INSTALL)

install-data: install-man
	$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-man

all: $(MANS) Makefile

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' install
installdirs:
	$(mkinstalldirs)  $(mandir)/man1

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
mostlyclean:  mostlyclean-generic

clean:  clean-generic mostlyclean

distclean:  distclean-generic clean
	rm -f config.status

maintainer-clean:  maintainer-clean-generic distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default install-man uninstall-man tags distdir info dvi \
installcheck install-exec install-data install uninstall all \
installdirs mostlyclean-generic distclean-generic clean-generic \
maintainer-clean-generic clean mostlyclean distclean maintainer-clean

# SUN targets:
MODIFIED_SRCS =
SRCS = chgrp.1 chmod.1 chown.1 cp.1 dd.1 df.1 dircolors.1 du.1 \
	install.1 ln.1 ls.1 mkdir.1 mkfifo.1 mknod.1 mv.1 rm.1 rmdir.1 touch.1 \
	Makefile.am Makefile.in

include $(DEPTH)/mk/targets.mk

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

include $(DEPTH)/mk/depend.mk
