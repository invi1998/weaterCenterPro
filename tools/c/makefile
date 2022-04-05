# 开发框架头文件路径
PUBINCL = -I/project/public

# 开发框架cpp文件名，。这里也直接包含进来，没有采用连接库，方便调试
PUBcpp = /project/public/_public.cpp

# 编译参数
CFLAGS = -g

all:procctl checkproc gzipfiles deletefiles ftpgetfiles ftpputfiles

procctl:procctl.cpp
	g++ -o procctl procctl.cpp
	cp procctl ../bin/.

checkproc:checkproc.cpp
	g++ $(CFLAGS) -o checkproc checkproc.cpp $(PUBINCL) $(PUBcpp) -lm -lc
	cp checkproc ../bin/.

gzipfiles:gzipfiles.cpp
	g++ $(CFLAGS) -o gzipfiles gzipfiles.cpp $(PUBINCL) $(PUBcpp) -lm -lc
	cp gzipfiles ../bin/.

deletefiles:deletefiles.cpp
	g++ $(CFLAGS) -o deletefiles deletefiles.cpp $(PUBINCL) $(PUBcpp) -lm -lc
	cp deletefiles ../bin/.

# ftp模块的编译指令需要包含ftp库文件和_ftp.cpp这两个文件一起编译
# 如果用静态库链接程序需要加入-no-pie这个参数，不然就要用动态库链接程序生产PIE格式的程序。
ftpgetfiles:ftpgetfiles.cpp
	g++ $(CFLAGS) -o ftpgetfiles ftpgetfiles.cpp $(PUBINCL) $(PUBcpp) /project/public/libftp.a /project/public/_ftp.cpp -lm -lc -no-pie
	cp ftpgetfiles ../bin/.

ftpputfiles:ftpputfiles.cpp
	g++ $(CFLAGS) -o ftpputfiles ftpputfiles.cpp $(PUBINCL) $(PUBcpp) /project/public/libftp.a /project/public/_ftp.cpp -lm -lc -no-pie
	cp ftpputfiles ../bin/.

clean:
	rm -f procctl checkproc gzipfiles deletefiles ftpgetfiles ftpputfiles