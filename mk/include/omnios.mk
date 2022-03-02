GCC=/opt/gcc-10/bin/gcc
ISA=$(subst /i386,,/$(ISA_TARGET))

DB_INCLUDE=/opt/ooce/include
DB_LIB= -L/opt/ooce/lib$(ISA) -R/opt/ooce/lib$(ISA) -ldb-5.3 

MYSQL_VERSION=mariadb-10.4
MYSQL_INCLUDE=-I/opt/ooce/$(MYSQL_VERSION)/include/mysql
MYSQL_LIB=-L/opt/ooce/$(MYSQL_VERSION)/lib$(ISA) -R/opt/ooce/$(MYSQL_VERSION)/lib$(ISA) -lmysqlclient

# OS-3294 add inotify support
OSDEPCFLAGS += -D_INOTIFY

SSL_INCLUDES = -I/usr/ssl-1.0/include
SSL_LIBDIR = -L/usr/ssl-1.0/lib -R/usr/ssl-1.0/lib

