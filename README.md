# samqfs

This is the source of Sun SamFS released to opensource 2008. The source it adapted to build on Openindiana and OmniOS. Some Storage API's of third party vendors are disabled. samst driver is not needed anymore.

## Setup build environemnt

needed packages:

-    developer/gcc-14
-    developer/build/onbld
-    text/locale
-    bison
-    git, bdb, mariadb, gnu-make, automake

## Customize for illumos and OmniOS

check files for used versions

    mk/include/omnios.mk
    mk/include/illumos.mk

## Build

    $ gmake -f GNUmakefile

for debug builds add 'DEBUG=yes'  
## Install

    $ gmake -f GNUmakefile install DESTDIR=$(PROTO_DIR)

for build IPS package

    $ gmake -f GNUmakefile pkg

