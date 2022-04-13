# mysql头文件存放的目录。
MYSQLINCL = -I/usr/local/mysql/include

# mysql库文件存放的目录。
MYSQLLIB = -L/usr/local/mysql/lib

# 需要链接的mysql库。
MYSQLLIBS = -lmysqlclient

CFLAGS=-g -Wno-write-strings

all:createtable

createtable:createtable.cpp _mysql.h _mysql.cpp
	g++ $(CFLAGS) -o createtable createtable.cpp $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) _mysql.cpp -lm -lc

clean:
	rm -rf createtable
