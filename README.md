# samqfs

This is the source of Sun SamFS released to opensource 2009. The source it adapted to build on Openindiana and OmniOS. Some Storage API's of third party vendors are disabled. samst driver is not needed anymore.

## Setup build environemnt

needed packages:

-    perl-522 perl-526
-    developer/gcc-7
-    system/network/avahi
-    developer/build/onbld
-    developer/java/openjdk8
-    text/locale
-    bison
-    git, bdb, mariadb, gnu-make, automake, swig

## Customize for illumos and OmniOS

check files in 

    mk/include/omnios.mk
    mk/include/illumos.mk

## Build

    $ gmake -f GNUmakefile
