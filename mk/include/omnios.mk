GCC=/opt/gcc-14/bin/gcc

DB_INCLUDE=/opt/ooce/include
DB_LIB= -L/opt/ooce/lib$(ISA) -R/opt/ooce/lib$(ISA) -ldb-5.3 

MYSQL_VERSION=mariadb-10.8
MYSQL_INCLUDE=-I/opt/ooce/$(MYSQL_VERSION)/include/mysql
MYSQL_LIB=-L/opt/ooce/$(MYSQL_VERSION)/lib$(ISA) -R/opt/ooce/$(MYSQL_VERSION)/lib$(ISA) -lmysqlclient

# OS-3294 add inotify support
OSDEPCFLAGS += -D_INOTIFY

CTFCONVERT = /opt/onbld/bin/i386/ctfconvert
CTFMERGE = /opt/onbld/bin/i386/ctfmerge
