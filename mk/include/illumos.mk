MYSQL_VERSION=mariadb/10.6
MYSQL_INCLUDE=-I/usr/$(MYSQL_VERSION)/include/
MYSQL_LIB=-L/usr/$(MYSQL_VERSION)/lib$(ISA) -R/usr/$(MYSQL_VERSION)/lib$(ISA) -lmysqlclient

CERRWARN += -Wno-unused-variable -Wno-unused-function -Wno-unused-label
CTFCONVERT = /opt/onbld/bin/i386/ctfconvert
CTFMERGE = /opt/onbld/bin/i386/ctfmerge
