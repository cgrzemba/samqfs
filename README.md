# samqfs

This is the source of Sun SamFS released to opensource 2008. The source it adapted to build on Openindiana and OmniOS. Some Storage API's of third party vendors are disabled. samst driver is not needed anymore.

## Setup build environemnt

needed packages:

-    perl
-    developer/gcc-7
-    system/network/avahi
-    developer/build/onbld
-    text/locale
-    bison
-    git, bdb, mariadb, gnu-make, automake

## Customize for illumos and OmniOS

check files in 

    mk/include/omnios.mk
    mk/include/illumos.mk

## Build

    $ gmake -f GNUmakefile

for non debug builds add 'DEBUG=no'  
## Install

    $ gmake -f GNUmakefile install DESTDIR=$(PROTO_DIR)
