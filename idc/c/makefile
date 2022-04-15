# 开发框架头文件路径
PUBINCL = -I/project/public

# 开发框架cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试
PUBCPP = /project/public/_public.cpp

# 指定mysql的头文件存放路径
MYSQLINCL = -I/usr/local/mysql/include -I/project/public/db/mysql

# mysql库文件的保存路径
MYSQLLIB = -L/usr/local/mysql/lib

# mysql链接库
MYSQLLIBS = -lmysqlclient

# 开发框架的mysql的cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试
MYSQLCPP = /project/public/db/mysql/_mysql.cpp

# 编译参数
CFLAGS = -g

# 需要编译的程序
all:crtsurfdata obtcodetodb obtmindtodb

# 这个程序的编译规则
# 编译命令（在这个程序中使用了开发框架的头文件，所以需要把开发框架的头文件目录给指定出来,然后开发框架的cpp文件我们给他直接包含进来编译）
# 完成编译之后，把它拷贝到bin目录
crtsurfdata:crtsurfdata.cpp
	g++ $(CFLAGS) -o crtsurfdata crtsurfdata.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp crtsurfdata ../bin/.

obtcodetodb:obtcodetodb.cpp
	g++ $(CFLAGS) -o obtcodetodb obtcodetodb.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) -lm -lc
	cp obtcodetodb ../bin/.

obtmindtodb:obtmindtodb.cpp idcapp.h idcapp.cpp
	g++ $(CFLAGS) -o obtmindtodb obtmindtodb.cpp $(PUBINCL) $(PUBCPP) $(MYSQLINCL) $(MYSQLLIB) $(MYSQLLIBS) $(MYSQLCPP) idcapp.cpp -lm -lc
	cp obtmindtodb ../bin/.

# 编译的清理
clean:
	rm -f crtsurfdata obtcodetodb obtmindtodb
