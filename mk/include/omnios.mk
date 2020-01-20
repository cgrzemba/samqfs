GCC=/opt/gcc-8/bin/gcc
ISA=$(subst /i386,,/$(ISA_TARGET))

DB_INCLUDE=/opt/ooce/include
DB_LIB= -L/opt/ooce/lib$(ISA) -R/opt/ooce/lib$(ISA) -ldb-5.3 

MYSQL_VERSION=mariadb-10.4
MYSQL_ICLUDE=/opt/ooce/$(MYSQL_VERSION)/include
MYSQL_LIB=-L/opt/ooce/$(MYSQL_VERSION)/lib$(ISA) -R/opt/ooce/$(MYSQL_VERSION)/lib$(ISA)
