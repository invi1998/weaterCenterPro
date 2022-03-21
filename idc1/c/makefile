# 开发框架头文件路径
PUBINCL = -I/project/public

# 开发框架cpp文件名，这里直接包含进来，没有采用链接库，是为了方便调试
PUBCPP = -I/project/public/_public.cpp

# 编译参数
CFLAGS = -g

# 需要编译的程序
all:crtsurfdata1

# 这个程序的编译规则
# 编译命令（在这个程序中使用了开发框架的头文件，所以需要把开发框架的头文件目录给指定出来,然后开发框架的cpp文件我们给他直接包含进来编译）
# 完成编译之后，把它拷贝到bin目录
crtsurfdata1:crtsurfdata1.cpp
	g++ $(CFLAGS) -o crtsurfdata1 crtsurfdata1.cpp $(PUBINCL) $(PUBCPP) -lm -lc
	cp crtsurfdata1 ../bin/.

# 编译的清理
clean:
	rm -f crtsurfdata1
