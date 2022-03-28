# 开发框架头文件路径
PUBINCL = -I/project/public

# 开发框架cpp文件名，。这里也直接包含进来，没有采用连接库，方便调试
PUBcpp = /project/public/_public.cpp

# 编译参数
CFLAGS = -g

all:procctl checkproc gzipfiles

procctl:procctl.cpp
	g++ -o procctl procctl.cpp
	cp procctl ../bin/.

checkproc:checkproc.cpp
	g++ $(CFLAGS) -o checkproc checkproc.cpp $(PUBINCL) $(PUBcpp) -lm -lc
	cp checkproc ../bin/.

gzipfiles:gzipfiles.cpp
	g++ $(CFLAGS) -o gzipfiles gzipfiles.cpp $(PUBINCL) $(PUBcpp) -lm -lc
	cp gzipfiles ../bin/.

clean:
	rm -f procctl checkproc gzipfiles