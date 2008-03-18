# makefile for example API programs.
# makes both local and remote versions.
#
# These programs are known to compile on Solaris 2.4 using the
# SunPro 3.0.1 compiler.
#

SHELL = /bin/sh
INCDIR = /opt/SUNWsamfs/include
LIBDIR = /opt/SUNWsamfs/lib
RPCLIBDIR = /opt/SUNWsamfs/client/lib

LCLLIBS = -L ${LIBDIR} -l sam -R ${LIBDIR}
RPCLIBS = -L ${RPCLIBDIR} -l samrpc -l nsl -R ${RPCLIBDIR}

all: archive_lcl archive_rem samstat_lcl samstat_rem

archive_lcl: archive.c
	${CC} -g -I${INCDIR} -o $@ $? ${LCLLIBS}

archive_rem: archive.c
	${CC} -g -I${INCDIR} -DREMOTE -o $@ $? ${RPCLIBS}

samstat_lcl: samstat.c
	${CC} -g -I${INCDIR} -o $@ $? ${LCLLIBS}

samstat_rem: samstat.c
	${CC} -g -I${INCDIR} -DREMOTE -o $@ $? ${RPCLIBS}

clean:
	/bin/rm -f archive_lcl archive_rem samstat_lcl samstat_rem

lint:
	${LINT} -u -x -Dlint -I${INCDIR} archive.c
	${LINT} -u -x -Dlint -DREMOTE -I${INCDIR} archive.c
	${LINT} -u -x -Dlint -I${INCDIR} samstat.c
	${LINT} -u -x -Dlint -DREMOTE -I${INCDIR} samstat.c
