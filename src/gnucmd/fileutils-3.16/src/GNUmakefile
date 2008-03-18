# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.1l from Makefile.am

# Copyright (C) 1994, 1995, 1996 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.

DEPTH = ../../../..
include $(DEPTH)/mk/common.mk
__dummy := $(shell [ -d $(OBJ_DIR) ] || mkdir -p $(OBJ_DIR) )

SAMINCLUDE = -I$(DEPTH)/include

.KEEP: .o

SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = ${SAMFS_TOOLS}
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
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
YACC = bison -y
GENCAT = 
MV = /bin/mv
GMSGFMT = /bin/msgfmt
CC = ${GCC}
RANLIB = ranlib
LN = /bin/ln
RM = /bin/rm

AUTOMAKE_OPTIONS = ansi2knr

EXTRAdir = $(bindir)
EXTRA_PROGRAMS = df mvdir

bin_PROGRAMS = du

ifeq ($(OS), Linux)
# Linux
DEFS = -DCONFIG_BROKETS -DSUNW -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
#DEFS = -DCONFIG_BROKETS -DSUNW
INCLUDES = $(OSFLAGS) -I$(LINUX_INCLUDE) 

LIBS = -L$(DEPTH)/lib/$(OBJ_DIR) -lsamconf 
LDADD = ../lib/$(OBJ_DIR)/libfu.a
else
# SunOS
LIBS = -lgen
LDADD = ../intl/$(OBJ_DIR)/libintl.a ../lib/$(OBJ_DIR)/libfu.a
endif
# End ifeq ($(OS), Linux)

libexec_PROGRAMS = 
datadir = /opt/SUNWsamfs
localedir = $(datadir)/locale

noinst_HEADERS = system.h cp.h ls.h dircolors.h
EXTRA_DIST = cp-aux.c cp-hash.c dcgen.pl dircolors.hin

INCLUDES += -I.. -I$(srcdir) -I$(top_srcdir)/lib -I../intl $(SAMINCLUDE)
DEFS += -DLOCALEDIR=\"$(localedir)\" -DSHAREDIR=\"$(datadir)\" -DHAVE_CONFIG_H

# Use `ginstall' in the definition of PROGRAMS and in dependencies to avoid
# confusion with the `install' target.  The install rule transforms `ginstall'
# to install before applying any user-specified name transformations.

transform = s/ginstall/install/; s,x,x,
ginstall_SOURCES = install.c

cp_SOURCES = cp.c cp-aux.c cp-hash.c
dir_SOURCES = ls.c ls-dir.c
vdir_SOURCES = ls.c ls-vdir.c
ls_SOURCES = ls.c ls-ls.c

BUILT_SOURCES = dircolors.h
PERL = /usr/local/bin/perl
editpl = sed -e 's,@''PERL''@,$(PERL),g'

CLEANFILES = dcgen
MAINTAINERCLEANFILES = dircolors.h

SUFFIXES = .pl
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
X_PROGRAMS =  $(bin_PROGRAMS) $(libexec_PROGRAMS)
PROGRAMS =  $(addprefix $(OBJ_DIR)/, $(bin_PROGRAMS) $(libexec_PROGRAMS))

CPPFLAGS = 
LDFLAGS = 
o = o
ANSI2KNR = 
df_SOURCES = df.c
df_OBJECTS =  $(OBJ_DIR)/df.$o
df_LDADD = $(LDADD)
df_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
df_LDFLAGS = 
mvdir_SOURCES = mvdir.c
mvdir_OBJECTS =  $(OBJ_DIR)/mvdir.$o
mvdir_LDADD = $(LDADD)
mvdir_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
mvdir_LDFLAGS = 
chgrp_SOURCES = chgrp.c
chgrp_OBJECTS =  $(OBJ_DIR)/chgrp.$o
chgrp_LDADD = $(LDADD)
chgrp_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
chgrp_LDFLAGS = 
chown_SOURCES = chown.c
chown_OBJECTS =  $(OBJ_DIR)/chown.$o
chown_LDADD = $(LDADD)
chown_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
chown_LDFLAGS = 
chmod_SOURCES = chmod.c
chmod_OBJECTS =  $(OBJ_DIR)/chmod.$o
chmod_LDADD = $(LDADD)
chmod_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
chmod_LDFLAGS = 
cp_OBJECTS =  $(OBJ_DIR)/cp.$o $(OBJ_DIR)/cp-aux.$o $(OBJ_DIR)/cp-hash.$o
cp_LDADD = $(LDADD)
cp_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
cp_LDFLAGS = 
dd_SOURCES = dd.c
dd_OBJECTS =  $(OBJ_DIR)/dd.$o
dd_LDADD = $(LDADD)
dd_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
dd_LDFLAGS = 
dircolors_SOURCES = dircolors.c
dircolors_OBJECTS =  $(OBJ_DIR)/dircolors.$o
dircolors_LDADD = $(LDADD)
dircolors_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
dircolors_LDFLAGS = 
du_SOURCES = du.c
du_OBJECTS =  $(OBJ_DIR)/du.$o
du_LDADD = $(LDADD) -L $(DEPTH)/lib/$(OBJ_DIR) -lsam $(LIBSO)
du_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
du_LDFLAGS = 
ginstall_OBJECTS =  $(OBJ_DIR)/install.$o
ginstall_LDADD = $(LDADD)
ginstall_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
ginstall_LDFLAGS = 
ln_SOURCES = ln.c
ln_OBJECTS =  $(OBJ_DIR)/ln.$o
ln_LDADD = $(LDADD)
ln_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
ln_LDFLAGS = 
dir_OBJECTS =  $(OBJ_DIR)/ls.$o $(OBJ_DIR)/ls-dir.$o
dir_LDADD = $(LDADD)
dir_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
dir_LDFLAGS = 
vdir_OBJECTS =  $(OBJ_DIR)/ls.$o $(OBJ_DIR)/ls-vdir.$o
vdir_LDADD = $(LDADD)
vdir_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
vdir_LDFLAGS = 
ls_OBJECTS =  $(OBJ_DIR)/ls.$o $(OBJ_DIR)/ls-ls.$o
ls_LDADD = $(LDADD)
ls_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
ls_LDFLAGS = 
mkdir_SOURCES = mkdir.c
mkdir_OBJECTS =  $(OBJ_DIR)/mkdir.$o
mkdir_LDADD = $(LDADD)
mkdir_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
mkdir_LDFLAGS = 
mkfifo_SOURCES = mkfifo.c
mkfifo_OBJECTS =  $(OBJ_DIR)/mkfifo.$o
mkfifo_LDADD = $(LDADD)
mkfifo_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
mkfifo_LDFLAGS = 
mknod_SOURCES = mknod.c
mknod_OBJECTS =  $(OBJ_DIR)/mknod.$o
mknod_LDADD = $(LDADD)
mknod_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
mknod_LDFLAGS = 
mv_SOURCES = mv.c
mv_OBJECTS =  $(OBJ_DIR)/mv.$o
mv_LDADD = $(LDADD)
mv_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
mv_LDFLAGS = 
rm_SOURCES = rm.c
rm_OBJECTS =  $(OBJ_DIR)/rm.$o
rm_LDADD = $(LDADD)
rm_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
rm_LDFLAGS = 
rmdir_SOURCES = rmdir.c
rmdir_OBJECTS =  $(OBJ_DIR)/rmdir.$o
rmdir_LDADD = $(LDADD)
rmdir_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
rmdir_LDFLAGS = 
sync_SOURCES = sync.c
sync_OBJECTS =  $(OBJ_DIR)/sync.$o
sync_LDADD = $(LDADD)
sync_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
sync_LDFLAGS = 
touch_SOURCES = touch.c
touch_OBJECTS =  $(OBJ_DIR)/touch.$o
touch_LDADD = $(LDADD)
touch_DEPENDENCIES =  ../lib/$(OBJ_DIR)/libfu.a
touch_LDFLAGS = 

CFLAGS = -g -O2 ${VERS} $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CERRWARN)
COMPILE = $(CC) $(CFLAGS)
LINK = $(CC) $(LDFLAGS) -o $@
HEADERS =  $(noinst_HEADERS)

DIST_COMMON =  Makefile.am Makefile.in ansi2knr.1 ansi2knr.c

PACKAGE = fileutils
VERSION = 3.16

DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) \
	$(TEXINFOS) $(MANS) $(EXTRA_DIST)

TAR = tar

ifeq ($(OS), Linux)
SOURCES = du.c
X_OBJECTS = du.$o 
OBJECTS = $(addprefix $(OBJ_DIR)/, $(X_OBJECTS))
else
SOURCES = du.c
X_OBJECTS = du.$o
OBJECTS = $(addprefix $(OBJ_DIR)/, $(X_OBJECTS))
endif

LNOPTS = -u -x 
LNLIBS = -L $(DEPTH)/lib/$(OBJ_DIR) -lsam

include $(DEPTH)/mk/targets.mk

default: all

.SUFFIXES:
.SUFFIXES: .c .o ._c ._o .pl
$(srcdir)/Makefile.in: #M# Makefile.am $(top_srcdir)/configure.in $(ACLOCAL) 
	cd $(top_srcdir) && automake --gnits src/Makefile

Makefile: #SUN# $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status

mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	$(NORMAL_INSTALL)
	$(mkinstalldirs) $(bindir)
	@list="$(bin_PROGRAMS)"; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	list="$(bin_PROGRAMS)"; for p in $$list; do \
	  rm -f $(bindir)/`echo $$p|sed '$(transform)'`; \
	done

mostlyclean-libexecPROGRAMS:

clean-libexecPROGRAMS:
	test -z "$(libexec_PROGRAMS)" || rm -f $(libexec_PROGRAMS)

distclean-libexecPROGRAMS:

maintainer-clean-libexecPROGRAMS:

install-libexecPROGRAMS: $(libexec_PROGRAMS)
	$(NORMAL_INSTALL)
	$(mkinstalldirs) $(libexecdir)
	@list="$(libexec_PROGRAMS)"; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(INSTALL_PROGRAM) $$p $(libexecdir)/`echo $$p|sed '$(transform)'`"; \
	     $(INSTALL_PROGRAM) $$p $(libexecdir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-libexecPROGRAMS:
	list="$(libexec_PROGRAMS)"; for p in $$list; do \
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

.c._c:
	$(ANSI2KNR) $< > $*.tmp && mv $*.tmp $@

$(OBJ_DIR)/._c._o:
	@echo "$(COMPILE) -c $<"
	@rm -f _$*.c
	@ln $< _$*.c && $(COMPILE) -c -o $@ _$*.c && mv $(OBJ_DIR)/_$*.o $@ && rm _$*.c

._c.l_o:
	@echo "$(LIBTOOL) --mode=compile $(COMPILE) -c $<"
	@rm -f _$*.c
	@ln $< _$*.c && $(LIBTOOL) --mode=compile $(COMPILE) -c -o $@ _$*.c && mv $(OBJ_DIR)/_$*.lo $@ && rm _$*.c

$(OBJ_DIR)/.c._o:
	$(ANSI2KNR) $< > $*.tmp && mv $*.tmp $*._c
	@echo "$(COMPILE) -c -o $@ $*._c"
	@rm -f _$*.c
	@ln $*._c _$*.c && $(COMPILE) -c -o $@ _$*.c && mv $(OBJ_DIR)/_$*.o $@ && rm _$*.c

$(OBJ_DIR)/.c.l_o:
	$(ANSI2KNR) $< > $*.tmp && mv $*.tmp $*._c
	@echo "$(LIBTOOL) --mode=compile $(COMPILE) -c -o $@ $*._c"
	@rm -f _$*.c
	@ln $*._c _$*.c && $(LIBTOOL) --mode=compile $(COMPILE) -c -o $@ _$*.c && mv $(OBJ_DIR)/_$*.lo $@ && rm _$*.c

mostlyclean-kr:
	rm -rf *._o *.l_o *._c _*.c _*.o $(OBJ_DIR)

clean-kr:

distclean-kr:
	rm -f ansi2knr

maintainer-clean-kr:
$(df_OBJECTS): ../config.h

$(OBJ_DIR)/df: $(df_OBJECTS) $(df_DEPENDENCIES)
	$(LINK) $(df_LDFLAGS) $(df_OBJECTS) $(df_LDADD) $(LIBS)
$(mvdir_OBJECTS): ../config.h

$(OBJ_DIR)/mvdir: $(mvdir_OBJECTS) $(mvdir_DEPENDENCIES)
	$(LINK) $(mvdir_LDFLAGS) $(mvdir_OBJECTS) $(mvdir_LDADD) $(LIBS)
$(chgrp_OBJECTS): ../config.h

$(OBJ_DIR)/chgrp: $(chgrp_OBJECTS) $(chgrp_DEPENDENCIES)
	$(LINK) $(chgrp_LDFLAGS) $(chgrp_OBJECTS) $(chgrp_LDADD) $(LIBS)
$(chown_OBJECTS): ../config.h

$(OBJ_DIR)/chown: $(chown_OBJECTS) $(chown_DEPENDENCIES)
	$(LINK) $(chown_LDFLAGS) $(chown_OBJECTS) $(chown_LDADD) $(LIBS)
$(chmod_OBJECTS): ../config.h

$(OBJ_DIR)/chmod: $(chmod_OBJECTS) $(chmod_DEPENDENCIES)
	$(LINK) $(chmod_LDFLAGS) $(chmod_OBJECTS) $(chmod_LDADD) $(LIBS)
$(cp_OBJECTS): ../config.h

$(OBJ_DIR)/cp: $(cp_OBJECTS) $(cp_DEPENDENCIES)
	$(LINK) $(cp_LDFLAGS) $(cp_OBJECTS) $(cp_LDADD) $(LIBS)
$(dd_OBJECTS): ../config.h

$(OBJ_DIR)/dd: $(dd_OBJECTS) $(dd_DEPENDENCIES)
	$(LINK) $(dd_LDFLAGS) $(dd_OBJECTS) $(dd_LDADD) $(LIBS)
$(dircolors_OBJECTS): ../config.h

$(OBJ_DIR)/dircolors: $(dircolors_OBJECTS) $(dircolors_DEPENDENCIES)
	$(LINK) $(dircolors_LDFLAGS) $(dircolors_OBJECTS) $(dircolors_LDADD) $(LIBS)
$(du_OBJECTS): ../config.h

$(OBJ_DIR)/du: $(du_OBJECTS) $(du_DEPENDENCIES)
	$(LINK) $(du_LDFLAGS) $(du_OBJECTS) $(du_LDADD) $(LIBS)
$(ginstall_OBJECTS): ../config.h

$(OBJ_DIR)/ginstall: $(ginstall_OBJECTS) $(ginstall_DEPENDENCIES)
	$(LINK) $(ginstall_LDFLAGS) $(ginstall_OBJECTS) $(ginstall_LDADD) $(LIBS)
$(ln_OBJECTS): ../config.h

$(OBJ_DIR)/ln: $(ln_OBJECTS) $(ln_DEPENDENCIES)
	$(LINK) $(ln_LDFLAGS) $(ln_OBJECTS) $(ln_LDADD) $(LIBS)
$(dir_OBJECTS): ../config.h

$(OBJ_DIR)/dir: $(dir_OBJECTS) $(dir_DEPENDENCIES)
	$(LINK) $(dir_LDFLAGS) $(dir_OBJECTS) $(dir_LDADD) $(LIBS)
$(vdir_OBJECTS): ../config.h

$(OBJ_DIR)/vdir: $(vdir_OBJECTS) $(vdir_DEPENDENCIES)
	$(LINK) $(vdir_LDFLAGS) $(vdir_OBJECTS) $(vdir_LDADD) $(LIBS)
$(ls_OBJECTS): ../config.h

$(OBJ_DIR)/ls: $(ls_OBJECTS) $(ls_DEPENDENCIES)
	$(LINK) $(ls_LDFLAGS) $(ls_OBJECTS) $(ls_LDADD) $(LIBS)
$(mkdir_OBJECTS): ../config.h

$(OBJ_DIR)/mkdir: $(mkdir_OBJECTS) $(mkdir_DEPENDENCIES)
	$(LINK) $(mkdir_LDFLAGS) $(mkdir_OBJECTS) $(mkdir_LDADD) $(LIBS)
$(mkfifo_OBJECTS): ../config.h

$(OBJ_DIR)/mkfifo: $(mkfifo_OBJECTS) $(mkfifo_DEPENDENCIES)
	$(LINK) $(mkfifo_LDFLAGS) $(mkfifo_OBJECTS) $(mkfifo_LDADD) $(LIBS)
$(mknod_OBJECTS): ../config.h

$(OBJ_DIR)/mknod: $(mknod_OBJECTS) $(mknod_DEPENDENCIES)
	$(LINK) $(mknod_LDFLAGS) $(mknod_OBJECTS) $(mknod_LDADD) $(LIBS)
$(mv_OBJECTS): ../config.h

$(OBJ_DIR)/mv: $(mv_OBJECTS) $(mv_DEPENDENCIES)
	$(LINK) $(mv_LDFLAGS) $(mv_OBJECTS) $(mv_LDADD) $(LIBS)
$(rm_OBJECTS): ../config.h

$(OBJ_DIR)/rm: $(rm_OBJECTS) $(rm_DEPENDENCIES)
	$(LINK) $(rm_LDFLAGS) $(rm_OBJECTS) $(rm_LDADD) $(LIBS)
$(rmdir_OBJECTS): ../config.h

$(OBJ_DIR)/rmdir: $(rmdir_OBJECTS) $(rmdir_DEPENDENCIES)
	$(LINK) $(rmdir_LDFLAGS) $(rmdir_OBJECTS) $(rmdir_LDADD) $(LIBS)
$(sync_OBJECTS): ../config.h

$(OBJ_DIR)/sync: $(sync_OBJECTS) $(sync_DEPENDENCIES)
	$(LINK) $(sync_LDFLAGS) $(sync_OBJECTS) $(sync_LDADD) $(LIBS)
$(touch_OBJECTS): ../config.h

$(OBJ_DIR)/touch: $(touch_OBJECTS) $(touch_DEPENDENCIES)
	$(LINK) $(touch_LDFLAGS) $(touch_OBJECTS) $(touch_LDADD) $(LIBS)

ALL_OBJS = cp-hash._o dd._o df._o ls-ls._o ls-dir._o chmod._o cp-aux._o \
	dircolors._o cp._o chgrp._o mkdir._o install._o mknod._o du._o ln._o \
	mkfifo._o sync._o rmdir._o rm._o ls._o chown._o ls-vdir._o mvdir._o \
	touch._o mv._o 

$(addprefix $(OBJ_DIR)/, $(ALL_OBJS)): $(ANSI2KNR)

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

subdir = src
distdir: $(DISTFILES)
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done
$(OBJ_DIR)/chgrp.$o $(OBJ_DIR)/chgrp.l$o: chgrp.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/xstrtoul.h ../lib/xstrtol.h \
	../lib/error.h
$(OBJ_DIR)/chmod.$o $(OBJ_DIR)/chmod.l$o: chmod.c ../config.h ../lib/getopt.h \
	../lib/modechange.h system.h ../lib/pathmax.h ../lib/error.h
$(OBJ_DIR)/chown.$o $(OBJ_DIR)/chown.l$o: chown.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/error.h
$(OBJ_DIR)/cp-aux.$o $(OBJ_DIR)/cp-aux.l$o: cp-aux.c ../config.h cp.h system.h \
	../lib/pathmax.h ../lib/error.h
$(OBJ_DIR)/cp-hash.$o $(OBJ_DIR)/cp-hash.l$o: cp-hash.c ../config.h cp.h system.h \
	../lib/pathmax.h ../lib/error.h
$(OBJ_DIR)/cp.$o $(OBJ_DIR)/cp.l$o: cp.c ../config.h ../lib/getopt.h cp.h system.h \
	../lib/pathmax.h ../lib/error.h ../lib/backupfile.h \
	../lib/argmatch.h ../lib/path-concat.h
$(OBJ_DIR)/dd.$o $(OBJ_DIR)/dd.l$o: dd.c ../config.h ../lib/getopt.h system.h ../lib/pathmax.h \
	../lib/error.h
$(OBJ_DIR)/df.$o $(OBJ_DIR)/df.l$o: df.c ../config.h ../lib/getopt.h ../lib/mountlist.h \
	../lib/fsusage.h system.h ../lib/pathmax.h ../lib/save-cwd.h \
	../lib/error.h
$(OBJ_DIR)/dircolors.$o $(OBJ_DIR)/dircolors.l$o: dircolors.c ../config.h ../lib/getopt.h \
	system.h ../lib/pathmax.h ../lib/getline.h \
	../lib/long-options.h ../lib/error.h ../lib/obstack.h \
	dircolors.h
$(OBJ_DIR)/du.$o $(OBJ_DIR)/du.l$o: du.c ../config.h ../lib/getopt.h system.h ../lib/pathmax.h \
	../lib/save-cwd.h ../lib/error.h
$(OBJ_DIR)/install.$o $(OBJ_DIR)/install.l$o: install.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/backupfile.h ../lib/modechange.h \
	../lib/makepath.h ../lib/error.h ../lib/xstrtol.h
$(OBJ_DIR)/ln.$o $(OBJ_DIR)/ln.l$o: ln.c ../config.h ../lib/getopt.h system.h ../lib/pathmax.h \
	../lib/backupfile.h ../lib/error.h
$(OBJ_DIR)/ls-dir.$o $(OBJ_DIR)/ls-dir.l$o: ls-dir.c ls.h
$(OBJ_DIR)/ls-ls.$o $(OBJ_DIR)/ls-ls.l$o: ls-ls.c ls.h
$(OBJ_DIR)/ls-vdir.$o $(OBJ_DIR)/ls-vdir.l$o: ls-vdir.c ls.h
$(OBJ_DIR)/ls.$o $(OBJ_DIR)/ls.l$o: ls.c ../config.h ../lib/getopt.h system.h ../lib/pathmax.h \
	../lib/fnmatch.h ../lib/obstack.h ls.h ../lib/error.h \
	../lib/argmatch.h ../lib/xstrtol.h
$(OBJ_DIR)/mkdir.$o $(OBJ_DIR)/mkdir.l$o: mkdir.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/modechange.h ../lib/makepath.h \
	../lib/error.h
$(OBJ_DIR)/mkfifo.$o $(OBJ_DIR)/mkfifo.l$o: mkfifo.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/modechange.h ../lib/error.h
$(OBJ_DIR)/mknod.$o $(OBJ_DIR)/mknod.l$o: mknod.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/modechange.h ../lib/error.h \
	../lib/xstrtol.h
$(OBJ_DIR)/mv.$o $(OBJ_DIR)/mv.l$o: mv.c ../config.h ../lib/getopt.h system.h ../lib/pathmax.h \
	../lib/path-concat.h ../lib/backupfile.h ../lib/error.h
$(OBJ_DIR)/mvdir.$o $(OBJ_DIR)/mvdir.l$o: mvdir.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/save-cwd.h ../lib/error.h
$(OBJ_DIR)/rm.$o $(OBJ_DIR)/rm.l$o: rm.c ../config.h ../lib/getopt.h system.h ../lib/pathmax.h \
	../lib/error.h
$(OBJ_DIR)/rmdir.$o $(OBJ_DIR)/rmdir.l$o: rmdir.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/error.h
$(OBJ_DIR)/sync.$o $(OBJ_DIR)/sync.l$o: sync.c ../config.h system.h ../lib/pathmax.h \
	../lib/long-options.h ../lib/error.h
$(OBJ_DIR)/touch.$o $(OBJ_DIR)/touch.l$o: touch.c ../config.h ../lib/getopt.h system.h \
	../lib/pathmax.h ../lib/error.h ../lib/argmatch.h

info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-binPROGRAMS install-libexecPROGRAMS
	$(NORMAL_INSTALL)

install-data: 
	$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-binPROGRAMS uninstall-libexecPROGRAMS

all: $(ANSI2KNR) $(PROGRAMS) $(BUILT_SOURCES) $(HEADERS) Makefile

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

$(bin_PROGRAMS) $(libexec_PROGRAMS): ../lib/$(OBJ_DIR)/libfu.a

dircolors.h: #M#dcgen dircolors.hin
	./dcgen $(srcdir)/dircolors.hin > $@-t
	mv $@-t $@

.pl:
	rm -f $@ $@.tmp
	$(editpl) $< > $@.tmp && chmod +x $@.tmp && mv $@.tmp $@

# SUN targets:
MODIFIED_SRCS = du.c system.h
SRCS = df.c mvdir.c chgrp.c chown.c chmod.c $(cp_SOURCES) dd.c dircolors.c \
	$(ginstall_SOURCES) ln.c $(dir_SOURCES) $(vdir_SOURCES) $(ls_SOURCES) \
	mkdir.c mkfifo.c mknod.c mv.c rm.c rmdir.c sync.c touch.c \
	ansi2knr.c Makefile.am Makefile.in

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

#
# List any parallel build directory dependencies here
# or .NOTPARALLEL: for sequential directory builds
#
.NOTPARALLEL:

include $(DEPTH)/mk/depend.mk
