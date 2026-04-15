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

Run the following commands in the root directory of source tre

Replace or set the shell variables, e.g. for OmniOS:

    os_release="i386-omnios-r151054"
    samqfs_publisher="samqfs.omnios"

or OpenIndiana:

    os_release="i386-illumos-fd4b009ae1"
    samqfs_publisher="samqfs.omnios"

Build the whole commponent

    $ gmake -f GNUmakefile

for debug builds add 'DEBUG=yes'  
## Install

    $ gmake -f GNUmakefile install DESTDIR=$(PROTO_DIR)


## Build IPS package
if not already exist, create IPS repository, eg:

    $ mkdir -p repo/${os_release}
    $ pkgrepo -s repo/${os_release} create
    $ pkgrepo -s repo/${os_release} add-publisher ${samqfs_publisher}

then create package

    $ gmake -f GNUmakefile pkg [REPO=../repo/${os_release}]

you can use an repository on different location with setting variable REPO:

    $ gmake -f GNUmakefile pkg REPO=/<other-repo-path>

for rebuild the package, remove the cookie file pkg/.packaged
