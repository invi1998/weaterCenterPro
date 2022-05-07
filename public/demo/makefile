all: demo1 demo2 demo4 demo5 demo7 demo8 demo10 demo12 demo16 demo18 demo20 demo21\
     demo22 demo24 demo26 demo28 demo29 demo30 demo32 demo34 demo36 demo37 demo39 demo40\
     demo42 demo43 demo45 demo47 demo48 demo50 demo51 demo52

demo:demo.cpp
	g++ -Wall -g -o demo demo.cpp ../_public.cpp -lpthread

demo1:demo1.cpp
	g++ -Wall -g -o demo1 demo1.cpp ../_public.cpp -lpthread

demo2:demo2.cpp
	g++ -g -o demo2 demo2.cpp ../_public.cpp -lpthread

demo4:demo4.cpp
	g++ -g -o demo4 demo4.cpp ../_public.cpp -lpthread

demo5:demo5.cpp
	g++ -g -o demo5 demo5.cpp ../_public.cpp -lpthread

demo7:demo7.cpp
	g++ -g -o demo7 demo7.cpp ../_public.cpp -lpthread

demo8:demo8.cpp
	g++ -g -o demo8 demo8.cpp ../_public.cpp -lpthread

demo10:demo10.cpp
	g++ -g -o demo10 demo10.cpp ../_public.cpp -lpthread

demo12:demo12.cpp
	g++ -g -o demo12 demo12.cpp ../_public.cpp -lpthread

demo16:demo16.cpp
	g++ -g -o demo16 demo16.cpp ../_public.cpp -lpthread

demo18:demo18.cpp
	g++ -g -o demo18 demo18.cpp ../_public.cpp -lpthread

demo20:demo20.cpp
	g++ -g -o demo20 demo20.cpp ../_public.cpp -lpthread

demo21:demo21.cpp
	g++ -g -o demo21 demo21.cpp ../_public.cpp -lpthread

demo22:demo22.cpp
	g++ -g -o demo22 demo22.cpp ../_public.cpp -lpthread

demo24:demo24.cpp
	g++ -g -o demo24 demo24.cpp ../_public.cpp -lpthread

demo26:demo26.cpp
	g++ -g -o demo26 demo26.cpp ../_public.cpp -lpthread

demo28:demo28.cpp
	g++ -g -o demo28 demo28.cpp ../_public.cpp -lpthread

demo29:demo29.cpp
	g++ -g -o demo29 demo29.cpp ../_public.cpp -lpthread

demo30:demo30.cpp
	g++ -g -o demo30 demo30.cpp ../_public.cpp -lpthread

demo32:demo32.cpp
	g++ -g -o demo32 demo32.cpp ../_public.cpp -lpthread

demo34:demo34.cpp
	g++ -g -o demo34 demo34.cpp ../_public.cpp -lpthread

demo36:demo36.cpp
	g++ -g -o demo36 demo36.cpp ../_public.cpp -lpthread

demo37:demo37.cpp
	g++ -g -o demo37 demo37.cpp ../_public.cpp -lpthread

demo39:demo39.cpp
	g++ -g -o demo39 demo39.cpp ../_public.cpp -lpthread

demo40:demo40.cpp
	g++ -g -o demo40 demo40.cpp ../_public.cpp -lpthread

demo42:demo42.cpp
	g++ -g -o demo42 demo42.cpp ../_public.cpp -lpthread

demo43:demo43.cpp
	g++ -g -o demo43 demo43.cpp ../_public.cpp -lpthread

demo45:demo45.cpp
	g++ -g -o demo45 demo45.cpp ../_public.cpp -lpthread

demo47:demo47.cpp
	g++ -g -o demo47 demo47.cpp ../_public.cpp -lpthread

demo48:demo48.cpp
	g++ -g -o demo48 demo48.cpp ../_public.cpp -lpthread

demo50:demo50.cpp
	cd .. && gcc -c -o libftp.a ftplib.c
	cd .. && gcc -fPIC -shared -o libftp.so ftplib.c
	g++ -g -o demo50 demo50.cpp ../_public.cpp -lpthread ../_ftp.cpp ../libftp.a -lm -lc

demo51:demo51.cpp
	g++ -g -o demo51 demo51.cpp ../_public.cpp -lpthread ../_ftp.cpp ../libftp.a -lm -lc

demo52:demo52.cpp
	g++ -g -o demo52 demo52.cpp ../_public.cpp -lpthread ../_ftp.cpp ../libftp.a -lm -lc

clean:
	rm -f demo1 demo2 demo4 demo5 demo7 demo8 demo10 demo12 demo16 demo18 demo20 demo21
	rm -f demo22 demo24 demo26 demo28 demo29 demo30 demo32 demo34 demo36 demo37 demo39 
	rm -f demo40 demo42 demo43 demo45 demo47 demo48 demo50 demo51 demo52
