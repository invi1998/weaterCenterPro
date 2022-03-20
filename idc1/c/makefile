

# 需要编译的程序
all:crtsurfdata1

# 这个程序的编译规则
crtsurfdata1:crtsurfdata1.cpp
# 编译命令（在这个程序中使用了开发框架的头文件，所以需要把开发框架的头文件目录给指定出来,然后开发框架的cpp文件我们给他直接包含进来编译）
	g++ -g -o crtsurfdata1 crtsurfdata1.cpp -I/project/public /project/public/_public.cpp -lm -lc
# 完成编译之后，把它拷贝到bin目录
	cp crtsurfdata1 ../bin/.

# 编译的清理
clean:
	rm crtsurfdata1
